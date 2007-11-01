#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#include "search.h"



void printUser(userid_t userid, user_t * user){
	printf("%u,%u,%s,%u,%u,%u,%u,%u\n",
		userid, user->age, (user->sex ? "Female" : "Male"), user->loc, user->active, user->pic, user->single, user->sexuality);
}

void verbosePrintUser(userid_t userid, user_t * user){
	printf("Userid: %u, Age: %u, Sex: %s, Loc: %u, Active: %u, Pic: %u, Single: %u, Sexuality: %u\n",
		userid, user->age, (user->sex ? "Female" : "Male"), user->loc, user->active, user->pic, user->single, user->sexuality);
}


void dumpSearchData(search_data_t * data, unsigned int max){
	unsigned int i;

	printf("Dumping %u of %u users\n", (max && max < data->size ? max : data->size), data->size);

	for(i = 0; i < data->size && (max == 0 || i < max); i++)
		printUser(data->usermapping[i], & data->userlist[i]);
}


search_data_t * initUserSearch(unsigned int maxentries){
	printf("Initializing with space for %u users\n", maxentries);

	search_data_t * data = (search_data_t *)malloc(sizeof(search_data_t));

	data->maxid = maxentries;
	data->size = 0;
	data->usermapping = (userid_t *)calloc(maxentries, sizeof(userid_t));
	data->userlist = (user_t *)calloc(maxentries, sizeof(user_t));

	return data;
}

search_data_t * initUserSearchDump(char * filename, uint32_t max){

	search_data_t * data;

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


	uint32_t i, count = 0;

	input = fopen(filename, "r");

	if(!input){
		printf("Failed to open file %s\n", filename);
		exit(1);
	}

//count the number of lines
	while(fgets(buf, 255, input))
		count++;

	rewind(input);

	if(max && count > max)
		count = max;

//initialize the data struct
	data = initUserSearch(count);
	

//read the file into the struct.
	i = 0;
	
	fgets(buf, 255, input); //ignore the first line

	while(fgets(buf, 255, input) && data->size < data->maxid){
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

		memcpy(& data->userlist[data->size], & user, sizeof(user_t));
		data->usermapping[data->size] = userid;

		data->size++;
	}

	if(data->size == data->maxid)
		printf("space is too small to store this file\n");

	fclose(input);

	return data;
}


search_data_t * initUserSearchRand(uint32_t count){
	uint32_t i;
	user_t *user;
	search_data_t * data;

	data = initUserSearch(count);

	for(i = 0; i < count; i++){
		data->size++;

		data->usermapping[i] = i+1;

		user = & data->userlist[i];

		user->loc = 1 + rand() % 300;
		user->age = 14 + rand() % 50;
		user->sex = rand() % 2;
		user->active = rand() % 3;
		user->pic = rand() % 3;
		user->single = rand() % 2;
		user->sexuality = rand() % 4;
	}

	return data;
}



inline char matchUser(const search_data_t * data, const unsigned int id, const search_t * search){
	user_t * user = & data->userlist[id];

	return 	user->age >= search->agemin && user->age <= search->agemax &&
			(!search->loc || user->loc == search->loc) &&
			(search->sex == 2 || user->sex == search->sex) &&
			user->active >= search->active &&
			user->pic    >= search->pic    &&
			user->single >= search->single &&
			(!search->sexuality || user->sexuality == search->sexuality);
}

void searchUsers(search_data_t * data, search_t ** searches, unsigned int numsearches){
	unsigned int i, searchnum;
	search_t * search;

	for(searchnum = 0; searchnum < numsearches; searchnum++){
		searches[searchnum]->totalrows = 0;
		searches[searchnum]->returnedrows = 0;
	}

	for(searchnum = 0; searchnum < numsearches; searchnum++){
		search = searches[searchnum];

		for(i = 0; i < data->size; i++){
			if(matchUser(data, i, search)){
				search->totalrows++;

				if(search->totalrows > search->offset && search->returnedrows < search->rowcount){
					search->results[search->returnedrows] = data->usermapping[i];
					search->returnedrows++;
				}
			}
		}
	}
}

void printSearch(search_t * search){
	printf("%u-%u,%s,%u,%u,%u,%u,%u\n",
		search->agemin, search->agemax, 
		(search->sex == 0 ? "Male" : search->sex == 1 ? "Female" : "Any"), 
		search->loc, 
		search->active, 
		search->pic, 
		search->single, 
		search->sexuality);
}

void verbosePrintSearch(search_t * search){
	printf("Age: %u-%u, Sex: %s, Loc: %u, Active: %u, Pic: %u, Single: %u, Sexuality: %u, Searching: %u-%u\n",
		search->agemin, search->agemax, 
		(search->sex == 0 ? "Male" : search->sex == 1 ? "Female" : "Any"), 
		search->loc, 
		search->active, 
		search->pic, 
		search->single, 
		search->sexuality,
		search->offset,
		search->offset+search->rowcount
		);
	
	if(search->returnedrows){
		printf("Results: %u of %u: ", search->returnedrows, search->totalrows);
		unsigned int i;
		for(i = 0; i < search->returnedrows; i++)
			printf("%u,", search->results[i]);
		printf("\n");
	}
}


void dumpSearchParams(search_t ** searches, unsigned int numsearches){
	unsigned int i;

	for(i = 0; i < numsearches; i++)
		verbosePrintSearch(searches[i]);
}



search_t ** generateSearch(unsigned int numsearches, unsigned int pagesize){
	search_t ** searches;
	search_t * search;
	unsigned int i;

	searches = (search_t **)calloc(numsearches, sizeof(search_t *));

	for(i = 0; i < numsearches; i++){
		search = initSearch(pagesize);

		search->loc = 0;//rand() % 300;
		search->agemin = 14 + rand() % 50;
		search->agemax = search->agemin + rand() % 15;
		search->sex = rand() % 3;
		search->active = rand() % 3;
		search->pic = rand() % 3;
		search->single = rand() % 2;
		search->sexuality = rand() % 4;

		search->offset = 0;

		searches[i] = search;
	}

	return searches;
}


search_t * initSearch(unsigned int pagesize){
	search_t * search;

	search = (search_t *)calloc(1, sizeof(search_t));

	search->results = (userid_t*)calloc(pagesize, sizeof(userid_t));
	search->rowcount = pagesize;

	return search;
}

void destroySearch(search_t * search){
	free(search->results);
	free(search);
}
