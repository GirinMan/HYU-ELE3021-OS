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
fmtsz(uint sz)
{
  char* result;
  uint t;
  char rest = 0;
  char unit = 0;
  char need_to_print_rest = 0;
  uint num_chars = 2;

  if(sz >> 30) {
    unit = 'G';
    need_to_print_rest = (sz & ((1 << 30) - 1)) != 0;
    rest = (sz & ((1 << 30) - 1)) / 10 % 10;
    sz = sz >> 30;
  }
  else if(sz >> 20) {
    unit = 'M';
    need_to_print_rest = (sz & ((1 << 20) - 1)) != 0;
    rest = (sz & ((1 << 20) - 1)) / 10 % 10;
    sz = sz >> 20;
  }
  else if(sz >> 10) {
    unit = 'K';
    need_to_print_rest = (sz & ((1 << 10) - 1)) != 0;
    rest = (sz & ((1 << 10) - 1)) / 10 % 10;
    sz = sz >> 10;
  }

  t = sz;
  do ++num_chars;
  while((t /= 10));

  if(unit)
    num_chars += 1;

  if(need_to_print_rest)
    num_chars += 2;

  result = malloc(num_chars);
  if(result){
    result[--num_chars] = 0;
    result[--num_chars] = 'B';
    if(unit) result[--num_chars] = unit;
    if(need_to_print_rest){
      result[--num_chars] = '0' + rest;
      result[--num_chars] = '.';
    }
    t = sz;
    do {
      result[--num_chars] = '0' + (t % 10);
    }while((t /= 10));
    if(num_chars > 0)
      printf(1, "fail %d\n", num_chars);
  }
  return result;
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
    char* sz_str = fmtsz(st.size);
    printf(1, "%s -%s %d %d %s     \t%s\n", fmtname(path), perm, st.type, st.ino, sz_str, st.owner);
    free(sz_str);
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
        char* sz_str = fmtsz(st.size);
        printf(1, "%s -%s %d %d %s     \t%s\n", fmtname(buf), perm, st.type, st.ino, sz_str, st.owner);
        free(sz_str);
      }
      else if(st.type == T_DIR){
        setperm(st.perm);
        char* sz_str = fmtsz(st.size);
        printf(1, "%s d%s %d %d %s     \t%s\n", fmtname(buf), perm, st.type, st.ino, sz_str, st.owner);
        free(sz_str);
      }
      else{
        char* sz_str = fmtsz(st.size);
        printf(1, "%s *DEVICE %d %d %s     \t%s\n", fmtname(buf), st.type, st.ino, sz_str, st.owner);
        free(sz_str);
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
