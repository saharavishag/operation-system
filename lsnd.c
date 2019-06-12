//
// Created by nadav and avishag on 6/12/19.
//
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

void getData(char *buf, char *s) {
    char *temp = s;
    char *s1, *s2;
    int len = 0;


    for (int i = 0; i < 7; ++i) {

        s1 = strchr(buf, ':') + 2;
        s2 = strchr(buf, '\n');
        len = (int) s2 - (int) s1;

        if (i == 0 || i == 1) {
            memmove(temp + 1, s1, len);
            *temp = '#';
            len++;
        } else {
            memmove(temp, s1, len);
        }
        buf = s2 + 1;
        temp += len;

        *temp = ' ';
        temp++;
    }
    *temp = '\0';
}


int main(void) {
    int inodesDirFd = open("/proc/inodeinfo", 0);
    struct dirent de;
    char path[64];
    char buf[512];
    char result[512];

    while (read(inodesDirFd, &de, sizeof(de)) > 0) {
        sprintf(path, "/proc/inodeinfo/%s", de.name);
        int inodeFd = open(path, 0);
        if (read(inodeFd, &buf, sizeof(buf)) <= 0) {
            printf(1, "reading name: %s inum: %d failed\n", de.name, de.inum);
        }
        getData(buf, result);
        printf(1, "%s\n", result);

        close(inodeFd);

    }
    close(inodesDirFd);


    exit();
}


