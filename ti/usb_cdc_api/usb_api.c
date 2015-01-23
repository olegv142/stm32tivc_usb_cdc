#include "usb_api.h"
#include "usb_serial_structs.h"
#include "usb_dev_serial.h"
#include "min_max.h"

/*
 * The API over USB implementation
 */

__no_init struct api g_api;

static uint16_t usb_api_get_data(void* buff, uint16_t max_len)
{
	if (buff) {
		return USBBufferRead(&g_sRxBuffer, buff, max_len);
	} else {
		uint16_t avail = USBBufferDataAvailable(&g_sRxBuffer);
		uint16_t skip = MIN_(avail, max_len);
		USBBufferDataRemoved(&g_sRxBuffer, skip);
		return skip;
	}
}

static uint16_t usb_api_put_data(void const* buff, uint16_t len)
{
	return USBBufferWrite(&g_sTxBuffer, buff, len);
}

static struct api_com_cb g_api_com = {
	usb_api_get_data,
	usb_api_put_data
};

void usb_api_init(void)
{
	USBDeviceInit();
	api_init(&g_api, &g_api_com);
}

void usb_api_process(void)
{
	api_process(&g_api, USBBufferDataAvailable(&g_sRxBuffer), USBBufferSpaceAvailable(&g_sTxBuffer));
}


