#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "Log.h"

const char *strLevels[] = { "UNKN", "FATAL", "ERROR", "WARN", "INFO", "DEBUG" };

// global log instance which must be initialized before any log calls
LogHandle *sysLogHandle = NULL;

LogHandle::LogHandle(const char *_basePath, int _logLevel, long _maxFileSize, long _maxBackup)
	: logLevel(_logLevel), fd(-1), maxFileSize(_maxFileSize), maxBackup(_maxBackup)
{
	strncpy(basePath, _basePath, sizeof(basePath));
	basePath[sizeof basePath - 1] = 0;
}

int LogHandle::doLog(int level, const char *fmt...)
{
	struct timeval ts;
	struct tm tmbuf, *ptm;
	va_list ap; 
	char buf[8192];

	gettimeofday(&ts, NULL);
	ptm = localtime_r(&ts.tv_sec, &tmbuf);
	int mlen = snprintf(buf, sizeof buf, "%04d-%03d-%02dT%02d:%02d:%02d.%03d %s ",
		ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
		ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
		(int)(ts.tv_usec / 1000),
		level >= LOG_LEVEL_FATAL && level <= LOG_LEVEL_DEBUG
			? strLevels[level] : strLevels[0]);
	
	va_start(ap, fmt);
	int slen = vsnprintf(buf + mlen, sizeof(buf) - mlen, fmt, ap);
	if (slen >= (int)(sizeof(buf) - mlen - 2)) {
		/* discard the rest */
		slen = (int)(sizeof(buf) - mlen) - 2;
	}

	mlen += slen;
	buf[mlen++] = '\n';
	buf[mlen] = 0;

	va_end(ap);

	int fd = open(basePath, O_CREAT | O_APPEND | O_WRONLY, 0664);
	if (fd < 0) return -1;

	ssize_t wlen = write(fd, buf, mlen);
	assert(wlen == mlen);

	close(fd);

	//
	// write console for fatal log too
	// then exit?
	if (level == LOG_LEVEL_FATAL) fprintf(stderr, "%s", buf);

	return 0;
}

