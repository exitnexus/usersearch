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
		user.active = (active == 2 ? 3 : active);
		user.pic = (pic == 2 ? 3 : pic);
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






void printbits(const char * prefix, userint_t bits, unsigned int size){
	int i;
	
	printf("%s", prefix);
	for(i = size*8 - 1; i >= 0; i--)
		printf("%u", ((bits >> i) & 1));
	printf("\n");
}



void searchUsers2(search_data_t * data, search_t ** searches, unsigned int numsearches){
	unsigned int i, searchnum;
	search_t * search;
	user_t user;

	userint_t userbits, setbits, unsetbits;


	for(searchnum = 0; searchnum < numsearches; searchnum++){
		searches[searchnum]->totalrows = 0;
		searches[searchnum]->returnedrows = 0;
	}





	for(searchnum = 0; searchnum < numsearches; searchnum++){
		search = searches[searchnum];


//bits to check are 0
//bits to check are 1


#if BIT_PACK_USER_STRUCT
	//32 bits
		setbits = 0;
		unsetbits = 0;

		switch(search->sex){
			case 0:  unsetbits |= 1 << 7; break; // male,   looking for a 0
			case 1:    setbits |= 1 << 7; break; // female, looking for a 1
			case 2:                       break; // either
		}

		switch(search->active){
			case 0:                       break; //anything
			case 1:    setbits |= 1 << 5; break; //active
			case 2:    setbits |= 3 << 5; break; //online
		}

		switch(search->pic){
			case 0:                       break; //anything
			case 1:    setbits |= 1 << 3; break; //pic
			case 2:    setbits |= 3 << 3; break; //sign pic
		}

		switch(search->single){
			case 0:                       break; //either
			case 1:    setbits |= 1 << 2; break; //single
		}

		switch(search->sexuality){
			case 0:                       break; //any
			case 1:    setbits |= 1 << 0;        //hetero
			         unsetbits |= 2 << 0; break;
			case 2:    setbits |= 2 << 0;        //homo
			         unsetbits |= 1 << 0; break;
			case 3:    setbits |= 3 << 0; break; //bi
		}

#else
	//64bits
		setbits = 0;
		unsetbits = 0;

		switch(search->sex){
			case 0:  unsetbits |= 1 << (8*4); break; // male,   looking for a 0
			case 1:    setbits |= 1 << (8*4); break; // female, looking for a 1
			case 2:                           break; // either
		}

		switch(search->active){
			case 0:                           break; //anything
			case 1:    setbits |= 2 << (8*3); break; //active
			case 2:    setbits |= 3 << (8*3); break; //online
		}

		switch(search->pic){
			case 0:                           break; //anything
			case 1:    setbits |= 1 << (8*2); break; //pic
			case 2:    setbits |= 3 << (8*2); break; //sign pic
		}

		switch(search->single){
			case 0:                           break; //either
			case 1:    setbits |= 1 << (8*1); break; //single
		}

		switch(search->sexuality){
			case 0:                           break; //any
			case 1:    setbits |= 1 << (8*0);        //hetero
			         unsetbits |= 2 << (8*0); break;
			case 2:    setbits |= 2 << (8*0);        //homo
			         unsetbits |= 1 << (8*0); break;
			case 3:    setbits |= 3 << (8*0); break; //bi
		}


#endif

//verbosePrintSearch(search);
//printf("set: %X, unset: %X\n", setbits, unsetbits);
//exit(255);


		for(i = 0; i < data->size; i++){
			user = data->userlist[i];
			userbits = *((userint_t *) & user); //ugly hack to get access to the bits

//printbits("user:  ", userbits, sizeof(userbits));
//printbits("set:   ", setbits, sizeof(setbits));
//printbits("unset: ", unsetbits, sizeof(unsetbits));
			if(
				(user.age >= search->agemin && user.age <= search->agemax) && 
				(!search->loc || user.loc == search->loc) &&
				((userbits & setbits) == setbits) && 
				((~userbits & unsetbits) == unsetbits)
				){

//	printf("fail\n\n");
//else
//	printf("match\n\n");
//			continue;

				search->totalrows++;
	
				if(search->totalrows > search->offset && search->returnedrows < search->rowcount){
					search->results[search->returnedrows] = data->usermapping[i];
					search->returnedrows++;
				}
			}
		}
	}
	
//exit(255);
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
