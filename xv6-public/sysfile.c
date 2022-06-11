//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"

#define MAX_USER 10
#define MAX_LEN 15

struct{
  int fd, userIdx, userCnt;
  struct file *f;
  char usernames[MAX_USER][MAX_LEN + 1];
  char passwords[MAX_USER][MAX_LEN + 1];
} ltable;

int
getfilebyfd(int fd, int *pfd, struct file **pf)
{
  struct file *f;

  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;

  if(argint(n, &fd) < 0)
    return -1;

  return getfilebyfd(fd, pfd, pf);
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f)
{
  int fd;
  struct proc *curproc = myproc();

  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd] == 0){
      curproc->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

int
sys_dup(void)
{
  struct file *f;
  int fd;

  if(argfd(0, 0, &f) < 0)
    return -1;
  if((fd=fdalloc(f)) < 0)
    return -1;
  filedup(f);
  return fd;
}

int
sys_read(void)
{
  struct file *f;
  int n;
  char *p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return fileread(f, p, n);
}

int
sys_write(void)
{
  struct file *f;
  int n;
  char *p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return filewrite(f, p, n);
}

int
sys_close(void)
{
  int fd;
  struct file *f;

  if(argfd(0, &fd, &f) < 0)
    return -1;
  myproc()->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

int
sys_fstat(void)
{
  struct file *f;
  struct stat *st;

  if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
    return -1;
  return filestat(f, st);
}

// Create the path new as a link to the same inode as old.
int
sys_link(void)
{
  char name[DIRSIZ], *new, *old;
  struct inode *dp, *ip;

  if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
    return -1;

  begin_op();
  if((ip = namei(old)) == 0){
    end_op();
    return -1;
  }

  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if((dp = nameiparent(new, name)) == 0)
    goto bad;
  ilock(dp);
  if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
    iunlockput(dp);
    goto bad;
  }
  iunlockput(dp);
  iput(ip);

  end_op();

  return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  end_op();
  return -1;
}

// Is the directory dp empty except for "." and ".." ?
static int
isdirempty(struct inode *dp)
{
  int off;
  struct dirent de;

  for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("isdirempty: readi");
    if(de.inum != 0)
      return 0;
  }
  return 1;
}

//PAGEBREAK!
int
sys_unlink(void)
{
  struct inode *ip, *dp;
  struct dirent de;
  struct stat st;
  char name[DIRSIZ], *path;
  uint off;

  if(argstr(0, &path) < 0)
    return -1;

  begin_op();
  if((dp = nameiparent(path, name)) == 0){
    end_op();
    return -1;
  }

  ilock(dp);
  
  stati(dp, &st);
  if(!(checkmod(st) & W_OK)){
    iunlock(dp);
    end_op();
    return -1;
  }

  // Cannot unlink "." or "..".
  if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
    goto bad;

  if((ip = dirlookup(dp, name, &off)) == 0)
    goto bad;
  ilock(ip);

  if(ip->nlink < 1)
    panic("unlink: nlink < 1");
  if(ip->type == T_DIR && !isdirempty(ip)){
    iunlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
    panic("unlink: writei");
  if(ip->type == T_DIR){
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  end_op();

  return 0;

bad:
  iunlockput(dp);
  end_op();
  return -1;
}

static struct inode*
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];
  struct stat st;

  if((dp = nameiparent(path, name)) == 0)
    return 0;
  ilock(dp);

  if((ip = dirlookup(dp, name, 0)) != 0){ // File already exists!
    iunlockput(dp);
    ilock(ip);

    if(type == T_FILE && ip->type == T_FILE){
      if(ltable.userCnt > 0){
        stati(ip, &st);

        if(!(checkmod(st) & W_OK)){
          iunlock(ip);
          return 0;
        }  
      }
      //cprintf("create %s already exists. permission check 1 success: %s\n", name, st.owner);
      return ip;
    }
    iunlockput(ip);
    return 0;
  }
  
  //cprintf("create: doesn't exist. create %s\n", name);
  stati(dp, &st);
  if(!(checkmod(st) & W_OK)){
    //cprintf("create %s: permission check 2 failed: %s\n", name, st.owner);
    iunlock(dp);
    return 0;
  }
  //cprintf("create %s: permission check 2 success: %s\n", name, st.owner);

  if((ip = ialloc(dp->dev, type)) == 0)
    panic("create: ialloc");

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  if(ip->type == T_DIR)
    ip->perm = MODE_RUSR | MODE_WUSR | MODE_XUSR | MODE_ROTH | MODE_XOTH;
  else if(ip->type == T_FILE)
    ip->perm = MODE_RUSR | MODE_WUSR | MODE_ROTH;
  if(ltable.userCnt == 0){
    strncpy((char*)ip->owner, (const char*)"root", 4);
    ip->owner[4] = 0;
  }
  else{
    whoami((char*)ip->owner);
  }
  iupdate(ip);

  if(type == T_DIR){  // Create . and .. entries.
    dp->nlink++;  // for ".."
    //dp->perm = MODE_RUSR | MODE_WUSR | MODE_XUSR | MODE_ROTH | MODE_XOTH;
    //whoami((char*)dp->owner);
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      panic("create dots");
  }

  if(dirlink(dp, name, ip->inum) < 0)
    panic("create: dirlink");

  iunlockput(dp);

  return ip;
}


int
open(char *path, int omode)
{
  int fd, check;
  struct file *f;
  struct inode *ip;
  struct stat st;


  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  }
  else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip); // Here the bug appears. chmod 44 passwd -> useradd -> ls -> boom!
    //cprintf(" open ");
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  stati(ip, &st);
  check = checkmod(st);

  if(((omode == O_RDONLY) | (omode & O_RDWR))){      
    if(!(check & R_OK)){
      iunlock(ip);
      end_op();
      return -1;
    }
  }
  if(((omode & O_WRONLY) | (omode & O_RDWR))){      
    if(!(check & W_OK)){
      iunlock(ip);
      end_op();
      return -1;
    }
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  end_op();

  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
  return fd;
}

int
sys_open(void)
{
  char *path;
  int omode;

  if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
    return -1;

  return open(path, omode);
}

int
mkdir(char *path){
  struct inode *ip;

  begin_op();
  if((ip = create(path, T_DIR, 0, 0)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

int
sys_mkdir(void)
{
  char *path;

  if(argstr(0, &path) < 0)
    return -1;

  return mkdir(path);
}

int
sys_mknod(void)
{
  struct inode *ip;
  char *path;
  int major, minor;

  begin_op();
  if((argstr(0, &path)) < 0 ||
     argint(1, &major) < 0 ||
     argint(2, &minor) < 0 ||
     (ip = create(path, T_DEV, major, minor)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

int
sys_chdir(void)
{
  char *path;
  struct inode *ip;
  struct proc *curproc = myproc();
  struct stat st;
  begin_op();
  if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  //ip->lock.locked = 0;
  ilock(ip);
  stati(ip, &st);
  if(ip->type != T_DIR){
    cprintf("chdir %s: not a directory.\n", path);
    printstat(st);
    iunlockput(ip);
    end_op();
    return -1;
  }
  if(!(checkmod(st) & X_OK)){
    iunlock(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  iput(curproc->cwd);
  end_op();
  curproc->cwd = ip;
  return 0;
}

int
sys_exec(void)
{
  char *path, *argv[MAXARG], rpath[256];
  int i, result;
  uint uargv, uarg;

  if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
    return -1;
  }
  memset(argv, 0, sizeof(argv));
  for(i=0;; i++){
    if(i >= NELEM(argv))
      return -1;
    if(fetchint(uargv+4*i, (int*)&uarg) < 0)
      return -1;
    if(uarg == 0){
      argv[i] = 0;
      break;
    }
    if(fetchstr(uarg, &argv[i]) < 0)
      return -1;
  }
  result = exec(path, argv);
  return result;
  if(result != -1){
    return result;
  }
  else if(path[0] != '.'){
    rpath[0] = '/';
    strncpy(rpath + 1, path, strlen(path));
    rpath[strlen(path) + 1] = 0;
    return exec(rpath, argv);
  }
  else{
    return -1;
  }
}

int
sys_pipe(void)
{
  int *fd;
  struct file *rf, *wf;
  int fd0, fd1;

  if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
    return -1;
  if(pipealloc(&rf, &wf) < 0)
    return -1;
  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
    if(fd0 >= 0)
      myproc()->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  fd[0] = fd0;
  fd[1] = fd1;
  return 0;
}

void loadUsers(){
  char buf[(MAX_LEN*2 + 2)*MAX_USER + 1] = {0};
  int idx, n;
  
  //cprintf("loadUsers begin\n");
  if((ltable.fd = open("/passwd", O_RDONLY | O_CREATE)) < 0){
    panic("loadUsers 1");
  }
  if(getfilebyfd(ltable.fd, 0, &ltable.f) < 0){
    panic("loadUsers 2");
  }

  // passwd is like "root-0000/user-1234/"
  idx = 0;
  if((n = fileread(ltable.f, buf, sizeof(buf))) > 6) {
      int pw = 0, offset = 0;

      for(int i = 0; i < sizeof(buf); i++){
        if(idx == MAX_USER || buf[i] == 0)
          break;

        if(!pw){
          if(buf[i] != '-'){
            ltable.usernames[idx][offset++] = buf[i];
          }
          else{
            pw = 1;
            offset = 0;
          }
        }
        else{
          if(buf[i] != '/'){
            ltable.passwords[idx][offset++] = buf[i];
          }
          else{
            pw = 0;
            offset = 0;
            idx++;
          }
        }
      }
  }
  ltable.userCnt = idx;

  if(n < 0){
      panic("loadUsers 3");
  }

  myproc()->ofile[ltable.fd] = 0;
  fileclose(ltable.f);

  ltable.fd = 0;
  ltable.f = 0;

  //cprintf("loadUsers end\n");
}

int flushUsers(){
  //cprintf("flushUsers begin\n");
  char buf[MAX_LEN*2 + 2 + 1] = {0}; // UID PW - / 0
  
  if((ltable.fd = open("/passwd", O_WRONLY)) < 0){
    return -1;
    panic("flushUsers open");
  }
  if(getfilebyfd(ltable.fd, 0, &ltable.f) < 0){
    return -1;
    panic("flushUsers lookup");
  }

  begin_op();
  ilock(ltable.f->ip);
  if (cleari(ltable.f->ip) < 0){
    return -1;
    panic("flushUsers cleari");
  }
  iunlock(ltable.f->ip);
  end_op();


  ltable.f->off = 0;

  // passwd is like "root-0000/user-1234/"
  for(int i = 0; i < ltable.userCnt; i++){
    char* p = buf;
    int len = strlen(ltable.usernames[i]);

    strncpy(p, ltable.usernames[i], len);
    for(int j = 0; j < len; j++)
      p++;
    *p = '-';
    p++;
    
    len = strlen(ltable.passwords[i]);
    strncpy(p, ltable.passwords[i], len);
    for(int j = 0; j < len; j++)
      p++;
    *p = '/';
    p++;
    *p = 0;

    filewrite(ltable.f, buf, strlen(buf));
  }

  myproc()->ofile[ltable.fd] = 0;
  fileclose(ltable.f);

  ltable.fd = 0;
  ltable.f = 0;
  //cprintf("flushUsers end\n");
  return 0;
}

int deleteUser(char *username){
  //cprintf("deleteUser begin\n");
  int found = 0;
  for(int i = 0; i < ltable.userCnt; i++){
    if(strlen(ltable.usernames[i]) != strlen(username))
      continue;
    if(equals(ltable.usernames[i], username)){
      found = i;
      if(!found)
        cprintf("Denined attempt to delete root user\n");
      break;
    }
  }

  if(!found)
    return -1; // Username is root or not found.

  for(int i = found + 1; i < ltable.userCnt; i++){
    strncpy(ltable.usernames[i-1], ltable.usernames[i], strlen(ltable.usernames[i]));
    ltable.usernames[i-1][strlen(ltable.usernames[i])] = 0;
    strncpy(ltable.passwords[i-1], ltable.passwords[i], strlen(ltable.passwords[i]));
    ltable.passwords[i-1][strlen(ltable.passwords[i])] = 0;
  }
  ltable.userCnt--;
  ltable.usernames[ltable.userCnt][0] = ltable.passwords[ltable.userCnt][0] = 0;

  if(flushUsers() < 0){
    return -1;
  }
  //cprintf("deleteUser end\n");
  return 0;
}

int addUser(char *username, char *password){
  //cprintf("addUser begin\n");
  if(ltable.userCnt == MAX_USER){
    return -1; // Cannot add more user!
  }

  int unlen, pwlen;
  unlen = strlen(username);
  pwlen = strlen(password);

  if(unlen < 2 || unlen > MAX_LEN || pwlen < 2 || pwlen > MAX_LEN){
    cprintf("Username and password must be between 2 and 15 characters\n");
    return -1;
  }

  for(int i = 0; i < ltable.userCnt; i++){
    if(equals(ltable.usernames[i], username)){
      cprintf("Username %s already exists!\n", ltable.usernames[i]);
      return -1; // Username already exists!
    }
  }

  strncpy(ltable.usernames[ltable.userCnt], username, strlen(username));
  strncpy(ltable.passwords[ltable.userCnt], password, strlen(password));


  ltable.userCnt++;

  if(flushUsers() < 0){
    return -1;
  }

  if(ltable.userCnt != 1) mkdir(username);

  struct inode *ip;
  if((ip = namei(username)) == 0){
    return -1;
  }

  begin_op();
  ilock(ip);
  strncpy(ip->owner, username, strlen(username));
  ip->owner[strlen(username)] = 0;
  iupdate(ip);
  iunlock(ip);
  end_op();

  return 0;
}

void whoami(char *dst){
  char *src;
  int len;
  if(ltable.userCnt == 0){
    strncpy(dst, (const char*)"root", 4);
    dst[4] = 0;
    return;
  }

  src = ltable.usernames[ltable.userIdx];
  len = strlen(src);

  strncpy(dst, src, len);
  dst[len] = 0;
}

int
sys_lbegin(void)
{
  loadUsers();

  if(ltable.userCnt == 0){ // Looks like passwd is empty.
    addUser("root", "0000");
  }

  ltable.userIdx = -1;

  return 0;
}

int sys_addUser(void){
  char *username, *password;

  if(argstr(0, &username) < 0 || argstr(1, &password) < 0){
    return -1;
  }

  if(ltable.userIdx > 0){
    cprintf("Only root is able to add a new user\n");
    return -1;
  }

  return addUser(username, password);
}


int sys_deleteUser(void){
  char *username;

  if(argstr(0, &username) < 0){
    return -1;
  }

  if(ltable.userIdx > 0){
    cprintf("Only root is able to delete a user\n");
    return -1;
  }

  return deleteUser(username);
}

int sys_login(void){
  char *username, *password;
  int found = -1;

  if(argstr(0, &username) < 0 || argstr(1, &password) < 0){
    return -1;
  }

  for(int i = 0; i < ltable.userCnt; i++){
    if(strlen(ltable.usernames[i]) != strlen(username))
      continue;
    if(equals(ltable.usernames[i], username)){
      found = i;
      break;
    }
  }

  if(found < 0){
    cprintf("login failed: unknown username\n");
    return -1;
  }

  if(equals(ltable.passwords[found], password)){
    cprintf("login successful\n");
    ltable.userIdx = found;
    return 0;
  }
  else{
    cprintf("login failed: invalid password\n");
    return -1;
  }
}

int sys_whoami(void){
  char dst[MAX_LEN + 1];

  whoami(dst);
  cprintf("%s\n", dst);
  return 0;
}

int
sys_chmod(void)
{
  char *path, curUser[MAX_LEN + 1];
  int mode;
  struct inode *ip;
  struct stat st;

  begin_op();
  if(argstr(0, &path) < 0 || argint(1, &mode) < 0 || (ip = namei(path)) == 0){
    end_op();
    return -1;
  }

  ilock(ip);

  stati(ip, &st);
  whoami(curUser);

  if(st.type == T_DEV){
    iunlock(ip);
    end_op();
    printstat(st);
    return -1;
  }

  if(!equals("root", curUser) && !equals(st.owner, curUser)){
    iunlock(ip);
    end_op();
    printstat(st);
    return -1;
  }

  ip->perm = mode;
  iupdate(ip);
  iunlock(ip);
  end_op();
  return 0;
}

void printstat(struct stat st){
  int p = st.perm;
  char perm[7];

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

  cprintf("[stat] owner: %s / mode: %s / type: ", st.owner, perm);
  if(st.type == T_DIR)
    cprintf("T_DIR");
  if(st.type == T_FILE)
    cprintf("T_FILE");
  if(st.type == T_DEV)
    cprintf("T_DEV");
  cprintf("\n");
}

int checkmod(struct stat st){

  int perm = st.perm, result = 0;
  const char *owner = st.owner;
  char username[MAX_LEN + 1];

  if(st.type == T_DEV){
    return R_OK | W_OK | X_OK;
  }

  whoami(username);
  //cprintf("owner: %s current user: %s\n", owner, username);
  if(equals(owner, username) || equals("root", username)){ // current user is either owner or root
    if(perm & MODE_RUSR)
      result |= R_OK;
    if(perm & MODE_WUSR)
      result |= W_OK;
    if(perm & MODE_XUSR)
      result |= X_OK;
  }
  else{ // owner != current user
    if(perm & MODE_ROTH)
      result |= R_OK;
    if(perm & MODE_WOTH)
      result |= W_OK;
    if(perm & MODE_XOTH)
      result |= X_OK;
  }

  return result;
}