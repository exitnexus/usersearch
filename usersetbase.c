

#include <stdio.h>
#include <sys/time.h>

#include "usersetbase.h"



void printset(userset & set){
	userset::iterator it;
	for(it = set.begin(); it != set.end(); ++it)
		printf("%u ", *it);
}

int main(){

	vector <userset> lists;
	vector <userset>::iterator listit;
	userset temp, temp2;
	vector <userset *> plist;
	struct timeval start, finish;

	unsigned int generate = 0;
	unsigned int linunion = 0;
	unsigned int comunion = 0;
	unsigned int lininter = 0;
	unsigned int cominter = 0;

	unsigned int foundunion = 0;
	unsigned int foundinter = 0;


	srand(time(NULL));

	const int numloops = 5;
	const int numruns = 10;
	const int numsets = 100;
	const int setsize = 10000;
	const int sparsity = 10;

	for(int a = 0; a < numloops; a++){

	//generate lists	
		gettimeofday(&start, NULL);

		lists = vector<userset>();

		for(int i = numsets; i; i--){
			temp = userset();
			for(int i = setsize; i; i--)
				temp.addUser(rand() % (setsize*sparsity) + 1);
	
			lists.push_back(temp);
		}
		gettimeofday(&finish, NULL);
	
		generate += ((finish.tv_sec*1000+finish.tv_usec/1000)-(start.tv_sec*1000+start.tv_usec/1000));
	
	
	
	//linear union
		gettimeofday(&start, NULL);
		
		for(int i = 0; i < numruns; i++){
			temp = lists[0];
			
			for(listit = lists.begin()+1; listit != lists.end(); ++listit)
				temp.union_set(*listit);
		}
	
		gettimeofday(&finish, NULL);
	
		linunion += ((finish.tv_sec*1000+finish.tv_usec/1000)-(start.tv_sec*1000+start.tv_usec/1000));
	
	
	
	//Combined union
		gettimeofday(&start, NULL);
		
		for(int i = 0; i < numruns; i++){
			temp2 = lists[0];
			
			plist.resize(0);
			
			for(listit = lists.begin()+1; listit != lists.end(); ++listit)
				plist.push_back(&*listit);
	
			temp2.union_set(plist);
		}
	
		gettimeofday(&finish, NULL);
	
		comunion += ((finish.tv_sec*1000+finish.tv_usec/1000)-(start.tv_sec*1000+start.tv_usec/1000));
	
		if(!(temp == temp2))
			printf("Union Boom!\n");
	
		foundunion += temp2.size();
	
	//linear intersect
		gettimeofday(&start, NULL);
		
		for(int i = 0; i < numruns; i++){
			temp = lists[0];
			
			for(listit = lists.begin()+1; listit != lists.end(); ++listit)
				temp.intersect_set(*listit);
		}
	
		gettimeofday(&finish, NULL);
	
		lininter += ((finish.tv_sec*1000+finish.tv_usec/1000)-(start.tv_sec*1000+start.tv_usec/1000));
	
	
	//Combined intersect
		gettimeofday(&start, NULL);
		
		for(int i = 0; i < numruns; i++){
			temp2 = lists[0];
			
			plist.resize(0);
			
			for(listit = lists.begin()+1; listit != lists.end(); ++listit)
				plist.push_back(&*listit);
		
			temp2.intersect_set(plist);
		}
	
		gettimeofday(&finish, NULL);
	
		cominter += ((finish.tv_sec*1000+finish.tv_usec/1000)-(start.tv_sec*1000+start.tv_usec/1000));

		if(!(temp == temp2))
			printf("Intersect Boom!\n");
		
		foundinter += temp2.size();
	}

	printf("Generate:  %u ms, %ux%u loops over %u sets of %u with sparsity of %u\n", generate, numloops, numruns, numsets, setsize, sparsity);
	printf("Union:     Found: %8u, Line: %5u ms, Comb: %5u ms, %6.2f %%\n", foundunion, linunion, comunion, 100.0*comunion/linunion);
	printf("Intersect: Found: %8u, Line: %5u ms, Comb: %5u ms, %6.2f %%\n", foundinter, lininter, cominter, 100.0*cominter/lininter);



	return 0;
}

