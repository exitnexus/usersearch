

#ifndef _USERSET_H_
#define _USERSET_H_

/**********************
 *
 *  implemented as an stl set
 *
 **********************/
 
using namespace std;

#include <set>

typedef set<uint32_t>::iterator userset_iter;

class userset {
	set<uint32_t> userlist;

public:
//	interests();

	void addUser(uint32_t val){
		userlist.insert(val);
	}

	void deleteUser(uint32_t val){
		userlist.erase(val);
	}

	unsigned int size(void) const {
		return userlist.size();
	}

	userset_iter begin(){
		return userlist.begin();
	}

	userset_iter end(){
		return userlist.end();
	}
};

#endif

