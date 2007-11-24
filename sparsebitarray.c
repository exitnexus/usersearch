

#include <stdio.h>

#include "sparsebitarray.h"
#include <vector>
#include <algorithm>


void print_array(sparse_bit_array & bitarray){
	printf("vals: %s\n", bitarray.print().c_str());
}


int testbitarray(sparse_bit_array bitarray, char * expected, char * description){
	printf("%s\n", description);

	int ret = 0;

	string returned = bitarray.printhex();

	returned.erase(returned.size()-1, 1); //remove the trailing space

	if(returned.compare(expected) != 0){
		printf("  Expected: '%s'\n", expected);
		printf("  Got:      '%s'\n", returned.c_str());
		ret = 1;
	}else{
		printf("  Data: %s\n", returned.c_str());
	}
	printf("  Vals: %s\n", bitarray.print().c_str());
	printf("\n");
	
	return ret;
}

int main(){
	
	sparse_bit_array bitarray;

	int f = 0;

	f += testbitarray(bitarray, "01 00", "init");

	bitarray.setbit(5);

	f += testbitarray(bitarray, "05 81 00", "set 5 - add a block to the end of a different value than last block");

	bitarray.setbit(7);

	f += testbitarray(bitarray, "05 81 01 81 00", "set 7 - add a block to the end of the same value as the last block");

	bitarray.setbit(5);

	f += testbitarray(bitarray, "05 81 01 81 00", "set 5 - already set");

	bitarray.setbit(6);

	f += testbitarray(bitarray, "05 83 00", "set 6 - join blocks");

	bitarray.setbit(4);

	f += testbitarray(bitarray, "04 84 00", "set 4 - trade with next defined block");
	
	bitarray.setbit(8);

	f += testbitarray(bitarray, "04 85 00", "set 8 - trade with next undefined block");

	bitarray.setbit(15);

	f += testbitarray(bitarray, "04 85 06 81 00", "set 15 - add a block past the end after a long block");

	bitarray.setbit(9);

	f += testbitarray(bitarray, "04 86 05 81 00", "set 9 - trade with prev block");

	bitarray.setbit(13);

	f += testbitarray(bitarray, "04 86 03 81 01 81 00", "set 13 - split block in 3");

	bitarray.setbit(50);

	f += testbitarray(bitarray, "04 86 03 81 01 81 22 01 81 00", "set 50 - add a high value");

	bitarray.setbit(30);

	f += testbitarray(bitarray, "04 86 03 81 01 81 0E 81 13 81 00", "set 30 - split a bigger gap");

/*
	bitarray.setbit(100);

	f += testbitarray(bitarray, "04 86 03 81 01 81 22 01 81 31 01 81 00", "set 100 - add a high value");

	bitarray.setbit(200);

	f += testbitarray(bitarray, "04 86 03 81 01 81 22 01 81 31 01 81 23 03 81 00", "set 200 - add a high value");

	bitarray.setbit(1000);

	f += testbitarray(bitarray, "04 86 03 81 01 81 22 01 81 31 01 81 23 03 81 3F 18 81 00",
				"set 1000 - add a high value");

	bitarray.setbit(10000);

	f += testbitarray(bitarray, "04 86 03 81 01 81 22 01 81 31 01 81 23 03 81 3F 18 81 47 19 01 81 00",
				"set 10000 - add a high value");

	bitarray.setbit(10000000);

	f += testbitarray(bitarray, "04 86 03 81 01 81 22 01 81 31 01 81 23 03 81 3F 18 81 47 19 01 81 6F 7B C3 04 81 00",
				"set 10000000 - add a high value");
*/

	vector<unsigned int> vec;
	sparse_bit_array bitarr;
	unsigned int randnum;

	for(int i = 0; i < 1000; i++){
		randnum = rand() % 100000;
		if(bitarr.setbit(randnum))
			vec.push_back(randnum);
//			printf("skipping %u\n", randnum);

//		printf("%u ", randnum);
	}
	printf("\n\n");

	

	sort(vec.begin(), vec.end());
	unique(vec.begin(), vec.end());

printf("sizes: vec %u, sba %u\n", vec.size(), bitarr.size());


	vector<unsigned int>::iterator vecit = vec.begin();
	sparse_bit_array::SBAiterator sbait = bitarr.begin();

	while(1){
		if(vecit == vec.end() && sbait.done()){
			break;
		}else if(sbait.done()){
			printf("sba finished early\n");
		}else if(vecit == vec.end()){
			printf("vec finished early\n");
		}

		if(*vecit != *sbait)
			printf("%u %u\n", *vecit, *sbait);

		++vecit;
		++sbait;
	}



	printf("\n%u failures\n", f);
	
	return 0;
}
