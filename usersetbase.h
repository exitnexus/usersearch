

#ifndef _USERSET_H_
#define _USERSET_H_

/**********************
 *
 *  implemented as a sorted vector
 *
 **********************/



#define USERSET_VEC 1
//#define USERSET_SET 1




 
using namespace std;



#ifdef USERSET_VEC
#include <vector>
#else
#include <set>
#endif


#include <algorithm>


class userset {


public:
	typedef uint32_t index_type;

#ifdef USERSET_VEC
	typedef vector<index_type> settype;
#define INSERTER(list) back_inserter(list)
#else
	typedef set<index_type> settype;
#define INSERTER(list) inserter(list, list.begin())
#endif


	typedef settype::iterator iterator;

private:
	settype userlist;
	mutable pthread_rwlock_t rwlock; //read/write lock


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

#ifdef USERSET_VEC

		iterator loc = lower_bound(userlist.begin(), userlist.end(), val);
		if(*loc != val)
			userlist.insert(loc, val);

#else //USERSET_SET

		userlist.insert(val);

#endif
		unlock();
	}

	void deleteUser(index_type val){
		writelock();

#ifdef USERSET_VEC

		iterator loc = lower_bound(userlist.begin(), userlist.end(), val);

		if(*loc == val)
			userlist.erase(loc);

#else
		userlist.erase(val);

#endif
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
		settype newlist;

#ifdef USERSET_VEC
		if(intersect)
			newlist.reserve(min(userlist.size(), other.userlist.size()));
		else
			newlist.reserve(userlist.size() + other.userlist.size());
#endif

		readlock();
		other.readlock();

		if(intersect)
			set_intersection(userlist.begin(), userlist.end(), other.begin(), other.end(), INSERTER(newlist));
		else
			set_union(userlist.begin(), userlist.end(), other.begin(), other.end(), INSERTER(newlist));

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

