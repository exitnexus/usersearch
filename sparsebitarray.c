

#include <stdio.h>
#include <sys/time.h>

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

	bitarray.setbit(100);

	f += testbitarray(bitarray, "04 86 03 81 01 81 0E 81 13 81 31 01 81 00", "set 100 - add a high value");

	bitarray.setbit(10000000);

	f += testbitarray(bitarray, "04 86 03 81 01 81 0E 81 13 81 31 01 81 7B B0 C4 04 81 00",
				"set 10000000 - add a high value");


	bitarray = sparse_bit_array();

	bitarray.setbit(5);

	f += testbitarray(bitarray, "05 81 00",	"set 5 - init for unset tests");

	bitarray.unsetbit(8);

	f += testbitarray(bitarray, "05 81 00",	"unset 8 - remove a value that's already not set");


	bitarray.unsetbit(5);

	f += testbitarray(bitarray, "06 00",	"unset 5 - remove the end value");

	bitarray.setbit(3);
	bitarray.setbit(4);
	bitarray.setbit(5);
	bitarray.setbit(6);
	bitarray.setbit(7);
	bitarray.setbit(10);
	bitarray.setbit(11);

	f += testbitarray(bitarray, "03 85 02 82 00",	"set 3,4,5,6,7,10,11 - re-init");
	
	bitarray.unsetbit(7);

	f += testbitarray(bitarray, "03 84 03 82 00",	"unset 7 - trade with before");

	bitarray.unsetbit(3);

	f += testbitarray(bitarray, "04 83 03 82 00",	"unset 3 - trade with after");

	bitarray.unsetbit(5);

	f += testbitarray(bitarray, "04 81 01 81 03 82 00",	"unset 5 - split a sequence");

	bitarray.unsetbit(6);

	f += testbitarray(bitarray, "04 81 05 82 00",	"unset 6 - join blocks");

	bitarray.unsetbit(4);

	f += testbitarray(bitarray, "0A 82 00",	"unset 4 - join blocks including first block");



	struct timeval start, finish;
	unsigned int runtime;
	int i, a;

	bitarray = sparse_bit_array();

	gettimeofday(&start, NULL);

	for(i = 0, a = 1; i < 100000; i++){
//		bitarray.setbit(rand() % 1000000 + 1);

		a += 2;// + rand() % 5;
		bitarray.setbit(a);
	}

	gettimeofday(&finish, NULL);

	runtime = ((finish.tv_sec*1000+finish.tv_usec/1000)-(start.tv_sec*1000+start.tv_usec/1000));
	printf("Write time: %u ms\n", runtime);

	gettimeofday(&start, NULL);

	for(sparse_bit_array_iter iter = bitarray.begin(); !iter.done(); ++iter)
		i = *iter;

	gettimeofday(&finish, NULL);

	runtime = ((finish.tv_sec*1000+finish.tv_usec/1000)-(start.tv_sec*1000+start.tv_usec/1000));
	printf("Read time: %u ms\n", runtime);

	printf("Elements: %u\n", bitarray.size());
	printf("Memsize: %u\n", bitarray.memsize());








/*



	vector<unsigned char> vec = vector<unsigned char>(10001, 0);
	vector<unsigned int> vec2;
	sparse_bit_array bitarr;
	unsigned int randnum;

	for(int i = 0; i < 10000; i++){
		randnum = rand() % 10000 + 1;
		
		if(rand() % 2){
			bitarr.setbit(randnum);
			vec[randnum] = 1;
		}else{
			bitarr.unsetbit(randnum);
			vec[randnum] = 0;
		}

//		printf("%u ", randnum);
	}
	printf("\n\n");


		
	for(unsigned int i = 0; i < vec.size(); i++)
		if(vec[i])
			vec2.push_back(i);

printf("sizes: vec %u, sba %u\n", vec2.size(), bitarr.size());


	vector<unsigned int>::iterator vecit = vec2.begin();
	sparse_bit_array::SBAiterator sbait = bitarr.begin();


	while(1){
		if(vecit == vec2.end() || sbait.done()){
			break;
		}else if(sbait.done()){
			printf("sba finished early\n");
		}else if(vecit == vec2.end()){
			printf("vec finished early\n");
		}

		if(*vecit != *sbait)
			printf("%u %u\n", *vecit, *sbait);

		++vecit;
		++sbait;
	}
*/


	printf("\n%u failures\n", f);
	
	return f;
}
