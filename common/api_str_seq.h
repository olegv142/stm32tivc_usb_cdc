#pragma once

/*
 * Sequential stream endpoint for testing.
 * Generate periodic sequence 0..max_n.
 */

#include "api_ep.h"

struct api_str_seq_ep {
	struct api_ep ep;
	uint8_t max_n;
	uint8_t next_n;
};

void api_str_seq_ep_init(struct api_str_seq_ep* e, const char* descr, uint8_t max_n);
