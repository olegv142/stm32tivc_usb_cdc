#pragma once

/*
 * Packet based API low level protocol
 */

#include "build_bug_on.h"
#include <stdint.h>

// Operation
enum {
	api_op_info    = 0,  // retrieve endpoint descriptor
	api_op_reg_rd  = 1,  // register read
	api_op_reg_wr  = 2,  // register write
	api_op_buf_rd  = 4,  // buffer read
	api_op_buf_wr  = 8,  // buffer write
	api_op_str_rd  = 16, // stream read
	api_op_str_wr  = 32, // stream write
	api_op_resp    = 0x80, // set in response
	// the request with response bit set is ignored, so the sequence of bytes with response bit
	// set will flush receiver provided that the sequence length exceeds max message length
};

// Response flags
enum {
	// Errors
	api_fl_err_ep  = 1,    // invalid endpoint
	api_fl_err_op  = 2,    // invalid operation
	api_fl_err_sz  = 4,    // invalid size parameter
	api_fl_err_app = 8,    // application specific error
	api_fl_err_    = 0xf,  // fatal errors mask
	// Notifications
	api_fl_str_ov  = 0x10, // stream overflow notification
};

// Packet header
struct api_pkt_hdr {
	uint8_t	op;    // Operation requested
	uint8_t ep;    // Target endpoint number
	uint8_t flags; // Response flags
	uint8_t sn;    // Sequence number
};

// The common packet structure optionally followed by payload data (if data_len != 0)
struct api_pkt {
	struct api_pkt_hdr h;
	union {
		uint32_t param[2];
		struct {
			uint32_t buf_sz; // buffer size
			uint8_t  ops;    // valid operation mask
			uint8_t  reserved[3];
		} info;
		struct {
			// Register may be set with bit granularity
			uint32_t val;
			uint32_t mask;
		} reg;
		struct {
			// Region of interest within buffer address space
			uint32_t off;
			uint16_t len;
			uint16_t reserved;
		} buf;
		struct {
			uint32_t remain_len; // remaining data length on response
			uint16_t max_len;    // max data length to read
			uint16_t reserved;
		} str_rd;
		struct {
			uint32_t written_len; // written data length on response
			uint32_t reserved;
		} str_wr;
	};
	uint16_t reserved;
	uint16_t data_len; // The length of the data that follows
};

BUILD_BUG_ON(sizeof(struct api_pkt) != 16);

//
// Predefined endpoints
//
enum {
	API_EP_SYS  = 0,
	API_EP_SEQ  = 254,
	API_EP_ECHO = 255,
};

#define API_EP_SEQ_MAX 250

struct api_sys_descr {
	// Returned by API_EP_SYS endpoint
#define API_MAGIC 0x7565c240
	uint32_t magic;
	uint16_t prod_id;
	uint16_t version;
	// Application specific capability bits
	uint32_t caps;
	// Zero-terminated strings
	uint8_t  vendor[16];
	uint8_t  prod_name[32];
	// Bitmap of valid API endpoints
	uint8_t  ep_bmap[256/8];
};

