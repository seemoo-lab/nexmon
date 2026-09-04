/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2012 Thomas Graf <tgraf@suug.ch>
 */

#ifndef __NETLINK_BASE_NL_BASE_UTILS_H__
#define __NETLINK_BASE_NL_BASE_UTILS_H__

#include <byteswap.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <inttypes.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef DISABLE_PTHREADS
#include <pthread.h>
#endif

/*****************************************************************************/

#if __BYTE_ORDER == __BIG_ENDIAN
#define ntohll(x) (x)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define ntohll(x) bswap_64((x))
#endif
#define htonll(x) ntohll(x)

/*****************************************************************************/

#define _NL_STRINGIFY_ARG(contents) #contents
#define _NL_STRINGIFY(macro_or_string) _NL_STRINGIFY_ARG(macro_or_string)

/*****************************************************************************/

#if defined(__GNUC__)
#define _NL_PRAGMA_WARNING_DO(warning) \
	_NL_STRINGIFY(GCC diagnostic ignored warning)
#elif defined(__clang__)
#define _NL_PRAGMA_WARNING_DO(warning) \
	_NL_STRINGIFY(clang diagnostic ignored warning)
#endif

/* you can only suppress a specific warning that the compiler
 * understands. Otherwise you will get another compiler warning
 * about invalid pragma option.
 * It's not that bad however, because gcc and clang often have the
 * same name for the same warning. */

#if defined(__GNUC__) && \
	(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#define _NL_PRAGMA_WARNING_DISABLE(warning)                 \
	_Pragma("GCC diagnostic push")                      \
		_Pragma(_NL_PRAGMA_WARNING_DO("-Wpragmas")) \
			_Pragma(_NL_PRAGMA_WARNING_DO(warning))
#elif defined(__clang__)
#define _NL_PRAGMA_WARNING_DISABLE(warning)                                \
	_Pragma("clang diagnostic push")                                   \
		_Pragma(_NL_PRAGMA_WARNING_DO("-Wunknown-warning-option")) \
			_Pragma(_NL_PRAGMA_WARNING_DO(warning))
#else
#define _NL_PRAGMA_WARNING_DISABLE(warning)
#endif

#if defined(__GNUC__) && \
	(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#define _NL_PRAGMA_WARNING_REENABLE _Pragma("GCC diagnostic pop")
#elif defined(__clang__)
#define _NL_PRAGMA_WARNING_REENABLE _Pragma("clang diagnostic pop")
#else
#define _NL_PRAGMA_WARNING_REENABLE
#endif

/*****************************************************************************/

#define _nl_packed __attribute__((__packed__))
#define _nl_unused __attribute__((__unused__))
#define _nl_always_inline __attribute__((__always_inline__))
#define _nl_used __attribute__((__used__))
#define _nl_pure __attribute__((__pure__))
#define _nl_const __attribute__((__const__))
#define _nl_noreturn __attribute__((__noreturn__))
#define _nl_warn_unused_result __attribute__((__warn_unused_result__))
#define _nl_printf(a, b) __attribute__((__format__(__printf__, a, b)))
#define _nl_align(s) __attribute__((__aligned__(s)))
#define _nl_section(s) __attribute__((__section__(s)))
#define _nl_alignof(type) __alignof(type)
#define _nl_alignas(type) _nl_align(_nl_alignof(type))
#define _nl_deprecated(msg) __attribute__((__deprecated__(msg)))
#define _nl_init __attribute__((constructor))
#define _nl_exit __attribute__((destructor))
#define _nl_auto(fcn) __attribute__((__cleanup__(fcn)))

/*****************************************************************************/

#ifdef thread_local
#define _nl_thread_local thread_local
/*
 * Don't break on glibc < 2.16 that doesn't define __STDC_NO_THREADS__
 * see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=53769
 */
#elif __STDC_VERSION__ >= 201112L &&                     \
	!(defined(__STDC_NO_THREADS__) ||                \
	  (defined(__GNU_LIBRARY__) && __GLIBC__ == 2 && \
	   __GLIBC_MINOR__ < 16))
#define _nl_thread_local _Thread_local
#else
#define _nl_thread_local __thread
#endif

/*****************************************************************************/

#define _NL_STATIC_ASSERT(cond) ((void)sizeof(char[(cond) ? 1 : -1]))

/*****************************************************************************/

#if defined(NL_MORE_ASSERTS) && NL_MORE_ASSERTS > 0
#define _nl_assert(cond) assert(cond)
#else
#define _nl_assert(cond)              \
	do {                          \
		if (0) {              \
			assert(cond); \
		}                     \
	} while (0)
#endif

#define _nl_assert_not_reached() assert(0)

/*****************************************************************************/

#define _NL_BIT(n) (1ull << (n))

/*****************************************************************************/

#define _NL_PASTE_ARGS(identifier1, identifier2) identifier1##identifier2
#define _NL_PASTE(identifier1, identifier2) \
	_NL_PASTE_ARGS(identifier1, identifier2)

/* Taken from systemd's UNIQ_T and UNIQ macros. */

#define _NL_UNIQ_T(x, uniq) _NL_PASTE(__unique_prefix_, _NL_PASTE(x, uniq))
#define _NL_UNIQ __COUNTER__

/*****************************************************************************/

#define _nl_assert_addr_family_or_unspec(addr_family)             \
	do {                                                      \
		typeof(addr_family) _addr_family = (addr_family); \
                                                                  \
		_nl_assert(_addr_family == AF_UNSPEC ||           \
			   _addr_family == AF_INET ||             \
			   _addr_family == AF_INET6);             \
	} while (0)

#define _nl_assert_addr_family(addr_family)                       \
	do {                                                      \
		typeof(addr_family) _addr_family = (addr_family); \
                                                                  \
		_nl_assert(_addr_family == AF_INET ||             \
			   _addr_family == AF_INET6);             \
	} while (0)

/*****************************************************************************/

#define _NL_SWAP(pa, pb)                   \
	do {                               \
		typeof(*(pa)) *_pa = (pa); \
		typeof(*(pb)) *_pb = (pb); \
		typeof(*_pa) _tmp;         \
                                           \
		_nl_assert(_pa);           \
		_nl_assert(_pb);           \
		_tmp = *_pa;               \
		*_pa = *_pb;               \
		*_pb = _tmp;               \
	} while (0)

/*****************************************************************************/

#define _NL_N_ELEMENTS(arr) (sizeof(arr) / sizeof((arr)[0]))

#define ARRAY_SIZE(arr) _NL_N_ELEMENTS(arr)

/*****************************************************************************/

/* This is also defined in stddef.h */
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t)&((TYPE *)0)->MEMBER)
#endif

/*****************************************************************************/

static inline uintptr_t _nl_ptr_to_uintptr(const void *p)
{
	/* in C, pointers can only be compared (with less-than or greater-than) under certain
	 * circumstances. Since uintptr_t is supposed to be able to represent the pointer
	 * as a plain integer and also support to convert the integer back to the pointer,
	 * it should be safer to compare the pointers directly.
	 *
	 * Of course, this function isn't very useful beyond that its use makes it clear
	 * that we want to compare pointers by value, which otherwise may not be valid. */
	return (uintptr_t)p;
}

/*****************************************************************************/

static inline int _nl_strcmp0(const char *s1, const char *s2)
{
	int c;

	/* like g_strcmp0(), but this is inlinable.
	 *
	 * Also, it is guaranteed to return either -1, 0, or 1. */
	if (s1 == s2)
		return 0;
	if (!s1)
		return -1;
	if (!s2)
		return 1;
	c = strcmp(s1, s2);
	if (c < 0)
		return -1;
	if (c > 0)
		return 1;
	return 0;
}

static inline bool _nl_streq(const char *a, const char *b)
{
	return !strcmp(a, b);
}

static inline bool _nl_streq0(const char *a, const char *b)
{
	return a == b || (a && b && _nl_streq(a, b));
}

static inline int _nl_memcmp(const void *s1, const void *s2, size_t n)
{
	/* Workaround undefined behavior in memcmp() with NULL pointers. */
	if (n == 0)
		return 0;
	_nl_assert(s1);
	_nl_assert(s2);
	return memcmp(s1, s2, n);
}

static inline bool _nl_memeq(const void *s1, const void *s2, size_t len)
{
	return _nl_memcmp(s1, s2, len) == 0;
}

static inline void *_nl_memcpy(void *restrict dest, const void *restrict src,
			       size_t n)
{
	/* Workaround undefined behavior in memcpy() with NULL pointers. */
	if (n == 0)
		return dest;

	_nl_assert(src);
	return memcpy(dest, src, n);
}

/*****************************************************************************/

#define _NL_INT_IS_SIGNED(arg) (!(((typeof(arg))-1) > 0))

#define _NL_INT_SAME_SIGNEDNESS(arg1, arg2) \
	(_NL_INT_IS_SIGNED(arg1) == _NL_INT_IS_SIGNED(arg2))

/*****************************************************************************/

/* glib's MIN()/MAX() macros don't have function-like behavior, in that they evaluate
 * the argument possibly twice.
 *
 * Taken from systemd's MIN()/MAX() macros. */

#define _NL_MIN(a, b) __NL_MIN(_NL_UNIQ, a, _NL_UNIQ, b)
#define __NL_MIN(aq, a, bq, b)                                                 \
	({                                                                     \
		typeof(a) _NL_UNIQ_T(A, aq) = (a);                             \
		typeof(b) _NL_UNIQ_T(B, bq) = (b);                             \
                                                                               \
		_NL_STATIC_ASSERT(_NL_INT_SAME_SIGNEDNESS(_NL_UNIQ_T(A, aq),   \
							  _NL_UNIQ_T(B, bq))); \
                                                                               \
		((_NL_UNIQ_T(A, aq) < _NL_UNIQ_T(B, bq)) ? _NL_UNIQ_T(A, aq) : \
							   _NL_UNIQ_T(B, bq)); \
	})

#define _NL_MAX(a, b) __NL_MAX(_NL_UNIQ, a, _NL_UNIQ, b)
#define __NL_MAX(aq, a, bq, b)                                                 \
	({                                                                     \
		typeof(a) _NL_UNIQ_T(A, aq) = (a);                             \
		typeof(b) _NL_UNIQ_T(B, bq) = (b);                             \
                                                                               \
		_NL_STATIC_ASSERT(_NL_INT_SAME_SIGNEDNESS(_NL_UNIQ_T(A, aq),   \
							  _NL_UNIQ_T(B, bq))); \
                                                                               \
		((_NL_UNIQ_T(A, aq) > _NL_UNIQ_T(B, bq)) ? _NL_UNIQ_T(A, aq) : \
							   _NL_UNIQ_T(B, bq)); \
	})

#define _NL_CLAMP(x, low, high) \
	__NL_CLAMP(_NL_UNIQ, x, _NL_UNIQ, low, _NL_UNIQ, high)
#define __NL_CLAMP(xq, x, lowq, low, highq, high)                     \
	({                                                            \
		typeof(x) _NL_UNIQ_T(X, xq) = (x);                    \
		typeof(low) _NL_UNIQ_T(LOW, lowq) = (low);            \
		typeof(high) _NL_UNIQ_T(HIGH, highq) = (high);        \
                                                                      \
		_NL_STATIC_ASSERT(_NL_INT_SAME_SIGNEDNESS(            \
			_NL_UNIQ_T(X, xq), _NL_UNIQ_T(LOW, lowq)));   \
		_NL_STATIC_ASSERT(_NL_INT_SAME_SIGNEDNESS(            \
			_NL_UNIQ_T(X, xq), _NL_UNIQ_T(HIGH, highq))); \
                                                                      \
		((_NL_UNIQ_T(X, xq) > _NL_UNIQ_T(HIGH, highq)) ?      \
			 _NL_UNIQ_T(HIGH, highq) :                    \
		 (_NL_UNIQ_T(X, xq) < _NL_UNIQ_T(LOW, lowq)) ?        \
			 _NL_UNIQ_T(LOW, lowq) :                      \
			 _NL_UNIQ_T(X, xq));                          \
	})

#define _NL_MAX_WITH_CMP(cmp, a, b)               \
	({                                        \
		typeof(a) _a = (a);               \
		typeof(b) _b = (b);               \
                                                  \
		(((cmp(_a, _b)) >= 0) ? _a : _b); \
	})

/* evaluates to (void) if _A or _B are not constant or of different types */
#define _NL_CONST_MAX(_A, _B)                                            \
	(__builtin_choose_expr(                                          \
		(__builtin_constant_p(_A) && __builtin_constant_p(_B) && \
		 __builtin_types_compatible_p(typeof(_A), typeof(_B))),  \
		((_A) > (_B)) ? (_A) : (_B), ((void)0)))

/*****************************************************************************/

#define _NL_CMP_RETURN(c)                        \
	do {                                     \
		const int _cc = (c);             \
                                                 \
		if (_cc)                         \
			return _cc < 0 ? -1 : 1; \
	} while (0)

#define _NL_CMP_RETURN_DIRECT(c)                                  \
	/* Usually we want that our CMP functions return strictly
	 * -1, 0, or 1. _NL_CMP_RETURN_DIRECT() is like _NL_CMP_RETURN(),
	 *  except, it does not clamp the integer value. */ \
	do {                                                      \
		const int _cc = (c);                              \
                                                                  \
		if (_cc)                                          \
			return _cc;                               \
	} while (0)

#define _NL_CMP_SELF(a, b)          \
	do {                        \
		typeof(a) _a = (a); \
		typeof(b) _b = (b); \
                                    \
		if (_a == _b)       \
			return 0;   \
		if (!_a)            \
			return -1;  \
		if (!_b)            \
			return 1;   \
	} while (0)

/*****************************************************************************/

#define _NL_CMP_DIRECT(a, b)                                        \
	do {                                                        \
		typeof(a) _a = (a);                                 \
		typeof(b) _b = (b);                                 \
                                                                    \
		_NL_STATIC_ASSERT(_NL_INT_SAME_SIGNEDNESS(_a, _b)); \
                                                                    \
		if (_a != _b)                                       \
			return (_a < _b) ? -1 : 1;                  \
	} while (0)

#define _NL_CMP_DIRECT_UNSAFE(a, b)                                                     \
	/* This variant is "unsafe", because it evaluates the arguments more then once.
	 * This is only useful for bitfields, for which typeof() doesn't work.
	 * Don't use otherwise. */ \
	do {                                                                            \
		if ((a) != (b))                                                         \
			return ((a) < (b)) ? -1 : 1;                                    \
	} while (0)

/* In the general case, direct pointer comparison is undefined behavior in C.
 * Avoid that by casting pointers to void* and then to uintptr_t. This comparison
 * is not really meaningful, except that it provides some kind of stable sort order
 * between pointers (that can otherwise not be compared). */
#define _NL_CMP_DIRECT_PTR(a, b) \
	_NL_CMP_DIRECT(_nl_ptr_to_uintptr(a), _nl_ptr_to_uintptr(b))

#define _NL_CMP_DIRECT_BOOL(a, b) _NL_CMP_DIRECT(!!(a), !!(b))

#define _NL_CMP_DIRECT_MEMCMP(a, b, size) \
	_NL_CMP_RETURN(_nl_memcmp((a), (b), (size)))

#define _NL_CMP_DIRECT_STRCMP(a, b) _NL_CMP_RETURN_DIRECT(strcmp((a), (b)))

#define _NL_CMP_DIRECT_STRCMP0(a, b) \
	_NL_CMP_RETURN_DIRECT(_nl_strcmp0((a), (b)))

#define _NL_CMP_DIRECT_STR_INTERNED(a, b)                                                    \
	/* This is interned strings, which are first checked for equality only using pointer
     * comparison. Only in case of differences, the sort order is still determined by strcmp(). */ \
	do {                                                                                 \
		const char *const _a = (a);                                                  \
		const char *const _b = (b);                                                  \
                                                                                             \
		if (_a != _b)                                                                \
			_NL_CMP_RETURN_DIRECT(_nl_strcmp0(_a, _b));                          \
	} while (0)

#define _NL_CMP_DIRECT_IN6ADDR(a, b)                                     \
	do {                                                             \
		const struct in6_addr *const _a = (a);                   \
		const struct in6_addr *const _b = (b);                   \
                                                                         \
		_NL_CMP_RETURN(memcmp(_a, _b, sizeof(struct in6_addr))); \
	} while (0)

/*****************************************************************************/

#define _NL_CMP_FIELD(a, b, field) _NL_CMP_DIRECT(((a)->field), ((b)->field))

#define _NL_CMP_FIELD_UNSAFE(a, b, field)                                               \
	/* This variant is "unsafe", because it evaluates the arguments more then once.
	 * This is only useful for bitfields, for which typeof() doesn't work.
	 * Don't use otherwise. */ \
	_NL_CMP_DIRECT_UNSAFE(((a)->field), ((b)->field))

#define _NL_CMP_FIELD_BOOL(a, b, field) \
	_NL_CMP_DIRECT_BOOL(((a)->field), ((b)->field))

#define _NL_CMP_FIELD_STR(a, b, field) \
	_NL_CMP_DIRECT_STRCMP(((a)->field), ((b)->field))

#define _NL_CMP_FIELD_STR0(a, b, field) \
	_NL_CMP_DIRECT_STRCMP0(((a)->field), ((b)->field))

#define _NL_CMP_FIELD_STR_INTERNED(a, b, field) \
	_NL_CMP_DIRECT_STR_INTERNED(((a)->field), ((b)->field))

#define _NL_CMP_FIELD_MEMCMP_LEN(a, b, field, len)          \
	_NL_CMP_DIRECT_MEMCMP(&((a)->field), &((b)->field), \
			      _NL_MIN(len, sizeof((a)->field)))

#define _NL_CMP_FIELD_MEMCMP(a, b, field) \
	_NL_CMP_DIRECT_MEMCMP(&((a)->field), &((b)->field), sizeof((a)->field))

#define _NL_CMP_FIELD_IN6ADDR(a, b, field) \
	_NL_CMP_DIRECT_IN6ADDR(&((a)->field), &((b)->field))

/*****************************************************************************/

/* internal macro to calculate the size of a struct @type up to (and including) @field.
 * this will be used for .minlen policy fields, so that we require only a field of up
 * to the given size. */
#define _nl_offsetofend(type, field) \
	(offsetof(type, field) + sizeof(((type *)NULL)->field))

/*****************************************************************************/

#define _nl_clear_pointer(pp, destroy)                                         \
	({                                                                     \
		__typeof__(*(pp)) *_pp = (pp);                                 \
		__typeof__(*_pp) _p;                                           \
		int _changed = 0;                                              \
                                                                               \
		if (_pp && (_p = *_pp)) {                                      \
			_nl_unused const void *const _p_check_is_pointer = _p; \
                                                                               \
			*_pp = NULL;                                           \
                                                                               \
			(destroy)(_p);                                         \
                                                                               \
			_changed = 1;                                          \
		}                                                              \
		_changed;                                                      \
	})

#define _nl_clear_free(pp) _nl_clear_pointer(pp, free)

#define _nl_steal_pointer(pp)                        \
	({                                           \
		__typeof__(*(pp)) *const _pp = (pp); \
		__typeof__(*_pp) _p = NULL;          \
                                                     \
		if (_pp && (_p = *_pp)) {            \
			*_pp = NULL;                 \
		}                                    \
                                                     \
		_p;                                  \
	})

/*****************************************************************************/

#define _nl_malloc_maybe_a(alloca_maxlen, bytes, to_free)  \
	({                                                 \
		const size_t _bytes = (bytes);             \
		__typeof__(to_free) _to_free = (to_free);  \
		__typeof__(*_to_free) _ptr;                \
                                                           \
		_NL_STATIC_ASSERT((alloca_maxlen) <= 500); \
		_nl_assert(_to_free && !*_to_free);        \
                                                           \
		if (_bytes <= (alloca_maxlen)) {           \
			_ptr = alloca(_bytes);             \
		} else {                                   \
			_ptr = malloc(_bytes);             \
			*_to_free = _ptr;                  \
		};                                         \
                                                           \
		_ptr;                                      \
	})

/*****************************************************************************/

static inline char *_nl_strncpy_trunc(char *dst, const char *src, size_t len)
{
	/* we don't use/reimplement strlcpy(), because we want the fill-all-with-NUL
	 * behavior of strncpy(). This is just strncpy() with gracefully handling truncation
	 * (and disabling the "-Wstringop-truncation" warning).
	 *
	 * Note that truncation is silently accepted.
	 */

	_NL_PRAGMA_WARNING_DISABLE("-Wstringop-truncation");
	_NL_PRAGMA_WARNING_DISABLE("-Wstringop-overflow");

	if (len > 0) {
		_nl_assert(dst);
		_nl_assert(src);

		strncpy(dst, src, len);

		dst[len - 1] = '\0';
	}

	_NL_PRAGMA_WARNING_REENABLE;
	_NL_PRAGMA_WARNING_REENABLE;

	return dst;
}

static inline char *_nl_strncpy_assert(char *dst, const char *src, size_t len)
{
	/* we don't use/reimplement strlcpy(), because we want the fill-all-with-NUL
	 * behavior of strncpy(). This is just strncpy() with assertion against truncation
	 * (and disabling the "-Wstringop-truncation" warning).
	 *
	 * Note that truncation is still a bug and there is an _nl_assert()
	 * against that.
	 */

	_NL_PRAGMA_WARNING_DISABLE("-Wstringop-truncation");
	_NL_PRAGMA_WARNING_DISABLE("-Wstringop-overflow");

	if (len > 0) {
		_nl_assert(dst);
		_nl_assert(src);

		strncpy(dst, src, len);

		_nl_assert(dst[len - 1] == '\0');

		dst[len - 1] = '\0';
	}

	_NL_PRAGMA_WARNING_REENABLE;
	_NL_PRAGMA_WARNING_REENABLE;

	return dst;
}

#define _NL_RETURN_ON_ERR(cmd)       \
	do {                         \
		int _err;            \
                                     \
		_err = (cmd);        \
		if (_err < 0)        \
			return _err; \
	} while (0)

#define _NL_RETURN_E_ON_ERR(e, cmd)                 \
	do {                                        \
		int _err;                           \
                                                    \
		_err = (cmd);                       \
		if (_err < 0) {                     \
			_NL_STATIC_ASSERT((e) > 0); \
			return -(e);                \
		}                                   \
	} while (0)

/* _NL_RETURN_ON_PUT_ERR() shall only be used with a put command (nla_put or nlmsg_append).
 * These commands can either fail with a regular error code (which gets propagated)
 * or with -NLE_NOMEM. However, they don't really try to allocate memory, so we don't
 * want to propagate -NLE_NOMEM. Instead, we coerce such failure to -NLE_MSGSIZE. */
#define _NL_RETURN_ON_PUT_ERR(put_cmd)                                                                  \
	do {                                                                                            \
		int _err;                                                                               \
                                                                                                        \
		_err = (put_cmd);                                                                       \
		if (_err < 0) {                                                                         \
			if (_err == -NLE_NOMEM) {                                                       \
				/* nla_put() returns -NLE_NOMEM in case of out of buffer size. We don't
				 * want to propagate that error and map it to -NLE_MSGSIZE. */ \
				return -NLE_MSGSIZE;                                                    \
			}                                                                               \
			/* any other error can only be due to invalid parameters. Propagate the
			 * error, however also assert that it cannot be reached. */         \
			_nl_assert_not_reached();                                                       \
			return _err;                                                                    \
		} else                                                                                  \
			_nl_assert(_err == 0);                                                          \
	} while (0)

static inline int _nl_close(int fd)
{
	int r;

	r = close(fd);
	_nl_assert(r == 0 || fd < 0 || errno != EBADF);
	return r;
}

static inline void *_nl_memdup(const void *ptr, size_t len)
{
	void *p;

	if (len == 0) {
		/* malloc() leaves it implementation defined whether to return NULL.
		 * Callers rely on returning NULL if len is zero. */
		return NULL;
	}

	p = malloc(len);
	if (!p)
		return NULL;
	memcpy(p, ptr, len);
	return p;
}

#define _nl_memdup_ptr(ptr) ((__typeof__(ptr))_nl_memdup((ptr), sizeof(*(ptr))))

/*****************************************************************************/

static inline size_t _nl_addr_family_to_size(int addr_family)
{
	if (addr_family == AF_INET)
		return sizeof(in_addr_t);
	if (addr_family == AF_INET6)
		return sizeof(struct in6_addr);
	return 0;
}

/*****************************************************************************/

typedef union {
	in_addr_t addr4;
	struct in_addr a4;
	struct in6_addr a6;
} _NLIPAddr;

static inline char *_nl_inet_ntop(int addr_family, const void *addr,
				  char buf[static INET_ADDRSTRLEN])
{
	char *r;

	_nl_assert_addr_family(addr_family);
	_nl_assert(addr);

	/* inet_ntop() is documented to fail, but if we pass a known address family
	 * and a suitably large buffer, it cannot. Assert for that. */

	r = (char *)inet_ntop(addr_family, addr, buf,
			      (addr_family == AF_INET) ? INET_ADDRSTRLEN :
							 INET6_ADDRSTRLEN);
	_nl_assert(r == buf);
	_nl_assert(strlen(r) < ((addr_family == AF_INET) ? INET_ADDRSTRLEN :
							   INET6_ADDRSTRLEN));

	return r;
}

static inline char *_nl_inet_ntop_dup(int addr_family, const void *addr)
{
	return (char *)_nl_inet_ntop(addr_family, addr,
				     malloc((addr_family == AF_INET) ?
						    INET_ADDRSTRLEN :
						    INET6_ADDRSTRLEN));
}

static inline char *_nl_inet_ntop4(in_addr_t addr,
				   char buf[static INET_ADDRSTRLEN])
{
	return _nl_inet_ntop(AF_INET, &addr, buf);
}

static inline char *_nl_inet_ntop6(const struct in6_addr *addr,
				   char buf[static INET6_ADDRSTRLEN])
{
	return _nl_inet_ntop(AF_INET6, &addr, buf);
}

/*****************************************************************************/

static inline size_t _nl_ptrarray_len_(const void *const *ptr, ssize_t len)
{
	size_t l = 0;

	if (len >= 0)
		return len;
	if (ptr) {
		while (ptr[l])
			l++;
	}
	return l;
}

#define _nl_ptrarray_len(ptr, len)                                   \
	({                                                           \
		typeof(*(ptr)) *const _ptr = (ptr);                  \
                                                                     \
		_nl_ptrarray_len_((const void *const *)_ptr, (len)); \
	})

/*****************************************************************************/

#define _NL_AUTO_DEFINE_FCN_VOID0(CastType, name, func) \
	static inline void name(void *v)                \
	{                                               \
		if (*((CastType *)v))                   \
			func(*((CastType *)v));         \
	}                                               \
	struct _nl_dummy_for_tailing_semicolon

#define _NL_AUTO_DEFINE_FCN_STRUCT(CastType, name, func) \
	static inline void name(CastType *v)             \
	{                                                \
		if (v)                                   \
			func(v);                         \
	}                                                \
	struct _nl_dummy_for_tailing_semicolon

#define _NL_AUTO_DEFINE_FCN_TYPED0(CastType, name, func) \
	static inline void name(CastType *v)             \
	{                                                \
		if (*v)                                  \
			func(*v);                        \
	}                                                \
	struct _nl_dummy_for_tailing_semicolon

#define _NL_AUTO_DEFINE_FCN_INDIRECT0(CastType, name, func) \
	static inline void name(CastType *v)                \
	{                                                   \
		if (*v)                                     \
			func(v);                            \
	}                                                   \
	struct _nl_dummy_for_tailing_semicolon

#define _nl_auto_free _nl_auto(_nl_auto_free_fcn)
_NL_AUTO_DEFINE_FCN_VOID0(void *, _nl_auto_free_fcn, free);

/*****************************************************************************/

#define _nl_swap(p_a, p_b)                          \
	do {                                        \
		typeof(*(p_a)) *const _p_a = (p_a); \
		typeof(*(p_a)) *const _p_b = (p_b); \
		typeof(*(p_a)) _tmp;                \
                                                    \
		_tmp = *_p_a;                       \
		*_p_a = *_p_b;                      \
		*_p_b = _tmp;                       \
	} while (0)

/*****************************************************************************/

#define NSEC_PER_SEC 1000000000L

struct trans_tbl {
	uint64_t i;
	const char *a;
};

#define __ADD(id, name) { .i = id, .a = #name }

#define BUG()                                                                \
	do {                                                                 \
		fprintf(stderr, "BUG at file position %s:%d:%s\n", __FILE__, \
			__LINE__, __func__);                                 \
		assert(0);                                                   \
	} while (0)

#define BUG_ON(condition)      \
	do {                   \
		if (condition) \
			BUG(); \
	} while (0)

#define APPBUG(msg)                                                          \
	do {                                                                 \
		fprintf(stderr, "APPLICATION BUG: %s:%d:%s: %s\n", __FILE__, \
			__LINE__, __func__, msg);                            \
		assert(0);                                                   \
	} while (0)

/*****************************************************************************/

#ifndef DISABLE_PTHREADS
#define NL_LOCK(NAME) pthread_mutex_t(NAME) = PTHREAD_MUTEX_INITIALIZER
#define NL_RW_LOCK(NAME) pthread_rwlock_t(NAME) = PTHREAD_RWLOCK_INITIALIZER

static inline void nl_lock(pthread_mutex_t *lock)
{
	pthread_mutex_lock(lock);
}

static inline void nl_unlock(pthread_mutex_t *lock)
{
	pthread_mutex_unlock(lock);
}

static inline void nl_read_lock(pthread_rwlock_t *lock)
{
	pthread_rwlock_rdlock(lock);
}

static inline void nl_read_unlock(pthread_rwlock_t *lock)
{
	pthread_rwlock_unlock(lock);
}

static inline void nl_write_lock(pthread_rwlock_t *lock)
{
	pthread_rwlock_wrlock(lock);
}

static inline void nl_write_unlock(pthread_rwlock_t *lock)
{
	pthread_rwlock_unlock(lock);
}

#else
#define NL_LOCK(NAME) int __unused_lock_##NAME _nl_unused
#define NL_RW_LOCK(NAME) int __unused_lock_##NAME _nl_unused

#define nl_lock(LOCK) \
	do {          \
	} while (0)
#define nl_unlock(LOCK) \
	do {            \
	} while (0)
#define nl_read_lock(LOCK) \
	do {               \
	} while (0)
#define nl_read_unlock(LOCK) \
	do {                 \
	} while (0)
#define nl_write_lock(LOCK) \
	do {                \
	} while (0)
#define nl_write_unlock(LOCK) \
	do {                  \
	} while (0)
#endif

#endif /* __NETLINK_BASE_NL_BASE_UTILS_H__ */
