#include "types.h"
#include "stat.h"
#include "user.h"

// A user program which print current
// process' pid & parent process' pid.

int main(){

	printf(1, "My pid is %d\n", getpid());
	printf(1, "My ppid is %d\n", getppid());
    exit();

    return 0; // This part will not be reached.
}