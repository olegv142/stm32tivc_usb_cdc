/**
  ******************************************************************************
  * @file    usbd_cdc_vcp.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   Generic media access Layer.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED 
#pragma     data_alignment = 4 
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_vcp.h"
#include "usbd_cdc_core.h"
#include "stm32f4xx_conf.h"

#include "min_max.h"
#include "ring_buff.h"
#include "atomic.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
LINE_CODING linecoding =
  {
    115200, /* baud rate*/
    0x00,   /* stop bits-1*/
    0x00,   /* parity - none*/
    0x08    /* nb. of bits 8*/
  };


/* Private function prototypes -----------------------------------------------*/
static uint16_t VCP_Init     (void);
static uint16_t VCP_DeInit   (void);
static uint16_t VCP_Ctrl     (uint32_t Cmd, uint8_t* Buf, uint32_t Len);

CDC_IF_Prop_TypeDef VCP_fops = 
{
  VCP_Init,
  VCP_DeInit,
  VCP_Ctrl,
  0,
  0
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  VCP_Init
  *         Initializes the Media on the STM32
  * @param  None
  * @retval Result of the opeartion (USBD_OK in all cases)
  */
static uint16_t VCP_Init(void)
{
  return USBD_OK;
}

/**
  * @brief  VCP_DeInit
  *         DeInitializes the Media on the STM32
  * @param  None
  * @retval Result of the opeartion (USBD_OK in all cases)
  */
static uint16_t VCP_DeInit(void)
{
  return USBD_OK;
}


/**
  * @brief  VCP_Ctrl
  *         Manage the CDC class requests
  * @param  Cmd: Command code            
  * @param  Buf: Buffer containing command data (request parameters)
  * @param  Len: Number of data to be sent (in bytes)
  * @retval Result of the opeartion (USBD_OK in all cases)
  */
static uint16_t VCP_Ctrl (uint32_t Cmd, uint8_t* Buf, uint32_t Len)
{ 
  switch (Cmd)
  {
  case SEND_ENCAPSULATED_COMMAND:
    /* Not  needed for this driver */
    break;

  case GET_ENCAPSULATED_RESPONSE:
    /* Not  needed for this driver */
    break;

  case SET_COMM_FEATURE:
    /* Not  needed for this driver */
    break;

  case GET_COMM_FEATURE:
    /* Not  needed for this driver */
    break;

  case CLEAR_COMM_FEATURE:
    /* Not  needed for this driver */
    break;

  case SET_LINE_CODING:
	/* Not  needed for this driver */ 
    break;

  case GET_LINE_CODING:
    Buf[0] = (uint8_t)(linecoding.bitrate);
    Buf[1] = (uint8_t)(linecoding.bitrate >> 8);
    Buf[2] = (uint8_t)(linecoding.bitrate >> 16);
    Buf[3] = (uint8_t)(linecoding.bitrate >> 24);
    Buf[4] = linecoding.format;
    Buf[5] = linecoding.paritytype;
    Buf[6] = linecoding.datatype; 
    break;

  case SET_CONTROL_LINE_STATE:
    /* Not  needed for this driver */
    break;

  case SEND_BREAK:
    /* Not  needed for this driver */
    break;    
    
  default:
    break;
  }

  return USBD_OK;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/*
 * VCP data Rx/Tx API
 */

void VCP_MarkRead(unsigned sz)
{
	atomic_t a = atomic_begin();
	USB_Rx_buff_tail = ring_wrap(USB_Rx_buff_size, USB_Rx_buff_tail + sz);
	atomic_end(a);
}

void VCP_MarkWritten(unsigned sz)
{
	atomic_t a = atomic_begin();
	USB_Tx_buff_head = ring_wrap(USB_TX_BUFF_SIZE, USB_Tx_buff_head + sz);
	atomic_end(a);
}

unsigned VCP_DataAvail(void)
{
	atomic_t a = atomic_begin();
	unsigned sz = ring_data_avail(USB_Rx_buff_size, USB_Rx_buff_head, USB_Rx_buff_tail);
	atomic_end(a);
	return sz;
}

unsigned VCP_SpaceAvail(void)
{
	atomic_t a = atomic_begin();
	unsigned sz = ring_space_avail(USB_TX_BUFF_SIZE, USB_Tx_buff_head, USB_Tx_buff_tail);
	atomic_end(a);
	return sz;
}

unsigned VCP_DataAvailContig(void)
{
	atomic_t a = atomic_begin();
	unsigned sz = ring_data_contig(USB_Rx_buff_size, USB_Rx_buff_head, USB_Rx_buff_tail);
	atomic_end(a);
	return sz;
}

unsigned VCP_SpaceAvailContig(void)
{
	atomic_t a = atomic_begin();
	unsigned sz = ring_space_contig(USB_TX_BUFF_SIZE, USB_Tx_buff_head, USB_Tx_buff_tail);
	atomic_end(a);
	return sz;
}

unsigned VCP_GetContig(void* buff, unsigned max_len)
{
	unsigned avail = VCP_DataAvailContig();
	unsigned sz = MIN_(avail, max_len);
	if (sz) {
		memcpy(buff, VCP_DataPtr(), sz);
		VCP_MarkRead(sz);
	}
	return sz;
}

unsigned VCP_PutContig(void const* buff, unsigned len)
{
	unsigned avail = VCP_SpaceAvailContig();
	unsigned sz = MIN_(avail, len);
	if (sz) {
		memcpy(VCP_SpacePtr(), buff, sz);
		VCP_MarkWritten(sz);
	}
	return sz;
}

unsigned VCP_Get(void* buff, unsigned max_len)
{
	unsigned sz = VCP_GetContig(buff, max_len);
	if (sz && (max_len -= sz))
		sz += VCP_GetContig((uint8_t*)buff + sz, max_len);
	return sz;
}

unsigned VCP_Put(void const* buff, unsigned len)
{
	unsigned sz = VCP_PutContig(buff, len);
	if (sz && (len -= sz))
		sz += VCP_PutContig((uint8_t*)buff + sz, len);
	return sz;
}

static unsigned total_echo_bytes = 0;

/* Copy as much input data to free output space as possible */
void VCP_Echo(void)
{
	for (;;) {
		unsigned avail_data  = VCP_DataAvailContig();
		unsigned avail_space = VCP_SpaceAvailContig();
		unsigned sz = MIN_(avail_data, avail_space);
		if (!sz)
			break;

		memcpy(VCP_SpacePtr(), VCP_DataPtr(), sz);
		total_echo_bytes += sz;
		VCP_MarkWritten(sz);
		VCP_MarkRead(sz);
	}
}
