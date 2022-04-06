#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char** argv){

    int pid;

    // fork child process
    pid = fork();

    if(pid < 0) { // error occurred
        printf(1, "Error: fork() failed.\n");
        exit();
    }

    else if (pid == 0) {// child process
        while(1){
            printf(1, "Child\n");
            yield();
        }
    }

    else { // parent process
        while(1){
            printf(1, "Parent\n");
            yield();
        }
    }

    exit();

    return 0;
}