#include "types.h"
#include "stat.h"
#include "fs.h"
#include "fcntl.h"
#include "user.h"

int
inum(char *path)
{
  struct stat st;
  int fd;

  fd = open(path, O_RDONLY);
  if(fstat(fd, &st) < 0){
    printf(1, "pwd: cannot stat %s\n", path);
    exit();
  }
  close(fd);
  return st.ino;
}

void
dirent_by_inum(char *dir_path, int inum, struct dirent *res)
{
  struct dirent de;
  int dir;

  dir = open(dir_path, O_RDONLY);
  while(read(dir, &de, sizeof(de)) == sizeof(de)){
    if(de.inum == inum){
      break;
    }
  }
  close(dir);
  *res = de;
}


int
main(int argc, char *argv[])
{
  char cur[256] = ".", parent[256] = "..";
  char res[64][DIRSIZ + 1];
  int level = 0;

  while(inum(cur) != inum(parent)){
    struct dirent cur_dirent;
    dirent_by_inum(parent, inum(cur), &cur_dirent);

    strcpy(res[level++], cur_dirent.name);

    strcpy(cur, parent);

    char buf[256], *p = parent;
    strcpy(buf, parent);

    strcpy(parent, "../");

    for(int i = 0; i < 3; i++) p++;
    strcpy(p, buf);
  }
  if(level == 0){
    level++;
    res[0][0] = 0;
  }

  for(int i = level - 1; i >= 0; i--){
    printf(1, "/%s", res[i]);
  }
  printf(1, "\n");

  exit();
}