#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include "search.h"


search_data::search_data(){
	pthread_rwlock_init(&rwlock, NULL);
}

void search_data::printUser(userid_t userid){
	user_t * user = & userlist[useridmap[userid]];
	printUser(userid, user);
}

void search_data::printUser(userid_t userid, user_t * user){
	printf("%u,%u,%s,%u,%u,%u,%u,%u\n",
		userid, user->age, (user->sex ? "Female" : "Male"), user->loc, user->active, user->pic, user->single, user->sexuality);
}

void search_data::verbosePrintUser(userid_t userid){
	user_t * user = & userlist[useridmap[userid]];
	verbosePrintUser(userid, user);
}

void search_data::verbosePrintUser(userid_t userid, user_t * user){
	printf("Userid: %u, Age: %u, Sex: %s, Loc: %u, Active: %u, Pic: %u, Single: %u, Sexuality: %u\n",
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


uint32_t search_data::setUser(const userid_t userid, const user_t user){
	uint32_t index = 0;

	if(useridmap.size() > userid && useridmap[userid]){ //already exists, put it there
		index = useridmap[userid];
		
		userlist[index] = user;
		//assume usermap and useridmap are already correct
	
	}else if(deluserlist.size()){ //space exists from a previous user
		index = deluserlist.back();
		deluserlist.pop_back();
	
		userlist[index] = user;
		usermap[index] = userid;

		useridmap.reserve(userid+1);
		useridmap[userid] = index;

	}else{ //make space
		index = userlist.size();

		assert(userlist.size() == usermap.size());

		userlist.push_back(user);
		usermap.push_back(userid);
		
		useridmap.reserve(userid+1);
		useridmap[userid] = index;
	}

	return index;
}

bool search_data::delUser(userid_t userid){
	uint32_t index = 0;
	
	if(useridmap.size() < userid || !useridmap[userid]) //doesn't exist
		return false;

	index = useridmap[userid];

	useridmap[userid] = 0;
	usermap[index] = 0;
//	userlist[index] = {0};

	deluserlist.push_back(index);

	return true;
}


void search_data::fillUserSearchDump(char * filename, uint32_t max){

	FILE *input;

	char buf[256];
	user_t user;
	unsigned int userid, id;

	unsigned int loc;
	unsigned int age;
	unsigned int sex;
	unsigned int active;
	unsigned int pic;
	unsigned int single;
	unsigned int sexuality;


	uint32_t i = 0;

	input = fopen(filename, "r");

	if(!input){
		printf("Failed to open file %s\n", filename);
		exit(1);
	}
	

//read the file into the struct.
	fgets(buf, 255, input); //ignore the first line

	while(fgets(buf, 255, input) && (max == 0 || ++i < max)){
		// id,userid,age,sex,loc,active,pic,single,sexualit
		// 56,2080347,19,Female,19,1,1,0,0

	//store in temp variables so that they can be full size
		if( 8 == sscanf(buf, "%u,%u,%u,Male,%u,%u,%u,%u,%u",   & id, & userid, & age, & loc, & active, & pic, & single, & sexuality)){
			sex = 0;
		}else if(8 == sscanf(buf, "%u,%u,%u,Female,%u,%u,%u,%u,%u", & id, & userid, & age, & loc, & active, & pic, & single, & sexuality)){
			sex = 1;
		}else{
			printf("Bad input file\n");
			exit(1);
		}

		user.loc = loc;
		user.age = age;
		user.sex = sex;
		user.active = active;
		user.pic = pic;
		user.single = single;
		user.sexuality = sexuality;

		setUser(userid, user);
	}

	fclose(input);

//	return data;
}


void search_data::fillRand(uint32_t count){
	uint32_t i;
	user_t user;

	for(i = 0; i < count; i++){
		user.loc = 1 + rand() % 300;
		user.age = 14 + rand() % 50;
		user.sex = rand() % 2;
		user.active = rand() % 3;
		user.pic = rand() % 3;
		user.single = rand() % 2;
		user.sexuality = rand() % 4;

		setUser(i+1, user);
	}
}



inline char search_data::matchUser(const unsigned int id, const search * srch){
	user_t * user = & userlist[id];

	return 	user->age >= srch->agemin && user->age <= srch->agemax &&
			(!srch->loc || user->loc == srch->loc) &&
			(srch->sex == 2 || user->sex == srch->sex) &&
			user->active >= srch->active &&
			user->pic    >= srch->pic    &&
			user->single >= srch->single &&
			(!srch->sexuality || user->sexuality == srch->sexuality);
}

void search_data::searchUsers(search * srch){
	unsigned int i;

	srch->totalrows = 0;
	srch->results.reserve(srch->rowcount);

	for(i = 0; i < userlist.size(); i++){
		if(matchUser(i, srch)){
			srch->totalrows++;

			if(srch->totalrows > srch->offset && srch->results.size() < srch->rowcount) //within the search range
				srch->results.push_back(usermap[i]); //append the userid to the results
		}
	}
}





//////////////////////////////////
////////// class search //////////
//////////////////////////////////



void search::print(){
	printf("%u-%u,%s,%u,%u,%u,%u,%u\n",
		agemin, agemax, (sex == 0 ? "Male" : sex == 1 ? "Female" : "Any"), 
		loc, active, pic, single, sexuality);
}

void search::verbosePrint(){
	printf("Age: %u-%u, Sex: %s, Loc: %u, Active: %u, Pic: %u, Single: %u, Sexuality: %u, Searching: %u-%u\n",
		agemin, agemax, (sex == 0 ? "Male" : sex == 1 ? "Female" : "Any"), 
		loc, active, pic, single, sexuality, offset, offset+rowcount);

	if(results.size()){
		printf("Results: %u of %u: ", results.size(), totalrows);
		for(unsigned int i = 0; i < results.size(); i++)
			printf("%u,", results[i]);
		printf("\n");
	}
}

void search::random(){
	loc = 0;//rand() % 300;
	agemin = 14 + rand() % 50;
	agemax = agemin + rand() % 15;
	sex = rand() % 3;
	active = rand() % 3;
	pic = rand() % 3;
	single = rand() % 2;
	sexuality = rand() % 4;

	offset = 0;
}

