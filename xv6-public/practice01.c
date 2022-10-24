#include "types.h"
#include "stat.h"
#include "user.h"

// A user program which invokes "int 128" instruction

int main(int argc, char *argv[]){

	__asm__("int $128");
    exit();

    return 0; // This part will not be reached.
}