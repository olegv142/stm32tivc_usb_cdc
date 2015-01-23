import elink_api
import random
import sys
import logging
log = logging.getLogger("serial")

def echo_test(api):
	"""Test API given instance by sending and receiving random data to/from echo stream endpoint"""
	print 'testing echo endpoint'
	api.rd_stream_all(elink_api.API_EP_ECHO)
	n = 0
	while True:
		sz = random.randrange(1, elink_api.max_buff)
		buff = bytearray(sz)
		for j in range(sz):
			buff[j] = random.randrange(0, 256)
		written = api.wr_stream(elink_api.API_EP_ECHO, buff)
		assert written <= sz
		read = ''
		while True:
			sz = random.randrange(1, elink_api.max_buff)
			data, remain = api.rd_stream(elink_api.API_EP_ECHO, sz)
			read += data
			if not remain:
				break
		assert buff[:written] == read
		n += 1
		if n % 100 == 0:
			print '*',

def seq_test(api):
	"""Test API given instance by sending and receiving random data to/from echo stream endpoint"""
	print 'testing sequential stream endpoint'
	n, last_byte = 0, None
	while True:
		sz = random.randrange(1, elink_api.max_buff)
		data, _ = api.rd_stream(elink_api.API_EP_SEQ, sz)
		assert data
		for c in data:
			byte = ord(c)
			if last_byte is None:
				last_byte = byte
			else:
				last_byte += 1
				if last_byte > elink_api.API_EP_SEQ_MAX:
					last_byte = 0
				assert last_byte == byte
		n += 1
		if n % 100 == 0:
			print '*',

board = 'stm'

def test():
	logging.basicConfig()
	log.setLevel(logging.WARN)
	api = elink_api.elink_api_open(board, 0xbeee)
	if api is not None:
		print api, api.info()
		for epn in api.get_system_descr().ep_list:
			epi = api.rd_ep_info(epn)
			print '#%d\t[%d]\t%s\t%s' % (epn, epi.buf_sz, api.ep_ops2string(epi.ops), epi.descr)
		if len(sys.argv) > 1:
			if sys.argv[1] == '255':
				echo_test(api)
			if sys.argv[1] == '254':
				seq_test(api)
	else:
		print 'device not found'

if __name__ == '__main__':
	test()
