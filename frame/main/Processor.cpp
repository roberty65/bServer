#include "Log.h"
#include "Queue.h"
#include "Processor.h"

namespace beyondy {
namespace Async {

int async_send_delegator(Message *msg, void *data)
{
	// set it
	gettimeofday(&msg->ts_enqueue, NULL);

	Queue<Message> *outQ = (Queue<Message> *)data;
	if (outQ->push(msg) < 0) {
		SYSLOG_ERROR("push msg(fd=%d, flow=%d size=%ld) into system queue overflow", msg->fd, msg->flow, (long)msg->getWptr());
		return -1;
	}

	return 0;
}

} /* Async */
} /* beyondy */
