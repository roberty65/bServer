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
extern "C" int getConnector(const char *name);

int Echo::onInit()
{
	maxInputSize = 50 * 1024 * 1024;
	maxOutputSize = 50 * 1024 * 1024;

	dstFlow = getConnector("dst");
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
	if (req->getWptr() >= (long)(sizeof(struct proto_h16_head) + sizeof(int))) {
		takenMs = *((int *)(req->data() + sizeof(struct proto_h16_head)));	
	}

	SYSLOG_DEBUG("Echo took %dms", takenMs);
	if (takenMs > 0) usleep((long)takenMs*1000);

	Message *rsp = Message::create(req->getWptr(), req->fd, req->flow);
	memcpy(rsp->data(), req->data(), req->getWptr());

	struct proto_h16_head *h = (struct proto_h16_head *)rsp->data();
	h->cmd = h->cmd + 1;

	/* set response size */
	rsp->setWptr(req->getWptr());

	Message::destroy(req);
	if (sendMessage(rsp) < 0) {
		Message::destroy(rsp);
		return -1;
	}

	return 0;
}

int Echo::doForward(Message *req)
{
	struct proto_h16_head *h = (struct proto_h16_head *)req->data();

	Message *rsp = Message::create(req->getWptr(), req->fd, req->flow);
	memcpy(rsp->data(), req->data(), req->getWptr());

	if (req->getWptr() >= (long)(sizeof(struct proto_h16_head) + 3 * sizeof(int))) {
		if (h->cmd == ECHO_CMD_FORWARD_REQ) {
			*(int *)(rsp->data() + sizeof(struct proto_h16_head) + 0) = *(int *)(req->data() + sizeof(struct proto_h16_head));
			*(int *)(rsp->data() + sizeof(struct proto_h16_head) + sizeof(int)) = req->flow;
			*(int *)(rsp->data() + sizeof(struct proto_h16_head) + 2* sizeof(int)) = req->fd;
			SYSLOG_DEBUG("Forward from fd=%d flow=%d size=%ld to flow=%d", req->fd, req->flow, (long)req->getWptr(), dstFlow);

			struct proto_h16_head *h = (struct proto_h16_head *)rsp->data();
			h->cmd = ECHO_CMD_MORE_REQ;

			rsp->fd = -1;
			rsp->flow = dstFlow;
		}
		else {
			int oldFlow = *(int *)(req->data() + sizeof(struct proto_h16_head) + sizeof(int));
			int oldFd = *(int *)(req->data() + sizeof(struct proto_h16_head) + 2 * sizeof(int));
			SYSLOG_DEBUG("Forward result from fd=%d flow=%d size=%ld replied to fd=%d flow=%d", \
				req->fd, req->flow, (long)req->getWptr(), oldFd, oldFlow);

			rsp->fd = oldFd;
			rsp->flow = oldFlow;
		}
	}
	else {
		SYSLOG_DEBUG("Forward replied directly");
	}

	/* set response size */
	rsp->setWptr(req->getWptr());

	Message::destroy(req);
	if (sendMessage(rsp) < 0) {
		Message::destroy(rsp);
		return -1;
	}

	return 0;
}

int Echo::doMore(Message *req)
{
	int takenMs = 0;
	if (req->getWptr() >= (long)(sizeof(struct proto_h16_head) + sizeof(int))) {
		takenMs = *((int *)(req->data() + sizeof(struct proto_h16_head)));	
	}

	SYSLOG_DEBUG("More took %dms", takenMs);
	if (takenMs > 0) usleep((long)takenMs*1000);

	Message *rsp = Message::create(req->getWptr(), req->fd, req->flow);
	memcpy(rsp->data(), req->data(), req->getWptr());

	struct proto_h16_head *h = (struct proto_h16_head *)rsp->data();
	h->cmd = ECHO_CMD_MORE_RSP;

	/* set response size */
	rsp->setWptr(req->getWptr());

	Message::destroy(req);
	if (sendMessage(rsp) < 0) {
		Message::destroy(rsp);
		return -1;
	}

	return 0;
}

int Echo::onMessage(beyondy::Async::Message *req)
{
	SYSLOG_DEBUG("Echo got a message from fd=%d flow=%d msize=%d, byte0=%ld,%d, enQ=%ld,%d, deQ=%ld,%ld", \
		req->fd, req->flow, req->getWptr(),
		req->ts_byte_first.tv_sec, req->ts_byte_first.tv_usec,
		req->ts_enqueue.tv_sec, req->ts_enqueue.tv_usec,
		req->ts_dequeue.tv_sec, req->ts_dequeue.tv_usec);
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
	SYSLOG_DEBUG("Echo message from fd=%d flow=%d msize=%d sent status=%d, enQ=%ld,%ld, deQ=%ld,%ld, byte0=%ld,%ld, sent=%ld,%ld", \
		msg->fd, msg->flow, msg->getWptr(), status,
		msg->ts_enqueue.tv_sec, msg->ts_enqueue.tv_usec,
		msg->ts_dequeue.tv_sec, msg->ts_dequeue.tv_usec,
		msg->ts_byte_first.tv_sec, msg->ts_byte_first.tv_usec,
		msg->ts_process_end.tv_sec, msg->ts_process_end.tv_usec);
	Message::destroy(msg);
	return 0;
}

extern "C" Processor* createProcessor()
{
	return new Echo();
}
