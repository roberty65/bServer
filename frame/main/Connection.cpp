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
	if ((size_t)inMsg->getWptr() < hsize) {
		size_t rest = hsize - inMsg->getWptr();
		assert(rest >= 0 && rest <= hsize);
		SYSLOG_DEBUG("connection fd=%d flow=%d to read head-rest=%d", fd, flow, (int)rest);
		
		ssize_t retval = readN(rest);
		if (retval > 0 && rest == hsize) {
			// only set tsLastRead for the first byte, so that can avoid client sending one message
			// bytes after bytes to take a long time to get a whole message
			gettimeofday(&tsLastRead, NULL);
			inMsg->ts_byte_first = tsLastRead;
		}

		if ((size_t)retval != rest) {
			SYSLOG_DEBUG("connection fd=%d flow=%d read head got %ld", fd, flow, (long)retval);
			return retval;	// partial, or error
		}

		msize = proc->calcMessageSize(inMsg);
		if ((ssize_t)msize < 1) {
			SYSLOG_ERROR("connection fd=%d flow=%d invalid message size: %ld", fd, flow, (long)msize);
			return -1;
		}

		SYSLOG_DEBUG("connection fd=%d flow=%d read head OK, got msize=%ld", fd, flow, (long)msize);
		if (inMsg->ensureCapacity(msize) < 0) {
			SYSLOG_DEBUG("connection fd=%d flow=%d read resize to msize=%ld failed", fd, flow, (long)msize);
			return -1;	/* ???try later */
		}
	}
	else {
		/* cache it? */
		msize = proc->calcMessageSize(inMsg);
		if (inMsg->ensureCapacity(msize) < 0) {
			SYSLOG_DEBUG("connection fd=%d flow=%d read resize to msize=%ld failed", fd, flow, (long)msize);
			return -1;	/* try later */
		}
	}

	size_t rest = msize - inMsg->getWptr();
	assert(rest >= 0 && rest <= msize - hsize);
	SYSLOG_DEBUG("connection fd=%d flow=%d to read body rest=%ld", fd, flow, (long)rest);

	ssize_t rlen = (rest == 0) ? (ssize_t)rest: readN(rest);
	if ((size_t)rlen == rest) {
		SYSLOG_DEBUG("connection fd=%d flow=%d got a msg, msize=%ld", fd, flow, (long)msize);

		gettimeofday(&tsLastRead, NULL);
		inMsg->ts_enqueue = tsLastRead;	// same time

		if (inQueue->push(inMsg) < 0) {
			SYSLOG_ERROR("connection fd=%d flow=%d inQ over-flow", fd, flow);
			Message::destroy(inMsg);
			
			inMsg = NULL;
			return -1;
		}

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
			// do not watch OUT event for it
			emgr->modifyConnection(this, 0, EVENT_OUT);
			return 0;
		}

		outMsg = outQueue->front();
		outQueue->pop_front();

		SYSLOG_DEBUG("connection fd=%d flow=%d start to send out msg(len=%ld)", fd, flow, (long)outMsg->getWptr());

		gettimeofday(&outMsg->ts_byte_first, NULL);
	}

	size_t left = outMsg->getWptr() - outMsg->getRptr();
	ssize_t wlen = writeN(outMsg, left);

	if ((size_t)wlen == left) {
		SYSLOG_DEBUG("connection fd=%d flow=%d send msg done(left=%ld)", fd, flow, (long)left);

		gettimeofday(&outMsg->ts_process_end, NULL);
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
		gettimeofday(&msg->ts_byte_first, NULL);

		size_t rest = msg->getWptr() - msg->getRptr();
		ssize_t wlen = writeN(msg, rest);

		if ((size_t)wlen == rest) {
			SYSLOG_ERROR("connection fd=%d flow=%d sendMessage %ld bytes done", fd, flow, (long)rest);

			gettimeofday(&msg->ts_process_end, NULL);
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

	// otherwise, outQueue is not empty (so need queue after them)
	// or outMsg is in sending, need queue too
	if (outQueue->size() >= (size_t)emgr->getConnectionOutQueueSize()) {
		SYSLOG_ERROR("connection fd=%d flow=%d sendMessge over-flow, out-queue-size=%ld", fd, flow, (long)outQueue->size());
		proc->onSent(msg, SS_QUEUE_FULL);
		return -1;
	}
	else {
		outQueue->push_back(msg);
	}

	SYSLOG_ERROR("connection fd=%d flow=%d sendMessge en-outQueue OK", fd, flow);
	return 0;
}

void Connection::checkMessageExpiration()
{
	struct timeval tNow;
	gettimeofday(&tNow, NULL);

	// TODO: how to check sending message?
	while (!outQueue->empty()) {
		Message *msg = outQueue->front();
		if (TV2MS(msg->ts_enqueue, tNow) < emgr->getOutMessageExpire())
			break;

		SYSLOG_ERROR("connection fd=%d flow=%d msg(size=%ld) expired in queue", fd, flow, (long)msg->getWptr());

		outQueue->pop_front();
		proc->onSent(msg, SS_TIMEDOUT);
	}
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

/* -1: error with errno, or closed by peer
 * > 0: bytes received
**/
ssize_t Connection::readN(size_t size)
{
	assert(size > 0);
	ssize_t rlen = ::read(fd, inMsg->wb(), size);
	++inMsg->ioCount;

	if ((size_t)rlen == size) {
		inMsg->incWptr(rlen);
		return rlen;
	}
	else if (rlen > 0) {
		inMsg->incWptr(rlen);
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
	ssize_t wlen = ::write(fd, msg->rb(), size);
	++msg->ioCount;

	if ((size_t)wlen == size) {
		msg->incRptr(wlen);
		return wlen;
	}
	else if (wlen > 0) {
		msg->incRptr(wlen);
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
