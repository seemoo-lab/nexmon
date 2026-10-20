Title: Conversion Macros

# Conversion Macros

## Type Conversion

Many times GLib, GTK, and other libraries allow you to pass "user data" to a
callback, in the form of a void pointer. From time to time you want to pass
an integer instead of a pointer. You could allocate an integer, with
something like:

```c
int *ip = g_new (int, 1);
*ip = 42;
```

But this is inconvenient, and it's annoying to have to free the memory at
some later time.

Pointers are always at least 32 bits in size (on all platforms GLib intends
to support). Thus you can store at least 32-bit integer values in a pointer
value. Naively, you might try this, but it's incorrect:

```c
gpointer p;
int i;
p = (void*) 42;
i = (int) p;
```

Again, that example was not correct, don't copy it.

The problem is that on some systems you need to do this:

```c
gpointer p;
int i;
p = (void*) (long) 42;
i = (int) (long) p;
```

The GLib macros [`GPOINTER_TO_INT()`](#gpointer-to-int), [`GINT_TO_POINTER()`](#gint-to-pointer),
etc. take care to do the right thing on every platform.

**Warning**: You may not store pointers in integers. This is not portable in
any way, shape or form. These macros only allow storing integers in
pointers, and only preserve 32 bits of the integer; values outside the range
of a 32-bit integer will be mangled.


`GINT_TO_POINTER(value)`<a name="gint-to-pointer"></a>, `GPOINTER_TO_INT(value)`<a name="gpointer-to-int"></a>

:   Stuffs an integer into a pointer type, and vice versa.

    Remember, you may not store pointers in integers. This is not portable in
    any way, shape or form. These macros only allow storing integers in
    pointers, and only preserve 32 bits of the integer; values outside the
    range of a 32-bit integer will be mangled.

`GUINT_TO_POINTER(value)`<a name="guint-to-pointer"></a>, `GPOINTER_TO_UINT(value)`<a name="gpointer-to-uint"></a>

:   Stuffs an unsigned integer into a pointer type, and vice versa.

`GSIZE_TO_POINTER(value)`<a name="gsize-to-pointer"></a>, `GPOINTER_TO_SIZE(value)`<a name="gpointer-to-size"></a>

:   Stuffs a `size_t` into a pointer type, and vice versa.

`GTYPE_TO_POINTER(value)`<a name="gtype-to-pointer"></a>, `GPOINTER_TO_TYPE(value)`<a name="gpointer-to-type"></a>

:   Stuffs a [`GType`](../gobject/alias.Type.html) into a pointer type, and
    vice versa.

    These macros should be used instead of [`GSIZE_TO_POINTER()`](#gsize-to-pointer),
    [`GPOINTER_TO_SIZE()`](#gpointer-to-size) to ensure portability, since
    `GType` is not guaranteed to be the same as `size_t`.

    Since: 2.80

## Byte Order Conversion

These macros provide a portable way to determine the host byte order and to
convert values between different byte orders.

The byte order is the order in which bytes are stored to create larger data
types such as the #gint and #glong values.  The host byte order is the byte
order used on the current machine.

Some processors store the most significant bytes (i.e. the bytes that hold
the largest part of the value) first. These are known as big-endian
processors. Other processors (notably the x86 family) store the most
significant byte last. These are known as little-endian processors.

Finally, to complicate matters, some other processors store the bytes in a
rather curious order known as PDP-endian. For a 4-byte word, the 3rd most
significant byte is stored first, then the 4th, then the 1st and finally the
2nd.

Obviously there is a problem when these different processors communicate
with each other, for example over networks or by using binary file formats.
This is where these macros come in. They are typically used to convert
values into a byte order which has been agreed on for use when communicating
between different processors. The Internet uses what is known as 'network
byte order' as the standard byte order (which is in fact the big-endian byte
order).

Note that the byte order conversion macros may evaluate their arguments
multiple times, thus you should not use them with arguments which have
side-effects.

`G_BYTE_ORDER`<a name="g-byte-order"></a>

:   The host byte order. This can be either [`G_LITTLE_ENDIAN`](#g-little-endian)
    or [`G_BIG_ENDIAN`](#g-big-endian).

`G_LITTLE_ENDIAN`<a name="g-little-endian"></a>

:   Specifies the little endian byte order.

`G_BIG_ENDIAN`<a name="g-big-endian"></a>

:   Specifies the big endian byte order.

`G_PDP_ENDIAN`<a name="g-pdp-endian"></a>

:   Specifies the PDP endian byte order.

### Signed

`GINT_FROM_BE(value)`<a name="gint-from-be"></a>

:   Converts an `int` value from big-endian to host byte order.

`GINT_FROM_LE(value)`<a name="gint-from-le"></a>

:   Converts an `int` value from little-endian to host byte order.

`GINT_TO_BE(value)`<a name="gint-to-be"></a>

:   Converts an `int` value from host byte order to big-endian.

`GINT_TO_LE(value)`<a name="gint-to-le"></a>

:   Converts an `int` value from host byte order to little-endian.

`GLONG_FROM_BE(value)`<a name="glong-from-be"></a>

:   Converts a `long` value from big-endian to the host byte order.

`GLONG_FROM_LE(value)`<a name="glong-from-le"></a>

:   Converts a `long` value from little-endian to host byte order.

`GLONG_TO_BE(value)`<a name="glong-to-be"></a>

:   Converts a `long` value from host byte order to big-endian.

`GLONG_TO_LE(value)`<a name="glong-to-le"></a>

:   Converts a `long` value from host byte order to little-endian.

`GSSIZE_FROM_BE(value)`<a name="gssize-from-be"></a>

:   Converts a `ssize_t` value from big-endian to host byte order.

`GSSIZE_FROM_LE(value)`<a name="gssize-from-le"></a>

:   Converts a `ssize_t` value from little-endian to host byte order.

`GSSIZE_TO_BE(value)`<a name="gssize-to-be"></a>

:   Converts a `ssize_t` value from host byte order to big-endian.

`GSSIZE_TO_LE(value)`<a name="gssize-to-le"></a>

:   Converts a `ssize_t` value from host byte order to little-endian.

`GINT16_FROM_BE(value)`<a name="gint16-from-be"></a>

:   Converts an `int16_t` value from big-endian to host byte order.

`GINT16_FROM_LE(value)`<a name="gint16-from-le"></a>

:   Converts an `int16_t` value from little-endian to host byte order.

`GINT16_TO_BE(value)`<a name="gint16-to-be"></a>

:   Converts an `int16_t` value from host byte order to big-endian.

`GINT16_TO_LE(value)`<a name="gint16-to-le"></a>

:   Converts an `int16_t` value from host byte order to little-endian.

`GINT32_FROM_BE(value)`<a name="gint32-from-be"></a>

:   Converts an `int32_t` value from big-endian to host byte order.

`GINT32_FROM_LE(value)`<a name="gint32-from-le"></a>

:   Converts an `int32_t` value from little-endian to host byte order.

`GINT32_TO_BE(value)`<a name="gint32-to-be"></a>

:   Converts an `int32_t` value from host byte order to big-endian.

`GINT32_TO_LE(value)`<a name="gint32-to-le"></a>

:   Converts an `int32_t` value from host byte order to little-endian.

`GINT64_FROM_BE(value)`<a name="gint64-from-be"></a>

:   Converts an `int64_t` value from big-endian to host byte order.

`GINT64_FROM_LE(value)`<a name="gint64-from-le"></a>

:   Converts an `int64_t` value from little-endian to host byte order.

`GINT64_TO_BE(value)`<a name="gint64-to-be"></a>

:   Converts an `int64_t` value from host byte order to big-endian.

`GINT64_TO_LE(value)`<a name="gint64-to-le"></a>

:   Converts an `int64_t` value from host byte order to little-endian.

### Unsigned

`GUINT_FROM_BE(value)`<a name="guint-from-be"></a>

:   Converts an `unsigned int` value from big-endian to host byte order.

`GUINT_FROM_LE(value)`<a name="guint-from-le"></a>

:   Converts an `unsigned int` value from little-endian to host byte order.

`GUINT_TO_BE(value)`<a name="guint-to-be"></a>

:   Converts an `unsigned int` value from host byte order to big-endian.

`GUINT_TO_LE(value)`<a name="guint-to-le"></a>

:   Converts an `unsigned int` value from host byte order to little-endian.

`GULONG_FROM_BE(value)`<a name="gulong-from-be"></a>

:   Converts an `unsigned long` value from big-endian to host byte order.

`GULONG_FROM_LE(value)`<a name="gulong-from-le"></a>

:   Converts an `unsigned long` value from little-endian to host byte order.

`GULONG_TO_BE(value)`<a name="gulong-to-be"></a>

:   Converts an `unsigned long` value from host byte order to big-endian.

`GULONG_TO_LE(value)`<a name="gulong-to-le"></a>

:   Converts an `unsigned long` value from host byte order to little-endian.

`GSIZE_FROM_BE(value)`<a name="gsize-from-be"></a>

:   Converts a `size_t` value from big-endian to the host byte order.

`GSIZE_FROM_LE(value)`<a name="gsize-from-le"></a>

:   Converts a `size_t` value from little-endian to host byte order.

`GSIZE_TO_BE(value)`<a name="gsize-to-be"></a>

:   Converts a `size_t` value from host byte order to big-endian.

`GSIZE_TO_LE(value)`<a name="gsize-to-le"></a>

:   Converts a `size_t` value from host byte order to little-endian.

`GUINT16_FROM_BE(value)`<a name="guint16-from-be"></a>

:   Converts a `uint16_t` value from big-endian to host byte order.

`GUINT16_FROM_LE(value)`<a name="guint16-from-le"></a>

:   Converts a `uint16_t` value from little-endian to host byte order.

`GUINT16_TO_BE(value)`<a name="guint16-to-be"></a>

:   Converts a `uint16_t` value from host byte order to big-endian.

`GUINT16_TO_LE(value)`<a name="guint16-to-le"></a>

:   Converts a `uint16_t` value from host byte order to little-endian.

`GUINT32_FROM_BE(value)`<a name="guint32-from-be"></a>

:   Converts a `uint32_t` value from big-endian to host byte order.

`GUINT32_FROM_LE(value)`<a name="guint32-from-le"></a>

:   Converts a `uint32_t` value from little-endian to host byte order.

`GUINT32_TO_BE(value)`<a name="guint32-to-be"></a>

:   Converts a `uint32_t` value from host byte order to big-endian.

`GUINT32_TO_LE(value)`<a name="guint32-to-le"></a>

:   Converts a `uint32_t` value from host byte order to little-endian.

`GUINT64_FROM_BE(value)`<a name="guint64-from-be"></a>

:   Converts a `uint64_t` value from big-endian to host byte order.

`GUINT64_FROM_LE(value)`<a name="guint64-from-le"></a>

:   Converts a `uint64_t` value from little-endian to host byte order.

`GUINT64_TO_BE(value)`<a name="guint64-to-be"></a>

:   Converts a `uint64_t` value from host byte order to big-endian.

`GUINT64_TO_LE(value)`<a name="guint64-to-le"></a>

:   Converts a `uint64_t` value from host byte order to little-endian.

`GUINT16_SWAP_BE_PDP(value)`<a name="guint16-swap-be-pdp"></a>

:   Converts a `uint16_t` value between big-endian and pdp-endian byte order.
    The conversion is symmetric so it can be used both ways.

`GUINT16_SWAP_LE_BE(value)`<a name="guint16-swap-le-be"></a>

:   Converts a `uint16_t` value between little-endian and big-endian byte order.
    The conversion is symmetric so it can be used both ways.

`GUINT16_SWAP_LE_PDP(value)`<a name="guint16-swap-le-pdp"></a>

:   Converts a `uint16_t` value between little-endian and pdp-endian byte order.
    The conversion is symmetric so it can be used both ways.

`GUINT32_SWAP_BE_PDP(value)`<a name="guint32-swap-be-pdp"></a>

:   Converts a `uint32_t` value between big-endian and pdp-endian byte order.
    The conversion is symmetric so it can be used both ways.

`GUINT32_SWAP_LE_BE(value)`<a name="guint32-swap-le-be"></a>

:   Converts a `uint32_t` value between little-endian and big-endian byte order.
    The conversion is symmetric so it can be used both ways.

`GUINT32_SWAP_LE_PDP(value)`<a name="guint32-swap-le-pdp"></a>

:   Converts a `uint32_t` value between little-endian and pdp-endian byte order.
    The conversion is symmetric so it can be used both ways.

`GUINT64_SWAP_LE_BE(value)`<a name="guint64-swap-le-be"></a>

:   Converts a `uint64_t` value between little-endian and big-endian byte order.
    The conversion is symmetric so it can be used both ways.
