#ifndef __LOG__H
#define __LOG__H

#include <log4cpp/Category.hh>
#include <signal.h>

#include <sstream>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

using namespace std;

int init_log(const char *log_properties);
void exit_log();

log4cpp::Category* syslog();
log4cpp::Category* auditlog();
log4cpp::Category* othlog(const char *name);
log4cpp::CategoryStream logStream(std::string level); /* FATAL,ALERT,CRIT,ERROR,WARN,NOTICE,INFO,DEBUG,NOTSET,UNKOWN */
log4cpp::CategoryStream logStream(std::string name, std::string level);
void DebugLog(const char* fmt, ...);
void Log(const string& msg);
__attribute__((unused))
static void sig_usr1_log_priority(int signo)
{
	struct {
		int   level;
		char *name;
	} log_level[] = {
		{ log4cpp::Priority::EMERG,  " EMERG  " },
		{ log4cpp::Priority::FATAL,  " FATAL  " },
		{ log4cpp::Priority::ALERT,  " ALERT  " },
		{ log4cpp::Priority::CRIT,   " CRIT   " },
		{ log4cpp::Priority::ERROR,  " ERROR  " },
		{ log4cpp::Priority::WARN,   " WARN   " },
		{ log4cpp::Priority::NOTICE, " NOTICE " },
		{ log4cpp::Priority::INFO,   " INFO   " },
		{ log4cpp::Priority::DEBUG,  " DEBUG  " },
		{ log4cpp::Priority::NOTSET, " NOTSET " }
	};
	if (signo == SIGUSR1) {
                static const int log_level_size = sizeof(log_level) / sizeof(log_level[0]);
                static int index = 0;
                index = (index + 1) % log_level_size;
                syslog()->setPriority(log_level[index].level);
                syslog()->fatal("===== LOG4CPP: set priority: %s =====", log_level[index].name);
	}
}

__attribute__((unused))
static void set_log_priority(int loglevel)
{
        struct {
                int   level;
                char *name;
        } log_level[] = {
                { log4cpp::Priority::EMERG,  " EMERG  " },
                { log4cpp::Priority::FATAL,  " FATAL  " },
                { log4cpp::Priority::ALERT,  " ALERT  " },
                { log4cpp::Priority::CRIT,   " CRIT   " },
                { log4cpp::Priority::ERROR,  " ERROR  " },
                { log4cpp::Priority::WARN,   " WARN   " },
                { log4cpp::Priority::NOTICE, " NOTICE " },
                { log4cpp::Priority::INFO,   " INFO   " },
                { log4cpp::Priority::DEBUG,  " DEBUG  " },
                { log4cpp::Priority::NOTSET, " NOTSET " }
        };

        int log_level_size = sizeof(log_level) / sizeof(log_level[0]);    
        int index = abs(loglevel) % log_level_size;	

        syslog()->setPriority(log_level[loglevel].level);
        syslog()->fatal("===== LOG4CPP: set priority: %s =====", log_level[index].name);
}

#endif /*!__LOG__H */

