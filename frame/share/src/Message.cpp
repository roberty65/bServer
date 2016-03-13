#include <malloc.h>
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

Message::~Message()
{
	/* nothing */
}

int Message::resize(size_t nsize)
{
	return dblk.resize(nsize, wptr);
}

} /* Async */
} /* beyondy */

