Title: Floating References

# Floating References

**Note**: Floating references are a C convenience API and should not be used
in modern GObject code. Language bindings in particular find the concept
highly problematic, as floating references are not identifiable through
annotations, and neither are deviations from the floating reference
behavior, like types that inherit from [class@GObject.InitiallyUnowned] and
still return a full reference from [`id@g_object_new`].

[class@GObject.InitiallyUnowned] is derived from [class@GObject.Object]. The
only difference between the two is that the initial reference of a
`GInitiallyUnowned` is flagged as a "floating" reference. This means that it
is not specifically claimed to be "owned" by any code portion. The main
motivation for providing floating references is C convenience. In
particular, it allows code to be written as:

```c
container = create_container ();
container_add_child (container, create_child());
```

If `container_add_child()` calls [`method@GObject.Object.ref_sink`] on the
passed-in child, no reference of the newly created child is leaked. Without
floating references, `container_add_child()` can only acquire a reference
the new child, so to implement this code without reference leaks, it would
have to be written as:

```c
Child *child;
container = create_container ();
child = create_child ();
container_add_child (container, child);
g_object_unref (child);
```

The floating reference can be converted into an ordinary reference by
calling `g_object_ref_sink()`. For already sunken objects (objects that
don't have a floating reference anymore), `g_object_ref_sink()` is
equivalent to [`method@GObject.Object.ref`] and returns a new reference.

Since floating references are useful almost exclusively for C convenience,
language bindings that provide automated reference and memory ownership
maintenance (such as smart pointers or garbage collection) should not expose
floating references in their API. The best practice for handling types that
have initially floating references is to immediately sink those references
after `g_object_new()` returns, by checking if the `GType` inherits from
`GInitiallyUnowned`. For instance:

```c
GObject *res = g_object_new_with_properties (gtype,
                                             n_props,
                                             prop_names,
                                             prop_values);

// or: if (g_type_is_a (gtype, G_TYPE_INITIALLY_UNOWNED))
if (G_IS_INITIALLY_UNOWNED (res))
  g_object_ref_sink (res);

return res;
```

Some object implementations may need to save an object's floating state
across certain code portions (an example is `GtkMenu` in GTK3), to achieve
this, the following sequence can be used:

```c
// save floating state
gboolean was_floating = g_object_is_floating (object);
g_object_ref_sink (object);
// protected code portion

...

// restore floating state
if (was_floating)
  g_object_force_floating (object);
else
  g_object_unref (object); // release previously acquired reference
```

## Replacing floating references with annotations

You should avoid basing your object hierarchy on floating references, as
they are hard to understand even in C, and introduce additional limitations
when consuming a GObject-based API in other languages.

One way to express the ownership transfer between ‘container’ and ‘child’
instances is to use ownership transfer annotations in your documentation and
introspection data. For instance, you can implement this pattern:

```c
container_add_child (container, create_child ());
```

without leaking by having `container_add_child()` defined as:

```c
/**
 * container_add_child:
 * @self: a container
 * @child: (transfer full): the child to add to the container
 *
 * ...
 */
void
container_add_child (Container *container,
                     Child *child)
{
  container->children = g_list_append (container->children, child);
}
```

The container does not explicitly acquire a reference on the child; instead,
the ownership of the child is transferred to the container. The transfer
annotation will be used by language bindings to ensure that there are no
leaks; and documentation tools will explicitly note that the callee now owns
the value passed by the caller.

## Replacing floating references with weak references

Another option for replacing floating references is to use weak references
in place of strong ones. A ‘container’ can acquire a weak reference on the
‘child’ instance by using [`method@GObject.Object.weak_ref`]. Once the
child instance loses its last strong reference, the container holding a
weak reference is notified, and it can either remove the child from its
internal list, or turn a weak reference into a strong one.
