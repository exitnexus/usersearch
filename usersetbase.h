

#ifndef _USERSET_H_
#define _USERSET_H_

/**********************
 *
 *  implemented as a sorted vector
 *
 **********************/

#include "iterpair.h"

using namespace std;

#include <vector>
#include <algorithm>


class userset {


public:
	typedef uint32_t index_type;

	typedef vector<index_type> settype;
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

	void printlist(unsigned int id){
		readlock();

		iterator it = userlist.begin();
		iterator itend = userlist.end();

		printf("Id %u, size %u: ", id, (unsigned int)userlist.size());

		for(; it != itend; ++it)
			printf("%u ", *it);

		printf("\n");

		unlock();
	}

	void addUser(index_type val){
		writelock();

		iterator loc = lower_bound(userlist.begin(), userlist.end(), val);
		if(loc == userlist.end() || *loc != val)
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

		if(intersect)
			newlist.resize(min(userlist.size(), other.userlist.size()));
		else
			newlist.resize(userlist.size() + other.userlist.size());

		if(newlist.size() == 0)
			return;

		readlock();
		other.readlock();

//use &* for the back_inserter to get a real pointer instead of an iterator. This makes it mildly faster
		if(intersect)
			itend = set_intersection(userlist.begin(), userlist.end(), other.begin(), other.end(), &*newlist.begin());
		else
			itend = set_union(userlist.begin(), userlist.end(), other.begin(), other.end(), &*newlist.begin());

		newlist.resize(itend - &*newlist.begin());

		other.unlock();
		unlock();

		writelock();
		userlist.swap(newlist);
		unlock();
	}

	void combine(bool intersect, vector<userset *> & lists){
		settype newlist;
		index_type * newit;

		vector <iterpair <userset::iterator> > its;

		vector <iterpair <userset::iterator> >::iterator it;
		vector <iterpair <userset::iterator> >::iterator itend;

		vector<userset *>::iterator listit = lists.begin();
		vector<userset *>::iterator listitend = lists.end();

		readlock();

		unsigned int totalsize = size();
		unsigned int minsize = size();


		its.push_back(iterpair<userset::iterator>(begin(), end()));

		for(; listit != listitend; ++listit){
			(**listit).readlock();

			totalsize += (**listit).size();
			if(minsize > (**listit).size())
				minsize = (**listit).size();

			its.push_back(iterpair<userset::iterator>((**listit).begin(), (**listit).end()));
		}

		it = its.begin();
		itend = its.end();

		index_type cur;

		if(intersect){

	//intersect!!!!
	
			if(minsize == 0)
				return;

	
			newlist.resize(minsize);
			newit = & *newlist.begin();

			unsigned int numlists = its.size();
			unsigned int found = 0;
			unsigned int skip = 0;

			it = its.begin();
			cur = 0;

			skip = 10;//numlists/4;

			while(1){

			//skip ahead more quickly, sometimes helpful, sometimes not...
				while((it->it) + skip < it->itend && *((it->it) + skip) < cur) //find it in this set
					it->it += skip + 1;

				while(it->it != it->itend && *(it->it) < cur) //find it in this set
					++(*it);

				if(it->it == it->itend) //one of them reached the end, no chance of matching anymore
					break;

				if(*(it->it) == cur){
					found++;

					if(found == numlists){ //exists in all sets
						*newit = cur;
						++newit;

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

	//union!!!!

			if(totalsize == 0)
				return;

			newlist.resize(totalsize);
			newit = & *newlist.begin();

			it = its.begin(); 
			while(it != its.end()){
				if(it->it == it->itend){
					it = its.erase(it);
					continue;
				}
				++it;
			}

		
			while(1){

			//find the next smallest
				it = its.begin();
				cur = *(it->it);

				for(++it; it != its.end(); ++it)
					if(*(it->it) < cur)
						cur = *(it->it);

			//found one, save it and increment all sets that had it
				*newit = cur;
				++newit;					

				it = its.begin();
				while(it != its.end()){
					if(*(it->it) == cur){
						++(*it);

						if(it->it == it->itend){
							it = its.erase(it);
							continue;
						}
					}
					++it;
				}
				if(its.size() == 0)
					break;
			}
		}


		unlock();

		for(; listit != listitend; ++listit)
			(**listit).unlock();

		writelock();

		newlist.resize(newit - &*newlist.begin());

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

