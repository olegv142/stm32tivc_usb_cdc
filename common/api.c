#include "api.h"
#include "api_buf.h"
#include "bug_on.h"
#include "app_info.h"
#include <string.h>

static inline uint8_t current_op(struct api* api)
{
	return api->pkt.h.op & ~api_op_resp;
}

static inline int is_write_op(uint8_t op)
{
	switch (op) {
	case api_op_reg_wr:
	case api_op_buf_wr:
	case api_op_str_wr:
		return 1;
	default:
		return 0;
	}
}

static inline int is_data_write_op(uint8_t op)
{
	switch (op) {
	case api_op_buf_wr:
	case api_op_str_wr:
		return 1;
	default:
		return 0;
	}
}

void api_init(struct api* api, struct api_com_cb const* com)
{
	memset(api, 0, sizeof(*api));
	api->com = com;
	api->state = api_idle;
	api->sys_descr.magic = API_MAGIC;
	api->sys_descr.prod_id = PROD_ID;
	api->sys_descr.version = VERSION;
	api->sys_descr.caps = CAPS;
	strncpy((char*)api->sys_descr.vendor,    VENDOR,    sizeof(api->sys_descr.vendor));
	strncpy((char*)api->sys_descr.prod_name, PROD_NAME, sizeof(api->sys_descr.prod_name));
	api_buf_ep_init(&api->ep_sys, "System descriptor", &api->sys_descr, sizeof(api->sys_descr), true, false);
	api_str_seq_ep_init(&api->ep_seq, "Test sequence 0..250", 250);
	api_str_buf_ep_init(&api->ep_echo, "Echo", &api->echo_buf, sizeof(api->echo_buf), true, true);
	api_ep_register(api, &api->ep_sys.ep,  API_EP_SYS);
	api_ep_register(api, &api->ep_seq.ep,  API_EP_SEQ);
	api_ep_register(api, &api->ep_echo.ep, API_EP_ECHO);
}

void api_ep_register(struct api* api, struct api_ep* ep, uint8_t epn)
{
	BUG_ON(api->ep[epn]);
	api->ep[epn] = ep;
	api->sys_descr.ep_bmap[epn / 8] |= 1 << (epn % 8);
}

static inline struct api_ep* api_get_ep(struct api* api)
{
	return api->ep[api->pkt.h.ep];
}

struct api_ep_info_ctx {
	uint32_t off;
	uint16_t len;	
};

static void api_ep_info_cb(struct api* api, struct api_ep* ep)
{
	struct api_ep_info_ctx* ctx;
	if (!api->ep_ctx) {
		api->ep_ctx = &api->ep_ctx_buf;
		ctx = api->ep_ctx;
		api->pkt.info.buf_sz = ep->buf_sz;
		api->pkt.info.ops = api_op_info;
		if (ep->reg_rd)
			api->pkt.info.ops |= api_op_reg_rd;
		if (ep->reg_wr)
			api->pkt.info.ops |= api_op_reg_wr;
		if (ep->buf_rd)
			api->pkt.info.ops |= api_op_buf_rd;
		if (ep->buf_wr)
			api->pkt.info.ops |= api_op_buf_wr;
		if (ep->str_rd)
			api->pkt.info.ops |= api_op_str_rd;
		if (ep->str_wr)
			api->pkt.info.ops |= api_op_str_wr;
		ctx->off = 0;
		ctx->len = strlen(ep->descr);
		api_op_responde(api, ctx->len);
	} else {
		ctx = api->ep_ctx;
		BUG_ON(!ctx->len);
	}
	uint16_t sz = api->com->put(ep->descr + ctx->off, ctx->len);
	ctx->off += sz;
	ctx->len -= sz;
	if (!ctx->len)
		api_op_complete(api);
}

static inline api_ep_cb api_get_ep_cb(struct api* api, struct api_ep* ep)
{
	switch (current_op(api)) {
	case api_op_info:
		return api_ep_info_cb;
	case api_op_reg_rd:
		return ep->reg_rd;
	case api_op_reg_wr:
		return ep->reg_wr;
	case api_op_buf_rd:
		return ep->buf_rd;
	case api_op_buf_wr:
		return ep->buf_wr;
	case api_op_str_rd:
		return ep->str_rd;
	case api_op_str_wr:
		return ep->str_wr;
	default:
		return 0;
	}
}

static inline void api_call_ep(struct api* api)
{
	struct api_ep* ep = api_get_ep(api);
	BUG_ON(!ep);
	api_ep_cb cb = api_get_ep_cb(api, ep);
	BUG_ON(!cb);
	cb(api, ep);
}

static void api_start_ep_req(struct api* api)
{
	if (is_data_write_op(current_op(api)) != (api->pkt.data_len != 0)) {
		api_op_complete_err(api, api_fl_err_sz);
		return;
	}
	struct api_ep* ep = api_get_ep(api);
	if (!ep) {
		api_op_complete_err(api, api_fl_err_ep);
		return;
	}
	api_ep_cb cb = api_get_ep_cb(api, ep);
	if (!cb) {
		api_op_complete_err(api, api_fl_err_op);
		return;
	}
	api->ep_ctx = 0;
	cb(api, ep);
}

static bool api_skip_invalid_bytes(struct api* api)
{
	int i;
	// skip bytes if response flag is set
	for (i = 0; api->valid_sz && (api->pkt_buf[i] & api_op_resp); ++i) {
		--api->valid_sz;
	}
	if (i) {
		memmove(api->pkt_buf, api->pkt_buf + i, api->valid_sz);
		return true;
	}
	return false;
}

void api_process(struct api* api, uint16_t has_data, uint16_t has_space)
{
	if (has_space < sizeof(api->pkt)) {
		return;
	}
	if (api->state == api_idle)
	{
		BUG_ON(api->valid_sz >= sizeof(api->pkt));
		if (has_data + api->valid_sz < sizeof(api->pkt))
			return;
		api->valid_sz += api->com->get(api->pkt_buf + api->valid_sz, sizeof(api->pkt) - api->valid_sz);
		BUG_ON(api->valid_sz != sizeof(api->pkt));
		if (api_skip_invalid_bytes(api))
			return;
		api->state = api_processing;
		api_start_ep_req(api);
	} else {
		BUG_ON(api->valid_sz != sizeof(api->pkt));
		if (is_write_op(current_op(api)) && !has_data)
			return;
		api_call_ep(api);
	}
}

void api_op_responde(struct api* api, uint16_t data_len)
{
	BUG_ON(api->state != api_processing);
	api->pkt.h.op |= api_op_resp;
	api->pkt.data_len = data_len;
	api->pkt.reserved = 0;
	uint16_t sz = api->com->put(&api->pkt, sizeof(api->pkt));
	BUG_ON(sz != sizeof(api->pkt));
	api->state = api_responding;	
}

void api_op_complete(struct api* api)
{
	BUG_ON(api->state == api_idle);
	if (api->state != api_responding)
		api_op_responde(api, 0);
	api->state = api_idle;
	api->valid_sz = 0;
}

void api_op_complete_err(struct api* api, uint8_t err_flags)
{
	BUG_ON(api->state != api_processing);
	api->pkt.h.flags |= err_flags;
	api_op_complete(api);
}
