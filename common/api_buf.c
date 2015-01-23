#include "api.h"
#include "api_buf.h"
#include "bug_on.h"
#include "build_bug_on.h"
#include <string.h>

struct api_buf_ctx {
	uint32_t off;
	uint16_t len;
};

BUILD_BUG_ON(sizeof(struct api_buf_ctx) > EP_CTX_SZ);

static void api_buf_rd(struct api* api, struct api_ep* ep)
{
	struct api_buf_ctx* ctx;
	if (!api->ep_ctx) {
		api->ep_ctx = &api->ep_ctx_buf;
		ctx = api->ep_ctx;
		ctx->off = api->pkt.buf.off;
		ctx->len = api->pkt.buf.len;
		if (ctx->off + ctx->len > ep->buf_sz) {
			api_op_complete_err(api, api_fl_err_sz);
			return;
		}
		api_op_responde(api, ctx->len);
	} else {
		ctx = api->ep_ctx;
		BUG_ON(!ctx->len);
	}
	struct api_buf_ep* e = (struct api_buf_ep*)ep;
	uint16_t sz = api->com->put(e->buf + ctx->off, ctx->len);
	ctx->off += sz;
	ctx->len -= sz;
	if (!ctx->len)
		api_op_complete(api);
}

static void api_buf_wr(struct api* api, struct api_ep* ep)
{
	struct api_buf_ctx* ctx;
	if (!api->ep_ctx) {
		api->ep_ctx = &api->ep_ctx_buf;
		ctx = api->ep_ctx;
		ctx->off = api->pkt.buf.off;
		ctx->len = api->pkt.buf.len;
		if (ctx->off + ctx->len > ep->buf_sz || api->pkt.data_len != ctx->len) {
			api_op_complete_err(api, api_fl_err_sz);
			return;
		}
	} else {
		ctx = api->ep_ctx;
		BUG_ON(!ctx->len);
	}
	struct api_buf_ep* e = (struct api_buf_ep*)ep;
	uint16_t sz = api->com->get(e->buf + ctx->off, ctx->len);
	ctx->off += sz;
	ctx->len -= sz;
	if (!ctx->len)
		api_op_complete(api);
}

void api_buf_ep_init(struct api_buf_ep* e, const char* descr, void* buf, uint32_t sz, bool can_rd, bool can_wr)
{
	memset(e, 0, sizeof(*e));
	e->buf = (uint8_t*)buf;
	e->ep.descr = descr;
	e->ep.buf_sz = sz;
	if (can_rd)
		e->ep.buf_rd = api_buf_rd;
	if (can_wr)
		e->ep.buf_wr = api_buf_wr;
}
