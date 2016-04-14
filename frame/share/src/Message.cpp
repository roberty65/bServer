/* Message.cpp
 * Copyright by Beyondy.c.w 2002-2020
**/
#include <malloc.h>
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
	/* nothing */
}

Message::Message(size_t bsize, int _fd, int _flow) : MemoryBuffer(bsize),
	fd(_fd), flow(_flow), ioCount(0)
{
	/* nothing */
}

Message::Message(unsigned char *buf, size_t bsize, int _fd, int _flow) : MemoryBuffer(buf, bsize, false),
	fd(_fd), flow(_flow), ioCount(0)
{
	/* nothing */
}

Message::~Message()
{
	/* nothing */
}

} /* Async */
} /* beyondy */

