#include "Queue.h"
#include "Processor.h"

namespace beyondy {
namespace Async {

int async_send_delegator(Message *msg, void *data)
{
	Queue<Message> *outQ = (Queue<Message> *)data;
	outQ->push(msg);
	return 0;
}

} /* Async */
} /* beyondy */
