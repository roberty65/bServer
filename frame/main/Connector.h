/* Connector.h
 * 1, created by Robert Yu
 *    2010 Feb
**/
#ifndef __CONNECTIOR__H
#define __CONNECTIOR__H

#include "Connection.h"

namespace beyondy {
namespace Async {

class Connector : public Connection {
public:
	Connector(int flow, const char *address, EventManager *emgr, Queue<Message> *inQueue, Processor *proc);
public:
	//virtual int onReadable(int fd);
	virtual int onWritable(int fd);
	//virtual int onError(int fd);
public:
	virtual int sendMessage(Message *msg);
	virtual int destroy();
public:
	int open();
private:
	typedef enum {
		CONN_CLOSE = 0,
		CONN_OPENNING,
		CONN_ESTABLISHED,
	} ConnectionStatus_t;

	ConnectionStatus_t status;
	char address[128];
};

} /* Async */
} /* beyondy */

#endif /* __CONNECTIOR__H  */
