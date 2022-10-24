// On-disk file system format.
// Both the kernel and user programs use this header file.


#define ROOTINO 1  // root i-number
#define BSIZE 512  // block size

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
  uint logstart;     // Block number of first log block
  uint inodestart;   // Block number of first inode block
  uint bmapstart;    // Block number of first free map block
};

#define NDIRECT 7
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)
#define MAX_LEN 15

// On-disk inode structure
// sizeof dinode => 64bytes
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint perm;            // Permission of this file
  char owner[MAX_LEN + 1]; // username of the owner of this file
  uint addrs[NDIRECT+1];   // Data block addresses
};

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

// Bitmap bits per block
#define BPB           (BSIZE*8)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) (b/BPB + sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};


#define MODE_RUSR 32 // owner read
#define MODE_WUSR 16 // owner write
#define MODE_XUSR 8 // owner execute
#define MODE_ROTH 4 // others read
#define MODE_WOTH 2 // others write
#define MODE_XOTH 1 // others execute

#define R_OK 4  // Read permission checked
#define W_OK 2  // Write permission checked
#define X_OK 1  // Execute permission checked