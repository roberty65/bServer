#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "beyondy/xbs_naddr.h"
#include "beyondy/xbs_socket.h"
#include "Processor.h"
#include "EventManager.h"
#include "Connector.h"
#include "Queue.h"
#include "Log.h"

namespace beyondy {
namespace Async {
	

Connector::Connector(int _flow, const char *_address, EventManager *_emgr, Queue<Message> *_inQueue, Processor *_proc)
	: Connection(-1, _flow, _emgr, _inQueue, _proc, CT_CONNECTOR, NULL), status(CONN_CLOSE)
{
	strncpy(address, _address, sizeof(address));
	address[sizeof(address) - 1] = 0;
}

int Connector::onWritable(int fd)
{
	SYSLOG_DEBUG("connector(%s) is onWritable", address);
	assert(this->fd == fd);

	if (status == CONN_ESTABLISHED) {
		return Connection::onWritable(fd);
	}
	else if (status == CONN_OPENNING) {
			int err = XbsGetError(this->fd);
			if (err != 0) {
				errno = err;
				SYSLOG_ERROR("connect to %s failed: %m", address);
				return -1;
			}

			status = CONN_ESTABLISHED;
			if (emgr->modifyConnection(this, EVENT_IN, 0) < 0) {
				SYSLOG_ERROR("connector(%s) fd=%d flow=%d add EVENT_IN failed: %m", address, fd, flow);
				return -1;
			}

			SYSLOG_ERROR("connector(%s) establish done: fd=%d flow=%d", address, fd, flow);
			return Connection::onWritable(fd);
	}

	SYSLOG_ERROR("connector %s is writable but in CLOSE state", address);
	return 0; // why here?
}

int Connector::open()
{	
	struct sockaddr_storage addr_buf;
	socklen_t addr_len = sizeof(addr_buf);
	int type, protocol;

	if (XbsPaddr2n(address, &type, &protocol, (struct sockaddr *)&addr_buf, &addr_len) < 0) return -1;
	if ((fd = XbsSocket(addr_buf.ss_family, type, protocol)) < 0) return -1;
	if (XbsSetFlags(fd, O_NONBLOCK, 0) < 0) return -1;
	if (XbsConnect(fd, (struct sockaddr *)&addr_buf, addr_len, 0) < 0) {
		if (errno == EINPROGRESS) {
			events = EVENT_OUT;
			status = CONN_OPENNING;
		}
		else {
			return -1;
		}
	} 
	else {
		events = EVENT_IN;
		status = CONN_ESTABLISHED;
	}

	emgr->mapFlow(flow, this);
	int retval = emgr->addConnection(this, events);
	if (retval < 0) {
		SYSLOG_ERROR("connector(%s) addConnection got error: %m", address);
		return retval;
	}

	SYSLOG_DEBUG("connector(%s) open OK, status=%d", address, (int)status);
	return 0;
}

int Connector::destroy()
{
	if (outMsg != NULL) {
		// TODO: push back to the top!!!
		outQueue->push(outMsg);
		outMsg = NULL;
	}

	// keep other message in the queue
	// TODO: check timeout? not here?

	::close(fd);
	return 0;	
}

int Connector::sendMessage(Message *msg)
{
	if (status == CONN_CLOSE) {
		if (open() < 0) {
			SYSLOG_WARN("open connector to %s failed", address);
			return -1;
		}

		SYSLOG_DEBUG("connector(%s) open OK when sendMessage", address);
	}

	if (status == CONN_OPENNING) {
		SYSLOG_DEBUG("connector(%s) is openning when sendMessage", address);
		outQueue->push(msg);
		return 0;
	}

	assert(status == CONN_ESTABLISHED);
	SYSLOG_DEBUG("connector(%s) is established when sendMessage", address);

	return Connection::sendMessage(msg);
}

} /* Async */
} /* beyondy */

