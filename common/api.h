#pragma once

/*
 * Generic API message handler
 */

#include "api_pkt.h"
#include "api_ep.h"
#include "api_buf.h"
#include "api_str_seq.h"
#include "api_str_buf.h"

struct api;
struct api_ep;

typedef uint16_t (*api_get_data_cb)(void* buff, uint16_t max_len);
typedef uint16_t (*api_put_data_cb)(void const* buff, uint16_t len);

// Communication callbacks
struct api_com_cb {
	api_get_data_cb get;
	api_put_data_cb put;
};

typedef enum {
	api_idle,       // waiting for header
	api_processing, // packet header is received
	api_responding, // packet response header is sent
} api_state_t;

#define EP_CTX_SZ 64
#define ECHO_BUF_SZ 256

struct api {
	struct api_com_cb const* com;
	union {
		struct api_pkt pkt;
		uint8_t        pkt_buf[sizeof(struct api_pkt)];
	};
	uint8_t               valid_sz;
	api_state_t           state;
	void*                 ep_ctx;
	uint8_t               ep_ctx_buf[EP_CTX_SZ];
	struct api_buf_ep     ep_sys;
	struct api_str_seq_ep ep_seq;
	struct api_str_buf_ep ep_echo;
	struct api_sys_descr  sys_descr;
	uint8_t               echo_buf[ECHO_BUF_SZ];
	struct api_ep*        ep[256];
};

void api_init(struct api* api, struct api_com_cb const* com);
void api_ep_register(struct api* api, struct api_ep* ep, uint8_t epn);
void api_process(struct api* api, uint16_t has_data, uint16_t has_space);
void api_op_responde(struct api* api, uint16_t data_len);
void api_op_complete(struct api* api);
void api_op_complete_err(struct api* api, uint8_t err_flags);

