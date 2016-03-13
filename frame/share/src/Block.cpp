#include <new>
#include <string.h>
#include <sys/types.h>

#include "Block.h"

namespace beyondy {
namespace Async {

Block::Block()
	: buf(NULL), bsize(0), autoDelete(false)
{
	/* nothing */
}

Block::Block(size_t _bsize)
	: buf(new unsigned char[_bsize]), bsize(_bsize), autoDelete(true)
{
	/* nothing */
}


Block::Block(unsigned char *_buf, size_t _bsize, bool _autoDelete)
	: buf(_buf), bsize(_bsize), autoDelete(_autoDelete)
{
	/* nothing */
}

Block::~Block()
{
	if (autoDelete) delete[] buf;
}

int Block::resize(size_t nsize, size_t usedSize)
{
	if (nsize <= bsize)
		return 0; // no need for allocation

	unsigned char *nbuf = new (std::nothrow)unsigned char[nsize];
	if (nbuf == NULL)
		return -1;

	if (buf != NULL && bsize > 0) {
		/* copy old content, release it when need */
		memcpy(nbuf, buf, usedSize);
		if (autoDelete) delete[] buf;
	}

	buf = nbuf;
	bsize = nsize;
	autoDelete = true;

	return 0;
}
	
} /* Async */
} /* beyondy */
