# Enumerate registered USB devices on windows host

import _winreg as reg

def cdc_enum(vid, pid):
	"""Returns the list of port names for the giver VID/PID"""
	k, i = None, 0
	ports = []
	try:
		k = reg.OpenKey(reg.HKEY_LOCAL_MACHINE, "SYSTEM\CurrentControlSet\Enum\USB\Vid_%04x&Pid_%04x" % (vid, pid))
		while True:
			sk = None
			dev = reg.EnumKey(k, i)
			i += 1
			try:
				sk = reg.OpenKey(k, dev + '\Device Parameters')
				v  = reg.QueryValueEx(sk, 'PortName')
				if v[1] == reg.REG_SZ:
					ports.append(v[0].encode('ascii'))
			except WindowsError:
				pass
			finally:
				if sk is not None: reg.CloseKey(sk)

	except WindowsError:
		pass
	finally:
		if k is not None: reg.CloseKey(k)

	return ports

def stm32_cdc_enum():
	return cdc_enum(0x0483, 0x5740)
