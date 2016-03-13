#ifndef __LOG__H
#define __LOG__H

#define LOG_LEVEL_DEBUG		5
#define LOG_LEVEL_INFO		4
#define LOG_LEVEL_WARN		3
#define LOG_LEVEL_ERROR		2
#define LOG_LEVEL_FATAL		1

#define SYSLOG_INIT(path, level, maxFileSize, maxBackup) do { sysLogHandle = new LogHandle(path, level, maxFileSize, maxBackup); } while (0)
#define SYSLOG_DOLOG(level, s...) do { if (sysLogHandle->isEnabled(level)) sysLogHandle->doLog(level, s); } while (0)
#define SYSLOG_EXIT() do { delete sysLogHandle; } while (0)

#define SYSLOG_DEBUG(s...) 	SYSLOG_DOLOG(LOG_LEVEL_DEBUG, s)
#define SYSLOG_INFO(s...)	SYSLOG_DOLOG(LOG_LEVEL_INFO, s)
#define SYSLOG_WARN(s...)	SYSLOG_DOLOG(LOG_LEVEL_WARN, s)
#define SYSLOG_ERROR(s...)	SYSLOG_DOLOG(LOG_LEVEL_ERROR, s)
#define SYSLOG_FATAL(s...)	SYSLOG_DOLOG(LOG_LEVEL_FATAL, s)

class LogHandle {
public:
	LogHandle(const char *basePath, int logLevel, long maxFileSize, long maxBackup);
	~LogHandle();
public:
	int isEnabled(int level) { if (level > logLevel) return 0; return 1; }
	int doLog(int level, const char *fmt...);
private:
	int logLevel;
	int fd;

	long maxFileSize;
	long maxBackup;

	char basePath[512];
};

extern LogHandle *sysLogHandle;

#endif /* __LOG__H */
