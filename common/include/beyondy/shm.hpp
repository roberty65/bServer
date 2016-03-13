/* shm.hpp
 * Copyright by beyondy, 2008-2010
 * All rights reserved.
 *
**/

#ifndef __BEYONDY__SHM_HPP
#define __BEYONDY__SHM_HPP

#include <string>
#include <stdexcept>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <cerrno>
#include <beyondy/ipc.hpp>
#include <beyondy/to_string.hpp>

namespace beyondy {
class shm {
public:
	shm()
		: id_(-1), addr_(0), size_(0)
	{}

	// only get
	shm(const char *name, const void *addr = 0)
		: id_(-1), addr_(0), size_(0)
	{
		if (get(n2key(name), addr) == -1) {
			throw std::runtime_error(
				toString("shm(name=%s, addr=%p) failed: %m", 
					name, addr)); 
		}
	}

	shm(key_t key, const void *addr = 0)
		: id_(-1), addr_(0), size_(0)
	{
		if (get(key, addr) == -1) {
			throw std::runtime_error(
				toString("shm(key=%d, addr=%p) failed: %m",
					key, addr));
		}
	}

	// open: create or get
	shm(const char *name, size_t size, const void *addr = 0)
		: id_(-1), addr_(0), size_(0)
	{
		if (open(n2key(name), size, addr) == -1) {
			throw std::runtime_error(
				toString("shm(name=%s, size=%lu, addr=%p) failed: %m",
					name, (unsigned long)size, addr));
		}
	}

	shm(key_t key, size_t size, const void *addr = 0)
		: id_(-1), addr_(0), size_(0)
	{
		if (open(key, size, addr) == -1) {
			throw std::runtime_error(
				toString("shm(key=%d, size=%lu, addr=%p) failed: %m",
					key, (unsigned long)size, addr));
		}
	}

	~shm() {
		detach();
	}
public:
	/* get only */
	int get(key_t key, const void *addr = 0) {
		id_ = shmget(key, 0, 0);
		if (id_ == -1) return -1;
		if (attach(addr) == -1) return -1;
		return 0;
	}

	/* create only */
	int create(key_t key, size_t size, const void *addr = 0) {
		id_ = shmget(key, size, IPC_CREAT | IPC_EXCL | IPC_PERM);
		if (id_ == -1) return -1;
		if (attach(addr) == -1) return -1;
		return 0;
	}

	/* get or create */
	int open(key_t key, size_t size, const void *addr = 0) {
		if (get(key, addr) != -1) return 0;
		if (create(key, size, addr) != -1) return 0;
		else if (errno == EEXIST && get(key, addr) != -1) return 0;
		return -1;
	}

	/* attach */
	int attach(const void *addr) {
		addr_ = shmat(id_, addr, 0);
		if (addr_ == (void *)-1) return addr_ = NULL, -1;
		return 0;
	}
	
	/* detach */
	int detach() {
		if (addr_ != 0 && shmdt(addr_) != -1) addr_ = 0;
		return (addr_ == 0) ? 0 : -1;
	}

	/* destroy */
	int destroy() {
		return shmctl(id_, IPC_RMID, 0);
	}

public:
	int id() const { return id_; }
	void *addr() const { return addr_; }
	size_t size() const { return get_size(); }
private:
	shm(const shm&);
	shm& operator=(const shm&);

	size_t get_size() const {
		struct shmid_ds buf;
		if (shmctl(id_, IPC_STAT, &buf) != -1) return buf.shm_segsz;
		return -1;
	}
private:
	int id_;
	void *addr_; 
	size_t size_;	/* cache field */
}; /* class */
}; /* namespace beyondy */

#endif /*! __BEYONDY__SHM_HPP */

