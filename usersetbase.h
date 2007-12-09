

#ifndef _USERSET_H_
#define _USERSET_H_

/**********************
 *
 *  implemented as a sorted vector
 *
 **********************/

#include "iterpair.h"

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
#else
	typedef set<index_type> settype;
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

	bool operator == ( const userset & other){
		return (userlist == other.userlist);
	}

	void addUser(index_type val){
		writelock();

#ifdef USERSET_VEC

		iterator loc = lower_bound(userlist.begin(), userlist.end(), val);
		if(loc == userlist.end() || *loc != val)
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

	void union_set(vector<userset *> & others){
		combine(false, others);
	}

	void intersect_set(vector<userset *> & others){
		combine(true, others);
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
		index_type * itend;

#ifdef USERSET_VEC
		if(intersect)
			newlist.resize(min(userlist.size(), other.userlist.size()));
		else
			newlist.resize(userlist.size() + other.userlist.size());

//use &* to get a real pointer instead of an iterator. This makes it mildly faster
#define INSERTER(list) &*list.begin()

#else
#define INSERTER(list) inserter(list, list.begin())
#endif


		readlock();
		other.readlock();

		if(intersect)
			itend = set_intersection(userlist.begin(), userlist.end(), other.begin(), other.end(), INSERTER(newlist));
		else
			itend = set_union(userlist.begin(), userlist.end(), other.begin(), other.end(), INSERTER(newlist));

#ifdef USERSET_VEC
		newlist.resize(itend - &*newlist.begin());
#endif

		other.unlock();
		unlock();

		writelock();
		userlist.swap(newlist);
		unlock();
	}

	void combine(bool intersect, vector<userset *> & lists){
		settype newlist;

		vector <iterpair <userset::iterator> > its;

		vector <iterpair <userset::iterator> >::iterator it;
		vector <iterpair <userset::iterator> >::iterator itend;

		vector<userset *>::iterator listit = lists.begin();
		vector<userset *>::iterator listitend = lists.end();

		readlock();

		unsigned int totalsize = size();
		unsigned int maxsize = size();


		its.push_back(iterpair<userset::iterator>(begin(), end()));

		for(; listit != listitend; ++listit){
			(**listit).readlock();

			totalsize += (**listit).size();
			if(maxsize < (**listit).size())
				maxsize = (**listit).size();

			its.push_back(iterpair<userset::iterator>((**listit).begin(), (**listit).end()));
		}

		it = its.begin();
		itend = its.end();

		index_type cur;

		if(intersect){

			newlist.reserve(maxsize);

			unsigned int numlists = its.size();
			unsigned int found = 0;

			it = its.begin();
			cur = 0;

			while(1){
				while(it->it != it->itend && *(it->it) < cur) //find it in this set
					++(*it);

				if(it->it == it->itend) //one of them reached the end, no chance of matching anymore
					break;

				if(*(it->it) == cur){
					found++;

					if(found == numlists){ //exists in all sets
						newlist.push_back(cur);
						++(*it);

						if(it->it == it->itend)
							break;

						cur = *(it->it);
						found = 1;						
					}
				}else{
					cur = *(it->it);
					found = 1;
				}

				++it; //check the next set
				if(it == its.end())
					it = its.begin();
			}
		}else{
		
			newlist.reserve(totalsize);
		
			while(1){
				cur = 0;

			//find the next smallest	
				for(it = its.begin(); it != itend; ++it)
					if(it->it != it->itend && (cur == 0 || *(it->it) < cur))
						cur = *(it->it);

			//found one, save it and increment all sets that had it
				if(cur){
					newlist.push_back(cur);

					for(it = its.begin(); it != itend; ++it)
						if(it->it != it->itend && *(it->it) == cur)
							++(*it);
				}else{
					break;
				}
			}
		}
		
		
		unlock();

		for(; listit != listitend; ++listit)
			(**listit).unlock();

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

