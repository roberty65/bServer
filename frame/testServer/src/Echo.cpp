#include <unistd.h>

#include "proto_h16.h"
#include "Block.h"
#include "Message.h"
#include "Echo.h"
#include "Log.h"

using namespace beyondy::Async;

int Echo::onInit()
{
	maxInputSize = 50 * 1024 * 1024;
	maxOutputSize = 50 * 1024 * 1024;

	return 0;
}

void Echo::onExit()
{
	/* nothing */
}

int Echo::onMessage(beyondy::Async::Message *req)
{
	SYSLOG_DEBUG("Echo got a message from fd=%d flow=%d msize=%d", \
		req->fd, req->flow, req->wptr);

	if (req->wptr >= (long)(sizeof(struct proto_h16_head) + sizeof(int))) {
		int takenMs = *((int *)(req->data() + sizeof(struct proto_h16_head)));	
		if (takenMs > 0) usleep((long)takenMs*1000);
	}

	Message *rsp = Message::create(req->wptr, req->fd, req->flow);
	memcpy(rsp->data(), req->data(), req->wptr);

	/* set response size */
	rsp->wptr = req->wptr;

	Message::destroy(req);
	int retval = sendMessage(rsp);

	return retval;
}

int Echo::onSent(Message *msg, int status)
{
	SYSLOG_DEBUG("Echo message from fd=%d flow=%d msize=%d sent status=%d", \
		msg->fd, msg->flow, msg->wptr, status);
	Message::destroy(msg);
	return 0;
}

extern "C" Processor* createProcessor()
{
	return new Echo();
}
