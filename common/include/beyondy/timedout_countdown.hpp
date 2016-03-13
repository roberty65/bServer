/* timedout_countdown.hpp
 * Copyright by beyondy, 2008-2010
 * All rights reserved.
 * 
 * 1, beyondy, jan 23, 2008
 *    a simple timer countdown implementation.
 * 2, beyondy, feb 7, 2008
 *    add Restart method.
 * 3, beyondy, feb 8, 2008
 *    Update call During instead of calculating itself during.
**/
#ifndef __BEYONDY__TIMEDOUT_COUNTDOWN__HPP
#define __BEYONDY__TIMEDOUT_COUNTDOWN__HPP

#if defined WIN32
static int gettimeofday (struct timeval *tv, struct timezone *tz)
{
  long tick = GetTickCount();
  tv->tv_sec = tick/1000;
  tv->tv_usec = (tick%1000) * 1000;
  return 0;
}
#else
#include <sys/time.h>
#include <stdio.h>
#endif

namespace beyondy {
class TimedoutCountdown {
public:
	TimedoutCountdown(long timedout = -1) : timedout_(timedout) {
		gettimeofday(&start_tv_, NULL);
	}

	long GetTimedout() const { return timedout_; }
	void SetTimedout(long timedout) { timedout_ = timedout; }

	void Restart() {
		gettimeofday(&start_tv_, NULL);
	}
	
	long During() const {
		struct timeval tn;
		gettimeofday(&tn, NULL);
		return (tn.tv_sec  - start_tv_.tv_sec ) * 1000 + \
		       (tn.tv_usec - start_tv_.tv_usec) / 1000;
	}
	
	long Update(long *during = NULL) const {
		if (timedout_ < 0) return -1;		// never timeout

		long past = During();
		if (during != NULL) *during = past;
		past = timedout_  - past;

		return (past < 1) ? 0 : past;
	}

private:
	long timedout_;
	struct timeval start_tv_;
}; /* class TimedoutCountdown */
}; /* namespace beyondy */

#endif /*! __BEYONDY__TIMEDOUT_COUNTDOWN__HPP */

