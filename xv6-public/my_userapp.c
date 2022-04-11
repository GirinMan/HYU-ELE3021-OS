#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[]){
	char* buf = "Hello xv6!";
	int ret_val;
	ret_val = myfunction(buf);
	printf(1, "Return value: 0x%x\n", ret_val);

	int pid = fork();

	if(pid < 0) exit();
	if(pid == 0){
		printf(1, "Child pid: %d\n", getpid());
		printf(1, "Current level: %d\n", getlev());
		printf(1, "setpriority(%d, %d)=%d\n", getpid(), 1, setpriority(getpid(), 2));
		
	}
	else{
		printf(1, "Parent pid: %d\n", getpid());
		printf(1, "Current level: %d\n", getlev());
		printf(1, "setpriority(%d, %d)=%d\n", pid, 1, setpriority(pid, 1));
		wait();
	}


	exit();
};

