#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>



#include "search.h"
#include "tqueue.h"








#define NUM_THREADS 1
#define NUM_LOOPS 10000
#define NUM_SEARCHES 1000




typedef struct {
	unsigned int threadid;
	char state;
	tq_queue_t * request;
	tq_queue_t * response;
	search_data_t * data;
} thread_data_t;


void * searchRunner(thread_data_t * threaddata){
//	search_t ** searches;
	search_t * search;

	while(threaddata->state){
		search = tq_pop(threaddata->request);

		if(!search)
			break;

		searchUsers(threaddata->data, & search, 1);

		tq_push(threaddata->response, 0, (void *) search);
	}
	
	return NULL;
}


int main(){
	search_data_t * data;
	search_t ** searches;
	clock_t start, init, finish;
	uint32_t i, found, returned;

	tq_queue_t * request, * response;

	pthread_t thread[NUM_THREADS];
	thread_data_t threaddata[NUM_THREADS];


	srand(time(NULL));


//setup the request and response queues
	request = tq_init(TQ_FIFO);

	response = tq_init(TQ_FIFO);
//	tq_nonblock(response);



	printf("user_t size: %u\n", (unsigned int) sizeof(user_t));
	printf("search_t size: %u\n", (unsigned int)sizeof(search_t));

	printf("Doing %u runs of %u searches\n", NUM_LOOPS, NUM_SEARCHES);


	start = clock();



	printf("Loading user data\n");
	data = initUserSearchDump("search.txt", 0);
//	data = initRand(100000);

//	dumpSearchData(data, 10);
	
	printf("Starting %u threads\n", NUM_THREADS);
	init = clock();


	for(i = 0; i < NUM_THREADS; i++){
		threaddata[i].threadid = i;
		threaddata[i].state = 1;
		threaddata[i].request = request;
		threaddata[i].response = response;
		threaddata[i].data = data;

		pthread_create(&thread[i], NULL, (void*) searchRunner, (void*) &threaddata[i]);
	}


	printf("Generating %u searches\n", NUM_SEARCHES);

	for(i = 0; i < NUM_SEARCHES; i++){
		searches = generateSearch(1);

//		verbosePrintSearch(searches[0]);

		tq_push(request, 0, (void * ) searches[0]);
	}


	printf("Counting %u searches\n", NUM_SEARCHES);

	found = returned = 0;

	for(i = 0; i < NUM_SEARCHES; i++){
		search_t * search = tq_pop(response);
//		verbosePrintSearch(search);

		found    += search->totalrows;
		returned += search->returnedrows;
	}

//*
	tq_nonblock(request);
	for(i = 0; i < NUM_THREADS; i++)
		pthread_join(thread[i], NULL);
/*/

	for(i = 0; i < NUM_THREADS; i++)
		pthread_cancel(thread[i]);
//*/



	finish = clock();

	printf("found: %u\n", (unsigned int) found);
	printf("returned: %u\n", (unsigned int) returned);
	printf("\n");
	printf("init: %u ms\n", (unsigned int) (1000*(init-start)/CLOCKS_PER_SEC));
	printf("find: %u ms\n", (unsigned int) (1000*(finish-init)/CLOCKS_PER_SEC));
	

	return 0;
}


