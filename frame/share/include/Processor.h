#ifndef __PROCESSOR__H
#define __PROCESSOR__H

#include <stdio.h>

#include "Message.h"

namespace beyondy {
namespace Async {

#define SS_OK		0
#define SS_TIMEDOUT	1
#define SS_IO_ERROR	2
#define SS_QUEUE_FULL	3
#define SS_OOM		4
#define SS_DESTROY	5

class Processor {
public:
	Processor() : sf(NULL), data(NULL) {}
	virtual ~Processor() {}
public:
	// below function should be const
	virtual size_t headerSize() const = 0;
	virtual ssize_t calcMessageSize(Message *msg) const = 0;

	// this will be called after it is created
	virtual int onInit() = 0;
	virtual void onExit() = 0;
	// this callback will be called in business-threads
	virtual int onMessage(Message *msg) =  0;
	// NOTE!!! this callback will be called in communication-thread(s)
	// DO LESS AS CAN AS POSSIBLE
	// msg->ts_process_end only set when status is OK
	virtual int onSent(Message *msg, int status) = 0;
	
	// call when connection is established/closed
	virtual int onConnected(const int& flow) { return 0; }
	virtual int onDisconnected(const int& flow) {return 0; }
	
	// will append message into outQueue
	int sendMessage(Message *msg) { if (sf != NULL) return (*sf)(msg, data); else return -1; }
public:	// !!!DO NOT CALL IT. It is just be called by framework in startup
	void setSendFunc(int (*sf)(Message *, void *), void *data) {
		this->sf = sf;
		this->data = data;
	}
private:
	int (*sf)(Message *, void *);
	void *data;
};

extern int async_send_delegator(Message *msg, void *data);

} /* Async */
} /* beyondy */

#endif /* __PROCESSOR__H */
