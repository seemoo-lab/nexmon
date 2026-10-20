Title: Data Structures
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2010 Allison Lortie
SPDX-FileCopyrightText: 2011 Collabora, Ltd.
SPDX-FileCopyrightText: 2012 Olivier Sessink
SPDX-FileCopyrightText: 2010, 2011, 2014 Matthias Clasen
SPDX-FileCopyrightText: 2018 Sébastien Wilmet
SPDX-FileCopyrightText: 2018 Emmanuele Bassi
SPDX-FileCopyrightText: 2019 Emmanuel Fleury
SPDX-FileCopyrightText: 2017, 2018, 2019 Endless Mobile, Inc.
SPDX-FileCopyrightText: 2020 Endless OS Foundation, LLC

# Data Structures

GLib includes a number of basic data structures, such as arrays, linked lists, hash tables,
queues, trees, etc.

## Arrays

GLib arrays ([struct@GLib.Array]) are similar to standard C arrays, except that they grow
automatically as elements are added.

Array elements can be of any size (though all elements of one array are the same size),
and the array can be automatically cleared to '0's and zero-terminated.

To create a new array use [func@GLib.Array.new].

To add elements to an array with a cost of O(n) at worst, use
[func@GLib.array_append_val],
[func@GLib.Array.append_vals],
[func@GLib.array_prepend_val],
[func@GLib.Array.prepend_vals],
[func@GLib.array_insert_val] and
[func@GLib.Array.insert_vals].

To access an element of an array in O(1) (to read it or to write it),
use [func@GLib.array_index].

To set the size of an array, use [func@GLib.Array.set_size].

To free an array, use [func@GLib.Array.unref] or [func@GLib.Array.free].

All the sort functions are internally calling a quick-sort (or similar)
function with an average cost of O(n log(n)) and a worst case cost of O(n^2).

Here is an example that stores integers in a [struct@GLib.Array]:

```c
GArray *array;
int i;

// We create a new array to store int values.
// We don't want it zero-terminated or cleared to 0's.
array = g_array_new (FALSE, FALSE, sizeof (int));

for (i = 0; i < 10000; i++)
  {
    g_array_append_val (array, i);
  }

for (i = 0; i < 10000; i++)
  {
    if (g_array_index (array, int, i) != i)
      g_print ("ERROR: got %d instead of %d\n",
               g_array_index (array, int, i), i);
  }

g_array_free (array, TRUE);
```

## Pointer Arrays

Pointer Arrays ([struct@GLib.PtrArray]) are similar to Arrays but are used
only for storing pointers.

If you remove elements from the array, elements at the end of the
array are moved into the space previously occupied by the removed
element. This means that you should not rely on the index of particular
elements remaining the same. You should also be careful when deleting
elements while iterating over the array.

To create a pointer array, use [func@GLib.PtrArray.new].

To add elements to a pointer array, use [func@GLib.PtrArray.add].

To remove elements from a pointer array, use
[func@GLib.PtrArray.remove],
[func@GLib.PtrArray.remove_index] or
[func@GLib.PtrArray.remove_index_fast].

To access an element of a pointer array, use [func@GLib.ptr_array_index].

To set the size of a pointer array, use [func@GLib.PtrArray.set_size].

To free a pointer array, use [func@GLib.PtrArray.unref] or [func@GLib.PtrArray.free].

An example using a [struct@GLib.PtrArray]:

```c
GPtrArray *array;
char *string1 = "one";
char *string2 = "two";
char *string3 = "three";

array = g_ptr_array_new ();
g_ptr_array_add (array, (gpointer) string1);
g_ptr_array_add (array, (gpointer) string2);
g_ptr_array_add (array, (gpointer) string3);

if (g_ptr_array_index (array, 0) != (gpointer) string1)
  g_print ("ERROR: got %p instead of %p\n",
           g_ptr_array_index (array, 0), string1);

g_ptr_array_free (array, TRUE);
```

## Byte Arrays

[struct@GLib.ByteArray] is a mutable array of bytes based on [struct@GLib.Array],
to provide arrays of bytes which grow automatically as elements are added.

To create a new `GByteArray` use [func@GLib.ByteArray.new].

To add elements to a `GByteArray`, use
[func@GLib.ByteArray.append] and [func@GLib.ByteArray.prepend].

To set the size of a `GByteArray`, use [func@GLib.ByteArray.set_size].

To free a `GByteArray`, use [func@GLib.ByteArray.unref] or [func@GLib.ByteArray.free].

An example for using a `GByteArray`:

```c
GByteArray *array;
int i;

array = g_byte_array_new ();
for (i = 0; i < 10000; i++)
  {
    g_byte_array_append (array, (guint8*) "abcd", 4);
  }

for (i = 0; i < 10000; i++)
  {
    g_assert (array->data[4*i] == 'a');
    g_assert (array->data[4*i+1] == 'b');
    g_assert (array->data[4*i+2] == 'c');
    g_assert (array->data[4*i+3] == 'd');
  }

g_byte_array_free (array, TRUE);
```

See [struct@GLib.Bytes] if you are interested in an immutable object representing a
sequence of bytes.

## Singly-linked Lists

The [struct@GLib.SList] structure and its associated functions provide a standard
singly-linked list data structure. The benefit of this data structure is to provide
insertion/deletion operations in O(1) complexity where access/search operations are
in O(n). The benefit of `GSList` over [struct@GLib.List] (doubly-linked list) is that
they are lighter in space as they only need to retain one pointer but it double the
cost of the worst case access/search operations.

Each element in the list contains a piece of data, together with a pointer which links
to the next element in the list. Using this pointer it is possible to move through the
list in one direction only (unlike the [doubly-linked lists](#doubly-linked-lists),
which allow movement in both directions).

The data contained in each element can be either integer values, by
using one of the [Type Conversion Macros](conversion-macros.html),
or simply pointers to any type of data.

Note that most of the #GSList functions expect to be passed a pointer to the first element
in the list. The functions which insert elements return the new start of the list, which
may have changed.

There is no function to create a `GSList`. `NULL` is considered to be the empty list so you
simply set a `GSList*` to `NULL`.

To add elements, use [func@GLib.SList.append], [func@GLib.SList.prepend],
[func@GLib.SList.insert] and [func@GLib.SList.insert_sorted].

To remove elements, use [func@GLib.SList.remove].

To find elements in the list use [func@GLib.SList.last], [func@GLib.slist_next],
[func@GLib.SList.nth], [func@GLib.SList.nth_data], [func@GLib.SList.find] and
[func@GLib.SList.find_custom].

To find the index of an element use [func@GLib.SList.position] and [func@GLib.SList.index].

To call a function for each element in the list use [func@GLib.SList.foreach].

To free the entire list, use [func@GLib.SList.free].

## Doubly-linked Lists

The [struct@GLib.List] structure and its associated functions provide a standard
doubly-linked list data structure. The benefit of this data-structure is to provide
insertion/deletion operations in O(1) complexity where access/search operations are in O(n).
The benefit of `GList` over [struct@GLib.SList] (singly-linked list) is that the worst case
on access/search operations is divided by two which comes at a cost in space as we need
to retain two pointers in place of one.

Each element in the list contains a piece of data, together with pointers which link to the
previous and next elements in the list. Using these pointers it is possible to move through
the list in both directions (unlike the singly-linked [struct@GLib.SList],
which only allows movement through the list in the forward direction).

The doubly-linked list does not keep track of the number of items and does not keep track of
both the start and end of the list. If you want fast access to both the start and the end of
the list,  and/or the number of items in the list, use a [struct@GLib.Queue] instead.

The data contained in each element can be either integer values, by using one of the
[Type Conversion Macros](conversion-macros.html), or simply pointers to any type of data.

Note that most of the `GList` functions expect to be passed a pointer to the first element in the list.
The functions which insert elements return the new start of the list, which may have changed.

There is no function to create a `GList`. `NULL` is considered to be a valid, empty list so you simply
set a `GList*` to `NULL` to initialize it.

To add elements, use [func@GLib.List.append], [func@GLib.List.prepend],
[func@GLib.List.insert] and [func@GLib.List.insert_sorted].

To visit all elements in the list, use a loop over the list:

```c
GList *l;
for (l = list; l != NULL; l = l->next)
  {
    // do something with l->data
  }
```

To call a function for each element in the list, use [func@GLib.List.foreach].

To loop over the list and modify it (e.g. remove a certain element) a while loop is more appropriate,
for example:

```c
GList *l = list;
while (l != NULL)
  {
    GList *next = l->next;
    if (should_be_removed (l))
      {
        // possibly free l->data
        list = g_list_delete_link (list, l);
      }
    l = next;
  }
```

To remove elements, use [func@GLib.List.remove].

To navigate in a list, use [func@GLib.List.first], [func@GLib.List.last],
[func@GLib.list_next], [func@GLib.list_previous].

To find elements in the list use [func@GLib.List.nth], [func@GLib.List.nth_data],
[func@GLib.List.find] and [func@GLib.List.find_custom].

To find the index of an element use [func@GLib.List.position] and [func@GLib.List.index].

To free the entire list, use [func@GLib.List.free] or [func@GLib.List.free_full].


## Hash Tables

A [struct@GLib.HashTable] provides associations between keys and values which is
optimized so that given a key, the associated value can be found, inserted or removed
in amortized O(1). All operations going through each element take O(n) time (list all
keys/values, table resize, etc.).

Note that neither keys nor values are copied when inserted into the `GHashTable`,
so they must exist for the lifetime of the `GHashTable`. This means that the use
of static strings is OK, but temporary strings (i.e. those created in buffers and those
returned by GTK widgets) should be copied with [func@GLib.strdup] before being inserted.

If keys or values are dynamically allocated, you must be careful to ensure that they are freed
when they are removed from the `GHashTable`, and also when they are overwritten by
new insertions into the `GHashTable`. It is also not advisable to mix static strings
and dynamically-allocated strings in a [struct@GLib.HashTable], because it then becomes difficult
to determine whether the string should be freed.

To create a `GHashTable`, use [func@GLib.HashTable.new].

To insert a key and value into a `GHashTable`, use [func@GLib.HashTable.insert].

To look up a value corresponding to a given key, use [func@GLib.HashTable.lookup] or
[func@GLib.HashTable.lookup_extended].

[func@GLib.HashTable.lookup_extended] can also be used to simply check if a key is present
in the hash table.

To remove a key and value, use [func@GLib.HashTable.remove].

To call a function for each key and value pair use [func@GLib.HashTable.foreach] or use
an iterator to iterate over the key/value pairs in the hash table, see [struct@GLib.HashTableIter].
The iteration order of a hash table is not defined, and you must not rely on iterating over
keys/values in the same order as they were inserted.

To destroy a `GHashTable` use [func@GLib.HashTable.unref] or [func@GLib.HashTable.destroy].

A common use-case for hash tables is to store information about a set of keys, without associating any
particular value with each key. `GHashTable` optimizes one way of doing so: If you store only
key-value pairs where key == value, then `GHashTable` does not allocate memory to store the values,
which can be a considerable space saving, if your set is large. The functions [func@GLib.HashTable.add]
and [func@GLib.HashTable.contains] are designed to be used when using `GHashTable` this way.

`GHashTable` is not designed to be statically initialised with keys and values known at compile time.
To build a static hash table, use a tool such as [gperf](https://www.gnu.org/software/gperf/).

## Double-ended Queues

The [struct@GLib.Queue] structure and its associated functions provide a standard queue data structure.
Internally, `GQueue` uses the same data structure as [struct@GLib.List] to store elements with the same
complexity over insertion/deletion (O(1)) and access/search (O(n)) operations.

The data contained in each element can be either integer values, by using one of the
[Type Conversion Macros](conversion-macros.html), or simply pointers to any type of data.

As with all other GLib data structures, `GQueue` is not thread-safe. For a thread-safe queue, use
[struct@GLib.AsyncQueue].

To create a new GQueue, use [func@GLib.Queue.new].

To initialize a statically-allocated GQueue, use `G_QUEUE_INIT` or [method@GLib.Queue.init].

To add elements, use [method@GLib.Queue.push_head], [method@GLib.Queue.push_head_link],
[method@GLib.Queue.push_tail] and [method@GLib.Queue.push_tail_link].

To remove elements, use [method@GLib.Queue.pop_head] and [method@GLib.Queue.pop_tail].

To free the entire queue, use [method@GLib.Queue.free].

## Asynchronous Queues

Often you need to communicate between different threads. In general it's safer not to do this
by shared memory, but by explicit message passing. These messages only make sense asynchronously
for multi-threaded applications though, as a synchronous operation could as well be done in the
same thread.

Asynchronous queues are an exception from most other GLib data structures, as they can be used
simultaneously from multiple threads without explicit locking and they bring their own builtin
reference counting. This is because the nature of an asynchronous queue is that it will always
be used by at least 2 concurrent threads.

For using an asynchronous queue you first have to create one with [func@GLib.AsyncQueue.new].
[struct@GLib.AsyncQueue] structs are reference counted, use [method@GLib.AsyncQueue.ref] and
[method@GLib.AsyncQueue.unref] to manage your references.

A thread which wants to send a message to that queue simply calls [method@GLib.AsyncQueue.push]
to push the message to the queue.

A thread which is expecting messages from an asynchronous queue simply calls [method@GLib.AsyncQueue.pop]
for that queue. If no message is available in the queue at that point, the thread is now put to sleep
until a message arrives. The message will be removed from the queue and returned. The functions
[method@GLib.AsyncQueue.try_pop] and [method@GLib.AsyncQueue.timeout_pop] can be used to only check
for the presence of messages or to only wait a certain time for messages respectively.

For almost every function there exist two variants, one that locks the queue and one that doesn't.
That way you can hold the queue lock (acquire it with [method@GLib.AsyncQueue.lock] and release it
with [method@GLib.AsyncQueue.unlock] over multiple queue accessing instructions. This can be necessary
to ensure the integrity of the queue, but should only be used when really necessary, as it can make your
life harder if used unwisely. Normally you should only use the locking function variants (those without
the `_unlocked` suffix).

In many cases, it may be more convenient to use [struct@GLib.ThreadPool] when you need to distribute work
to a set of worker threads instead of using `GAsyncQueue` manually. `GThreadPool` uses a `GAsyncQueue`
internally.

## Binary Trees

The [struct@GLib.Tree] structure and its associated functions provide a sorted collection of key/value
pairs optimized for searching and traversing in order. This means that most of the operations (access,
search, insertion, deletion, …) on `GTree` are O(log(n)) in average and O(n) in worst case for time
complexity. But, note that maintaining a balanced sorted `GTree` of n elements is done in time O(n log(n)).

To create a new `GTree` use [ctor@GLib.Tree.new].

To insert a key/value pair into a `GTree` use [method@GLib.Tree.insert] (O(n log(n))).

To remove a key/value pair use [method@GLib.Tree.remove] (O(n log(n))).

To look up the value corresponding to a given key, use [method@GLib.Tree.lookup] and
[method@GLib.Tree.lookup_extended].

To find out the number of nodes in a `GTree`, use [method@GLib.Tree.nnodes].
To get the height of a `GTree`, use [method@GLib.Tree.height].

To traverse a `GTree`, calling a function for each node visited in
the traversal, use [method@GLib.Tree.foreach].

To destroy a `GTree`, use [method@GLib.Tree.destroy].

## N-ary Trees

The [struct@GLib.Node] struct and its associated functions provide a N-ary tree
data structure, where nodes in the tree can contain arbitrary data.

To create a new tree use [func@GLib.Node.new].

To insert a node into a tree use [method@GLib.Node.insert], [method@GLib.Node.insert_before],
[func@GLib.node_append] and [method@GLib.Node.prepend],

To create a new node and insert it into a tree use [func@GLib.node_insert_data],
[func@GLib.node_insert_data_after], [func@GLib.node_insert_data_before],
[func@GLib.node_append_data] and [func@GLib.node_prepend_data].

To reverse the children of a node use [method@GLib.Node.reverse_children].

To find a node use [method@GLib.Node.get_root], [method@GLib.Node.find], [method@GLib.Node.find_child],
[method@GLib.Node.child_index], [method@GLib.Node.child_position], [func@GLib.node_first_child],
[method@GLib.Node.last_child], [method@GLib.Node.nth_child], [method@GLib.Node.first_sibling],
[func@GLib.node_prev_sibling], [func@GLib.node_next_sibling] or [method@GLib.Node.last_sibling].

To get information about a node or tree use `G_NODE_IS_LEAF()`,
`G_NODE_IS_ROOT()`, [method@GLib.Node.depth], [method@GLib.Node.n_nodes],
[method@GLib.Node.n_children], [method@GLib.Node.is_ancestor] or [method@GLib.Node.max_height].

To traverse a tree, calling a function for each node visited in the traversal, use
[method@GLib.Node.traverse] or [method@GLib.Node.children_foreach].

To remove a node or subtree from a tree use [method@GLib.Node.unlink] or [method@GLib.Node.destroy].

## Scalable Lists

The [struct@GLib.Sequence] data structure has the API of a list, but is implemented internally with
a balanced binary tree. This means that most of the operations  (access, search, insertion, deletion,
...) on `GSequence` are O(log(n)) in average and O(n) in worst case for time complexity. But, note that
maintaining a balanced sorted list of n elements is done in time O(n log(n)). The data contained
in each element can be either integer values, by using of the
[Type Conversion Macros](conversion-macros.md), or simply pointers to any type of data.

A `GSequence` is accessed through "iterators", represented by a [struct@GLib.SequenceIter]. An iterator
represents a position between two elements of the sequence. For example, the "begin" iterator represents
the gap immediately before the first element of the sequence, and the "end" iterator represents the gap
immediately after the last element. In an empty sequence, the begin and end iterators are the same.

Some methods on `GSequence` operate on ranges of items. For example [func@GLib.Sequence.foreach_range]
will call a user-specified function on each element with the given range. The range is delimited by the
gaps represented by the passed-in iterators, so if you pass in the begin and end iterators, the range in
question is the entire sequence.

The function [func@GLib.Sequence.get] is used with an iterator to access the element immediately following
the gap that the iterator represents. The iterator is said to "point" to that element.

Iterators are stable across most operations on a `GSequence`. For example an iterator pointing to some element
of a sequence will continue to point to that element even after the sequence is sorted. Even moving an element
to another sequence using for example [func@GLib.Sequence.move_range] will not invalidate the iterators pointing
to it. The only operation that will invalidate an iterator is when the element it points to is removed from
any sequence.

To sort the data, either use [method@GLib.Sequence.insert_sorted] or [method@GLib.Sequence.insert_sorted_iter]
to add data to the `GSequence` or, if you want to add a large amount of data, it is more efficient to call
[method@GLib.Sequence.sort] or [method@GLib.Sequence.sort_iter] after doing unsorted insertions.

## Reference-counted strings

Reference-counted strings are normal C strings that have been augmented with a reference count to manage
their resources. You allocate a new reference counted string and acquire and release references as needed,
instead of copying the string among callers; when the last reference on the string is released, the resources
allocated for it are freed.

Typically, reference-counted strings can be used when parsing data from files and storing them into data
structures that are passed to various callers:

```c
PersonDetails *
person_details_from_data (const char *data)
{
  // Use g_autoptr() to simplify error cases
  g_autoptr(GRefString) full_name = NULL;
  g_autoptr(GRefString) address =  NULL;
  g_autoptr(GRefString) city = NULL;
  g_autoptr(GRefString) state = NULL;
  g_autoptr(GRefString) zip_code = NULL;

  // parse_person_details() is defined elsewhere; returns refcounted strings
  if (!parse_person_details (data, &full_name, &address, &city, &state, &zip_code))
    return NULL;

  if (!validate_zip_code (zip_code))
    return NULL;

  // add_address_to_cache() and add_full_name_to_cache() are defined
  // elsewhere; they add strings to various caches, using refcounted
  // strings to avoid copying data over and over again
  add_address_to_cache (address, city, state, zip_code);
  add_full_name_to_cache (full_name);

  // person_details_new() is defined elsewhere; it takes a reference
  // on each string
  PersonDetails *res = person_details_new (full_name,
                                           address,
                                           city,
                                           state,
                                           zip_code);

  return res;
}
```

In the example above, we have multiple functions taking the same strings for different uses; with typical
C strings, we'd have to copy the strings every time the life time rules of the data differ from the
life-time of the string parsed from the original buffer. With reference counted strings, each caller can
take a reference on the data, and keep it as long as it needs to own the string.

Reference-counted strings can also be "interned" inside a global table owned by GLib; while an interned
string has at least a reference, creating a new interned reference-counted string with the same contents
will return a reference to the existing string instead of creating a new reference-counted string instance.
Once the string loses its last reference, it will be automatically removed from the global interned strings
table.

Reference-counted strings were added to GLib in 2.58.
