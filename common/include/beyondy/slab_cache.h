/* slab_cache.h
 * Copyright by beyondy, 2008-2010
 * All rights reserved.
 *
**/
#ifndef __SLAB_CACHE__H
#define __SLAB_CACHE__H

#include <stdio.h>
#include <pthread.h>
#include <beyondy/list.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SLAB_NAMLEN		64

typedef unsigned int slab_size_t;	// do not need too large size_t
struct slab_block;

typedef struct slab_cache {
	pthread_mutex_t lock;

	struct slab_block *cur;		// cache, hit 90%
	struct list_head list_partial;
	struct list_head list_full;
	struct list_head list_free;

	slab_size_t object_size;
	slab_size_t object_num;		// objects per slab
	slab_size_t free_objects;

	slab_size_t object_align;
	slab_size_t slab_size;		// slab size
	slab_size_t free_limit;

	slab_size_t color_next;		// in color-align
	slab_size_t color_range;	// in color-align
	slab_size_t color_align;	// 16

	struct list_head sc_list;
	char name[SLAB_NAMLEN];

#ifdef ENABLE_STATISTICS
#endif /* ENABLE_STATISTICS */
} slab_cache_t;


slab_cache_t* slab_cache_create(const char* name, slab_size_t size, slab_size_t align, slab_size_t maxfree);
void slab_cache_destroy(slab_cache_t* sc);

void* slab_cache_allocate(slab_cache_t *sc);
int slab_cache_free(slab_cache_t* sc, void* addr);

void* slab_malloc(slab_size_t size);
void* slab_realloc(void* old, slab_size_t nsize);
void slab_free(void* addr, slab_size_t size);
size_t slab_capacity(void *addr);

int slab_cache_init(slab_size_t __max_blocks);
void slab_cache_fini();
void slab_cache_info(FILE* fp);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*! __SLAB_CACHE__H */

