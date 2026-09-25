/* String descriptors.
   Copyright (C) 2023-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2023.  */

#ifndef _STRING_DESC_H
#define _STRING_DESC_H 1

/* This file uses _GL_INLINE_HEADER_BEGIN, _GL_INLINE,
   _GL_ATTRIBUTE_NODISCARD, _GL_ATTRIBUTE_NONNULL,
   _GL_ATTRIBUTE_NONNULL_IF_NONZERO.  */
#if !_GL_CONFIG_H_INCLUDED
 #error "Please include config.h first."
#endif

/* Get ptrdiff_t.  */
#include <stddef.h>

/* Get FILE.  */
#include <stdio.h>

/* Get abort(), free().  */
#include <stdlib.h>

/* Get idx_t.  */
#include "idx.h"

/* Whether the compiler supports statement-expressions.
   Test program:
     int f (int x) { return ({ x; x; }); }
 */
#if defined __GNUC__ /* both C and C++ mode */ \
    || defined __clang__ /* both C and C++ mode */ \
    || (defined __SUNPRO_C && __SUNPRO_C >= 0x5150) /* C mode */ \
    || (defined __SUNPRO_CC && __SUNPRO_CC >= 0x5150) /* C++ mode */
# define HAVE_STATEMENT_EXPRESSIONS 1
#endif

/* Whether the compiler supports _Generic.
   Test program:
     int f (int x) { return _Generic (x, char *: 2, int: 3); }
 */
#if (defined __GNUC__ && __GNUC__ + (__GNUC_MINOR__ >= 9) > 4 && !defined __cplusplus) /* C mode */ \
    || (defined __clang__ && __clang_major__ >= 3) /* both C and C++ mode */ \
    || (defined __SUNPRO_C && __SUNPRO_C >= 0x5150) /* C mode */ \
    || (__STDC_VERSION__ >= 201112L && !defined __GNUC__) /* C mode */
# define HAVE__GENERIC 1
#endif

/* Whether the compiler supports typeof.
   Test program:
     int f (int x) { typeof (x) y = x; return y; }
 */
#if (defined __GNUC__ && __GNUC__ + (__GNUC_MINOR__ >= 1) > 3) /* both C and C++ mode */ \
    || (defined __clang__ && __clang_major__ >= 3 /* both C and C++ mode */ \
        && !(defined __cplusplus && !defined __GNUC__)) /* except for clang-cl in C++ mode */ \
    || (defined __SUNPRO_C && __SUNPRO_C >= 0x5110) /* C mode */ \
    || __STDC_VERSION__ >= 202311L /* C mode */
# define HAVE_TYPEOF 1
#endif

/* Whether the compiler supports __builtin_choose_expr.
   _Generic and __builtin_choose_expr are like conditional expressions,
   except that the return types of the branches need not match: They avoid an
   "error: type mismatch in conditional expression".
   Test program:
     int f (int x) { return __builtin_choose_expr (sizeof (x) == 4, 2, 3); }
 */
#if (defined __GNUC__ && __GNUC__ + (__GNUC_MINOR__ >= 1) > 3) /* both C and C++ mode */ \
    || (defined __clang__ && __clang_major__ >= 3) /* both C and C++ mode */
# define HAVE_BUILTIN_CHOOSE_EXPR 1
#endif

/* Whether the compiler supports __builtin_constant_p and whether that built-in
   returns true for string literals.
   _Generic has an antiquated treatment of string literals: It treats string
   literals like 'char *', not 'const char *'.  To compensate for this, we need
   __builtin_constant_p.
   Test program:
     int i, v[__builtin_constant_p ("x") > __builtin_constant_p (i) ? 1 : -1];
 */
#if (defined __GNUC__ && __GNUC__ + (__GNUC_MINOR__ >= 1) > 3) /* both C and C++ mode */ \
    || (defined __clang__ && __clang_major__ >= 4) /* both C and C++ mode */
# define HAVE_BUILTIN_CONSTANT_P 1
#endif

/* Whether we support rw_string_desc_t as distinct from string_desc_t.  */
#if HAVE_STATEMENT_EXPRESSIONS && HAVE__GENERIC && HAVE_TYPEOF \
    && HAVE_BUILTIN_CHOOSE_EXPR && HAVE_BUILTIN_CONSTANT_P
# define HAVE_RW_STRING_DESC 1
#endif

_GL_INLINE_HEADER_BEGIN
#ifndef GL_STRING_DESC_INLINE
# define GL_STRING_DESC_INLINE _GL_INLINE
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* Type describing a string that may contain NUL bytes.
   It's merely a descriptor of an array of bytes.
   It comes in two flavours:
     - string_desc_t is read-only,
     - rw_string_desc_t is read-write.
   When we write
     [rw_]string_desc_t
   it means either string_desc_t or rw_string_desc_t.  */
typedef struct rw_string_desc_t rw_string_desc_t;
struct rw_string_desc_t
{
  /* The fields of this struct should be considered private.  */
  idx_t _nbytes;
  char *_data;
};
#if HAVE_RW_STRING_DESC
typedef struct string_desc_t string_desc_t;
struct string_desc_t
{
  /* The fields of this struct should be considered private.  */
  idx_t _nbytes;
  const char *_data;
};
#else
typedef rw_string_desc_t string_desc_t;
#endif

/* String descriptors can be passed and returned by value.

   String descriptors and NUL-terminated 'const char *'/'char *' C strings
   cannot be used interchangeably.  You will get compilation errors if you
   attempt to assign a string descriptor to a C string or vice versa.  */


/* ==== Conversions between string descriptors ==== */

/* Return a read-only view of S.  */
#if 0 /* Defined inline below.  */
extern string_desc_t sd_readonly (rw_string_desc_t s);
#endif

/* Return a read-only view of S.
   Attention: This function is as dangerous as a cast from 'const char *'
   to 'char *'!  */
#if 0 /* Defined inline below.  */
extern rw_string_desc_t sd_readwrite (string_desc_t s);
#endif


/* ==== Side-effect-free operations on string descriptors ==== */

/* Return the length of the string S.  */
#if 0 /* Defined inline below.  */
extern idx_t sd_length ([rw_]string_desc_t s);
#endif

/* Return the byte at index I of string S.
   I must be < length(S).  */
#if 0 /* Defined inline below.  */
extern char sd_char_at ([rw_]string_desc_t s, idx_t i);
#endif

/* Return a read-only view of the bytes of S.  */
#if 0 /* Defined inline below.  */
extern const char * sd_data (string_desc_t s);
extern       char * sd_data (rw_string_desc_t s);
#endif

/* Return true if S is the empty string.  */
#if 0 /* Defined inline below.  */
extern bool sd_is_empty ([rw_]string_desc_t s);
#endif

/* Return true if A and B are equal.  */
#if 0 /* Defined inline below.  */
extern bool sd_equals ([rw_]string_desc_t a, [rw_]string_desc_t b);
#endif

/* Return true if S starts with PREFIX.  */
#if 0 /* Defined inline below.  */
extern bool sd_startswith ([rw_]string_desc_t s, [rw_]string_desc_t prefix);
#endif

/* Return true if S ends with SUFFIX.  */
#if 0 /* Defined inline below.  */
extern bool sd_endswith ([rw_]string_desc_t s, [rw_]string_desc_t suffix);
#endif

/* Return > 0, == 0, or < 0 if A > B, A == B, A < B.
   This uses a lexicographic ordering, where the bytes are compared as
   'unsigned char'.  */
#if 0 /* Defined inline below.  */
extern int sd_cmp ([rw_]string_desc_t a, [rw_]string_desc_t b);
#endif

/* Return > 0, == 0, or < 0 if A > B, A == B, A < B.
   Either A or B must be entirely ASCII.
   This uses a lexicographic ordering, where the bytes are compared as
   'unsigned char', ignoring case, in the "C" locale.  */
#if 0 /* Defined inline below.  */
extern int sd_c_casecmp ([rw_]string_desc_t a, [rw_]string_desc_t b);
#endif

/* Return the index of the first occurrence of C in S,
   or -1 if there is none.  */
#if 0 /* Defined inline below.  */
extern ptrdiff_t sd_index ([rw_]string_desc_t s, char c);
#endif

/* Return the index of the last occurrence of C in S,
   or -1 if there is none.  */
#if 0 /* Defined inline below.  */
extern ptrdiff_t sd_last_index ([rw_]string_desc_t s, char c);
#endif

/* Return the index of the first occurrence of NEEDLE in HAYSTACK,
   or -1 if there is none.  */
#if 0 /* Defined inline below.  */
extern ptrdiff_t sd_contains ([rw_]string_desc_t haystack, [rw_]string_desc_t needle);
#endif

/* Return an empty string.  */
extern string_desc_t sd_new_empty (void);

/* Construct and return a string of length N, at the given memory address.  */
#if 0 /* Defined inline below.  */
extern string_desc_t sd_new_addr (idx_t n, const char *addr)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (2, 1);
extern rw_string_desc_t sd_new_addr (idx_t n, char *addr)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (2, 1);
#endif

/* Return a string that represents the C string S, of length strlen (S).  */
extern string_desc_t sd_from_c (const char *s);

/* Return the substring of S, starting at offset START and ending at offset END.
   START must be <= END.
   The result is of length END - START.
   The result must not be freed (since its storage is part of the storage
   of S).  */
#if 0 /* Defined inline below.  */
extern string_desc_t sd_substring (string_desc_t s, idx_t start, idx_t end);
extern rw_string_desc_t sd_substring (rw_string_desc_t s, idx_t start, idx_t end);
#endif

/* Output S to the file descriptor FD.
   Return 0 if successful.
   Upon error, return -1 with errno set.  */
#if 0 /* Defined inline below.  */
extern int sd_write (int fd, [rw_]string_desc_t s);
#endif

/* Output S to the FILE stream FP.
   Return 0 if successful.
   Upon error, return -1.  */
#if 0 /* Defined inline below.  */
extern int sd_fwrite (FILE *fp, [rw_]string_desc_t s);
#endif


/* ==== Memory-allocating operations on string descriptors ==== */

/* Construct a string of length N, with uninitialized contents.
   Return 0 if successful.
   Upon error, return -1 with errno set.  */
_GL_ATTRIBUTE_NODISCARD
extern int sd_new (rw_string_desc_t *resultp, idx_t n);

/* Construct a string of length N, filled with C.
   Return 0 if successful.
   Upon error, return -1 with errno set.  */
_GL_ATTRIBUTE_NODISCARD
extern int sd_new_filled (rw_string_desc_t *resultp, idx_t n, char c);

/* Construct a copy of string S.
   Return 0 if successful.
   Upon error, return -1 with errno set.  */
#if 0 /* Defined inline below.  */
_GL_ATTRIBUTE_NODISCARD
extern int sd_copy (rw_string_desc_t *resultp, [rw_]string_desc_t s);
#endif

/* Construct the concatenation of N strings.  N must be > 0.
   Return 0 if successful.
   Upon error, return -1 with errno set.  */
_GL_ATTRIBUTE_NODISCARD
extern int sd_concat (rw_string_desc_t *resultp, idx_t n, /* [rw_]string_desc_t string1, */ ...);

/* Construct a copy of string S, as a NUL-terminated C string.
   Return it is successful.
   Upon error, return NULL with errno set.  */
#if 0 /* Defined inline below.  */
extern char * sd_c ([rw_]string_desc_t s) _GL_ATTRIBUTE_DEALLOC_FREE;
#endif


/* ==== Operations with side effects on string descriptors ==== */

/* Overwrite the byte at index I of string S with C.
   I must be < length(S).  */
extern void sd_set_char_at (rw_string_desc_t s, idx_t i, char c);

/* Fill part of S, starting at offset START and ending at offset END,
   with copies of C.
   START must be <= END.  */
extern void sd_fill (rw_string_desc_t s, idx_t start, idx_t end, char c);

/* Overwrite part of S with T, starting at offset START.
   START + length(T) must be <= length (S).  */
#if 0 /* Defined inline below.  */
extern void sd_overwrite (rw_string_desc_t s, idx_t start, [rw_]string_desc_t t);
#endif

/* Free S.  */
extern void sd_free (rw_string_desc_t s);


/* ==== Inline function definitions ==== */

#if HAVE_RW_STRING_DESC
GL_STRING_DESC_INLINE string_desc_t
sd_readonly (rw_string_desc_t s)
{
  string_desc_t result;
  result._nbytes = s._nbytes;
  result._data = s._data;
  return result;
}
#else
# define sd_readonly(s) (s)
#endif

#if HAVE_RW_STRING_DESC
GL_STRING_DESC_INLINE rw_string_desc_t
sd_readwrite (string_desc_t s)
{
  rw_string_desc_t result;
  result._nbytes = s._nbytes;
  result._data = (char *) s._data;
  return result;
}
#else
# define sd_readwrite(s) (s)
#endif

#if HAVE_RW_STRING_DESC
# define sd_length(s) \
   _Generic (s, \
             string_desc_t    : (s)._nbytes, \
             rw_string_desc_t : (s)._nbytes)
#else
GL_STRING_DESC_INLINE idx_t
sd_length (string_desc_t s)
{
  return s._nbytes;
}
#endif

#if HAVE_RW_STRING_DESC
# define sd_char_at(s, i) \
   ({typeof (s) _s_ = (s); \
     _sd_char_at (_s_._nbytes, _s_._data, i); \
   })
GL_STRING_DESC_INLINE char
_sd_char_at (idx_t s_nbytes, const char *s_data, idx_t i)
  _GL_ATTRIBUTE_NONNULL ((2));
GL_STRING_DESC_INLINE char
_sd_char_at (idx_t s_nbytes, const char *s_data, idx_t i)
{
  if (!(i >= 0 && i < s_nbytes))
    /* Invalid argument.  */
    abort ();
  return s_data[i];
}
#else
GL_STRING_DESC_INLINE char
sd_char_at (string_desc_t s, idx_t i)
{
  if (!(i >= 0 && i < s._nbytes))
    /* Invalid argument.  */
    abort ();
  return s._data[i];
}
#endif

#if HAVE_RW_STRING_DESC
# define sd_data(s) \
   _Generic (s, \
             string_desc_t    : (s)._data, \
             rw_string_desc_t : (s)._data)
#else
GL_STRING_DESC_INLINE const char *
sd_data (string_desc_t s)
{
  return s._data;
}
#endif

#if HAVE_RW_STRING_DESC
# define sd_is_empty(s) \
   _Generic (s, \
             string_desc_t    : (s)._nbytes == 0, \
             rw_string_desc_t : (s)._nbytes == 0)
#else
GL_STRING_DESC_INLINE bool
sd_is_empty (string_desc_t s)
{
  return s._nbytes == 0;
}
#endif

extern bool _sd_equals (idx_t a_nbytes, const char *a_data,
                        idx_t b_nbytes, const char *b_data)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (2, 1)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (4, 3);
#if HAVE_RW_STRING_DESC
# define sd_equals(a, b) \
   ({typeof (a) _a_ = (a); \
     typeof (b) _b_ = (b); \
     _sd_equals (_a_._nbytes, _a_._data, _b_._nbytes, _b_._data); \
   })
#else
GL_STRING_DESC_INLINE bool
sd_equals (string_desc_t a, string_desc_t b)
{
  return _sd_equals (a._nbytes, a._data, b._nbytes, b._data);
}
#endif

extern bool _sd_startswith (idx_t s_nbytes, const char *s_data,
                            idx_t prefix_nbytes, const char *prefix_data)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (2, 1)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (4, 3);
#if HAVE_RW_STRING_DESC
# define sd_startswith(s, prefix) \
   ({typeof (s) _s_ = (s); \
     typeof (prefix) _prefix_ = (prefix); \
     _sd_startswith (_s_._nbytes, _s_._data, _prefix_._nbytes, _prefix_._data); \
   })
#else
GL_STRING_DESC_INLINE bool
sd_startswith (string_desc_t s, string_desc_t prefix)
{
  return _sd_startswith (s._nbytes, s._data, prefix._nbytes, prefix._data);
}
#endif

extern bool _sd_endswith (idx_t s_nbytes, const char *s_data,
                          idx_t suffix_nbytes, const char *suffix_data)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (2, 1)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (4, 3);
#if HAVE_RW_STRING_DESC
# define sd_endswith(s, suffix) \
   ({typeof (s) _s_ = (s); \
     typeof (suffix) _suffix_ = (suffix); \
     _sd_endswith (_s_._nbytes, _s_._data, _suffix_._nbytes, _suffix_._data); \
   })
#else
GL_STRING_DESC_INLINE bool
sd_endswith (string_desc_t s, string_desc_t suffix)
{
  return _sd_endswith (s._nbytes, s._data, suffix._nbytes, suffix._data);
}
#endif

extern int _sd_cmp (idx_t a_nbytes, const char *a_data,
                    idx_t b_nbytes, const char *b_data)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (2, 1)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (4, 3);
#if HAVE_RW_STRING_DESC
# define sd_cmp(a, b) \
   ({typeof (a) _a_ = (a); \
     typeof (b) _b_ = (b); \
     _sd_cmp (_a_._nbytes, _a_._data, _b_._nbytes, _b_._data); \
   })
#else
GL_STRING_DESC_INLINE int
sd_cmp (string_desc_t a, string_desc_t b)
{
  return _sd_cmp (a._nbytes, a._data, b._nbytes, b._data);
}
#endif

extern int _sd_c_casecmp (idx_t a_nbytes, const char *a_data,
                          idx_t b_nbytes, const char *b_data)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (2, 1)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (4, 3);
#if HAVE_RW_STRING_DESC
# define sd_c_casecmp(a, b) \
   ({typeof (a) _a_ = (a); \
     typeof (b) _b_ = (b); \
     _sd_c_casecmp (_a_._nbytes, _a_._data, _b_._nbytes, _b_._data); \
   })
#else
GL_STRING_DESC_INLINE int
sd_c_casecmp (string_desc_t a, string_desc_t b)
{
  return _sd_c_casecmp (a._nbytes, a._data, b._nbytes, b._data);
}
#endif

extern ptrdiff_t _sd_index (idx_t s_nbytes, const char *s_data, char c)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (2, 1);
#if HAVE_RW_STRING_DESC
# define sd_index(s, c) \
   ({typeof (s) _s_ = (s); \
     _sd_index (_s_._nbytes, _s_._data, c); \
   })
#else
GL_STRING_DESC_INLINE ptrdiff_t
sd_index (string_desc_t s, char c)
{
  return _sd_index (s._nbytes, s._data, c);
}
#endif

extern ptrdiff_t _sd_last_index (idx_t s_nbytes, const char *s_data, char c)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (2, 1);
#if HAVE_RW_STRING_DESC
# define sd_last_index(s, c) \
   ({typeof (s) _s_ = (s); \
     _sd_last_index (_s_._nbytes, _s_._data, c); \
   })
#else
GL_STRING_DESC_INLINE ptrdiff_t
sd_last_index (string_desc_t s, char c)
{
  return _sd_last_index (s._nbytes, s._data, c);
}
#endif

extern ptrdiff_t _sd_contains (idx_t haystack_nbytes, const char *haystack_data,
                               idx_t needle_nbytes, const char *needle_data)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (2, 1)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (4, 3);
#if HAVE_RW_STRING_DESC
# define sd_contains(haystack, needle) \
   ({typeof (haystack) _haystack_ = (haystack); \
     typeof (needle) _needle_ = (needle); \
     _sd_contains (_haystack_._nbytes, _haystack_._data, \
                   _needle_._nbytes, _needle_._data); \
   })
#else
GL_STRING_DESC_INLINE ptrdiff_t
sd_contains (string_desc_t haystack, string_desc_t needle)
{
  return _sd_contains (haystack._nbytes, haystack._data,
                       needle._nbytes, needle._data);
}
#endif

extern string_desc_t _sd_new_addr (idx_t n, const char *addr)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (2, 1);
extern rw_string_desc_t _rwsd_new_addr (idx_t n, /*!*/const/*!*/ char *addr)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (2, 1);
#if HAVE_RW_STRING_DESC
# define sd_new_addr(n, addr) \
   _Generic (addr, \
             const char * : _sd_new_addr (n, addr), \
             char * : /* Treat string literals like 'const char *'.  */ \
                      __builtin_choose_expr (__builtin_constant_p (addr), \
                                             _sd_new_addr (n, addr), \
                                             _rwsd_new_addr (n, addr)))
#else
# define sd_new_addr(n, addr) \
   _rwsd_new_addr (n, addr)
#endif

#if HAVE_RW_STRING_DESC
# define sd_substring(s, start, end) \
   ({typeof (s) _s_ = (s); \
     idx_t _start_ = (start); \
     idx_t _end_ = (end); \
     if (!(_start_ >= 0 && _start_ <= _end_ && _end_ <= _s_._nbytes)) \
       /* Invalid arguments.  */ \
       abort (); \
     typeof (s) _result_; \
     _result_._nbytes = _end_ - _start_; \
     _result_._data = _s_._data + _start_; \
     _result_; \
   })
#else
GL_STRING_DESC_INLINE string_desc_t
sd_substring (string_desc_t s, idx_t start, idx_t end)
{
  if (!(start >= 0 && start <= end && end <= s._nbytes))
    /* Invalid arguments.  */
    abort ();
  string_desc_t result;
  result._nbytes = end - start;
  result._data = s._data + start;
  return result;
}
#endif

extern int _sd_write (int fd, idx_t s_nbytes, const char *s_data)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (3, 2);
#if HAVE_RW_STRING_DESC
# define sd_write(fd, s) \
   ({int _fd_ = (fd); \
     typeof (s) _s_ = (s); \
     _sd_write (_fd_, _s_._nbytes, _s_._data); \
   })
#else
GL_STRING_DESC_INLINE int
sd_write (int fd, string_desc_t s)
{
  return _sd_write (fd, s._nbytes, s._data);
}
#endif

extern int _sd_fwrite (FILE *fp, idx_t s_nbytes, const char *s_data)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (3, 2);
#if HAVE_RW_STRING_DESC
# define sd_fwrite(fp, s) \
   ({FILE *_fp_ = (fp); \
     typeof (s) _s_ = (s); \
     _sd_fwrite (_fp_, _s_._nbytes, _s_._data); \
   })
#else
GL_STRING_DESC_INLINE int
sd_fwrite (FILE *fp, string_desc_t s)
{
  return _sd_fwrite (fp, s._nbytes, s._data);
}
#endif

extern int _sd_copy (rw_string_desc_t *resultp,
                     idx_t s_nbytes, const char *s_data)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (3, 2);
#if HAVE_RW_STRING_DESC
# define sd_copy(resultp, s) \
   ({rw_string_desc_t *_resultp_ = (resultp); \
     typeof (s) _s_ = (s); \
     _sd_copy (_resultp_, _s_._nbytes, _s_._data); \
   })
#else
_GL_ATTRIBUTE_NODISCARD GL_STRING_DESC_INLINE int
sd_copy (rw_string_desc_t *resultp, string_desc_t s)
{
  return _sd_copy (resultp, s._nbytes, s._data);
}
#endif

extern char *_sd_c (idx_t s_nbytes, const char *s_data)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (2, 1);
#if HAVE_RW_STRING_DESC
# define sd_c(s) \
   ({typeof (s) _s_ = (s); \
     _sd_c (_s_._nbytes, _s_._data); \
   })
#else
GL_STRING_DESC_INLINE char *
sd_c (string_desc_t s)
{
  return _sd_c (s._nbytes, s._data);
}
#endif

extern void _sd_overwrite (rw_string_desc_t s, idx_t start,
                           idx_t t_nbytes, const char *t_data)
  _GL_ATTRIBUTE_NONNULL_IF_NONZERO (4, 3);
#if HAVE_RW_STRING_DESC
# define sd_overwrite(s, start, t) \
   ({rw_string_desc_t _s_ = (s); \
     idx_t _start_ = (start); \
     typeof (t) _t_ = (t); \
     _sd_overwrite (_s_, _start_, _t_._nbytes, _t_._data); \
   })
#else
GL_STRING_DESC_INLINE void
sd_overwrite (rw_string_desc_t s, idx_t start, string_desc_t t)
{
  _sd_overwrite (s, start, t._nbytes, t._data);
}
#endif


#ifdef __cplusplus
}
#endif

_GL_INLINE_HEADER_END


#endif /* _STRING_DESC_H */
