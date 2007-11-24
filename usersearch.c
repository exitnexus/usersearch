
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


struct search_stats {
	unsigned int starttime;
	unsigned int search;
	unsigned int adduser;
	unsigned int updateuser;
	unsigned int deleteuser;
};

struct global_data {
	int pushfd;
	int popfd;
	search_stats stats;
	tqueue<user_update> * updates;
	tqueue<search_t> * request;
	tqueue<search_t> * response;
	search_data * data;
};

struct thread_data_t {
/*	thread_data_t(){
		threadid = 0;
		state = 0;
		global = NULL;
	}
*/	unsigned int threadid;
	char state;
	global_data * global;
};


unsigned int get_now(void){
	struct timeval now_tv;
	gettimeofday(&now_tv, NULL);
	return now_tv.tv_sec;
}

//to tell the main thread that there is new stuff in the queue
void signalfd(int fd){
	const char signalbuf[1] = { 0 };
	if(fd)
		write(fd, signalbuf, 1);
}

void * searchRunner(thread_data_t * threaddata){
	search_t * srch;
	global_data * global = threaddata->global;

	while(threaddata->state){
		srch = global->request->pop();

		if(srch){
			global->data->searchUsers(srch);
	
			global->response->push(srch);
			signalfd(global->pushfd);
		}
	}

	return NULL;
}


void * updateRunner(thread_data_t * threaddata){
	user_update * upd;
	global_data * global = threaddata->global;

	while(threaddata->state){
		upd = global->updates->pop();

		if(upd){
			switch(upd->op){
				case USER_ADD:
					global->data->setUser(upd->userid, upd->user);
					break;

				case USER_UPDATE:
					global->data->updateUser(upd);
					break;

				case USER_DELETE:
					global->data->delUser(upd->userid);
					break;
			}

			delete upd;
		}
	}

	return NULL;
}




void handle_queue_searchresponse(int fd, short event, void *arg){
	global_data * global = (global_data *) arg;
	search_t * srch;

	struct evbuffer *evb;

	unsigned int i;

	char buf[64];
	read(fd, buf, sizeof(buf));

	while((srch = global->response->pop(0))){ //pop a response in non-block mode
		evb = evbuffer_new();

		evbuffer_add_printf(evb, "totalrows: %u\n", srch->totalrows);
		evbuffer_add_printf(evb, "returnedrows: %u\n", srch->results.size());
		evbuffer_add_printf(evb, "userids: ", srch->results.size());
		for(i = 0; i < srch->results.size(); i++)
			evbuffer_add_printf(evb, "%u,", srch->results[i]);
		evbuffer_add_printf(evb, "\n");

		evhttp_send_reply(srch->req, HTTP_OK, "OK", evb);

		evbuffer_free(evb);
		delete srch;
	}
}

//handles /search?...
void handle_request_search(struct evhttp_request *req, void *arg){
	global_data * global = (global_data *) arg;

	global->stats.search++;

	search_t * srch = new search_t();
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
	srch->interest = 0;

	srch->offset = 0;
	srch->rowcount = 25;
	srch->totalrows = 0;

	if((ptr = evhttp_find_header(&searchoptions, "agemin")))    srch->agemin    = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "agemax")))    srch->agemax    = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "sex")))       srch->sex       = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "loc")))       srch->loc       = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "active")))    srch->active    = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "pic")))       srch->pic       = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "single")))    srch->single    = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "sexuality"))) srch->sexuality = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "interest")))  srch->interest  = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "offset")))    srch->offset    = atoi(ptr);
	if((ptr = evhttp_find_header(&searchoptions, "rowcount")))  srch->rowcount  = atoi(ptr);

//	srch.verbosePrint();

	global->request->push(srch);
	evhttp_clear_headers(&searchoptions);
}


void handle_request_printuser(struct evhttp_request *req, void *arg){
	global_data * global = (global_data *) arg;

	const char * ptr;
	userid_t userid;
	char buf[100];

	struct evkeyvalq searchoptions;
	struct evbuffer * evb = evbuffer_new();

	evhttp_parse_query(req->uri, &searchoptions);

	ptr = evhttp_find_header(&searchoptions, "userid");

	if(ptr){
		userid = atoi(ptr);

		if(global->data->userToStringVerbose(userid, buf))
			evbuffer_add_printf(evb, "%s", buf);
		else
			evbuffer_add_printf(evb, "Bad Userid\r\n");
	}else{
		evbuffer_add_printf(evb, "FAIL\r\n");
	}

	evhttp_send_reply(req, HTTP_OK, "OK", evb);
	evhttp_clear_headers(&searchoptions);
	evbuffer_free(evb);
}

void push_update(global_data * global, userid_t userid, userfield field, uint32_t val){
	user_update * upd = new user_update;

	upd->op = USER_UPDATE;
	upd->userid = userid;

	upd->field = field;
	upd->val = val;

	global->updates->push(upd);
}

void push_update_interest(global_data * global, userid_t userid, userfield field, const char * origstr){
	char * newstr = strdup(origstr);
	char * ptr, * next;
	
	ptr = newstr;

	do{
		next = strchr(ptr, ',');

		if(next)
			*next = '\0';

		push_update(global, userid, field, atoi(ptr));

		ptr = next+1;
	}while(next);

	free(newstr);
}

//handles /updateuser?userid=<userid>&age=<age>&....
void handle_request_updateuser(struct evhttp_request *req, void *arg){
	global_data * global = (global_data *) arg;

	global->stats.updateuser++;

	const char * ptr;
	userid_t userid;

	struct evkeyvalq searchoptions;

	struct evbuffer * evb = evbuffer_new();

	evhttp_parse_query(req->uri, &searchoptions);

	ptr = evhttp_find_header(&searchoptions, "userid");

	if(ptr){
		userid = atoi(ptr);

		if((ptr = evhttp_find_header(&searchoptions, "age")))       push_update(global, userid, UF_AGE,       atoi(ptr));
		if((ptr = evhttp_find_header(&searchoptions, "sex")))       push_update(global, userid, UF_SEX,       atoi(ptr));
		if((ptr = evhttp_find_header(&searchoptions, "loc")))       push_update(global, userid, UF_LOC,       atoi(ptr));
		if((ptr = evhttp_find_header(&searchoptions, "active")))    push_update(global, userid, UF_ACTIVE,    atoi(ptr));
		if((ptr = evhttp_find_header(&searchoptions, "pic")))       push_update(global, userid, UF_PIC,       atoi(ptr));
		if((ptr = evhttp_find_header(&searchoptions, "single")))    push_update(global, userid, UF_SINGLE,    atoi(ptr));
		if((ptr = evhttp_find_header(&searchoptions, "sexuality"))) push_update(global, userid, UF_SEXUALITY, atoi(ptr));

		if((ptr = evhttp_find_header(&searchoptions, "addinterests"))) push_update_interest(global, userid, UF_ADD_INTEREST, ptr);
		if((ptr = evhttp_find_header(&searchoptions, "delinterests"))) push_update_interest(global, userid, UF_DEL_INTEREST, ptr);

		evbuffer_add_printf(evb, "SUCCESS\r\n");
	}else{
		evbuffer_add_printf(evb, "FAIL\r\n");
	}

	evhttp_send_reply(req, HTTP_OK, "OK", evb);
	evhttp_clear_headers(&searchoptions);
	evbuffer_free(evb);
}


//handles /adduser?userid=<userid>&age=<age>&....
void handle_request_adduser(struct evhttp_request *req, void *arg){
	global_data * global = (global_data *) arg;

	global->stats.adduser++;

	user_t   user;
	userid_t userid = 0;

	int error = 0;

	user_update * upd;

	struct evkeyvalq searchoptions;
	struct evbuffer * evb = evbuffer_new();
	const char * ptr;

	evhttp_parse_query(req->uri, &searchoptions);

	if((ptr = evhttp_find_header(&searchoptions, "userid")))    userid         = atoi(ptr); else error = 1;
	if((ptr = evhttp_find_header(&searchoptions, "age")))       user.age       = atoi(ptr); else error = 1;
	if((ptr = evhttp_find_header(&searchoptions, "sex")))       user.sex       = atoi(ptr); else error = 1;
	if((ptr = evhttp_find_header(&searchoptions, "loc")))       user.loc       = atoi(ptr); else error = 1;
	if((ptr = evhttp_find_header(&searchoptions, "active")))    user.active    = atoi(ptr); else error = 1;
	if((ptr = evhttp_find_header(&searchoptions, "pic")))       user.pic       = atoi(ptr); else error = 1;
	if((ptr = evhttp_find_header(&searchoptions, "single")))    user.single    = atoi(ptr); else error = 1;
	if((ptr = evhttp_find_header(&searchoptions, "sexuality"))) user.sexuality = atoi(ptr); else error = 1;

	if(!error){
		upd = new user_update;

		upd->op = USER_ADD;
		upd->userid = userid;
		upd->user = user;

		global->updates->push(upd);

		evbuffer_add_printf(evb, "SUCCESS\r\n");
	}else{
		evbuffer_add_printf(evb, "FAIL\r\n");
	}

	evhttp_send_reply(req, HTTP_OK, "OK", evb);
	evhttp_clear_headers(&searchoptions);
	evbuffer_free(evb);
}


// handles /deleteuser?userid=<userid>
void handle_request_deleteuser(struct evhttp_request *req, void *arg){
	global_data * global = (global_data *) arg;

	global->stats.deleteuser++;

	user_update * upd;

	struct evkeyvalq searchoptions;
	struct evbuffer *evb = evbuffer_new();
	const char * ptr;

	evhttp_parse_query(req->uri, &searchoptions);

	ptr = evhttp_find_header(&searchoptions, "userid");

	if(ptr){
		upd = new user_update;

		upd->op = USER_DELETE;
		upd->userid = atoi(ptr);

		global->updates->push(upd);

		evbuffer_add_printf(evb, "SUCCESS\r\n");
	}else{
		evbuffer_add_printf(evb, "FAIL\r\n");
	}

	evhttp_send_reply(req, HTTP_OK, "OK", evb);
	evhttp_clear_headers(&searchoptions);
	evbuffer_free(evb);
}


void handle_request_stats(struct evhttp_request *req, void *arg){
	global_data * global = (global_data *) arg;

	struct evbuffer *evb;
	evb = evbuffer_new();

	evbuffer_add_printf(evb, "Uptime:      %u\n", (get_now() - global->stats.starttime));
	evbuffer_add_printf(evb, "Search:      %u\n", global->stats.search);
	evbuffer_add_printf(evb, "Add User:    %u\n", global->stats.adduser);
	evbuffer_add_printf(evb, "Update User: %u\n", global->stats.updateuser);
	evbuffer_add_printf(evb, "Delete User: %u\n", global->stats.deleteuser);
	evbuffer_add_printf(evb, "Num Users:   %u\n", global->data->size());

	evhttp_send_reply(req, HTTP_OK, "OK", evb);

	evbuffer_free(evb);
}

void handle_request_help(struct evhttp_request *req, void *arg){
	struct evbuffer *evb;
	evb = evbuffer_new();

	evbuffer_add_printf(evb, 
	"This is the user search daemon\n"
	"Commands:\n"
	"  /search?... params: [agemin,agemax,sex,loc,active,pic,single,sexuality,offset,rowcount]\n"
	"  /adduser?... params: userid,age,sex,loc,active,pic,single,sexuality\n"
	"  /updateuser?... params: userid[,age,sex,loc,active,pic,single,sexuality]\n"
	"  /deleteuser?... params: userid\n"
	"  /printuser?...  params: userid\n"
	"  /stats\n"
	"  /help\n"
	);
	
	evhttp_send_reply(req, HTTP_OK, "OK", evb);

	evbuffer_free(evb);
}

void benchmarkSearch(global_data * global, unsigned int numsearches){
	printf("Generating %u searches\n", numsearches);

	struct timeval start, finish;
	unsigned int i;
	unsigned int found, returned;
	unsigned int runtime;
	search_t * srch;

	gettimeofday(&start, NULL);

	for(i = 0; i < numsearches; i++){
		srch = new search_t();
		srch->rowcount = 25;
		srch->random();

//		srch->verbosePrint();

		global->request->push(srch);
	}


	printf("Counting %u searches\n", numsearches);

	found = returned = 0;

	for(i = 0; i < numsearches; i++){
		srch = global->response->pop();

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

	global_data * global = new global_data;

	global->stats.starttime = get_now();

//main search data
	global->data = new search_data;

//3 queues
	global->updates  = new tqueue<user_update>();
	global->request  = new tqueue<search_t>();
	global->response = new tqueue<search_t>();

//thread stuff
	pthread_t thread[MAX_THREADS];
	thread_data_t threaddata[MAX_THREADS];
	//memset(thread, 0, sizeof(pthread_t)*MAX_THREADS);
	//memset(threaddata, 0, sizeof(thread_data_t)*MAX_THREADS);
//http server stuff
	struct evhttp *http;
	struct event updateEvent;


//notification fds
	int fds[2];


	srand(time(NULL));



	//defaults
	char port_def[] = "8872";
	char hostname_def[] = "0.0.0.0";
	char threads_def[] = "3";
	char bench_def[] = "0";
	char load_loc_def[] = "-";

	//Argument Pointers
	char *port_arg = port_def;
	char *hostname_arg = hostname_def;
	char *threads_arg = threads_def;
	char *bench_arg = bench_def;
	char *load_loc = load_loc_def;


//Parse command line options
	for (i = 1; i < (unsigned int)argc; i++) {
		ptr = argv[i];
		if(strcmp(ptr, "--help") == 0){
			printf("Usage:\n"
				"\t--help        Show this help\n"
				"\t-l            Load data from this location (rand:num, url, filename or - for stdin)\n"
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
		else if (strcmp(ptr, "-l") == 0)
			load_loc = ptr = argv[++i];
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
	if(load_loc[0] == '-')
		global->data->fillSearchStdin();
	else if(strlen(load_loc) > 7 && strncmp("http://", load_loc, 7) == 0)
		global->data->fillSearchUrl(load_loc);
	else if(strlen(load_loc) > 6 && strncmp("rand:", load_loc, 5) == 0)
		global->data->fillRand(atoi(load_loc+5));
	else
		global->data->fillSearchFile(load_loc);

	printf("%u users loaded\n", (unsigned int) global->data->size());

//	data->dumpSearchData(10);


	if(!benchruns){
	//initialize update notification pipe
		socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

		global->pushfd = fds[0];
		global->popfd = fds[1];
	}

	printf("Starting %u threads\n", numthreads);

	for(i = 0; i < numthreads; i++){
		threaddata[i].threadid = i;
		threaddata[i].state = 1;
		threaddata[i].global = global;
		
		pthread_create(&thread[i], NULL, (void* (*)(void*)) searchRunner, (void*) &threaddata[i]);
	}
	
	threaddata[i].threadid = i;
	threaddata[i].state = 1;
	threaddata[i].global = global;
	pthread_create(&thread[i], NULL, (void* (*)(void*)) updateRunner, (void*) &threaddata[i]);


	if(benchruns){
		benchmarkSearch(global, benchruns);
		return 0;
	}



//init the event lib
	event_init();


	printf("Listening on %s:%s\n", hostname_arg, port_arg);

//start the http server
	http = evhttp_start(hostname_arg, atoi(port_arg));
	if(http == NULL) {
		printf("Couldn't start server on %s:%s\n", hostname_arg, port_arg);
		return 1;
	}


//Register a callback for requests
//	evhttp_set_cb(http, "/", http_dispatcher_cb, NULL);
	evhttp_set_cb(http, "/search",     handle_request_search,     global);

	evhttp_set_cb(http, "/updateuser", handle_request_updateuser, global);
	evhttp_set_cb(http, "/adduser",    handle_request_adduser,    global);
	evhttp_set_cb(http, "/deleteuser", handle_request_deleteuser, global);

	evhttp_set_cb(http, "/printuser",  handle_request_printuser,  global);

	evhttp_set_cb(http, "/stats",      handle_request_stats,      global);
	evhttp_set_cb(http, "/help",       handle_request_help,       NULL);

	event_set(& updateEvent, global->popfd, EV_READ|EV_PERSIST, handle_queue_searchresponse, global);
	event_add(& updateEvent, 0);


	printf("Starting event loop\n");

	event_dispatch();



	printf("Exiting\n");

	global->request->nonblock();
	for(i = 0; i < numthreads; i++)
		pthread_join(thread[i], NULL);


	return 0;
}

