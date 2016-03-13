#ifndef __SINGLETON__HPP 
#define __SINGLETON__HPP 

#include <pthread.h>

template <typename T>
class singleton {
public:
	singleton() {} 
	static T* instance();
	static void destroy();
private:
	singleton(const singleton<T>&);
	singleton<T>& operator=(const singleton<T>&);
private:
	static singleton<T>* inst_;
	static pthread_mutex_t lock_;
	T obj_;
};

template <typename T>
singleton<T>* singleton<T>::inst_ = NULL;
template <typename T>
pthread_mutex_t singleton<T>::lock_ = PTHREAD_MUTEX_INITIALIZER;

template <typename T>
T* singleton<T>::instance()
{
	if (inst_ == NULL) {
		singleton<T>* t = new singleton<T>();

		pthread_mutex_lock(&lock_);
		if (inst_ == NULL)
			inst_ = t;
		else
			delete t;
		pthread_mutex_unlock(&lock_);
		
	}

	return &inst_->obj_;
}

template <typename T>
void singleton<T>::destroy()
{
	if (inst_ != NULL)
		delete inst_;
}

#endif /*!__SINGLETON__HPP */
