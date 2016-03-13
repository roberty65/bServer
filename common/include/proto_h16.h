#ifndef __PROTO__H16__H
#define __PROTO__H16__H

#include <stdint.h>

struct proto_h16_head {	// 16 bytes
	uint32_t len;	// 00
	uint16_t cmd;	// 04
	uint16_t ver;	// 06
	uint32_t syn;	// 08
	uint32_t ack;	// 12
};

struct proto_h16_res : public proto_h16_head {
	int32_t ret_;
};

#endif /*! __PROTO__H16__H */
