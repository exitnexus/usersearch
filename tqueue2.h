
#ifndef _TQUEUE_H_
#define _TQUEUE_H_

#include <pthread.h>
#include <queue>
using std::queue;

template <class T> class tqueue {
private:
	queue <T *> q;            // The queue
	pthread_mutex_t lock;  // The queue lock
	pthread_cond_t  cv;    // Lock conditional variable
	int             blck; // should pop() block by default

public:
	tqueue(){
		blck = 1;
		pthread_mutex_init(&lock, NULL);
		pthread_cond_init (&cv, NULL);
	}

/**
 * Pop an element off the queue
 * If block is 0, return null if the queue is empty, otherwise wait until an item is placed in the queue
 * Potentially in the future:
 *  block = -1 => block until an item is placed in the queue, and return it
 *  block = 0  => don't block, return null if the queue is empty
 *  block > 0  => block for <block> seconds  
 */  
	T * pop(const int thisblock = -1){
		T * ret;
		pthread_mutex_lock(&lock);

		/**
		 * If the queue is empty
		 *   if we're in non-blocking mode, just return
		 *	 otherwise wait on the condition to be triggered. The condition gets triggered
		 *	   either when something is added, in which case the while condition fails and
		 *	   the value gets returned, or the queue is changed into non-blocking mode and returns
		 **/

		while(q.empty()){
			if(thisblock == 0 || blck == 0){
				pthread_mutex_unlock(&lock);
				return NULL;
			}

			pthread_cond_wait(&cv, &lock);
		}

	//we know there is at least one item, so dequeue one and return
		ret = q.front();
		q.pop();

		pthread_mutex_unlock(&lock);

		return ret;
	}


	void push( T * val ){
		pthread_mutex_lock(&lock);

		q.push(val);

		pthread_mutex_unlock(&lock);
		pthread_cond_signal(&cv);
	}

	unsigned int size() const {
		pthread_mutex_lock(&lock);
		unsigned int ret = (unsigned int)q.size();
		pthread_mutex_unlock(&lock);
		return ret;
	}

	bool empty() const{
		pthread_mutex_lock(&lock);
		bool ret = q.empty();
		pthread_mutex_unlock(&lock);
		return ret;
	}

	void block(){
		pthread_mutex_lock(&lock);
		blck = 1;
		pthread_mutex_unlock(&lock);
		pthread_cond_broadcast(&cv);
	}

	void nonblock(){
		pthread_mutex_lock(&lock);
		blck = 0;
		pthread_mutex_unlock(&lock);
		pthread_cond_broadcast(&cv);
	}
};

#endif

