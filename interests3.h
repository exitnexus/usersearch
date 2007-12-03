

#ifndef _USERSET_H_
#define _USERSET_H_

/**********************
 *
 *  implemented as a sorted vector
 *
 **********************/
 
using namespace std;

#include <vector>
#include <algorithm>


class userset {

public:
	typedef uint32_t index_type;
	typedef vector<index_type>::iterator iterator;

private:
	vector<index_type> userlist;
	mutable pthread_rwlock_t rwlock;    //read/write lock


public:
	userset(){
		pthread_rwlock_init(&rwlock, NULL);
	}

	~userset(){
		pthread_rwlock_destroy(&rwlock);
	}

	userset( const userset & other){
		pthread_rwlock_init(&rwlock, NULL);

		writelock();
		other.readlock();

		userlist = other.userlist;

		other.unlock();
		unlock();
	}

	userset & operator = ( const userset & other){
		if(this != & other){
			pthread_rwlock_destroy(&rwlock);
			pthread_rwlock_init(&rwlock, NULL);
	
			writelock();
			other.readlock();
	
			userlist = other.userlist;
	
			other.unlock();
			unlock();
		}
		
		return *this;
	}

	void addUser(index_type val){
		writelock();

		iterator loc = lower_bound(userlist.begin(), userlist.end(), val);
		userlist.insert(loc, val);

		unlock();
	}

	void deleteUser(index_type val){
		writelock();

		iterator loc = lower_bound(userlist.begin(), userlist.end(), val);
	
		if(*loc == val)
			userlist.erase(loc);
		
		unlock();
	}

	unsigned int size(void) const {
		readlock();
		unsigned int temp = userlist.size();
		unlock();
		return temp;
	}

	void swap(userset & other){
		writelock();
		other.writelock();

		userlist.swap(other.userlist);

		other.unlock();
		unlock();
	}

	void union_set(userset & other){
		combine(false, other);
	}

	void intersect_set(userset & other){
		combine(true, other);
	}

	iterator begin(){
		return userlist.begin();
	}

	iterator end(){
		return userlist.end();
	}

private:

	void combine(bool intersect, userset & other){
		vector<index_type> newlist;

		readlock();
		other.readlock();

		if(intersect)
			set_intersection(userlist.begin(), userlist.end(), other.begin(), other.end(),  back_inserter(newlist));
		else
			set_union(userlist.begin(), userlist.end(), other.begin(), other.end(),  back_inserter(newlist));

		other.unlock();
		unlock();

		writelock();
		userlist.swap(newlist);
		unlock();
	}

	void readlock() const{
		pthread_rwlock_rdlock(&rwlock);
	}

	void writelock() const{
		pthread_rwlock_wrlock(&rwlock);
	}

	void unlock() const{
		pthread_rwlock_unlock(&rwlock);
	}
};

#endif

