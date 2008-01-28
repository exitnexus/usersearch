#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include "search.h"

#include <unistd.h>
#include <fcntl.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

int fillibuf(const char * ptr, unsigned int * ibuf, unsigned int numints){
	unsigned int found = 0;
	unsigned int number = 0;

	do{
		while(*ptr >= '0' && *ptr <= '9'){
			number *= 10;
			number += *ptr - '0';
			++ptr;
		}

		*ibuf = number;
		++ibuf;
		number = 0;
		++found;
	}while(found < numints && *ptr++ == ',');

	return found;
}

search_data::search_data(){
	pthread_rwlock_init(&rwlock, NULL);

//add a null user that is skipped in the search, so that useridmap[0] being 0 means no user
	userlist.push_back(user_t());
	usermap.push_back(0);
	useridmap.push_back(0);
}

search_data::~search_data(){
	pthread_rwlock_destroy(&rwlock);
}


//print given only a userid
bool search_data::printUser(userid_t userid){
	if(userid >= useridmap.size() || useridmap[userid] == 0)
		return false;

	user_t * user = & userlist[useridmap[userid]];
	printUser(userid, user);

	return true;
}

bool search_data::verbosePrintUser(userid_t userid){
	if(userid >= useridmap.size() || useridmap[userid] == 0)
		return false;

	user_t * user = & userlist[useridmap[userid]];
	verbosePrintUser(userid, user);

	return true;
}

//print, given a user struct
void search_data::printUser(userid_t userid, user_t * user){
	char buf[100];
	userToString(userid, user, buf);
	printf("%s", buf);
}

void search_data::verbosePrintUser(userid_t userid, user_t * user){
	char buf[100];
	userToStringVerbose(userid, user, buf);
	printf("%s", buf);
}

//userToString with just userid
bool search_data::userToString(userid_t userid, char * buffer){
	if(userid >= useridmap.size() || useridmap[userid] == 0)
		return false;

	user_t * user = & userlist[useridmap[userid]];
	userToString(userid, user, buffer);
	
	return true;
}

bool search_data::userToStringVerbose(userid_t userid, char * buffer){
	if(userid >= useridmap.size() || useridmap[userid] == 0)
		return false;

	user_t * user = & userlist[useridmap[userid]];
	userToStringVerbose(userid, user, buffer);
	
	return true;
}

//actually fill the buffers
void search_data::userToString(userid_t userid, user_t * user, char * buffer){
	sprintf(buffer, "%u,%u,%s,%u,%u,%u,%u,%u\n",
		userid, user->age, (user->sex ? "Female" : "Male"), user->loc, user->active, user->pic, user->single, user->sexuality);
}

void search_data::userToStringVerbose(userid_t userid, user_t * user, char * buffer){
	sprintf(buffer, "Userid: %u, Age: %u, Sex: %s, Loc: %u, Active: %u, Pic: %u, Single: %u, Sexuality: %u\n",
		userid, user->age, (user->sex ? "Female" : "Male"), user->loc, user->active, user->pic, user->single, user->sexuality);

}
	

void search_data::dumpSearchData(unsigned int max){
	unsigned int i, size;

	size = userlist.size();
	max = (max && max+1 < size ? max+1 : size); //+1 to skip the first user, which is a null user

	printf("Dumping %u of %u users\n", max, size);

	for(i = 1; i < max; i++)
		printUser(usermap[i], & userlist[i]);
}


unsigned int search_data::size(){
	return userlist.size();
}

uint32_t search_data::setUser(const userid_t userid, const user_t user){
	uint32_t index = 0;
	bool locchanged = false;

	pthread_rwlock_wrlock(&rwlock);

	if(useridmap.size() > userid && useridmap[userid]){ //already exists, put it there
		index = useridmap[userid];

		locchanged = (userlist[index].loc != user.loc);

		if(locchanged)
			unsetLoc(userid, userlist[index].loc);

		userlist[index] = user;
		//assume usermap and useridmap are already correct

		if(locchanged)
			setLoc(userid, userlist[index].loc);

	}else if(deluserlist.size()){ //space exists from a previous user
		index = deluserlist.back();
		deluserlist.pop_back();

		userlist[index] = user;
		usermap[index] = userid;

		while(useridmap.size() <= userid)
			useridmap.push_back(0);

		useridmap[userid] = index;

		setLoc(userid, userlist[index].loc);

	}else{ //make space
		index = userlist.size();

//		assert(userlist.size() == usermap.size());

		userlist.push_back(user);
		usermap.push_back(userid);

		while(useridmap.size() <= userid)
			useridmap.push_back(0);

		useridmap[userid] = index;

		setLoc(userid, userlist[index].loc);
	}

	pthread_rwlock_unlock(&rwlock);

	return index;
}

void search_data::setInterest(userid_t userid, uint32_t interest){
	if(useridmap.size() > userid && useridmap[userid]){
		while(interestlist.size() <= interest)
			interestlist.push_back( userset() );
	
		interestlist[interest].addUser(useridmap[userid]);
	}
}

void search_data::unsetInterest(userid_t userid, uint32_t interest){
	if(useridmap.size() > userid && useridmap[userid])
		interestlist[interest].deleteUser(useridmap[userid]);
}

void search_data::setLoc(userid_t userid, uint32_t loc){
	userid_t index = 0;

	if(useridmap.size() > userid && useridmap[userid])
		index = useridmap[userid];

	while(index && loc){
		while(loclist.size() <= loc)
			loclist.push_back( userset() );

		loclist[loc].addUser(index);
		loc = locationtree[loc];
	}
}

void search_data::unsetLoc(userid_t userid, uint32_t loc){
	userid_t index = 0;

	if(useridmap.size() > userid && useridmap[userid])
		index = useridmap[userid];

	while(index && loc){
		loclist[loc].deleteUser(index);
		loc = locationtree[loc];
	}
}

bool search_data::updateUser(user_update * upd){
	uint32_t index;

	if(useridmap.size() <= upd->userid || !useridmap[upd->userid]) //doesn't exist, can't update
		return false;

	pthread_rwlock_wrlock(&rwlock);

	index = useridmap[upd->userid];

	switch(upd->field){
		case UF_LOC:
			unsetLoc(upd->userid, userlist[index].loc);
			userlist[index].loc = upd->val;
			setLoc(upd->userid, userlist[index].loc);
			break;
		case UF_AGE:       userlist[index].age       = upd->val; break;
		case UF_SEX:       userlist[index].sex       = upd->val; break;
		case UF_ACTIVE:    userlist[index].active    = upd->val; break;
		case UF_PIC:       userlist[index].pic       = upd->val; break;
		case UF_SINGLE:    userlist[index].single    = upd->val; break;
		case UF_SEXUALITY: userlist[index].sexuality = upd->val; break;
		case UF_ADD_INTEREST:   setInterest(upd->userid, upd->val); break;
		case UF_DEL_INTEREST: unsetInterest(upd->userid, upd->val); break;
	}

	pthread_rwlock_unlock(&rwlock);

	return true;
}

bool search_data::delUser(userid_t userid){
	uint32_t index = 0;
	
	if(useridmap.size() <= userid || !useridmap[userid]) //doesn't exist
		return false;

	pthread_rwlock_wrlock(&rwlock);

	index = useridmap[userid];

	unsetLoc(userid, userlist[index].loc);

	useridmap[userid] = 0;
	usermap[index] = 0;
	userlist[index] = user_t();

	deluserlist.push_back(index);

	pthread_rwlock_unlock(&rwlock);

	return true;
}

bool search_data::loaddata(char * load_loc){
	if(load_loc)
		datasource = load_loc;

	if(!datasource)
		return false;

	if(strlen(datasource) > 6 && strncmp("rand:", datasource, 5) == 0){
		fillRandom(atoi(datasource+5));
		return true;
	}


	loadlocations();
	loadusers();
	loadinterests();

	return true;
}

FILE * search_data::getfd(char * filename){
	FILE * input;
	
	char * path = (char *)malloc(strlen(datasource) + strlen(filename));
	strcpy(path, datasource);
	strcat(path, filename);

//get the fd from the url
	if(strlen(datasource) > 7 && strncmp("http://", datasource, 7) == 0){
		struct hostent *hp;
		struct sockaddr_in addr;
		int on = 1, sock;     

		char * host;
		char * uri;

	//parse the host and uri
		host = path + 7;
		uri = strchr(host, '/')+1; //position one past the / separator between the host and the uri
	
		host[(uri-1-host)] = '\0'; //turn the / separator into a \0
	
	//setup the socket	
		if((hp = gethostbyname(host)) == NULL){
			printf("unknown host: %s\n", host);
			exit(1);
		}
		bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);
		addr.sin_port = htons(80);
		addr.sin_family = AF_INET;
		sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));
		if(sock == -1){
			perror("setsockopt");
			exit(1);
		}
		if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
			perror("connect");
			exit(1);
		}

	//write the request
		char buf[1024];

		sprintf(buf, "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", uri, host);

		write(sock, buf, strlen(buf));

	//get a FILE * from the socket for reading with fgets
		input = fdopen(sock, "r");

		while(fgets(buf, sizeof(buf), input) && buf[0] != '\r'); //ignore the headers, stop when the first char is \r


//get the url from the file path
	}else{
		input = fopen(path, "r");

		if(!input){
			printf("Failed to open file %s\n", path);
			exit(1);
		}
	}

	free(path);

	return input;
}


void search_data::loadlocations(){
	char buf[256];
	unsigned int ibuf[2];
	uint32_t i = 0;

	FILE * input = getfd("locations");

//interest definitions
	fgets(buf, sizeof(buf), input); //first line is the signature
	i++;

	if(strcmp(buf, "id,parent\n") != 0){
		printf("Location file signature doesn't match\n");
		exit(1);
	}

	while(fgets(buf, sizeof(buf), input)){
		i++;

		if(2 != fillibuf(buf, ibuf, 2)){
			printf("Bad location input file on line %u\n", i);
			exit(1);
		}

		while(locationtree.size() <= ibuf[0])
			locationtree.push_back(0);

		locationtree[ibuf[0]] = ibuf[1];
	}
}

//fill the users information
void search_data::loadusers(){
	char buf[256];
	unsigned int ibuf[8];
	uint32_t i = 0;
	
	userid_t userid;
	user_t   user;

	FILE * input = getfd("users");

	fgets(buf, sizeof(buf), input); //ignore the first line

	if(strcmp(buf, "userid,age,sex,loc,active,pic,single,sexuality\n") != 0){
		printf("Users file signature doesn't match\n");
		exit(1);
	}

	while(fgets(buf, sizeof(buf), input)){
		i++;

		if(8 != fillibuf(buf, ibuf, 8)){
			printf("Bad users input file on line %u\n", i);
			exit(1);
		}

		userid         = ibuf[0];
		user.age       = ibuf[1];
		user.sex       = ibuf[2];
		user.loc       = ibuf[3];
		user.active    = ibuf[4];
		user.pic       = ibuf[5];
		user.single    = ibuf[6];
		user.sexuality = ibuf[7];

		setUser(userid, user);
	}
}

//load the users interests
void search_data::loadinterests(){
	char buf[256];
	unsigned int ibuf[2];
	uint32_t i = 0;

	userid_t userid;
	uint32_t interestid;

	FILE * input = getfd("interests");

//interest definitions
	fgets(buf, sizeof(buf), input); //first line is the signature
	i++;

	if(strcmp(buf, "userid,interestid\n") != 0){
		printf("Interest file signature doesn't match\n");
		exit(1);
	}

	while(fgets(buf, sizeof(buf), input)){
		i++;

		if(2 != fillibuf(buf, ibuf, 2)){
			printf("Bad interest input file on line %u\n", i);
			exit(1);
		}

		userid = ibuf[0];
		interestid = ibuf[1];

		setInterest(userid, interestid);
	}
}

void search_data::fillRandom(uint32_t count){
	uint32_t i;
	user_t user;

	locationtree.push_back(0);
	for(i = 1; i <= 300; i++)
		locationtree.push_back(rand()%i);

	for(i = 1; i <= count; i++){
		user.loc = 1 + rand() % 300;
		user.age = 14 + rand() % 50;
		user.sex = rand() % 2;
		user.active = rand() % 3;
		user.pic = rand() % 3;
		user.single = rand() % 2;
		user.sexuality = rand() % 4;

		setUser(i, user);
		
		while(rand()%10 != 0)
			setInterest(i, rand()%300 + 1);
	}
}



inline char search_data::matchUser(const user_t & user, const search_t & srch){
	return 	user.age >= srch.agemin && user.age <= srch.agemax &&
			(srch.sex == 2 || user.sex == srch.sex) &&
			user.active >= srch.active &&
			user.pic    >= srch.pic    &&
			user.single >= srch.single &&
			(!srch.sexuality || user.sexuality == srch.sexuality);
}

void search_data::searchUsers(search_t * srch){

	srch->totalrows = 0;
	srch->results.reserve(srch->rowcount);

	unsigned int i;

	userset::iterator uit, uitend;
	vector<uint32_t>::iterator vit, vitend;

	userset users;
	userset temp;

	bool uselist = false;

//intersect the union of the locations above
	if(srch->locs.size()){
		temp = userset(); //reset to empty

		vit    = srch->locs.begin();
		vitend = srch->locs.end();
	
		for(; vit != vitend; ++vit)
			if(*vit < loclist.size()) //if searching for a location not yet defined, skip it
				temp.union_set(loclist[*vit]);

		if(!uselist)
			users = temp;
		else
			users.intersect_set(temp);

		uselist = true;
	}

//intersect the union or intersection of the interests
	if(srch->interests.size()){
		temp = userset(); //reset to empty

		vit    = srch->interests.begin();
		vitend = srch->interests.end();

		for(; vit != vitend; ++vit){
			if(!srch->allinterests){ //union
				if(*vit < interestlist.size()) // if undefined, skip
					temp.union_set(interestlist[*vit]);
			}else{ //intersect
				if(vit == srch->interests.begin()){ //need to initialize the set
					if(*vit < interestlist.size())
						temp = interestlist[*vit];
					//if undefined, keep the already empty set
				}else{
					if(*vit < interestlist.size())
						temp = userset();  //if undefined, intersection leads to an empty set
					else
						temp.intersect_set(interestlist[*vit]);
				}
			}
		}

		if(!uselist)
			users = temp;
		else
			users.intersect_set(temp);

		uselist = true;
	}


//intersect the union of the social circles
	if(srch->socials.size()){
		temp = userset(); //reset to empty

		vit    = srch->socials.begin();
		vitend = srch->socials.end();


		for(; vit != vitend; ++vit)
			if(*vit < sociallist.size()) //if searching for an undefined social list, skip it
				temp.union_set(sociallist[*vit]);

		if(!uselist)
			users = temp;
		else
			users.intersect_set(temp);

		uselist = true;
	}

//lock userlist
	pthread_rwlock_rdlock(&rwlock);


//limit to a subset of the users, as defined above
	if(uselist){
		uit = users.begin();
		uitend = users.end();
		for(; uit != uitend; ++uit){
			if(matchUser(userlist[*uit], * srch)){
				srch->totalrows++;

				if(srch->totalrows > srch->offset && srch->results.size() < srch->rowcount){ //within the search range
					srch->results.push_back(usermap[*uit]); //append the userid to the results

					if(srch->quick && srch->results.size() == srch->rowcount)
						break;
				}
			}
		}
	}else{
//search over all users
		vector<user_t>::iterator ulit = userlist.begin();
		vector<user_t>::iterator ulitend = userlist.end();

		for(++ulit, i = 1; ulit != ulitend; ++ulit, ++i){ //skip the first because it's a null user
			if(matchUser(* ulit, * srch)){
				srch->totalrows++;

				if(srch->totalrows > srch->offset && srch->results.size() < srch->rowcount){ //within the search range
					srch->results.push_back(usermap[i]); //append the userid to the results

					if(srch->quick && srch->results.size() == srch->rowcount)
						break;
				}
			}
		}
	}

//unlock userlist
	pthread_rwlock_unlock(&rwlock);
}





//////////////////////////////////
////////// class search //////////
//////////////////////////////////



void search_t::print(){
	printf("%u-%u,%s,%u,%u,%u,%u\n",
		agemin, agemax, (sex == 0 ? "Male" : sex == 1 ? "Female" : "Any"), 
		active, pic, single, sexuality);
}

void search_t::verbosePrint(){
	printf("Age: %u-%u, Sex: %s, Active: %u, Pic: %u, Single: %u, Sexuality: %u, Searching: %u-%u\n",
		agemin, agemax, (sex == 0 ? "Male" : sex == 1 ? "Female" : "Any"), 
		active, pic, single, sexuality, offset, offset+rowcount);

	if(results.size()){
		printf("Results: %u of %u: ", (unsigned int)results.size(), totalrows);
		for(unsigned int i = 0; i < results.size(); i++)
			printf("%u,", results[i]);
		printf("\n");
	}
}

void search_t::random(){
	agemin = 14 + rand() % 50;
	agemax = agemin + rand() % 15;
	sex = rand() % 3;
	active = rand() % 3;
	pic = rand() % 3;
	single = rand() % 2;
	sexuality = rand() % 4;

	for(int i = rand() % 20; i >= 0; i--)
		locs.push_back((rand() % 300) + 1);


//	interests = rand() % 300 + 1;

	offset = 0;
}

