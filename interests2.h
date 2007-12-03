

#ifndef _USERSET_H_
#define _USERSET_H_

/**********************
 *
 *  implemented with the sparse bit array
 *
 **********************/

using namespace std;

#include "sparsebitarray.h"

typedef sparse_bit_array_iter userset_iter;

class userset {
	sparse_bit_array userlist;

public:
//	interests();

	void addUser(uint32_t index){
		userlist.setbit(index);
	}

	void deleteUser(uint32_t index){
		userlist.unsetbit(index);
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

