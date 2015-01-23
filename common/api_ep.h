#pragma once

/*
 * Generic API endpoint
 */

#include <stdint.h>

struct api;
struct api_ep;

typedef void (*api_ep_cb)(struct api*, struct api_ep*);

struct api_ep {
	const char* descr;
	uint32_t  buf_sz;
	api_ep_cb reg_rd;
	api_ep_cb reg_wr;
	api_ep_cb buf_rd;
	api_ep_cb buf_wr;
	api_ep_cb str_rd;
	api_ep_cb str_wr;
};

