#pragma once

/*
 * Buffer endpoint implementation
 */

#include "api_ep.h"
#include <stdbool.h>

struct api_buf_ep {
	struct api_ep ep;
	uint8_t* buf;
};

void api_buf_ep_init(struct api_buf_ep* e, const char* descr, void* buf, uint32_t sz, bool can_rd, bool can_wr);
