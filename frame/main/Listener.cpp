#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "beyondy/xbs_socket.h"
#include "EventManager.h"
#include "Queue.h"
#include "Connection.h"
#include "Processor.h"
#include "Listener.h"
#include "Log.h"

namespace beyondy {
namespace Async {

Listener::Listener(const char *_address, int _connMax, EventManager *_emgr, Queue<Message> *_inQueue, Processor *_proc)
	: EventHandler(-1), connActive(0), connMax(_connMax), emgr(_emgr), inQueue(_inQueue), proc(_proc)
{
	strncpy(address, _address, sizeof(address));
	address[sizeof(address) - 1] = 0;	
}

Listener::~Listener()
{
	::close(fd);
}

int Listener::onReadable(int fd) {
	assert(this->fd == fd);

	int cfd = ::accept(fd, NULL, NULL);
	if (cfd < 0) {
		SYSLOG_WARN("listener(%s) accept failed: %m", address);
		return 0; // ignore
	}

	char peerName[128];
	if (beyondy::XbsGetPeername(cfd, peerName, sizeof(peerName)) < 0) {
		::close(cfd);
		SYSLOG_ERROR("listener(%s) new accepted fd=%d can not get peer name: %m", address, cfd);
		return 0; /* ignore (this is usually caused by peer closed immediately */
	}

	beyondy::XbsSetFlags(cfd, O_NONBLOCK, 0);
	if (connActive >= connMax) {
		::close(fd);
		SYSLOG_ERROR("listener(%s) reject connection from %s for overflow limit %d", address, peerName, connActive);
		return 0; // ignore
	}

	SYSLOG_DEBUG("listener(%s) accepted a connection: fd=%d peer=%s", address, cfd, peerName);
	Connection *connection = createConnection(cfd);
	if (connection == NULL) {
		::close(fd);
		SYSLOG_ERROR("listener(%s) failed to create connection from %s", address, peerName);
		return 0; // ignore
	}

	++connActive;
	return 0;
}

int Listener::onWritable(int fd)
{
	assert(0);	
}

int Listener::onError(int fd)
{
	assert(0);
}

Connection *Listener::createConnection(int cfd) 
{
	int flow = emgr->nextFlow();
	
	Connection *connection = new Connection(cfd, flow, emgr, inQueue, proc, CT_LISTENER, static_cast<void *>(this));
	if (connection != NULL) {
		emgr->mapFlow(flow, connection);
		emgr->addConnection(connection, EVENT_IN);
	}
	else {
		SYSLOG_ERROR("listener(%s) new Connection failed: %m", address);
	}
	
	return connection;
}

void Listener::destroyConnection(Connection *connection) 
{
	SYSLOG_INFO("listener(%s) close its connection: fd=%d, flow=%d", address, connection->fd, connection->flow);

	emgr->unmapFlow(connection);
	emgr->deleteConnection(connection);
	::close(connection->fd);

	--connActive;
	delete connection;
}

int Listener::open()
{
	fd = XbsServer(address, 1024, O_NONBLOCK);
	if (fd < 0) {
		SYSLOG_ERROR("listener(%s) open failed: %m", address);
		return -1;
	}

	return 0;
}

} /* Async */
} /* beyondy */

