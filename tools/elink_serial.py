from cdc_detect import cdc_enum
import serial
from serial import SerialException
import logging
log = logging.getLogger("serial.stream")
import time

# TI board VID/PID
ti_vid = 0x1cbe
ti_vcp_pid = 2

# STM32 board VID/PID
stm_vid = 0x0483
stm_vcp_pid = 0x5740

com_timeout = .5
max_buff  = 2*4096
max_read  = max_buff + 1024
max_write = max_read

class ELinkSerial:
	"""Serial communication channel"""
	def __init__(self, com, name):
		self.com = com
		self.name = name
	def __str__(self):
		return '[' +  self.name + ']'
	def __repr__(self):
		return 'ELinkSerial(' +  self.name + ')'
	def write(self, str):
		self.com.write(str)
	def read(self, sz):
		buff, sz_ = '', sz
		start_ts = None
		while sz_ > 0:
			chunk = self.com.read(sz_)
			chunk_sz = len(chunk)
			assert 0 <= chunk_sz <= sz_
			if chunk_sz == sz:
				return chunk
			if start_ts is None:
				start_ts = time.clock()
			elif not chunk_sz and time.clock() > start_ts + com_timeout:
				raise SerialException("failed to read %d bytes from %s" % (sz_, str(self)))
			buff += chunk
			sz_ -= chunk_sz
		return buff

def elink_open_port(port):
	"""Open specified serial port"""
	try:
		com = serial.Serial(port, timeout=com_timeout, writeTimeout=com_timeout)
	except IOError:
		log.debug('failed to open %s', port)
		return None
	if not com.isOpen():
		log.debug('failed to open %s', port)
		return None
	log.info("connected to %s", port)
	if not serial.win32.SetupComm(com.hComPort, max_read, max_write):
		log.debug('failed to setup bufferring for %s', port)
	return com

def elink_open_all_(vid, pid):
	"""Open all ports with matching vid, pid"""
	ports = cdc_enum(vid, pid)
	for port in ports:
		com = elink_open_port(port)
		if com is not None:
			yield ELinkSerial(com, port)

def elink_open_(vid, pid):
	"""Open first with matching vid, pid"""
	for com in elink_open_all_(vid, pid):
		return com

def elink_open_all(what):
	if what == 'ti':
		return elink_open_all_(ti_vid, ti_vcp_pid)
	elif what == 'stm':
		return elink_open_all_(stm_vid, stm_vcp_pid);
	else:
		return elink_open_all_(what[0], what[1])

def elink_open(what):
	if what == 'ti':
		return elink_open_(ti_vid, ti_vcp_pid)
	elif what == 'stm':
		return elink_open_(stm_vid, stm_vcp_pid);
	else:
		return elink_open_(what[0], what[1])
