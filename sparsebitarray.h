

#ifndef _SPARSEBITARRAY_H_
#define _SPARSEBITARRAY_H_


using namespace std;

#include <vector>
#include "search.h"

/***********
 * The rledata has the format:
 * - top bit represents what this block is encoding (ie run of 0s or 1s)
 * - next 2 bits represent the length of this block, in terms of additional bytes
 *    (ie 0,1,2,3 additional bytes, for a total of 1,2,3,4 bytes)
 * - remainder of the block (varying between 5 bits and 29 bits) represents the run length
 **********/



/****************
 * The rledata is made as a series of frames. Each frame will start with a keyframe
 * which specifies the initial offset, followed by an arbitrary number of blocks.
 * The top bit will be used to flag the start of a frame. Keyframes are 4 bytes in length,
 * using 31 bits to specify the offset.
 * The following blocks will have the format:
 * - top bit will be 0
 * - next bit will represent the value this block is encoding (a series of 1s or 0s)
 * - next 2 bits represent the length of this block in terms of additional bytes
 *     (ie 0,1,2,3 additional bytes, for a total of 1,2,3,4 bytes)
 * - The remaining 4 bits will be the top bits of the length
 * - In the 0-3 bytes that follow that are part of the block (according to the length)
 *     will have the top bit set to 0, and the remaining 7 bits will be the remaining bits of the length
 * - Any number of these blocks can follow a keyframe
 * - The keyframe offset plus the cumulative lengths of the blocks shall not exceed the value of the next keyframe
 * - Keyframes can be used to do semi-random access to the middle of the rledata
 ****************/

class sparse_bit_array {
	string rledata;
	unsigned int numitems; //number of successful sets - number of successful unsets

	//rwlock;

public:
	sparse_bit_array(){
		rledata = string();
		numitems = 0;
	}

	bool setbit(unsigned int index){
		return set(index, 1);
	}

	bool unsetbit(unsigned int index){
		return set(index, 0);
	}

	unsigned int size(void) const {
		return numitems;
	}


	iterator begin(){
		return iterator.iterator(rledata);
	}

	iterator end(){
		return iterator.iterator(rledata + _size);
	}

private:
	bool set(unsigned int index, unsigned int newval){




/*
		find frame
			if no frame, create one at this offset
		find block
			if no block, create one or two?
		if block value == newval
			return false
		if block length == 1
			join prev and next blocks
		else if index is first of the block
			shorten this block
			lengthen the prev block
		else if(index is last of the block
			lengthen this block
			shorten next block
		else
			split block in 3
			potentially introduce a keyframe
*/		



		unsigned int curindex = 0;
		frame curframe;
		frame tempframe;

		block curblock;
		block prevblock;

		string newblocks;

		//create an initial frame at offset 0
		if(rledata.empty()){
			tempframe = frame(0);
			rledata.append(tempframe.encode());
		}

		string::iterator ptr = rledata.begin();
		string::iterator end = rledata.end();

	//find the right frame
		curframe = frame(ptr);

		while(ptr != end){
			if(*ptr & 0x80){
				tempframe = frame(ptr);
				if(tempframe.value > index) //if we've found the next frame, stop
					break;
				curframe = tempframe;
			}
			++ptr;
		}
	
	//find the right block
		ptr = curframe.ptr + 4; // point to the first block in the frame
		curindex = curframe.value;
		

		while(ptr != end && !(*ptr & 0x80)){ //not the end, and not the next frame
			curblock = block(ptr, curindex);
			
		
		
		}
		
		
		
		/*
		 * Possible operations on a set (set to 0 or 1)
		 * - Already set
		 * - Add to the end:
		 *   - create a new block or two (key frame?)
		 * - Add to the end of a block      ie 100011 => 100111
		 * - Add to the begining of a block ie 100111 => 100011
		 *   - Update the lengths of the two bordering blocks
		 *
		 * - Split a block in three         ie 100011 => 101011
		 *   - Create 2 new blocks
		 *   - Update the length of the existing block
		 * - Join three blocks              ie 101011 => 100011
		 *   - Update the length of the first block
		 *   - Remove the extra blocks		 
		 *
		 * If block lengths change (ie run length could be represented with a shorter block)
		 *   should it be shortened?
		 *		 
		 * Return whether a change was made or not
		 */
		
		
		
	}


	struct frame {
		unsigned int value; //value of the start index of this frame
		unsigned int numblocks;
		string::iterator ptr;
	
		frame(unsigned int _value){
			value = _value;
			assert(value & (1 << 31) == 0); //only takes 31 bits
		}

		frame(string::iterator _ptr){
			ptr = _ptr;

			assert(ptr & (1 << 7) != 0); //top bit is set

			value = 0;
			value |= (*(ptr) & 0x7F) << 24;
			value |= (*(ptr + 1) << 16;
			value |= (*(ptr + 2) <<  8;
			value |= (*(ptr + 3) <<  0;
		}

		string encode(){
			assert(value & (1 << 31) == 0); //only takes 31 bits

			string str = string(4, '\0');

			str[0] = (value & (0x7F << 24)) >> 24;
			str[1] = (value & (0xFF << 16)) >> 16;
			str[2] = (value & (0xFF <<  8)) >>  8;
			str[3] = (value & (0xFF <<  0)) >>  0;

			str[0] |= 0x80; //set the top bit

			return str;
		}
	}

	struct block {
		unsigned int value;        //value of the key (ie offset in the bit array)
		unsigned int bitvalue;     //value of the bits in the run
		unsigned int blocklength;  //length of memory used by this block (0-3 bytes)
		unsigned int runlength;    //run length
		string::iterator rledata;  //iterator to start of this block

		block(){
			value = 0;
			bitvalue = 0;
			blocklength = 0;
			runlength = 0;
		}

		block(string::iterator _rledata, unsigned int _value){
			rledata = _rledata;
			value   = _value;

			assert((*rledata & 0x80) != 0x80); //not a keyframe

			bitvalue    = ((*rledata & 0x40) >> 6); //grab the 7th bit
			blocklength = ((*rledata & 0x30) >> 4); //grab the 5th and 6th bit

			runlength = 0;

			switch(blocklength){ // acts as an unrolled loop
				case 3: runlength |= ((*(rledata + 3) & 0x7F) << 18);
				case 2: runlength |= ((*(rledata + 2) & 0x7F) << 11);
				case 1: runlength |= ((*(rledata + 1) & 0x7F) <<  4);
				case 0: runlength |= ((*(rledata    ) & 0x0F) <<  0);
			}
		}

		string encode(){
			string ret;

			if(runlength <= 0x0F)	      blocklength = 0; // 4 bits of 1s
			else if(runlength <= 0x7FF)   blocklength = 1; //11 bits of 1s
			else if(runlength <= 0x3FFFF) blocklength = 2; //18 bits of 1s
			else                          blocklength = 3;

			ret = string(runlength+1, '\0');

			ret[0] = (char) (((bitvalue & 1) << 6) | (blocklength << 4)); //just the block header, ie bit value and block length
			switch(blocklength){
				case 3: ret[3] |= ((runlength & (0x7F << 18)) >> 18);
				case 2: ret[2] |= ((runlength & (0x7F << 11)) >> 11);
				case 1: ret[1] |= ((runlength & (0x7F <<  4)) >>  4);
				case 0: ret[0] |= ((runlength & (0x0F <<  0)) >>  0);
			}

			return ret;
		}
	}


	class iterator {

		unsigned int value;

		unsigned int datapos; //which block I'm on
		unsigned int blockpos;
		
		string * rledata;
		
		iterator(string * data){
			rledata = data;

			datapos = 0;
			blockpos = 0;

			value = 0;
		}
		
		const unsigned int operator *() const {
			return value;
		}
		
		iterator operator ++(){ //prefix form
			
		}


/*		
		iterator operator ++(int){ //postfix form

		}
*/
		
	
	}
};

#endif

