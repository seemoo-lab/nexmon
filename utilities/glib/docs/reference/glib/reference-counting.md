Title: Reference Counting

## Reference counting types

Reference counting is a garbage collection mechanism that is based on
assigning a counter to a data type, or any memory area; the counter is
increased whenever a new reference to that data type is acquired, and
decreased whenever the reference is released. Once the last reference is
released, the resources associated to that data type are freed.

GLib uses reference counting in many of its data types, and provides the
`grefcount` and `gatomicrefcount` types to implement safe and atomic
reference counting semantics in new data types.

It is important to note that `grefcount` and `gatomicrefcount` should be
considered completely opaque types; you should always use the provided API
to increase and decrease the counters, and you should never check their
content directly, or compare their content with other values.

## Reference counted data

A "reference counted box", or "RcBox", is an opaque wrapper data type that
is guaranteed to be as big as the size of a given data type, and which
augments the given data type with reference counting semantics for its
memory management.

RcBox is useful if you have a plain old data type, like a structure
typically placed on the stack, and you wish to provide additional API to use
it on the heap; or if you want to implement a new type to be passed around
by reference without necessarily implementing copy/free semantics or your
own reference counting.

The typical use is:

```c
typedef struct {
  char *name;
  char *address;
  char *city;
  char *state;
  int age;
} Person;

Person *
person_new (void)
{
  return g_rc_box_new0 (Person);
}
```

Every time you wish to acquire a reference on the memory, you should call
`g_rc_box_acquire()`; similarly, when you wish to release a reference you
should call `g_rc_box_release()`:

```c
// Add a Person to the Database; the Database acquires ownership
// of the Person instance
void
add_person_to_database (Database *db, Person *p)
{
  db->persons = g_list_prepend (db->persons, g_rc_box_acquire (p));
}

// Removes a Person from the Database; the reference acquired by
// add_person_to_database() is released here
void
remove_person_from_database (Database *db, Person *p)
{
  db->persons = g_list_remove (db->persons, p);
  g_rc_box_release (p);
}
```

If you have additional memory allocated inside the structure, you can use
`g_rc_box_release_full()`, which takes a function pointer, which will be
called if the reference released was the last:

```c
void
person_clear (Person *p)
{
  g_free (p->name);
  g_free (p->address);
  g_free (p->city);
  g_free (p->state);
}

void
remove_person_from_database (Database *db, Person *p)
{
  db->persons = g_list_remove (db->persons, p);
  g_rc_box_release_full (p, (GDestroyNotify) person_clear);
}
```

If you wish to transfer the ownership of a reference counted data
type without increasing the reference count, you can use `g_steal_pointer()`:

```c
  Person *p = g_rc_box_new (Person);

  // fill_person_details() is defined elsewhere
  fill_person_details (p);

  // add_person_to_database_no_ref() is defined elsewhere; it adds
  // a Person to the Database without taking a reference
  add_person_to_database_no_ref (db, g_steal_pointer (&p));
```


## Thread safety

The reference counting operations on data allocated using
`g_rc_box_alloc()`, `g_rc_box_new()`, and `g_rc_box_dup()` are not thread
safe; it is your code's responsibility to ensure that references are
acquired are released on the same thread.

If you need thread safe reference counting, you should use the
`g_atomic_rc_*` API:

| Operation                       | Atomic equivalent                      |
|---------------------------------|----------------------------------------|
| [func@GLib.rc_box_alloc]        | [func@GLib.atomic_rc_box_alloc]        |
| [func@GLib.rc_box_alloc0]       | [func@GLib.atomic_rc_box_alloc0]       |
| [func@GLib.rc_box_new]          | [func@GLib.atomic_rc_box_new]          |
| [func@GLib.rc_box_new0]         | [func@GLib.atomic_rc_box_new0]         |
| [func@GLib.rc_box_dup]          | [func@GLib.atomic_rc_box_dup]          |
| [func@GLib.rc_box_acquire]      | [func@GLib.atomic_rc_box_acquire]      |
| [func@GLib.rc_box_release]      | [func@GLib.atomic_rc_box_release]      |
| [func@GLib.rc_box_release_full] | [func@GLib.atomic_rc_box_release_full] |
| [func@GLib.rc_box_get_size]     | [func@GLib.atomic_rc_box_get_size]     |

The reference counting operations on data allocated using
`g_atomic_rc_box_alloc()`, `g_atomic_rc_box_alloc0()`,
`g_atomic_rc_box_new()`, `g_atomic_rc_box_new0` and
`g_atomic_rc_box_dup()` are guaranteed to be atomic, and thus can be safely
be performed by different threads. It is important to note that only the
reference acquisition and release are atomic; changes to the content of the
data are your responsibility.

It is a programmer error to mix the atomic and non-atomic reference counting
operations.

## Automatic pointer clean up

If you want to add `g_autoptr()` support to your plain old data type through
reference counting, you can use the `G_DEFINE_AUTOPTR_CLEANUP_FUNC()` and
`g_rc_box_release()`:

```c
G_DEFINE_AUTOPTR_CLEANUP_FUNC (MyDataStruct, g_rc_box_release)
```

If you need to clear the contents of the data, you will need to use an
ancillary function that calls `g_rc_box_release_full()`:

```c
static void
my_data_struct_release (MyDataStruct *data)
{
  // my_data_struct_clear() is defined elsewhere
  g_rc_box_release_full (data, (GDestroyNotify) my_data_struct_clear);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (MyDataStruct, my_data_struct_release)
```

The `g_rc_box*` and `g_atomic_rc_box*` APIs were introduced in GLib 2.58.
