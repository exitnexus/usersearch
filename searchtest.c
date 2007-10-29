#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

/************************
 * Organizing Users:
 * -single array of users, where the index is the userid
 *   - userids of deleted users take space
 *   - must be in userid order
 *   - adding/deleting users is easy
 *   - very compressed user struct
 * -single array of users, where the userid is a field
 *   - no wasted space for deleted users
 *   - can be in any sorted order
 *   - adding/deleting users in sorted order may be hard
 *   - big user struct
 * -array of users and an array of index->userid
 *   - no wasted space for deleted users
 *   - can be in any sorted order
 *   - small user struct
 *   - needs an extra lookup, extra complexity
 *
 * Organizing Interests:
 * -included in the user struct, as a bitmap or list
 * -bitmap of users with each interest
 * -bitmap of interests for each user
 * -list of selected interests for each user
 *
 ************************/

#define MAXUSERS 3000000
#define NUM_LOOPS 1000
#define NUM_SEARCHES 1
#define NUM_INTEREST_BITS 320 //number of bits per userid
#define PAGESIZE 25
#define BIT_PACK_USER_STRUCT 1

#if 1

#define INTEREST_WORD_SIZE 32
#define INTEREST_TYPE uint32_t
#define INTEREST_SHIFT 5
#define INTEREST_MASK 31

#else

#define INTEREST_WORD_SIZE 64
#define INTEREST_TYPE uint64_t
#define INTEREST_SHIFT 6
#define INTEREST_MASK 63

#endif

#define INTEREST_WORDS NUM_INTEREST_BITS / INTEREST_WORD_SIZE //number of words of interests per user


#define CHECK_INTEREST(userid, interestid) \
	(( *(userinterestlist + (userid * INTEREST_WORDS) + (interestid >> INTEREST_SHIFT))) & (1 << (interestid & INTEREST_MASK)))

#define SET_INTEREST(userid, interestid) \
	( *(userinterestlist + ((userid) * INTEREST_WORDS) + ((interestid) >> INTEREST_SHIFT))) |= (1 << ((interestid) & INTEREST_MASK))

#define UNSET_INTEREST(userid, interestid) \
	( *(userinterestlist + ((userid) * INTEREST_WORDS) + ((interestid) >> INTEREST_SHIFT))) &= ~(1 << ((interestid) & INTEREST_MASK))

#define CLEAR_INTEREST(userid) \
	int clear_interest_counter; \
	for(clear_interest_counter = 0; clear_interest_counter < INTEREST_WORDS; clear_interest_counter++) \
		( *(userinterestlist + ((userid) * INTEREST_WORDS) + clear_interest_counter) = 0;



#if BIT_PACK_USER_STRUCT

typedef struct {
//	uint32_t userid;
	uint16_t loc;
	unsigned char age;
	unsigned int sex       :1 ;
	unsigned int active    :2 ;
	unsigned int pic       :2 ;
	unsigned int single    :1 ;
	unsigned int sexuality :2 ;
} user_t;

#else

typedef struct {
//	uint32_t userid;
	uint16_t loc;
	unsigned char age;
	unsigned char sex;
	unsigned char active;
	unsigned char pic;
	unsigned char single;
	unsigned char sexuality;
} user_t;

#endif


typedef struct { //search_t
//criteria
	uint16_t loc;
	unsigned char age, age2;
	unsigned char sex;
	unsigned char active;
	unsigned char pic;
	unsigned char single;
	unsigned char sexuality;
	uint16_t interest;

//limits
	unsigned int rowcount; // ie find 25 results
	unsigned int offset;   //    starting from row 100

//results
	unsigned int totalrows; //total rows matched
	unsigned int returnedrows; //num rows returned
	uint32_t * results; //array of userids that match
} search_t;


	uint32_t maxuserid;
	user_t * userlist;
	INTEREST_TYPE * userinterestlist;






char matchUser(unsigned int userid, search_t * search){

	user_t * user = & userlist[userid];

	return 	//user->userid &&
			user->age >= search->age && user->age <= search->age2 &&
			(!search->loc || user->loc == search->loc) &&
			(!search->sex == 2 || user->sex == search->sex) &&
			user->active >= search->active &&
			user->pic    >= search->pic    &&
			user->single >= search->single &&
			(!search->sexuality || user->sexuality == search->sexuality)
			 &&
			(!search->interest || CHECK_INTEREST(userid, search->interest) );

}


void searchUsers(search_t ** searches, unsigned int numsearches){
	unsigned int userid, searchnum;
	search_t * search;

	for(searchnum = 0; searchnum < numsearches; searchnum++){
		searches[searchnum]->totalrows = 0;
		searches[searchnum]->returnedrows = 0;
	}

	for(userid = 1; userid < maxuserid; userid++){
		for(searchnum = 0; searchnum < numsearches; searchnum++){
			search = searches[searchnum];

			if(matchUser(userid, search)){
				search->totalrows++;
	
				if(search->totalrows >= search->offset && search->returnedrows < search->rowcount){
					search->results[search->returnedrows] = userid;
					search->returnedrows++;
				}
			}
		}
	}
}

void initRand(){
	int i, a, b, c;
	user_t *user;

	for(i = 0; i < MAXUSERS; i++){
		user = & userlist[i];
//		user->userid = i;
		user->loc = 1 + rand() % 300;
		user->age = 14 + rand() % 50;
		user->sex = rand() % 2;
		user->active = rand() % 3;
		user->pic = rand() % 3;
		user->single = rand() % 2;
		user->sexuality = rand() % 4;

		for(a = 0; a < NUM_INTEREST_BITS/8; a++){
			c = rand();
			for(b = 0; b < 8; b++){
				if(c & 1)
					SET_INTEREST(i, a*8+b);
				c >>= 1;
			}
		}
	}
	maxuserid = MAXUSERS;
}

void initUserSearchDump(char * filename){

	FILE *input = fopen(filename, "r");
	
	char buf[100];
	user_t user;
	uint32_t userid, id;
	
	while(fgets(&buf, 99, input)){
//	id,userid,age,sex,loc,active,pic,single,sexualit
//	56,2080347,19,Female,19,1,1,0,0

		if(sscanf(buf, "%u,%u,%u,Male,%u,%u,%u,%u,%u", & id, & userid, & user.age, & user.loc, & user.active, & user.pic, & user.single, & user.sexuality)){
			user.sex = 0;
		}else if(sscanf(buf, "%u,%u,%u,Female,%u,%u,%u,%u,%u", & id, & userid, & user.age, & user.loc, & user.active, & user.pic, & user.single, & user.sexuality){
			user.sex = 1;
		}else{
			printf("Bad input file\n");
			exit 1;
		}

		if(userid > MAXUSERS){
			printf("userid %u is bigger than the maximum %u\n", userid, MAXUSERS);
			exit 1;
		}

		if(userid > maxuserid)
			maxuserid = userid;

		memcpy(& userlist[userid], & user, sizeof(user_t));
	}
}

search_t ** generateSearch(int numsearches){
	search_t ** searches;
	search_t * search;
	int i;

	searches = calloc(numsearches, sizeof(search_t *));

	for(i = 0; i < numsearches; i++){
		search = malloc(sizeof(search_t));
	
		search->results = calloc(PAGESIZE, sizeof(uint32_t));

		search->loc = rand() % 300;
		search->age = 14 + rand() % 50;
		search->age2 = search->age + rand() % 15;
		search->sex = rand() % 3;
		search->active = rand() % 3;
		search->pic = rand() % 3;
		search->single = rand() % 2;
		search->sexuality = rand() % 4;
		search->interest = 0;//rand() % NUM_INTEREST_BITS;

		search->rowcount = PAGESIZE;
		search->offset = 0;

		searches[i] = search;
	}
	
	return searches;
}




int main(){

	printf("user_t size: %u\n", (unsigned int) sizeof(user_t));
	printf("interest_t size: %u\n", (unsigned int) sizeof(INTEREST_TYPE));
	printf("search_t size: %u\n", (unsigned int)sizeof(search_t));

	maxuserid = 0;
	userlist = calloc(MAXUSERS, sizeof(user_t));
	userinterestlist = calloc(MAXUSERS, INTEREST_WORDS*sizeof(INTEREST_TYPE) );
	

	search_t ** searches;
	
	clock_t start, init, finish;

	srand(time(NULL));


	uint32_t a, i, found, returned;


	start = clock();

	initRand();

	init = clock();

	found = returned = 0;
	
	for(a = 0; a < NUM_LOOPS; a++){
		searches = generateSearch(NUM_SEARCHES);

		searchUsers(searches, NUM_SEARCHES);
		
		for(i = 0; i < NUM_SEARCHES; i++){
			found    += searches[i]->totalrows;
			returned += searches[i]->returnedrows;
		}
	}

	finish = clock();

	printf("found: %u\n", (unsigned int) found);
	printf("returned: %u\n", (unsigned int) returned);
	printf("\n");
	printf("init: %u ms\n", (unsigned int) (1000*(init-start)/CLOCKS_PER_SEC));
	printf("find: %u ms\n", (unsigned int) (1000*(finish-init)/CLOCKS_PER_SEC));
	

	return 0;
}

/*
<?

	$fp = fopen("/home/nexopia/search.txt", 'w');
	
	$res = $usersdb->query("SELECT * FROM usersearch");
	
	$str = "";
	
	$line = $res->fetchrow();
	$str .= implode(",", array_keys($line)) . "\n";
	$str .= implode(",", $line) . "\n";
	
	$i = 0;
	
	while($line = $res->fetchrow()){
		$str .= implode(",", $line) . "\n";

		if(++$i >= 10000){
			fwrite($fp, $str);
			$str = "";
			$i = 0;
		}
	}
	fwrite($fp, $str);
	fclose($fp);
*/

