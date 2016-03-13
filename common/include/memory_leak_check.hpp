#ifndef MEMORY_LEAK_CHECK__HPP
#define MEMORY_LEAK_CHECK__HPP

#include <malloc.h>
#include <string>
#include "log.h"

static inline void check_memory_from_status(long size)
{
	static long ii = 0;
	char file[32];
	char buf[1024];
	memset(buf, 0, sizeof(buf));
	snprintf(file, 32, "/proc/%d/status", getpid());
	FILE* fp = fopen(file, "r");
	long rssMem;
	while( fgets(buf, 1024, fp) != NULL )
	{
		std::string str(buf);
		int pos = str.find(":");
		if ( str.substr(0, pos) == "VmRSS" ) {
			str = str.substr(pos + 1, str.find("kB") - pos - 1);
			sscanf(str.c_str(), "%ld", &rssMem);
			break;
		}	
	}

	if(fp != NULL)
	{
		fclose(fp);
		fp = NULL;
	}

	syslog()->debug("[%ld] Memory VmRSS Use: \t%ld kB,limit : %ld kB", ii, rssMem,size);
	if ( rssMem > size ) 
	{
		syslog()->error("[%ld] Memory VmRSS Leak: %ld kB, over limit: %ld kB", ii, rssMem, size);
		fprintf(stderr,"[%ld] Memory VmRSS Leak: %ld kB, over limit: %ld kB", ii, rssMem, size);
		exit(1);
	}
        if ( rssMem > long(size * 0.5) )
        {
                malloc_trim(0);
                syslog()->debug("Memory VmRSS - malloc_trim()");
        }
	++ii;
}

static inline void check_memory_leak(long ml_size)
{
	static long ii = 0;
	char tempBuf[1204];

	struct mallinfo mInfo = mallinfo();
	sprintf(tempBuf, "[%ld]mallinfo:arena=%d ordblks=%d smblks=%d hblks=%d hblkhd=%d usmblks=%d fsmblks=%d uordblks=%d fordblks=%d keepcost=%d",
		ii,
		mInfo.arena,
		mInfo.ordblks,
		mInfo.smblks,
		mInfo.hblks,
		mInfo.hblkhd,
		mInfo.usmblks,
		mInfo.fsmblks,
		mInfo.uordblks,
		mInfo.fordblks,
		mInfo.keepcost);
	syslog()->debug("Memory Use: %s", tempBuf);
	
	if (mInfo.arena + mInfo.hblkhd > ml_size)
	{
		syslog()->error("Memory Leak: %s, over limit: %ld", tempBuf, ml_size);
		exit(1);
	}

	ii++;
}

#endif /*!MEMORY_LEAK_CHECK__HPP */
