#ifndef BCM43xx_DASM_UTIL_H_
#define BCM43xx_DASM_UTIL_H_

#include <stdlib.h>
#include <stdint.h>


#define ARRAY_SIZE(x)	(sizeof(x)/sizeof(x[0]))


void dump(const char *data,
	  size_t size,
	  const char *description);


void * xmalloc(size_t size);
char * xstrdup(const char *str);
void * xrealloc(void *in, size_t size);

typedef _Bool bool;

typedef uint32_t be32_t;

#endif /* BCM43xx_DASM_UTIL_H_ */
