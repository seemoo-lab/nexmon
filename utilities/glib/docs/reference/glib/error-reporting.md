Title: Error Reporting

# Error Reporting

GLib provides a standard method of reporting errors from a called function
to the calling code. (This is the same problem solved by exceptions in other
languages.) It's important to understand that this method is both a data
type (the [`type@GLib.Error`] struct) and a set of rules. If you use
`GError` incorrectly, then your code will not properly interoperate with
other code that uses `GError`, and users of your API will probably get
confused. In most cases, using `GError` is preferred over numeric error
codes, but there are situations where numeric error codes are useful for
performance.

First and foremost: `GError` should only be used to report recoverable
runtime errors, never to report programming errors. If the programmer has
screwed up, then you should use `g_warning()`, `g_return_if_fail()`,
`g_assert()`, `g_error()`, or some similar facility. (Incidentally, remember
that the `g_error()` function should only be used for programming errors, it
should not be used to print any error reportable via `GError`.)

Examples of recoverable runtime errors are "file not found" or "failed to
parse input." Examples of programming errors are "NULL passed to `strcmp()`"
or "attempted to free the same pointer twice." These two kinds of errors are
fundamentally different: runtime errors should be handled or reported to the
user, programming errors should be eliminated by fixing the bug in the
program. This is why most functions in GLib and GTK do not use the `GError`
facility.

Functions that can fail take a return location for a `GError` as their last
argument. On error, a new `GError` instance will be allocated and returned
to the caller via this argument. For example:

```c
gboolean g_file_get_contents (const char  *filename,
                              char       **contents,
                              gsize       *length,
                              GError     **error);
```

If you pass a non-`NULL` value for the `error` argument, it should
point to a location where an error can be placed. For example:

```c
char *contents;
GError *err = NULL;

g_file_get_contents ("foo.txt", &contents, NULL, &err);
g_assert ((contents == NULL && err != NULL) || (contents != NULL && err == NULL));
if (err != NULL)
  {
    // Report error to user, and free error
    g_assert (contents == NULL);
    fprintf (stderr, "Unable to read file: %s\n", err->message);
    g_error_free (err);
  }
else
  {
    // Use file contents
    g_assert (contents != NULL);
  }
```

Note that `err != NULL` in this example is a reliable indicator of whether
`g_file_get_contents()` failed. Additionally, `g_file_get_contents()`
returns a boolean which indicates whether it was successful.

Because `g_file_get_contents()` returns `FALSE` on failure, if you
are only interested in whether it failed and don't need to display
an error message, you can pass `NULL` for the `error` argument:

```c
if (g_file_get_contents ("foo.txt", &contents, NULL, NULL)) // ignore errors
  // no error occurred
  ;
else
  // error
  ;
```

The `GError` object contains three fields: `domain` indicates the module the
error-reporting function is located in, `code` indicates the specific error
that occurred, and `message` is a user-readable error message with as many
details as possible. Several functions are provided to deal with an error
received from a called function: `g_error_matches()` returns `TRUE` if the
error matches a given domain and code, `g_propagate_error()` copies an error
into an error location (so the calling function will receive it), and
`g_clear_error()` clears an error location by freeing the error and
resetting the location to `NULL`. To display an error to the user, simply
display the `message`, perhaps along with additional context known only to
the calling function (the file being opened, or whatever - though in the
`g_file_get_contents()` case, the `message` already contains a filename).

Since error messages may be displayed to the user, they need to be valid
UTF-8 (all GTK widgets expect text to be UTF-8). Keep this in mind in
particular when formatting error messages with filenames, which are in the
'filename encoding', and need to be turned into UTF-8 using
`g_filename_to_utf8()`, `g_filename_display_name()` or
`g_utf8_make_valid()`.

Note, however, that many error messages are too technical to display to the
user in an application, so prefer to use `g_error_matches()` to categorize
errors from called functions, and build an appropriate error message for the
context within your application. Error messages from a `GError` are more
appropriate to be printed in system logs or on the command line. They are
typically translated.

## Reporting errors

When implementing a function that can report errors, the basic tool is
`g_set_error()`. Typically, if a fatal error occurs you want to
`g_set_error()`, then return immediately. `g_set_error()` does nothing if
the error location passed to it is `NULL`.  Here's an example:

```c
int
foo_open_file (GError **error)
{
  int fd;
  int saved_errno;

  g_return_val_if_fail (error == NULL || *error == NULL, -1);

  fd = open ("file.txt", O_RDONLY);
  saved_errno = errno;

  if (fd < 0)
    {
      g_set_error (error,
                   FOO_ERROR,                 // error domain
                   FOO_ERROR_BLAH,            // error code
                   "Failed to open file: %s", // error message format string
                   g_strerror (saved_errno));
      return -1;
    }
  else
    return fd;
}
```

Things are somewhat more complicated if you yourself call another function
that can report a `GError`. If the sub-function indicates fatal errors in
some way other than reporting a `GError`, such as by returning `TRUE` on
success, you can simply do the following:

```c
gboolean
my_function_that_can_fail (GError **err)
{
  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);

  if (!sub_function_that_can_fail (err))
    {
      // assert that error was set by the sub-function
      g_assert (err == NULL || *err != NULL);
      return FALSE;
    }

  // otherwise continue, no error occurred
  g_assert (err == NULL || *err == NULL);
}
```

If the sub-function does not indicate errors other than by reporting a
`GError` (or if its return value does not reliably indicate errors) you need
to create a temporary `GError` since the passed-in one may be `NULL`.
`g_propagate_error()` is intended for use in this case.

```c
gboolean
my_function_that_can_fail (GError **err)
{
  GError *tmp_error;

  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);

  tmp_error = NULL;
  sub_function_that_can_fail (&tmp_error);

  if (tmp_error != NULL)
    {
      // store tmp_error in err, if err != NULL,
      // otherwise call g_error_free() on tmp_error
      g_propagate_error (err, tmp_error);
      return FALSE;
    }

  // otherwise continue, no error occurred
}
```

Error pileups are always a bug. For example, this code is incorrect:

```c
gboolean
my_function_that_can_fail (GError **err)
{
  GError *tmp_error;

  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);

  tmp_error = NULL;
  sub_function_that_can_fail (&tmp_error);
  other_function_that_can_fail (&tmp_error);

  if (tmp_error != NULL)
    {
      g_propagate_error (err, tmp_error);
      return FALSE;
    }
}
```

`tmp_error` should be checked immediately after
`sub_function_that_can_fail()`, and either cleared or propagated upward. The
rule is: after each error, you must either handle the error, or return it to
the calling function.

Note that passing `NULL` for the error location is the equivalent of
handling an error by always doing nothing about it. So the following code is
fine, assuming errors in `sub_function_that_can_fail()` are not fatal to
`my_function_that_can_fail()`:

```c
gboolean
my_function_that_can_fail (GError **err)
{
  GError *tmp_error;

  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);

  sub_function_that_can_fail (NULL); // ignore errors

  tmp_error = NULL;
  other_function_that_can_fail (&tmp_error);

  if (tmp_error != NULL)
    {
      g_propagate_error (err, tmp_error);
      return FALSE;
    }
}
```

Note that passing `NULL` for the error location ignores errors; it's
equivalent to:

```cpp
try { sub_function_that_can_fail (); } catch (...) {}
```

in C++. It does not mean to leave errors unhandled; it means to handle them
by doing nothing.

## Error domains

Error domains and codes are conventionally named as follows:

- The error domain is called `<NAMESPACE>_<MODULE>_ERROR`, for example
  `G_SPAWN_ERROR` or `G_THREAD_ERROR`:
  ```c
  #define G_SPAWN_ERROR g_spawn_error_quark ()

  G_DEFINE_QUARK (g-spawn-error-quark, g_spawn_error)
  ```

- The quark function for the error domain is called
  `<namespace>_<module>_error_quark`, for example `g_spawn_error_quark()` or
  `g_thread_error_quark()`.

- The error codes are in an enumeration called `<Namespace><Module>Error`;
  for example, `GThreadError` or `GSpawnError`.

- Members of the error code enumeration are called
  `<NAMESPACE>_<MODULE>_ERROR_<CODE>`, for example `G_SPAWN_ERROR_FORK` or
  `G_THREAD_ERROR_AGAIN`.

- If there's a "generic" or "unknown" error code for unrecoverable errors it
  doesn't make sense to distinguish with specific codes, it should be called
  `<NAMESPACE>_<MODULE>_ERROR_FAILED`, for example `G_SPAWN_ERROR_FAILED`.
  In the case of error code enumerations that may be extended in future
  releases, you should generally not handle this error code explicitly, but
  should instead treat any unrecognized error code as equivalent to
  `FAILED`.

## Comparison of `GError` and traditional error handling

`GError` has several advantages over traditional numeric error codes:
importantly, tools like [gobject-introspection](https://gi.readthedocs.org)
understand `GError`s and convert them to exceptions in bindings; the message
includes more information than just a code; and use of a domain helps
prevent misinterpretation of error codes.

`GError` has disadvantages though: it requires a memory allocation, and
formatting the error message string has a performance overhead. This makes
it unsuitable for use in retry loops where errors are a common case, rather
than being unusual. For example, using `G_IO_ERROR_WOULD_BLOCK` means
hitting these overheads in the normal control flow. String formatting
overhead can be eliminated by using `g_set_error_literal()` in some cases.

These performance issues can be compounded if a function wraps the `GError`s
returned by the functions it calls: this multiplies the number of
allocations and string formatting operations. This can be partially
mitigated by using `g_prefix_error()`.

## Rules for use of `GError`

Summary of rules for use of `GError`:

- Do not report programming errors via `GError`.

- The last argument of a function that returns an error should be a location
  where a `GError` can be placed (i.e. `GError **error`).  If `GError` is
  used with varargs, the `GError**` should be the last argument before the
  `...`.

- The caller may pass `NULL` for the `GError**` if they are not interested
  in details of the exact error that occurred.

- If `NULL` is passed for the `GError**` argument, then errors should not be
  returned to the caller, but your function should still abort and return if
  an error occurs. That is, control flow should not be affected by whether
  the caller wants to get a `GError`.

- If a `GError` is reported, then your function by definition had a fatal
  failure and did not complete whatever it was supposed to do.  If the
  failure was not fatal, then you handled it and you should not report it.
  If it was fatal, then you must report it and discontinue whatever you were
  doing immediately.

- If a `GError` is reported, out parameters are not guaranteed to be set to
  any defined value.

- A `GError*` must be initialized to `NULL` before passing its address to a
  function that can report errors.

- `GError` structs must not be stack-allocated.

- "Piling up" errors is always a bug. That is, if you assign a new `GError`
  to a `GError*` that is non-`NULL`, thus overwriting the previous error, it
  indicates that you should have aborted the operation instead of
  continuing. If you were able to continue, you should have cleared the
  previous error with `g_clear_error()`.  `g_set_error()` will complain if
  you pile up errors.

- By convention, if you return a boolean value indicating success then
  `TRUE` means success and `FALSE` means failure. Avoid creating functions
  which have a boolean return value and a `GError` parameter, but where the
  boolean does something other than signal whether the `GError` is set.
  Among other problems, it requires C callers to allocate a temporary error.
  Instead, provide a `gboolean *` out parameter.  There are functions in
  GLib itself such as `g_key_file_has_key()` that are hard to use because of
  this. If `FALSE` is returned, the error must be set to a non-`NULL` value.
  One exception to this is that in situations that are already considered to
  be undefined behaviour (such as when a `g_return_val_if_fail()` check
  fails), the error need not be set.  Instead of checking separately whether
  the error is set, callers should ensure that they do not provoke undefined
  behaviour, then assume that the error will be set on failure.

- A `NULL` return value is also frequently used to mean that an error
  occurred. You should make clear in your documentation whether `NULL` is a
  valid return value in non-error cases; if `NULL` is a valid value, then
  users must check whether an error was returned to see if the function
  succeeded.

- When implementing a function that can report errors, you may want
  to add a check at the top of your function that the error return
  location is either `NULL` or contains a `NULL` error (e.g.
  `g_return_if_fail (error == NULL || *error == NULL);`).

## Extended `GError` Domains

Since GLib 2.68 it is possible to extend the `GError` type. This is
done with the `G_DEFINE_EXTENDED_ERROR()` macro. To create an
extended `GError` type do something like this in the header file:

```c
typedef enum
{
  MY_ERROR_BAD_REQUEST,
} MyError;
#define MY_ERROR (my_error_quark ())
GQuark my_error_quark (void);
int
my_error_get_parse_error_id (GError *error);
const char *
my_error_get_bad_request_details (GError *error);
```

and in the implementation:

```c
typedef struct
{
  int parse_error_id;
  char *bad_request_details;
} MyErrorPrivate;

static void
my_error_private_init (MyErrorPrivate *priv)
{
  priv->parse_error_id = -1;
  // No need to set priv->bad_request_details to NULL,
  // the struct is initialized with zeros.
}

static void
my_error_private_copy (const MyErrorPrivate *src_priv, MyErrorPrivate *dest_priv)
{
  dest_priv->parse_error_id = src_priv->parse_error_id;
  dest_priv->bad_request_details = g_strdup (src_priv->bad_request_details);
}

static void
my_error_private_clear (MyErrorPrivate *priv)
{
  g_free (priv->bad_request_details);
}

// This defines the my_error_get_private and my_error_quark functions.
G_DEFINE_EXTENDED_ERROR (MyError, my_error)

int
my_error_get_parse_error_id (GError *error)
{
  MyErrorPrivate *priv = my_error_get_private (error);
  g_return_val_if_fail (priv != NULL, -1);
  return priv->parse_error_id;
}

const char *
my_error_get_bad_request_details (GError *error)
{
  MyErrorPrivate *priv = my_error_get_private (error);
  g_return_val_if_fail (priv != NULL, NULL);
  g_return_val_if_fail (error->code != MY_ERROR_BAD_REQUEST, NULL);
  return priv->bad_request_details;
}

static void
my_error_set_bad_request (GError     **error,
                          const char  *reason,
                          int          error_id,
                          const char  *details)
{
  MyErrorPrivate *priv;
  g_set_error (error, MY_ERROR, MY_ERROR_BAD_REQUEST, "Invalid request: %s", reason);
  if (error != NULL && *error != NULL)
    {
      priv = my_error_get_private (error);
      g_return_val_if_fail (priv != NULL, NULL);
      priv->parse_error_id = error_id;
      priv->bad_request_details = g_strdup (details);
    }
}
```

An example of use of the error could be:

```c
gboolean
send_request (GBytes *request, GError **error)
{
  ParseFailedStatus *failure = validate_request (request);
  if (failure != NULL)
    {
      my_error_set_bad_request (error, failure->reason, failure->error_id, failure->details);
      parse_failed_status_free (failure);
      return FALSE;
    }

  return send_one (request, error);
}
```

Please note that if you are a library author and your library exposes an
existing error domain, then you can't make this error domain an extended one
without breaking ABI. This is because earlier it was possible to create an
error with this error domain on the stack and then copy it with
`g_error_copy()`. If the new version of your library makes the error domain
an extended one, then `g_error_copy()` called by code that allocated the
error on the stack will try to copy more data than it used to, which will
lead to undefined behavior. You must not stack-allocate errors with an
extended error domain, and it is bad practice to stack-allocate any other
`GError`s.

Extended error domains in unloadable plugins/modules are not supported.
