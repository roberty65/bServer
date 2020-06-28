/* Message.cpp
 * Copyright by Beyondy.c.w 2002-2020
**/
#include <string.h>

#include "Message.h"

namespace beyondy {
namespace Async {

Message *Message::create(int fd, int flow)
{
	// TODO: pool-create
	Message *msg = new Message(fd, flow);
	return msg;
}

Message *Message::create(size_t bsize, int fd, int flow)
{
	// TODO: pool-create
	Message *msg = new Message(bsize, fd, flow);
	return msg;
}

void Message::destroy(Message *msg)
{
	// TODO: pool-release
	delete msg;
}

Message::Message(int _fd, int _flow) : MemoryBuffer((unsigned char *)extra, sizeof extra, false), 
	fd(_fd), flow(_flow), ioCount(0)
{
	init();
}

Message::Message(size_t bsize, int _fd, int _flow) : MemoryBuffer(bsize),
	fd(_fd), flow(_flow), ioCount(0)
{
	init();
}

Message::Message(unsigned char *buf, size_t bsize, int _fd, int _flow) : MemoryBuffer(buf, bsize, false),
	fd(_fd), flow(_flow), ioCount(0)
{
	init();
}

Message::~Message()
{
	/* nothing */
}

void Message::init()
{
	memset(&ts_byte_first, 0, sizeof ts_byte_first);	
	memset(&ts_enqueue, 0, sizeof ts_enqueue);	
	memset(&ts_dequeue, 0, sizeof ts_dequeue);	
	memset(&ts_process_end, 0, sizeof ts_process_end);
	memset(extra, 0, sizeof extra);
	attached = NULL;
}

} /* Async */
} /* beyondy */

