/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * MT safe
 */

/**
 * SECTION:warnings
 * @Title: Message Output and Debugging Functions
 * @Short_description: functions to output messages and help debug applications
 *
 * These functions provide support for outputting messages.
 *
 * The g_return family of macros (g_return_if_fail(),
 * g_return_val_if_fail(), g_return_if_reached(),
 * g_return_val_if_reached()) should only be used for programming
 * errors, a typical use case is checking for invalid parameters at
 * the beginning of a public function. They should not be used if
 * you just mean "if (error) return", they should only be used if
 * you mean "if (bug in program) return". The program behavior is
 * generally considered undefined after one of these checks fails.
 * They are not intended for normal control flow, only to give a
 * perhaps-helpful warning before giving up.
 */

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#include <errno.h>

#include "glib-init.h"
#include "gbacktrace.h"
#include "gcharset.h"
#include "gconvert.h"
#include "genviron.h"
#include "gmem.h"
#include "gprintfint.h"
#include "gtestutils.h"
#include "gthread.h"
#include "gstrfuncs.h"
#include "gstring.h"
#include "gpattern.h"

#ifdef G_OS_UNIX
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#include <process.h>		/* For getpid() */
#include <io.h>
#  define _WIN32_WINDOWS 0x0401 /* to get IsDebuggerPresent */
#  include <windows.h>
#endif


/**
 * SECTION:messages
 * @title: Message Logging
 * @short_description: versatile support for logging messages
 *     with different levels of importance
 *
 * These functions provide support for logging error messages
 * or messages used for debugging.
 *
 * There are several built-in levels of messages, defined in
 * #GLogLevelFlags. These can be extended with user-defined levels.
 */

/**
 * G_LOG_DOMAIN:
 *
 * Defines the log domain.
 *
 * For applications, this is typically left as the default %NULL
 * (or "") domain. Libraries should define this so that any messages
 * which they log can be differentiated from messages from other
 * libraries and application code. But be careful not to define
 * it in any public header files.
 *
 * For example, GTK+ uses this in its Makefile.am:
 * |[
 * AM_CPPFLAGS = -DG_LOG_DOMAIN=\"Gtk\"
 * ]|
 */

/**
 * G_LOG_FATAL_MASK:
 *
 * GLib log levels that are considered fatal by default.
 */

/**
 * GLogFunc:
 * @log_domain: the log domain of the message
 * @log_level: the log level of the message (including the
 *     fatal and recursion flags)
 * @message: the message to process
 * @user_data: user data, set in g_log_set_handler()
 *
 * Specifies the prototype of log handler functions.
 *
 * The default log handler, g_log_default_handler(), automatically appends a
 * new-line character to @message when printing it. It is advised that any
 * custom log handler functions behave similarly, so that logging calls in user
 * code do not need modifying to add a new-line character to the message if the
 * log handler is changed.
 */

/**
 * GLogLevelFlags:
 * @G_LOG_FLAG_RECURSION: internal flag
 * @G_LOG_FLAG_FATAL: internal flag
 * @G_LOG_LEVEL_ERROR: log level for errors, see g_error().
 *     This level is also used for messages produced by g_assert().
 * @G_LOG_LEVEL_CRITICAL: log level for critical warning messages, see
 *     g_critical().
 *     This level is also used for messages produced by g_return_if_fail()
 *     and g_return_val_if_fail().
 * @G_LOG_LEVEL_WARNING: log level for warnings, see g_warning()
 * @G_LOG_LEVEL_MESSAGE: log level for messages, see g_message()
 * @G_LOG_LEVEL_INFO: log level for informational messages, see g_info()
 * @G_LOG_LEVEL_DEBUG: log level for debug messages, see g_debug()
 * @G_LOG_LEVEL_MASK: a mask including all log levels
 *
 * Flags specifying the level of log messages.
 *
 * It is possible to change how GLib treats messages of the various
 * levels using g_log_set_handler() and g_log_set_fatal_mask().
 */

/**
 * G_LOG_LEVEL_USER_SHIFT:
 *
 * Log levels below 1<<G_LOG_LEVEL_USER_SHIFT are used by GLib.
 * Higher bits can be used for user-defined log levels.
 */

/**
 * g_message:
 * @...: format string, followed by parameters to insert
 *     into the format string (as with printf())
 *
 * A convenience function/macro to log a normal message.
 *
 * If g_log_default_handler() is used as the log handler function, a new-line
 * character will automatically be appended to @..., and need not be entered
 * manually.
 */

/**
 * g_warning:
 * @...: format string, followed by parameters to insert
 *     into the format string (as with printf())
 *
 * A convenience function/macro to log a warning message.
 *
 * This is not intended for end user error reporting. Use of #GError is
 * preferred for that instead, as it allows calling functions to perform actions
 * conditional on the type of error.
 *
 * You can make warnings fatal at runtime by setting the `G_DEBUG`
 * environment variable (see
 * [Running GLib Applications](glib-running.html)).
 *
 * If g_log_default_handler() is used as the log handler function,
 * a newline character will automatically be appended to @..., and
 * need not be entered manually.
 */

/**
 * g_critical:
 * @...: format string, followed by parameters to insert
 *     into the format string (as with printf())
 *
 * Logs a "critical warning" (#G_LOG_LEVEL_CRITICAL).
 * It's more or less application-defined what constitutes
 * a critical vs. a regular warning. You could call
 * g_log_set_always_fatal() to make critical warnings exit
 * the program, then use g_critical() for fatal errors, for
 * example.
 *
 * You can also make critical warnings fatal at runtime by
 * setting the `G_DEBUG` environment variable (see
 * [Running GLib Applications](glib-running.html)).
 *
 * If g_log_default_handler() is used as the log handler function, a new-line
 * character will automatically be appended to @..., and need not be entered
 * manually.
 */

/**
 * g_error:
 * @...: format string, followed by parameters to insert
 *     into the format string (as with printf())
 *
 * A convenience function/macro to log an error message.
 *
 * This is not intended for end user error reporting. Use of #GError is
 * preferred for that instead, as it allows calling functions to perform actions
 * conditional on the type of error.
 *
 * Error messages are always fatal, resulting in a call to
 * abort() to terminate the application. This function will
 * result in a core dump; don't use it for errors you expect.
 * Using this function indicates a bug in your program, i.e.
 * an assertion failure.
 *
 * If g_log_default_handler() is used as the log handler function, a new-line
 * character will automatically be appended to @..., and need not be entered
 * manually.
 *
 */

/**
 * g_info:
 * @...: format string, followed by parameters to insert
 *     into the format string (as with printf())
 *
 * A convenience function/macro to log an informational message. Seldom used.
 *
 * If g_log_default_handler() is used as the log handler function, a new-line
 * character will automatically be appended to @..., and need not be entered
 * manually.
 *
 * Such messages are suppressed by the g_log_default_handler() unless
 * the G_MESSAGES_DEBUG environment variable is set appropriately.
 *
 * Since: 2.40
 */

/**
 * g_debug:
 * @...: format string, followed by parameters to insert
 *     into the format string (as with printf())
 *
 * A convenience function/macro to log a debug message.
 *
 * If g_log_default_handler() is used as the log handler function, a new-line
 * character will automatically be appended to @..., and need not be entered
 * manually.
 *
 * Such messages are suppressed by the g_log_default_handler() unless
 * the G_MESSAGES_DEBUG environment variable is set appropriately.
 *
 * Since: 2.6
 */

/* --- structures --- */
typedef struct _GLogDomain	GLogDomain;
typedef struct _GLogHandler	GLogHandler;
struct _GLogDomain
{
  gchar		*log_domain;
  GLogLevelFlags fatal_mask;
  GLogHandler	*handlers;
  GLogDomain	*next;
};
struct _GLogHandler
{
  guint		 id;
  GLogLevelFlags log_level;
  GLogFunc	 log_func;
  gpointer	 data;
  GDestroyNotify destroy;
  GLogHandler	*next;
};


/* --- variables --- */
static GMutex         g_messages_lock;
static GLogDomain    *g_log_domains = NULL;
static GPrintFunc     glib_print_func = NULL;
static GPrintFunc     glib_printerr_func = NULL;
static GPrivate       g_log_depth;
static GLogFunc       default_log_func = g_log_default_handler;
static gpointer       default_log_data = NULL;
static GTestLogFatalFunc fatal_log_func = NULL;
static gpointer          fatal_log_data;

/* --- functions --- */

static void _g_log_abort (gboolean breakpoint);

static void
_g_log_abort (gboolean breakpoint)
{
  if (g_test_subprocess ())
    {
      /* If this is a test case subprocess then it probably caused
       * this error message on purpose, so just exit() rather than
       * abort()ing, to avoid triggering any system crash-reporting
       * daemon.
       */
      _exit (1);
    }

  if (breakpoint)
    G_BREAKPOINT ();
  else
    abort ();
}

#ifdef G_OS_WIN32
#  include <windows.h>
static gboolean win32_keep_fatal_message = FALSE;

/* This default message will usually be overwritten. */
/* Yes, a fixed size buffer is bad. So sue me. But g_error() is never
 * called with huge strings, is it?
 */
static gchar  fatal_msg_buf[1000] = "Unspecified fatal error encountered, aborting.";
static gchar *fatal_msg_ptr = fatal_msg_buf;

#undef write
static inline int
dowrite (int          fd,
	 const void  *buf,
	 unsigned int len)
{
  if (win32_keep_fatal_message)
    {
      memcpy (fatal_msg_ptr, buf, len);
      fatal_msg_ptr += len;
      *fatal_msg_ptr = 0;
      return len;
    }

  write (fd, buf, len);

  return len;
}
#define write(fd, buf, len) dowrite(fd, buf, len)

#endif

static void
write_string (FILE        *stream,
	      const gchar *string)
{
  fputs (string, stream);
}

static GLogDomain*
g_log_find_domain_L (const gchar *log_domain)
{
  GLogDomain *domain;
  
  domain = g_log_domains;
  while (domain)
    {
      if (strcmp (domain->log_domain, log_domain) == 0)
	return domain;
      domain = domain->next;
    }
  return NULL;
}

static GLogDomain*
g_log_domain_new_L (const gchar *log_domain)
{
  GLogDomain *domain;

  domain = g_new (GLogDomain, 1);
  domain->log_domain = g_strdup (log_domain);
  domain->fatal_mask = G_LOG_FATAL_MASK;
  domain->handlers = NULL;
  
  domain->next = g_log_domains;
  g_log_domains = domain;
  
  return domain;
}

static void
g_log_domain_check_free_L (GLogDomain *domain)
{
  if (domain->fatal_mask == G_LOG_FATAL_MASK &&
      domain->handlers == NULL)
    {
      GLogDomain *last, *work;
      
      last = NULL;  

      work = g_log_domains;
      while (work)
	{
	  if (work == domain)
	    {
	      if (last)
		last->next = domain->next;
	      else
		g_log_domains = domain->next;
	      g_free (domain->log_domain);
	      g_free (domain);
	      break;
	    }
	  last = work;
	  work = last->next;
	}  
    }
}

static GLogFunc
g_log_domain_get_handler_L (GLogDomain	*domain,
			    GLogLevelFlags log_level,
			    gpointer	*data)
{
  if (domain && log_level)
    {
      GLogHandler *handler;
      
      handler = domain->handlers;
      while (handler)
	{
	  if ((handler->log_level & log_level) == log_level)
	    {
	      *data = handler->data;
	      return handler->log_func;
	    }
	  handler = handler->next;
	}
    }

  *data = default_log_data;
  return default_log_func;
}

/**
 * g_log_set_always_fatal:
 * @fatal_mask: the mask containing bits set for each level
 *     of error which is to be fatal
 *
 * Sets the message levels which are always fatal, in any log domain.
 * When a message with any of these levels is logged the program terminates.
 * You can only set the levels defined by GLib to be fatal.
 * %G_LOG_LEVEL_ERROR is always fatal.
 *
 * You can also make some message levels fatal at runtime by setting
 * the `G_DEBUG` environment variable (see
 * [Running GLib Applications](glib-running.html)).
 *
 * Returns: the old fatal mask
 */
GLogLevelFlags
g_log_set_always_fatal (GLogLevelFlags fatal_mask)
{
  GLogLevelFlags old_mask;

  /* restrict the global mask to levels that are known to glib
   * since this setting applies to all domains
   */
  fatal_mask &= (1 << G_LOG_LEVEL_USER_SHIFT) - 1;
  /* force errors to be fatal */
  fatal_mask |= G_LOG_LEVEL_ERROR;
  /* remove bogus flag */
  fatal_mask &= ~G_LOG_FLAG_FATAL;

  g_mutex_lock (&g_messages_lock);
  old_mask = g_log_always_fatal;
  g_log_always_fatal = fatal_mask;
  g_mutex_unlock (&g_messages_lock);

  return old_mask;
}

/**
 * g_log_set_fatal_mask:
 * @log_domain: the log domain
 * @fatal_mask: the new fatal mask
 *
 * Sets the log levels which are fatal in the given domain.
 * %G_LOG_LEVEL_ERROR is always fatal.
 *
 * Returns: the old fatal mask for the log domain
 */
GLogLevelFlags
g_log_set_fatal_mask (const gchar   *log_domain,
		      GLogLevelFlags fatal_mask)
{
  GLogLevelFlags old_flags;
  GLogDomain *domain;
  
  if (!log_domain)
    log_domain = "";
  
  /* force errors to be fatal */
  fatal_mask |= G_LOG_LEVEL_ERROR;
  /* remove bogus flag */
  fatal_mask &= ~G_LOG_FLAG_FATAL;
  
  g_mutex_lock (&g_messages_lock);

  domain = g_log_find_domain_L (log_domain);
  if (!domain)
    domain = g_log_domain_new_L (log_domain);
  old_flags = domain->fatal_mask;
  
  domain->fatal_mask = fatal_mask;
  g_log_domain_check_free_L (domain);

  g_mutex_unlock (&g_messages_lock);

  return old_flags;
}

/**
 * g_log_set_handler:
 * @log_domain: (allow-none): the log domain, or %NULL for the default ""
 *     application domain
 * @log_levels: the log levels to apply the log handler for.
 *     To handle fatal and recursive messages as well, combine
 *     the log levels with the #G_LOG_FLAG_FATAL and
 *     #G_LOG_FLAG_RECURSION bit flags.
 * @log_func: the log handler function
 * @user_data: data passed to the log handler
 *
 * Sets the log handler for a domain and a set of log levels.
 * To handle fatal and recursive messages the @log_levels parameter
 * must be combined with the #G_LOG_FLAG_FATAL and #G_LOG_FLAG_RECURSION
 * bit flags.
 *
 * Note that since the #G_LOG_LEVEL_ERROR log level is always fatal, if
 * you want to set a handler for this log level you must combine it with
 * #G_LOG_FLAG_FATAL.
 *
 * Here is an example for adding a log handler for all warning messages
 * in the default domain:
 * |[<!-- language="C" --> 
 * g_log_set_handler (NULL, G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL
 *                    | G_LOG_FLAG_RECURSION, my_log_handler, NULL);
 * ]|
 *
 * This example adds a log handler for all critical messages from GTK+:
 * |[<!-- language="C" --> 
 * g_log_set_handler ("Gtk", G_LOG_LEVEL_CRITICAL | G_LOG_FLAG_FATAL
 *                    | G_LOG_FLAG_RECURSION, my_log_handler, NULL);
 * ]|
 *
 * This example adds a log handler for all messages from GLib:
 * |[<!-- language="C" --> 
 * g_log_set_handler ("GLib", G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
 *                    | G_LOG_FLAG_RECURSION, my_log_handler, NULL);
 * ]|
 *
 * Returns: the id of the new handler
 */
guint
g_log_set_handler (const gchar	 *log_domain,
                   GLogLevelFlags log_levels,
                   GLogFunc       log_func,
                   gpointer       user_data)
{
  return g_log_set_handler_full (log_domain, log_levels, log_func, user_data, NULL);
}

/**
 * g_log_set_handler_full: (rename-to g_log_set_handler)
 * @log_domain: (allow-none): the log domain, or %NULL for the default ""
 *     application domain
 * @log_levels: the log levels to apply the log handler for.
 *     To handle fatal and recursive messages as well, combine
 *     the log levels with the #G_LOG_FLAG_FATAL and
 *     #G_LOG_FLAG_RECURSION bit flags.
 * @log_func: the log handler function
 * @user_data: data passed to the log handler
 * @destroy: destroy notify for @user_data, or %NULL
 *
 * Like g_log_sets_handler(), but takes a destroy notify for the @user_data.
 *
 * Returns: the id of the new handler
 *
 * Since: 2.46
 */
guint
g_log_set_handler_full (const gchar    *log_domain,
                        GLogLevelFlags  log_levels,
                        GLogFunc        log_func,
                        gpointer        user_data,
                        GDestroyNotify  destroy)
{
  static guint handler_id = 0;
  GLogDomain *domain;
  GLogHandler *handler;
  
  g_return_val_if_fail ((log_levels & G_LOG_LEVEL_MASK) != 0, 0);
  g_return_val_if_fail (log_func != NULL, 0);
  
  if (!log_domain)
    log_domain = "";

  handler = g_new (GLogHandler, 1);

  g_mutex_lock (&g_messages_lock);

  domain = g_log_find_domain_L (log_domain);
  if (!domain)
    domain = g_log_domain_new_L (log_domain);
  
  handler->id = ++handler_id;
  handler->log_level = log_levels;
  handler->log_func = log_func;
  handler->data = user_data;
  handler->destroy = destroy;
  handler->next = domain->handlers;
  domain->handlers = handler;

  g_mutex_unlock (&g_messages_lock);
  
  return handler_id;
}

/**
 * g_log_set_default_handler:
 * @log_func: the log handler function
 * @user_data: data passed to the log handler
 *
 * Installs a default log handler which is used if no
 * log handler has been set for the particular log domain
 * and log level combination. By default, GLib uses
 * g_log_default_handler() as default log handler.
 *
 * Returns: the previous default log handler
 *
 * Since: 2.6
 */
GLogFunc
g_log_set_default_handler (GLogFunc log_func,
			   gpointer user_data)
{
  GLogFunc old_log_func;
  
  g_mutex_lock (&g_messages_lock);
  old_log_func = default_log_func;
  default_log_func = log_func;
  default_log_data = user_data;
  g_mutex_unlock (&g_messages_lock);
  
  return old_log_func;
}

/**
 * g_test_log_set_fatal_handler:
 * @log_func: the log handler function.
 * @user_data: data passed to the log handler.
 *
 * Installs a non-error fatal log handler which can be
 * used to decide whether log messages which are counted
 * as fatal abort the program.
 *
 * The use case here is that you are running a test case
 * that depends on particular libraries or circumstances
 * and cannot prevent certain known critical or warning
 * messages. So you install a handler that compares the
 * domain and message to precisely not abort in such a case.
 *
 * Note that the handler is reset at the beginning of
 * any test case, so you have to set it inside each test
 * function which needs the special behavior.
 *
 * This handler has no effect on g_error messages.
 *
 * Since: 2.22
 **/
void
g_test_log_set_fatal_handler (GTestLogFatalFunc log_func,
                              gpointer          user_data)
{
  g_mutex_lock (&g_messages_lock);
  fatal_log_func = log_func;
  fatal_log_data = user_data;
  g_mutex_unlock (&g_messages_lock);
}

/**
 * g_log_remove_handler:
 * @log_domain: the log domain
 * @handler_id: the id of the handler, which was returned
 *     in g_log_set_handler()
 *
 * Removes the log handler.
 */
void
g_log_remove_handler (const gchar *log_domain,
		      guint	   handler_id)
{
  GLogDomain *domain;
  
  g_return_if_fail (handler_id > 0);
  
  if (!log_domain)
    log_domain = "";
  
  g_mutex_lock (&g_messages_lock);
  domain = g_log_find_domain_L (log_domain);
  if (domain)
    {
      GLogHandler *work, *last;
      
      last = NULL;
      work = domain->handlers;
      while (work)
	{
	  if (work->id == handler_id)
	    {
	      if (last)
		last->next = work->next;
	      else
		domain->handlers = work->next;
	      g_log_domain_check_free_L (domain); 
	      g_mutex_unlock (&g_messages_lock);
              if (work->destroy)
                work->destroy (work->data);
	      g_free (work);
	      return;
	    }
	  last = work;
	  work = last->next;
	}
    } 
  g_mutex_unlock (&g_messages_lock);
  g_warning ("%s: could not find handler with id '%d' for domain \"%s\"",
	     G_STRLOC, handler_id, log_domain);
}

#define CHAR_IS_SAFE(wc) (!((wc < 0x20 && wc != '\t' && wc != '\n' && wc != '\r') || \
			    (wc == 0x7f) || \
			    (wc >= 0x80 && wc < 0xa0)))
     
static gchar*
strdup_convert (const gchar *string,
		const gchar *charset)
{
  if (!g_utf8_validate (string, -1, NULL))
    {
      GString *gstring = g_string_new ("[Invalid UTF-8] ");
      guchar *p;

      for (p = (guchar *)string; *p; p++)
	{
	  if (CHAR_IS_SAFE(*p) &&
	      !(*p == '\r' && *(p + 1) != '\n') &&
	      *p < 0x80)
	    g_string_append_c (gstring, *p);
	  else
	    g_string_append_printf (gstring, "\\x%02x", (guint)(guchar)*p);
	}
      
      return g_string_free (gstring, FALSE);
    }
  else
    {
      GError *err = NULL;
      
      gchar *result = g_convert_with_fallback (string, -1, charset, "UTF-8", "?", NULL, NULL, &err);
      if (result)
	return result;
      else
	{
	  /* Not thread-safe, but doesn't matter if we print the warning twice
	   */
	  static gboolean warned = FALSE; 
	  if (!warned)
	    {
	      warned = TRUE;
	      _g_fprintf (stderr, "GLib: Cannot convert message: %s\n", err->message);
	    }
	  g_error_free (err);
	  
	  return g_strdup (string);
	}
    }
}

/* For a radix of 8 we need at most 3 output bytes for 1 input
 * byte. Additionally we might need up to 2 output bytes for the
 * readix prefix and 1 byte for the trailing NULL.
 */
#define FORMAT_UNSIGNED_BUFSIZE ((GLIB_SIZEOF_LONG * 3) + 3)

static void
format_unsigned (gchar  *buf,
		 gulong  num,
		 guint   radix)
{
  gulong tmp;
  gchar c;
  gint i, n;

  /* we may not call _any_ GLib functions here (or macros like g_return_if_fail()) */

  if (radix != 8 && radix != 10 && radix != 16)
    {
      *buf = '\000';
      return;
    }
  
  if (!num)
    {
      *buf++ = '0';
      *buf = '\000';
      return;
    } 
  
  if (radix == 16)
    {
      *buf++ = '0';
      *buf++ = 'x';
    }
  else if (radix == 8)
    {
      *buf++ = '0';
    }
	
  n = 0;
  tmp = num;
  while (tmp)
    {
      tmp /= radix;
      n++;
    }

  i = n;

  /* Again we can't use g_assert; actually this check should _never_ fail. */
  if (n > FORMAT_UNSIGNED_BUFSIZE - 3)
    {
      *buf = '\000';
      return;
    }

  while (num)
    {
      i--;
      c = (num % radix);
      if (c < 10)
	buf[i] = c + '0';
      else
	buf[i] = c + 'a' - 10;
      num /= radix;
    }
  
  buf[n] = '\000';
}

/* string size big enough to hold level prefix */
#define	STRING_BUFFER_SIZE	(FORMAT_UNSIGNED_BUFSIZE + 32)

#define	ALERT_LEVELS		(G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING)

/* these are emitted by the default log handler */
#define DEFAULT_LEVELS (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE)
/* these are filtered by G_MESSAGES_DEBUG by the default log handler */
#define INFO_LEVELS (G_LOG_LEVEL_INFO | G_LOG_LEVEL_DEBUG)

static FILE *
mklevel_prefix (gchar          level_prefix[STRING_BUFFER_SIZE],
		GLogLevelFlags log_level)
{
  gboolean to_stdout = TRUE;

  /* we may not call _any_ GLib functions here */

  switch (log_level & G_LOG_LEVEL_MASK)
    {
    case G_LOG_LEVEL_ERROR:
      strcpy (level_prefix, "ERROR");
      to_stdout = FALSE;
      break;
    case G_LOG_LEVEL_CRITICAL:
      strcpy (level_prefix, "CRITICAL");
      to_stdout = FALSE;
      break;
    case G_LOG_LEVEL_WARNING:
      strcpy (level_prefix, "WARNING");
      to_stdout = FALSE;
      break;
    case G_LOG_LEVEL_MESSAGE:
      strcpy (level_prefix, "Message");
      to_stdout = FALSE;
      break;
    case G_LOG_LEVEL_INFO:
      strcpy (level_prefix, "INFO");
      break;
    case G_LOG_LEVEL_DEBUG:
      strcpy (level_prefix, "DEBUG");
      break;
    default:
      if (log_level)
	{
	  strcpy (level_prefix, "LOG-");
	  format_unsigned (level_prefix + 4, log_level & G_LOG_LEVEL_MASK, 16);
	}
      else
	strcpy (level_prefix, "LOG");
      break;
    }
  if (log_level & G_LOG_FLAG_RECURSION)
    strcat (level_prefix, " (recursed)");
  if (log_level & ALERT_LEVELS)
    strcat (level_prefix, " **");

#ifdef G_OS_WIN32
  if ((log_level & G_LOG_FLAG_FATAL) != 0 && !g_test_initialized ())
    win32_keep_fatal_message = TRUE;
#endif
  return to_stdout ? stdout : stderr;
}

typedef struct {
  gchar          *log_domain;
  GLogLevelFlags  log_level;
  gchar          *pattern;
} GTestExpectedMessage;

static GSList *expected_messages = NULL;

/**
 * g_logv:
 * @log_domain: (nullable): the log domain, or %NULL for the default ""
 * application domain
 * @log_level: the log level
 * @format: the message format. See the printf() documentation
 * @args: the parameters to insert into the format string
 *
 * Logs an error or debugging message.
 *
 * If the log level has been set as fatal, the abort()
 * function is called to terminate the program.
 *
 * If g_log_default_handler() is used as the log handler function, a new-line
 * character will automatically be appended to @..., and need not be entered
 * manually.
 */
void
g_logv (const gchar   *log_domain,
	GLogLevelFlags log_level,
	const gchar   *format,
	va_list	       args)
{
  gboolean was_fatal = (log_level & G_LOG_FLAG_FATAL) != 0;
  gboolean was_recursion = (log_level & G_LOG_FLAG_RECURSION) != 0;
  gchar buffer[1025], *msg, *msg_alloc = NULL;
  gint i;

  log_level &= G_LOG_LEVEL_MASK;
  if (!log_level)
    return;

  if (log_level & G_LOG_FLAG_RECURSION)
    {
      /* we use a stack buffer of fixed size, since we're likely
       * in an out-of-memory situation
       */
      gsize size G_GNUC_UNUSED;

      size = _g_vsnprintf (buffer, 1024, format, args);
      msg = buffer;
    }
  else
    msg = msg_alloc = g_strdup_vprintf (format, args);

  if (expected_messages)
    {
      GTestExpectedMessage *expected = expected_messages->data;

      if (g_strcmp0 (expected->log_domain, log_domain) == 0 &&
          ((log_level & expected->log_level) == expected->log_level) &&
          g_pattern_match_simple (expected->pattern, msg))
        {
          expected_messages = g_slist_delete_link (expected_messages,
                                                   expected_messages);
          g_free (expected->log_domain);
          g_free (expected->pattern);
          g_free (expected);
          g_free (msg_alloc);
          return;
        }
      else if ((log_level & G_LOG_LEVEL_DEBUG) != G_LOG_LEVEL_DEBUG)
        {
          gchar level_prefix[STRING_BUFFER_SIZE];
          gchar *expected_message;

          mklevel_prefix (level_prefix, expected->log_level);
          expected_message = g_strdup_printf ("Did not see expected message %s-%s: %s",
                                              expected->log_domain ? expected->log_domain : "**",
                                              level_prefix, expected->pattern);
          g_log_default_handler (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, expected_message, NULL);
          g_free (expected_message);

          log_level |= G_LOG_FLAG_FATAL;
        }
    }

  for (i = g_bit_nth_msf (log_level, -1); i >= 0; i = g_bit_nth_msf (log_level, i))
    {
      GLogLevelFlags test_level;

      test_level = 1 << i;
      if (log_level & test_level)
	{
	  GLogDomain *domain;
	  GLogFunc log_func;
	  GLogLevelFlags domain_fatal_mask;
	  gpointer data = NULL;
          gboolean masquerade_fatal = FALSE;
          guint depth;

	  if (was_fatal)
	    test_level |= G_LOG_FLAG_FATAL;
	  if (was_recursion)
	    test_level |= G_LOG_FLAG_RECURSION;

	  /* check recursion and lookup handler */
	  g_mutex_lock (&g_messages_lock);
          depth = GPOINTER_TO_UINT (g_private_get (&g_log_depth));
	  domain = g_log_find_domain_L (log_domain ? log_domain : "");
	  if (depth)
	    test_level |= G_LOG_FLAG_RECURSION;
	  depth++;
	  domain_fatal_mask = domain ? domain->fatal_mask : G_LOG_FATAL_MASK;
	  if ((domain_fatal_mask | g_log_always_fatal) & test_level)
	    test_level |= G_LOG_FLAG_FATAL;
	  if (test_level & G_LOG_FLAG_RECURSION)
	    log_func = _g_log_fallback_handler;
	  else
	    log_func = g_log_domain_get_handler_L (domain, test_level, &data);
	  domain = NULL;
	  g_mutex_unlock (&g_messages_lock);

	  g_private_set (&g_log_depth, GUINT_TO_POINTER (depth));

          log_func (log_domain, test_level, msg, data);

          if ((test_level & G_LOG_FLAG_FATAL)
              && !(test_level & G_LOG_LEVEL_ERROR))
            {
              masquerade_fatal = fatal_log_func
                && !fatal_log_func (log_domain, test_level, msg, fatal_log_data);
            }

          if ((test_level & G_LOG_FLAG_FATAL) && !masquerade_fatal)
            {
#ifdef G_OS_WIN32
              if (win32_keep_fatal_message)
                {
                  gchar *locale_msg = g_locale_from_utf8 (fatal_msg_buf, -1, NULL, NULL, NULL);

                  MessageBox (NULL, locale_msg, NULL,
                              MB_ICONERROR|MB_SETFOREGROUND);
                }
	      _g_log_abort (IsDebuggerPresent () && !(test_level & G_LOG_FLAG_RECURSION));
#else
	      _g_log_abort (!(test_level & G_LOG_FLAG_RECURSION));
#endif /* !G_OS_WIN32 */
	    }
	  
	  depth--;
	  g_private_set (&g_log_depth, GUINT_TO_POINTER (depth));
	}
    }

  g_free (msg_alloc);
}

/**
 * g_log:
 * @log_domain: (nullable): the log domain, usually #G_LOG_DOMAIN, or %NULL
 * for the default
 * @log_level: the log level, either from #GLogLevelFlags
 *     or a user-defined level
 * @format: the message format. See the printf() documentation
 * @...: the parameters to insert into the format string
 *
 * Logs an error or debugging message.
 *
 * If the log level has been set as fatal, the abort()
 * function is called to terminate the program.
 *
 * If g_log_default_handler() is used as the log handler function, a new-line
 * character will automatically be appended to @..., and need not be entered
 * manually.
 */
void
g_log (const gchar   *log_domain,
       GLogLevelFlags log_level,
       const gchar   *format,
       ...)
{
  va_list args;
  
  va_start (args, format);
  g_logv (log_domain, log_level, format, args);
  va_end (args);
}

/**
 * g_return_if_fail_warning: (skip)
 * @log_domain: (nullable):
 * @pretty_function:
 * @expression: (nullable):
 */
void
g_return_if_fail_warning (const char *log_domain,
			  const char *pretty_function,
			  const char *expression)
{
  g_log (log_domain,
	 G_LOG_LEVEL_CRITICAL,
	 "%s: assertion '%s' failed",
	 pretty_function,
	 expression);
}

/**
 * g_warn_message: (skip)
 * @domain: (nullable):
 * @file:
 * @line:
 * @func:
 * @warnexpr: (nullable):
 */
void
g_warn_message (const char     *domain,
                const char     *file,
                int             line,
                const char     *func,
                const char     *warnexpr)
{
  char *s, lstr[32];
  g_snprintf (lstr, 32, "%d", line);
  if (warnexpr)
    s = g_strconcat ("(", file, ":", lstr, "):",
                     func, func[0] ? ":" : "",
                     " runtime check failed: (", warnexpr, ")", NULL);
  else
    s = g_strconcat ("(", file, ":", lstr, "):",
                     func, func[0] ? ":" : "",
                     " ", "code should not be reached", NULL);
  g_log (domain, G_LOG_LEVEL_WARNING, "%s", s);
  g_free (s);
}

void
g_assert_warning (const char *log_domain,
		  const char *file,
		  const int   line,
		  const char *pretty_function,
		  const char *expression)
{
  if (expression)
    g_log (log_domain,
	   G_LOG_LEVEL_ERROR,
	   "file %s: line %d (%s): assertion failed: (%s)",
	   file,
	   line,
	   pretty_function,
	   expression);
  else
    g_log (log_domain,
	   G_LOG_LEVEL_ERROR,
	   "file %s: line %d (%s): should not be reached",
	   file,
	   line,
	   pretty_function);
  _g_log_abort (FALSE);
  abort ();
}

/**
 * g_test_expect_message:
 * @log_domain: (allow-none): the log domain of the message
 * @log_level: the log level of the message
 * @pattern: a glob-style [pattern][glib-Glob-style-pattern-matching]
 *
 * Indicates that a message with the given @log_domain and @log_level,
 * with text matching @pattern, is expected to be logged. When this
 * message is logged, it will not be printed, and the test case will
 * not abort.
 *
 * Use g_test_assert_expected_messages() to assert that all
 * previously-expected messages have been seen and suppressed.
 *
 * You can call this multiple times in a row, if multiple messages are
 * expected as a result of a single call. (The messages must appear in
 * the same order as the calls to g_test_expect_message().)
 *
 * For example:
 *
 * |[<!-- language="C" --> 
 *   // g_main_context_push_thread_default() should fail if the
 *   // context is already owned by another thread.
 *   g_test_expect_message (G_LOG_DOMAIN,
 *                          G_LOG_LEVEL_CRITICAL,
 *                          "assertion*acquired_context*failed");
 *   g_main_context_push_thread_default (bad_context);
 *   g_test_assert_expected_messages ();
 * ]|
 *
 * Note that you cannot use this to test g_error() messages, since
 * g_error() intentionally never returns even if the program doesn't
 * abort; use g_test_trap_subprocess() in this case.
 *
 * If messages at %G_LOG_LEVEL_DEBUG are emitted, but not explicitly
 * expected via g_test_expect_message() then they will be ignored.
 *
 * Since: 2.34
 */
void
g_test_expect_message (const gchar    *log_domain,
                       GLogLevelFlags  log_level,
                       const gchar    *pattern)
{
  GTestExpectedMessage *expected;

  g_return_if_fail (log_level != 0);
  g_return_if_fail (pattern != NULL);
  g_return_if_fail (~log_level & G_LOG_LEVEL_ERROR);

  expected = g_new (GTestExpectedMessage, 1);
  expected->log_domain = g_strdup (log_domain);
  expected->log_level = log_level;
  expected->pattern = g_strdup (pattern);

  expected_messages = g_slist_append (expected_messages, expected);
}

void
g_test_assert_expected_messages_internal (const char     *domain,
                                          const char     *file,
                                          int             line,
                                          const char     *func)
{
  if (expected_messages)
    {
      GTestExpectedMessage *expected;
      gchar level_prefix[STRING_BUFFER_SIZE];
      gchar *message;

      expected = expected_messages->data;

      mklevel_prefix (level_prefix, expected->log_level);
      message = g_strdup_printf ("Did not see expected message %s-%s: %s",
                                 expected->log_domain ? expected->log_domain : "**",
                                 level_prefix, expected->pattern);
      g_assertion_message (G_LOG_DOMAIN, file, line, func, message);
      g_free (message);
    }
}

/**
 * g_test_assert_expected_messages:
 *
 * Asserts that all messages previously indicated via
 * g_test_expect_message() have been seen and suppressed.
 *
 * If messages at %G_LOG_LEVEL_DEBUG are emitted, but not explicitly
 * expected via g_test_expect_message() then they will be ignored.
 *
 * Since: 2.34
 */

void
_g_log_fallback_handler (const gchar   *log_domain,
			 GLogLevelFlags log_level,
			 const gchar   *message,
			 gpointer       unused_data)
{
  gchar level_prefix[STRING_BUFFER_SIZE];
#ifndef G_OS_WIN32
  gchar pid_string[FORMAT_UNSIGNED_BUFSIZE];
#endif
  FILE *stream;

  /* we cannot call _any_ GLib functions in this fallback handler,
   * which is why we skip UTF-8 conversion, etc.
   * since we either recursed or ran out of memory, we're in a pretty
   * pathologic situation anyways, what we can do is giving the
   * the process ID unconditionally however.
   */

  stream = mklevel_prefix (level_prefix, log_level);
  if (!message)
    message = "(NULL) message";

#ifndef G_OS_WIN32
  format_unsigned (pid_string, getpid (), 10);
#endif

  if (log_domain)
    write_string (stream, "\n");
  else
    write_string (stream, "\n** ");

#ifndef G_OS_WIN32
  write_string (stream, "(process:");
  write_string (stream, pid_string);
  write_string (stream, "): ");
#endif

  if (log_domain)
    {
      write_string (stream, log_domain);
      write_string (stream, "-");
    }
  write_string (stream, level_prefix);
  write_string (stream, ": ");
  write_string (stream, message);
}

static void
escape_string (GString *string)
{
  const char *p = string->str;
  gunichar wc;

  while (p < string->str + string->len)
    {
      gboolean safe;
	    
      wc = g_utf8_get_char_validated (p, -1);
      if (wc == (gunichar)-1 || wc == (gunichar)-2)  
	{
	  gchar *tmp;
	  guint pos;

	  pos = p - string->str;

	  /* Emit invalid UTF-8 as hex escapes 
           */
	  tmp = g_strdup_printf ("\\x%02x", (guint)(guchar)*p);
	  g_string_erase (string, pos, 1);
	  g_string_insert (string, pos, tmp);

	  p = string->str + (pos + 4); /* Skip over escape sequence */

	  g_free (tmp);
	  continue;
	}
      if (wc == '\r')
	{
	  safe = *(p + 1) == '\n';
	}
      else
	{
	  safe = CHAR_IS_SAFE (wc);
	}
      
      if (!safe)
	{
	  gchar *tmp;
	  guint pos;

	  pos = p - string->str;
	  
	  /* Largest char we escape is 0x0a, so we don't have to worry
	   * about 8-digit \Uxxxxyyyy
	   */
	  tmp = g_strdup_printf ("\\u%04x", wc); 
	  g_string_erase (string, pos, g_utf8_next_char (p) - p);
	  g_string_insert (string, pos, tmp);
	  g_free (tmp);

	  p = string->str + (pos + 6); /* Skip over escape sequence */
	}
      else
	p = g_utf8_next_char (p);
    }
}

/**
 * g_log_default_handler:
 * @log_domain: (nullable): the log domain of the message, or %NULL for the
 * default "" application domain
 * @log_level: the level of the message
 * @message: (nullable): the message
 * @unused_data: (nullable): data passed from g_log() which is unused
 *
 * The default log handler set up by GLib; g_log_set_default_handler()
 * allows to install an alternate default log handler.
 * This is used if no log handler has been set for the particular log
 * domain and log level combination. It outputs the message to stderr
 * or stdout and if the log level is fatal it calls abort(). It automatically
 * prints a new-line character after the message, so one does not need to be
 * manually included in @message.
 *
 * The behavior of this log handler can be influenced by a number of
 * environment variables:
 *
 * - `G_MESSAGES_PREFIXED`: A :-separated list of log levels for which
 *   messages should be prefixed by the program name and PID of the
 *   aplication.
 *
 * - `G_MESSAGES_DEBUG`: A space-separated list of log domains for
 *   which debug and informational messages are printed. By default
 *   these messages are not printed.
 *
 * stderr is used for levels %G_LOG_LEVEL_ERROR, %G_LOG_LEVEL_CRITICAL,
 * %G_LOG_LEVEL_WARNING and %G_LOG_LEVEL_MESSAGE. stdout is used for
 * the rest.
 */
void
g_log_default_handler (const gchar   *log_domain,
		       GLogLevelFlags log_level,
		       const gchar   *message,
		       gpointer	      unused_data)
{
  gchar level_prefix[STRING_BUFFER_SIZE], *string;
  GString *gstring;
  FILE *stream;
  const gchar *domains;

  if ((log_level & DEFAULT_LEVELS) || (log_level >> G_LOG_LEVEL_USER_SHIFT))
    goto emit;

  domains = g_getenv ("G_MESSAGES_DEBUG");
  if (((log_level & INFO_LEVELS) == 0) ||
      domains == NULL ||
      (strcmp (domains, "all") != 0 && (!log_domain || !strstr (domains, log_domain))))
    return;

 emit:
  /* we can be called externally with recursion for whatever reason */
  if (log_level & G_LOG_FLAG_RECURSION)
    {
      _g_log_fallback_handler (log_domain, log_level, message, unused_data);
      return;
    }

  stream = mklevel_prefix (level_prefix, log_level);

  gstring = g_string_new (NULL);
  if (log_level & ALERT_LEVELS)
    g_string_append (gstring, "\n");
  if (!log_domain)
    g_string_append (gstring, "** ");

  if ((g_log_msg_prefix & (log_level & G_LOG_LEVEL_MASK)) == (log_level & G_LOG_LEVEL_MASK))
    {
      const gchar *prg_name = g_get_prgname ();
      
      if (!prg_name)
	g_string_append_printf (gstring, "(process:%lu): ", (gulong)getpid ());
      else
	g_string_append_printf (gstring, "(%s:%lu): ", prg_name, (gulong)getpid ());
    }

  if (log_domain)
    {
      g_string_append (gstring, log_domain);
      g_string_append_c (gstring, '-');
    }
  g_string_append (gstring, level_prefix);

  g_string_append (gstring, ": ");
  if (!message)
    g_string_append (gstring, "(NULL) message");
  else
    {
      GString *msg;
      const gchar *charset;

      msg = g_string_new (message);
      escape_string (msg);

      if (g_get_charset (&charset))
	g_string_append (gstring, msg->str);	/* charset is UTF-8 already */
      else
	{
	  string = strdup_convert (msg->str, charset);
	  g_string_append (gstring, string);
	  g_free (string);
	}

      g_string_free (msg, TRUE);
    }
  g_string_append (gstring, "\n");

  string = g_string_free (gstring, FALSE);

  write_string (stream, string);
  g_free (string);
}

/**
 * g_set_print_handler:
 * @func: the new print handler
 *
 * Sets the print handler.
 *
 * Any messages passed to g_print() will be output via
 * the new handler. The default handler simply outputs
 * the message to stdout. By providing your own handler
 * you can redirect the output, to a GTK+ widget or a
 * log file for example.
 *
 * Returns: the old print handler
 */
GPrintFunc
g_set_print_handler (GPrintFunc func)
{
  GPrintFunc old_print_func;

  g_mutex_lock (&g_messages_lock);
  old_print_func = glib_print_func;
  glib_print_func = func;
  g_mutex_unlock (&g_messages_lock);

  return old_print_func;
}

/**
 * g_print:
 * @format: the message format. See the printf() documentation
 * @...: the parameters to insert into the format string
 *
 * Outputs a formatted message via the print handler.
 * The default print handler simply outputs the message to stdout, without
 * appending a trailing new-line character. Typically, @format should end with
 * its own new-line character.
 *
 * g_print() should not be used from within libraries for debugging
 * messages, since it may be redirected by applications to special
 * purpose message windows or even files. Instead, libraries should
 * use g_log(), or the convenience functions g_message(), g_warning()
 * and g_error().
 */
void
g_print (const gchar *format,
         ...)
{
  va_list args;
  gchar *string;
  GPrintFunc local_glib_print_func;

  g_return_if_fail (format != NULL);

  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);

  g_mutex_lock (&g_messages_lock);
  local_glib_print_func = glib_print_func;
  g_mutex_unlock (&g_messages_lock);

  if (local_glib_print_func)
    local_glib_print_func (string);
  else
    {
      const gchar *charset;

      if (g_get_charset (&charset))
        fputs (string, stdout); /* charset is UTF-8 already */
      else
        {
          gchar *lstring = strdup_convert (string, charset);

          fputs (lstring, stdout);
          g_free (lstring);
        }
      fflush (stdout);
    }
  g_free (string);
}

/**
 * g_set_printerr_handler:
 * @func: the new error message handler
 *
 * Sets the handler for printing error messages.
 *
 * Any messages passed to g_printerr() will be output via
 * the new handler. The default handler simply outputs the
 * message to stderr. By providing your own handler you can
 * redirect the output, to a GTK+ widget or a log file for
 * example.
 *
 * Returns: the old error message handler
 */
GPrintFunc
g_set_printerr_handler (GPrintFunc func)
{
  GPrintFunc old_printerr_func;

  g_mutex_lock (&g_messages_lock);
  old_printerr_func = glib_printerr_func;
  glib_printerr_func = func;
  g_mutex_unlock (&g_messages_lock);

  return old_printerr_func;
}

/**
 * g_printerr:
 * @format: the message format. See the printf() documentation
 * @...: the parameters to insert into the format string
 *
 * Outputs a formatted message via the error message handler.
 * The default handler simply outputs the message to stderr, without appending
 * a trailing new-line character. Typically, @format should end with its own
 * new-line character.
 *
 * g_printerr() should not be used from within libraries.
 * Instead g_log() should be used, or the convenience functions
 * g_message(), g_warning() and g_error().
 */
void
g_printerr (const gchar *format,
            ...)
{
  va_list args;
  gchar *string;
  GPrintFunc local_glib_printerr_func;

  g_return_if_fail (format != NULL);

  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);

  g_mutex_lock (&g_messages_lock);
  local_glib_printerr_func = glib_printerr_func;
  g_mutex_unlock (&g_messages_lock);

  if (local_glib_printerr_func)
    local_glib_printerr_func (string);
  else
    {
      const gchar *charset;

      if (g_get_charset (&charset))
        fputs (string, stderr); /* charset is UTF-8 already */
      else
        {
          gchar *lstring = strdup_convert (string, charset);

          fputs (lstring, stderr);
          g_free (lstring);
        }
      fflush (stderr);
    }
  g_free (string);
}

/**
 * g_printf_string_upper_bound:
 * @format: the format string. See the printf() documentation
 * @args: the parameters to be inserted into the format string
 *
 * Calculates the maximum space needed to store the output
 * of the sprintf() function.
 *
 * Returns: the maximum space needed to store the formatted string
 */
gsize
g_printf_string_upper_bound (const gchar *format,
                             va_list      args)
{
  gchar c;
  return _g_vsnprintf (&c, 1, format, args) + 1;
}
