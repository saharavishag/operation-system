//
// Created by nadav on 6/4/19.
//
#include "types.h"
#include "stat.h"
#include "user.h"

int main(){
    char buf[16];
//    memset(buf, 0, sizeof(buf));
//    sprintf(buf,"Hello %s %d %p\n","World!", sizeof(ushort), &buf);
//    printf(1,"%s",buf);
    printf(1,"Hello %s %d %p\n","World!",5, &buf);
    exit();
}

