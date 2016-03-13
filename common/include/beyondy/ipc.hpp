/* ipc.hpp
 * Copyright by beyondy, 2008-2010
 * All rights reserved.
**/

#ifndef __BEYONDY__IPC_HPP
#define __BEYONDY__IPC_HPP

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>

#ifndef IPC_PERM
#define IPC_PERM (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#endif /* IPC_PEERM */

static key_t n2key(const char *name)
{
	char *endptr;
	key_t retval = strtoul(name, &endptr, 0);

	if (!retval || endptr == name || *endptr != '\0') {
		if ((retval = ftok(name, 'b')) == (key_t)-1) {
			retval = 0;	// TODO: crc32(name);
		}
	}

	return retval;
}

static void msec2timespec(long msec, struct timespec& tspec)
{
    clock_gettime(CLOCK_REALTIME, &tspec);	

    uint32_t sec = 0;
    sec = msec/1000;
    msec = msec - sec*1000;
    tspec.tv_sec += sec;
    tspec.tv_nsec += (msec * 1000 * 1000);
}

#endif /*! __BEYONDY__IPC_HPP */

