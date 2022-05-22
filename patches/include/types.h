#ifndef TYPES_H
#define TYPES_H

typedef unsigned char           byte;
typedef unsigned short          word;
typedef unsigned int            dword;
typedef unsigned char           bool;
typedef signed char             int8_t;
typedef unsigned char           uint8_t;
typedef signed short int        int16_t;
typedef unsigned short int      uint16_t;
typedef signed int              int32_t;
typedef unsigned int            uint32_t;
#if __WORDSIZE == 64
typedef unsigned long int       uint64_t;
typedef signed long int         int64_t;
typedef long unsigned int       size_t;
#else
typedef unsigned long long      uint64_t;
typedef long long               int64_t;
typedef uint32_t                size_t;
#endif
typedef int8_t                  int8;
typedef uint8_t                 uint8;
typedef int16_t                 int16;
typedef uint16_t                uint16;
typedef int32_t                 int32;
typedef uint32_t                uint32;
typedef long long               int64;
typedef unsigned long long      uint64;
typedef unsigned char           uchar_t;
typedef uint32_t                wchar_t;
typedef uint32_t                addr_t;
typedef int32_t                 pid_t;
typedef uint32_t                uint;

#endif
