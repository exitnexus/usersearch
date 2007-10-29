
/************************
 * Assumptions:
 *   - searches will have the following criteria:
 *     - age range
 *     - optional sex
 *     - optional small location list (ie not 500 options, but 1-20)
 *     - optional active
 *     - optional picture/sign pic
 *     - optional single
 *     - optional sexuality
 *     - optional interests, one of
 *       - single interest
 *       - small list of interests:
 *         - must have all
 *         - must have at least one
 *     - page size
 *     - sort order
 *     - start offset 
 *
 * Options: 
 *   Organizing Users:
 *    - single array of users, where the index is the userid
 *      - userids of deleted users take space
 *      - must be in userid order
 *      - adding/deleting users is easy
 *      - very compressed user struct
 *    - single array of users, where the userid is a field
 *      - no wasted space for deleted users
 *      - can be in any sorted order
 *      - adding/deleting users in sorted order may be hard
 *      - big user struct (because of userid field)
 *    - array of users and an array of index->userid
 *      - no wasted space for deleted users
 *      - deleting users is hard, may need to repack the whole working set periodically 
 *      - can be in any sorted order
 *      - small user struct
 *      - needs an extra lookup, extra complexity
 *
 *   Organizing Interests:
 *    - included in the user struct, as a bitmap or list
 *    - bitmap of users with each interest
 *    - bitmap of interests for each user
 *    - list of selected interests for each user
 *    - list of users for each interest
 *   
 *   
 * Choices:
 *   - list of users, with index->userid lookup
 *   - each interest has a either a bitmap or a sorted list of users
 *
 ************************/


#ifndef _SEARCH_H_
#define _SEARCH_H_


typedef uint32_t userid_t;


#define PAGESIZE 25


#define BIT_PACK_USER_STRUCT 1


#if BIT_PACK_USER_STRUCT

typedef struct {
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
	uint16_t loc;            // location id
	unsigned char age;       // age (14-60)
	unsigned char sex;       // 0 : male, 1 : female
	unsigned char active;    // 0 : inactive, 1 : active, 2 : online
	unsigned char pic;       // 0 : no pic, 1 : pic, 2 : sign pic
	unsigned char single;    // 0 : not single, 1 : single
	unsigned char sexuality; // 0 : unknown, 1 : hetero, 2 : homo, 3 : bi
} user_t;

#endif



typedef struct { //search_t
//criteria
	uint16_t loc;
	unsigned char agemin, agemax;
	unsigned char sex;
	unsigned char active;
	unsigned char pic;
	unsigned char single;
	unsigned char sexuality;
//	uint16_t interest;

//limits
	unsigned int rowcount; // ie find 25 results
	unsigned int offset;   //    starting from row 100

//results
	unsigned int totalrows; //total rows matched
	unsigned int returnedrows; //num rows returned
	userid_t * results; //array of userids that match
} search_t;



typedef struct {
	uint32_t size;          //current number of entries
	uint32_t maxid;         //max number of entries
	userid_t * usermapping; //offset to userid mapping
	user_t * userlist;      //list of users

	// interests, not yet implemented

} search_data_t;






void printUser(userid_t userid, user_t * user);
void verbosePrintUser(userid_t userid, user_t * user);
void dumpSearchData(search_data_t * data, int max);
search_data_t * initUserSearch(unsigned int maxentries);
search_data_t * initUserSearchDump(char * filename, uint32_t max);
search_data_t * initUserSearchRand(uint32_t count);
char matchUser(search_data_t * data, unsigned int id, search_t * search);
void searchUsers(search_data_t * data, search_t ** searches, unsigned int numsearches);
void printSearch(search_t * search);
void verbosePrintSearch(search_t * search);
void dumpSearchParams(search_t ** searches, unsigned int numsearches);
search_t ** generateSearch(unsigned int numsearches);





#endif
