// SPDX-License-Identifier: GPL-3.0-only
/* Fixed own-supplicant control and snapshot; never transmit or change RF mode. */
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

enum { MAGIC=0x32545541u, VERSION=2, TOTAL=2368, HEADER=32, CHUNK=64 };
struct request64 { char name[16]; uint32_t kind,command,length,padding; void *data; };
_Static_assert(sizeof(struct request64)==40,"legacy ABI");
_Static_assert(offsetof(struct request64,data)==32,"pointer ABI");
static int failure;

static int command(int fd,unsigned number,void *data,unsigned length) {
    struct request64 r;memset(&r,0,sizeof(r));memcpy(r.name,"en0",4);
    r.kind=UINT32_MAX;r.command=number;r.length=length;r.data=data;
    if(ioctl(fd,0x802869c8UL,&r)){failure=errno;return -1;}return 0;
}
static int valid(const uint32_t *w,unsigned offset,unsigned length) {
    return w[0]==MAGIC && w[1]==VERSION && w[2]==w[3] && !(w[2]&1) &&
        w[4]==TOTAL && w[5]==offset && w[6]==length && w[7]==0;
}
static int query(int fd,unsigned offset,unsigned length,uint8_t *out,uint32_t *sequence) {
    if(length>CHUNK || offset>TOTAL || length>TOTAL-offset)return -1;
    uint32_t w[(HEADER+CHUNK)/4];memset(w,0,sizeof(w));memcpy(w,"nxauth2",8);
    w[2]=MAGIC;w[3]=VERSION;w[4]=offset;w[5]=length;
    if(command(fd,262,w,HEADER+length))return -1;
    if(!valid(w,offset,length)){failure=EPROTO;return -1;}
    memcpy(out,(uint8_t *)w+HEADER,length);*sequence=w[2];return 0;
}
static int get_state(int fd,uint32_t *state) {
    uint32_t seq;
    if(query(fd,0,64,(uint8_t *)state,&seq))return -1;
    if(state[0]!=MAGIC || state[6]!=VERSION || state[1]!=seq || state[4] || state[7]>1){failure=EPROTO;return -1;}
    return 0;
}
static void hex(const uint8_t *data,unsigned count) {for(unsigned i=0;i<count;i++)printf("%02x",data[i]);}
static int parse_mac(const char *text,uint8_t *bytes) {
    if(strlen(text)!=17)return -1;
    unsigned total=0;
    for(unsigned i=0;i<6;i++){
        char part[3]={text[i*3],text[i*3+1],0};char *end=NULL;
        unsigned long value=strtoul(part,&end,16);
        if(!end || *end || value>255 || (i<5 && text[i*3+2]!=':'))return -1;
        bytes[i]=(uint8_t)value;total|=bytes[i];
    }
    return total && !(bytes[0]&1)?0:-1;
}
int main(int argc,char **argv) {
    if(argc==2 && !strcmp(argv[1],"--self-test")) {
        uint32_t w[8]={MAGIC,VERSION,2,2,TOTAL,0,64,0};uint8_t mac[6];
        if(!valid(w,0,64))return 1;w[3]=4;if(valid(w,0,64))return 2;
        if(parse_mac("02:00:00:00:00:01",mac) || !parse_mac("ff:ff:ff:ff:ff:ff",mac))return 3;
        puts("{\"self_test\":true,\"firmware_commands\":0}");return 0;
    }
    int info=argc==2 && !strcmp(argv[1],"--info");
    int network=argc==2 && !strcmp(argv[1],"--network");
    int disarm=argc==2 && !strcmp(argv[1],"--disarm");
    int arm=argc==3 && !strcmp(argv[1],"--arm");
    int snapshot=argc==2 && !strcmp(argv[1],"--snapshot");
    if(!info && !network && !disarm && !arm && !snapshot)return 64;
    uint8_t mac[6]={0};if(arm && parse_mac(argv[2],mac))return 64;
    int fd=socket(AF_INET,SOCK_DGRAM,0);if(fd<0)return 1;int status=0;
    if(network) {
        uint8_t bssid[6]={0};struct {uint32_t length;uint8_t bytes[32];} ssid={0};
        status=command(fd,23,bssid,sizeof(bssid));if(!status)status=command(fd,25,&ssid,sizeof(ssid));
        if(!status && (!ssid.length || ssid.length>32 || !(bssid[0]|bssid[1]|bssid[2]|bssid[3]|bssid[4]|bssid[5]))){failure=ENOTCONN;status=-1;}
        if(!status){printf("{\"ok\":true,\"bssid\":\"%02x:%02x:%02x:%02x:%02x:%02x\",\"ssid_hex\":\"",bssid[0],bssid[1],bssid[2],bssid[3],bssid[4],bssid[5]);hex(ssid.bytes,ssid.length);puts("\"}");}
    } else if(arm || disarm) {
        uint32_t w[8]={0};memcpy(w,"nxauth2",8);w[2]=MAGIC;w[3]=VERSION;w[4]=arm?1:0;memcpy((uint8_t *)w+20,mac,6);
        status=command(fd,263,w,sizeof(w));
        uint32_t state[16];if(!status)status=get_state(fd,state);
        if(!status && (state[7]!=(unsigned)arm || (arm && memcmp((uint8_t *)state+32,mac,6)))){failure=EPROTO;status=-1;}
        if(!status)printf("{\"ok\":true,\"enabled\":%s,\"epoch\":%u}\n",state[7]?"true":"false",state[10]);
    } else if(info) {
        uint32_t s[16];status=get_state(fd,s);
        if(!status)printf("{\"ok\":true,\"version\":2,\"enabled\":%s,\"sequence\":%u,\"total_events\":%u,\"allocation_failures\":%u,\"epoch\":%u}\n",s[7]?"true":"false",s[1],s[2],s[5],s[10]);
    } else {
        uint8_t bytes[TOTAL];int coherent=0;
        for(unsigned attempt=0;attempt<3 && !coherent;attempt++) {
            uint32_t first=UINT32_MAX;coherent=1;
            for(unsigned offset=0;offset<TOTAL;) {
                unsigned n=TOTAL-offset<CHUNK?TOTAL-offset:CHUNK;uint32_t seq;
                if(query(fd,offset,n,bytes+offset,&seq)){status=-1;coherent=0;break;}
                if(first!=UINT32_MAX && seq!=first){coherent=0;break;}first=seq;offset+=n;
            }
            if(status)break;
        }
        if(!status && !coherent){failure=EBUSY;status=-1;}
        if(!status && fwrite(bytes,1,sizeof(bytes),stdout)!=sizeof(bytes)){failure=EIO;status=-1;}
    }
    close(fd);
    if(status){
        if(snapshot)fprintf(stderr,"Snapshot failed errno=%d\n",failure);
        else printf("{\"ok\":false,\"errno\":%d}\n",failure);
        return 2;
    }
    return 0;
}
