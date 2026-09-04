Title: Message Logging

# Message Logging

The `g_return` family of macros (`g_return_if_fail()`,
`g_return_val_if_fail()`, `g_return_if_reached()`,
`g_return_val_if_reached()`) should only be used for programming errors, a
typical use case is checking for invalid parameters at the beginning of a
public function. They should not be used if you just mean "if (error)
return", they should only be used if you mean "if (bug in program) return".
The program behavior is generally considered undefined after one of these
checks fails. They are not intended for normal control flow, only to give a
perhaps-helpful warning before giving up.

Structured logging output is supported using `g_log_structured()`. This
differs from the traditional `g_log()` API in that log messages are handled
as a collection of key–value pairs representing individual pieces of
information, rather than as a single string containing all the information
in an arbitrary format.

The convenience macros `g_info()`, `g_message()`, `g_debug()`, `g_warning()`
and `g_error()` will use the traditional `g_log()` API unless you define the
symbol `G_LOG_USE_STRUCTURED` before including `glib.h`. But note that even
messages logged through the traditional `g_log()` API are ultimatively
passed to `g_log_structured()`, so that all log messages end up in same
destination. If `G_LOG_USE_STRUCTURED` is defined, `g_test_expect_message()`
will become ineffective for the wrapper macros `g_warning()` and friends
(see [Testing for Messages](#testing-for-messages).)

The support for structured logging was motivated by the following needs
(some of which were supported previously; others weren’t):

- Support for multiple logging levels.
- Structured log support with the ability to add `MESSAGE_ID`s (see
  `g_log_structured()`).
- Moving the responsibility for filtering log messages from the program to
  the log viewer — instead of libraries and programs installing log handlers
  (with `g_log_set_handler()`) which filter messages before output, all log
  messages are outputted, and the log viewer program (such as
  [`journalctl`](https://www.freedesktop.org/software/systemd/man/journalctl.html))
  must filter them. This is based on the idea that bugs are sometimes hard
  to reproduce, so it is better to log everything possible and then use
  tools to analyse the logs than it is to not be able to reproduce a bug to
  get additional log data. Code which uses logging in performance-critical
  sections should compile out the `g_log_structured()` calls in release
  builds, and compile them in in debugging builds.
- A single writer function which handles all log messages in a process, from
  all libraries and program code; rather than multiple log handlers with
  poorly defined interactions between them. This allows a program to easily
  change its logging policy by changing the writer function, for example to
  log to an additional location or to change what logging output fallbacks
  are used. The log writer functions provided by GLib are exposed publicly
  so they can be used from programs’ log writers. This allows log writer
  policy and implementation to be kept separate.
- If a library wants to add standard information to all of its log messages
  (such as library state) or to redact private data (such as passwords or
  network credentials), it should use a wrapper function around its
  `g_log_structured()` calls or implement that in the single log writer
  function.
- If a program wants to pass context data from a `g_log_structured()` call
  to its log writer function so that, for example, it can use the correct
  server connection to submit logs to, that user data can be passed as a
  zero-length `GLogField` to `g_log_structured_array()`.
- Color output needed to be supported on the terminal, to make reading
  through logs easier.

## Using Structured Logging

To use structured logging (rather than the old-style logging), either use
the `g_log_structured()` and `g_log_structured_array()` functions; or define
`G_LOG_USE_STRUCTURED` before including any GLib header, and use the
`g_message()`, `g_debug()`, `g_error()` (etc.) macros.

You do not need to define `G_LOG_USE_STRUCTURED` to use
`g_log_structured()`, but it is a good idea to avoid confusion.

## Log Domains

Log domains may be used to broadly split up the origins of log messages.
Typically, there are one or a few log domains per application or library.
`G_LOG_DOMAIN` should be used to define the default log domain for the current
compilation unit — it is typically defined at the top of a source file, or
in the preprocessor flags for a group of source files.

Log domains must be unique, and it is recommended that they are the
application or library name, optionally followed by a hyphen and a
sub-domain name. For example, `bloatpad` or `bloatpad-io`.

## Debug Message Output

The default log functions (`g_log_default_handler()` for the old-style API
and `g_log_writer_default()` for the structured API) both drop debug and
informational messages by default, unless the log domains of those messages
are listed in the `G_MESSAGES_DEBUG` environment variable (or it is set to
`all`), or `DEBUG_INVOCATION=1` is set in the environment.

It is recommended that custom log writer functions re-use the
`G_MESSAGES_DEBUG` and `DEBUG_INVOCATION` environment variables, rather than
inventing a custom one, so that developers can re-use the same debugging
techniques and tools across projects. Since GLib 2.68, this can be implemented
by dropping messages for which `g_log_writer_default_would_drop()` returns
`TRUE`.

## Testing for Messages

With the old `g_log()` API, `g_test_expect_message()` and
`g_test_assert_expected_messages()` could be used in simple cases to check
whether some code under test had emitted a given log message. These
functions have been deprecated with the structured logging API, for several
reasons:

- They relied on an internal queue which was too inflexible for many use
  cases, where messages might be emitted in several orders, some messages
  might not be emitted deterministically, or messages might be emitted by
  unrelated log domains.
- They do not support structured log fields.
- Examining the log output of code is a bad approach to testing it, and
  while it might be necessary for legacy code which uses `g_log()`, it
  should be avoided for new code using `g_log_structured()`.

They will continue to work as before if `g_log()` is in use (and
`G_LOG_USE_STRUCTURED` is not defined). They will do nothing if used with
the structured logging API.

Examining the log output of code is discouraged: libraries should not emit
to stderr during defined behaviour, and hence this should not be tested. If
the log emissions of a library during undefined behaviour need to be tested,
they should be limited to asserting that the library aborts and prints a
suitable error message before aborting. This should be done with
`g_test_trap_assert_stderr()`.

If it is really necessary to test the structured log messages emitted by a
particular piece of code – and the code cannot be restructured to be more
suitable to more conventional unit testing – you should write a custom log
writer function (see `g_log_set_writer_func()`) which appends all log
messages to a queue. When you want to check the log messages, examine and
clear the queue, ignoring irrelevant log messages (for example, from log
domains other than the one under test).
