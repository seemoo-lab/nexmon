#ifndef BCM43xx_ASM_UTIL_H_
#define BCM43xx_ASM_UTIL_H_

#include <stdlib.h>
#include <stdint.h>


#define ARRAY_SIZE(x)	(sizeof(x)/sizeof(x[0]))

#undef min
#undef max
#define min(x,y)	((x) < (y) ? (x) : (y))
#define max(x,y)	((x) > (y) ? (x) : (y))


void dump(const char *data,
	  size_t size,
	  const char *description);


void * xmalloc(size_t size);
char * xstrdup(const char *str);


typedef _Bool bool;

typedef uint16_t be16_t;
typedef uint32_t be32_t;

be16_t cpu_to_be16(uint16_t x);
be32_t cpu_to_be32(uint32_t x);

#endif /* BCM43xx_ASM_UTIL_H_ */
