/* slab_cache.c
 * Copyright by beyondy, 2008-2010
 * All rights reserved.
 *
**/
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>

#include "beyondy/slab_cache.h"
//#include "beyondy/rbtree.h"

//#define SLAB_BLOCK_DEBUG 1
#define SLAB_MAGIC_NUMBER	0x87654321

#define SLAB_BLOCK_SMALL	(1UL << 12)	// 4k
#define SLAB_BLOCK_BIG		(1UL << 16)	// 64k
#define SLAB_BLOCK_SMALL_MASK	(~(SLAB_BLOCK_SMALL - 1))
#define IS_BIG_OBJ(x)		((x) >> 8)

#ifndef SLAB_USAGE_RATIO
#define SLAB_USAGE_RATIO	95	// !!!re-define before compiling
#endif /*!SLAB_USAGE_RATIO */

#define SLAB_HASH_MAP_SIZE	1024	// default (re-initializable)

// !!!define it as short can support object-size=2
typedef unsigned int slab_bufctl_t;

#define BUFCTL_END	(((slab_bufctl_t)(~0UL))-0)
#define BUFCTL_NEXT	(((slab_bufctl_t)(0)))
#define	SLAB_LIMIT	(((slab_bufctl_t)(~0UL))-1)

#define MIN_OBJECT_SIZE	(sizeof(slab_bufctl_t))	// can not be less than
#define MAX_OBJECT_SIZE	((slab_size_t)~0UL)	// why?


typedef struct slab_block {
	slab_bufctl_t next_free;
	slab_size_t object_inuse;

	struct list_head sb_list;
	slab_size_t color_offset;

	slab_cache_t *sc;
	//struct rb_node rb_list;
} slab_block_t;

struct sized_slab_cache {
	slab_size_t size;
	slab_cache_t* sc;
};

// for big block object, quickly fetching block address of the allocated object
typedef struct slab_addr
{
#ifdef SLAB_BLOCK_DEBUG
	long          magic;
#endif
	slab_block_t* addr;
} slab_addr_t;

//struct huge_block {
//	struct list_head hb_list;
//	size_t size;
//	void* addr;
//};

#define CI(s)			{s,NULL}
// size should larger than 0
#define SLAB_INDEX(size) \
		(((unsigned long)(size) >>  2) <= 20 ?   0 + ((unsigned long)(size - 1) >>  2) : \
		 ((unsigned long)(size) >>  3) <= 20 ?  10 + ((unsigned long)(size - 1) >>  3) : \
		 ((unsigned long)(size) >>  4) <= 20 ?  20 + ((unsigned long)(size - 1) >>  4) : \
		 ((unsigned long)(size) >>  5) <= 20 ?  30 + ((unsigned long)(size - 1) >>  5) : \
		 ((unsigned long)(size) >>  6) <= 20 ?  40 + ((unsigned long)(size - 1) >>  6) : \
		 ((unsigned long)(size) >>  7) <= 20 ?  50 + ((unsigned long)(size - 1) >>  7) : \
		 ((unsigned long)(size) >>  8) <= 20 ?  60 + ((unsigned long)(size - 1) >>  8) : \
		 ((unsigned long)(size) >>  9) <= 20 ?  70 + ((unsigned long)(size - 1) >>  9) : \
		 ((unsigned long)(size) >> 10) <= 20 ?  80 + ((unsigned long)(size - 1) >> 10) : \
		 ((unsigned long)(size) >> 11) <= 20 ?  90 + ((unsigned long)(size - 1) >> 11) : \
		 ((unsigned long)(size) >> 12) <= 20 ? 100 + ((unsigned long)(size - 1) >> 12) : \
		 ((unsigned long)(size) >> 13) <= 20 ? 110 + ((unsigned long)(size - 1) >> 13) : \
		 ((unsigned long)(size) >> 14) <= 20 ? 120 + ((unsigned long)(size - 1) >> 14) : \
		 ((unsigned long)(size) >> 15) <= 20 ? 130 + ((unsigned long)(size - 1) >> 15) : \
		 ((unsigned long)(size) >> 16) <= 20 ? 140 + ((unsigned long)(size - 1) >> 16) : \
						       -1 )
#define sized_count	(sizeof(sized_caches) / sizeof(sized_caches[0]))
#define sized_max	(sized_caches[sized_count - 1].size)

#define CHECK_INIT() do { \
			if (!initialized) { \
				pthread_mutex_lock(&init_mutex); \
				if (!initialized) { \
					int retval = slab_cache_init(slab_max_blocks); \
					assert(retval == 0); \
				} \
				pthread_mutex_unlock(&init_mutex); \
			} \
		 } while (0)

static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
static int initialized = 0;
static int slab_max_blocks = 64; // def, re-initialiable

static pthread_mutex_t slabs_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct list_head slab_cache_head = LIST_HEAD_INIT(slab_cache_head);
//static struct rb_root slabs_root = RB_ROOT;

static struct sized_slab_cache sized_caches[] = {
	/*     4*20 */ CI(4),CI(8),CI(12),CI(16),CI(20),CI(24),CI(28),CI(32),CI(36),CI(40),CI(44),CI(48),
		       CI(52),CI(56),CI(60),CI(64),CI(68),CI(72),CI(76),CI(80),
	/*     8*10 */ CI(88),CI(96),CI(104),CI(112),CI(120),CI(128),CI(136),CI(144),CI(152),CI(160),
	/*    16*10 */ CI(176),CI(192),CI(208),CI(224),CI(240),CI(256),CI(272),CI(288),CI(304),CI(320),
	/*    32*10 */ CI(352),CI(384),CI(416),CI(448),CI(480),CI(512),CI(544),CI(576),CI(608),CI(640),
	/*    64*10 */ CI(704),CI(768),CI(832),CI(896),CI(960),CI(1024),CI(1088),CI(1152),CI(1216),CI(1280),
	/*   128*10 */ CI(1408),CI(1536),CI(1664),CI(1792),CI(1920),CI(2048),CI(2176),CI(2304),CI(2432),CI(2560),
	/*   256*10 */ CI(2816),CI(3072),CI(3328),CI(3584),CI(3840),CI(4096),CI(4352),CI(4608),CI(4864),CI(5120),
	/*   512*10 */ CI(5632),CI(6144),CI(6656),CI(7168),CI(7680),CI(8192),CI(8704),CI(9216),CI(9728),CI(10240),
	/*  1024*10 */ CI(11264),CI(12288),CI(13312),CI(14336),CI(15360),CI(16384),CI(17408),
		       CI(18432),CI(19456),CI(20480),
	/*  2048*10 */ CI(22528),CI(24576),CI(26624),CI(28672),CI(30720),CI(32768),CI(34816),CI(36864),
		       CI(38912),CI(40960),
	/*  4096*10 */ CI(45056),CI(49152),CI(53248),CI(57344),CI(61440),CI(65536),CI(69632),CI(73728),
		       CI(77824),CI(81920),
	/*  8192*10 */ CI(90112),CI(98304),CI(106496),CI(114688),CI(122880),CI(131072),CI(139264),
		       CI(147456),CI(155648),CI(163840),
	/* 16384*10 */ CI(180224),CI(196608),CI(212992),CI(229376),CI(245760),CI(262144),CI(278528),
	               CI(294912),CI(311296),CI(327680),
	/* 32768*10 */ CI(360448),CI(393216),CI(425984),CI(458752),CI(491520),CI(524288),CI(557056),
		       CI(589824),CI(622592),CI(655360),
	/* 65536*10 */ CI(720896),CI(786432),CI(851968),CI(917504),CI(983040),CI(1048576),CI(1114112),
		       CI(1179648),CI(1245184),CI(1310720),
};


static slab_cache_t slab_cache_cache = {
	.lock = PTHREAD_MUTEX_INITIALIZER,		// lock
	.name = { 0, },					// name
};
//static struct list_head hb_head = LIST_HEAD_INIT(hb_head);
//static pthread_mutex_t hb_mutex = PTHREAD_MUTEX_INITIALIZER;

static void* mmap_allocate(size_t size)
{
	void* ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
//	void* ptr = mmap(0x2ab562000000, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (ptr == (void *)-1) {
		return NULL;
	}
	return ptr;
}

static void mmap_deallocate(void* ptr, size_t size)
{
	munmap(ptr, size);
}

#define SLAB_START(sb)	((char *)((sb) + 1) + (sb)->color_offset)

static inline slab_addr_t* slab_addr(void* obj, slab_cache_t* sc)
{
	if (sc->slab_size != SLAB_BLOCK_SMALL) {
		return (slab_addr_t*)((char*)obj + sc->object_size);
	}
	return NULL;
}

static inline void* index_to_obj(slab_cache_t* sc, slab_block_t* sb, slab_size_t idx)
{
	if (sc->slab_size != SLAB_BLOCK_SMALL) {
		return (void *)(SLAB_START(sb) + idx * (sc->object_size + sizeof(struct slab_addr)));
	}
	return (void *)(SLAB_START(sb) + idx * sc->object_size);
}

static slab_size_t obj_to_index(slab_cache_t* sc, slab_block_t* sb, void* objp)
{
	unsigned long idx = (unsigned long)((char *)objp - SLAB_START(sb));
	if (sc->slab_size != SLAB_BLOCK_SMALL) {
		idx = idx / (sc->object_size + sizeof(struct slab_addr));
	} else {
		idx = idx / sc->object_size;
	}
	assert(idx < (slab_size_t)~0UL);
	return (slab_size_t)idx;
}

static inline slab_block_t* __obj_to_small_slab(void* obj)
{
	return (slab_block_t *)((unsigned long)obj & SLAB_BLOCK_SMALL_MASK);
}

static inline slab_cache_t* obj_to_cache(void* objp)
{
	slab_block_t* sb = __obj_to_small_slab(objp);
	if (sb != NULL) return sb->sc;
	return NULL;
}

static inline slab_block_t* obj_to_slab(slab_cache_t* sc, void* objp)
{
	if (sc->slab_size != SLAB_BLOCK_SMALL) {
		slab_addr_t* p = slab_addr(objp, sc);
#ifdef SLAB_BLOCK_DEBUG
		return (p->magic == SLAB_MAGIC_NUMBER) ? p->addr : NULL;
#else
		return p->addr;
#endif
	}
	return __obj_to_small_slab(objp);
}

static inline void __link_cache_list(slab_cache_t* sc)
{
	_list_add(&sc->sc_list, &slab_cache_head);
}

static inline void link_cache_list(slab_cache_t* sc)
{
	pthread_mutex_lock(&slabs_mutex);
	__link_cache_list(sc);
	pthread_mutex_unlock(&slabs_mutex);
}

static inline void __unlink_cache_list(slab_cache_t* sc)
{
	_list_del(&sc->sc_list);
}

static inline void unlink_cache_list(slab_cache_t* sc)
{
	pthread_mutex_lock(&slabs_mutex);
	__unlink_cache_list(sc);
	pthread_mutex_unlock(&slabs_mutex);
}

static inline slab_bufctl_t* small_slab_bufctl(slab_block_t* sb, slab_cache_t* sc, slab_size_t idx)
{
	return (slab_bufctl_t *)(SLAB_START(sb) + (idx * sc->object_size));
}

static inline slab_bufctl_t* big_slab_bufctl(slab_block_t* sb, slab_cache_t* sc, slab_size_t idx)
{
	return (slab_bufctl_t *)(SLAB_START(sb) + (idx * (sc->object_size + sizeof(struct slab_addr))));
}

static inline slab_bufctl_t* slab_bufctl(slab_block_t* sb, slab_cache_t* sc, slab_size_t idx)
{
	if (sc->slab_size != SLAB_BLOCK_SMALL) {
		return big_slab_bufctl(sb, sc, idx);
	}
	return small_slab_bufctl(sb, sc, idx);

}

static slab_block_t* slab_cache_grow(slab_cache_t* sc)
{
	assert(sc != NULL);
	unsigned int offset;
	void* objp = mmap_allocate(sc->slab_size);

	if (objp == NULL) {
		return NULL;
	}

	if (++sc->color_next >= sc->color_range) {
		sc->color_next = 0;
	}

	offset = sc->color_next * sc->color_align;
	slab_block_t* sb = (slab_block_t *)objp;
	sb->object_inuse = 0;
	sb->color_offset = offset;
	sb->next_free = 0;
	if (sc->slab_size != SLAB_BLOCK_SMALL) {
		assert((SLAB_START(sb) + sc->object_num * (sc->object_size + sizeof(struct slab_addr))) <= ((char *)objp + sc->slab_size));
	} else {
		assert((SLAB_START(sb) + sc->object_num * sc->object_size) <= ((char *)objp + sc->slab_size));
	}

	slab_size_t i = 0;
	if (sc->slab_size != SLAB_BLOCK_SMALL) {
		slab_bufctl_t* obj;
		slab_addr_t* p;
		for (; i < sc->object_num - 1; ++i) {
			obj  = big_slab_bufctl(sb, sc, i);
			*obj = i + 1;
			p    = slab_addr(obj, sc);
#ifdef SLAB_BLOCK_DEBUG
			p->magic = SLAB_MAGIC_NUMBER;
#endif
			p->addr   = sb;
		}
		obj  = big_slab_bufctl(sb, sc, i);
		*obj = BUFCTL_END;
		p    = slab_addr(obj, sc);
#ifdef SLAB_BLOCK_DEBUG
		p->magic = SLAB_MAGIC_NUMBER;
#endif
		p->addr   = sb;
	} else {
		for (; i < sc->object_num - 1; ++i) {
			*small_slab_bufctl(sb, sc, i) = i + 1;
		}
		*small_slab_bufctl(sb, sc, i) = BUFCTL_END;
	}

	_list_add(&sb->sb_list, &sc->list_free);
	sc->free_objects += sc->object_num;

	sb->sc = sc;
	return sb;
}

static void slab_cache_reap(slab_cache_t* sc)
{
	struct list_head* pl, *n;
	slab_block_t* sb;
	assert(sc != NULL);

	list_for_each_safe(pl, n, &sc->list_free) {
		if (sc->free_objects < sc->free_limit)
			break;
		sb = list_entry(pl, slab_block_t, sb_list);
		_list_del_init(&sb->sb_list);

		assert(sc->free_objects >= sc->object_num);
		sc->free_objects -= sc->object_num;
		mmap_deallocate(sb, sc->slab_size);
	}

	return;
}

static slab_size_t __calc_slab_sizes(slab_size_t slab_size, slab_size_t object_size, slab_size_t* num_p_slab, int* up)
{
	*num_p_slab = (slab_size - sizeof(slab_block_t)) / object_size;
	*up = *num_p_slab * object_size * 100 / slab_size;
	return (slab_size - sizeof(slab_block_t) - *num_p_slab * object_size);
}

// create small page of 4k slab_block
static slab_size_t __estimate_small_slab(size_t size, slab_size_t* num_p_slab, slab_size_t* slab_size)
{
	*slab_size = SLAB_BLOCK_SMALL;
	int up;
	return __calc_slab_sizes(*slab_size, size, num_p_slab, &up);
}

// create big page of 64k slab_block
static slab_size_t __estimate_big_slab(size_t size, slab_size_t* num_p_slab, slab_size_t* slab_size)
{
	slab_size_t i, left;
	for (i = 1; i < slab_max_blocks; ++i) {
		int up;
		*slab_size = i * SLAB_BLOCK_BIG;
		if (*slab_size < (size + sizeof(slab_block_t)))
			continue;
		left = __calc_slab_sizes(*slab_size, size, num_p_slab, &up);
		if (*num_p_slab > 1 && up >= SLAB_USAGE_RATIO) {
			return left;	
		}
	}
	// not find the best one
	return (slab_size_t)-1;
}

static int __create(slab_cache_t* sc, const char* name, slab_size_t size, slab_size_t align, slab_size_t maxfree)
{
	if (align == 0) {
		// auto determine align
		if ((size & 0x0f) == 0)
			align = 16;
		else if ((size & 0x07) == 0)
			align = 8;
		else if ((size & 0x03) == 0)
			align = 4;
		else if ((size & 0x01) == 0)
			align = 2;
		else
			align = 1;
	}

	assert(align > 0); // in fact, it must be 2^n=1,2,4,..
	size = (size + align -1) & ~(align - 1);

	// check name
	if (name == NULL || strlen(name) > (SLAB_NAMLEN - 1)) {
		errno = EINVAL;
		return -1;
	}

	// check object size in [min, max]
	if (size < MIN_OBJECT_SIZE || size > MAX_OBJECT_SIZE) {
		errno = EINVAL;
		return -1;
	}
	slab_size_t num_p_slab = 0, slab_size, color_align = 16, left;
	if (IS_BIG_OBJ(size)) { // if size is big object, create a big slab_block
		// the objects of big slab_block has its affective size and a size of sb_pointer, which for storing the address of slab_block
		left = __estimate_big_slab(size + sizeof(struct slab_addr), &num_p_slab, &slab_size);
	} else {
		left = __estimate_small_slab(size, &num_p_slab, &slab_size);
	}
	if (left == (slab_size_t)-1) {
		return -1;
	}
	assert(num_p_slab > 1); // must be larger than 2

	memset(sc, 0, sizeof(*sc));
	strncpy(sc->name, name, sizeof(sc->name));

	sc->cur = NULL;

	INIT_LIST_HEAD(&sc->list_partial);
	INIT_LIST_HEAD(&sc->list_full);
	INIT_LIST_HEAD(&sc->list_free);

	sc->object_size = size;
	sc->object_align = align;
	sc->object_num = num_p_slab;
	sc->slab_size = slab_size;

	sc->free_limit = maxfree;
	sc->free_objects = 0;

	sc->color_next = 0;
	sc->color_align = color_align;
	sc->color_range = left / sc->color_align;

	//pthread_mutex_init(&sc->lock, NULL);
	//_list_add(&sc->sc_list, &slab_cache_head);

	return 0;
}

slab_cache_t* slab_cache_create(const char* name, slab_size_t size, slab_size_t align, slab_size_t maxfree)
{
	slab_cache_t* sc = (slab_cache_t *)slab_cache_allocate(&slab_cache_cache);
	if (sc == NULL)
		return NULL;

	if (__create(sc, name, size, align, maxfree) < 0) {
		slab_cache_free(&slab_cache_cache, sc);
		return NULL;
	}

	pthread_mutex_init(&sc->lock, NULL);
	link_cache_list(sc);
	return sc;
}

static void __destroy_blocks(slab_cache_t* sc, struct list_head* head)
{
	struct list_head* pl, *n;
	list_for_each_safe(pl, n, head) {
		slab_block_t* sb = list_entry(pl, slab_block_t, sb_list);
		_list_del_init(&sb->sb_list);
		mmap_deallocate(sb, sc->slab_size);
	}
}

static void __destroy(slab_cache_t* sc)
{
	__destroy_blocks(sc, &sc->list_free);
	__destroy_blocks(sc, &sc->list_partial);
	__destroy_blocks(sc, &sc->list_full);
}

void slab_cache_destroy(slab_cache_t* sc)
{
	__destroy(sc);
	unlink_cache_list(sc);
	slab_cache_free(&slab_cache_cache, sc);
}

static inline void* __slab_cache_allocate_partial(slab_block_t* sb, slab_cache_t* sc)
{
	void *objp = index_to_obj(sc, sb, sb->next_free);
	assert((char*)objp + sc->object_size <= (char*)sb + sc->slab_size);
	// check if it belongs to big block
	if (sc->slab_size != SLAB_BLOCK_SMALL) {
		slab_addr_t* p = slab_addr(objp, sc);
#ifdef SLAB_BLOCK_DEBUG
		assert(p->magic == SLAB_MAGIC_NUMBER && p->addr == sb);
#else
		assert(p->addr == sb);
#endif
	}

	++sb->object_inuse;
	assert(sb->object_inuse > 1 && sb->object_inuse <= sc->object_num);

	assert(sc->free_objects > 0);
	--sc->free_objects;

	sb->next_free = *slab_bufctl(sb, sc, sb->next_free);
	if (sb->next_free == BUFCTL_END) {
		_list_del(&sb->sb_list);
		_list_add(&sb->sb_list, &sc->list_full);
		sc->cur = NULL; // no more
	}
	else {
		assert(sb->next_free < sc->object_num);
		if (sc->cur != sb) {
			sc->cur = sb;
		}
	}

	return objp;
}

static inline void* __slab_cache_allocate_free(slab_block_t* sb, slab_cache_t* sc)
{
	void *objp = index_to_obj(sc, sb, sb->next_free);
	assert((char*)objp + sc->object_size <= (char*)sb + sc->slab_size);
	// check if it belongs to big block
	if (sc->slab_size != SLAB_BLOCK_SMALL) {
		slab_addr_t* p = slab_addr(objp, sc);
#ifdef SLAB_BLOCK_DEBUG
		assert(p->magic == SLAB_MAGIC_NUMBER && p->addr == sb);
#else
		assert(p->addr == sb);
#endif
	}

	++sb->object_inuse;
	assert(sb->object_inuse == 1);

	assert(sc->free_objects > 0);
	--sc->free_objects; 
	sb->next_free = *slab_bufctl(sb, sc, sb->next_free);
	assert(sb->next_free != BUFCTL_END);

	_list_del(&sb->sb_list);
	_list_add(&sb->sb_list, &sc->list_partial);
	sc->cur = sb;

	return objp;
}

void* slab_cache_allocate(slab_cache_t *sc)
{
	assert(sc != NULL);
	void* objp;

	pthread_mutex_lock(&sc->lock);
	if (sc->cur != NULL) {
		objp = __slab_cache_allocate_partial(sc->cur, sc);
		assert(objp != NULL);
	}
	else if (!_list_empty(&sc->list_partial)) {
		slab_block_t* sb = list_entry(sc->list_partial.next, slab_block_t, sb_list);
		objp = __slab_cache_allocate_partial(sb, sc);
		assert(objp != NULL);
	}
	else if (!_list_empty(&sc->list_free)) {
		slab_block_t* sb = list_entry(sc->list_free.next, slab_block_t, sb_list);
		objp = __slab_cache_allocate_free(sb, sc);
		assert(objp != NULL);
	}
	else {
		slab_block_t* sb = slab_cache_grow(sc);
		if (sb != NULL) objp = __slab_cache_allocate_free(sb, sc);
		else objp = NULL;
	}

	pthread_mutex_unlock(&sc->lock);
	return objp;
}

static void __slab_cache_free(slab_cache_t* sc, slab_block_t* sb, void* addr)
{
	slab_size_t idx = obj_to_index(sc, sb, addr);

	*slab_bufctl(sb, sc, idx) = sb->next_free;
	sb->next_free = idx;

	assert(sb->object_inuse > 0 && sb->object_inuse <= sc->object_num);
	--sb->object_inuse;

	++sc->free_objects;
	assert(sc->free_objects > 0);

	if (sb->object_inuse == 0) {
		_list_del(&sb->sb_list);
		_list_add(&sb->sb_list, &sc->list_free);

		if (sc->cur == sb) {
			sc->cur = NULL;
		}
		// reap if too many free objects
		if (sc->free_objects > sc->free_limit) {
			slab_cache_reap(sc);
		}
	} else if (sb->object_inuse == sc->object_num - 1) {
		_list_del(&sb->sb_list);
		_list_add_tail(&sb->sb_list, &sc->list_partial);
	}

	return;	
}

static void __slab_free(slab_block_t* sb, void* addr)
{
	slab_cache_t* sc = sb->sc;	
	pthread_mutex_lock(&sc->lock);
	__slab_cache_free(sc, sb, addr);
	pthread_mutex_unlock(&sc->lock);
	return;
}

int slab_cache_free(slab_cache_t* sc, void* addr)
{
	slab_block_t* sb;

	pthread_mutex_lock(&sc->lock);
	if ((sb = obj_to_slab(sc, addr)) == NULL) {
		pthread_mutex_unlock(&sc->lock);
		return -1;
	}

	__slab_cache_free(sc, sb, addr);
	pthread_mutex_unlock(&sc->lock);
	return 0;
}

static void* huge_malloc(slab_size_t size)
{
	return mmap_allocate(size);
}

static int huge_free(void* addr, slab_size_t size)
{
	mmap_deallocate(addr, size);
	return 0;
}

void* slab_malloc(slab_size_t size)
{
	CHECK_INIT();

	if (!size) ++size;
	if (size <= sized_max) {
		long i = SLAB_INDEX(size);
		return slab_cache_allocate(sized_caches[i].sc);
	}
	// final, try allocate directly?
	return huge_malloc(size);
}

void* slab_realloc(void* old, slab_size_t nsize)
{
	CHECK_INIT();

	void* addr = slab_malloc(nsize);
	if (addr != NULL && old != NULL) {
		slab_block_t* sb = obj_to_slab(NULL, old);
		if (sb == NULL) {
			slab_free(addr, nsize);
		}
		else {
			memcpy(addr, old, sb->sc->object_size > nsize ? nsize : sb->sc->object_size);
			__slab_free(sb, old);
		}
	}

	return addr;
}

void slab_free(void* addr, slab_size_t size)
{
	if (addr == NULL)
		return;
	
	if (!size) ++size;
	if (size <= sized_max) {
		slab_block_t* sb;
		long i = SLAB_INDEX(size);
		if ((sb = obj_to_slab(sized_caches[i].sc, addr)) != NULL) {
			return __slab_free(sb, addr);
		} else {
			*(volatile int *)0x0 = 0x1234;
		}
	} else {
		if (huge_free(addr, size) < 0) {
			*(volatile int *)0x0 = 0x1234;
		}
	}
}

size_t slab_capacity(void *addr)
{
	CHECK_INIT();

	slab_cache_t* sc = obj_to_cache(addr);
	if (sc != NULL) {
		return sc->object_size;
	}
	else {
		assert(0);
		*(int *)0x0 = 0x1234;
	}

	return 0; // unknow
}

// 
int slab_cache_init(slab_size_t __max_blocks)
{
	if (initialized) {
		return 0;
	}

	if (__create(&slab_cache_cache, "cache-cache",  sizeof(slab_cache_t), sizeof(void  *), 0) < 0) {
		return -1;
	}

	__link_cache_list(&slab_cache_cache);
	initialized = 1;

	size_t i, sizes_count = sizeof(sized_caches) / sizeof(sized_caches[0]);	
	for (i = 0; i < sizes_count; ++i) {
		assert(sized_caches[i].size > 0);

		char name[64];
		snprintf(name, sizeof(name), "size-%ld", (long)sized_caches[i].size);
		sized_caches[i].sc = slab_cache_create(name, sized_caches[i].size, 0, 0);

		if (sized_caches[i].sc == NULL) {
			goto free_sizes;
		}
	}

	return 0; // all are OK

free_sizes:
	for (--i; i != (size_t)-1; --i) {
		slab_cache_destroy(sized_caches[i].sc);
		sized_caches[i].sc = NULL;
	}
	__destroy(&slab_cache_cache);
	__unlink_cache_list(&slab_cache_cache);
	return 0;
}

void slab_cache_fini()
{
	int i = 0;
	for (i = 0; i < sized_count; ++i)
		slab_cache_destroy(sized_caches[i].sc);
	__destroy(&slab_cache_cache);
	__unlink_cache_list(&slab_cache_cache);
}

static void __cache_info(FILE* fp, slab_cache_t* sc, int i)
{
	size_t pc = _list_count(&sc->list_partial);
	size_t fc = _list_count(&sc->list_full);
	size_t ff = _list_count(&sc->list_free);

	//double total = (double)(pc + fc + ff) * SLAB_BLOCK_SIZE / 1024.0 / 1024.0;
	//double used = (double)((pc + fc + ff) * sc->object_num - sc->free_objects) * sc->object_size / 1024.0 / 1024.0;
	double total = (double)(pc + fc + ff) * (sc->object_size != SLAB_BLOCK_BIG ? SLAB_BLOCK_SMALL : SLAB_BLOCK_BIG) / 1024.0 / 1024.0;
	double used = (double)((pc + fc + ff) * sc->object_num - sc->free_objects) * (sc->object_size != SLAB_BLOCK_BIG ? sc->object_size : sc->object_size + sizeof(struct slab_addr)) / 1024.0 / 1024.0;
	double ratio = 100 * used / ((pc + fc + ff) == 0 ? 1 : total);
	
	fprintf(fp, "*cache[%d]: %s, partial=%ld, full=%ld, free=%ld, object(size=%ld, align=%ld),\n"
		    "    slab(size=%ld, num/p=%ld), free(limit=%ld, num=%ld),\n"
		    "    color(next=%ld, rng=%ld, align=%ld)\n"
		    "    total-allocated=%.3fMb, used=%.3fMb(%.3f%%)\n",
		i, sc->name,
		(long)pc, (long)fc, (long)ff,
		(long)sc->object_size, (long)sc->object_align,
		(long)sc->slab_size, (long)sc->object_num,
		(long)sc->free_limit, (long)sc->free_objects,
		(long)sc->color_next, (long)sc->color_range,
		(long)sc->color_align, total, used, ratio); 
}

void slab_cache_info(FILE* fp)
{
	struct list_head* pl;
	int i;
	fprintf(fp, "slab cache infomation\n"
		    //"   allocate: %p%s, deallocate: %p%s, block-size=%d, slab-max-blocks=%d\n"
		    "   allocate: %p%s, deallocate: %p%s, small-block-size=%d, big-block-size=%d, slab-max-blocks=%d\n"
		    "   sized-caches: [%lu] (",
	 	    mmap_allocate, mmap_allocate == malloc ? "(malloc)" : "",
		    mmap_deallocate, (void *)mmap_deallocate == (void *)free ? "(free)" : "",
		    //(int)SLAB_BLOCK_SIZE, slab_max_blocks, sized_count);
		    (int)SLAB_BLOCK_SMALL, (int)SLAB_BLOCK_BIG, slab_max_blocks, sized_count);
	for (i = 0; i < sized_count; ++i) {
		fprintf(fp, "%s%ld", (i == 0) ? "" : ", ", (long)sized_caches[i].size);
	}

	fprintf(fp, ")\n");

	if (pthread_mutex_lock(&slabs_mutex) == 0) {
		i = 0;
		list_for_each(pl, &slab_cache_head) {
			slab_cache_t* sc = list_entry(pl, slab_cache_t, sc_list);
			__cache_info(fp, sc, i++);
		}

		//fprintf(fp, "slabs information:\n");
		//struct rb_node* pn;
		//i = 0;
		//for (pn = rb_first(&slabs_root); pn != NULL; pn = rb_next(pn)) {
		//	slab_block_t* sb = rb_entry(pn, slab_block_t, rb_list);
		//	fprintf(fp, "slab[%d]: inuse=%ld, offset=%ld, next-free=%ld, sc=%s\n", i++,
		//		(long)sb->object_inuse, (long)sb->color_offset, (long)sb->next_free, sb->sc->name);
		//}

		pthread_mutex_unlock(&slabs_mutex);
	}

	return;
}

