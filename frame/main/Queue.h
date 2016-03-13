#ifndef __QUEUE__H
#define __QUEUE__H

#include <pthread.h>
#include <queue>

namespace beyondy {
namespace Async {

template <class T>
class Queue {
public:
	Queue(size_t size);
private:
	Queue(const Queue&);
	Queue operator=(const Queue&);
public:
	int push(T* obj);
	T* pop(long ms = 0);
public:
	bool isEmpty();
private:
	pthread_mutex_t lock;
	pthread_cond_t cond;

	std::queue<T*> queue;
};

template <class T>
Queue<T>::Queue(size_t size)
{
	pthread_mutex_init(&lock, NULL);
	pthread_cond_init(&cond, NULL);
}

template <class T>
int Queue<T>::push(T *obj)
{
	pthread_mutex_lock(&lock);
	queue.push(obj);

	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&lock);

	return 0;
}

template <class T>
T* Queue<T>::pop(long ms) 
{
	pthread_mutex_lock(&lock);
	if (queue.empty() && ms > 0) {
		// TODO:		
		pthread_cond_wait(&cond, &lock);
	}

	T *obj = NULL;
	if (!queue.empty()) {
		obj = queue.front();
		queue.pop();
	}
	
	pthread_mutex_unlock(&lock);
	return obj;
}

template <class T>
bool Queue<T>::isEmpty()
{
	pthread_mutex_lock(&lock);
	bool retval = queue.empty();
	pthread_mutex_unlock(&lock);

	return retval;
}

} /* Async */
} /* beyondy */

#endif /* __QUEUE__H */
