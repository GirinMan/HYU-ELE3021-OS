#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char** argv){

    int n = 0;
    for(int i = 0; i < 10000; i++){
        getpid();
        n++;
        getppid();
    }

    printf(1, "%d times loop ended.\n", n);

    exit();

    return 0;
}