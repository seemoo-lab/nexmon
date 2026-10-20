#include <stdint.h>
#include <glib.h>

#ifndef __CMPH_TYPES_H__
#define __CMPH_TYPES_H__

typedef int8_t cmph_int8;
typedef uint8_t cmph_uint8;

typedef int16_t cmph_int16;
typedef uint16_t cmph_uint16;

typedef int32_t cmph_int32;
typedef uint32_t cmph_uint32;

typedef int64_t cmph_int64;
typedef uint64_t cmph_uint64;

typedef enum { CMPH_HASH_JENKINS, CMPH_HASH_COUNT } CMPH_HASH;
extern const char *cmph_hash_names[];
typedef enum { CMPH_BMZ, CMPH_BMZ8, CMPH_CHM, CMPH_BRZ, CMPH_FCH,
               CMPH_BDZ, CMPH_BDZ_PH,
               CMPH_CHD_PH, CMPH_CHD, CMPH_COUNT } CMPH_ALGO;
extern const char *cmph_names[];

#endif
