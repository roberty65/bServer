/* mutex_cond.hpp
 * Copyright by beyondy, 2008-2010
 * All rights reserved.
 *
**/

#ifndef __BEYONDY__MUTEX_COND_HPP
#define __BEYONDY__MUTEX_COND_HPP

#include <sys/types.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <beyondy/sem.hpp>
#include <beyondy/ipc.hpp>

namespace beyondy {

class null_mutex {
public:
	null_mutex(const char *name) {}
	~null_mutex() {}
public:
	int lock() { return 0; }
	int trylock() { return 0; }
	int unlock() { return 0; }
};

class null_cond {
public:
	null_cond(const char *name) {}
	~null_cond() {}
public:
	int wait() { return 0; }
	int timedwait(int msec) { return 0; }
	int signal() { return 0; }
	int broadcast() { return 0; }
};

class null_mutex_cond {
public:
	null_mutex_cond(const char *m_name, const char *c_name) {}
	~null_mutex_cond() {}
public:
	int lock() { return 0; }
	int trylock() { return 0; }
	int unlock() { return 0; }

	int wait() { return 0; }
	int timedwait(int msec) { return 0; }
	int signal() { return 0; }
	int broadcast() { return 0; }
};

class thread_cond;

class thread_mutex {
public:
	thread_mutex(const char *name) {
		pthread_mutex_init(&mutex_, NULL);
	}
	~thread_mutex()	{
		pthread_mutex_destroy(&mutex_);
	}
public:
	int lock() {
		int retval = pthread_mutex_lock(&mutex_);
		if (retval) {
			errno = retval;
			return -1;
		}
		return 0;
	}
	int trylock() { 
		int retval = pthread_mutex_trylock(&mutex_);
		if (retval) {
			errno = retval;
			return -1;
		}
		return 0;
	}
	int unlock() {
		int retval = pthread_mutex_unlock(&mutex_);
		if (retval) {
			errno = retval;
			return -1;
		}
		return 0;
	}
private:
	pthread_mutex_t mutex_;
	friend class thread_cond;
};

class thread_cond {
public:
	thread_cond(thread_mutex& mutex, const char *name)
			: mutex_(mutex) {
		pthread_cond_init(&cond_, NULL);
	}
	~thread_cond() {
		pthread_cond_destroy(&cond_);
	}
public:
	int wait() {
		int retval = pthread_cond_wait(&cond_, &mutex_.mutex_);
		if (retval) {
			errno = retval;
			return -1;
		}
		return 0;
	}
	int timedwait(int msec) {
		struct timespec ts;
		msec2timespec(msec, ts);
		int retval = pthread_cond_timedwait(&cond_, &mutex_.mutex_, &ts);
		if (retval) {
			errno = retval;
			return -1;
		}
		return 0;
	}
	int signal() { 
		int retval = pthread_cond_signal(&cond_);
		if (retval) {
			errno = retval;
			return -1;
		}
		return 0;
	}
	int broadcast() {
		int retval = pthread_cond_broadcast(&cond_);
		if (retval) {
			errno = retval;
			return -1;
		}
		return retval;
	}
private:
	thread_mutex &mutex_;
	pthread_cond_t cond_;
};

class thread_mutex_cond {
public:
	thread_mutex_cond(const char *m_name, const char *c_name)
		: mutex_(m_name), cond_(mutex_, c_name) {}
	~thread_mutex_cond() {}
public:
	int lock() { return mutex_.lock(); }
	int trylock() { return mutex_.trylock(); }
	int unlock() { return mutex_.unlock(); }

	int wait() { return cond_.wait(); }
	int timedwait(int msec) { return cond_.timedwait(msec); }
	int signal() { return cond_.signal(); }
	int broadcast() { return cond_.broadcast(); }
public:
	thread_mutex& mutex() { return mutex_; }
private:
	thread_mutex mutex_;
	thread_cond cond_;
};

class system_mutex {
public:
	system_mutex(const char *name) : sem_(name, 1, 1) {}
	~system_mutex() {}
public:
	int lock() { return sem_.lock(); }
	int trylock() { return sem_.trylock(); }
	int unlock() { return sem_.unlock(); }
private:
	beyondy::sem sem_;
};

class system_cond {
public:
	system_cond(system_mutex& mutex, const char *name)
		: mutex_(mutex), cond_(name, 1, 0) {}
	~system_cond();
public:
	int wait() {
		if (cond_.trylock() == 0)
			return 0;
		mutex_.unlock();
		cond_.lock();
		mutex_.lock();
		return 0;
	}
	int timedwait(int msec) {
		if (cond_.trylock() == 0)
			return 0;
		mutex_.unlock();
		int retval = cond_.timedlock(msec);
		mutex_.lock();

		return retval;
	}
	int signal() { cond_.unlock(); return 0; }
	int broadcast() { cond_.unlock(); return 0; }
private:
	system_mutex mutex_;
	beyondy::sem cond_;
};

class system_mutex_cond {
public:
	system_mutex_cond(const char *m_name, const char *c_name)
		: mutex_(m_name), cond_(mutex_, c_name) {}
	~system_mutex_cond() {}
public:
	int lock() { return mutex_.lock(); }
	int trylock() { return mutex_.trylock(); }
	int unlock() { return mutex_.unlock(); }

	int wait() { return cond_.wait(); }
	int timedwait(int msec) { return cond_.timedwait(msec); }
	int signal() { return cond_.signal(); }
	int broadcast() { return cond_.broadcast(); }
private:
	system_mutex mutex_;
	system_cond cond_;
};

template <typename _Lock>
class auto_lock {
public:
	auto_lock(_Lock& lock) : lock_(lock) { lock_.lock(); }
	~auto_lock() { lock_.unlock(); }
private:
	_Lock& lock_;
};

class system_thread_mutex {
public:
	enum mutex_state_type { ML_THREAD, ML_SYSTEM } state_;

	system_thread_mutex();

	int lock() {
		return state_ == ML_SYSTEM ?
			  s_mutex_.lock()
			: t_mutex_.lock();
	}

	void unlock() {
	}

	void mutex_context(enum mutex_state_type state) {
		state_ = state;
	}

	enum mutex_state_type mutex_context() const {
		return state_;
	}
private:
	system_mutex s_mutex_;
	thread_mutex t_mutex_;
};

}; /* namespace beyondy */

#endif /*!__BEYONDY__MUTEX_COND_HPP */
