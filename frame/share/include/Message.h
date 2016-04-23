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

#define TV2MS(t1,t2) (((t2).tv_sec - (t1).tv_sec) * 1000 + ((t2).tv_usec - (t1).tv_usec) / 1000)

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
private:
	void init();
public:	
	int fd;
	int flow;
	
	// TIMESTAMPS FOR A MESSAGE
	// in message
	//    ts_byte_first 
	//    ts_enqueue[ts_byte_last]
	//    [QUEUEU]
	//    ts_dequeue[ts_process_start]
	//    [PROCESS]
	//    ts_process_end(BUSINESS's job)
	//
	// out message
	//    ts_enqueue
	//    [QUEUE]
	//    ts_dequeue, out SYSTEM-QUEUE, in CONNECTION's QUEUE
	//    ts_byte_first when writable, start to send out
	//    ts_process_end[ts_byte_last]: onSent
	//
	int ioCount;	/* when IN, how many times read is called, when out, how many write is called */
	struct timeval ts_byte_first;
	struct timeval ts_enqueue, ts_dequeue;
	struct timeval ts_process_end;

	long extra[10];
	void *attached;	/* user-attached data */
};

} /* Async */
} /* beyondy */

#endif /* __MESSAGE__H */

