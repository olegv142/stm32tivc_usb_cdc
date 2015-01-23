#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usb_conf.h"
#include "usbd_cdc_vcp.h"
#include "usbd_desc.h"
#include "usb_api.h"
#include "min_max.h"

/*
 * The API over USB implementation
 */

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */

__ALIGN_BEGIN USB_OTG_CORE_HANDLE USB_OTG_dev __ALIGN_END ;

__no_init struct api g_api;

static uint16_t usb_api_get_data(void* buff, uint16_t max_len)
{
	if (buff) {
		return VCP_Get(buff, max_len);
	} else {
		uint16_t avail = VCP_DataAvail();
		uint16_t skip = MIN_(avail, max_len);
		VCP_MarkRead(skip);
		return skip;
	}
}

static uint16_t usb_api_put_data(void const* buff, uint16_t len)
{
	return VCP_Put(buff, len);
}

static struct api_com_cb g_api_com = {
	usb_api_get_data,
	usb_api_put_data
};

void usb_api_init(void)
{
	USBD_Init(&USB_OTG_dev,
#ifdef USE_USB_OTG_HS 
		USB_OTG_HS_CORE_ID,
#else            
		USB_OTG_FS_CORE_ID,
#endif  
		&USR_desc, 
		&USBD_CDC_cb, 
		&USR_cb
	);

	api_init(&g_api, &g_api_com);
}

void usb_api_process(void)
{
	api_process(&g_api, VCP_DataAvail(), VCP_SpaceAvail());
}


