// SPDX-License-Identifier: GPL-3.0-only
/* Fixed107/108, read-only29, bounded20MHz30 while monitor1. No hostRFMON. */
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
struct req {char name[16];uint32_t kind,command,length,padding;void *data;};
_Static_assert(sizeof(struct req)==40,"ABI40");
_Static_assert(offsetof(struct req,data)==32,"ABI pointer");
static volatile sig_atomic_t interrupted;
static void stop(int unused){(void)unused;interrupted=1;}
static int command(int fd,uint32_t operation,uint32_t *value){
    if(operation!=107 && operation!=108){errno=EINVAL;return -1;}
    if(operation==108 && *value>1){errno=EINVAL;return -1;}
    struct req r;memset(&r,0,sizeof(r));memcpy(r.name,"en0",4);
    r.kind=UINT32_MAX;r.command=operation;r.length=4;r.data=value;
    return ioctl(fd,0x802869c8UL,&r);
}
static int chanspec(int fd,uint32_t operation,uint32_t *value){
    if(operation!=262 && operation!=263){errno=EINVAL;return -1;}
    unsigned char buffer[16]={0};memcpy(buffer,"chanspec",9);
    if(operation==263)memcpy(buffer+9,value,4);
    struct req r;memset(&r,0,sizeof(r));memcpy(r.name,"en0",4);r.kind=UINT32_MAX;
    r.command=operation;r.length=operation==262 ? sizeof(buffer):13;r.data=buffer;
    int status=ioctl(fd,0x802869c8UL,&r);
    if(!status && operation==262)memcpy(value,buffer,4);
    return status;
}
int main(int argc,char **argv){
    if(argc==2 && !strcmp(argv[1],"--self-test")){
        puts("{\"self_test\":true,\"firmware_commands\":0}");return 0;
    }
    if(argc==2 && !strcmp(argv[1],"--chanspec")){
        int fd=socket(AF_INET,SOCK_DGRAM,0);if(fd<0)return 1;
        uint32_t value=0;int status=chanspec(fd,262,&value),saved=errno;close(fd);
        printf("{\"status\":%d,\"errno\":%d,\"chanspec\":%u,\"mode_writes\":0}\n",status,status?saved:0,value);
        return status ? 1:0;
    }
    if(argc==3 && !strcmp(argv[1],"--pin-chanspec")){
        char *end=NULL;errno=0;unsigned long candidate=strtoul(argv[2],&end,0);
        if(errno || !end || *end || !candidate || candidate>65535)return 64;
        unsigned int band=candidate&0xc000,bw=candidate&0x3800,side=(candidate>>8)&7,center=candidate&255;
        int width=bw==0x1000 ? 20:bw==0x1800 ? 40:bw==0x2000 ? 80:0;
        int primary=(int)center-(width==80 ? 6:width==40 ? 2:0)+4*(int)side;
        if(!width || (band!=0 && band!=0xc000) || side>=(unsigned int)width/20 || primary<1 || primary>196 || !center || center>196 || (!band && (primary>14 || width==80)) || (band==0xc000 && primary<=14))return 64;
        int fd=socket(AF_INET,SOCK_DGRAM,0);if(fd<0)return 1;
        uint32_t monitor=0,value=(uint32_t)candidate,actual=0;int status=command(fd,107,&monitor),saved=errno;
        if(!status && monitor!=1){status=-1;saved=EBUSY;}
        if(!status){status=chanspec(fd,263,&value);saved=errno;}
        if(!status){status=chanspec(fd,262,&actual);saved=errno;}
        if(!status && actual!=value){status=-1;saved=EIO;}
        close(fd);printf("{\"status\":%d,\"errno\":%d,\"chanspec\":%u,\"width\":%d,\"primary_channel\":%d}\n",status,status?saved:0,actual,width,primary);
        return status ? 1:0;
    }
    if(argc==3 && !strcmp(argv[1],"--pin-channel")){
        char *end=NULL;errno=0;long channel=strtol(argv[2],&end,10);
        if(errno || !end || *end || channel<1 || channel>196)return 64;
        int fd=socket(AF_INET,SOCK_DGRAM,0);if(fd<0)return 1;
        uint32_t monitor=0,value=(uint32_t)channel,actual[3]={0};
        int status=command(fd,107,&monitor),saved=errno;
        if(!status && monitor!=1){status=-1;saved=EBUSY;}
        struct req request;memset(&request,0,sizeof(request));memcpy(request.name,"en0",4);request.kind=UINT32_MAX;
        if(!status){request.command=30;request.length=4;request.data=&value;status=ioctl(fd,0x802869c8UL,&request);saved=errno;}
        if(!status){request.command=29;request.length=sizeof(actual);request.data=actual;status=ioctl(fd,0x802869c8UL,&request);saved=errno;}
        if(!status && (actual[0]!=value || actual[2]!=0)){status=-1;saved=EIO;}
        close(fd);printf("{\"status\":%d,\"errno\":%d,\"pinned_channel\":%u,\"hardware_channel\":%u,\"scan_channel\":%u}\n",status,status?saved:0,value,actual[0],actual[2]);
        return status ? 1 : 0;
    }
    if(argc==2 && !strcmp(argv[1],"--channel")){
        /* Exact19H12 ROM case29 writes three uint32 values; verified offline.
           This path has no setter and never changes monitor/association. */
        int fd=socket(AF_INET,SOCK_DGRAM,0);if(fd<0)return 1;
        uint32_t values[3]={0};struct req request;memset(&request,0,sizeof(request));
        memcpy(request.name,"en0",4);request.kind=UINT32_MAX;request.command=29;
        request.length=sizeof(values);request.data=values;
        int status=ioctl(fd,0x802869c8UL,&request),saved=errno;close(fd);
        printf("{\"status\":%d,\"errno\":%d,\"hardware_channel\":%u,\"target_channel\":%u,\"scan_channel\":%u,\"mode_writes\":0}\n",status,status?saved:0,values[0],values[1],values[2]);
        return status ? 1 : 0;
    }
    int status_only=argc==2 && !strcmp(argv[1],"--status");
    int continuous=argc==2 && !strcmp(argv[1],"--continuous");
    long seconds=0;char *end=NULL;
    if(!status_only && !continuous){
        if(argc!=3 || strcmp(argv[1],"--hold"))return 64;
        errno=0;seconds=strtol(argv[2],&end,10);
        if(errno || !end || *end || seconds<1 || seconds>7200)return 64;
    }
    struct sigaction sa;memset(&sa,0,sizeof(sa));sa.sa_handler=stop;
    sigemptyset(&sa.sa_mask);sigaction(SIGINT,&sa,NULL);sigaction(SIGTERM,&sa,NULL);
    int fd=socket(AF_INET,SOCK_DGRAM,0);if(fd<0)return 1;
    uint32_t before=UINT32_MAX,during=UINT32_MAX,after=UINT32_MAX,restored=UINT32_MAX,value;
    int changed=0,status=1,error=0,restore_status=0;
    const char *phase="baseline";
    if(command(fd,107,&before)){error=errno;goto cleanup;}
    if(status_only){printf("{\"status\":0,\"monitor\":%u,\"firmware_commands\":1,\"mode_writes\":0}\n",before);close(fd);return 0;}
    if(before!=0){error=EBUSY;goto cleanup;}
    if(interrupted){status=0;phase="cancelled_before_enable";goto cleanup;}
    phase="enable";changed=1;value=1;
    if(command(fd,108,&value)){error=errno;goto cleanup;}
    if(command(fd,107,&during) || during!=1){error=errno;goto cleanup;}
    phase="hold";
    long left=seconds;
    while(!interrupted && (continuous || left>0)) {
        struct timespec remaining={1,0};
        while(!interrupted && nanosleep(&remaining,&remaining)<0){if(errno!=EINTR){error=errno;goto cleanup;}}
        if(!continuous)left--;
    }
    phase=interrupted ? "stopped" : "expired";
    if(command(fd,107,&after) || after!=1){error=errno;goto cleanup;}
    status=0;
cleanup:
    if(changed){value=0;restore_status=command(fd,108,&value);}
    if(command(fd,107,&restored) || restored!=0 || restore_status)status=1;
    close(fd);
    printf("{\"status\":%d,\"phase\":\"%s\",\"errno\":%d,\"before\":%u,\"during\":%u,\"after\":%u,"
           "\"restore_status\":%d,\"restored_monitor\":%u,\"monitor_enable_attempts\":%d,\"hold_seconds\":%ld,"
           "\"host_rfmon_used\":false,\"interrupted\":%s}\n",status,phase,error,before,during,after,
           restore_status,restored,changed,seconds,interrupted ? "true" : "false");
    return status;
}
