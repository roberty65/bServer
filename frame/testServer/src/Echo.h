#ifndef __ECHO__H
#define __ECHO__H

#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "Block.h"
#include "Message.h"
#include "Processor.h"
#include "proto_h16.h"

class Echo : public beyondy::Async::Processor {
public:
	Echo() : beyondy::Async::Processor(), maxInputSize(2000000000), maxOutputSize(2000000000) {}
public:
	virtual size_t headerSize() const {
		return sizeof (proto_h16_head);
	}
	
	virtual ssize_t calcMessageSize(beyondy::Async::Message *msg) const {
		uint32_t len;
		memcpy(&len, msg->data() + msg->rptr, sizeof(len));

		if (len < sizeof(proto_h16_head) || len > maxInputSize) {
			errno = EINVAL;
			ssize_t retval = (ssize_t)len;
			if (retval > 0) return -retval;
			return retval;
		}
		
		return len;
	}

	virtual int onInit();
	virtual void onExit();

	virtual int onMessage(beyondy::Async::Message *req);
	virtual int onSent(beyondy::Async::Message *msg, int status);
private:
	int doEcho(beyondy::Async::Message *req);
	int doForward(beyondy::Async::Message *req);
	int doMore(beyondy::Async::Message *req);
private:
	size_t maxInputSize;
	size_t maxOutputSize;
};

#endif /* __ECHO__H */
