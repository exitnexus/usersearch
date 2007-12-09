
#ifndef _ITERPAIR_H_
#define _ITERPAIR_H_

template <class T>
struct iterpair {
	T it;
	T itend;

	iterpair(){
	}

	iterpair(T _it, T _itend){
		it = _it;
		itend = _itend;
	}

	iterpair(const iterpair<T> & _it){
		it = _it.it;
		itend = _it.itend;
	}

	bool done(){
		return it == itend;
	}

	void operator ++(){ //prefix form
		++it;
	}

	void operator ++(int){ //prefix form
		++it;
	}
};


#endif
