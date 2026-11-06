/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Denis Lemire */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/consio.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
static int opened, closed, indexes[64], next_vt, active, mode, bad_identity;
static int fail_open, fail_ioctl, fail_dup, fail_fcntl, activated, slept, attached;
static unsigned long bad_request;
static int
mock_open(const char *path, int flags, ...)
{
    const char *suffix = "0123456789abcdefghijklmnopqrstuv";
    assert((flags & (O_RDWR | O_NOCTTY | O_CLOEXEC)) == (O_RDWR | O_NOCTTY | O_CLOEXEC));
    if (fail_open) { errno = EACCES; return -1; }
    assert(strncmp(path,"/dev/ttyv",9)==0 && strlen(path)==10);
    const char *found = strchr(suffix, path[9]);
    assert(found);
    indexes[++opened] = (int)(found-suffix)+1;
    return opened;
}
static int mock_close(int fd) { assert(fd>0); closed++; return 0; }
static int mock_fstat(int fd, struct stat *st) { (void)fd; st->st_mode=bad_identity ? S_IFREG : S_IFCHR; return 0; }
static int
mock_ioctl(int fd, unsigned long request, ...)
{
    va_list ap; va_start(ap,request);
    if (fail_ioctl && request == bad_request) { va_end(ap); errno=EIO; return -1; }
    if (request==VT_ACTIVATE) { activated=va_arg(ap,int); }
    else if (request==TIOCSCTTY) { assert(va_arg(ap,int)==0); attached=1; }
    else {
        void *out=va_arg(ap,void *);
        if(request==VT_GETINDEX) *(int *)out=indexes[fd];
        else if(request==VT_GETMODE) ((struct vt_mode *)out)->mode=mode;
        else if(request==VT_OPENQRY) *(int *)out=next_vt;
        else if(request==VT_GETACTIVE) *(int *)out=active;
        else if(request==CONS_GETVERS) *(int *)out=0x200;
        else assert(0);
    }
    va_end(ap); return 0;
}
static int mock_nanosleep(const struct timespec *a, struct timespec *b) { (void)a; (void)b; slept++; return 0; }
static pid_t mock_getsid(pid_t p) { assert(p==0); return 99; }
static pid_t mock_tcgetsid(int fd) { (void)fd; return attached==1 ? 99 : 98; }
static int mock_dup2(int a,int b) { assert(a>0 && b==0); if(fail_dup){errno=EBADF;return -1;} return 0; }
static int mock_fcntl(int fd,int cmd,...) { assert(fd==0); if(fail_fcntl){errno=EIO;return -1;} if(cmd==F_GETFD)return FD_CLOEXEC; va_list a;va_start(a,cmd);assert(va_arg(a,int)==0);va_end(a);return 0; }
#define open mock_open
#define close mock_close
#define fstat mock_fstat
#define ioctl mock_ioctl
#define nanosleep mock_nanosleep
#define getsid mock_getsid
#define tcgetsid mock_tcgetsid
#define dup2 mock_dup2
#define fcntl mock_fcntl
#include "gdm-freebsd-vt.c"
static void reset(void) {opened=closed=0;memset(indexes,0,sizeof(indexes));next_vt=10;active=9;mode=VT_AUTO;bad_identity=fail_open=fail_ioctl=fail_dup=fail_fcntl=activated=slept=attached=0;bad_request=0;}
int main(void)
{
 int vt,fd;char path[32];
 for(int i=1;i<=32;i++) {reset();fd=gdm_freebsd_vt_open(i,1,&vt,path,sizeof(path));assert(fd==1 && vt==i && indexes[fd]==i);mock_close(fd);assert(closed==opened);}
 reset();fd=gdm_freebsd_vt_open(9,0,&vt,path,sizeof(path));assert(fd==2 && vt==10 && !strcmp(path,"/dev/ttyv9") && closed==1);mock_close(fd);assert(closed==opened);
 for(int i=1;i<=32;i++) {if(i==9)continue;reset();next_vt=i;fd=gdm_freebsd_vt_open(9,0,&vt,path,sizeof(path));assert(fd==2 && vt==i);mock_close(fd);assert(closed==opened);}
 reset();next_vt=9;assert(gdm_freebsd_vt_open(9,0,&vt,path,sizeof(path))==-1 && errno==EAGAIN && closed==opened);
 reset();next_vt=33;assert(gdm_freebsd_vt_open(9,0,&vt,path,sizeof(path))==-1 && closed==opened);
 reset();mode=VT_PROCESS;assert(gdm_freebsd_vt_open(9,1,&vt,path,sizeof(path))==-1 && errno==EBUSY && closed==opened);
 reset();bad_identity=1;assert(gdm_freebsd_vt_open(9,1,&vt,path,sizeof(path))==-1 && closed==opened);
 reset();fail_open=1;assert(gdm_freebsd_vt_open(9,1,&vt,path,sizeof(path))==-1 && errno==EACCES);
 reset();assert(gdm_freebsd_vt_open(0,1,&vt,path,sizeof(path))==-1);assert(gdm_freebsd_vt_open(33,1,&vt,path,sizeof(path))==-1);assert(gdm_freebsd_vt_open(9,1,NULL,path,sizeof(path))==-1);assert(gdm_freebsd_vt_open(9,1,&vt,path,2)==-1 && closed==opened);
 for(int i=0;i<3;i++){reset();fail_ioctl=1;bad_request=(unsigned long[]){VT_GETINDEX,VT_GETMODE,VT_OPENQRY}[i];assert(gdm_freebsd_vt_open(9,0,&vt,path,sizeof(path))==-1);}
 reset();assert(gdm_freebsd_vt_available()==1 && closed==opened);reset();fail_ioctl=1;bad_request=VT_GETACTIVE;assert(gdm_freebsd_vt_available()==0 && closed==opened);
 reset();indexes[1]=9;assert(gdm_freebsd_vt_activate(1,9)==0 && activated==9 && slept==0);assert(gdm_freebsd_vt_activate(1,10)==-1 && errno==ENOTTY);
 reset();indexes[1]=9;active=1;assert(gdm_freebsd_vt_activate(1,9)==-1 && errno==ETIMEDOUT && slept==60);
 reset();assert(gdm_freebsd_vt_prepare_stdin(0)==0 && closed==0);assert(gdm_freebsd_vt_prepare_stdin(4)==0 && closed==1);fail_dup=1;assert(gdm_freebsd_vt_prepare_stdin(4)==-1 && closed==1);fail_fcntl=1;assert(gdm_freebsd_vt_prepare_stdin(0)==-1);
 reset();assert(gdm_freebsd_vt_attach(0)==0);fail_ioctl=1;bad_request=TIOCSCTTY;assert(gdm_freebsd_vt_attach(0)==-1);
 puts("PASS: actual native helper, 32 VT names, allocation/failures, owned closure, activation bound, fd0 and child attachment");return 0;
}
