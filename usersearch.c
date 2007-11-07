
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <unistd.h>



#include "search.h"
#include "tqueue.h"
#include "libevent/compat/sys/queue.h"
#include "libevent/event.h"
#include "libevent/evhttp.h"



#define MAX_THREADS 100


typedef struct {
	unsigned int threadid;
	char state;
	int pushfd;
	tq_queue_t * updates;
	tq_queue_t * request;
	tq_queue_t * response;
	search_data * data;
} thread_data_t;


//to tell the main thread that there is new stuff in the queue
void signalfd(int fd){
	const char signalbuf[1] = { 0 };
	if(fd)
		write(fd, signalbuf, 1);
}

void * searchRunner(thread_data_t * threaddata){
	search * srch;

	while(threaddata->state){
		srch = (search *)tq_pop(threaddata->request);

		if(!srch)
			break;

		threaddata->data->searchUsers(srch);

		tq_push(threaddata->response, 0, (void *) srch);
		signalfd(threaddata->pushfd);
	}

	return NULL;
}


void handle_queue_response(int fd, short event, void *arg){
	tq_queue_t * response = (tq_queue_t *)arg;
	search * srch;

	struct evbuffer *evb;

	unsigned int i;

	char buf[64];
	read(fd, buf, sizeof(buf));


	while(!tq_isempty(response)){
		srch = (search *)tq_pop(response);

		if(!srch)
			continue;

		evb = evbuffer_new();

		evbuffer_add_printf(evb, "Total Rows: %u\n", srch->totalrows);
		evbuffer_add_printf(evb, "Returned Rows: %u\n", srch->results.size());
		for(i = 0; i < srch->results.size(); i++)
			evbuffer_add_printf(evb, "%u,", srch->results[i]);
		evbuffer_add_printf(evb, "\n");

		evhttp_send_reply(srch->req, HTTP_OK, "OK", evb);

		evbuffer_free(evb);
		delete srch;
	}
}

void handle_search_request(struct evhttp_request *req, void *arg){
	tq_queue_t * request = (tq_queue_t *)arg;

	search * srch = new search();
	const char * ptr;
	struct evkeyvalq searchoptions;


	evhttp_parse_query(req->uri, &searchoptions);


	srch->req = req;

	srch->loc = 0;
	srch->agemin = 14;
	srch->agemax = 60;
	srch->sex = 2;
	srch->active = 1;
	srch->pic = 1;
	srch->single = 0;
	srch->sexuality = 0;

	srch->offset = 0;
	srch->rowcount = 25;

	if((ptr = evhttp_find_header(&searchoptions, "loc")))       srch->loc       = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "agemin")))    srch->agemin    = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "agemax")))    srch->agemax    = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "sex")))       srch->sex       = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "active")))    srch->active    = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "pic")))       srch->pic       = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "single")))    srch->single    = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "sexuality"))) srch->sexuality = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "offset")))    srch->offset    = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "rowcount")))  srch->rowcount  = atoi(ptr);

//	search.verbosePrint();

	tq_push(request, 0, (void * ) & srch);
}

void handle_search_update(struct evhttp_request *req, void *arg){
//	tq_queue_t * updates = (tq_queue_t *)arg;
}

void handle_stats(struct evhttp_request *req, void *arg){
	struct evbuffer *evb;
	evb = evbuffer_new();

	evbuffer_add_printf(evb, "Stuff is good!");
	
	evhttp_send_reply(req, HTTP_OK, "OK", evb);

	evbuffer_free(evb);
}

/*
void * updateRunner(thread_data_t * threaddata){
//	search_t ** searches;
	search_t * search;

	while(threaddata->state){
		search = tq_pop(threaddata->request);

		if(!search)
			break;

		searchUsers(threaddata->data, & search, 1);

		tq_push(threaddata->response, 0, (void *) search);
		signalfd(threaddata->pushfd);
	}

	return NULL;
}
*/


unsigned int get_now(void){
	struct timeval now_tv;
	gettimeofday(&now_tv, NULL);
	return now_tv.tv_sec;
}


void benchmarkSearch(tq_queue_t * request, tq_queue_t * response, unsigned int numsearches){
	printf("Generating %u searches\n", numsearches);

	struct timeval start, finish;
	unsigned int i;
	unsigned int found, returned;
	unsigned int runtime;
	search * srch;

	gettimeofday(&start, NULL);

	for(i = 0; i < numsearches; i++){
		srch = new search();
		srch->rowcount = 25;
		srch->random();

//		srch->verbosePrint();

		tq_push(request, 0, (void * ) srch);
	}


	printf("Counting %u searches\n", numsearches);

	found = returned = 0;

	for(i = 0; i < numsearches; i++){
		srch = (search *)tq_pop(response);

//		srch->verbosePrint();

		found    += srch->totalrows;
		returned += srch->results.size();
	}

	gettimeofday(&finish, NULL);

	runtime = ((finish.tv_sec*1000+finish.tv_usec/1000)-(start.tv_sec*1000+start.tv_usec/1000));

	printf("found:      %u\n",    (unsigned int) found);
	printf("returned:   %u\n",    (unsigned int) returned);
	printf("run time:   %u ms\n", (unsigned int) runtime);
	printf("per search: %.2f ms\n", (float) 1.0*runtime/numsearches);
}

int main(int argc, char **argv){
	unsigned int i;
	char * ptr;

	unsigned int numthreads;
	unsigned int benchruns;
	unsigned int port;

//main search data
	search_data * data;

//3 queues
	tq_queue_t * updates, * request, * response;

//thread stuff
	pthread_t thread[MAX_THREADS];
	thread_data_t threaddata[MAX_THREADS];

//http server stuff
	struct evhttp *http;
	struct event updateEvent;

//notification fds
	int fds[2], pushfd = 0, popfd = 0;


//setup the request and response queues
	updates = tq_init(TQ_FIFO);
	request = tq_init(TQ_FIFO);
	response = tq_init(TQ_FIFO);
//	tq_nonblock(response);


	srand(time(NULL));



	//defaults
	char port_def[] = "8872";
	char hostname_def[] = "localhost";
	char threads_def[] = "3";
	char bench_def[] = "0";

	//Argument Pointers
	char *port_arg = port_def;
	char *hostname_arg = hostname_def;
	char *threads_arg = threads_def;
	char *bench_arg = bench_def;


//Parse command line options
	for (i = 1; i < (unsigned int)argc; i++) {
		ptr = argv[i];
		if(strcmp(ptr, "--help") == 0){
			printf("Usage:\n"
				"\t--help        Show this help\n"
				"\t-p            Port Number [%s]\n"
				"\t-h            Hostname [%s]\n"
				"\t-b            Benchark with number of random searches\n"
				"\t-t            Number of threads [%s]\n\n",
				port_def, hostname_def, threads_def);
			exit(255);
		}else if (strcmp(ptr, "-p") == 0)
			port_arg = ptr = argv[++i];
		else if (strcmp(ptr, "-h") == 0)
			hostname_arg = ptr = argv[++i];
		else if (strcmp(ptr, "-t") == 0)
			threads_arg = ptr = argv[++i];
		else if (strcmp(ptr, "-b") == 0)
			bench_arg = ptr = argv[++i];
	}


	numthreads = atoi(threads_arg);
	if(numthreads < 1)
		numthreads = 1;

	if(numthreads > MAX_THREADS){
		printf("Invalid number of threads '%s', setting to max threads %i\n", threads_arg, MAX_THREADS);
		numthreads = MAX_THREADS;
	}

	benchruns = atoi(bench_arg);
	port = atoi(port_arg);
	if(benchruns == 0 && (atoi(port_arg) < 1 || atoi(port_arg) > 65535)){
		printf("Invalid port number '%s'\n", port_arg);
		return 1;
	}



	printf("Loading user data ... ");
	data = new search_data();
	data->fillUserSearchDump("search.txt");
//	data->fillRand(100000);

	printf("%u users loaded\n", data->userlist.size());

//	data->dumpSearchData(10);



	if(!benchruns){
	//initialize update notification pipe
		socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

		pushfd = fds[0];
		popfd = fds[1];
	}

	printf("Starting %u threads\n", numthreads);

	for(i = 0; i < numthreads; i++){
		threaddata[i].threadid = i;
		threaddata[i].state = 1;
		threaddata[i].pushfd = pushfd;
		threaddata[i].request = request;
		threaddata[i].response = response;
		threaddata[i].data = data;

		pthread_create(&thread[i], NULL, (void* (*)(void*)) searchRunner, (void*) &threaddata[i]);
	}


	if(benchruns){
		benchmarkSearch(request, response, benchruns);
		return 0;
	}



//init the event lib
	event_init();


	printf("Listening on %s:%s\n", hostname_arg, port_arg);

//start the http server
	http = evhttp_start(hostname_arg, atoi(port_arg));
	if(http == NULL) {
		printf("Couldn't start server on %s\n", port_arg);
		return 1;
	}


//Register a callback for requests
//	evhttp_set_cb(http, "/", http_dispatcher_cb, NULL);
	evhttp_set_cb(http, "/search", handle_search_request, request);
	evhttp_set_cb(http, "/update", handle_search_update,  updates);
	evhttp_set_cb(http, "/stats",  handle_stats,          NULL);

	event_set(& updateEvent, popfd, EV_READ|EV_PERSIST, handle_queue_response, response);
	event_add(& updateEvent, 0);


	printf("Starting event loop\n");

	event_dispatch();



	printf("Exiting\n");

	tq_nonblock(request);
	for(i = 0; i < numthreads; i++)
		pthread_join(thread[i], NULL);


	return 0;
}

