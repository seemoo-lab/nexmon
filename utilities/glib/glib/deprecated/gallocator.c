/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

/* we know we are deprecated here, no need for warnings */
#ifndef GLIB_DISABLE_DEPRECATION_WARNINGS
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include "gallocator.h"

#include <glib/gmessages.h>
#include <glib/gslice.h>

/**
 * GAllocator:
 *
 * Deprecated: 2.10
 */

/**
 * G_ALLOC_ONLY:
 *
 * Deprecated: 2.10
 */

/**
 * G_ALLOC_AND_FREE:
 *
 * Deprecated: 2.10
 */

/**
 * G_ALLOCATOR_LIST:
 *
 * Deprecated: 2.10
 */

/**
 * G_ALLOCATOR_SLIST:
 *
 * Deprecated: 2.10
 */

/**
 * G_ALLOCATOR_NODE:
 *
 * Deprecated: 2.10
 */

/**
 * g_chunk_new:
 *
 * Deprecated: 2.10
 */

/**
 * g_chunk_new0:
 *
 * Deprecated: 2.10
 */

/**
 * g_chunk_free:
 *
 * Deprecated: 2.10
 */

/**
 * g_mem_chunk_create:
 *
 * Deprecated: 2.10
 */

/**
 * GMemChunk:
 *
 * Deprecated: 2.10
 */
struct _GMemChunk {
  guint alloc_size;           /* the size of an atom */
};

/**
 * g_mem_chunk_new:
 *
 * Deprecated: 2.10
 */
GMemChunk*
g_mem_chunk_new (const gchar *name,
                 gint         atom_size,
                 gsize        area_size,
                 gint         type)
{
  GMemChunk *mem_chunk;

  g_return_val_if_fail (atom_size > 0, NULL);

  mem_chunk = g_slice_new (GMemChunk);
  mem_chunk->alloc_size = (guint) atom_size;

  return mem_chunk;
}

/**
 * g_mem_chunk_destroy:
 *
 * Deprecated: 2.10
 */
void
g_mem_chunk_destroy (GMemChunk *mem_chunk)
{
  g_return_if_fail (mem_chunk != NULL);

  g_slice_free (GMemChunk, mem_chunk);
}

/**
 * g_mem_chunk_alloc:
 *
 * Deprecated: 2.10
 */
gpointer
g_mem_chunk_alloc (GMemChunk *mem_chunk)
{
  g_return_val_if_fail (mem_chunk != NULL, NULL);

  return g_slice_alloc (mem_chunk->alloc_size);
}

/**
 * g_mem_chunk_alloc0:
 *
 * Deprecated: 2.10
 */
gpointer
g_mem_chunk_alloc0 (GMemChunk *mem_chunk)
{
  g_return_val_if_fail (mem_chunk != NULL, NULL);

  return g_slice_alloc0 (mem_chunk->alloc_size);
}

/**
 * g_mem_chunk_free:
 *
 * Deprecated: 2.10
 */
void
g_mem_chunk_free (GMemChunk *mem_chunk,
                  gpointer   mem)
{
  g_return_if_fail (mem_chunk != NULL);

  g_slice_free1 (mem_chunk->alloc_size, mem);
}

/**
 * g_allocator_new:
 *
 * Deprecated: 2.10
 */
GAllocator*
g_allocator_new (const gchar *name,
                 guint        n_preallocs)
{
  /* some (broken) GAllocator uses depend on non-NULL allocators */
  return (void *) 1;
}

/**
 * g_allocator_free:
 *
 * Deprecated: 2.10
 */
void g_allocator_free           (GAllocator *allocator) { }

/**
 * g_mem_chunk_clean:
 *
 * Deprecated: 2.10
 */
void g_mem_chunk_clean          (GMemChunk *mem_chunk)  { }

/**
 * g_mem_chunk_reset:
 *
 * Deprecated: 2.10
 */
void g_mem_chunk_reset          (GMemChunk *mem_chunk)  { }

/**
 * g_mem_chunk_print:
 *
 * Deprecated: 2.10
 */
void g_mem_chunk_print          (GMemChunk *mem_chunk)  { }

/**
 * g_mem_chunk_info:
 *
 * Deprecated: 2.10
 */
void g_mem_chunk_info           (void)                  { }

/**
 * g_blow_chunks:
 *
 * Deprecated: 2.10
 */
void g_blow_chunks              (void)                  { }

/**
 * g_list_push_allocator:
 *
 * Deprecated: 2.10
 */
void g_list_push_allocator      (GAllocator *allocator) { }

/**
 * g_list_pop_allocator:
 *
 * Deprecated: 2.10
 */
void g_list_pop_allocator       (void)                  { }

/**
 * g_slist_push_allocator:
 *
 * Deprecated: 2.10
 */
void g_slist_push_allocator     (GAllocator *allocator) { }

/**
 * g_slist_pop_allocator:
 *
 * Deprecated: 2.10
 */
void g_slist_pop_allocator      (void)                  { }

/**
 * g_node_push_allocator:
 *
 * Deprecated: 2.10
 */
void g_node_push_allocator      (GAllocator *allocator) { }

/**
 * g_node_pop_allocator:
 *
 * Deprecated: 2.10
 */
void g_node_pop_allocator       (void)                  { }
