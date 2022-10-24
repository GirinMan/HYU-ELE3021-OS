#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device 

#define MAX_LEN 15

struct stat {
  short type;  // Type of file
  int dev;     // File system's disk device
  uint ino;    // Inode number
  short nlink; // Number of links to file
  uint size;   // Size of file in bytes
  uint perm;   // Permission of file
  char owner[MAX_LEN + 1]; // Username of owner
};
