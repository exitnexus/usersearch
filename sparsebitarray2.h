

#ifndef _SPARSEBITARRAY_H_
#define _SPARSEBITARRAY_H_


using namespace std;

#include <string>
#include <list>
#include <assert.h>

/***********
 * The rledata is formed as a series of blocks. Each block has the format:
 * - top bit represents what this block is encoding (ie run of 0s or 1s)
 * - next 2 bits represent the length of this block, in terms of additional bytes
 *    (ie 0,1,2,3 additional bytes, for a total of 1,2,3,4 bytes)
 * - remainder of the block (varying between 5 bits and 29 bits) represents the run length
 **********/


class sparse_bit_array {

/***********************
 **** struct block *****
 ***********************/

	struct block {
		unsigned int value;        //value of the key (ie offset in the bit array)
		unsigned int bitvalue;     //value of the bits in the run
		unsigned int blocklength;  //length of memory used by this block (1-4 bytes)
		unsigned int runlength;    //run length
		string::iterator rleptr;  //iterator to start of this block

		block(){
			block(0,0);
		}

		block(unsigned int _bitvalue, unsigned int _runlength){
			value = 0;
			blocklength = 0;

			bitvalue = _bitvalue;
			runlength = _runlength;
		}

		block(string::iterator _rleptr, unsigned int _value){
			rleptr = _rleptr;
			value   = _value;

			bitvalue    = ((*rleptr & (1 << 7)) >> 7);     //grab the 7th bit
			blocklength = ((*rleptr & (3 << 5)) >> 5) + 1; //grab the 5th and 6th bit, +1 to give the full block length

			runlength = *rleptr & 0x1F;

			switch(blocklength){ // acts as an unrolled loop
				case 4: runlength |= ((*(rleptr + 3) & 0xFF) << 21);
				case 3: runlength |= ((*(rleptr + 2) & 0xFF) << 13);
				case 2: runlength |= ((*(rleptr + 1) & 0xFF) <<  5);
				case 1: break; //already decoded above
			}
		}

		string encode(){
			string ret;

			if(     runlength <= 0x00001F) blocklength = 1; // 5 bits of 1s
			else if(runlength <= 0x001FFF) blocklength = 2; //13 bits of 1s
			else if(runlength <= 0x1FFFFF) blocklength = 3; //21 bits of 1s
			else                           blocklength = 4;

			assert( bitvalue    >= 0 && bitvalue    <= 1);
			assert( blocklength >= 1 && blocklength <= 4);
			assert( runlength   >= 1 && runlength   <= ((1 << 29) - 1)); //min of 1, max of 29 bits long

			ret = string(blocklength, '\0');

			ret[0] = 0;
			ret[0] |= ((bitvalue & 1) << 7);    //store the bit value
			ret[0] |= ((blocklength - 1) << 5); //store block length. The -1 is to store 1-4 as 0-3 so it fits in 2 bits
			ret[0] |= runlength & 0x1F;         //store the first 5 bits of the run length 

			switch(blocklength){
				case 4: ret[3] |= ((runlength & (0xFF << 21)) >> 21);
				case 3: ret[2] |= ((runlength & (0xFF << 13)) >> 13);
				case 2: ret[1] |= ((runlength & (0xFF <<  5)) >>  5);
				case 1: break;
			}

			return ret;
		}

		string::iterator blockbegin(){
			return rleptr;
		}

		string::iterator blockend(){
			return rleptr + blocklength;
		}

		block next(){
			return block(blockend(), value + runlength);
		}
	};

/***************************
 **** end struct block *****
 ***************************/


/***********************
 **** struct frame *****
 ***********************/

	struct frame {
		string rledata;
		unsigned int startvalue; //value 1 smaller than the smallest possible to store. For the first frame, this is 0
		unsigned int maxvalue;   //max value to store in this frame. Is equal to the next frames startvalue
		unsigned int numitems;

		frame(){
			frame(0, 0);
		}

		frame(unsigned int _startvalue, unsigned int _maxvalue){
			startvalue = _startvalue;
			maxvalue = _maxvalue;

			rledata = "";

			rledata.append(block(0,1).encode());
			rledata.append(1, '\0'); //null terminate. \0 is equal to a 1 byte block of 0s of runlength 0

			numitems = 0;
		}

		bool setbit(unsigned int index){
			bool ret = set(index, 1);
			if(ret)
				numitems++;
			return ret;
		}

		bool unsetbit(unsigned int index){
			bool ret = set(index, 0);
			if(ret)
				numitems--;
			return ret;
		}

		unsigned int size(void) const {
			return numitems;
		}

		unsigned int memsize(void) const {
			return rledata.size();
		}

		string printhex(){
			return print("%02X ", rledata);
		}

		string print(){
			string ret;
			char buf[100];

			for(iterator it = begin(); !it.done(); ++it){
				sprintf(buf, "%u ", *it);
				ret.append(buf);
			}

			return ret;
		}

		string print(char * format, string & str){
			string ret;
			char buf[100];

			string::iterator it, end;
			it = str.begin();
			end = str.end();

			for(; it != end; it++){
				sprintf(buf, format,(unsigned char) *it);
				ret.append(buf);
			}

			return ret;
		}

		frame split(){
			frame nframe;

			block cur;

			unsigned int bytesneeded = rledata.size()/2;
			unsigned int bytesfound = 0;
			unsigned int itemsfound = 0;

		//find the right block
			cur = block(rledata.begin(), startvalue);

			while(cur.runlength){
				bytesfound += cur.blocklength;
				if(cur.bitvalue == 1)
					itemsfound += cur.runlength;

				if(bytesfound >= bytesneeded && cur.bitvalue == 0) //found!
					break;

				cur = cur.next();
			}

		//if found, copy the rest of this one into the new frame
			if(cur.runlength){
				//startvalue is 1 less than the first usable state
				//this is so that the first block will ALWAYS exist
				nframe.startvalue = cur.value-1;
				nframe.maxvalue = maxvalue;

				maxvalue = nframe.startvalue;				
				cur.runlength++;

				nframe.rledata = cur.encode() + string(cur.blockend(), rledata.end());
				nframe.numitems = numitems - itemsfound;

				rledata.replace(cur.rleptr, rledata.end(), 1, '\0');
				numitems = itemsfound;
			}

			return nframe;
		}

		void join(frame other){

			block cur;
			block prev;
			block middle;
			block next;


		//find the right block
			prev = cur = block(rledata.begin(), startvalue);

			while(cur.runlength){
				prev = cur;
				cur = cur.next();
			}

			middle = block(other.rledata.begin(), other.startvalue);
			next = middle.next();

		/*
		 * at this point we've got:
		 *   prev being the last real block in the current frame
		 *   middle being the start offset block of the next frame
		 *   next being the first real block of the next frame
		 * 
		 * either join prev and next or insert a new separator block		 		 
		 */

			unsigned int diff = next.value + 1 - prev.value - prev.runlength;

			if(diff == 0){ //join them
				rledata.erase(prev.blockbegin(), rledata.end()); //remove the last block and the terminator
				rledata.append(block(1, prev.runlength + next.runlength).encode()); //append the new joined block
				rledata.append(next.blockend(), other.rledata.end()); //append the rest of the other frame
			}else{
				rledata.erase(rledata.size()-1); //remove the terminator
				rledata.append(block(0, diff).encode()); //append the middle piece
				rledata.append(middle.blockend(), other.rledata.end()); //append the rest of the other frame
			}

			numitems += other.numitems;
			maxvalue = other.maxvalue;
		}

		bool set(unsigned int index, unsigned int newval){

			assert(newval == 0 || newval == 1);
			assert(index > startvalue);
			assert(maxvalue == 0 || index <= maxvalue);
	
	/*
		find block
		if no block found
			create one or two
		if block value == newval
			return false
		if block runlength == 1
			join prev and next blocks
		else if index is first of the block
			shorten this block
			lengthen the prev block
		else if index is last of the block
			shorten this block
			lengthen next block
		else
			split block in 3
	*/
	
	
	
	
			block cur;
			block prev;
			block next;
	
			string newblocks;
			string::iterator repstart;
			string::iterator repend;
	
		//find the right block
			cur = block(rledata.begin(), startvalue);
	
			while(cur.runlength){
				if(cur.value + cur.runlength - 1 >= index) //found!
					break;

				prev = cur;
				cur = cur.next();
			}

		//at this point, cur will obviously always be set.
		//previs also always set, because of the head block of 0s set at offset 0
		//this is always going to be true, since offset 0 is an illegal index to set
		//this block may change in length, but will always exist
	
	
		//already set, this includes setting 0s past the end of the string
			if(cur.bitvalue == newval)
				return false;
	
	
		//got to the end without finding a block
			if(!cur.runlength){
				if(prev.bitvalue == newval){
					if(prev.value + prev.runlength == index){
					 //extend the previous block by one
						prev.runlength++;
	
						newblocks = prev.encode();
	
						repstart = prev.blockbegin();
						repend   = prev.blockend();
					}else{
					//create a separator block and a new block
						newblocks = block((1 - newval), (index - prev.value - prev.runlength)).encode() +
						            block(newval, 1).encode() +
									string(1, '\0');
	
						repstart = cur.blockbegin();
						repend   = cur.blockend();
					}
				}else{
				 //extend the previous block and create a new block
					newblocks = block(prev.bitvalue, (prev.runlength + index - prev.value - 1)).encode() + 
					            block(newval, 1).encode();
	
					repstart = prev.blockbegin();
					repend = prev.blockend();
				}
			}
	
		//if block runlength == 1
		//	join prev and next blocks
			else if(cur.runlength == 1){
				next = cur.next();
	
				if(next.runlength == 0){ //at the end, don't replace the next block as its the null terminator
					newblocks = block(newval, prev.runlength + 1).encode();
	
					repstart = prev.blockbegin();
					repend = cur.blockend();
				}else{
					newblocks = block(newval, prev.runlength + 1 + next.runlength).encode();
	
					repstart = prev.blockbegin();
					repend = next.blockend();
				}
			}
	
		//else if index is first of the block
		//	shorten this block
		//	lengthen the prev block
			else if(cur.value == index){
				cur.runlength--;
				prev.runlength++;
	
				newblocks = prev.encode() + cur.encode();
	
				repstart = prev.blockbegin();
				repend = cur.blockend();
			}
	
		//else if index is last of the block
		//	shorten this block
		//	lengthen next block
			else if(cur.value + cur.runlength - 1 == index){
				next = cur.next();
	
				if(next.runlength == 0){ //setting last bit of the last block
					cur.runlength--;
	
					newblocks = cur.encode() + 
					            block(newval, 1).encode();
	
					repstart = cur.blockbegin();
					repend = cur.blockend();
				}else{
					cur.runlength--;
					next.runlength++;
	
					newblocks = cur.encode() + next.encode();
	
					repstart = cur.blockbegin();
					repend = next.blockend();
				}
			}

		//else
		//	split block in 3
			else {
				newblocks = block(cur.bitvalue, index - cur.value).encode();
				newblocks += block(newval, 1).encode();
				newblocks += block(cur.bitvalue, cur.value - index + cur.runlength - 1 ).encode();

				repstart = cur.blockbegin();
				repend = cur.blockend();
			}

			rledata.replace(repstart, repend, newblocks);

			return true;
		}

 
 /*******************************
 **** class frame::iterator *****
 ********************************/
		
		class iterator {
			block cur;
			unsigned int progress;
	
		public:
			iterator(){
			}
	
			iterator(string::iterator rledata, unsigned int startvalue){
				cur = block(rledata, startvalue);
				progress = 0;
				++(*this);
			}
	
			iterator(const sparse_bit_array::frame::iterator & it){
				cur = it.cur;
				progress = it.progress;
			}
	
			const unsigned int operator *() const {
				return cur.value + progress;
			}

			bool done(){
				return (cur.runlength == 0);
			}

			bool operator == (const sparse_bit_array::frame::iterator & rhs) const {
				return (cur.value == rhs.cur.value && progress == rhs.progress);
			}

			bool operator != (const sparse_bit_array::frame::iterator & rhs) const {
				return !(*this == rhs);
			}

			iterator operator ++(){ //prefix form
	//			assert(cur.runlength);

				if(cur.runlength)
					progress++;

			//move forward if
			//   this isn't the terminator block and
			//   this is a block of 0s, or done walking this block of 1s
				while(cur.runlength && (progress >= cur.runlength || !cur.bitvalue)){
					cur = cur.next();
					progress = 0;
				}

				return *this;
			}

			iterator operator ++(int){ //postfix form
				iterator newit(*this);
				++(*this);
				return newit;
			}
		};

/************************************
 **** end class frame::iterator *****
 ************************************/

		iterator begin(){ //sparse_bit_array::frame::iterator
			return iterator(rledata.begin(), startvalue);
		}

		iterator end(){ //sparse_bit_array::frame::iterator
			return iterator(rledata.end() - 1, 0); //point to the last byte, which is a single byte block as a terminator
		}
	};

/**************************
 **** end class frame *****
 **************************/












	list<frame> frames;
	unsigned int numitems; //number of successful sets - number of successful unsets


	//rwlock;

public:
	const static unsigned int splitthreshhold = 100; //when one block exceeds this size, split it into two
	const static unsigned int jointhreshhold  = 40;  //when two consecutive blocks are smaller than this, join them

	sparse_bit_array(){
		frames.push_back(frame(0, 0));
		numitems = 0;
	}

	bool setbit(unsigned int index){
		bool ret = set(index, 1);
		if(ret)
			numitems++;
		return ret;
	}

	bool unsetbit(unsigned int index){
		bool ret = set(index, 0);
		if(ret)
			numitems--;
		return ret;
	}

	unsigned int size(void) const {
		return numitems;
	}

	unsigned int memsize(void) const {
	
		unsigned int ret = 0;
		
		list<frame>::const_iterator it = frames.begin();
		list<frame>::const_iterator end = frames.end();
		
		for(; it != end; ++it)
			ret += it->memsize();
	
		return ret;
	}

	unsigned int framecount(void) const {
		return frames.size();
	}

	string printhex(){
		string ret;

		list<frame>::iterator it, itend;
		it = frames.begin();
		itend = frames.end();

		for(; it != itend; ++it)
			ret.append(it->printhex() + "\n");

		return ret;
	}

	string print(){
		string ret;

		list<frame>::iterator it, itend;
		it = frames.begin();
		itend = frames.end();

		for(; it != itend; ++it)
			ret.append(it->print());

		return ret;
	}

private:
	bool set(unsigned int index, unsigned int newval){
		assert(newval == 0 || newval == 1);
		assert(index > 0);

		list<frame>::iterator cur;
		list<frame>::iterator prev;

		list<frame>::iterator iter = frames.begin();
		list<frame>::iterator itend = frames.end();

	//find the frame
		while(iter != itend){
			prev = cur;
			cur = iter;
			++iter;

			if((*iter).startvalue >= index)
				break;
		}

	//cur points to the frame it will be added to,
	//iter points to the next frame, ie the frame a split would insert in front of

		bool ret = cur->set(index, newval);

	//resize if it's bigger than the min threshhold and bigger than the number of frames
	//the threshhold is to stop it from degenerating into a linked list at small sizes
	//the size being relative to the number of frames is to balance the two sequential scans 
	//(list of frames, blocks in frames) to be approximately even
		if(cur->rledata.size() > splitthreshhold && cur->rledata.size() > frames.size()){
			frames.insert(iter, cur->split());
		}else if(iter != itend && cur->rledata.size() + iter->rledata.size() < jointhreshhold){
			cur->join(*iter);
			frames.erase(iter);
		}

		return ret;
	}




public:


/*************************
 **** class iterator *****
 *************************/

	class iterator { //sparse_bit_array::iterator
		list<frame>::iterator it;
		list<frame>::iterator itend;
		frame::iterator frameit;

	public:
		iterator(){
		}

		iterator(list<frame>::iterator _it, list<frame>::iterator _itend){
			it = _it;
			itend = _itend;
			frameit = (*it).begin();
		}

		iterator(const sparse_bit_array::iterator & _it){
			it = _it.it;
			itend = _it.itend;
			frameit = _it.frameit;
		}

		const unsigned int operator *() const {
			return *frameit;
		}

		bool done(){
			return (it == itend);
		}

		bool operator == (const sparse_bit_array::iterator & rhs) const {
			return (it == rhs.it && frameit == rhs.frameit);
		}

		bool operator != (const sparse_bit_array::iterator & rhs) const {
			return !(*this == rhs);
		}

		iterator operator ++(){ //prefix form
//			assert(cur.runlength);

			++frameit;

			if(frameit.done()){
				++it;
				if(it != itend)
					frameit = (*it).begin();
			}

			return *this;
		}

		iterator operator ++(int){ //postfix form
			iterator newit(*this);
			++(*this);
			return newit;
		}

	};


/*****************************
 **** end class iterator *****
 *****************************/


	iterator begin(){ //sparse_bit_array::iterator
		return iterator(frames.begin(), frames.end());
	}

	iterator end(){  //sparse_bit_array::iterator
		return iterator(frames.end(), frames.end()); //point to the last byte, which is a single byte block as a terminator
	}

	
};

typedef sparse_bit_array::iterator sparse_bit_array_iter;

#endif

