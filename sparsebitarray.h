

#ifndef _SPARSEBITARRAY_H_
#define _SPARSEBITARRAY_H_


using namespace std;

#include <string>
#include <assert.h>

/***********
 * The rledata is formed as a series of blocks. Each block has the format:
 * - top bit represents what this block is encoding (ie run of 0s or 1s)
 * - next 2 bits represent the length of this block, in terms of additional bytes
 *    (ie 0,1,2,3 additional bytes, for a total of 1,2,3,4 bytes)
 * - remainder of the block (varying between 5 bits and 29 bits) represents the run length
 **********/


class sparse_bit_array {
	string rledata;
	unsigned int numitems; //number of successful sets - number of successful unsets

	//rwlock;

public:
	sparse_bit_array(){
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
		string ret;
		char buf[100];
	
		string::iterator it, end;
		it = rledata.begin();
		end = rledata.end();
		
		for(; it != end; it++){
			sprintf(buf, "%02X ",(unsigned char) *it);
			ret.append(buf);
		}

		return ret;
	}

	string print(){
		string ret;
		char buf[100];

		for(SBAiterator it = begin(); !it.done(); ++it){
			sprintf(buf, "%u ", *it);
			ret.append(buf);
		}

		return ret;
	
	}

private:
	bool set(unsigned int index, unsigned int newval){

		assert(newval == 0 || newval == 1);
		assert(index > 0);


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
		string::iterator start;
		string::iterator end;

	//find the right block
		cur = block(rledata.begin(), 0);

		while(cur.runlength){
			if(cur.value + cur.runlength - 1 >= index) //found!
				break;

			prev = cur;
			cur = cur.next();
		}


	//at this point, cur will obviously always be set.
	//prevblock is also always set, because of the head block of 0s set at offset 0
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

					start = prev.rledata;
					end   = prev.blockend();
				}else{
				//create a separator block and a new block
					newblocks = block((1 - newval), (index - prev.value - prev.runlength)).encode() +
					            block(newval, 1).encode();

					start = cur.rledata;
					end   = cur.rledata;
				}
			}else{
			 //extend the previous block and create a new block
				newblocks = block(prev.bitvalue, (prev.runlength + index - prev.value - 1)).encode() + 
				            block(newval, 1).encode();

				start = prev.rledata;
				end = prev.blockend();
			}
		}

	//if block runlength == 1
	//	join prev and next blocks
		else if(cur.runlength == 1){
			next = cur.next();

			if(next.runlength == 0){ //at the end, don't replace the next block as its the null terminator
				newblocks = block(newval, prev.runlength + 1).encode();

				start = prev.rledata;
				end = cur.blockend();
			}else{
				newblocks = block(newval, prev.runlength + 1 + next.runlength).encode();

				start = prev.rledata;
				end = next.blockend();
			}
		}

	//else if index is first of the block
	//	shorten this block
	//	lengthen the prev block
		else if(cur.value == index){
			cur.runlength--;
			prev.runlength++;

			newblocks = prev.encode() + cur.encode();

			start = prev.rledata;
			end = cur.blockend();
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

				start = cur.rledata;
				end = cur.blockend();
			}else{
				cur.runlength--;
				next.runlength++;

				newblocks = cur.encode() + next.encode();

				start = cur.rledata;
				end = next.blockend();
			}
		}

	//else
	//	split block in 3
		else {
			newblocks = block(cur.bitvalue, index - cur.value).encode() +
						block(newval, 1).encode() +
						block(cur.bitvalue, cur.value + cur.runlength - index - 1 ).encode();


			start = cur.rledata;
			end = cur.blockend();
		}

		rledata.replace(start, end, newblocks);
		return true;
	}


	struct block {
		unsigned int value;        //value of the key (ie offset in the bit array)
		unsigned int bitvalue;     //value of the bits in the run
		unsigned int blocklength;  //length of memory used by this block (1-4 bytes)
		unsigned int runlength;    //run length
		string::iterator rledata;  //iterator to start of this block

		block(){
			value = 0;
			bitvalue = 0;
			blocklength = 0;
			runlength = 0;
		}

		block(unsigned int _bitvalue, unsigned int _runlength){
			value = 0;
			blocklength = 0;

			bitvalue = _bitvalue;
			runlength = _runlength;
		}

		block(string::iterator _rledata, unsigned int _value){
			rledata = _rledata;
			value   = _value;

			bitvalue    = ((*rledata & (1 << 7)) >> 7);     //grab the 7th bit
			blocklength = ((*rledata & (3 << 5)) >> 5) + 1; //grab the 5th and 6th bit, +1 to give the full block length

			runlength = *rledata & 0x1F;

			switch(blocklength){ // acts as an unrolled loop
				case 4: runlength |= ((*(rledata + 3) & 0xFF) << 21);
				case 3: runlength |= ((*(rledata + 2) & 0xFF) << 13);
				case 2: runlength |= ((*(rledata + 1) & 0xFF) <<  5);
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

		string::iterator blockend(){
			return rledata + blocklength;
		}

		block next(){
			return block(blockend(), value + runlength);
		}		
	};

public:

	class SBAiterator {
		block cur;
		unsigned int progress;

	public:
		SBAiterator(){
		}

		SBAiterator(string::iterator rledata){
			cur = block(rledata, 0);
			progress = 0;
			++(*this);
		}

		SBAiterator(const sparse_bit_array::SBAiterator & it){
			cur = it.cur;
			progress = it.progress;
		}

		const unsigned int operator *() const {
			return cur.value + progress;
		}

		bool done(){
			return (cur.runlength == 0);
		}

		bool operator == (const sparse_bit_array::SBAiterator & rhs) const {
			return (cur.value == rhs.cur.value && progress == rhs.progress);
		}

		bool operator != (const sparse_bit_array::SBAiterator & rhs) const {
			return !(*this == rhs);
		}

		SBAiterator operator ++(){ //prefix form
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

		SBAiterator operator ++(int){ //postfix form
			SBAiterator newit(*this);
			++(*this);
			return newit;
		}

	};
	
	SBAiterator begin(){
		return SBAiterator(rledata.begin());
	}

	SBAiterator end(){
		return SBAiterator(rledata.end() - 1); //point to the last byte, which is a single byte block as a terminator
	}

	
};

typedef sparse_bit_array::SBAiterator sparse_bit_array_iter;

#endif

