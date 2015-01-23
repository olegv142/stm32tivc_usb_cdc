#include "usb_api.h"

void main(void)
{
	usb_api_init();
	for (;;) {
		usb_api_process();
	}
}
