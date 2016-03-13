#include <log4cpp/PropertyConfigurator.hh>
#include "log.h"

log4cpp::Category* syslog()
{
	return &log4cpp::Category::getInstance("root");
}

log4cpp::Category* auditlog()
{
	return &log4cpp::Category::getInstance("audit");
}

log4cpp::Category* othlog(const char *name)
{
	return &log4cpp::Category::getInstance(name);
}

log4cpp::CategoryStream logStream(std::string level)
{
	return logStream("root", level);
}

log4cpp::CategoryStream logStream(std::string name, std::string level)
{
	return log4cpp::Category::getInstance(name).getStream(log4cpp::Priority::getPriorityValue(level));
}

int init_log(const char *log_properties_file)
{
	try {
		log4cpp::PropertyConfigurator::configure(log_properties_file);
	} catch(log4cpp::ConfigureFailure& f) {
		fprintf(stderr, "configure(%s) failed: %s\n", log_properties_file, f.what());
		return -1;
	}

	return 0;
}

void exit_log()
{
	// nothing	
}

void DebugLog(const char* fmt, ...)
{
        if( syslog()->getPriority() < log4cpp::Priority::DEBUG )
                return;

        int cnt = -1;
        char buf[128];
        va_list args;
        va_start(args, fmt);
        cnt = vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        if (cnt+1 <= int(sizeof(buf))) {
                Log(buf);
                return;
        } else if (cnt < 0) {
                Log(strerror(errno));
                return;
        }

        int newcnt = cnt+1;
        char* bufTmp = new char[newcnt];
        va_list args1;
        va_start(args1, fmt);
        cnt = vsnprintf(bufTmp, newcnt, fmt, args1);
        va_end(args1);

        if (cnt+1 <= newcnt) {
                Log(bufTmp);
        } else if (cnt < 0) {
                Log(strerror(errno));                
        }
        if(bufTmp) {
                delete []bufTmp;
        }
}


void Log(const string& msg)
{
        if( syslog()->getPriority() < log4cpp::Priority::DEBUG )
                return;
        syslog()->debug("%s", msg.c_str());
}

