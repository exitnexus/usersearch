

#include <stdio.h>

#include "sparsebitarray.h"



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

	f += testbitarray(bitarray, "04 86 03 81 01 81 00", "set 12 - split block in 3");


	printf("%u failures\n", f);
	
	return 0;
}
