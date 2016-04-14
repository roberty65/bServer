#ifndef __MEMORY_BUFFER__H
#define __MEMORY_BUFFER__H

#include <sys/types.h>
#include <sys/time.h>
#include <stdint.h>

class MemoryBuffer {
public:
	MemoryBuffer();
	MemoryBuffer(size_t bsize);
	MemoryBuffer(unsigned char *buf, size_t bsize, bool autoDelete);
	~MemoryBuffer();
public:
#define PTR(p)		((unsigned char *)p)
#define XB1(d,s)	*(d) = *(s)
#define XB2(d,s)	XB1(d,s);XB1(d+1,s+1)
#define XB4(d,s)	XB2(d,s);XB2(d+2,s+2)
#define XB8(d,s)	XB4(d,s);XB4(d+4,s+4)
#define XRN(v, n) 	do {					\
		if ((rptr + n) <= wptr) { 			\
			XB ## n(PTR(&v), PTR(mbuf + rptr)); 	\
			rptr += (n);				\
			return 0; 				\
		}						\
								\
		return -1;					\
	} while (0)
#define XWN(v, n)	do {					\
		if ((wptr + n) <= (long)bsize || resize(n) >= 0) {	\
			XB ## n(PTR(mbuf + wptr), PTR(&v));	\
			wptr += n;				\
			return 0;				\
		}						\
								\
		return -1;					\
	} while (0)

	int readInt8(int8_t& v) { XRN(v, 1); }
	int readInt16(int16_t& v) { XRN(v, 2); }
	int readInt32(int32_t& v) { XRN(v, 4); }
	int readInt64(int64_t& v) { XRN(v, 8); }
	int readUint8(uint8_t& v) { XRN(v, 1); }
	int readUint16(uint16_t& v) { XRN(v, 2); }
	int readUint32(uint32_t& v) { XRN(v, 4); }
	int readUint64(uint64_t& v) { XRN(v, 8); }
	int readString(char *buf, size_t size);

	int writeInt8(int8_t v) { XWN(v, 1); }
	int writeInt16(int16_t v) { XWN(v, 2); }
	int writeInt32(int32_t v) { XWN(v, 4); }
	int writeInt64(int64_t v) { XWN(v, 8); }
	int writeUint8(uint8_t v) { XWN(v, 1); }
	int writeUint16(uint16_t v) { XWN(v, 2); }
	int writeUint32(uint32_t v) { XWN(v, 4); }
	int writeUint64(uint64_t v) { XWN(v, 8); }
	int writeString(const char *buf, size_t size = 0);
private:
	int resize(size_t added);
public:
	size_t leftCapacity() const { return bsize - (size_t)wptr; }
	int ensureCapacity(size_t nsize) { if (bsize >= nsize) return 0; return resize(nsize - bsize); }
	void reset() { rptr = wptr = 0; }
public:
	unsigned char *rb() const { return (mbuf + rptr); }
	unsigned char *wb() const { return (mbuf + wptr); }
	unsigned char *data() const { return mbuf; }
	unsigned char *data(unsigned char *buf, size_t size, bool autoDelete);
public:
	long getRptr() const { return rptr; }
	long setRptr(long _rptr) { return (rptr = _rptr); }
	long incRptr(long _inc) { return (rptr += _inc); }
	long getWptr() const { return wptr; }
	long setWptr(long _wptr) { return (wptr = _wptr); }
	long incWptr(long _inc) { return (wptr += _inc); }
private:
	unsigned char *mbuf;
	long rptr;		/* current read-pointer */
	long wptr;		/* current write-pointer */
	size_t bsize;		/* mbufer's size */
	bool autoDelete;
};

#endif /* __MEMORY_BUFFER__H */
