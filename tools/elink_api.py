import time
import struct
from collections import namedtuple
from elink_serial import *

import logging
log = logging.getLogger("serial.api")

#
# Operations
#
api_op_info    = 0    # retrieve endpoint descriptor
api_op_reg_rd  = 1    # register read
api_op_reg_wr  = 2    # register write
api_op_buf_rd  = 4    # buffer read
api_op_buf_wr  = 8    # buffer write
api_op_str_rd  = 16   # stream read
api_op_str_wr  = 32   # stream write
api_op_resp    = 0x80 # set in response
#
# Response flags
#
                      # Errors
api_fl_err_ep  = 1    # invalid endpoint
api_fl_err_op  = 2    # invalid operation
api_fl_err_sz  = 4    # invalid size parameter
api_fl_err_app = 8    # application specific error
api_fl_err_    = 0xf  # fatal errors mask
                      # Notifications
api_fl_str_ov  = 0x10 # stream overflow notification

#
# Predefined endpoints
#
API_EP_SYS  = 0
API_EP_SEQ  = 254
API_EP_ECHO = 255

API_EP_SEQ_MAX = 250

#
# Protocol related definitions
#
hdr_fmt = '<BBBBIIHH'
hdr_size = struct.calcsize(hdr_fmt)
assert hdr_size == 16

sys_descr_fmt = '<IHHI16s32s8I'
sys_descr_size = struct.calcsize(sys_descr_fmt)
assert sys_descr_size == 92

ApiPktHdr = namedtuple(
	'ApiPktHdr', 
	(
		'op',
		'ep',
		'flags',
		'sn',
		'param1',
		'param2',
		'reserved',
		'data_len'
	)
)

ApiEPInfo = namedtuple(
	'ApiEPInfo',
	(
		'buf_sz',
		'ops',
		'descr',
	)
)

SystemDescr = namedtuple(
	'SystemDescr',
	(
		'magic',
		'prod_id',
		'version',
		'caps',
		'vendor',
		'prod_name',
		'ep_list'
	)
)

api_magic = 0x7565c240

class ELinkApiException(IOError):
    """API exception class"""

class ELinkApi:
	def __init__(self, com):
		self.com = com
		self.last_sn = 0
		self.sys_descr = None

	def __str__(self):
		return str(self.com)
	def __repr__(self):
		return 'ELinkApi(' +  self.com.name + ')'

	@staticmethod
	def mk_packet(op, ep, sn, param1=0, param2=0, data=''):
		"""Build packet from provided header fields and data payload"""
		return struct.pack(hdr_fmt, op, ep, 0, sn, param1, param2, 0, len(data)) + data

	@staticmethod
	def decode_hdr(str):
		"""Parse header and returns tuple filled with fields"""
		fields = struct.unpack(hdr_fmt, str)
		return ApiPktHdr(*fields)

	def wr_packet(self, op, ep, param1=0, param2=0, data=''):
		"""Build and send packet to device"""
		self.last_sn += 1
		self.last_sn &= 0xff
		pkt = self.mk_packet(op, ep, self.last_sn, param1, param2, data)
		self.com.write(pkt)

	def send_packet(self, op, ep, param1=0, param2=0, data=''):
		"""Build and send packet then receive response. Returns parsed response header followed by payload data"""
		self.wr_packet(op, ep, param1, param2, data)
		hdr = self.com.read(hdr_size)
		h  = self.decode_hdr(hdr)
		if h.sn != self.last_sn:
			raise ELinkApiException("sequence number mismatch")
		if h.flags & api_fl_err_ep:
			raise ELinkApiException("invalid endpoint")
		if h.flags & api_fl_err_op:
			raise ELinkApiException("invalid operation")
		if h.flags & api_fl_err_sz:
			raise ELinkApiException("invalid size")
		if h.flags & api_fl_err_app:
			raise ELinkApiException("application error")
		if h.flags & api_fl_str_ov:
			log.warn('stream #%d overflow' % ep)
		data = self.com.read(h.data_len)
		return h, data

	def rd_ep_info(self, ep):
		"""Read and parse endpoint info from device"""
		h, data = self.send_packet(api_op_info, ep, 0, 0)
		return ApiEPInfo(h.param1, h.param2, data)

	@staticmethod
	def	ep_ops2string(ops):
		"""Convert endpoint operations mask to string representation"""
		if ops & (api_op_reg_rd|api_op_reg_wr):
			return 'REG'
		if ops & (api_op_buf_rd|api_op_buf_wr):
			name = 'BUF_'
			if ops & api_op_buf_rd:
				name += 'R'
			if ops & api_op_buf_wr:
				name += 'W'
			return name
		if ops & (api_op_str_rd|api_op_str_wr):
			name = 'STR_'
			if ops & api_op_str_rd:
				name += 'R'
			if ops & api_op_str_wr:
				name += 'W'
			return name
		return ''

	def rd_buff(self, ep, off, sz):
		"""Read from specified buffer endpoint"""
		h, data = self.send_packet(api_op_buf_rd, ep, off, sz)
		if len(data) != sz:
			raise ELinkApiException("protocol error")
		return data

	def rd_stream(self, ep, sz):
		"""Read stream from specified EP. Return chunk of data and remaining length."""
		assert sz <= max_buff
		h, data = self.send_packet(api_op_str_rd, ep, 0, sz)
		return data, h.param1

	def rd_stream_all(self, ep, wait_tout=None):
		"""Read all data available in the specified stream endpoint"""
		data, deadline = None, None
		while True:
			buff, remaining = self.rd_stream(ep, max_buff)
			if data is None:
				data = buff
			else:
				data += buff
			if remaining:
				continue
			if data:
				return data
			if deadline and time.clock() > deadline:
				return None
			if not wait_tout:
				return None
			deadline = time.clock() + wait_tout

	def wr_stream(self, ep, data):
		"""Write chunk of stream data to specified EP. Returns the number of bytes actually written."""
		assert len(data) <= max_buff
		h, _ = self.send_packet(api_op_str_wr, ep, 0, 0, data)
		return h.param1
		
	def rd_system_descr(self):
		"""Read system descriptor"""
		epbm = [0]*8
		d = struct.unpack(sys_descr_fmt, self.rd_buff(API_EP_SYS, 0, sys_descr_size))
		ep_bmap, ep_list = d[6:], []
		for i in range(256):
			if ep_bmap[i/32] & (1 << (i%32)):
				ep_list.append(i)
		return SystemDescr(d[0], d[1], d[2], d[3], d[4].rstrip('\0'), d[5].rstrip('\0'), ep_list)

	def get_system_descr(self):
		"""Read system descriptor or return the cached one"""
		if self.sys_descr is None:
			self.sys_descr = self.rd_system_descr()
		return self.sys_descr

	def verify(self, prod_id):
		"""Verify device and check its product id matches the one passed as parameter"""
		descr = self.get_system_descr()
		if descr.magic != api_magic:
			return False
		if descr.prod_id != prod_id:
			return False
		log.debug("%s", self.info())
		return True

	def info(self):
		"""Return printable system info"""
		descr = self.get_system_descr()
		return '%s %s v.%x' % (descr.vendor, descr.prod_name, descr.version)

	def flush(self):
		"""Flush communication interface"""
		self.com.write(chr(api_op_resp)*max_write)
		try:
			flush_rd = self.com.read(max_read)
		except IOError:
			pass


def elink_api_open(what, prod_id, flush=True):
	"""Open API given the device ID and VID/PID of USB port"""
	for com in elink_open_all(what):
		try:
			api = ELinkApi(com)
			if flush:
				api.flush()
			if api.verify(prod_id):
				return api
		except IOError:
			log.debug('failed to initialize %s', com)
	return None
