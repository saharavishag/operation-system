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
#define FILE_STAT 7
#define FILE_STAT_OFF 1
#define IDE_INFO 8
#define IDE_INFO_OFF 0
#define INODE_INFO 100
#define INODE_INFO_OFF 2
#define IS_INODE_INFO(minor) (minor >= 100 && minor <= 200)
#define GET_INODE_INFO_INDEX(minor) (minor - 100)
