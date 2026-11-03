/* common.h */
/* See Copyright Notice in the file LICENSE */
/* SPDX-License-Identifier: MIT */

#ifndef COMMON_H
#define COMMON_H

#include "lua.h"

#if LUA_VERSION_NUM > 501
# ifndef lua_objlen
#  define lua_objlen lua_rawlen
# endif
  int luaL_typerror (lua_State *L, int narg, const char *tname);
#endif

/* REX_API can be overridden from the command line or Makefile */
#ifndef REX_API
#  define REX_API LUALIB_API
#endif

/* Special values for maxmatch in gsub. They all must be negative. */
#define GSUB_UNLIMITED   -1
#define GSUB_CONDITIONAL -2

/* Common structs and functions */

typedef struct {
  const char* key;
  int val;
} flag_pair;

typedef struct {            /* compile arguments */
  const char * pattern;
  size_t       patlen;
  void       * ud;
  int          cflags;
  const char * locale;             /* PCRE, Oniguruma */
  const unsigned char * tables;    /* PCRE */
  int          tablespos;          /* PCRE */
  void       * syntax;             /* Oniguruma */
  const unsigned char * translate; /* GNU */
  int          gnusyn;             /* GNU */
} TArgComp;

typedef struct {            /* exec arguments */
  const char * text;
  size_t       textlen;
  int          startoffset;
  int          eflags;
  int          funcpos;
  int          maxmatch;
  int          funcpos2;          /* used with gsub */
  int          reptype;           /* used with gsub */
  size_t       ovecsize;          /* PCRE: dfa_exec */
  size_t       wscount;           /* PCRE: dfa_exec */
} TArgExec;

struct tagFreeList; /* forward declaration */

struct tagBuffer {
  size_t      size;
  size_t      top;
  char      * arr;
  lua_State * L;
  struct tagFreeList * freelist;
};

struct tagFreeList {
  struct tagBuffer * list[16];
  int top;
};

typedef struct tagBuffer TBuffer;
typedef struct tagFreeList TFreeList;

/**
 * @brief Initialize a free list.
 *
 * @param fl Pointer to the free list structure to be initialized.
 */
void freelist_init (TFreeList *fl);

/**
 * @brief Adds a buffer to the free list.
 *
 * @param fl The free list to which the buffer will be added.
 * @param buf The buffer to add to the free list.
 */
void freelist_add (TFreeList *fl, TBuffer *buf);

/**
 * @brief Frees all buffers in a free list.
 *
 * @param fl Pointer to the free list to be freed.
 */
void freelist_free (TFreeList *fl);

void buffer_init (TBuffer *buf, size_t sz, lua_State *L, TFreeList *fl);

/**
 * @brief Free the memory allocated for a buffer.
 *
 * @param buf Pointer to the buffer to be freed.
 */
void buffer_free (TBuffer *buf);

/**
 * @brief Clears the contents of a buffer.
 *
 * @param buf Pointer to the buffer to be cleared.
 */
void buffer_clear (TBuffer *buf);

/**
 * @brief Adds the contents of one buffer to another.
 *
 * @param trg The target buffer where data will be added.
 * @param src The source buffer whose contents will be added to the target buffer.
 */
void buffer_addbuffer (TBuffer *trg, TBuffer *src);

/**
 * @brief Adds a long string to the buffer.
 *
 * @param buf The buffer to add the string to.
 * @param src The source string to add.
 * @param sz The size of the source string.
 */
void buffer_addlstring (TBuffer *buf, const void *src, size_t sz);
void buffer_addvalue (TBuffer *buf, int stackpos);

/**
 * @brief Pushes the result of a buffer to Lua.
 *
 * @param buf The buffer containing the data to be pushed.
 */
void buffer_pushresult (TBuffer *buf);

/**
 * @brief Puts a repeated string into the buffer.
 *
 * @param buf The buffer to modify.
 * @param reppos The position of the repeated string in the Lua stack.
 * @param nsub The number of substitutions.
 */
void bufferZ_putrepstring (TBuffer *buf, int reppos, int nsub);

/**
 * @brief Iterate over a buffer and retrieve the next element.
 *
 * @param buf Pointer to the TBuffer structure.
 * @param iter Pointer to the current iteration index.
 * @param len Pointer to store the length of the current element.
 * @param str Pointer to store the string value of the current element (if applicable).
 * @return 1 if an element is found, 0 otherwise.
 */
int  bufferZ_next (TBuffer *buf, size_t *iter, size_t *len, const char **str);

/**
 * @brief Adds a long string to the buffer.
 *
 * @param buf The buffer to add the string to.
 * @param src The source string to add.
 * @param len The length of the source string.
 */
void bufferZ_addlstring (TBuffer *buf, const void *src, size_t len);

/**
 * @brief Adds a number to a buffer.
 *
 * @param buf The buffer to which the number will be added.
 * @param num The number to add to the buffer.
 */
void bufferZ_addnum (TBuffer *buf, size_t num);

/**
 * @brief Retrieves an integer field from a Lua table.
 *
 * @param L The Lua state.
 * @param field The name of the field to retrieve.
 * @return The value of the field as an integer.
 */
int  get_int_field (lua_State *L, const char* field);

/**
 * @brief Sets an integer field in a Lua table.
 *
 * @param L The Lua state.
 * @param field The name of the field to set.
 * @param val The value to set the field to.
 */
void set_int_field (lua_State *L, const char* field, int val);

/**
 * @brief Retrieves flags from a table and stores them in an array.
 *
 * This function takes a Lua state and an array of flag pairs, then retrieves
 * flags from the Lua table and stores them in the provided array.
 *
 * @param L The Lua state.
 * @param arrs Pointer to an array of flag pairs.
 * @return Number of tables processed.
 */
int get_flags (lua_State *L, const flag_pair **arrs);

/**
 * @brief Retrieves the key for a given flag value.
 *
 * @param fp Pointer to the flag pair.
 * @param val The flag value.
 * @return The key corresponding to the flag value.
 */
const char *get_flag_key (const flag_pair *fp, int val);

/**
 * @brief Allocate memory using Lua's allocator.
 *
 * @param L Lua state.
 * @param size Size of memory to allocate.
 * @return Pointer to allocated memory or NULL on failure.
 */
void *Lmalloc (lua_State *L, size_t size);

/**
 * @brief Reallocates memory using Lua's allocator.
 *
 * This function reallocates a block of memory previously allocated by Lrealloc or Lmalloc.
 *
 * @param L The Lua state.
 * @param p Pointer to the old memory block, or NULL if allocating new memory.
 * @param osize Size of the old memory block in bytes.
 * @param nsize New size for the memory block in bytes.
 * @return Pointer to the reallocated memory block, or NULL on failure.
 */
void *Lrealloc (lua_State *L, void *p, size_t osize, size_t nsize);

/**
 * @brief Frees memory allocated for a Lua state.
 *
 * This function is used to free memory that was previously allocated using Lua's allocator.
 *
 * @param L The Lua state from which memory is being freed.
 * @param p Pointer to the memory block to be freed.
 * @param size Size of the memory block to be freed.
 */
void Lfree (lua_State *L, void *p, size_t size);

#ifndef REX_NOEMBEDDEDTEST

/**
 * @brief Creates a new memory buffer from a Lua string.
 *
 * This function takes a Lua string and creates a new memory buffer with the same content.
 * The buffer is returned as a userdata in the Lua stack along with its metatable.
 *
 * @param L The Lua state.
 * @return Number of values pushed to the Lua stack (1).
 */
int newmembuffer (lua_State *L);
#endif

#endif
