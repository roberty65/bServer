#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <string>
#include <map>

#include "beyondy/bprof.h"
#include "ConfigProperty.h"
#include "Bprof_ids.h"
#include "Message.h"
#include "Queue.h"
#include "Processor.h"
#include "EventManager.h"
#include "MtWorker.h"
#include "Log.h"

using namespace beyondy::Async;

static int logLevel = LOG_LEVEL_DEBUG;
static const char *logFileName = "../logs/server.log";
static long logFileMaxSize = 10*1024*1024;
static long logMaxBackup = 10;

static const char *listenAddress = NULL;
static int listenBacklog = 1024;
static int listenMaxConnection = 2000;

static int systemThreadCount = 100;
static int systemInQueueSize = 8000;
static int systemOutQueueSize = 9000;
static int systemInMessageExpire = 30;
static int systemOutMessageExpire = 30;

static const char *businessProcessor = NULL;

static int connectionIdleTimeout = 5;
static int reconnectDelay = 2;
static int connectionOutQueueSize = 100;

struct ConnectorInfo {
	const char *address;
	int flow;
	ConnectorInfo(const char *_address, int _flow) : address(_address), flow(_flow) {}
};
typedef std::map<std::string, ConnectorInfo> ConnectorMap;
static ConnectorMap connectors;

static int connectorHelloInterval = 3;

// if need, 
// must be called in Processor->onInit
extern "C" int addConnector(const char *name, const char *address, int openImmediately)
{
	// TODO:
	return -1;
}

extern "C" int getConnector(const char *name)
{
	std::map<std::string, ConnectorInfo>::iterator iter = connectors.find(name);
	if (iter == connectors.end()) return -1;
	return iter->second.flow;
}

static int loadConfig(const char *file)
{
	ConfigProperty cfp;
	if (cfp.parse(file) < 0) return -1;

	const char *val = cfp.getString("logLevel", "ERROR");
	if (strcmp(val, "FATAL") == 0) logLevel = LOG_LEVEL_FATAL;
	else if (strcmp(val, "ERROR") == 0) logLevel = LOG_LEVEL_ERROR;
	else if (strcmp(val, "WARN") == 0) logLevel = LOG_LEVEL_WARN;
	else if (strcmp(val, "INFO") == 0) logLevel = LOG_LEVEL_INFO;
	else if (strcmp(val, "DEBUG") == 0) logLevel = LOG_LEVEL_DEBUG;
	else logLevel = LOG_LEVEL_ERROR;
	
	val = cfp.getString("logFileName", "../logs/server.log");	
	if ((logFileName = strdup(val)) == NULL) return -1;
	
	logFileMaxSize = cfp.getInt("logFileMaxSize", 10*1024*1024);
	logMaxBackup = cfp.getInt("logMaxBackup", 10);

	val = cfp.getString("listenAddress", "tcp://*:6010");
	if ((listenAddress = strdup(val)) == NULL) return -1;
	
	listenBacklog = cfp.getInt("listenBacklog", 1024);
	listenMaxConnection = cfp.getInt("listenMaxConnection", 8192);

	systemThreadCount = cfp.getInt("systemThreadCount", 8);
	systemInQueueSize = cfp.getInt("systemInQueueSize", 8192);
	systemOutQueueSize = cfp.getInt("systemOutQueueSize", 16384);
	systemInMessageExpire = cfp.getInt("systemInMessageExpire", 30);
	systemOutMessageExpire = cfp.getInt("systemOutMessageExpire", 30);

	val = cfp.getString("businessProcessor", "../lib/libEcho.so");
	if ((businessProcessor = strdup(val)) == NULL) return -1;

	connectionIdleTimeout = cfp.getInt("connectionIdleTimeout", 10);
	connectionOutQueueSize = cfp.getInt("connectionOutQueueSize", 100);

	connectorHelloInterval = cfp.getInt("connectorHelloInterval", 3);
	reconnectDelay = cfp.getInt("reconnectDelay", 2);

	std::set<std::string> ctrs = cfp.getChildren("connector");
	for (std::set<std::string>::iterator iter = ctrs.begin();
		iter != ctrs.end();
			++iter) {
		val = cfp.getString("connector", iter->c_str(), "address", NULL);
		if (val == NULL || (val = strdup(val)) == NULL) return -1;
		if (val != NULL) {
			connectors.insert(std::make_pair(*iter, ConnectorInfo(val, -1)));
		}
	}

	return 0;
}

Processor *loadProcessor(const char *lib)
{
	void *dh = dlopen(lib, RTLD_NOW);
	if (dh == NULL) {
		SYSLOG_ERROR("can not load plugin %s, error: %s", lib, dlerror());
		return NULL;
	}

	Processor *(*creator)();
	if ((creator = (Processor *(*)())dlsym(dh, "createProcessor")) == NULL) {
		SYSLOG_ERROR("plugin %s has no createProcessor defined: %s", lib, dlerror());
		dlclose(dh);
		return NULL;
	}
	
	Processor *processor = (*creator)();
	if (processor == NULL) {
		SYSLOG_ERROR("plugin %s createProcessor failed: %m", lib);
		dlclose(dh);
		return NULL;
	}

	return processor;
}

static void sigExit(int sig)
{
	// TODO:
	SYSLOG_ERROR("exit by sinal %d", sig);
	exit(sig);
}

static void usage(const char *p)
{
	fprintf(stderr, "Usage: %s [ -d ] [ -c config-file ] | -h\n", p);
	exit(0);
}

beyondy::bprof_item items[] = {
	{ BPT_LOOP, "mai-loop", 0, 0 },
	{ BPT_EM_WAITING, "waiting events", 0, 0 },
	{ BPT_EM_HEVENTS, "handle events", 0, 0 },
	{ BPT_EM_DISPATCH, "dispatch outting", 0, 0 },
	{ BPT_EM_HEVENT, "in handle event", 0, 0 },
	{ BPT_EM_CHKTIMEOUT, "check-timeout", 0, 0 },
	{ BPT_CONN_READABLE, "conn-readable", 0, 0 },
	{ BPT_CONN_WRITABLE, "conn-writable", 0, 0 },
	{ BPT_CONN_SENDMESSAGE, "conn-send-message", 0, 0 },
	{ BPT_CONN_WRITEN, "conn-writeN", 0, 0 },
	{ -1, NULL, 0, 0 }
};

int main(int argc, char **argv)
{
	const char *etcFile = "../conf/server.conf";
	int daemonMode = 1;
	int ch, showVersion = 0;

	while ((ch = getopt(argc, argv, "dc:hv")) != EOF) {
		switch (ch) {
		case 'd':
			daemonMode = 0;
			break;
		case 'c':
			etcFile = optarg;
			break;
		case 'v':
			showVersion = 1;
			break;
		case 'h':
			usage(argv[0]);
			exit(0);
		default:
			fprintf(stderr, "invalid argument: %c\n", ch);
			usage(argv[0]);
			exit(1);
		}	
	}

	if (loadConfig(etcFile) < 0) {
		fprintf(stderr, "load config from %s failed: %m\n", etcFile);
		exit(1);
	}

	if (daemonMode) {
		int retval = daemon(1, 1);
		assert(retval == 0);

		if (fork() != 0) exit(0); // exit if not child
		setsid();

		struct sigaction act;
		act.sa_flags = 0;
		act.sa_handler = sigExit;
		sigaction(SIGTERM, &act, NULL);
		sigaction(SIGINT, &act, NULL);

		signal(SIGHUP, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGPIPE, SIG_IGN);
		signal(SIGCHLD, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);

		// TODO: mask other signal???

		// fork again?
		//setpgrp();
	}

	if (showVersion) {
		//Processor *processor = loadProcessor(businessProcessor);
		// TODO:
		exit(0);
	}

	if (BPROF_INIT(items, sizeof(items)/sizeof(items[0])-1) < 0) {
		fprintf(stderr, "bprof-init failed: %m\n");
		exit(1);
	}

	SYSLOG_INIT(logFileName, logLevel, logFileMaxSize, logMaxBackup);
	if (sysLogHandle == NULL) {
		fprintf(stderr, "SYS-LOG-INIT failed: %m\n");
		exit(1);
	}
	else {
		SYSLOG_INFO("serverd started");
	}
	
	Queue<Message> *inQ = new Queue<Message>(systemInQueueSize);
	Queue<Message> *outQ = new Queue<Message>(systemOutQueueSize);

	Processor *processor = loadProcessor(businessProcessor);
	processor->setSendFunc(async_send_delegator, outQ);

	EventManager *emgr = new EventManager(outQ, 1024);
	emgr->setMessageExpire(systemInMessageExpire, systemOutMessageExpire);

	emgr->setConnectionMaxIdle(connectionIdleTimeout);
	emgr->setConnectionOutQueueSize(connectionOutQueueSize);

	emgr->setHelloInterval(connectorHelloInterval);
	emgr->setReconnectDelay(reconnectDelay);

	int retval = emgr->addListener(listenAddress, inQ, processor, listenMaxConnection);
	if (retval < 0) {
		SYSLOG_FATAL("addListener at %s failed", listenAddress);
		return 1;
	}

	if (!connectors.empty()) for (ConnectorMap::iterator iter = connectors.begin(); iter != connectors.end(); ++iter) {
		int cid = emgr->addConnector(iter->second.address, inQ, processor, 1);
		if (cid < 0) {
			SYSLOG_FATAL("addConnector(%s) failed", iter->first.c_str());
			return 2;
		}
	
		iter->second.flow = cid;
		SYSLOG_DEBUG("connector=%s, flow=%d", iter->first.c_str(), cid);
	}

	// init here for it may call add/getConnector
	if (processor->onInit() < 0) {
		SYSLOG_FATAL("processor onInit failed: %m");
		exit(1);
	}

	// start worker before event manager starts
	MtWorker *mtWorker = new MtWorker(inQ, processor, systemThreadCount);
	mtWorker->start();

	emgr->start();

	// nothing
	processor->onExit();
	return 0;
}

