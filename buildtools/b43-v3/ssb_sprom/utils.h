#ifndef SSB_SPROMTOOL_UTILS_H_
#define SSB_SPROMTOOL_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define ssb_stringify_1(x)	#x
#define ssb_stringify(x)	ssb_stringify_1(x)

#ifdef ATTRIBUTE_FORMAT
# undef ATTRIBUTE_FORMAT
#endif
#ifdef __GNUC__
# define ATTRIBUTE_FORMAT(t, a, b)	__attribute__((format(t, a, b)))
#else
# define ATTRIBUTE_FORMAT(t, a, b)	/* nothing */
#endif

int prinfo(const char *fmt, ...) ATTRIBUTE_FORMAT(printf, 1, 2);
int prerror(const char *fmt, ...) ATTRIBUTE_FORMAT(printf, 1, 2);
int prdata(const char *fmt, ...) ATTRIBUTE_FORMAT(printf, 1, 2);

void internal_error(const char *message);
#define internal_error_on(condition) \
	do {								\
		if (condition)						\
			internal_error(ssb_stringify(condition));	\
	} while (0)

void * malloce(size_t size);
void * realloce(void *ptr, size_t newsize);

/* CRC-8 with polynomial x^8+x^7+x^6+x^4+x^2+1 */
uint8_t crc8(uint8_t crc, uint8_t data);

#endif /* SSB_SPROMTOOL_UTILS_H_ */
