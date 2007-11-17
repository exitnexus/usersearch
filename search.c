#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include "search.h"


search_data::search_data(){
	pthread_rwlock_init(&rwlock, NULL);
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

	return index;
}

bool search_data::updateUser(user_update * upd){
	uint32_t index;

	if(useridmap.size() < upd->userid || !useridmap[upd->userid]) //doesn't exist, can't update
		return false;

	index = useridmap[upd->userid];

	switch(upd->field){
		case UF_LOC:       userlist[index].loc       = upd->val; break;
		case UF_AGE:       userlist[index].age       = upd->val; break;
		case UF_SEX:       userlist[index].sex       = upd->val; break;
		case UF_ACTIVE:    userlist[index].active    = upd->val; break;
		case UF_PIC:       userlist[index].pic       = upd->val; break;
		case UF_SINGLE:    userlist[index].single    = upd->val; break;
		case UF_SEXUALITY: userlist[index].sexuality = upd->val; break;
		case UF_INTEREST:  break;
	}

	return true;
}

bool search_data::delUser(userid_t userid){
	uint32_t index = 0;
	
	if(useridmap.size() < userid || !useridmap[userid]) //doesn't exist
		return false;

	index = useridmap[userid];

	useridmap[userid] = 0;
	usermap[index] = 0;
	userlist[index] = user_t();

	deluserlist.push_back(index);

	return true;
}


void search_data::fillSearchFile(char * filename, uint32_t max){

	FILE *input;
	char buf[256];
	uint32_t i = 0;

	input = fopen(filename, "r");

	if(!input){
		printf("Failed to open file %s\n", filename);
		exit(1);
	}
	
//read the file into the struct.
	fgets(buf, 255, input); //ignore the first line

	while(fgets(buf, 255, input) && (max == 0 || ++i < max)){

		if(!parseBuf(buf)){
			printf("Bad input file on line %u\n", i);
			exit(1);
		}
	}

	fclose(input);

//	return data;
}


void search_data::fillSearchStdin(){
	char buf[256];
	uint32_t i = 0;

//read the file into the struct.
	fgets(buf, 255, stdin); //ignore the first line

	while(fgets(buf, 255, stdin)){
		i++;

		if(!parseBuf(buf)){
			printf("Bad input file on line %u\n", i);
			exit(1);
		}
	}
}


void search_data::fillSearchUrl(char * url){
/*
	FILE *input;
	char buf[256];
	uint32_t i = 0;

	input = fopen(filename, "r");

	if(!input){
		printf("Failed to open file %s\n", filename);
		exit(1);
	}
	
//read the file into the struct.
	fgets(buf, 255, input); //ignore the first line

	while(fgets(buf, 255, input) && (max == 0 || ++i < max)){

		if(!parseBuf(buf)){
			printf("Bad input file on line %u\n", i);
			exit(1);
		}
	}

	fclose(input);

//	return data;
*/
}

userid_t search_data::parseBuf(char *buf){
	user_t user;
	unsigned int userid, id;

	unsigned int loc;
	unsigned int age;
	unsigned int sex;
	unsigned int active;
	unsigned int pic;
	unsigned int single;
	unsigned int sexuality;

	// id,userid,age,sex,loc,active,pic,single,sexualit
	// 56,2080347,19,1,19,1,1,0,0

//store in temp variables so that they can be full size
	if(9 != sscanf(buf, "%u,%u,%u,%u,%u,%u,%u,%u,%u", & id, & userid, & age, & sex, & loc, & active, & pic, & single, & sexuality))
		return 0;

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



inline char search_data::matchUser(const user_t & user, const search & srch){
	return 	user.age >= srch.agemin && user.age <= srch.agemax &&
			(!srch.loc || user.loc == srch.loc) &&
			(srch.sex == 2 || user.sex == srch.sex) &&
			user.active >= srch.active &&
			user.pic    >= srch.pic    &&
			user.single >= srch.single &&
			(!srch.sexuality || user.sexuality == srch.sexuality);
}

void search_data::searchUsers(search * srch){
	unsigned int i;
	vector<user_t>::iterator it, end;

	srch->totalrows = 0;
	srch->results.reserve(srch->rowcount);

	for(i = 0, it = userlist.begin(), end = userlist.end(); it != end; it++, i++){
		if(matchUser(* it, * srch)){
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
		printf("Results: %u of %u: ", (unsigned int)results.size(), totalrows);
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

