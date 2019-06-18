#include "types.h"
#include "stat.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

int
procfsisdir(struct inode *ip) {
    if (ip->minor == PROC_MINOR) {
        return 1;
    } else if (ip->minor == INODE_INFO) {
        return 1;
    } else if (ip->minor > PID_INUM_START && (ip->minor & 3) == 0) {
        return 1;
    }
    return 0;
}

void
procfsiread(struct inode *dp, struct inode *ip) {
    ip->major = PROCFS;
    ip->type = T_DEV;
    ip->valid = 1;
    ip->ref = (ip->ref > 0) ? ip->ref : 1;
    if (ip->inum == TOTAL_INODE_NUM + FILE_STAT) {
        ip->minor = FILE_STAT;
    } else if (ip->inum == TOTAL_INODE_NUM + IDE_INFO) {
        ip->minor = IDE_INFO;
    } else if (ip->inum == TOTAL_INODE_NUM + INODE_INFO) {
        ip->minor = INODE_INFO;
    } else if (IS_INODE_INFO_INDEX_INUM(ip)) {
        ip->minor = (short) GET_INODE_INFO_INDEX_MINOR(ip);
    } else if (ip->inum > PID_INUM_START && (ip->inum & 3) == 0) {
        ip->minor = (short) ip->inum;
    } else if (ip->inum > PID_INUM_START) {
        ip->minor = (short) ip->inum;
    }


}

int
procfsread(struct inode *ip, char *dst, int off, int n) {
    //case /proc
    if (ip->minor == PROC_MINOR) {
        struct dirent *de = (struct dirent *) dst;
        int deOff = off / sizeof(struct dirent);
        int pid = 0;
        int index = 0;
        switch (deOff) {

            case CURRENT_DIR_OFF:
                safestrcpy(de->name, ".", sizeof("."));
                de->inum = (ushort) ip->inum;
                return sizeof(struct dirent);
            case PARENT_DIR_OFF:
                safestrcpy(de->name, "..", sizeof(".."));
                de->inum = (ushort) namei("/")->inum;
                return sizeof(struct dirent);
            case IDE_INFO_OFF:
                safestrcpy(de->name, "ideinfo", sizeof("ideinfo"));
                de->inum = (IDE_INFO + TOTAL_INODE_NUM);
                return sizeof(struct dirent);

            case FILE_STAT_OFF:
                safestrcpy(de->name, "filestat", sizeof("filestat"));
                de->inum = (FILE_STAT + TOTAL_INODE_NUM);
                return sizeof(struct dirent);
            case INODE_INFO_OFF:
                safestrcpy(de->name, "inodeinfo", sizeof("inodeinfo"));
                de->inum = (INODE_INFO + TOTAL_INODE_NUM);
                return sizeof(struct dirent);
            default:
                index = deOff - INODE_INFO_OFF;
                pid = getPIDByIndex(index);
                if (pid < 0)
                    return 0;
                sprintf(de->name, "%d", pid);
                de->inum = (ushort) (PID_INUM_START + pid) << 2;
                return sizeof(struct dirent);
        }
    } else if (ip->minor == FILE_STAT) {
        if (off > 0)
            return 0;
        int freeFds = 0;
        int uniqueInodesFds = 0;
        int writableFds = 0;
        int readablesFds = 0;
        int totalRefs = 0;
        int usedFds = 0;
        procfs_filestat(&freeFds, &uniqueInodesFds, &writableFds, &readablesFds, &totalRefs, &usedFds);
        sprintf(dst, "Free fds: %d\nUnique inode fds: %d\nWritable fds: %d\nReadable fds: %d\nRefs per fds: %d/%d\n",
                freeFds, uniqueInodesFds, writableFds, readablesFds, totalRefs, usedFds);
        return strlen(dst);

    }//case /proc/ideinfo
    else if (ip->minor == IDE_INFO) {
        if (off > 0)
            return 0;
        int waitOp = 0;
        int readOp = 0;
        int writeOp = 0;
        char workingBlocks[512];
        ideinfo(&waitOp, &readOp, &writeOp, workingBlocks);
        sprintf(dst,
                "Waiting operations: %d\nRead waiting operations: %d\nWrite waiting operations: %d\nWorking blocks: %s\n",
                waitOp, readOp, writeOp, workingBlocks);
        return strlen(dst);
    } //case /proc/inodeinfo
    else if (ip->minor == INODE_INFO) {
        struct dirent *de = (struct dirent *) dst;
        int deOff = off / sizeof(struct dirent);
        if (deOff == CURRENT_DIR_OFF) {
            safestrcpy(de->name, ".", sizeof("."));
            de->inum = (ushort) ip->inum;
            return sizeof(struct dirent);
        } else if (deOff == PARENT_DIR_OFF) {
            safestrcpy(de->name, "..", sizeof(".."));
            de->inum = (ushort) namei("/proc")->inum;
            return sizeof(struct dirent);
        } else {
            deOff -= 2;
            int index = indexInInodeTable(deOff);
            if (index < 0)
                return 0;
            sprintf(de->name, "%d", index);
            de->inum = (ushort) GET_INODE_INFO_INDEX_INUM(index);
            return sizeof(struct dirent);
        }
    } //case /proc/inodeinfo/...
    else if (ip->minor > INODE_INFO && ip->minor <= INODE_INFO + NINODE + 1) {
        if (off > 0)
            return 0;
        int index = ip->minor - 1 - INODE_INFO;
        struct inode *np = getInodeByIndex(index);
        char *types[] = {"", "FILE", "DIR", "DEV"};
        sprintf(dst,
                "Device: %d\nInode number: %d\nis valid: %d\ntype: %s\nmajor minor: (%d,%d)\nhard links: %d\nblocks used: %d\n",
                np->dev, np->inum, (np->valid) ? 1 : 0, types[np->type], np->major, np->minor, np->nlink,
                (np->type != T_DEV) ? np->size / BSIZE + (np->size % BSIZE > 0) : 0);
        return strlen(dst);
    }//case /proc/PID
    else if (ip->minor > PID_INUM_START && (ip->minor & 3) == 0) {
        struct dirent *de = (struct dirent *) dst;
        int deOff = off / sizeof(struct dirent);
        if(deOff == CURRENT_DIR_OFF){
            safestrcpy(de->name, ".", sizeof("."));
            de->inum = (ushort) ip->inum;
            return sizeof(struct dirent);
        }else if(deOff == PARENT_DIR_OFF){
            safestrcpy(de->name, "..", sizeof(".."));
            de->inum = (ushort) namei("/proc")->inum;
            return sizeof(struct dirent);
        }else if (deOff == PID_NAME_OFF) {
            sprintf(de->name, "name");
            de->inum = (ushort) (ip->inum + 1);
            return sizeof(struct dirent);
        } else if (deOff == PID_STATUS_OFF) {
            sprintf(de->name, "status");
            de->inum = (ushort) (ip->inum + 2);
            return sizeof(struct dirent);
        }
    } else if (ip->minor > PID_INUM_START && ip->minor & 1) {
        if (off > 0)
            return 0;
        char procName[16];
        int pid = ((ip->minor - 1) >> 2) - PID_INUM_START;
        if (getProcInfo(pid, procName, 0, 0) < 0)
            return 0;
        sprintf(dst, "%s\n", procName);
        return strlen(dst);
    } else if (ip->minor > PID_INUM_START && ip->minor & 2) {
        if (off > 0)
            return 0;
        char status[16];
        int size = 0;
        int pid = ((ip->minor - 2) >> 2) - PID_INUM_START;
        if (getProcInfo(pid, 0, &size, status) < 0)
            return 0;
        sprintf(dst, "state: %s\nsize: %d\n", status, size);
        return strlen(dst);
    }
    return 0;
}

int
procfswrite(struct inode *ip, char *buf, int n) {
    return -1;
}

void
procfsinit(void) {
    devsw[PROCFS].isdir = procfsisdir;
    devsw[PROCFS].iread = procfsiread;
    devsw[PROCFS].write = procfswrite;
    devsw[PROCFS].read = procfsread;
}
