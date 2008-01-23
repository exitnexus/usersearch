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
	max = (max && max < size ? max : size);

	printf("Dumping %u of %u users\n", max, size);

	for(i = 0; i < max; i++)
		printUser(usermap[i], & userlist[i]);
}


unsigned int search_data::size(){
	return userlist.size();
}

uint32_t search_data::setUser(const userid_t userid, const user_t user){
	uint32_t index = 0;

	while(loclist.size() <= user.loc)
		loclist.push_back( userset() );

	pthread_rwlock_wrlock(&rwlock);

	if(useridmap.size() > userid && useridmap[userid]){ //already exists, put it there
		index = useridmap[userid];

		userlist[index] = user;
		//assume usermap and useridmap are already correct

	}else if(deluserlist.size()){ //space exists from a previous user
		index = deluserlist.back();
		deluserlist.pop_back();

		userlist[index] = user;
		usermap[index] = userid;

		while(useridmap.size() < userid)
			useridmap.push_back(0);

		useridmap[userid] = index;

	}else{ //make space
		index = userlist.size();

//		assert(userlist.size() == usermap.size());

		userlist.push_back(user);
		usermap.push_back(userid);

		while(useridmap.size() < userid)
			useridmap.push_back(0);

		useridmap[userid] = index;
	}

	loclist[user.loc].addUser(index);

	pthread_rwlock_unlock(&rwlock);

	return index;
}

void search_data::setInterest(userid_t index, uint32_t interest){
	while(interestlist.size() <= interest)
		interestlist.push_back( userset() );

	interestlist[interest].addUser(index);
}

void search_data::unsetInterest(userid_t index, uint32_t interest){
	interestlist[interest].deleteUser(index);
}

bool search_data::updateUser(user_update * upd){
	uint32_t index;

	if(useridmap.size() < upd->userid || !useridmap[upd->userid]) //doesn't exist, can't update
		return false;

	pthread_rwlock_wrlock(&rwlock);

	index = useridmap[upd->userid];

	switch(upd->field){
		case UF_LOC:
			loclist[userlist[index].loc].deleteUser(upd->userid); //unset the previous
			userlist[index].loc = upd->val;
			loclist[userlist[index].loc].addUser(upd->userid);
			break;
		case UF_AGE:       userlist[index].age       = upd->val; break;
		case UF_SEX:       userlist[index].sex       = upd->val; break;
		case UF_ACTIVE:    userlist[index].active    = upd->val; break;
		case UF_PIC:       userlist[index].pic       = upd->val; break;
		case UF_SINGLE:    userlist[index].single    = upd->val; break;
		case UF_SEXUALITY: userlist[index].sexuality = upd->val; break;
		case UF_ADD_INTEREST:   setInterest(index, upd->val);    break;
		case UF_DEL_INTEREST: unsetInterest(index, upd->val);    break;
	}

	pthread_rwlock_unlock(&rwlock);

	return true;
}

bool search_data::delUser(userid_t userid){
	uint32_t index = 0;
	
	if(useridmap.size() < userid || !useridmap[userid]) //doesn't exist
		return false;

	pthread_rwlock_wrlock(&rwlock);

	index = useridmap[userid];

	useridmap[userid] = 0;
	usermap[index] = 0;
	userlist[index] = user_t();

	deluserlist.push_back(index);

	pthread_rwlock_unlock(&rwlock);

	return true;
}


void search_data::fillSearchFile(char * filename){
	FILE * input;

	input = fopen(filename, "r");

	if(!input){
		printf("Failed to open file %s\n", filename);
		exit(1);
	}

	fillSearchFd(input);

	fclose(input);
}


void search_data::fillSearchStdin(){
	fillSearchFd(stdin);
}

void search_data::fillSearchFd(FILE * input){
	char buf[256];
	unsigned int ibuf[100];
	uint32_t i = 0;

	userid_t userid;
	uint32_t interestid;

//user definitions
	fgets(buf, sizeof(buf), input); //ignore the first line

	while(fgets(buf, sizeof(buf), input)){
		i++;

		if(buf[0] == '\n') //blank line marks the end of this section
			break;

		if(!parseUserBuf(buf)){
			printf("Bad input file on line %u\n", i);
			exit(1);
		}
	}

//interest definitions
	fgets(buf, sizeof(buf), input); //ignore the first line
	i++;

	while(fgets(buf, sizeof(buf), input)){
		i++;

		if(2 != fillibuf(buf, ibuf, 2)){
			printf("Bad input file on line %u\n", i);
			exit(1);
		}

		userid = ibuf[0];
		interestid = ibuf[1];

		if(useridmap[userid])
			setInterest(useridmap[userid], interestid);
	}
}

//only takes urls of the form: http://domain.com/uri
//https and port arne't valid
void search_data::fillSearchUrl(char * url){

	struct hostent *hp;
	struct sockaddr_in addr;
	int on = 1, sock;     

	char * host;
	char * uri;

//parse the host
	host = strdup(url+7);
	char * pos = strchr(host, '/');

	host[(pos-host)] = '\0';

//parse the uri
	uri = strdup(url + 7 + (pos-host));


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
	char buf[256];
	
	sprintf(buf, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", uri, host);

	write(sock, buf, strlen(buf));

//get a FILE * from the socket for reading with fgets
	FILE * fsock = fdopen(sock, "r");

	while(fgets(buf, sizeof(buf), fsock) && buf[0] != '\r')
		if(buf[0] == '\r')
			break;

	fillSearchFd(fsock);

	fclose(fsock);
	free(host);
	free(uri);
}

userid_t search_data::parseUserBuf(char *buf){
	user_t user;
	unsigned int userid, id;

	unsigned int loc;
	unsigned int age;
	unsigned int sex;
	unsigned int active;
	unsigned int pic;
	unsigned int single;
	unsigned int sexuality;

	unsigned int ibuf[100];

	// id,userid,age,sex,loc,active,pic,single,sexualit
	// 56,2080347,19,1,19,1,1,0,0

//store in temp variables so that they can be full size

	if(9 != fillibuf(buf, ibuf, 9))
		return 0;

	id        = ibuf[0];
	userid    = ibuf[1];
	age       = ibuf[2];
	sex       = ibuf[3];
	loc       = ibuf[4];
	active    = ibuf[5];
	pic       = ibuf[6];
	single    = ibuf[7];
	sexuality = ibuf[8];

	//ignore id
	user.age = age;
	user.sex = sex;
	user.loc = loc;
	user.active = active;
	user.pic = pic;
	user.single = single;
	user.sexuality = sexuality;

	setUser(userid, user);

	return userid;
}



void search_data::fillRand(uint32_t count){
	uint32_t i;
	uint32_t index;
	user_t user;

	for(i = 0; i < count; i++){
		user.loc = 1 + rand() % 300;
		user.age = 14 + rand() % 50;
		user.sex = rand() % 2;
		user.active = rand() % 3;
		user.pic = rand() % 3;
		user.single = rand() % 2;
		user.sexuality = rand() % 4;

		index = setUser(i+1, user);
		
		while(rand()%10 != 0)
			setInterest(index, rand()%300 + 1);
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
		for(; uit != uitend; uit++){
			if(matchUser(userlist[*uit], * srch)){
				srch->totalrows++;

				if(srch->totalrows > srch->offset && srch->results.size() < srch->rowcount) //within the search range
					srch->results.push_back(usermap[*uit]); //append the userid to the results
			}
		}
	}else{
//search over all users
		vector<user_t>::iterator ulit = userlist.begin();
		vector<user_t>::iterator ulitend = userlist.end();

		for(i = 0; ulit != ulitend; ulit++, i++){
			if(matchUser(* ulit, * srch)){
				srch->totalrows++;

				if(srch->totalrows > srch->offset && srch->results.size() < srch->rowcount) //within the search range
					srch->results.push_back(usermap[i]); //append the userid to the results
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

