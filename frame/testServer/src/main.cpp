#include "Message.h"
#include "Queue.h"
#include "Processor.h"
#include "EventManager.h"
#include "MtWorker.h"
#include "Log.h"
#include "../../../serverd/src/Echo.h"

using namespace beyondy::Async;
extern "C" int getConnector(const char *name)
{
	// TODO:
	return -1;
}

int main(int argc, char **argv)
{
	SYSLOG_INIT("../logs/server.log", LOG_LEVEL_DEBUG, 1048576, 10);	
	
	beyondy::Async::Queue<beyondy::Async::Message> *inQ = new beyondy::Async::Queue<beyondy::Async::Message>(1024);
	beyondy::Async::Queue<beyondy::Async::Message> *outQ = new beyondy::Async::Queue<beyondy::Async::Message>(2048);

	beyondy::Async::Processor *processor = new Echo();
	processor->setSendFunc(async_send_delegator, outQ);
	int retval = processor->onInit();

	beyondy::Async::MtWorker *mtWorker = new beyondy::Async::MtWorker(inQ, processor, 10);
	mtWorker->start();

	beyondy::Async::EventManager *emgr = new beyondy::Async::EventManager(outQ, 1024);

	retval = emgr->addListener("tcp://0.0.0.0:5010", inQ, processor, 100);
	if (retval < 0) {
		SYSLOG_ERROR("addListener failed");
		return 1;
	}
/*
	int cid = emgr->addConnector("inet@localhost:5010/tcp", inQ, processor, 1);
	if (cid < 0) {
		SYSLOG_ERROR("addConnector failed");
		return 2;
	}
**/
	emgr->start();

	// nothing
	processor->onExit();

	return 0;
}

