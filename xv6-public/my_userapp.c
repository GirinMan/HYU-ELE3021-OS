#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_THREAD 5

int status;
thread_t thread[NUM_THREAD];
int expected[NUM_THREAD];

void *thread_basic(void *arg)
{
	int val = (int)arg;
	printf(1, "Thread %d start\n", val);
	if (val == 1) {
		sleep(100);
		status = 1;
	}
	int *ptr = (int *)malloc(65536);
	procdump();
	ptr[15000] = 777;

	printf(1, "Thread %d end\n", val);
	thread_exit((void*)(ptr[15000]));

	return 0; // This code won't be executed.
}

int main(int argc, char* argv[]){

	int result;
	void* retval = (void*)1;

	if((result = thread_create(&thread[0], thread_basic, retval)) != 0){
		printf(1, "Error creating thread. errcode: %d\n", result);
	}
	sleep(100);
	printf(1, "Status: %d with Addr: %p\n", status, &status);
	
	thread_join(thread[0], (void**)&result);

	printf(1, "return val: %d\n", result);
	exit();
};

