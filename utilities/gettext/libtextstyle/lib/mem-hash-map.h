/* Simple hash table (no removals) where the keys are memory blocks.
   Copyright (C) 1995-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#ifndef _GL_MEM_HASH_MAP_H
#define _GL_MEM_HASH_MAP_H

#include <stddef.h>

#include "obstack.h"

#ifdef __cplusplus
extern "C" {
#endif

struct hash_entry;

typedef struct hash_table
{
  size_t size;              /* Number of allocated entries.  */
  size_t filled;            /* Number of used entries.  */
  struct hash_entry *first; /* Pointer to head of list of entries.  */
  struct hash_entry *table; /* Pointer to array of entries.  */
  struct obstack mem_pool;  /* Memory pool holding the keys.  */
}
hash_table;

/* Initialize a hash table.  INIT_SIZE > 1 is the initial number of available
   entries.
   Return 0 always.  */
extern int hash_init (hash_table *htab, size_t init_size);

/* Delete a hash table's contents.
   Return 0 always.  */
extern int hash_destroy (hash_table *htab);

/* Look up the value of a key in the given table.
   If found, return 0 and set *RESULT to it.  Otherwise return -1.  */
extern int hash_find_entry (const hash_table *htab,
                            const void *key, size_t keylen,
                            void **result);

/* Try to insert the pair (KEY[0..KEYLEN-1], DATA) in the hash table.
   Return non-NULL (more precisely, the address of the KEY inside the table's
   memory pool) if successful, or NULL if there is already an entry with the
   given key.  */
extern const void * hash_insert_entry (hash_table *htab,
                                       const void *key, size_t keylen,
                                       void *data);

/* Insert the pair (KEY[0..KEYLEN-1], DATA) in the hash table.
   Return 0.  */
extern int hash_set_value (hash_table *htab,
                           const void *key, size_t keylen,
                           void *data);

/* Steps *PTR forward to the next used entry in the given hash table.  *PTR
   should be initially set to NULL.  Store information about the next entry
   in *KEY, *KEYLEN, *DATA.
   Return 0 normally, -1 when the whole hash table has been traversed.  */
extern int hash_iterate (hash_table *htab, void **ptr,
                         const void **key, size_t *keylen,
                         void **data);

/* Steps *PTR forward to the next used entry in the given hash table.  *PTR
   should be initially set to NULL.  Store information about the next entry
   in *KEY, *KEYLEN, *DATAP.  *DATAP is set to point to the storage of the
   value; modifying **DATAP will modify the value of the entry.
   Return 0 normally, -1 when the whole hash table has been traversed.  */
extern int hash_iterate_modify (hash_table *htab, void **ptr,
                                const void **key, size_t *keylen,
                                void ***datap);

#ifdef __cplusplus
}
#endif

#endif /* not _GL_MEM_HASH_MAP_H */
