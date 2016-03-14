#include <unistd.h>

#include "proto_h16.h"
#include "Block.h"
#include "Message.h"
#include "Echo.h"
#include "Log.h"

#define ECHO_CMD_ECHO_REQ	0
#define ECHO_CMD_ECHO_RSP	1

#define ECHO_CMD_FORWARD_REQ	2
#define ECHO_CMD_FORWARD_RSP	3

#define ECHO_CMD_MORE_REQ	4
#define ECHO_CMD_MORE_RSP	5

using namespace beyondy::Async;
extern "C" void appCallback(const char *);

int Echo::onInit()
{
//	void *dl = dlopen(NULL, RTLD_NOW|RTLD_GLOBAL);
//	void (*cb)(const char *) = (void (*)(const char*))dlsym(dl, "appCallback");
//	if (cb != NULL) {
//		(*cb)("hello, get u!!!");
//	}
//	NOT WORK!!!
	
	appCallback("hello, this is Echo onInit");

	maxInputSize = 50 * 1024 * 1024;
	maxOutputSize = 50 * 1024 * 1024;

	// TODO: use more general log
	// after compile the frame with -rdynamic, lib can use tis symbols
	// SYSLOG_INIT("../logs/business.log", LOG_LEVEL_DEBUG, 10*1024*1024, 10);
	return 0;
}

void Echo::onExit()
{
	/* nothing */
}

int Echo::doEcho(Message *req)
{
	int takenMs = 0;
	if (req->wptr >= (long)(sizeof(struct proto_h16_head) + sizeof(int))) {
		takenMs = *((int *)(req->data() + sizeof(struct proto_h16_head)));	
	}

	SYSLOG_DEBUG("Echo took %dms", takenMs);
	if (takenMs > 0) usleep((long)takenMs*1000);

	Message *rsp = Message::create(req->wptr, req->fd, req->flow);
	memcpy(rsp->data(), req->data(), req->wptr);

	struct proto_h16_head *h = (struct proto_h16_head *)rsp->data();
	h->cmd = h->cmd + 1;

	/* set response size */
	rsp->wptr = req->wptr;

	Message::destroy(req);
	return sendMessage(rsp);
}

int Echo::doForward(Message *req)
{
	struct proto_h16_head *h = (struct proto_h16_head *)req->data();

	Message *rsp = Message::create(req->wptr, req->fd, req->flow);
	memcpy(rsp->data(), req->data(), req->wptr);

	if (req->wptr >= (long)(sizeof(struct proto_h16_head) + 3 * sizeof(int))) {
		if (h->cmd == ECHO_CMD_FORWARD_REQ) {
			*(int *)(rsp->data() + sizeof(struct proto_h16_head) + 0) = *(int *)(req->data() + sizeof(struct proto_h16_head));
			*(int *)(rsp->data() + sizeof(struct proto_h16_head) + sizeof(int)) = req->flow;
			*(int *)(rsp->data() + sizeof(struct proto_h16_head) + 2* sizeof(int)) = req->fd;
			SYSLOG_DEBUG("Forward from fd=%d flow=%d size=%ld to fd=%d flow=%d", req->fd, req->flow, (long)req->wptr, 5, 1);

			struct proto_h16_head *h = (struct proto_h16_head *)rsp->data();
			h->cmd = ECHO_CMD_MORE_REQ;

			rsp->fd = 5;
			rsp->flow = 1;
		}
		else {
			int oldFlow = *(int *)(req->data() + sizeof(struct proto_h16_head) + sizeof(int));
			int oldFd = *(int *)(req->data() + sizeof(struct proto_h16_head) + 2 * sizeof(int));
			SYSLOG_DEBUG("Forward result from fd=%d flow=%d size=%ld replied to fd=%d flow=%d", \
				req->fd, req->flow, (long)req->wptr, oldFd, oldFlow);

			rsp->fd = oldFd;
			rsp->flow = oldFlow;
		}
	}
	else {
		SYSLOG_DEBUG("Forward replied directly");
	}

	/* set response size */
	rsp->wptr = req->wptr;

	Message::destroy(req);
	return sendMessage(rsp);
}

int Echo::doMore(Message *req)
{
	int takenMs = 0;
	if (req->wptr >= (long)(sizeof(struct proto_h16_head) + sizeof(int))) {
		takenMs = *((int *)(req->data() + sizeof(struct proto_h16_head)));	
	}

	SYSLOG_DEBUG("More took %dms", takenMs);
	if (takenMs > 0) usleep((long)takenMs*1000);

	Message *rsp = Message::create(req->wptr, req->fd, req->flow);
	memcpy(rsp->data(), req->data(), req->wptr);

	struct proto_h16_head *h = (struct proto_h16_head *)rsp->data();
	h->cmd = ECHO_CMD_MORE_RSP;

	/* set response size */
	rsp->wptr = req->wptr;

	Message::destroy(req);
	return sendMessage(rsp);
}

int Echo::onMessage(beyondy::Async::Message *req)
{
	SYSLOG_DEBUG("Echo got a message from fd=%d flow=%d msize=%d", \
		req->fd, req->flow, req->wptr);
	struct proto_h16_head *h = (struct proto_h16_head *)req->data();
	int retval = 0;

	if (h->cmd == ECHO_CMD_ECHO_REQ) {
		retval = doEcho(req);
	}
	else if (h->cmd == ECHO_CMD_FORWARD_REQ || h->cmd == ECHO_CMD_MORE_RSP) {
		retval = doForward(req);
	}
	else if (h->cmd == ECHO_CMD_MORE_REQ) {
		retval = doMore(req);
	}
	else {
		SYSLOG_DEBUG("Echo uknown cmd=%d", h->cmd);
	}

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
