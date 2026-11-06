/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Denis Lemire */
/* Disposable KVM only: attach as root, test after setuid, restore original VT. */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/consio.h>
#include <sys/sysctl.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include "gdm-freebsd-vt.h"
int main(int argc, char **argv)
{
 char guest[32], device[32];size_t size=sizeof(guest);
 struct passwd *pw=NULL;struct vt_mode mode;
 int control=-1, fd=-1, original=0, active=0, vt, status=0, ok=0, forced=0;
 pid_t child=-1;
 struct timespec delay={.tv_nsec=30000000};
 if(argc!=3 || strcmp(argv[1],"--disposable-kvm"))return 64;
 pw=getpwnam(argv[2]);
 if(geteuid()!=0 || !pw || pw->pw_uid==0 ||
    sysctlbyname("kern.vm_guest",guest,&size,NULL,0)!=0 || strcmp(guest,"kvm"))return 64;
 control=open("/dev/ttyv0",O_RDWR|O_NOCTTY|O_CLOEXEC);
 if(control<0 || ioctl(control,VT_GETACTIVE,&original)<0 || original<1)return 1;
 if(!gdm_freebsd_vt_available())goto out;
 fd=gdm_freebsd_vt_open(9,0,&vt,device,sizeof(device));
 if(fd<0 || vt==original || ioctl(fd,VT_GETMODE,&mode)<0 || mode.mode!=VT_AUTO)goto out;
 if(gdm_freebsd_vt_activate(fd,vt)<0)goto out;
 child=fork();
 if(child<0)goto out;
 if(child==0){
  close(control);
  if(setsid()<0 || gdm_freebsd_vt_prepare_stdin(fd)<0 ||
     gdm_freebsd_vt_attach(STDIN_FILENO)<0 ||
     setgroups(1,&pw->pw_gid)<0 || setgid(pw->pw_gid)<0 || setuid(pw->pw_uid)<0)_exit(1);
  if(getuid()==0 || tcgetsid(STDIN_FILENO)!=getsid(0) ||
     (fcntl(STDIN_FILENO,F_GETFD)&FD_CLOEXEC)!=0)_exit(2);
  _exit(0);
 }
 close(fd);fd=-1;
 for(int i=0;i<100;i++){
  pid_t result=waitpid(child,&status,WNOHANG);
  if(result==child){child=-1;ok=WIFEXITED(status)&&WEXITSTATUS(status)==0;break;}
  if(result<0 && errno!=EINTR)break;
  nanosleep(&delay,NULL);
 }
 if(child>0){forced=1;kill(child,SIGKILL);waitpid(child,&status,0);child=-1;}
out:
 if(fd>=0)close(fd);
 if(child>0){forced=1;kill(child,SIGKILL);waitpid(child,&status,0);}
 if(ioctl(control,VT_ACTIVATE,original)<0)ok=0;
 for(int i=0;i<100;i++){
  if(ioctl(control,VT_GETACTIVE,&active)==0 && active==original)break;
  nanosleep(&delay,NULL);
 }
 close(control);
 if(active!=original || forced)ok=0;
 printf("native-vt=%s restored=%s forced-kill=%d\n",ok?"PASS":"FAIL",active==original?"PASS":"FAIL",forced);
 return ok?0:1;
}
