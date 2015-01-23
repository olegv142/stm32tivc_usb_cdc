from elink_serial import *
log = logging.getLogger("serial")

max_xfer_sz = max_write

def elink_echo_test(s):
	"""Test open serial communication channel"""
	import random
	txi, rxi, n = 0, 0, 0
	while True:
		sz = random.randrange(1, max_xfer_sz)
		buff = bytearray(sz)
		for j in range(sz):
			buff[j] = (txi ^ (txi >> 8)) & 0xff
			txi += 1
		s.write(buff)
		while sz > 0:
			rsz = random.randrange(1, sz + 1)
			rbuff = s.read(rsz)
			assert len(rbuff) == rsz
			for j in range(rsz):
				assert ord(rbuff[j]) == ((rxi ^ (rxi >> 8)) & 0xff)
				rxi += 1
			sz -= rsz
		assert txi == rxi
		n += 1
		if n % 100 == 0:
			print '*',

board = 'stm'

if __name__ == '__main__':
	logging.basicConfig()
	log.setLevel(logging.INFO)
	s = elink_open(board)
	if s:
		elink_echo_test(s)
	else:
		print 'device not found'


