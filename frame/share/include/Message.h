#ifndef __MESSAGE__H
#define __MESSAGE__H

#include <sys/types.h>
#include <sys/time.h>
#include <stdint.h>

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
	Message(unsigned char *buf, size_t bsize, int fd = -1, int flow = -1);
	~Message();
public:
#define PTR(p)		((unsigned char *)p)
#define XB1(d,s)	*(d) = *(s)
#define XB2(d,s)	XB1(d,s);XB1(d+1,s+1)
#define XB4(d,s)	XB2(d,s);XB2(d+2,s+2)
#define XB8(d,s)	XB4(d,s);XB4(d+4,s+4)
	int readInt8(int8_t& v) { if (rptr < wptr) { XB1(PTR(&v), PTR(data() + rptr)); rptr++; return 0; } return -1; }
	int readInt16(int16_t& v) { if (rptr + 1 < wptr) { XB2(PTR(&v), PTR(data() + rptr)); rptr += 2; return 0; } return -1; }
	int readInt32(int32_t& v) { if (rptr + 3 < wptr) { XB4(PTR(&v), PTR(data() + rptr)); rptr += 4; return 0; } return -1; }
	int readInt64(int64_t& v) { if (rptr + 7 < wptr) { XB8(PTR(&v), PTR(data() + rptr)); rptr += 8; return 0; } return -1; }
	int readUint8(uint8_t& v) { if (rptr < wptr) { XB1(PTR(&v), PTR(data() + rptr)); rptr++; return 0; } return -1; }
	int readUint16(uint16_t& v) { if (rptr + 1 < wptr) { XB2(PTR(&v), PTR(data() + rptr)); rptr += 2; return 0; } return -1; }
	int readUint32(uint32_t& v) { if (rptr + 3 < wptr) { XB4(PTR(&v), PTR(data() + rptr)); rptr += 4; return 0; } return -1; }
	int readUint64(uint64_t& v) { if (rptr + 7 < wptr) { XB8(PTR(&v), PTR(data() + rptr)); rptr += 8; return 0; } return -1; }
	int readString(char *buf, size_t size);

	int writeInt8(int8_t v) { if (wptr < (long)dblk.dsize()) { XB1(PTR(data() + wptr), PTR(&v)); wptr++; return 0; } return -1; }
	int writeInt16(int16_t v) { if (wptr + 1 < (long)dblk.dsize()) { XB2(PTR(data() + wptr), PTR(&v)); wptr += 2; return 0; } return -1; }
	int writeInt32(int32_t v) { if (wptr + 3 < (long)dblk.dsize()) { XB4(PTR(data() + wptr), PTR(&v)); wptr += 4; return 0; } return -1; }
	int writeInt64(int64_t v) { if (wptr + 7 < (long)dblk.dsize()) { XB8(PTR(data() + wptr), PTR(&v)); wptr += 8; return 0; } return -1; }
	int writeUint8(uint8_t v) { if (wptr < (long)dblk.dsize()) { XB1(PTR(data() + wptr), PTR(&v)); wptr++; return 0; } return -1; }
	int writeUint16(uint16_t v) { if (wptr + 1 < (long)dblk.dsize()) { XB2(PTR(data() + wptr), PTR(&v)); wptr += 2; return 0; } return -1; }
	int writeUint32(uint32_t v) { if (wptr + 3 < (long)dblk.dsize()) { XB4(PTR(data() + wptr), PTR(&v)); wptr += 4; return 0; } return -1; }
	int writeUint64(uint64_t v) { if (wptr + 7 < (long)dblk.dsize()) { XB8(PTR(data() + wptr), PTR(&v)); wptr += 8; return 0; } return -1; }
	int writeString(const char *buf, size_t size = 0);
private:
	int resize(size_t nsize);
public:
	unsigned char *data() const { return dblk.data(); }
	size_t leftCapacity() const { return dblk.dsize() - (size_t)wptr; }
	int enlargeCapacity(size_t nsize) { if (dblk.dsize() >= nsize) return 0; return resize(nsize); }
	void reset() { rptr = wptr = 0; }
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

