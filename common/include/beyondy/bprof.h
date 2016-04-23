#ifndef __BPROF__H
#define __BPROF__H

#include <sys/time.h>

#if BPROF == 1
#define BPROF_INIT(items,size) beyondy::bprof_init(items, size)
#define BPROF_EXIT() beyondy::bprof_exit()
#define BPROF_TRACE(id)	beyondy::bprof_guard __bprof_guard(id);

#else
#define BPROF_INIT(items,size) (0)
#define BPROF_EXIT() do {} while(0)
#define BPROF_TRACE(id) /* nothing */
#endif

namespace beyondy {
/* item, no mt protection
**/
struct bprof_item {
	int id;
	const char *name;
	unsigned long count;
	unsigned long usec;
};

/* caller must define a static items array
 * NOTE: they are not thread-safe. 
**/
extern bprof_item *bp_items;
extern size_t bp_size;

class bprof_guard {
public:
	bprof_guard(int _id) : id(_id) {
		gettimeofday(&t1, NULL);
	}
	~bprof_guard() {
		gettimeofday(&t2, NULL);
		unsigned long usec = (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);
		++bp_items[id].count;
		bp_items[id].usec += usec;
	}
private:
	struct timeval t1, t2;
	int id;
};

extern int bprof_init(struct bprof_item *items, size_t size);
extern void bprof_exit();

} /* beyondy */

#endif /* __BPROF__H */

