#pragma once

/*
 * Sequential stream endpoint for testing
 */

#include "api.h"
#include "api_str_seq.h"
#include "min_max.h"

#include <string.h>

#define CHUNK_SZ 256

static inline void api_str_seq_get_bytes(struct api_str_seq_ep* e, uint8_t* buff, unsigned sz)
{
	uint8_t n = e->next_n;
	for (; sz; --sz, ++buff) {
		*buff = n;
		if (++n > e->max_n)
			n = 0;
	}
}

static inline void api_str_seq_advance(struct api_str_seq_ep* e, unsigned sz)
{
	e->next_n = (e->next_n + sz) % (e->max_n + 1);
}

static void api_str_seq_rd(struct api* api, struct api_ep* ep)
{
	struct api_str_seq_ep* e = (struct api_str_seq_ep*)ep;
	if (!api->ep_ctx) {
		api->ep_ctx = &api->ep_ctx_buf;
		api->pkt.str_rd.remain_len = ~0;
		api_op_responde(api, api->pkt.str_rd.max_len);
	}
	BUG_ON(!api->pkt.data_len);
	uint8_t buff[CHUNK_SZ];
	unsigned chunk = MIN_(CHUNK_SZ, api->pkt.data_len);
	api_str_seq_get_bytes(e, buff, chunk);
	uint16_t sz = api->com->put(buff, chunk);
	api->pkt.data_len -= sz;
	api_str_seq_advance(e, sz);
	if (!api->pkt.data_len)
		api_op_complete(api);
}

void api_str_seq_ep_init(struct api_str_seq_ep* e, const char* descr, uint8_t max_n)
{
	memset(e, 0, sizeof(*e));
	e->max_n = max_n;
	e->ep.descr = descr;
	e->ep.buf_sz = 0;
	e->ep.str_rd = api_str_seq_rd;
}

