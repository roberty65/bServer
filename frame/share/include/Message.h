/* Message.h
 * Copyright by Beyondy.c.w 2002-2020
 *
**/
#ifndef __MESSAGE__H
#define __MESSAGE__H

#include <sys/types.h>
#include <sys/time.h>
#include <stdint.h>

#include "MemoryBuffer.h"

namespace beyondy {
namespace Async {

class Message : public MemoryBuffer {
public:
	static Message *create(int fd, int flow);	
	static Message *create(size_t bsize, int fd, int flow);
	static void destroy(Message *msg);
public:
	Message(int fd, int flow);
	Message(size_t bsize, int fd, int flow);
	Message(unsigned char *buf, size_t bsize, int fd = -1, int flow = -1);
	~Message();
public:	
	int fd;
	int flow;
	
	int ioCount;	/* when IN, how many times read is called, when out, how many write is called */
	struct timeval ts_byte_first, ts_byte_byte_last;
	struct timeval ts_enqueue, ts_dequeue;

	struct timeval ts_process_start, ts_process_end;
#define ts_connection_start ts_process_start	/* for out-message in connector-xxx */
#define ts_connection_end ts_process_end

	long extra[10];
	void *attached;	/* user-attached data */
};

} /* Async */
} /* beyondy */

#endif /* __MESSAGE__H */

