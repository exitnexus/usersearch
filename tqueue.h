/*****************************************************************************
 * tqueue.h
 * Threaded Queues
 ****************************************************************************/

#ifndef _TQUEUE_H_
#define _TQUEUE_H_

/*****************************************************************************
 * Includes
 ****************************************************************************/
#include <pthread.h>
#include <assert.h>

/*****************************************************************************
 * Defines
 ****************************************************************************/
#define TQ_FIFO					0x01	/* FIFO Queue */
#define TQ_HEAP					0x02	/* Heap (priority) queue */

#define _TQ_INIT_NODES			32		/* Number of initial nodes */
#define _TQ_REALLOC_NODES		32		/* The number of nodes to re-allocate
											when we run out of nodes. */
#define _TQ_FIRST				0		/* First Node (for fifo) */
#define _TQ_LAST				1		/* Last node (for fifo) */
/*****************************************************************************
 * Structures
 ****************************************************************************/

typedef struct tq_node {
	short				prio;					/* Priority (for heap) */
	struct tq_node		*next;					/* Next item (for fifo) */
	struct tq_node		*prev;					/* Prev item (for fifo) */
	void				*data;					/* The data */
} tq_node_t;

typedef struct {
	pthread_mutex_t		lock;					/* The queue lock */
	pthread_cond_t		cv;						/* Lock conditional variable */
	short				nonblock;				/* Causes threads to not block */
	int					type;					/* The type of node */
	int					size;					/* Number of nodes in **node */
	int					count;					/* Current # of nodes */
	tq_node_t			**node;					/* Array of nodes */
} tq_queue_t;


/*****************************************************************************
 * API
 ****************************************************************************/

/**
 * Initializes a queue of type |type|
 **/
tq_queue_t *tq_init(int type);

/**
 * Pops a value off the queue and returns the data.
 **/
void *tq_pop(tq_queue_t *queue);	/* Pop a value off the queue */

/**
 * Pushes a value onto a FIFO queue or Priority queue, respectively.
 **/
int tq_push(tq_queue_t *queue, short prio, void *data);

/**
 * Frees any unused memory, returns number of nodes freed or -1 on error
 **/
int tq_clean(tq_queue_t *queue);

/**
 * Destroys the queue and all allocated memory.
 **/
int tq_destroy(tq_queue_t *queue);

/**
 * Sets blocking and nonblocking mode (blocking is default).
 * Calling either function will trigger a cond_signal as well.
 **/
#define tq_block(q) {assert(q); pthread_mutex_lock(&q->lock); q->nonblock = 0; pthread_mutex_unlock(&q->lock); pthread_cond_broadcast(&q->cv);}
#define tq_nonblock(q) {assert(q); pthread_mutex_lock(&q->lock); q->nonblock = 1; pthread_mutex_unlock(&q->lock); pthread_cond_broadcast(&q->cv);}
#define tq_isempty(q) (q->count == 0)?1:0
#define tq_count(q) q->count
#define tq_size(q) q->size

#endif	// !_TQUEUE_H_
