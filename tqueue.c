/*****************************************************************************
 * tqueue.c
 * Threaded Queues
 ****************************************************************************/

#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include "tqueue.h"


tq_queue_t *tq_init(int type) {
	tq_queue_t *queue;

	switch (type) {
		case TQ_FIFO:
		case TQ_HEAP:
			break;
		default:
			errno = EINVAL;
			return NULL;
	}

	queue = (tq_queue_t *)malloc(sizeof(tq_queue_t));
	if (queue == NULL)
		return NULL;

	/**
	 * Initialize Queue
	 **/
	queue->type = type;
	queue->nonblock = 0;
	switch (queue->type) {
		case TQ_FIFO:
			queue->size = 2;
			break;
		case TQ_HEAP:
			queue->size = _TQ_INIT_NODES;
			break;
	}
	queue->node = (tq_node_t **)calloc(queue->size, sizeof(tq_node_t *));
	queue->count = 0;
	pthread_mutex_init(&queue->lock, NULL);
	pthread_cond_init (&queue->cv, NULL);

	return queue;
}
#define _MARK_ printf("%s:%i %s()  %i, %i\n", __FILE__, __LINE__, __FUNCTION__, queue->count, queue->nonblock);
void *tq_pop(tq_queue_t *queue) {
	void *data;
	tq_node_t *node, *tnode;
	int p, c;

	if (queue == NULL)
		return NULL;

	pthread_mutex_lock(&queue->lock);

	/**
	 * This handles race conditions
	 * We have a lock on the queue, but the queue is empty so we wait on the 
	 * queue->cv condition and lock queue->lock (atomicly) and check if the
	 * queue is greater then 0 again.  We keep doing it until it is > 0,
	 * then fall break out of the blocking loop (which is called from threads)
	 * and dequeue and item.
	 **/

	if (queue->count == 0) {
		for (;;) {
			if (queue->nonblock == 1) {
				pthread_mutex_unlock(&queue->lock);
				return 0;
			}

			pthread_cond_wait(&queue->cv, &queue->lock);
			if (queue->count > 0)
				break;
		}
	}

	/**
	 * This is where things are dequeued.  At this point we know that
	 * there is at least 1 item in the queue and we have a lock on the
	 * queue.
	 **/
	switch (queue->type) {
		case TQ_FIFO:
			node = queue->node[_TQ_LAST];
			queue->count--;
			if (queue->count == 0) {
				queue->node[_TQ_FIRST] = NULL;
				queue->node[_TQ_LAST] = NULL;
			} else {
				queue->node[_TQ_LAST] = node->prev;
				if (queue->node[_TQ_LAST] != NULL)
					queue->node[_TQ_LAST]->next = NULL;
			}
			break;
		case TQ_HEAP:
			node = queue->node[0];
			queue->count--;
			if (queue->count > 1) {
				p = 1;
				c = 2;
				tnode = queue->node[p-1];
				while (c <= queue->count) {
					if (c < queue->count && queue->node[c]->prio > queue->node[c-1]->prio)
						c++;
					if (tnode->prio < queue->node[c-1]->prio) {
						queue->node[p-1] = queue->node[c-1];
						p = c;
						c = p * 2;
					} else {
						break;
					}
				}
				queue->node[p-1] = node;
			}
			break;
		default:
			/* If the API is uses properly, this is impossible */
			return NULL;
	}
	pthread_mutex_unlock(&queue->lock);

	data = node->data;
	free(node);
	return data;

}

int tq_push(tq_queue_t *queue, short prio, void *data) {
	tq_node_t *node;
	int retsize;	/* Return the current node size */
	int c, p;		/* Current/Parent Index (heap) */

	if (queue == NULL)
		return -1;

	node = (tq_node_t*)malloc(sizeof(tq_node_t));
	if (node == NULL)
		return -1;

	node->data = data;
	if (queue->type == TQ_HEAP)
		node->prio = prio;

	pthread_mutex_lock(&queue->lock);

	/**
	 * Allocate more nodes if needed. (heap only)
	 **/
	if (queue->type == TQ_HEAP && queue->count+1 > queue->size) {
		queue->size += _TQ_REALLOC_NODES;
		queue->node = (tq_node_t **)realloc(queue->node, sizeof(tq_node_t) * queue->size);
		if (queue->node == NULL)
			return -1;	// Unable to allocate more nodes
	}

	queue->count++;
	switch (queue->type) {
		case TQ_FIFO:
			if (queue->count == 1) { /* We are the first node */
				queue->node[_TQ_FIRST] = node;
				queue->node[_TQ_LAST] = node;
				node->next = NULL;
				node->prev = NULL;
			} else {
				queue->node[_TQ_FIRST]->prev = node;
				node->next = queue->node[_TQ_FIRST];
				queue->node[_TQ_FIRST] = node;
			}
			break;
		case TQ_HEAP:
			queue->node[queue->count-1] = node;
			c = queue->count;
			p = queue->count / 2;
			while (p >= 1) {
				if (queue->node[p-1]->prio < node->prio) {
					queue->node[c-1] = queue->node[p-1];
					c = p;
					p = c/2;
				} else {
					break;
				}
			}
			queue->node[c-1] = node;
			break;
		default:
			/* If the API is uses properly, this is impossible */
			free(node);
			queue->count--;
			return -1;
	}
	retsize = queue->count;
	pthread_cond_signal(&queue->cv);
	pthread_mutex_unlock(&queue->lock);
	return retsize;
}

int tq_clean(tq_queue_t *queue){
	int s;
	if (queue == NULL)
		return -1;
	
	s = 0;
	pthread_mutex_lock(&queue->lock);
	if (queue->type == TQ_HEAP) {	// We only need to clean heaps
		if (queue->size - (queue->count + _TQ_REALLOC_NODES) > 1) {
			s = queue->size - (queue->count + _TQ_REALLOC_NODES);
			queue->size = queue->count + _TQ_REALLOC_NODES;
			queue->node = (tq_node_t **)realloc(queue->node, sizeof(tq_node_t) * queue->size);
			if (queue->node == NULL) {
				pthread_mutex_unlock(&queue->lock);
				return -1;
			}
		}
	}
	pthread_mutex_unlock(&queue->lock);
	return s;
}

int tq_destroy(tq_queue_t *queue) {
	if (queue == NULL)
		return -1;

	free(queue->node);
	free(queue);
	return 0;
}


