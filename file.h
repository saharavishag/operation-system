struct file {
    enum {
        FD_NONE, FD_PIPE, FD_INODE
    } type;
    int ref; // reference count
    char readable;
    char writable;
    struct pipe *pipe;
    struct inode *ip;
    uint off;
};


// in-memory copy of an inode
struct inode {
    uint dev;           // Device number
    uint inum;          // Inode number
    int ref;            // Reference count
    struct sleeplock lock; // protects everything below here
    int valid;          // inode has been read from disk?

    short type;         // copy of disk inode
    short major;
    short minor;
    short nlink;
    uint size;
    uint addrs[NDIRECT + 1];
};

// table mapping major device number to
// device functions
struct devsw {
    int (*isdir)(struct inode *);

    void (*iread)(struct inode *, struct inode *);

    int (*read)(struct inode *, char *, int, int);

    int (*write)(struct inode *, char *, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
#define PROCFS  2
#define IS_DEV_DIR(ip) (ip->type == T_DEV && devsw[ip->major].isdir && devsw[ip->major].isdir(ip))
#define PROC_MINOR 0
#define PROC_DIR_SIZE 3 + NPROC
#define FILE_STAT 7
#define FILE_STAT_OFF 1
#define IDE_INFO 8
#define PID_INUM_START 300
#define IDE_INFO_OFF 0
#define INODE_INFO 100
#define INODE_INFO_OFF 2
#define IS_INODE_INFO_INDEX_INUM(ip) ip->inum >= NINODE + INODE_INFO && ip->inum <= NINODE + 2 * INODE_INFO
#define GET_INODE_INFO_INDEX_MINOR(ip) (INODE_INFO + (ip->inum - NINODE - INODE_INFO))
#define GET_INODE_INFO_INDEX_INUM(index) (INODE_INFO + NINODE + index + 1)
#define IS_PID_OFF(off) (off > 2 && off < PROC_DIR_SIZE)
#define PID_NAME_OFF 0
#define PID_STATUS_OFF 1

