/* MemoryBuffer.cpp
 * Copyright by Beyondy.c.w 2002-2020
**/
#include <string.h>
#include <new>

#include "MemoryBuffer.h"

MemoryBuffer::MemoryBuffer()
	: mbuf(NULL), rptr(0), wptr(0),
	  bsize(0), autoDelete(false)
{
	/* nothing */
}

MemoryBuffer::MemoryBuffer(size_t _bsize)
	: mbuf(new unsigned char[_bsize]), rptr(0), wptr(0),
	  bsize(_bsize), autoDelete(true)
{
	/* nothing */
}

MemoryBuffer::MemoryBuffer(unsigned char *_buf, size_t _bsize, bool _autoDelete)
	: mbuf(_buf), rptr(0), wptr(0),
	  bsize(_bsize), autoDelete(_autoDelete)
{
	/* nothing */
}

MemoryBuffer::~MemoryBuffer()
{
	if (mbuf != NULL && autoDelete) delete[] mbuf;	
}

// return the length of the string, including the last '\0'
// so, if string is empty, return 0
int MemoryBuffer::readString(char *buf, size_t size)
{
	long savedRptr = rptr;
	uint32_t len = 0;

	if (readUint32(len) < 0) return - 1;
	if (rptr + len > wptr) {
		// buffer is not enough for this string
		rptr = savedRptr;
		return -1;
	}

	if (len < size) size = len;
	memcpy(buf, mbuf + rptr, size);

	rptr += len;
	return len; 
}

int MemoryBuffer::writeString(const char *buf, size_t size)
{
	if (buf == NULL) size = 0;
	else if (size == 0) size = strlen(buf) + 1;

	if (ensureCapacity(bsize + 4 + size) < 0)
		return -1;

	writeUint32((uint32_t)size);
	memcpy(mbuf + wptr, buf, size); 
	wptr += size;

	return 0;
}

int MemoryBuffer::resize(size_t added)
{
	size_t nsize = bsize + added, n = 1;
	if (nsize < bsize) return -1;	// overflow

	if (nsize >= (((size_t)1) << (8 * sizeof(size_t) - 1))) n = nsize;
	else while (n < nsize) n = n << 1;
	
	unsigned char *nbuf = new (std::nothrow)unsigned char[n];
	if (nbuf == NULL) return -1;

	if (mbuf != NULL) {
		// copy old data, release it when need 
		memcpy(nbuf, mbuf, wptr);
		if (autoDelete) delete[] mbuf;
	}

	mbuf = nbuf;
	bsize = n;
	autoDelete = true;
	
	return 0;
}

unsigned char *MemoryBuffer::data(unsigned char *_buf, size_t _size, bool _autoDelete)
{
	if (mbuf != NULL && autoDelete) delete[] mbuf;

	mbuf = _buf;
	bsize = _size;
	autoDelete = _autoDelete;

	return mbuf;
}

