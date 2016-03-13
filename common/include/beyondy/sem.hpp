/* sem.hpp
 * Copyright by beyondy, 2008-2010
 * All rights reserved.
**/

#ifndef __BEYONDY__SEM_HPP
#define __BEYONDY__SEM_HPP

#include <stdexcept>
#include <sys/sem.h>
#include <sys/stat.h>
#include <errno.h>
#include <beyondy/ipc.hpp>


namespace beyondy {

union semun
{
	int val;			/* value for SETVAL */
	struct semid_ds *buf;		/* buffer for IPC_STAT & IPC_SET */
	unsigned short int *array;	/* array for GETALL & SETALL */
	struct seminfo *__buf;		/* buffer for IPC_INFO */
};

struct auto_sembuf {
	auto_sembuf(int i, int op, int flags) {
		sbuf_.sem_num = i;
		sbuf_.sem_op = op;
		sbuf_.sem_flg = flags;
	}
	struct sembuf& s_buf() { 
		return sbuf_; 
	}

	struct sembuf sbuf_;
};

class sem {
public:
	sem() : key_(0), id_(-1) {}

	sem(const char *name) {
		sem(n2key(name));
	}

	sem(key_t key) {
		if (get(key) == -1) {
			throw std::runtime_error("sem get failed");
		}
	}

	sem(key_t key, int count, int initial) {
		if (open(key, count, initial) == -1) {
			throw std::runtime_error("sem create failed(1)");
		}
	}

	sem(const char *name, int count, int initial) {
		if (open(n2key(name), count, initial) == -1) {
			throw std::runtime_error("sem create failed(2)");
		}
	}
public:
	int get(key_t key) {
		int id = semget(key, 0, 0);
		if (id == -1) {
			return -1;
		}

		key_ = key;
		id_ = id;

		return 0;
	}

	int create(key_t key, int count) {
		int id = semget(key, count, IPC_CREAT | IPC_EXCL | 0644);
		if (id == -1) {
			return -1;
		}

		key_ = key;
		id_ = id;

		return 0;
	}

	int open(key_t key, int count, int initial) {
		if (get(key) == -1) { 
			if (create(key, count) == -1) {
				if (errno == EEXIST) {
					return get(key);
				}

				return -1;
			}
			else for (int i = 0; i < count; ++i) {
				if (op(i, initial, 0) == -1) {
					return -1;
				}
			}
		}

		return 0;
	}

	int op(int index, int op, int flags) {
		auto_sembuf sb(index, op, flags | SEM_UNDO);
		return semop(id_, &sb.s_buf(), 1);
	}

	int timedop(int index, int op, int flags, int msec) {
		struct timespec ts;
		auto_sembuf sb(index, op, flags | SEM_UNDO);

		msec2timespec(msec, ts);
		return semtimedop(id_, &sb.s_buf(), 1, &ts);
	}
	
	int setval(int val, int index = 0) {
		union semun arg;

		arg.val = val;
		return semctl(id_, index, SETVAL, arg);
	}
	
	int getval(int index = 0) {
		return semctl(id_, index, GETVAL);
	}

	int remove() {
		return semctl(id_, 0, IPC_RMID);
	}
public:
	int lock(int index = 0) { return op(index, -1, 0); }
	int trylock(int index = 0) { return op(index, -1, IPC_NOWAIT); }
	int timedlock(int msec, int index = 0) { return timedop(index, -1, 0, msec); }

	int unlock(int index = 0) { return op(index, +1, 0); }
private:
	key_t key_;
	int id_;
};

} /* namespace beyondy */

#endif /*!__BEYONDY__SEM_HPP */

