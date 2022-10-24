#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

int main(int argc, char *argv[])
{
    int fd;
    char buf[31];
    
    printf(1, "[TEST1] FILE\n");
    if(mkdir("testfile") < 0)
        goto bad;
    if(mkdir("testfile/t1") < 0)
        goto bad;
    fd = open("testfile/t1/f1", O_CREATE|O_RDWR);
    if(fd < 0)
    {
        goto bad;
    }
    memset(buf, 0, 32);
    if(write(fd, "1", 2) != 2)
    {
        goto bad;
    }
    
    // FILE MODE
    // Read : 파일을 읽을 수 있는 권한입니다. 구체적으로, read 권한이 없다면 open 시스템 콜에
    // O_RDONLY 또는 O_RDWR 옵션이 들어간 경우 실패해야 합니다.
    if(chmod("testfile/t1/f1", MODE_WUSR|MODE_XUSR) == -1)
    {
        printf(1,"1\n");
        goto bad;
    }
    if(open("testfile/t1/f1", O_RDWR) == 0)
    {
        printf(1,"2\n");
        goto bad;
    }
    if(open("testfile/t1/f1", O_RDONLY) == 0)
    {
        printf(1,"3\n");
        goto bad;
    }
    
    // FILE MODE
    // Write : 파일을 쓸 수 있는 권한입니다. 구체적으로, write 권한이 없다면 open 시스템 콜에
    // O_WRONLY 또는 O_RDWR 옵션이 들어간 경우 실패해야 합니다.
    if(chmod("testfile/t1/f1", MODE_RUSR|MODE_XUSR) == -1)
    {
        printf(1,"4\n");
        goto bad;
    }
    if(open("testfile/t1/f1", O_RDWR) == 0)
    {
        printf(1,"5\n");
        goto bad;
    }
    if(open("testfile/t1/f1", O_WRONLY) == 0)
    {
        printf(1,"6\n");
        goto bad;
    }
    
    // FILE MODE
    // Execute : 파일을 exec을 통해 실행할 수 있는 권한입니다.
    // 다시 말해, execute 권한이 없는 파일을 실행하려 하면 exec이 실패해야 합니다.
    if(chmod("ls", MODE_RUSR|MODE_WUSR|MODE_ROTH) == -1)
    {
        printf(1,"7\n");
        goto bad;
    }
    if(exec("ls",argv) == 0)
    {
        printf(1,"8\n");
        goto bad;
    }
    if(chmod("ls", MODE_RUSR|MODE_WUSR|MODE_XUSR|MODE_ROTH|MODE_XOTH) == -1)
    {
        printf(1,"9\n");
        goto bad;
    }
    
    
    printf(1, "[TEST2] DIR\n");
    if(mkdir("testfile/t2") < 0)
        goto bad;
    if(mkdir("testfile/t2/d1") < 0)
        goto bad;
    fd = open("testfile/t2/f1", O_CREATE|O_RDWR);
    if(fd < 0)
    {
        goto bad;
    }
    memset(buf, 0, 32);
    if(write(fd, "2", 2) != 2)
    {
        goto bad;
    }
    
    // FILE MODE
    // 디렉토리(T_DIR)의 경우 다음과 같은 사항이 적용됩니다.
    // Read 권한이 없는 경우,디렉토리 내의 파일목록을 볼 수 없습니다.
    // 파일의 open시 read 권한과 같은 방법으로 처리하면 됩니다.
    if(chmod("testfile/t2/", MODE_WUSR|MODE_XUSR) == -1)
    {
        printf(1,"1\n");
        goto bad;
    }
    if(open("testfile/t2/d1", O_RDWR) == 0)
    {
        printf(1,"2\n");
        goto bad;
    }
    if(open("testfile/t2/d1", O_RDONLY) == 0)
    {
        printf(1,"3\n");
        goto bad;
    }
    
    // Write 권한이 없는 경우, 디렉토리 내의 파일을 삭제하거나, 생성할 수 없습니다.
    // 예를 들어, create 함수에서 새로운 파일을 생성하려 하는 디렉토리에 write 권한이 없다면 생성에 실패해야 하고.
    // sys_unlink 함수에서 파일을 삭제하려 하는 디렉토리에 write 권한이 없다면 삭제에 실패해야 합니다.
    if(chmod("testfile/t2", MODE_RUSR|MODE_XUSR) == -1)
    {
        printf(1, "4\n");
        goto bad;
    }
    fd = open("testfile/t2/f2", O_CREATE|O_RDWR);
    if(fd >= 0)
    {
        printf(1,"5\n");
        goto bad;
    }
    if(unlink("testfile/t2/f1") == 0)
    {
        printf(1,"6\n");
        goto bad;
    }
    
    // Execute 권한이 없는 경우, 디렉토리 내의 파일에 접근할 수 없습니다.
    // 예를 들어, namex를 통해 경로를 탐색하는 과정 중 execute 권한이 없는 디렉토리가 있다면 실패해야 합니다. 또한,
    // execute 권한이 없는 디렉토리로 current working directory를 설정할 수도 없습니다. (이미 cwd 상태인
    // 디렉토리에서 execute 권한이 없어지는 것은 가능합니다.)
    if(chmod("testfile/t2", MODE_WUSR|MODE_RUSR) == -1)
    {
        printf(1,"7\n");
        goto bad;
    }
    if(open("testfile/t2/f1", O_RDWR) == 0)
    {
        printf(1,"8\n");
        goto bad;
    }
    if(chdir("testfile/t2") == 0)
    {
        printf(1, "9\n");
        goto bad;
    }
    
    // TEST 3
    if(chmod("testfile/t2", MODE_RUSR|MODE_WUSR|MODE_XUSR|MODE_ROTH|MODE_XOTH) == -1)
    {
        printf(1,"10\n");
        goto bad;
    }
    if(chmod("testfile/t2/f1", MODE_RUSR|MODE_WUSR|MODE_XUSR|MODE_ROTH|MODE_XOTH) == -1)
    {
        printf(1,"11\n");
        goto bad;
    }
    
    // 파일의 경로에 접근할 때는 그 경로상에 있는 모든 디렉토리에 execute 권한이 있어야만 합니다.
    // 즉, 같은 파일이라고 해도 사용한 경로에 따라 접근이 가능할 수도, 가능하지 않을 수도 있습니다.
    // 현재 작업 중인 디렉토리의 부모 디렉토리에서 execute 권한이 없는 경우,
    // 현재 디렉토리에서 직접적으로 작업하는 것은 가능하지만,
    // 루트로부터 절대경로를 따라 내려오거나, “../현재디렉토리“ 등과 같이 부모 디렉토리를 거쳐오게 할 수는 없습니다.
    printf(1, "[TEST3] TRICKY CASE\n");
    if(chdir("./testfile") == -1)
    {
        printf(1,"1\n");
        goto bad;
    }
    if(chmod("..", MODE_RUSR|MODE_WUSR|MODE_ROTH) == -1)
    {
        printf(1,"2\n");
        goto bad;
    }
    
    if(exec("../ls", argv) == 0)
    {
        printf(1,"3\n");
        goto bad;
    }
    if(exec("/testfile/../ls", argv) == 0)
    {
        printf(1,"4\n");
        goto bad;
    }
    if(open("../testfile/t2/f1", O_RDWR) == 0)
    {
        printf(1,"5\n");
        goto bad;
    }
    if(open("t2/f1", O_RDWR) == -1)
    {
        printf(1,"6\n");
        goto bad;
    }
    // 원상복귀
    if(chmod("..", MODE_RUSR|MODE_WUSR|MODE_XUSR|MODE_ROTH|MODE_XOTH) == -1)
    {
        printf(1,"7\n");
        goto bad;
    }
    if(chdir("..") == -1)
    {
        printf(1,"8\n");
        goto bad;
    }
    if(open("testfile/t2/f1", O_RDWR) == -1)
    {
        printf(1,"9\n");
        goto bad;
    }
    if(open("/testfile/t2/f1", O_RDWR) == -1)
    {
        printf(1,"10\n");
        goto bad;
    }
    
    // 주의: 파일의 권한에 따른 것이 아닌, 단순히 해당 파일의 소유자 여부를 통해서만 change mode를 수행할 권한이 결정됩니다.
    // 단, 그 파일의 경로를 탐색하는 과정에서 execute 권한이 없어 실패할 수는 있습니다.
    printf(1, "[TEST4] CHECK CHMOD\n");
    if(chdir("./testfile") == -1)
    {
        printf(1,"1\n");
        goto bad;
    }
    
    if(chmod(".", MODE_RUSR|MODE_WUSR|MODE_ROTH) == -1)
    {
        printf(1,"2\n");
        goto bad;
    }
    
    if(exec("../ls",argv) == 0)
    {
        printf(1,"3\n");
        goto bad;
    }
    
    if(chmod(".", MODE_RUSR|MODE_WUSR|MODE_XUSR|MODE_ROTH|MODE_XOTH) == 0)
    {
        printf(1,"4\n");
        goto bad;
    }
    
    // sys_chdir 함수의 목적지에 execute 권한이 있는지 확인
    if(chdir("/") == -1)
    {
        printf(1,"5\n");
        goto bad;
    }
    
    printf(1, "[RESULT] ALL PASS!\n");
    close(fd);
    exit();
    bad:
    printf(1, "[RESULT] PLEASE CHECK AGAIN\n");
    exit();
}
