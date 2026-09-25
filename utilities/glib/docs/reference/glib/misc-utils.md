Title: Miscellaneous Utilities
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2000 Red Hat, Inc.

# Miscellaneous Utilities

These are portable utility functions.

## Application Name and Environment

* [func@GLib.get_application_name]
* [func@GLib.set_application_name]
* [func@GLib.get_prgname]
* [func@GLib.set_prgname]
* [func@GLib.get_environ]
* [func@GLib.environ_getenv]
* [func@GLib.environ_setenv]
* [func@GLib.environ_unsetenv]
* [func@GLib.getenv]
* [func@GLib.setenv]
* [func@GLib.unsetenv]
* [func@GLib.listenv]
* [func@GLib.get_user_name]
* [func@GLib.get_real_name]

## System Directories

* [func@GLib.get_user_cache_dir]
* [func@GLib.get_user_data_dir]
* [func@GLib.get_user_config_dir]
* [func@GLib.get_user_state_dir]
* [func@GLib.get_user_runtime_dir]
* [func@GLib.get_user_special_dir]
* [func@GLib.get_system_data_dirs]
* [func@GLib.get_system_config_dirs]
* [func@GLib.reload_user_special_dirs_cache]

## OS Info

Information about the current OS can be retrieved by calling
[func@GLib.get_os_info] and passing it one of the following keys (this list may
grow in future):

 * `G_OS_INFO_KEY_NAME`
 * `G_OS_INFO_KEY_PRETTY_NAME`
 * `G_OS_INFO_KEY_VERSION`
 * `G_OS_INFO_KEY_VERSION_CODENAME`
 * `G_OS_INFO_KEY_VERSION_ID`
 * `G_OS_INFO_KEY_ID`
 * `G_OS_INFO_KEY_HOME_URL`
 * `G_OS_INFO_KEY_DOCUMENTATION_URL`
 * `G_OS_INFO_KEY_SUPPORT_URL`
 * `G_OS_INFO_KEY_BUG_REPORT_URL`
 * `G_OS_INFO_KEY_PRIVACY_POLICY_URL`

## Paths

 * [func@GLib.get_host_name]
 * [func@GLib.get_home_dir]
 * [func@GLib.get_tmp_dir]
 * [func@GLib.get_current_dir]
 * [func@GLib.canonicalize_filename]
 * [func@GLib.path_is_absolute]
 * [func@GLib.path_skip_root]
 * [func@GLib.path_get_basename]
 * [func@GLib.path_get_dirname]
 * [func@GLib.build_filename]
 * [func@GLib.build_filenamev]
 * [func@GLib.build_filename_valist]
 * [func@GLib.build_path]
 * [func@GLib.build_pathv]

## Size Formatting

 * [func@GLib.format_size]
 * [func@GLib.format_size_full]
 * [func@GLib.format_size_for_display]

## Executables

 * [func@GLib.find_program_in_path]

## Bit Manipulation

 * [func@GLib.bit_nth_lsf]
 * [func@GLib.bit_nth_msf]
 * [func@GLib.bit_storage]

## Primes

 * [func@GLib.spaced_primes_closest]

## Process Lifetime

 * [func@GLib.abort]

## Debug

 * [func@GLib.parse_debug_string]

## Sorting

 * [func@GLib.qsort_with_data]

## Pointers

 * [func@GLib.nullify_pointer]

## Deprecated API

 * [type@GLib.VoidFunc]
 * [type@GLib.FreeFunc]
 * [func@GLib.atexit]
