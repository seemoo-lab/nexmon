Title: Memory Slices

# Memory Slices

GSlice was a space-efficient and multi-processing scalable way to allocate
equal sized pieces of memory. Since GLib 2.76, its implementation has been
removed and it calls `g_malloc()` and `g_free()`, because the performance of the
system-default allocators has improved on all platforms since GSlice was
written.

The GSlice APIs have not been deprecated, as they are widely in use and doing
so would be very disruptive for little benefit.

New code should be written using [`func@GLib.new`]/[`func@GLib.malloc`] and
[`func@GLib.free`]. There is no particular benefit in porting existing code away
from `g_slice_new()`/`g_slice_free()` unless it’s being rewritten anyway.

Here is an example for using the slice allocator:

```c
gchar *mem[10000];
gint i;

// Allocate 10000 blocks.
for (i = 0; i < 10000; i++)
  {
    mem[i] = g_slice_alloc (50);

    // Fill in the memory with some junk.
    for (j = 0; j < 50; j++)
      mem[i][j] = i * j;
  }

// Now free all of the blocks.
for (i = 0; i < 10000; i++)
  g_slice_free1 (50, mem[i]);
```

And here is an example for using the using the slice allocator with data
structures:

```c
GRealArray *array;

// Allocate one block, using the g_slice_new() macro.
array = g_slice_new (GRealArray);

// We can now use array just like a normal pointer to a structure.
array->data            = NULL;
array->len             = 0;
array->alloc           = 0;
array->zero_terminated = (zero_terminated ? 1 : 0);
array->clear           = (clear ? 1 : 0);
array->elt_size        = elt_size;

// We can free the block, so it can be reused.
g_slice_free (GRealArray, array);
```
