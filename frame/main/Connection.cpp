#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "beyondy/bprof.h"
#include "Bprof_ids.h"
#include "Processor.h"
#include "Block.h"
#include "Message.h"
#include "EventManager.h"
#include "Listener.h"
#include "Queue.h"
#include "Connection.h"
#include "Log.h"

namespace beyondy {
namespace Async {

#define IO_DONE		1
#define IO_PARTIAL	0

Connection::Connection(int _fd, int _flow, EventManager *_emgr, Queue<Message> *_inQueue, Processor *_proc, int _creatorType, void *_parent)
	: EventHandler(_fd), flow(_flow),
	  inMsg(NULL), inQueue(_inQueue),
	  outMsg(NULL), outQueue(new std::deque<Message*>()),
	  emgr(_emgr), proc(_proc),
	  creatorType(_creatorType)
{
	if (creatorType == CT_LISTENER)
		this->parentListener = static_cast<Listener *>(_parent);
	else
		this->parentData = _parent;
}

int Connection::onReadable(int fd)
{
	BPROF_TRACE(BPT_CONN_READABLE)

	assert(this->fd == fd);
	SYSLOG_DEBUG("connection fd=%d flow=%d onReadable", fd, flow);

	if (inMsg == NULL) {
		// TODO: message-pool
		inMsg = Message::create(fd, flow);
		if (inMsg == NULL) {
			/* not enough memory now, remove it for 1s */
			//emgr->ModifyConnection(this, 0, IN);
			//emgr->AddTimer(this, 1000, ENABLE_IN_EVENT);
			SYSLOG_ERROR("connection fd=%d flow=%d allocate memory for incoming message failed. try later", fd, flow);
			return 0;
		}
	}

	size_t hsize = proc->headerSize();
	size_t msize;
	if ((size_t)inMsg->wptr < hsize) {
		size_t rest = hsize - inMsg->wptr;
		assert(rest >= 0 && rest <= hsize);
		SYSLOG_DEBUG("connection fd=%d flow=%d to read head-rest=%d", fd, flow, (int)rest);
		
		ssize_t retval = readN(rest);
		if ((size_t)retval != rest) {
			SYSLOG_DEBUG("connection fd=%d flow=%d read head got %ld", fd, flow, (long)retval);
			return retval;	// partial, or error
		}

		gettimeofday(&tsLastRead, NULL);
		msize = proc->calcMessageSize(inMsg);
		if ((ssize_t)msize < 1) {
			SYSLOG_ERROR("connection fd=%d flow=%d invalid message size: %ld", fd, flow, (long)msize);
			return -1;
		}

		SYSLOG_DEBUG("connection fd=%d flow=%d read head OK, got msize=%ld", fd, flow, (long)msize);
		if (inMsg->enlargeCapacity(msize) < 0) {
			SYSLOG_DEBUG("connection fd=%d flow=%d read resize to msize=%ld failed", fd, flow, (long)msize);
			return -1;	/* ???try later */
		}
	}
	else {
		/* cache it? */
		msize = proc->calcMessageSize(inMsg);
		if (inMsg->enlargeCapacity(msize) < 0) {
			SYSLOG_DEBUG("connection fd=%d flow=%d read resize to msize=%ld failed", fd, flow, (long)msize);
			return -1;	/* try later */
		}
	}

	size_t rest = msize - inMsg->wptr;
	assert(rest >= 0 && rest <= msize - hsize);
	SYSLOG_DEBUG("connection fd=%d flow=%d to read body rest=%ld", fd, flow, (long)rest);

	ssize_t rlen = (rest == 0) ? (ssize_t)rest: readN(rest);
	if ((size_t)rlen == rest) {
		SYSLOG_DEBUG("connection fd=%d flow=%d got a msg, msize=%ld", fd, flow, (long)msize);

		gettimeofday(&tsLastRead, NULL);
		inQueue->push(inMsg);
		inMsg = NULL;

		return 0;
	}

	SYSLOG_DEBUG("connection fd=%d flow=%d read body got rlen=%ld", fd, flow, (long)rlen);	
	int retval = rlen < 0 ? (errno == EINPROGRESS ? 0 : -1) : 0;

	return retval;
}

int Connection::onWritable(int fd) 
{
	BPROF_TRACE(BPT_CONN_WRITABLE)

	assert(this->fd == fd);
	SYSLOG_DEBUG("connection fd=%d flow=%d onWritable", fd, flow);

	if (outMsg == NULL) {
		if (outQueue->empty()) {
			/* do not watch OUT event for it */
			emgr->modifyConnection(this, 0, EVENT_OUT);
			return 0;
		}

		outMsg = outQueue->front();
		outQueue->pop_front();

		SYSLOG_DEBUG("connection fd=%d flow=%d start to send out msg(len=%ld)", fd, flow, (long)outMsg->wptr);
	}

	size_t left = outMsg->wptr - outMsg->rptr;
	ssize_t wlen = writeN(outMsg, left);
	++outMsg->ioCount;

	if ((size_t)wlen == left) {
		SYSLOG_DEBUG("connection fd=%d flow=%d send msg done(left=%ld)", fd, flow, (long)left);
		proc->onSent(outMsg, SS_OK);
		outMsg = NULL;

		// TODO: send more this time?
		return 0;
	}
	else if ((size_t)wlen < left) {
		/* try next time */
		SYSLOG_DEBUG("connection fd=%d flow=%d partially sent: should=%ld, real=%ld", fd, flow, (long)left, (long)wlen);
		return 0;
	}

	SYSLOG_ERROR("connection fd=%d flow=%d send msg(rest=%d) failed", fd, flow, (long)left);
	return -1;
}

int Connection::onError(int fd) {
	assert(this->fd == fd);
	SYSLOG_DEBUG("connection fd=%d flow=%d onError", fd, flow);

	int retval = destroy();
	return retval;
}

int Connection::sendMessage(Message *msg) 
{
	BPROF_TRACE(BPT_CONN_SENDMESSAGE)
	if (outMsg == NULL && outQueue->empty()) {
		/* send out directly as can as possible */
		size_t rest = msg->wptr - msg->rptr;
		ssize_t wlen = writeN(msg, rest);
		++msg->ioCount;

		if ((size_t)wlen == rest) {
			SYSLOG_ERROR("connection fd=%d flow=%d sendMessage %ld bytes done", fd, flow, (long)rest);
			proc->onSent(msg, SS_OK);
			return 0;
		}
		else if ((size_t)wlen < rest) {
			/* send later */
			SYSLOG_ERROR("connection fd=%d flow=%d sendMessage %ld bytes, done=%ld ", fd, flow, (long)rest, (long)wlen);
			outMsg = msg;
			emgr->modifyConnection(this, EVENT_OUT, 0); // watch out-able
			return 0;
		}
		
		SYSLOG_ERROR("connection fd=%d flow=%d sendMessage %ld bytes failed", fd, flow, (long)rest);
		proc->onSent(msg, SS_IO_ERROR);
		return -1;
	}

	outQueue->push_back(msg); /*{
		SYSLOG_ERROR("connection fd=%d flow=%d sendMessge en-outQueue failed", fd, flow);
		proc->onSent(msg, SS_QUEUE_FULL);
		return -1;
	} */

	SYSLOG_ERROR("connection fd=%d flow=%d sendMessge en-outQueue OK", fd, flow);
	return 0;
}

int Connection::destroy() {
	/* notify un-sent message */
	if (inMsg != NULL) {
		Message::destroy(inMsg);
		inMsg = NULL;
	}

	if (outMsg != NULL) {
		proc->onSent(outMsg, SS_DESTROY);
		outMsg = NULL;
	}

	while (!outQueue->empty()) {
		outMsg = outQueue->front(); outQueue->pop_front();
		proc->onSent(outMsg, SS_DESTROY);
	}
	
	if (this->creatorType == CT_LISTENER) {
		Listener *listener = this->parentListener;
		listener->destroyConnection(this);
	}

	return 0;
}

/* 0: read some, but not done, 1: exactly done, -1: error */
ssize_t Connection::readN(size_t size)
{
	assert(size > 0);
	ssize_t rlen = ::read(fd, inMsg->data() + inMsg->wptr, size);
	if ((size_t)rlen == size) {
		inMsg->wptr += rlen;
		return rlen;
	}
	else if (rlen > 0) {
		inMsg->wptr += rlen;
		return rlen;
	}
	else if (rlen == 0) {
		/* closed by peer. reset errno */
		SYSLOG_ERROR("connection fd=%d flow=%d has been closed by peer", fd, flow);
		return errno = 0, -1;
	}

	/* error with errno set */
	SYSLOG_ERROR("connection fd=%d flow=%d read error: %m", fd, flow);
	return -1;
}

ssize_t Connection::writeN(Message *msg, size_t size) 
{
	BPROF_TRACE(BPT_CONN_WRITEN)

	assert(size > 0);
	ssize_t wlen = ::write(fd, msg->data() + msg->rptr, size);

	if ((size_t)wlen == size) {
		msg->rptr += wlen;
		return wlen;
	}
	else if (wlen > 0) {
		msg->rptr += wlen;
		return wlen;
	}
	else {
		SYSLOG_ERROR("connection fd=%d flow=%d write error: %m", fd, flow);
	}

	/* error */
	return -1;
}

} /* Async */
} /* beyondy */
