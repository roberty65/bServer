#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <ctype.h>
#include <errno.h>

#include "beyondy/bprof.h"
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

static const char *businessProcessor = NULL;

static int connectionIdleTimeout = 5;
static int connectionOutQueueSize = 10;

static const char *connectorAddress = NULL;
static int connectorHelloInterval = 5;

static int parseNameValue(const char *name, const char *val)
{
	fprintf(stderr, "%s=%s\n", name, val);
	if (strcmp(name, "logLevel") == 0) {
		if (strcmp(val, "FATAL") == 0) logLevel = LOG_LEVEL_FATAL;
		else if (strcmp(val, "ERROR") == 0) logLevel = LOG_LEVEL_ERROR;
		else if (strcmp(val, "WARN") == 0) logLevel = LOG_LEVEL_WARN;
		else if (strcmp(val, "INFO") == 0) logLevel = LOG_LEVEL_INFO;
		else if (strcmp(val, "DEBUG") == 0) logLevel = LOG_LEVEL_DEBUG;
		else return -1;
	}
	else if (strcmp(name, "logFileName") == 0) {
		if ((logFileName = strdup(val)) == NULL) return -1;
	}
	else if (strcmp(name, "logFileMaxSize") == 0) {
		char *endptr;
		logFileMaxSize = strtol(val, &endptr, 0);
		logFileMaxSize *= (*endptr == 'k' || *endptr == 'K') ? 1024 :
				  (*endptr == 'm' || *endptr == 'M') ? 1024*1024 :
				  (*endptr == 'g' || *endptr == 'G') ? 1024*1024*1024 : 1;
	}
	else if (strcmp(name, "logMaxBackup") == 0) {
		logMaxBackup = strtol(val, NULL, 0);
	}
	else if (strcmp(name, "listenAddress") == 0) {
		if ((listenAddress = strdup(val)) == NULL) return -1;
	}
	else if (strcmp(name, "listenBacklog") == 0) {
		listenBacklog = strtol(val, NULL, 0);
	}
	else if (strcmp(name, "listenMaxConnection") == 0) {
		listenMaxConnection = strtol(val, NULL, 0);
	}
	else if (strcmp(name, "systemThreadCount") == 0) {
		systemThreadCount = strtol(val, NULL, 0);
	}
	else if (strcmp(name, "systemInQueueSize") == 0) {
		systemInQueueSize = strtol(val, NULL, 0);
	}
	else if (strcmp(name, "systemOutQueueSize") == 0) {
		systemOutQueueSize = strtol(val, NULL, 0);
	}
	else if (strcmp(name, "businessProcessor") == 0) {
		if ((businessProcessor = strdup(val)) == NULL) return -1;
	}
	else if (strcmp(name, "connectionIdleTimeout") == 0) {
		connectionIdleTimeout = strtol(val, NULL, 0);
	}
	else if (strcmp(name, "connectorAddress") == 0) {
		if ((connectorAddress = strdup(val)) == NULL) return -1;
	}
	else if (strcmp(name, "connectorHelloInterval") == 0) {
		connectorHelloInterval = strtol(val, NULL, 0);
	}

	return 0;
}

static int loadConfig(const char *file)
{
	FILE *fp = fopen(file, "r");
	if (fp == NULL) return -1;

	char buf[8192];
	int lineno = 0;

	while (fgets(buf, sizeof buf, fp) != NULL) {
		++lineno;
		if (*buf == '#') continue;

		int slen = strlen(buf);	
		while (slen > 0 && (buf[slen - 1] == '\n' || buf[slen - 1] == '\r'))
			--slen;
		buf[slen] = 0;

		char *endptr = buf + slen;
		if ((endptr = strchr(buf, '#')) != NULL) {
			*endptr = 0;
		}

#define SKIP_SPACE(p)	do { while (isspace(*(p))) (p)++; } while (0)
#define SKIP_CHARS(p)	do { while (*(p) && !isspace(*(p))) (p)++; } while (0)
		char *nptr = buf;
		SKIP_SPACE(nptr);
		if (*nptr == 0) continue; // empty-line

		char *vptr = strchr(nptr, '=');
		if (vptr == NULL) {
			fprintf(stderr, "No '=' found at line: %d\n", lineno);
			return errno = EINVAL, -1;
		}
		else {
			*vptr++ = 0;
		}

		SKIP_SPACE(nptr); endptr = nptr; SKIP_CHARS(endptr); *endptr = 0;
		SKIP_SPACE(vptr); endptr = vptr; SKIP_CHARS(endptr); *endptr = 0;
#undef SKIP_SPACE
#undef SKIP_CHARS
		if (parseNameValue(nptr, vptr) < 0)
			break;
	}

	fclose(fp);
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

static void usage(const char *p)
{
	fprintf(stderr, "Usage: %s [ -d ] [ -c config-file ] | -h\n", p);
	exit(0);
}

extern "C" void appCallback(const char* s)
{
	fprintf(stderr, "appCallback: %s\n", s);
}

// must be called in Processor->onInit
extern "C" int addConnector(const char *name, const char *address, int openImmediately)
{
	// TODO:
	return -1;
}

extern "C" int getConnector(const char *name)
{
	// TODO:
	return -1;
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
		daemon(1, 1);

		if (fork() != 0) exit(0); // exit if not child
		setsid();

		signal(SIGHUP, SIG_IGN);
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGPIPE, SIG_IGN);
		signal(SIGTERM, SIG_IGN);
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
	
	Queue<Message> *inQ = new Queue<Message>(systemInQueueSize);
	Queue<Message> *outQ = new Queue<Message>(systemOutQueueSize);

	Processor *processor = loadProcessor(businessProcessor);
	processor->setSendFunc(async_send_delegator, outQ);
	if (processor->onInit() < 0) {
		SYSLOG_ERROR("processor onInit failed: %m");
		exit(1);
	}

	MtWorker *mtWorker = new MtWorker(inQ, processor, systemThreadCount);
	mtWorker->start();

	EventManager *emgr = new EventManager(outQ, 1024);

	connectionOutQueueSize = 10;
	int retval = emgr->addListener(listenAddress, inQ, processor, listenMaxConnection);
	if (retval < 0) {
		SYSLOG_ERROR("addListener failed");
		return 1;
	}

	if (connectorAddress != NULL) {
		SYSLOG_DEBUG("connector=%s, interval=%d", connectorAddress, connectorHelloInterval);
		int cid = emgr->addConnector(connectorAddress, inQ, processor, 1);
		if (cid < 0) {
			SYSLOG_ERROR("addConnector failed");
			return 2;
		}
	}

	emgr->start();

	// nothing
	processor->onExit();
	return 0;
}

