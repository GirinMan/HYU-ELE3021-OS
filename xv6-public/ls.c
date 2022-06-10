#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

char perm[7] = {0};

static void setperm(int p){
  if(p & MODE_RUSR)
    perm[0] = 'r';
  else
    perm[0] = '-';
  if(p & MODE_WUSR)
    perm[1] = 'w';
  else
    perm[1] = '-';
  if(p & MODE_XUSR)
    perm[2] = 'x';
  else
    perm[2] = '-';
  if(p & MODE_ROTH)
    perm[3] = 'r';
  else
    perm[3] = '-';
  if(p & MODE_WOTH)
    perm[4] = 'w';
  else
    perm[4] = '-';
  if(p & MODE_XOTH)
    perm[5] = 'x';
  else
    perm[5] = '-';
  perm[6] = 0;
}

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    printf(1, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    printf(1, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    setperm(st.perm);
    printf(1, "%s -%s  %d %d %d   \t%s\n", 
      fmtname(path), perm, st.type, st.ino, st.size, st.owner);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf(1, "ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }
      if(st.type == T_FILE){
        setperm(st.perm);
        printf(1, "%s -%s  %d %d %d   \t%s\n", 
          fmtname(buf), perm, st.type, st.ino, st.size, st.owner);
      }
      else if(st.type == T_DIR){
        setperm(st.perm);
        printf(1, "%s d%s  %d %d %d   \t%s\n", 
          fmtname(buf), perm, st.type, st.ino, st.size, st.owner);
      }
      else{
        printf(1, "%s -------  %d %d %d\n", 
          fmtname(buf), st.type, st.ino, st.size);
      }
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    ls(".");
    exit();
  }
  for(i=1; i<argc; i++)
    ls(argv[i]);
  exit();
}
