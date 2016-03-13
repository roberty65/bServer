#ifndef __MESSAGE__H
#define __MESSAGE__H

#include <sys/types.h>
#include <sys/time.h>

#include "Block.h"

namespace beyondy {
namespace Async {

class Message {
public:
	static Message *create(int fd, int flow);	
	static Message *create(size_t bsize, int fd, int flow);
	static void destroy(Message *msg);
public:
	Message(int fd, int flow);
	Message(size_t bsize, int fd, int flow);
	~Message();
public:
	int readInt8();
	int readInt16();
	// ...
	int writeInt8(int8_t v);
	int writeInt16(int16_t v);
	// ...
private:
	int resize(size_t nsize);
public:
	unsigned char *data() const { return dblk.data(); }
	int enlargeCapacity(size_t nsize) { if (dblk.dsize() >= nsize) return 0; return resize(nsize); }
public:	
	Block dblk;
	long rptr;	/* current read-pointer */
	long wptr;	/* current write-pointer */
	
	int fd;
	int flow;
	
	int ioCount;	/* when IN, how many times read is called, when out, how many write is called */
	struct timeval ts_byte_first, ts_byte_byte_last;
	struct timeval ts_enqueue, ts_dequeue;

	struct timeval ts_process_start, ts_process_end;
#define ts_connection_start ts_process_start	/* for out-message in connector-xxx */
#define ts_connection_end ts_process_end

	long extra[10];
	void *attached;	/* user-attached data */
};

} /* Async */
} /* beyondy */

#endif /* __MESSAGE__H */

