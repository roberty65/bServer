/* slab_cache_t.hpp
 * Copyright by beyondy, 2008-2010
 * All rights reserved.
 *
**/

#include <typeinfo>
#include <stdexcept>
#include <beyondy/slab_cache.h>
#include <beyondy/singleton.hpp>

template <typename T>
class slab_object {
public:
	slab_object() {
		sc = slab_cache_create(typeid(T).name(), sizeof(T), 0, 0);
		if (sc == NULL) {
			throw std::runtime_error("slab_cache_create failed");
		}
	}
public:
	static slab_object<T>* instance()
	{
		return singleton<slab_object<T> >::instance();
	}

	static void destroy()
	{
		singleton<slab_object<T> >::destroy();
	}

	T* allocate() 
	{
		void* addr = slab_cache_allocate(sc);
		if (addr !=  NULL) {
			return new(addr) T;
		}

		return NULL;
	}

	void destroy(T* t) {
		t->~T();
		slab_cache_free(sc, t);
	}
private:
	slab_cache_t *sc;
};

