// SPDX-License-Identifier: GPL-3.0-only
/* Exact-target experimental prototype for the exact 43452a3/7.64.133.15 image.
 * Ownership: gate result1 transfers the original RX packet to fetch_complete;
 * the assembly caller must then take RX epilogue1b7c5e, NOT free1b7c54.
 * Bounds deliberately cover only a subset until hardware validation.
 */
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef unsigned int uptr;
_Static_assert(sizeof(void *)==4,"Wi-Fi ARM32 only");

enum {MAGIC=0x32584652, MAX_PENDING=2, MAX_INLINE=136, MAX_FRAGMENT=1416};
enum {SEQ=1,TOTAL=2,SPLIT=3,QUEUED=4,PENDING=5,SCHEMA=6,COMPLETE=7,
      DELIVERED=8,CANCELLED=9,REJECTED=10,ALLOC_FAIL=11,DISPATCH_FAIL=12,
      MONITOR_OFF=13,MAX_FULL=14,LAST_FULL=15};
static volatile u32 *const stats=(volatile u32 *)0x1eeaf0;
static u16 half(void *p,u32 offset){return *(u16 *)((u8 *)p+offset);}
static u32 word(void *p,u32 offset){return *(u32 *)((u8 *)p+offset);}
static void put_half(void *p,u32 offset,u16 value){*(u16 *)((u8 *)p+offset)=value;}
static void put_word(void *p,u32 offset,u32 value){*(u32 *)((u8 *)p+offset)=value;}

extern void *allocate(u32 n);
extern void release(void *p);
static void zero(void *p,u32 n){((void *(*)(void *,int,u32))0x2989)(p,0,n);}
extern void copy(void *dst,void *src,u32 n);
static void packet_free(void *osh,void *p){((void (*)(void *,void *,int))0x1a71f1)(osh,p,0);}
static void monitor_copy(void *wlc,void *header,void *packet){
    ((void (*)(void *,void *,void *,void *))0x1f6c9)(wlc,header,packet,(void *)0);
}

__attribute__((noinline)) static void change(u32 index,int delta){
    stats[index]+=(u32)delta; /* Pending count only; not a coherent telemetry snapshot. */
}

struct context {
    u32 magic;
    void *wlc,*rx_header,*osh;
    u32 inline_bytes,fragment_bytes;
    void *original,*original_data;
};
struct request {
    void *original;
    u16 headroom;
    short offset;
    void (*callback)(void *,void *,void *,int);
    void *context;
    void *next;
};
struct work {struct request request;struct context context;};
extern int chip_dispatch(struct request *request);
extern void mirror_capture(const void *raw,u32 length);
_Static_assert(sizeof(struct request)==20,"measured hndrte request ABI");
_Static_assert(sizeof(struct work)==52,"bounded allocation");

__attribute__((section(".text.callback"),used))
void fetch_complete(void *fetched,void *original,void *argument,int cancelled){
    struct context *c=(struct context *)argument;
    if(!c || c->magic!=MAGIC)return; /* Unknown callback context is not freed. */

    if(!cancelled && fetched && original==c->original &&
       word(original,8)==(uptr)c->original_data && half(original,12)==c->inline_bytes &&
       half(fetched,12)==c->fragment_bytes){
        u32 base=word(fetched,4)&0x1fffff;
        u32 data=word(fetched,8),low=data&0x1fffff;
        u32 end=base+(half(fetched,6)>>5)+1;
        /* Validate local prepend space and monitor trailer space before CPU writes. */
        if(low>=base && low-base>=c->inline_bytes && low<=end &&
           c->fragment_bytes+45<=end-low){
            if(word(c->wlc,0x204)){
                void *joined=(void *)(data-c->inline_bytes);
                copy(joined,c->original_data,c->inline_bytes);
                put_word(fetched,8,(uptr)joined);
                u32 full=c->inline_bytes+c->fragment_bytes;
                put_half(fetched,12,(u16)full);
                mirror_capture((u8 *)joined+6,full-6);
                monitor_copy(c->wlc,c->rx_header,fetched);

            }
        }
    }

    /* fetched and original are distinct owned packets; context is released last. */
    if(fetched && fetched!=c->original)packet_free(c->osh,fetched);
    packet_free(c->osh,c->original);
    c->magic=0;
    release((u8 *)c-sizeof(struct request));
    change(PENDING,-1);
}

__attribute__((section(".text.gate"),used))
int fetch_gate(void *wlc,void *rx_header,void *packet){

    u16 flags=half(packet,14);
    if(!(flags&0x2000) || (flags&0x0200))return 0;
    u16 fragment=half(packet,0x50);
    if(!fragment)return 0;

    u16 length=half(packet,12);
    if(!word(wlc,0x204) || length<6 || (length&3) || length>MAX_INLINE || fragment>MAX_FRAGMENT ||
       *((u8 *)packet+2)!=1 || half(packet,0x14) || stats[PENDING]>=MAX_PENDING){
        return 0;
    }
    /* Exact dispatch19b4c4..c8 obtains this pool; pool+0xe is its buffer size.
     * Keep the standard136-byte headroom and45-byte monitor trailer inside it
     * BEFORE dispatch can start writing DMA data. Heap fallback is1920 bytes.
     */
    void *group=(void *)word((void *)0x1986dc,0);
    void *pool=group?(void *)word(group,0):(void *)0;
    if(!pool || 136u+fragment+45u>half(pool,14)){
        return 0;
    }
    struct work *work=allocate(sizeof(*work));
    if(!work){return 0;}
    zero(work,sizeof(*work));
    work->request.original=packet;
    work->request.headroom=136; /* Same headroom as the measured normal RX fetch. */
    work->request.callback=fetch_complete;
    work->request.context=&work->context;
    work->context.magic=MAGIC;
    work->context.wlc=wlc;
    work->context.rx_header=rx_header;
    work->context.osh=(void *)word(wlc,4);
    work->context.inline_bytes=length;
    work->context.fragment_bytes=fragment;
    work->context.original=packet;
    work->context.original_data=(void *)word(packet,8);
    change(PENDING,1);
    int result=chip_dispatch(&work->request);
    /* On success callback may already have freed work: do not access it again. */
    if(!result){return 1;}
    change(PENDING,-1);release(work);
    return 0;
}
