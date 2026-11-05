// SPDX-License-Identifier: GPL-3.0-only
/* Experimental nxrawv1 client. No loader, mode switch, channel change or transmit. */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef MIRROR_HOST_TEST
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

enum {MAGIC=0x31574152u,VERSION=1,TOTAL=4160,CHUNK=64,HEADER=32};
struct request64 {char name[16];uint32_t kind,command,length,padding;void *data;};
_Static_assert(sizeof(struct request64)==40 && offsetof(struct request64,data)==32,"legacy64 ABI");
static int failure;
static unsigned commands;
static int command(int fd,unsigned number,void *data,unsigned length){
    commands++;
#ifdef MIRROR_HOST_TEST
    (void)fd;(void)number;(void)data;(void)length;failure=ENOSYS;return -1;
#else
    struct request64 r={0};memcpy(r.name,"en0",4);r.kind=UINT32_MAX;
    r.command=number;r.length=length;r.data=data;
    if(ioctl(fd,0x802869c8UL,&r)){failure=errno;return -1;}return 0;
#endif
}
static unsigned le16(const uint8_t *p){return p[0]|((unsigned)p[1]<<8);}
static unsigned be16(const uint8_t *p){return ((unsigned)p[0]<<8)|p[1];}
static uint32_t word(const uint8_t *p){uint32_t v;memcpy(&v,p,4);return v;}
static int mac_valid(const uint8_t *p){unsigned v=0;for(unsigned i=0;i<6;i++)v|=p[i];return v && !(p[0]&1);}
static int nibble(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
static int parse_mac(const char *s,uint8_t *p){
    if(strlen(s)!=17)return -1;
    for(unsigned i=0;i<6;i++){int a=nibble(s[3*i]),b=nibble(s[3*i+1]);
        if(a<0||b<0||(i<5&&s[3*i+2]!=':'))return -1;p[i]=(uint8_t)((a<<4)|b);}
    return mac_valid(p)?0:-1;
}
static uint32_t crc32(const uint8_t *p,unsigned n){
    uint32_t c=UINT32_MAX;for(unsigned i=0;i<n;i++){c^=p[i];for(unsigned j=0;j<8;j++)c=(c>>1)^((0u-(c&1u))&0xedb88320u);}return ~c;
}
static int header_valid(const uint32_t *w,unsigned offset,unsigned n){
    return n<=CHUNK && offset<=TOTAL && n<=TOTAL-offset && w[0]==MAGIC && w[1]==VERSION &&
        w[2]==w[3] && !(w[2]&1) && w[4]==TOTAL && w[5]==offset && w[6]==n && w[7]<=2;
}
static int query(int fd,unsigned offset,unsigned n,uint8_t *out,uint32_t *seq,uint32_t *pending){
    if(n>CHUNK||offset>TOTAL||n>TOTAL-offset){failure=EINVAL;return -1;}
    uint32_t w[(HEADER+CHUNK)/4]={0};memcpy(w,"nxrawv1",8);
    w[2]=MAGIC;w[3]=VERSION;w[4]=offset;w[5]=n;
    if(command(fd,262,w,HEADER+n))return -1;
    if(!header_valid(w,offset,n)){failure=EPROTO;return -1;}
    memcpy(out,(uint8_t *)w+HEADER,n);*seq=w[2];*pending=w[7];return 0;
}
static int state_valid(const uint8_t *s,uint32_t seq){
    return word(s)==MAGIC && word(s+24)==VERSION && word(s+4)==seq && !(seq&1) &&
        !word(s+16) && word(s+28)<=1 && (!word(s+28)||
        (mac_valid(s+32)&&mac_valid(s+38)&&mac_valid(s+44)));
}
static int state_get(int fd,uint8_t *s,uint32_t *pending){
    uint32_t seq;if(query(fd,0,64,s,&seq,pending))return -1;
    if(!state_valid(s,seq)){failure=EPROTO;return -1;}return 0;
}
static int frame_valid(const uint8_t *p,unsigned n,const uint8_t *s){
    if(n<135||n>496||crc32(p,n-4)!=word(p+n-4))return 0;
    unsigned fc=le16(p),type=fc&0xc4ff,ds=(fc>>8)&3;
    if((type!=8&&type!=0x88)||(ds!=1&&ds!=2)||(p[22]&15))return 0;
    const uint8_t *ap=p+(ds==1?4:10),*peer=p+(ds==1?10:4);
    if(!mac_valid(peer)||memcmp(ap,s+32,6))return 0;
    if(!memcmp(peer,s+38,6)||!memcmp(peer,s+44,6))return 0;
    unsigned h=24;if(fc&0x80){if(p[24]&0x80)return 0;h=26;}
    static const uint8_t llc[8]={0xaa,0xaa,3,0,0,0,0x88,0x8e};
    if(memcmp(p+h,llc,8))return 0;
    const uint8_t *e=p+h+8;unsigned body=be16(e+2);
    if((unsigned)(e[0]-1)>2||e[1]!=3||e[4]!=2||body<95||h+16+body!=n)return 0;
    return be16(e+97)+95==body;
}
static int snapshot_valid(const uint8_t *s,uint32_t seq){
    if(!state_valid(s,seq))return 0;
    uint32_t total=word(s+8);unsigned found=0;
    for(unsigned i=0;i<8;i++){
        const uint8_t *slot=s+64+i*512;uint32_t ordinal=word(slot);
        if(!ordinal)continue;
        unsigned n=word(slot+4);
        if(!total||total-ordinal>=8||((ordinal-1)&7)!=i||word(slot+12)||n<135||n>496||
           word(slot+8)!=le16(slot+16)||!frame_valid(slot+16,n,s))return 0;
        found++;
    }
    return found==(total<8?total:8);
}
static int snapshot_get(int fd,uint8_t *bytes){
    for(unsigned attempt=0;attempt<3;attempt++){
        uint32_t first=UINT32_MAX;int coherent=1;
        for(unsigned offset=0;offset<TOTAL;offset+=CHUNK){
            unsigned n=TOTAL-offset<CHUNK?TOTAL-offset:CHUNK;uint32_t seq,pending;
            if(query(fd,offset,n,bytes+offset,&seq,&pending))return -1;
            if(first!=UINT32_MAX&&seq!=first){coherent=0;break;}first=seq;
        }
        if(coherent){if(snapshot_valid(bytes,first))return 0;failure=EPROTO;return -1;}
    }
    failure=EBUSY;return -1;
}
static int self_test(void){
    uint32_t h[8]={MAGIC,VERSION,2,2,TOTAL,0,64,0};uint8_t mac[6];
    if(!header_valid(h,0,64))return 1;
    h[7]=3;if(header_valid(h,0,64))return 2;h[7]=0;
    h[3]=4;if(header_valid(h,0,64))return 3;h[3]=2;
    if(header_valid(h,4161,0)||header_valid(h,0,65))return 4;
    if(parse_mac("02:00:00:00:00:01",mac)||!parse_mac("+2:00:00:00:00:01",mac)||!parse_mac("ff:ff:ff:ff:ff:ff",mac))return 5;
    if(crc32((const uint8_t *)"123456789",9)!=0xcbf43926u)return 6;
    uint8_t s[TOTAL]={0};memcpy(s,h,8);memcpy(s+4,h+2,4);memcpy(s+24,h+1,4);
    if(!snapshot_valid(s,2))return 7;
    uint32_t one=1;memcpy(s+8,&one,4);memcpy(s+64,&one,4);
    if(snapshot_valid(s,2))return 8;
    s[32]=2;s[38]=2;s[39]=1;s[44]=2;s[45]=2;
    uint8_t *p=s+80;p[0]=0x88;p[1]=2;p[4]=2;p[5]=3;memcpy(p+10,s+32,6);
    static const uint8_t llc[8]={0xaa,0xaa,3,0,0,0,0x88,0x8e};memcpy(p+26,llc,8);
    p[34]=2;p[35]=3;p[37]=95;p[38]=2;
    uint32_t n=137,fc=0x288,crc=crc32(p,133);memcpy(p+133,&crc,4);
    memcpy(s+68,&n,4);memcpy(s+72,&fc,4);
    if(!snapshot_valid(s,2))return 9;
    p[136]^=1;if(snapshot_valid(s,2))return 10;p[136]^=1;
    s[32]^=2;if(snapshot_valid(s,2))return 11;s[32]^=2;
    if(commands)return 12;
    puts("{\"self_test\":true,\"firmware_commands\":0}");return 0;
}
int main(int argc,char **argv){
    if(argc==2&&!strcmp(argv[1],"--self-test"))return self_test();
#ifdef MIRROR_HOST_TEST
    if(argc==3&&!strcmp(argv[1],"--validate-fixture")){
        FILE *f=fopen(argv[2],"rb");if(!f)return 1;
        uint8_t bytes[TOTAL];size_t n=fread(bytes,1,TOTAL,f);int extra=fgetc(f);fclose(f);
        int ok=n==TOTAL&&extra==EOF&&snapshot_valid(bytes,word(bytes+4));
        printf("{\"valid\":%s,\"firmware_commands\":%u}\n",ok?"true":"false",commands);
        return ok?0:1;
    }
#endif
    int info=argc==2&&!strcmp(argv[1],"--info"),snap=argc==2&&!strcmp(argv[1],"--snapshot");
    int off=argc==2&&!strcmp(argv[1],"--disarm"),on=argc==5&&!strcmp(argv[1],"--arm");
    uint8_t addresses[18]={0};
    if(!info&&!snap&&!off&&!on)return 64;
    if(on)for(unsigned i=0;i<3;i++)if(parse_mac(argv[2+i],addresses+6*i))return 64;
#ifdef MIRROR_HOST_TEST
    int fd=-1;failure=ENOSYS;
#else
    int fd=socket(AF_INET,SOCK_DGRAM,0);if(fd<0)return 1;
#endif
    int status=0;
    if(on||off){
        uint32_t w[16]={0};memcpy(w,"nxrawv1",8);w[2]=MAGIC;w[3]=VERSION;w[4]=on?1:0;
        memcpy((uint8_t *)w+20,addresses,18);
        status=command(fd,263,w,64);
        uint8_t state[64];uint32_t pending;
        if(!status)status=state_get(fd,state,&pending);
        if(!status&&(word(state+28)!=(unsigned)on||(on&&memcmp(state+32,addresses,18)))){failure=EPROTO;status=-1;}
        if(!status)printf("{\"ok\":true,\"enabled\":%s,\"epoch\":%u}\n",on?"true":"false",word(state+52));
    }else if(info){
        uint8_t s[64];uint32_t pending;status=state_get(fd,s,&pending);
        if(!status)printf("{\"ok\":true,\"version\":1,\"enabled\":%s,\"events\":%u,\"allocation_failures\":%u,\"epoch\":%u,\"pending\":%u}\n",word(s+28)?"true":"false",word(s+8),word(s+20),word(s+52),pending);
    }else{
        uint8_t bytes[TOTAL];status=snapshot_get(fd,bytes);
        if(!status&&fwrite(bytes,1,TOTAL,stdout)!=TOTAL){failure=EIO;status=-1;}
    }
#ifndef MIRROR_HOST_TEST
    close(fd);
#endif
    if(status){if(snap)fprintf(stderr,"Raw snapshot failed errno=%d\n",failure);
        else printf("{\"ok\":false,\"errno\":%d}\n",failure);return 2;}return 0;
}
