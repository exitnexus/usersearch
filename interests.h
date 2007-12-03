

#ifndef _USERSET_H_
#define _USERSET_H_


/**********************
 *
 *  implemented as an unsorted vector
 *
 **********************/

using namespace std;

#include <vector>

typedef vector<uint32_t>::iterator userset_iter;

class userset {
	vector<uint32_t> userlist;

public:
//	userset();

	void addUser(uint32_t index){
		userlist.push_back(index);
	}

	void deleteUser(uint32_t index){
//		userlist.remove(index);

		interest_iter iit, iitend;

		for(iit = userlist.begin(), iitend = userlist.end(); iit != iitend; iit++){
			if(*iit == index){
				*iit = userlist.back(); //replace with the last element
				userlist.pop_back();    // (no need to shuffle all the elements down by one)
			}
		}
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

