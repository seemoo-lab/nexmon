## lock-obj-pub.linux-android.h
## Android (bionic) fallback picked up by mkheader when cross building
## for *-linux-android hosts; the 48-byte buffer is large enough for
## bionic's pthread_mutex_t on all supported ABIs (modeled on
## lock-obj-pub.aarch64-unknown-linux-gnu.h).

typedef struct
{
  long _vers;
  union {
    volatile char _priv[48];
    long _x_align;
    long *_xp_align;
  } u;
} gpgrt_lock_t;

#define GPGRT_LOCK_INITIALIZER {1,{{0,0,0,0,0,0,0,0, \
                                    0,0,0,0,0,0,0,0, \
                                    0,0,0,0,0,0,0,0, \
                                    0,0,0,0,0,0,0,0, \
                                    0,0,0,0,0,0,0,0, \
                                    0,0,0,0,0,0,0,0}}}
##
## Local Variables:
## mode: c
## buffer-read-only: t
## End:
##
