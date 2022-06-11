// login: The user-account selecting program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define MAX_LEN 15

char *argv[] = { "sh", 0 };

int
main(void)
{
    int pid, wpid;
    char username[MAX_LEN + 1], password[MAX_LEN + 1];
    for(;;){
        printf(1, "Enter username: ");
        gets(username, sizeof username);
        username[strlen(username) - 1] = 0;

        printf(1, "Enter password: ");
        conswtch();
        gets(password, sizeof password);
        password[strlen(password) - 1] = 0;
        conswtch();

        if(strlen(username) > MAX_LEN || strlen(password) > MAX_LEN){
            printf(1, "Usernames and passwords must be >1 and <16 characters\n");
            continue;
        }

        if(login(username, password) == 0){
            pid = fork();
            if(pid < 0){
                printf(1, "login: fork failed\n");
                exit();
            }
            if(pid == 0){
                exec("sh", argv);
                printf(1, "login: exec sh failed\n");
                exit();
            }
            while((wpid=wait()) >= 0 && wpid != pid)
                printf(1, "zombie!\n");
        }
    }
}
