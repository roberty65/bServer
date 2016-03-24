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

Message::Message(int _fd, int _flow) : dblk((unsigned char *)extra, sizeof extra, false), 
	rptr(0), wptr(0), fd(_fd), flow(_flow), ioCount(0)
{
	/* nothing */
}

Message::Message(size_t bsize, int _fd, int _flow) : dblk(bsize), 
	rptr(0), wptr(0), fd(_fd), flow(_flow), ioCount(0)
{
	/* nothing */
}

Message::Message(unsigned char *buf, size_t bsize, int _fd, int _flow) : dblk(buf, bsize, false),
	rptr(0), wptr(0), fd(_fd), flow(_flow), ioCount(0)
{
	/* nothing */
}

Message::~Message()
{
	/* nothing */
}

int Message::readString(char *buf, size_t size)
{
	uint32_t len = 0;
	if (readUint32(len) < 0) return - 1;

	if (len < size) size = len;
	memcpy(buf, (char *)data() + rptr, len);
	buf[size - 1] = 0; 

	rptr += len;
	return 0; 
}

int Message::writeString(const char *buf, size_t size)
{
	if (buf == NULL) size = 0;
	else if (size == 0) size = strlen(buf) + 1;

	if (writeUint32((uint32_t)size) < 0) return -1;
	if (wptr + size <= dblk.dsize()) {
		memcpy(data() + wptr, buf, size); 
		wptr += size;
		return 0;
	}

	return -1;
}

int Message::resize(size_t nsize)
{
	return dblk.resize(nsize, wptr);
}

} /* Async */
} /* beyondy */

