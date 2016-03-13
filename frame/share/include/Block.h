#ifndef __BLOCK__H
#define __BLOCK__H

#include <sys/types.h>

namespace beyondy {
namespace Async {

class Block {
public:
	Block();
	Block(size_t bsize);
	Block(unsigned char *buf, size_t bsize, bool autoDelete);
	~Block();
private:
	Block(const Block&);
	Block& operator=(const Block&);
public:
	unsigned char *data() const { return buf; }
	size_t dsize() const { return bsize; }
	int resize(size_t nsize, size_t usedSize);
private:
	unsigned char *buf;
	size_t bsize;
private:
	bool autoDelete;
};

} /* Async */
} /* beyondy */

#endif /* __BLOCK__H */

