#pragma once

/*
 * Buffer with stream interface endpoint implementation.
 * We prefer it to the ring version since the contiguous space
 * is more simpler to manage.
 */

#include "api_ep.h"
#include <stdbool.h>
#include "bug_on.h"

struct api_str_buf_ep {
	struct api_ep ep;
	uint8_t* buf;
	uint32_t data_size;
	uint32_t read_size;
	bool ov;
};

void api_str_buf_ep_init(struct api_str_buf_ep* e, const char* descr, void* buf, uint32_t sz, bool can_rd, bool can_wr);

static inline bool api_str_buf_ep_is_unread(struct api_str_buf_ep* e)
{
	BUG_ON(e->data_size < e->read_size);
	return e->data_size != e->read_size;
}

/* Function to be called by data producer */
static inline void api_str_buf_ep_on_new_data(struct api_str_buf_ep* e, uint32_t sz)
{
	BUG_ON(api_str_buf_ep_is_unread(e));
	BUG_ON(sz > e->ep.buf_sz);
	e->data_size = sz;
	e->read_size = 0;
}

/*
 * The following functions may be used by data producer to signal overflow - the
 * situation when the new data is ready but the previous data was not read yet.
 */
static inline void api_str_buf_ep_signal_overflow(struct api_str_buf_ep* e)
{
	e->ov = true;
}
