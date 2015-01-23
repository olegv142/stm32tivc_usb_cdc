#include "api.h"
#include "api_str_buf.h"
#include "build_bug_on.h"
#include "min_max.h"
#include <string.h>

struct api_str_buf_ctx {
	uint16_t len;
};

BUILD_BUG_ON(sizeof(struct api_str_buf_ctx) > EP_CTX_SZ);

static void api_str_buf_rd(struct api* api, struct api_ep* ep)
{
	struct api_str_buf_ep* e = (struct api_str_buf_ep*)ep;
	struct api_str_buf_ctx* ctx;
	if (!api->ep_ctx) {
		api->ep_ctx = &api->ep_ctx_buf;
		ctx = api->ep_ctx;
		uint32_t avail = e->data_size - e->read_size;
		ctx->len = MIN_(avail, api->pkt.str_rd.max_len);
		api->pkt.str_rd.remain_len = avail - ctx->len;
		if (e->ov) {
			// Pass overflow flag to application
			api->pkt.h.flags |= api_fl_str_ov;
			e->ov = false;
		}
		api_op_responde(api, ctx->len);
	} else {
		ctx = api->ep_ctx;
		BUG_ON(!ctx->len);
	}
	uint16_t sz = api->com->put(e->buf + e->read_size, ctx->len);
	e->read_size += sz;
	ctx->len -= sz;
	if (!ctx->len)
		api_op_complete(api);
}

static void api_str_buf_wr(struct api* api, struct api_ep* ep)
{
	struct api_str_buf_ep* e = (struct api_str_buf_ep*)ep;
	struct api_str_buf_ctx* ctx;
	if (!api->ep_ctx) {
		api->ep_ctx = &api->ep_ctx_buf;
		ctx = api->ep_ctx;
		if (!api_str_buf_ep_is_unread(e))
			e->data_size = e->read_size = 0;
		uint32_t avail = e->ep.buf_sz - e->data_size;
		ctx->len = MIN_(avail, api->pkt.data_len);
		api->pkt.str_wr.written_len = ctx->len;
	} else {
		ctx = api->ep_ctx;
		BUG_ON(!api->pkt.data_len);
		BUG_ON(api->pkt.data_len < ctx->len);
	}
	if (ctx->len) {
		uint16_t sz = api->com->get(e->buf + e->data_size, ctx->len);
		e->data_size += sz;
		ctx->len -= sz;
		api->pkt.data_len -= sz;
	} else {
		// Just skip data received
		uint16_t sz = api->com->get(0, api->pkt.data_len);
		api->pkt.data_len -= sz;
	}
	if (!api->pkt.data_len)
		api_op_complete(api);
}

void api_str_buf_ep_init(struct api_str_buf_ep* e, const char* descr, void* buf, uint32_t sz, bool can_rd, bool can_wr)
{
	memset(e, 0, sizeof(*e));
	e->buf = (uint8_t*)buf;
	e->ep.descr = descr;
	e->ep.buf_sz = sz;
	if (can_rd)
		e->ep.str_rd = api_str_buf_rd;
	if (can_wr)
		e->ep.str_wr = api_str_buf_wr;
}
