
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
 *      - adding/deleting users is O(1)
 *      - very compressed user struct
 *      - scan of users touches ~2.5x memory
 *    - array of users, where the userid is a field, and an array of userid->index
 *      - no wasted space for deleted users
 *      - can be in any sorted order
 *      - adding/deleting users in sorted order may be O(n), O(1) in unsorted order
 *      - big user struct (because of userid field)
 *      - updating/deleting users needs an extra lookup
 *      - scan of users touches 2x memory
 *    - array of users, an array of index->userid, and an array of userid->index
 *      - no wasted space for deleted users
 *      - deleting users is hard, may need to repack the whole working set periodically
 *      - can be in any sorted order
 *      - adding/deleting users in sorted order may be O(n), O(1) in unsorted order
 *      - small user struct
 *      - needs an extra lookup for update/delete and returning results, extra complexity
 *      - scan of user touches 1x memory
 *
 *   Organizing Interests:
 *    - included in the user struct, as a bitmap or list
 *    - bitmap of users for each interest
 *    - list of users for each interest
 *    - bitmap of interests for each user
 *    - list of interests for each user
 *   
 *   
 * Choices:
 *   - list of users, with index->userid lookup and userid->index list
 *   - each interest has a either a bitmap or a sorted list of users
 *
 ************************/

using namespace std;

#include <vector>

#ifndef _SEARCH_H_
#define _SEARCH_H_


typedef uint32_t userid_t;



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


typedef struct {
	



} interest_t;






class search {
public:

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
	unsigned int totalrows;    //total rows matched
	vector<userid_t> results;  //array of userids that match

//request
	struct evhttp_request *req;

	void random();
	void print();
	void verbosePrint();
};



class search_data {
public:
	vector<user_t>     userlist;     //list of user info (age, sex, etc)
	vector<interest_t> interestlist; //list of interest maps

	vector<userid_t> deluserlist; //list of user objs that are empty
	vector<userid_t> usermap;   // user -> userid
	vector<userid_t> useridmap; // userid -> user

	pthread_rwlock_t rwlock;    //read/write lock

	search_data();

	void fillRand(uint32_t count);
	void fillUserSearchDump(char * filename, uint32_t max = 0);

	uint32_t setUser(const userid_t userid, const user_t user);
	bool delUser(userid_t userid);

	void printUser(userid_t userid);
	void printUser(userid_t userid, user_t * user);
	void verbosePrintUser(userid_t userid);
	void verbosePrintUser(userid_t userid, user_t * user);
	
	void dumpSearchData(unsigned int max = 0);

	void searchUsers(search * srch);

private:
	inline char matchUser(const user_t & user, const search & srch);

};


#endif
