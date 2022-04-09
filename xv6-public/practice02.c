#include "types.h"
#include "stat.h"
#include "user.h"

#define LOOP 4

int main(int argc, char** argv){

    int pid;

    // fork child process
    pid = fork();

    if(pid < 0) { // error occurred
        printf(1, "Error: fork() failed.\n");
        exit();
    }

    if (pid == 0) {// child process
        //procdump();
        int loop = LOOP;
        while(loop-- > 0){
            printf(1, "Child\n");
            //procdump();
            yield();
        }
    }

    if(pid > 0){ // parent process
        //sleep(10);
        //procdump();
        int loop = LOOP;
        while(loop-- > 0){
            printf(1, "Parent\n");
            //procdump();
            yield();
        }
        //procdump();
        wait();
        //procdump();
    }
    //procdump();
    exit();

    return 0;
}