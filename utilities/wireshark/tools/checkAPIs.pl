#!/usr/bin/env perl

#
# Copyright 2006, Jeff Morriss <jeff.morriss.ws[AT]gmail.com>
#
# A simple tool to check source code for function calls that should not
# be called by Wireshark code and to perform certain other checks.
#
# Usage:
# checkAPIs.pl [-M] [-g group1] [-g group2] ...
#              [-s summary-group1] [-s summary-group2] ...
#              [--nocheck-value-string-array]
#              [--nocheck-addtext] [--nocheck-hf] [--debug] file1 file2 ...
#
# Wireshark - Network traffic analyzer
# By Gerald Combs <gerald@wireshark.org>
# Copyright 1998 Gerald Combs
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

use strict;
use Getopt::Long;

my %APIs = (
        # API groups.
        # Group name, e.g. 'prohibited'
        # '<name>' => {
        #   'count_errors'      => 1,                     # 1 if these are errors, 0 if warnings
        #   'functions'         => [ 'f1', 'f2', ...],    # Function array
        #   'function-counts'   => {'f1',0, 'f2',0, ...}, # Function Counts hash (initialized in the code)
        # }
        #
        # APIs that MUST NOT be used in Wireshark
        'prohibited' => { 'count_errors' => 1, 'functions' => [
                # Memory-unsafe APIs
                # Use something that won't overwrite the end of your buffer instead
                # of these.
                #
                # Microsoft provides lists of unsafe functions and their
                # recommended replacements in "Security Development Lifecycle
                # (SDL) Banned Function Calls"
                # https://msdn.microsoft.com/en-us/library/bb288454.aspx
                # and "Deprecated CRT Functions"
                # https://msdn.microsoft.com/en-us/library/ms235384.aspx
                #
                'gets',
                'sprintf',
                'g_sprintf',
                'vsprintf',
                'g_vsprintf',
                'strcpy',
                'strncpy',
                'strcat',
                'strncat',
                'cftime',
                'ascftime',
                ### non-portable APIs
                # use glib (g_*) versions instead of these:
                'ntohl',
                'ntohs',
                'htonl',
                'htons',
                'strdup',
                'strndup',
                # Windows doesn't have this; use g_ascii_strtoull() instead
                'strtoull',
                ### non-portable: fails on Windows Wireshark built with VC newer than VC6
                # See https://bugs.wireshark.org/bugzilla/show_bug.cgi?id=6695#c2
                'g_fprintf',
                'g_vfprintf',
                ### non-ANSI C
                # use memset, memcpy, memcmp instead of these:
                'bzero',
                'bcopy',
                'bcmp',
                # The MSDN page for ZeroMemory recommends SecureZeroMemory
                # instead.
                'ZeroMemory',
                # use wmem_*, ep_*, or g_* functions instead of these:
                # (One thing to be aware of is that space allocated with malloc()
                # may not be freeable--at least on Windows--with g_free() and
                # vice-versa.)
                'malloc',
                'calloc',
                'realloc',
                'valloc',
                'free',
                'cfree',
                # Locale-unsafe APIs
                # These may have unexpected behaviors in some locales (e.g.,
                # "I" isn't always the upper-case form of "i", and "i" isn't
                # always the lower-case form of "I").  Use the g_ascii_* version
                # instead.
                'isalnum',
                'isascii',
                'isalpha',
                'iscntrl',
                'isdigit',
                'islower',
                'isgraph',
                'isprint',
                'ispunct',
                'isspace',
                'isupper',
                'isxdigit',
                'tolower',
                'atof',
                'strtod',
                'strcasecmp',
                'strncasecmp',
                'g_strcasecmp',
                'g_strncasecmp',
                'g_strup',
                'g_strdown',
                'g_string_up',
                'g_string_down',
                'strerror',     # use g_strerror
                # Use the ws_* version of these:
                # (Necessary because on Windows we use UTF8 for throughout the code
                # so we must tweak that to UTF16 before operating on the file.  Code
                # using these functions will work unless the file/path name contains
                # non-ASCII chars.)
                'open',
                'rename',
                'mkdir',
                'stat',
                'unlink',
                'remove',
                'fopen',
                'freopen',
                'fstat',
                'lseek',
                # Misc
                'tmpnam',       # use mkstemp
                '_snwprintf'    # use StringCchPrintf
                ] },

        ### Soft-Deprecated functions that should not be used in new code but
        # have not been entirely removed from old code. These will become errors
        # once they've been removed from all existing code.
        'soft-deprecated' => { 'count_errors' => 0, 'functions' => [
                'tvb_length_remaining', # replaced with tvb_captured_length_remaining

                # Locale-unsafe APIs
                # These may have unexpected behaviors in some locales (e.g.,
                # "I" isn't always the upper-case form of "i", and "i" isn't
                # always the lower-case form of "I").  Use the g_ascii_* version
                # instead.
                'toupper'
            ] },

        # APIs that SHOULD NOT be used in Wireshark (any more)
        'deprecated' => { 'count_errors' => 1, 'functions' => [
                'perror',                                       # Use g_strerror() and report messages in whatever
                                                                #  fashion is appropriate for the code in question.
                'ctime',                                        # Use abs_time_secs_to_str()
                'next_tvb_add_port',                            # Use next_tvb_add_uint() (and a matching change
                                                                #  of NTVB_PORT -> NTVB_UINT)

                ### Deprecated GLib/GObject functions/macros
                # (The list is based upon the GLib 2.30.2 & GObject 2.30.2 documentation;
                #  An entry may be commented out if it is currently
                #  being used in Wireshark and if the replacement functionality
                #  is not available in all the GLib versions that Wireshark
                #  currently supports.
                # Note: Wireshark currently (Jan 2012) requires GLib 2.14 or newer.
                #  The Wireshark build currently (Jan 2012) defines G_DISABLE_DEPRECATED
                #  so use of any of the following should cause the Wireshark build to fail and
                #  therefore the tests for obsolete GLib function usage in checkAPIs should not be needed.
                'G_ALLOC_AND_FREE',
                'G_ALLOC_ONLY',
                'g_allocator_free',                             # "use slice allocator" (avail since 2.10,2.14)
                'g_allocator_new',                              # "use slice allocator" (avail since 2.10,2.14)
                'g_async_queue_ref_unlocked',                   # g_async_queue_ref()   (OK since 2.8)
                'g_async_queue_unref_and_unlock',               # g_async_queue_unref() (OK since 2.8)
                'g_atomic_int_exchange_and_add',                # since 2.30
                'g_basename',
                'g_blow_chunks',                                # "use slice allocator" (avail since 2.10,2.14)
                'g_cache_value_foreach',                        # g_cache_key_foreach()
                'g_chunk_free',                                 # g_slice_free (avail since 2.10)
                'g_chunk_new',                                  # g_slice_new  (avail since 2.10)
                'g_chunk_new0',                                 # g_slice_new0 (avail since 2.10)
                'g_completion_add_items',                       # since 2.26
                'g_completion_clear_items',                     # since 2.26
                'g_completion_complete',                        # since 2.26
                'g_completion_complete_utf8',                   # since 2.26
                'g_completion_free',                            # since 2.26
                'g_completion_new',                             # since 2.26
                'g_completion_remove_items',                    # since 2.26
                'g_completion_set_compare',                     # since 2.26
                'G_CONST_RETURN',                               # since 2.26
                'g_date_set_time',                              # g_date_set_time_t (avail since 2.10)
                'g_dirname',
                'g_format_size_for_display',                    # since 2.30: use g_format_size()
                'G_GNUC_FUNCTION',
                'G_GNUC_PRETTY_FUNCTION',
                'g_hash_table_freeze',
                'g_hash_table_thaw',
                'G_HAVE_GINT64',
                'g_io_channel_close',
                'g_io_channel_read',
                'g_io_channel_seek',
                'g_io_channel_write',
                'g_list_pop_allocator',                         # "does nothing since 2.10"
                'g_list_push_allocator',                        # "does nothing since 2.10"
                'g_main_destroy',
                'g_main_is_running',
                'g_main_iteration',
                'g_main_new',
                'g_main_pending',
                'g_main_quit',
                'g_main_run',
                'g_main_set_poll_func',
                'g_mapped_file_free',                           # [as of 2.22: use g_map_file_unref]
                'g_mem_chunk_alloc',                            # "use slice allocator" (avail since 2.10)
                'g_mem_chunk_alloc0',                           # "use slice allocator" (avail since 2.10)
                'g_mem_chunk_clean',                            # "use slice allocator" (avail since 2.10)
                'g_mem_chunk_create',                           # "use slice allocator" (avail since 2.10)
                'g_mem_chunk_destroy',                          # "use slice allocator" (avail since 2.10)
                'g_mem_chunk_free',                             # "use slice allocator" (avail since 2.10)
                'g_mem_chunk_info',                             # "use slice allocator" (avail since 2.10)
                'g_mem_chunk_new',                              # "use slice allocator" (avail since 2.10)
                'g_mem_chunk_print',                            # "use slice allocator" (avail since 2.10)
                'g_mem_chunk_reset',                            # "use slice allocator" (avail since 2.10)
                'g_node_pop_allocator',                         # "does nothing since 2.10"
                'g_node_push_allocator',                        # "does nothing since 2.10"
                'g_relation_count',                             # since 2.26
                'g_relation_delete',                            # since 2.26
                'g_relation_destroy',                           # since 2.26
                'g_relation_exists',                            # since 2.26
                'g_relation_index',                             # since 2.26
                'g_relation_insert',                            # since 2.26
                'g_relation_new',                               # since 2.26
                'g_relation_print',                             # since 2.26
                'g_relation_select',                            # since 2.26
                'g_scanner_add_symbol',
                'g_scanner_remove_symbol',
                'g_scanner_foreach_symbol',
                'g_scanner_freeze_symbol_table',
                'g_scanner_thaw_symbol_table',
                'g_slist_pop_allocator',                        # "does nothing since 2.10"
                'g_slist_push_allocator',                       # "does nothing since 2.10"
                'g_source_get_current_time',                    # since 2.28: use g_source_get_time()
                'g_strcasecmp',                                 #
                'g_strdown',                                    #
                'g_string_down',                                #
                'g_string_sprintf',                             # use g_string_printf() instead
                'g_string_sprintfa',                            # use g_string_append_printf instead
                'g_string_up',                                  #
                'g_strncasecmp',                                #
                'g_strup',                                      #
                'g_tree_traverse',
                'g_tuples_destroy',                             # since 2.26
                'g_tuples_index',                               # since 2.26
                'g_unicode_canonical_decomposition',            # since 2.30: use g_unichar_fully_decompose()
                'G_UNICODE_COMBINING_MARK',                     # since 2.30:use G_UNICODE_SPACING_MARK
                'g_value_set_boxed_take_ownership',             # GObject
                'g_value_set_object_take_ownership',            # GObject
                'g_value_set_param_take_ownership',             # GObject
                'g_value_set_string_take_ownership',            # Gobject
                'G_WIN32_DLLMAIN_FOR_DLL_NAME',
                'g_win32_get_package_installation_directory',
                'g_win32_get_package_installation_subdirectory',
                ] },

        # APIs that make the program exit. Dissectors shouldn't call these
        'abort' => { 'count_errors' => 1, 'functions' => [
                'abort',
                'assert',
                'assert_perror',
                'exit',
                'g_assert',
                'g_error',
                ] },

        # APIs that print to the terminal. Dissectors shouldn't call these
        'termoutput' => { 'count_errors' => 0, 'functions' => [
                'printf',
                'g_warning',
                ] },

        # Deprecated GTK APIs
        #  which SHOULD NOT be used in Wireshark (any more).
        #  (Filled in from 'E' entries in %deprecatedGtkFunctions below)
        'deprecated-gtk' => { 'count_errors' => 1, 'functions' => [
                ] },

        # Deprecated GTK APIs yet to be replaced
        #  (Filled in from 'W' entries in %deprecatedGtkFunctions below)
        'deprecated-gtk-todo' => { 'count_errors' => 0, 'functions' => [
                ] },

);

my @apiGroups = qw(prohibited deprecated soft-deprecated);


# Deprecated GTK+ (and GDK) functions/macros with (E)rror or (W)arning flag:
# (The list is based upon the GTK+ 2.24.8 documentation;
# E: There should be no current Wireshark use so Error if seen;
# W: Not all Wireshark use yet fixed so Warn if seen; (Change to E as fixed)

# Note: Wireshark currently (Jan 2012) requires GTK 2.12 or newer.
#       The Wireshark build currently (Jan 2012) defines GTK_DISABLE_DEPRECATED.
#       However: Wireshark source still has a few uses of deprecated GTK functions
#                (which either are ifdef'd out or GTK_DISABLE_DEPRECATED is undef'd).
#                Thus: there a few GTK functions still marked as 'W' below.
#       Deprecated GDK functions are included in the list of deprecated GTK functions.
#       The Wireshark build currently (Jan 2012) does not define GDK_DISABLE_DEPRECATED
#         since there are still some uses of deprecated GDK functions.
#         They are marked with 'W' below.

my %deprecatedGtkFunctions = (
                'gtk_about_dialog_get_name',                    'E',
                'gtk_about_dialog_set_name',                    'E',
                'gtk_about_dialog_set_email_hook',              'E', # since 2.24
                'gtk_about_dialog_set_url_hook',                'E', # since 2.24
                'gtk_accel_group_ref',                          'E',
                'gtk_accel_group_unref',                        'E',
                'gtk_action_block_activate_from',               'E', # since 2.16
                'gtk_action_connect_proxy',                     'E', # since 2.16: use gtk_activatable_set_related_action() (as of 2.16)
                'gtk_action_disconnect_proxy',                  'E', # since 2.16: use gtk_activatable_set_related_action() (as of 2.16)
                'gtk_action_unblock_activate_from',             'E', # since 2.16
                'gtk_binding_entry_add',                        'E',
                'gtk_binding_entry_add_signal',                 'E',
                'gtk_binding_entry_clear',                      'E',
                'gtk_binding_parse_binding',                    'E',
                'gtk_box_pack_end_defaults',                    'E',
                'gtk_box_pack_start_defaults',                  'E',
                'gtk_button_box_get_child_ipadding',            'E',
                'gtk_button_box_get_child_size',                'E',
                'gtk_button_box_get_spacing',                   'E',
                'gtk_button_box_set_child_ipadding',            'E', # style properties child-internal-pad-x/-y
                'gtk_button_box_set_child_size',                'E', # style properties child-min-width/-height
                'gtk_button_box_set_spacing',                   'E', # gtk_box_set_spacing [==]
                'gtk_button_enter',                             'E', # since 2.20
                'gtk_button_leave',                             'E', # since 2.20
                'gtk_button_pressed',                           'E', # since 2.20
                'gtk_button_released',                          'E', # since 2.20
                'gtk_calendar_display_options',                 'E',
                'gtk_calendar_freeze',                          'E',
                'gtk_calendar_thaw',                            'E',
                'GTK_CELL_PIXMAP',                              'E', # GtkTreeView (& related) ...
                'GTK_CELL_PIXTEXT',                             'E',
                'gtk_cell_renderer_editing_canceled',           'E',
                'GTK_CELL_TEXT',                                'E',
                'gtk_cell_view_get_cell_renderers',             'E', # gtk_cell_layout_get_cells ()             (avail since 2.12)
                'GTK_CELL_WIDGET',                              'E',
                'GTK_CHECK_CAST',                               'E', # G_TYPE_CHECK_INSTANCE_CAST [==]
                'GTK_CHECK_CLASS_CAST',                         'E', # G_TYPE_CHECK_CLASS_CAST [==]
                'GTK_CHECK_CLASS_TYPE',                         'E', # G_TYPE_CHECK_CLASS_TYPE [==]
                'GTK_CHECK_GET_CLASS',                          'E', # G_TYPE_INSTANCE_GET_CLASS [==]
                'gtk_check_menu_item_set_show_toggle',          'E', # Does nothing; remove; [show_toggle is always TRUE]
                'gtk_check_menu_item_set_state',                'E',
                'GTK_CHECK_TYPE',                               'E', # G_TYPE_CHECK_INSTANCE_TYPE [==]
                'GTK_CLASS_NAME',                               'E',
                'GTK_CLASS_TYPE',                               'E',
                'GTK_CLIST_ADD_MODE',                           'E', # GtkTreeView (& related) ...
                'gtk_clist_append',                             'E',
                'GTK_CLIST_AUTO_RESIZE_BLOCKED',                'E',
                'GTK_CLIST_AUTO_SORT',                          'E',
                'gtk_clist_clear',                              'E',
                'gtk_clist_column_title_active',                'E',
                'gtk_clist_column_title_passive',               'E',
                'gtk_clist_column_titles_active',               'E',
                'gtk_clist_column_titles_hide',                 'E',
                'gtk_clist_column_titles_passive',              'E',
                'gtk_clist_column_titles_show',                 'E',
                'gtk_clist_columns_autosize',                   'E',
                'GTK_CLIST_DRAW_DRAG_LINE',                     'E',
                'GTK_CLIST_DRAW_DRAG_RECT',                     'E',
                'gtk_clist_find_row_from_data',                 'E',
                'GTK_CLIST_FLAGS',                              'E',
                'gtk_clist_freeze',                             'E',
                'gtk_clist_get_cell_style',                     'E',
                'gtk_clist_get_cell_type',                      'E',
                'gtk_clist_get_column_title',                   'E',
                'gtk_clist_get_column_widget',                  'E',
                'gtk_clist_get_hadjustment',                    'E',
                'gtk_clist_get_pixmap',                         'E',
                'gtk_clist_get_pixtext',                        'E',
                'gtk_clist_get_row_data',                       'E',
                'gtk_clist_get_row_style',                      'E',
                'gtk_clist_get_selectable',                     'E',
                'gtk_clist_get_selection_info',                 'E',
                'gtk_clist_get_text',                           'E',
                'gtk_clist_get_vadjustment',                    'E',
                'GTK_CLIST_IN_DRAG',                            'E',
                'gtk_clist_insert',                             'E',
                'gtk_clist_moveto',                             'E',
                'gtk_clist_new',                                'E',
                'gtk_clist_new_with_titles',                    'E',
                'gtk_clist_optimal_column_width',               'E',
                'gtk_clist_prepend',                            'E',
                'gtk_clist_remove',                             'E',
                'GTK_CLIST_REORDERABLE',                        'E',
                'GTK_CLIST_ROW',                                'E',
                'GTK_CLIST_ROW_HEIGHT_SET',                     'E',
                'gtk_clist_row_is_visible',                     'E',
                'gtk_clist_row_move',                           'E',
                'gtk_clist_select_all',                         'E',
                'gtk_clist_select_row',                         'E',
                'gtk_clist_set_auto_sort',                      'E',
                'gtk_clist_set_background',                     'E',
                'gtk_clist_set_button_actions',                 'E',
                'gtk_clist_set_cell_style',                     'E',
                'gtk_clist_set_column_auto_resize',             'E',
                'gtk_clist_set_column_justification',           'E',
                'gtk_clist_set_column_max_width',               'E',
                'gtk_clist_set_column_min_width',               'E',
                'gtk_clist_set_column_resizeable',              'E',
                'gtk_clist_set_column_title',                   'E',
                'gtk_clist_set_column_visibility',              'E',
                'gtk_clist_set_column_widget',                  'E',
                'gtk_clist_set_column_width',                   'E',
                'gtk_clist_set_compare_func',                   'E',
                'GTK_CLIST_SET_FLAG',                           'E',
                'gtk_clist_set_foreground',                     'E',
                'gtk_clist_set_hadjustment',                    'E',
                'gtk_clist_set_pixmap',                         'E',
                'gtk_clist_set_pixtext',                        'E',
                'gtk_clist_set_reorderable',                    'E',
                'gtk_clist_set_row_data',                       'E',
                'gtk_clist_set_row_data_full',                  'E',
                'gtk_clist_set_row_height',                     'E',
                'gtk_clist_set_row_style',                      'E',
                'gtk_clist_set_selectable',                     'E',
                'gtk_clist_set_selection_mode',                 'E',
                'gtk_clist_set_shadow_type',                    'E',
                'gtk_clist_set_shift',                          'E',
                'gtk_clist_set_sort_column',                    'E',
                'gtk_clist_set_sort_type',                      'E',
                'gtk_clist_set_text',                           'E',
                'gtk_clist_set_use_drag_icons',                 'E',
                'gtk_clist_set_vadjustment',                    'E',
                'GTK_CLIST_SHOW_TITLES',                        'E',
                'gtk_clist_sort',                               'E',
                'gtk_clist_swap_rows',                          'E',
                'gtk_clist_thaw',                               'E',
                'gtk_clist_undo_selection',                     'E',
                'gtk_clist_unselect_all',                       'E',
                'gtk_clist_unselect_row',                       'E',
                'GTK_CLIST_UNSET_FLAG',                         'E',
                'GTK_CLIST_USE_DRAG_ICONS',                     'E',
                'gtk_color_selection_get_color',                'E',
                'gtk_color_selection_set_change_palette_hook',  'E',
                'gtk_color_selection_set_color',                'E',
                'gtk_color_selection_set_update_policy',        'E',
                'gtk_combo_box_append_text',                    'E', #
                'gtk_combo_box_entry_get_text_column',          'E', #
                'gtk_combo_box_entry_new',                      'E', #
                'gtk_combo_box_entry_new_text',                 'E', #
                'gtk_combo_box_entry_new_with_model',           'E', #
                'gtk_combo_box_entry_set_text_column',          'E', #
                'gtk_combo_box_get_active_text',                'E', #
                'gtk_combo_box_insert_text',                    'E', #
                'gtk_combo_box_new_text',                       'E', #
                'gtk_combo_box_prepend_text',                   'E', #
                'gtk_combo_box_remove_text',                    'E', #
                'gtk_combo_disable_activate',                   'E', # GtkComboBoxEntry ... (avail since 2.4/2.6/2.10/2.14)
                'gtk_combo_new',                                'E',
                'gtk_combo_set_case_sensitive',                 'E',
                'gtk_combo_set_item_string',                    'E',
                'gtk_combo_set_popdown_strings',                'E',
                'gtk_combo_set_use_arrows',                     'E',
                'gtk_combo_set_use_arrows_always',              'E',
                'gtk_combo_set_value_in_list',                  'E',
                'gtk_container_border_width',                   'E', # gtk_container_set_border_width [==]
                'gtk_container_children',                       'E', # gtk_container_get_children [==]
                'gtk_container_foreach_full',                   'E',
                'gtk_ctree_collapse',                           'E',
                'gtk_ctree_collapse_recursive',                 'E',
                'gtk_ctree_collapse_to_depth',                  'E',
                'gtk_ctree_expand',                             'E',
                'gtk_ctree_expand_recursive',                   'E',
                'gtk_ctree_expand_to_depth',                    'E',
                'gtk_ctree_export_to_gnode',                    'E',
                'gtk_ctree_find',                               'E',
                'gtk_ctree_find_all_by_row_data',               'E',
                'gtk_ctree_find_all_by_row_data_custom',        'E',
                'gtk_ctree_find_by_row_data',                   'E',
                'gtk_ctree_find_by_row_data_custom',            'E',
                'gtk_ctree_find_node_ptr',                      'E',
                'GTK_CTREE_FUNC',                               'E',
                'gtk_ctree_get_node_info',                      'E',
                'gtk_ctree_insert_gnode',                       'E',
                'gtk_ctree_insert_node',                        'E',
                'gtk_ctree_is_ancestor',                        'E',
                'gtk_ctree_is_hot_spot',                        'E',
                'gtk_ctree_is_viewable',                        'E',
                'gtk_ctree_last',                               'E',
                'gtk_ctree_move',                               'E',
                'gtk_ctree_new',                                'E',
                'gtk_ctree_new_with_titles',                    'E',
                'GTK_CTREE_NODE',                               'E',
                'gtk_ctree_node_get_cell_style',                'E',
                'gtk_ctree_node_get_cell_type',                 'E',
                'gtk_ctree_node_get_pixmap',                    'E',
                'gtk_ctree_node_get_pixtext',                   'E',
                'gtk_ctree_node_get_row_data',                  'E',
                'gtk_ctree_node_get_row_style',                 'E',
                'gtk_ctree_node_get_selectable',                'E',
                'gtk_ctree_node_get_text',                      'E',
                'gtk_ctree_node_is_visible',                    'E',
                'gtk_ctree_node_moveto',                        'E',
                'GTK_CTREE_NODE_NEXT',                          'E',
                'gtk_ctree_node_nth',                           'E',
                'GTK_CTREE_NODE_PREV',                          'E',
                'gtk_ctree_node_set_background',                'E',
                'gtk_ctree_node_set_cell_style',                'E',
                'gtk_ctree_node_set_foreground',                'E',
                'gtk_ctree_node_set_pixmap',                    'E',
                'gtk_ctree_node_set_pixtext',                   'E',
                'gtk_ctree_node_set_row_data',                  'E',
                'gtk_ctree_node_set_row_data_full',             'E',
                'gtk_ctree_node_set_row_style',                 'E',
                'gtk_ctree_node_set_selectable',                'E',
                'gtk_ctree_node_set_shift',                     'E',
                'gtk_ctree_node_set_text',                      'E',
                'gtk_ctree_post_recursive',                     'E',
                'gtk_ctree_post_recursive_to_depth',            'E',
                'gtk_ctree_pre_recursive',                      'E',
                'gtk_ctree_pre_recursive_to_depth',             'E',
                'gtk_ctree_real_select_recursive',              'E',
                'gtk_ctree_remove_node',                        'E',
                'GTK_CTREE_ROW',                                'E',
                'gtk_ctree_select',                             'E',
                'gtk_ctree_select_recursive',                   'E',
                'gtk_ctree_set_drag_compare_func',              'E',
                'gtk_ctree_set_expander_style',                 'E',
                'gtk_ctree_set_indent',                         'E',
                'gtk_ctree_set_line_style',                     'E',
                'gtk_ctree_set_node_info',                      'E',
                'gtk_ctree_set_reorderable',                    'E',
                'gtk_ctree_set_show_stub',                      'E',
                'gtk_ctree_set_spacing',                        'E',
                'gtk_ctree_sort_node',                          'E',
                'gtk_ctree_sort_recursive',                     'E',
                'gtk_ctree_toggle_expansion',                   'E',
                'gtk_ctree_toggle_expansion_recursive',         'E',
                'gtk_ctree_unselect',                           'E',
                'gtk_ctree_unselect_recursive',                 'E',
                'gtk_curve_get_vector',                         'E', # since 2.20
                'gtk_curve_new',                                'E', # since 2.20
                'gtk_curve_reset',                              'E', # since 2.20
                'gtk_curve_set_curve_type',                     'E', # since 2.20
                'gtk_curve_set_gamma',                          'E', # since 2.20
                'gtk_curve_set_range',                          'E', # since 2.20
                'gtk_curve_set_vector',                         'E', # since 2.20
                'gtk_dialog_get_has_separator',                 'E', # This function will be removed in GTK+ 3
                'gtk_dialog_set_has_separator',                 'E', # This function will be removed in GTK+ 3
                'gtk_drag_set_default_icon',                    'E',
                'gtk_draw_arrow',                               'E',
                'gtk_draw_box',                                 'E',
                'gtk_draw_box_gap',                             'E',
                'gtk_draw_check',                               'E',
                'gtk_draw_diamond',                             'E',
                'gtk_draw_expander',                            'E',
                'gtk_draw_extension',                           'E',
                'gtk_draw_flat_box',                            'E',
                'gtk_draw_focus',                               'E',
                'gtk_draw_handle',                              'E',
                'gtk_draw_hline',                               'E',
                'gtk_draw_layout',                              'E',
                'gtk_draw_option',                              'E',
                'gtk_draw_polygon',                             'E',
                'gtk_draw_resize_grip',                         'E',
                'gtk_draw_shadow',                              'E',
                'gtk_draw_shadow_gap',                          'E',
                'gtk_draw_slider',                              'E',
                'gtk_draw_string',                              'E',
                'gtk_draw_tab',                                 'E',
                'gtk_draw_vline',                               'E',
                'gtk_drawing_area_size',                        'E', # >> g_object_set() [==] ?
                                                                     #    gtk_widget_set_size_request() [==?]
                'gtk_entry_append_text',                        'E', # >> gtk_editable_insert_text() [==?]
                'gtk_entry_new_with_max_length',                'E', # gtk_entry_new(); gtk_entry_set_max_length()
                'gtk_entry_prepend_text',                       'E',
                'gtk_entry_select_region',                      'E',
                'gtk_entry_set_editable',                       'E', # >> gtk_editable_set_editable() [==?]
                'gtk_entry_set_position',                       'E',
                'gtk_exit',                                     'E', # exit() [==]
                'gtk_file_chooser_button_new_with_backend',     'E',
                'gtk_file_chooser_dialog_new_with_backend',     'E',
                'gtk_file_chooser_widget_new_with_backend',     'E',
                'gtk_file_selection_complete',                  'E',
                'gtk_file_selection_get_filename',              'E', # GtkFileChooser ...
                'gtk_file_selection_get_select_multiple',       'E',
                'gtk_file_selection_get_selections',            'E',
                'gtk_file_selection_hide_fileop_buttons',       'E',
                'gtk_file_selection_new',                       'E',
                'gtk_file_selection_set_filename',              'E',
                'gtk_file_selection_set_select_multiple',       'E',
                'gtk_file_selection_show_fileop_buttons',       'E',
                'gtk_fixed_get_has_window',                     'E', # gtk_widget_get_has_window() (available since 2.18)
                'gtk_fixed_set_has_window',                     'E', # gtk_widget_set_has_window() (available since 2.18)
                'gtk_font_selection_dialog_get_apply_button',   'E',
                'gtk_font_selection_dialog_get_font',           'E',
                'gtk_font_selection_get_font',                  'E', # gtk_font_selection_get_font_name() [!=]
                'GTK_FUNDAMENTAL_TYPE',                         'E',
                'gtk_gamma_curve_new',                          'E', # since 2.20
                'gtk_hbox_new',                                 'W', # gtk_box_new
                'gtk_hbutton_box_get_layout_default',           'E',
                'gtk_hbutton_box_get_spacing_default',          'E',
                'gtk_hbutton_box_new',                          'W', # gtk_button_box_new
                'gtk_hbutton_box_set_layout_default',           'E',
                'gtk_hbutton_box_set_spacing_default',          'E',
                'gtk_hruler_new',                               'E', # since 2.24
                'gtk_icon_view_get_orientation',                'E', # gtk_icon_view_get_item_orientation()
                'gtk_icon_view_set_orientation',                'E', # gtk_icon_view_set_item_orientation()
                'gtk_idle_add',                                 'E',
                'gtk_idle_add_full',                            'E',
                'gtk_idle_add_priority',                        'E',
                'gtk_idle_remove',                              'E',
                'gtk_idle_remove_by_data',                      'E',
                'gtk_image_get',                                'E',
                'gtk_image_set',                                'E',
                'gtk_init_add',                                 'E', # removed in 3.0
                'gtk_input_add_full',                           'E', # >>> g_io_add_watch_full()
                'gtk_input_dialog_new',                         'E', # since 2.20
                'gtk_input_remove',                             'E', # >>> g_source_remove()
                'GTK_IS_ROOT_TREE',                             'E',
                'gtk_item_deselect',                            'E', # gtk_menu_item_deselect()
                'gtk_item_select',                              'E', # gtk_menu_item_select()
                'gtk_item_toggle',                              'E', #
                'gtk_item_factories_path_delete',               'E', # GtkUIManager (avail since 2.4) ...
                'gtk_item_factory_add_foreign',                 'E',
                'gtk_item_factory_construct',                   'E',
                'gtk_item_factory_create_item',                 'W',
                'gtk_item_factory_create_items',                'E',
                'gtk_item_factory_create_items_ac',             'E',
                'gtk_item_factory_create_menu_entries',         'E',
                'gtk_item_factory_delete_entries',              'E',
                'gtk_item_factory_delete_entry',                'E',
                'gtk_item_factory_delete_item',                 'E',
                'gtk_item_factory_from_path',                   'E',
                'gtk_item_factory_from_widget',                 'W',
                'gtk_item_factory_get_item',                    'W',
                'gtk_item_factory_get_item_by_action',          'E',
                'gtk_item_factory_get_widget',                  'W',
                'gtk_item_factory_get_widget_by_action',        'E',
                'gtk_item_factory_new',                         'E',
                'gtk_item_factory_path_from_widget',            'E',
                'gtk_item_factory_popup',                       'E',
                'gtk_item_factory_popup_data',                  'E',
                'gtk_item_factory_popup_data_from_widget',      'E',
                'gtk_item_factory_popup_with_data',             'E',
                'gtk_item_factory_set_translate_func',          'E',
                'gtk_label_get',                                'E', # gtk_label_get_text() [!=]
                'gtk_label_parse_uline',                        'E',
                'gtk_label_set',                                'E', # gtk_label_set_text() [==]
                'gtk_layout_freeze',                            'E',
                'gtk_layout_thaw',                              'E',
                'gtk_link_button_set_uri_hook',                 'E', # since 2.24
                'gtk_list_append_items',                        'E',
                'gtk_list_child_position',                      'E',
                'gtk_list_clear_items',                         'E',
                'gtk_list_end_drag_selection',                  'E',
                'gtk_list_end_selection',                       'E',
                'gtk_list_extend_selection',                    'E',
                'gtk_list_insert_items',                        'E',
                'gtk_list_item_deselect',                       'E',
                'gtk_list_item_new',                            'E',
                'gtk_list_item_new_with_label',                 'E',
                'gtk_list_item_select',                         'E',
                'gtk_list_new',                                 'E',
                'gtk_list_prepend_items',                       'E',
                'gtk_list_remove_items',                        'E',
                'gtk_list_remove_items_no_unref',               'E',
                'gtk_list_scroll_horizontal',                   'E',
                'gtk_list_scroll_vertical',                     'E',
                'gtk_list_select_all',                          'E',
                'gtk_list_select_child',                        'E',
                'gtk_list_select_item',                         'E',
                'gtk_list_set_selection_mode',                  'E',
                'gtk_list_start_selection',                     'E',
                'gtk_list_toggle_add_mode',                     'E',
                'gtk_list_toggle_focus_row',                    'E',
                'gtk_list_toggle_row',                          'E',
                'gtk_list_undo_selection',                      'E',
                'gtk_list_unselect_all',                        'E',
                'gtk_list_unselect_child',                      'E',
                'gtk_list_unselect_item',                       'E',
                'gtk_menu_append',                              'E', # gtk_menu_shell_append() [==?]
                'gtk_menu_bar_append',                          'E',
                'gtk_menu_bar_insert',                          'E',
                'gtk_menu_bar_prepend',                         'E',
                'gtk_menu_insert',                              'E',
                'gtk_menu_item_remove_submenu',                 'E',
                'gtk_menu_item_right_justify',                  'E',
                'gtk_menu_prepend',                             'E', # gtk_menu_shell_prepend() [==?]
                'gtk_menu_tool_button_set_arrow_tooltip',       'E',
                'gtk_notebook_current_page',                    'E',
                'gtk_notebook_get_group',                       'E', # since 2.24
                'gtk_notebook_get_group_id',                    'E',
                'gtk_notebook_query_tab_label_packing',         'E', # since 2.20
                'gtk_nitebook_set_group',                       'E', # since 2.24
                'gtk_notebook_set_group_id',                    'E',
                'gtk_notebook_set_homogeneous_tabs',            'E',
                'gtk_notebook_set_page',                        'E', # gtk_notebook_set_current_page() [==]
                'gtk_notebook_set_tab_border',                  'E',
                'gtk_notebook_set_tab_hborder',                 'E',
                'gtk_notebook_set_tab_label_packing',           'E', # since 2.20
                'gtk_notebook_set_tab_vborder',                 'E',
                'gtk_notebook_set_window_creation_hook',        'E', # since 2.24
                'gtk_object_add_arg_type',                      'E',
                'gtk_object_data_force_id',                     'E',
                'gtk_object_data_try_key',                      'E',
                'gtk_object_destroy',                           'E', # since 2.24
                'GTK_OBJECT_FLAGS',                             'E', # since 2.22
                'GTK_OBJECT_FLOATING',                          'E',
                'gtk_object_get',                               'E',
                'gtk_object_get_data',                          'E',
                'gtk_object_get_data_by_id',                    'E',
                'gtk_object_get_user_data',                     'E',
                'gtk_object_new',                               'E',
                'gtk_object_ref',                               'E',
                'gtk_object_remove_data',                       'E',
                'gtk_object_remove_data_by_id',                 'E',
                'gtk_object_remove_no_notify',                  'E',
                'gtk_object_remove_no_notify_by_id',            'E',
                'gtk_object_set',                               'E',
                'gtk_object_set_data',                          'E',
                'gtk_object_set_data_by_id',                    'E',
                'gtk_object_set_data_by_id_full',               'E',
                'gtk_object_set_data_full',                     'E',
                'gtk_object_set_user_data',                     'E',
                'gtk_object_sink',                              'E',
                'GTK_OBJECT_TYPE',                              'E', # G_OBJECT_TYPE
                'GTK_OBJECT_TYPE_NAME',                         'E', # G_OBJECT_TYPE_NAME
                'gtk_object_unref',                             'E',
                'gtk_object_weakref',                           'E',
                'gtk_object_weakunref',                         'E',
                'gtk_old_editable_changed',                     'E',
                'gtk_old_editable_claim_selection',             'E',
                'gtk_option_menu_get_history',                  'E', # GtkComboBox ... (avail since 2.4/2.6/2.10/2.14)
                'gtk_option_menu_get_menu',                     'E',
                'gtk_option_menu_new',                          'E',
                'gtk_option_menu_remove_menu',                  'E',
                'gtk_option_menu_set_history',                  'E',
                'gtk_option_menu_set_menu',                     'E',
                'gtk_paint_string',                             'E',
                'gtk_paned_gutter_size',                        'E', # gtk_paned_set_gutter_size()
                'gtk_paned_set_gutter_size',                    'E', # "does nothing"
                'gtk_pixmap_get',                               'E', # GtkImage ...
                'gtk_pixmap_new',                               'E',
                'gtk_pixmap_set',                               'E',
                'gtk_pixmap_set_build_insensitive',             'E',
                'gtk_preview_draw_row',                         'E',
                'gtk_preview_get_cmap',                         'E',
                'gtk_preview_get_info',                         'E',
                'gtk_preview_get_visual',                       'E',
                'gtk_preview_new',                              'E',
                'gtk_preview_put',                              'E',
                'gtk_preview_reset',                            'E',
                'gtk_preview_set_color_cube',                   'E',
                'gtk_preview_set_dither',                       'E',
                'gtk_preview_set_expand',                       'E',
                'gtk_preview_set_gamma',                        'E',
                'gtk_preview_set_install_cmap',                 'E',
                'gtk_preview_set_reserved',                     'E',
                'gtk_preview_size',                             'E',
                'gtk_preview_uninit',                           'E',
                'GTK_PRIORITY_DEFAULT',                         'E',
                'GTK_PRIORITY_HIGH',                            'E',
                'GTK_PRIORITY_INTERNAL',                        'E',
                'GTK_PRIORITY_LOW',                             'E',
                'GTK_PRIORITY_REDRAW',                          'E',
                'gtk_progress_bar_new_with_adjustment',         'E',
                'gtk_progress_bar_set_activity_blocks',         'E',
                'gtk_progress_bar_set_activity_step',           'E',
                'gtk_progress_bar_set_bar_style',               'E',
                'gtk_progress_bar_set_discrete_blocks',         'E',
                'gtk_progress_bar_update',                      'E', # >>> "gtk_progress_set_value() or
                                                                     #    gtk_progress_set_percentage()"
                                                                     ##  Actually: GtkProgress is deprecated so the
                                                                     ##  right answer appears to be to use
                                                                     ##  gtk_progress_bar_set_fraction()
                'gtk_progress_configure',                       'E',
                'gtk_progress_get_current_percentage',          'E',
                'gtk_progress_get_current_text',                'E',
                'gtk_progress_get_percentage_from_value',       'E',
                'gtk_progress_get_text_from_value',             'E',
                'gtk_progress_get_value',                       'E',
                'gtk_progress_set_activity_mode',               'E',
                'gtk_progress_set_adjustment',                  'E',
                'gtk_progress_set_format_string',               'E',
                'gtk_progress_set_percentage',                  'E',
                'gtk_progress_set_show_text',                   'E',
                'gtk_progress_set_text_alignment',              'E',
                'gtk_progress_set_value',                       'E',
                'gtk_quit_add',                                 'E', # removed in 3.0
                'gtk_quit_add_destroy',                         'E',
                'gtk_quit_add_full',                            'E',
                'gtk_quit_remove',                              'E',
                'gtk_quit_remove_by_data',                      'E',
                'gtk_radio_button_group',                       'E', # gtk_radio_button_get_group() [==]
                'gtk_radio_menu_item_group',                    'E',
                'gtk_range_get_update_policy',                  'E',
                'gtk_range_set_update_policy',                  'E',
                'gtk_rc_add_class_style',                       'E',
                'gtk_rc_add_widget_class_style',                'E',
                'gtk_rc_add_widget_name_style',                 'E',
                'gtk_rc_style_ref',                             'E',
                'gtk_rc_style_unref',                           'E',
                'gtk_recent_chooser_get_show_numbers',          'E',
                'gtk_recent_chooser_set_show_numbers',          'E',
                'gtk_recent_manager_get_for_screen',            'E',
                'gtk_recent_manager_get_limit',                 'E', # Use GtkRecentChooser
                'gtk_recent_manager_set_limit',                 'E', #
                'gtk_recent_manager_set_screen',                'E',
                'GTK_RETLOC_BOOL',                              'E',
                'GTK_RETLOC_BOXED',                             'E',
                'GTK_RETLOC_CHAR',                              'E',
                'GTK_RETLOC_DOUBLE',                            'E',
                'GTK_RETLOC_ENUM',                              'E',
                'GTK_RETLOC_FLAGS',                             'E',
                'GTK_RETLOC_FLOAT',                             'E',
                'GTK_RETLOC_INT',                               'E',
                'GTK_RETLOC_LONG',                              'E',
                'GTK_RETLOC_OBJECT',                            'E',
                'GTK_RETLOC_POINTER',                           'E',
                'GTK_RETLOC_STRING',                            'E',
                'GTK_RETLOC_UCHAR',                             'E',
                'GTK_RETLOC_UINT',                              'E',
                'GTK_RETLOC_ULONG',                             'E',
                'gtk_ruler_get_metric',                         'E',
                'gtk_ruler_get_range',                          'E',
                'gtk_ruler_set_metric',                         'E',
                'gtk_ruler_set_range',                          'E',
                'gtk_scale_button_get_orientation',             'E', # gtk_orientable_get_orientation()         (avail since 2.16)
                'gtk_scale_button_set_orientation',             'E', # gtk_orientable_set_orientation()         (avail since 2.16)
                'gtk_status_icon_set_tooltip',                  'E', # gtk_status_icon_set_tooltip_text()       (avail since 2.16)
                'gtk_selection_clear',                          'E',
                'gtk_set_locale',                               'E',
                'gtk_signal_connect',                           'E', # GSignal ...
                'gtk_signal_connect_after',                     'E',
                'gtk_signal_connect_full',                      'E',
                'gtk_signal_connect_object',                    'E',
                'gtk_signal_connect_object_after',              'E',
                'gtk_signal_connect_object_while_alive',        'E',
                'gtk_signal_connect_while_alive',               'E',
                'gtk_signal_default_marshaller',                'E',
                'gtk_signal_disconnect',                        'E',
                'gtk_signal_disconnect_by_data',                'E',
                'gtk_signal_disconnect_by_func',                'E',
                'gtk_signal_emit',                              'E',
                'gtk_signal_emit_by_name',                      'E',
                'gtk_signal_emit_stop',                         'E',
                'gtk_signal_emit_stop_by_name',                 'E',
                'gtk_signal_emitv',                             'E',
                'gtk_signal_emitv_by_name',                     'E',
                'GTK_SIGNAL_FUNC',                              'E',
                'gtk_signal_handler_block',                     'E',
                'gtk_signal_handler_block_by_data',             'E',
                'gtk_signal_handler_block_by_func',             'E',
                'gtk_signal_handler_pending',                   'E',
                'gtk_signal_handler_pending_by_func',           'E',
                'gtk_signal_handler_unblock',                   'E',
                'gtk_signal_handler_unblock_by_data',           'E',
                'gtk_signal_handler_unblock_by_func',           'E',
                'gtk_signal_lookup',                            'E',
                'gtk_signal_name',                              'E',
                'gtk_signal_new',                               'E',
                'gtk_signal_newv',                              'E',
                'GTK_SIGNAL_OFFSET',                            'E',
                'gtk_socket_steal',                             'E',
                'gtk_spin_button_get_value_as_float',           'E', # gtk_spin_button_get_value() [==]
                'gtk_status_icon_get_blinking',                 'E',
                'gtk_status_icon_set_blinking',                 'E',
                'GTK_STRUCT_OFFSET',                            'E',
                'gtk_style_apply_default_pixmap',               'E',
                'gtk_style_get_font',                           'E',
                'gtk_style_ref',                                'E',
                'gtk_style_set_font',                           'E',
                'gtk_style_unref',                              'E', # g_object_unref() [==?]
                'gtk_text_backward_delete',                     'E',
                'gtk_text_forward_delete',                      'E',
                'gtk_text_freeze',                              'E',
                'gtk_text_get_length',                          'E',
                'gtk_text_get_point',                           'E',
                'GTK_TEXT_INDEX',                               'E',
                'gtk_text_insert',                              'E', # GtkTextView (GtkText "known to be buggy" !)
                'gtk_text_new',                                 'E',
                'gtk_text_set_adjustments',                     'E',
                'gtk_text_set_editable',                        'E',
                'gtk_text_set_line_wrap',                       'E',
                'gtk_text_set_point',                           'E',
                'gtk_text_set_word_wrap',                       'E',
                'gtk_text_thaw',                                'E',
                'gtk_timeout_add',                              'E', # g_timeout_add()
                'gtk_timeout_add_full',                         'E',
                'gtk_timeout_remove',                           'E', # g_source_remove()
                'gtk_tips_query_new',                           'E',
                'gtk_tips_query_set_caller',                    'E',
                'gtk_tips_query_set_labels',                    'E',
                'gtk_tips_query_start_query',                   'E',
                'gtk_tips_query_stop_query',                    'E',
                'gtk_toggle_button_set_state',                  'E', # gtk_toggle_button_set_active [==]
                'gtk_toolbar_append_element',                   'E',
                'gtk_toolbar_append_item',                      'E',
                'gtk_toolbar_append_space',                     'E', # Use gtk_toolbar_insert() instead
                'gtk_toolbar_append_widget',                    'E', # ??
                'gtk_toolbar_get_orientation',                  'E', # gtk_orientable_get_orientation()         (avail since 2.16)
                'gtk_toolbar_get_tooltips',                     'E',
                'gtk_toolbar_insert_element',                   'E',
                'gtk_toolbar_insert_item',                      'E',
                'gtk_toolbar_insert_space',                     'E',
                'gtk_toolbar_insert_stock',                     'E',
                'gtk_toolbar_insert_widget',                    'E',
                'gtk_toolbar_prepend_element',                  'E',
                'gtk_toolbar_prepend_item',                     'E',
                'gtk_toolbar_prepend_space',                    'E',
                'gtk_toolbar_prepend_widget',                   'E',
                'gtk_toolbar_remove_space',                     'E',
                'gtk_toolbar_set_orientation',                  'E', # gtk_orientable_set_orientation()         (avail since 2.16)
                'gtk_toolbar_set_tooltips',                     'E',
                'gtk_tooltips_data_get',                        'E', # new API: GtkToolTip (avail since 2.12) ...
                'gtk_tooltips_disable',                         'E',
                'gtk_tooltips_enable',                          'E',
                'gtk_tooltips_force_window',                    'E',
                'gtk_tooltips_get_info_from_tip_window',        'E',
                'gtk_tooltips_new',                             'E',
                'gtk_tooltips_set_delay',                       'E',
                'gtk_tooltips_set_tip',                         'E',
                'gtk_tool_item_set_tooltip',                    'E', # gtk_tool_item_set_tooltip_text() (avail since 2.12)
                'gtk_tree_append',                              'E',
                'gtk_tree_child_position',                      'E',
                'gtk_tree_clear_items',                         'E',
                'gtk_tree_insert',                              'E',
                'gtk_tree_item_collapse',                       'E',
                'gtk_tree_item_deselect',                       'E',
                'gtk_tree_item_expand',                         'E',
                'gtk_tree_item_new',                            'E',
                'gtk_tree_item_new_with_label',                 'E',
                'gtk_tree_item_remove_subtree',                 'E',
                'gtk_tree_item_select',                         'E',
                'gtk_tree_item_set_subtree',                    'E',
                'GTK_TREE_ITEM_SUBTREE',                        'E',
                'gtk_tree_model_get_iter_root',                 'E',
                'gtk_tree_new',                                 'E',
                'gtk_tree_path_new_root',                       'E',
                'gtk_tree_prepend',                             'E',
                'gtk_tree_remove_item',                         'E',
                'gtk_tree_remove_items',                        'E',
                'GTK_TREE_ROOT_TREE',                           'E',
                'gtk_tree_select_child',                        'E',
                'gtk_tree_select_item',                         'E',
                'GTK_TREE_SELECTION_OLD',                       'E',
                'gtk_tree_set_selection_mode',                  'E',
                'gtk_tree_set_view_lines',                      'E',
                'gtk_tree_set_view_mode',                       'E',
                'gtk_tree_unselect_child',                      'E',
                'gtk_tree_unselect_item',                       'E',
                'gtk_tree_view_column_get_cell_renderers',      'E', # gtk_cell_layout_get_cells ()             (avail since 2.12)
                'gtk_tree_view_tree_to_widget_coords',          'E',
                'gtk_tree_view_widget_to_tree_coords',          'E',
                'gtk_type_class',                               'E', # g_type_class_peek() or g_type_class_ref()
                'GTK_TYPE_CTREE_NODE',                          'E',
                'gtk_type_enum_find_value',                     'E',
                'gtk_type_enum_get_values',                     'E',
                'gtk_type_flags_find_value',                    'E',
                'gtk_type_flags_get_values',                    'E',
                'gtk_type_from_name',                           'E',
                'gtk_type_init',                                'E',
                'gtk_type_is_a',                                'E',
                'GTK_TYPE_FUNDAMENTAL_LAST',                    'E',
                'GTK_TYPE_FUNDAMENTAL_MAX',                     'E',
                'GTK_TYPE_IS_OBJECT',                           'E',
                'gtk_type_name',                                'E',
                'gtk_type_new',                                 'E',
                'gtk_type_parent',                              'E',
                'gtk_type_unique',                              'E',
                'GTK_VALUE_BOOL',                               'E',
                'GTK_VALUE_BOXED',                              'E',
                'GTK_VALUE_CHAR',                               'E',
                'GTK_VALUE_DOUBLE',                             'E',
                'GTK_VALUE_ENUM',                               'E',
                'GTK_VALUE_FLAGS',                              'E',
                'GTK_VALUE_FLOAT',                              'E',
                'GTK_VALUE_INT',                                'E',
                'GTK_VALUE_LONG',                               'E',
                'GTK_VALUE_OBJECT',                             'E',
                'GTK_VALUE_POINTER',                            'E',
                'GTK_VALUE_SIGNAL',                             'E',
                'GTK_VALUE_STRING',                             'E',
                'GTK_VALUE_UCHAR',                              'E',
                'GTK_VALUE_UINT',                               'E',
                'GTK_VALUE_ULONG',                              'E',
                'gtk_vbox_new',                                 'W', # ws_gtk_box_new
                'gtk_vbutton_box_get_layout_default',           'E',
                'gtk_vbutton_box_get_spacing_default',          'E',
                'gtk_vbutton_box_set_layout_default',           'E',
                'gtk_vbutton_box_set_spacing_default',          'E',
                'gtk_vruler_new',                               'E',
                'GTK_WIDGET_APP_PAINTABLE',                     'E', # gtk_widget_get_app_paintable()    (avail since 2.18)
                'GTK_WIDGET_CAN_DEFAULT',                       'E', # gtk_widget_get_can_default()      (avail since 2.18)
                'GTK_WIDGET_CAN_FOCUS',                         'E', # gtk_widget_get_can_focus()        (avail since 2.18)
                'GTK_WIDGET_COMPOSITE_CHILD',                   'E', # gtk_widget_get_composite_child()  (avail since 2.18)
                'GTK_WIDGET_DOUBLE_BUFFERED',                   'E', # gtk_widget_get_double_buffered()  (avail since 2.18)
                'GTK_WIDGET_DRAWABLE',                          'E', # gtk_widget_get_drawable()         (avail since 2.18)
                'GTK_WIDGET_FLAGS',                             'E', # gtk_widget_get_flags()            (avail since 2.18)
                'GTK_WIDGET_HAS_DEFAULT',                       'E', # gtk_widget_get_has_default()      (avail since 2.18)
                'GTK_WIDGET_HAS_FOCUS',                         'E', # gtk_widget_get_has_focus()        (avail since 2.18)
                'GTK_WIDGET_HAS_GRAB',                          'E', # gtk_widget_get_has_grab()         (avail since 2.18)
                'GTK_WIDGET_IS_SENSITIVE',                      'E', # gtk_widget_get_is_sensitive()     (avail since 2.18)
                'GTK_WIDGET_MAPPED',                            'E', # gtk_widget_get_mapped()           (avail since 2.18)
                'GTK_WIDGET_NO_WINDOW',                         'E', # gtk_widget_get_no_window()        (avail since 2.18)
                'GTK_WIDGET_PARENT_SENSITIVE',                  'E', # gtk_widget_get_parent_sensitive() (avail since 2.18)
                'GTK_WIDGET_RC_STYLE',                          'E', # gtk_widget_get_rc_style()         (avail since 2.18)
                'GTK_WIDGET_REALIZED',                          'E', # gtk_widget_get_realized()         (avail since 2.18)
                'GTK_WIDGET_RECEIVES_DEFAULT',                  'E', # gtk_widget_get_receives_default() (avail since 2.18)
                'GTK_WIDGET_SAVED_STATE',                       'E', # gtk_widget_get_saved_state()      (avail since 2.18)
                'GTK_WIDGET_SENSITIVE',                         'E', # gtk_widget_get_sensitive()        (avail since 2.18)
                'GTK_WIDGET_SET_FLAGS',                         'W', # since GTK 2.22
                'GTK_WIDGET_STATE',                             'E', # gtk_widget_get_state()            (avail since 2.18)
                'GTK_WIDGET_TOPLEVEL',                          'E', # gtk_widget_get_toplevel()         (avail since 2.18)
                'GTK_WIDGET_TYPE',                              'E', # gtk_widget_get_type()             (avail since 2.18)
                'GTK_WIDGET_UNSET_FLAGS',                       'E',
                'GTK_WIDGET_VISIBLE',                           'E', # gtk_widget_get_visible()          (avail since 2.18)
                'gtk_widget_draw',                              'E', # gtk_widget_queue_draw_area():
                                                                     #  "in general a better choice if you want
                                                                     #  to draw a region of a widget."
                'gtk_widget_get_action',                        'E', # gtk_activatable_get_related_action() (avail since 2.16)
                'gtk_widget_hide_all',                          'E',
                'gtk_widget_pop_visual',                        'E',
                'gtk_widget_push_visual',                       'E',
                'gtk_widget_queue_clear',                       'E',
                'gtk_widget_queue_clear_area',                  'E',
                'gtk_widget_ref',                               'E', # g_object_ref() [==]
                'gtk_widget_reset_shapes',                      'E',
                'gtk_widget_restore_default_style',             'E',
                'gtk_widget_set',                               'E', # g_object_set() [==]
                'gtk_widget_set_default_visual',                'E',
                'gtk_widget_set_rc_style',                      'E',
                'gtk_widget_set_uposition',                     'E', # ?? (see GTK documentation)
                'gtk_widget_set_usize',                         'E', # gtk_widget_set_size_request()
                'gtk_widget_set_visual',                        'E',
                'gtk_widget_unref',                             'E',
                'gtk_window_get_frame_dimensions',              'E',
                'gtk_window_get_has_frame',                     'E',
                'gtk_window_set_frame_dimensions',              'E',
                'gtk_window_set_has_frame',                     'E',
                'gtk_window_position',                          'E',
                'gtk_window_set_policy',                        'E', # >>? gtk_window_set_resizable()

## GDK deprecated functions:
                'gdk_bitmap_create_from_data',                  'E', #
                'gdk_bitmap_ref',                               'E', #
                'gdk_bitmap_unref',                             'E', #
                'gdk_cairo_set_source_pixmap',                  'W', # deprecated since version 2.24.
                                                                     # Use gdk_cairo_set_source_window() where appropriate(Since 2.24).
                'gdk_char_height',                              'E', #
                'gdk_char_measure',                             'E', #
                'gdk_char_width',                               'E', #
                'gdk_char_width_wc',                            'E', #
                'gdk_colormap_change',                          'E', #
                'gdk_colormap_get_system_size',                 'E', #
                'gdk_colormap_ref',                             'E', #
                'gdk_colormap_unref',                           'E', #
                'gdk_colors_alloc',                             'E', #
                'gdk_colors_free',                              'E', #
                'gdk_colors_store',                             'E', #
                'gdk_color_alloc',                              'E', #
                'gdk_color_black',                              'E', #
                'gdk_color_change',                             'E', #
                'gdk_color_white',                              'E', #
                'gdk_cursor_destroy',                           'E', #
                'GdkDestroyNotify',                             'E', #
                'gdk_DISPLAY',                                  'E', #
                'gdk_display_set_pointer_hooks',                'E', #
                'gdk_drag_context_new',                         'E', #
                'gdk_drag_context_ref',                         'E', #
                'gdk_drag_context_unref',                       'E', #
                'gdk_drag_find_window',                         'E', #
                'gdk_drag_get_protocol',                        'E', #
                'gdk_drawable_copy_to_image',                   'E', #
                'gdk_drawable_get_data',                        'E', #
                'gdk_drawable_get_display',                     'E', #
                'gdk_drawable_get_image',                       'E', #
                'gdk_drawable_get_screen',                      'E', #
                'gdk_drawable_get_size',                        'W', # deprecated since version 2.24 Use gdk_window_get_width()
                                                                     #  and gdk_window_get_height() for GdkWindows.
                                                                     # Use gdk_pixmap_get_size() for GdkPixmaps
                'gdk_drawable_get_visual',                      'E', #
                'gdk_drawable_ref',                             'E', #
                'gdk_drawable_set_data',                        'E', #
                'gdk_drawable_unref',                           'E', #
                'gdk_draw_arc',                                 'E', # deprecated since version 2.22. Use cairo_arc() and
                                                                     #  cairo_fill() or cairo_stroke() instead.
                'gdk_draw_drawable',                            'E', # deprecated since version 2.22. Use  gdk_cairo_set_source_pixmap(),
                                                                     #  cairo_rectangle() and cairo_fill() to draw pixmap
                                                                     #  on top of other drawables
                'gdk_draw_glyphs',                              'E', #
                'gdk_draw_glyphs_transformed',                  'E', #
                'gdk_draw_gray_image',                          'E', #
                'gdk_draw_image',                               'E', #
                'gdk_draw_indexed_image',                       'E', #
                'gdk_draw_layout',                              'E', #
                'gdk_draw_layout_line',                         'E', #
                'gdk_draw_layout_line_with_colors',             'E', #
                'gdk_draw_layout_with_colors',                  'E', #
                'gdk_draw_line',                                'W', # deprecated since version 2.22. Use cairo_line_to() and cairo_stroke()
                'gdk_draw_lines',                               'E', # deprecated since version 2.22. Use cairo_line_to() and cairo_stroke()
                'gdk_draw_pixbuf',                              'E', # gdk_cairo_set_source_pixbuf() and cairo_paint() or
                                                                     #  cairo_rectangle() and cairo_fill() instead.
                'gdk_draw_pixmap',                              'E', # gdk_draw_drawable() (gdk_draw_drawable has been
                                                                     #  deprecated since version 2.22 )
                'gdk_draw_point',                               'E', #
                'gdk_draw_points',                              'E', #
                'gdk_draw_polygon',                             'E', # deprecated since version 2.22. Use cairo_line_to()
                                                                     #  or cairo_append_path() and cairo_fill()
                                                                     #  or cairo_stroke() instead.
                'gdk_draw_rectangle',                           'E', # deprecated since version 2.22, Use cairo_rectangle()
                                                                     #  and cairo_fill() or cairo_stroke()
                'gdk_draw_rgb_32_image',                        'E', #
                'gdk_draw_rgb_32_image_dithalign',              'E', #
                'gdk_draw_rgb_image',                           'E', #
                'gdk_draw_rgb_image_dithalign',                 'E', #
                'gdk_draw_segments',                            'E', #
                'gdk_draw_string',                              'E', #
                'gdk_draw_text',                                'E', #
                'gdk_draw_text_wc',                             'E', #
                'gdk_draw_trapezoids',                          'E', #
                'gdk_event_get_graphics_expose',                'E', #
                'gdk_exit',                                     'E', #
                'GdkFillRule',                                  'E', #
                'GdkFont',                                      'E', #
                'gdk_fontset_load',                             'E', #
                'gdk_fontset_load_for_display',                 'E', #
                'GdkFontType',                                  'E', #
                'gdk_font_equal',                               'E', #
                'gdk_font_from_description',                    'E', #
                'gdk_font_from_description_for_display',        'E', #
                'gdk_font_get_display',                         'E', #
                'gdk_font_id',                                  'E', #
                'gdk_font_load',                                'E', #
                'gdk_font_load_for_display',                    'E', #
                'gdk_font_lookup',                              'E', #
                'gdk_font_lookup_for_display',                  'E', #
                'gdk_font_ref',                                 'E', #
                'gdk_font_unref',                               'E', #
                'gdk_FONT_XDISPLAY',                            'E', #
                'gdk_FONT_XFONT',                               'E', #
                'gdk_free_compound_text',                       'E', #
                'gdk_free_text_list',                           'E', #
                'gdk_gc_copy',                                  'E', #
                'gdk_gc_destroy',                               'E', #
                'gdk_gc_get_colormap',                          'E', #
                'gdk_gc_get_screen',                            'E', #
                'gdk_gc_get_values',                            'E', #
                'gdk_gc_new',                                   'W', # deprecated since version 2.22 and should not be used
                                                                     #  in newly-written code. Use Cairo for rendering.
                'gdk_gc_new_with_values',                       'E', # deprecated since version 2.22
                'gdk_gc_offset',                                'E', #
                'gdk_gc_ref',                                   'E', #
                'gdk_gc_set_background',                        'E', #
                'gdk_gc_set_clip_mask',                         'E', #
                'gdk_gc_set_clip_origin',                       'E', #
                'gdk_gc_set_clip_rectangle',                    'E', #
                'gdk_gc_set_clip_region',                       'E', #
                'gdk_gc_set_colormap',                          'E', #
                'gdk_gc_set_dashes',                            'E', #
                'gdk_gc_set_exposures',                         'E', #
                'gdk_gc_set_fill',                              'E', # deprecated since version 2.22. Use cairo_pattern_set_extend()
                                                                     #  on the source.
                'gdk_gc_set_font',                              'E', #
                'gdk_gc_set_foreground',                        'W', # deprecated since version 2.22. Use gdk_cairo_set_source_color()
                                                                     #  to use a GdkColor as the source in Cairo.
                'gdk_gc_set_function',                          'W', # deprecated since version 2.22. Use cairo_set_operator() with Cairo.
                'gdk_gc_set_line_attributes',                   'E', #
                'gdk_gc_set_rgb_bg_color',                      'E', #
                'gdk_gc_set_rgb_fg_color',                      'E', # deprecated since version 2.22.
                                                                     #  Use gdk_cairo_set_source_color() instead.
                'gdk_gc_set_stipple',                           'E', #
                'gdk_gc_set_subwindow',                         'E', #
                'gdk_gc_set_tile',                              'E', # deprecated since version 2.22.
                                                                     # The following code snippet sets a tiling GdkPixmap as the
                                                                     #   source in Cairo:
                                                                     # gdk_cairo_set_source_pixmap (cr, tile, ts_origin_x, ts_origin_y);
                                                                     # cairo_pattern_set_extend (cairo_get_source (cr),
                                                                     #  CAIRO_EXTEND_REPEAT);
                'gdk_gc_set_ts_origin',                         'E', #
                'gdk_gc_set_values',                            'E', #
                'gdk_gc_unref',                                 'E', # deprecated since version 2.0. Use g_object_unref()
                'gdk_get_use_xshm',                             'E', #
                'gdk_image_destroy',                            'E', #
                'gdk_image_get',                                'E', #
                'gdk_image_get_bits_per_pixel',                 'E', #
                'gdk_image_get_bytes_per_line',                 'E', #
                'gdk_image_get_bytes_per_pixel',                'E', #
                'gdk_image_get_byte_order',                     'E', #
                'gdk_image_get_colormap',                       'E', #
                'gdk_image_get_depth',                          'E', #
                'gdk_image_get_height',                         'E', #
                'gdk_image_get_image_type',                     'E', #
                'gdk_image_get_pixel',                          'E', #
                'gdk_image_get_pixels',                         'E', #
                'gdk_image_get_visual',                         'E', #
                'gdk_image_get_width',                          'E', #
                'gdk_image_new',                                'E', #
                'gdk_image_new_bitmap',                         'E', #
                'gdk_image_put_pixel',                          'E', #
                'gdk_image_ref',                                'E', #
                'gdk_image_set_colormap',                       'E', #
                'gdk_image_unref',                              'E', #
                'gdk_input_add',                                'E', #
                'gdk_input_add_full',                           'E', #
                'gdk_input_remove',                             'E', #
                'gdk_mbstowcs',                                 'E', #
                'gdk_net_wm_supports',                          'E', #
                'gdk_pango_context_set_colormap',               'E', #
                'gdk_pixbuf_render_to_drawable',                'E', #
                'gdk_pixbuf_render_to_drawable_alpha',          'E', #
                'gdk_pixmap_colormap_create_from_xpm',          'E', #
                'gdk_pixmap_colormap_create_from_xpm_d',        'E', #
                'gdk_pixmap_create_from_data',                  'E', #
                'gdk_pixmap_create_from_xpm',                   'E', #
                'gdk_pixmap_create_from_xpm_d',                 'E', # deprecated since version 2.22. Use a GdkPixbuf instead. You can
                                                                     #  use gdk_pixbuf_new_from_xpm_data() to create it.
                                                                     # If you must use a pixmap, use gdk_pixmap_new() to create it
                                                                     #  and Cairo to draw the pixbuf onto it.
                'gdk_pixmap_ref',                               'E', #
                'gdk_pixmap_unref',                             'E', # Deprecated equivalent of g_object_unref().
                'gdk_region_polygon',                           'E', #
                'gdk_region_rect_equal',                        'E', #
                'gdk_region_shrink',                            'E', #
                'gdk_region_spans_intersect_foreach',           'E', #
                'GdkRgbCmap',                                   'E', #
                'gdk_rgb_cmap_free',                            'E', #
                'gdk_rgb_cmap_new',                             'E', #
                'gdk_rgb_colormap_ditherable',                  'E', #
                'gdk_rgb_ditherable',                           'E', #
                'gdk_rgb_find_color',                           'E', #
                'gdk_rgb_gc_set_background',                    'E', #
                'gdk_rgb_gc_set_foreground',                    'E', #
                'gdk_rgb_get_cmap',                             'E', #
                'gdk_rgb_get_colormap',                         'E', #
                'gdk_rgb_get_visual',                           'E', #
                'gdk_rgb_init',                                 'E', #
                'gdk_rgb_set_install',                          'E', #
                'gdk_rgb_set_min_colors',                       'E', #
                'gdk_rgb_set_verbose',                          'E', #
                'gdk_rgb_xpixel_from_rgb',                      'E', #
                'gdk_ROOT_PARENT',                              'E', #
                'gdk_screen_get_rgb_colormap',                  'E', #
                'gdk_screen_get_rgb_visual',                    'E', #
                'GdkSelection',                                 'E', #
                'GdkSelectionType',                             'E', #
                'gdk_set_locale',                               'E', #
                'gdk_set_pointer_hooks',                        'E', #
                'gdk_set_sm_client_id',                         'E', #
                'gdk_set_use_xshm',                             'E', #
                'GdkSpanFunc',                                  'E', #
                'gdk_spawn_command_line_on_screen',             'E', #
                'gdk_spawn_on_screen',                          'E', #
                'gdk_spawn_on_screen_with_pipes',               'E', #
                'gdk_string_extents',                           'E', #
                'gdk_string_height',                            'E', #
                'gdk_string_measure',                           'E', #
                'gdk_string_to_compound_text',                  'E', #
                'gdk_string_to_compound_text_for_display',      'E', #
                'gdk_string_width',                             'E', #
                'GdkTarget',                                    'E', #
                'gdk_text_extents',                             'E', #
                'gdk_text_extents_wc',                          'E', #
                'gdk_text_height',                              'E', #
                'gdk_text_measure',                             'E', #
                'gdk_text_property_to_text_list',               'E', #
                'gdk_text_property_to_text_list_for_display',   'E', #
                'gdk_text_property_to_utf8_list',               'E', #
                'gdk_text_width',                               'E', #
                'gdk_text_width_wc',                            'E', #
                'gdk_threads_mutex',                            'E', #
                'gdk_utf8_to_compound_text',                    'E', #
                'gdk_utf8_to_compound_text_for_display',        'E', #
                'gdk_visual_ref',                               'E', #
                'gdk_visual_unref',                             'E', #
                'gdk_wcstombs',                                 'E', #
                'gdk_window_copy_area',                         'E', #
                'gdk_window_foreign_new',                       'E', #
                'gdk_window_foreign_new_for_display',           'E', #
                'gdk_window_get_colormap',                      'E', # Deprecated equivalent of gdk_drawable_get_colormap().
                'gdk_window_get_deskrelative_origin',           'E', #
                'gdk_window_get_size',                          'E', # Deprecated equivalent of gdk_drawable_get_size().
                'gdk_window_get_toplevels',                     'E', #
                'gdk_window_get_type',                          'E', #
                'gdk_window_lookup',                            'E', #
                'gdk_window_lookup_for_display',                'E', #
                'gdk_window_ref',                               'E', #
                'gdk_window_set_colormap',                      'E', #
                'gdk_window_set_hints',                         'E', #
                'gdk_window_unref',                             'E', #
                'gdk_x11_font_get_name',                        'E', #
                'gdk_x11_font_get_xdisplay',                    'E', #
                'gdk_x11_font_get_xfont',                       'E', #
                'gdk_x11_gc_get_xdisplay',                      'E', #
                'gdk_x11_gc_get_xgc',                           'E', #
                'gdk_xid_table_lookup',                         'E', #
                'gdk_xid_table_lookup_for_display',             'E', #
                'gdkx_colormap_get',                            'E', #
                'gdkx_visual_get',                              'E', #

);

@{$APIs{'deprecated-gtk'}->{'functions'}}      = grep {$deprecatedGtkFunctions{$_} eq 'E'} keys %deprecatedGtkFunctions;
@{$APIs{'deprecated-gtk-todo'}->{'functions'}} = grep {$deprecatedGtkFunctions{$_} eq 'W'} keys %deprecatedGtkFunctions;



# Given a ref to a hash containing "functions" and "functions_count" entries:
# Determine if any item of the list of APIs contained in the array referenced by "functions"
# exists in the file.
# For each API which appears in the file:
#     Push the API onto the provided list;
#     Add the number of times the API appears in the file to the total count
#      for the API (stored as the value of the API key in the hash referenced by "function_counts").

sub findAPIinFile($$$)
{
        my ($groupHashRef, $fileContentsRef, $foundAPIsRef) = @_;

        for my $api ( @{$groupHashRef->{functions}} )
        {
                my $cnt = 0;
                while (${$fileContentsRef} =~ m/ \W $api \W* \( /gx)
                {
                        $cnt += 1;
                }
                if ($cnt > 0) {
                        push @{$foundAPIsRef}, $api;
                        $groupHashRef->{function_counts}->{$api} += 1;
                }
        }
}

# APIs which (generally) should not be called with an argument of tvb_get_ptr()
my @TvbPtrAPIs = (
        # Use NULL for the value_ptr instead of tvb_get_ptr() (only if the
        # given offset and length are equal) with these:
        'proto_tree_add_bytes_format',
        'proto_tree_add_bytes_format_value',
        'proto_tree_add_ether',
        # Use the tvb_* version of these:
        # Use tvb_bytes_to_str[_punct] instead of:
        'bytes_to_str',
        'bytes_to_str_punct',
        'SET_ADDRESS',
        'SET_ADDRESS_HF',
);

sub checkAPIsCalledWithTvbGetPtr($$$)
{
        my ($APIs, $fileContentsRef, $foundAPIsRef) = @_;

        for my $api (@{$APIs}) {
                my @items;
                my $cnt = 0;

                @items = (${$fileContentsRef} =~ m/ ($api [^;]* ; ) /xsg);
                while (@items) {
                        my ($item) = @items;
                        shift @items;
                        if ($item =~ / tvb_get_ptr /xos) {
                                $cnt += 1;
                        }
                }

                if ($cnt > 0) {
                        push @{$foundAPIsRef}, $api;
                }
        }
}

# List of possible shadow variable (Majority coming from Mac OS X..)
my @ShadowVariable = (
        'index',
        'time',
        'strlen',
        'system'
);

sub checkShadowVariable($$$)
{
        my ($groupHashRef, $fileContentsRef, $foundAPIsRef) = @_;

        for my $api ( @{$groupHashRef} )
        {
                my $cnt = 0;
                while (${$fileContentsRef} =~ m/ \s $api \s*+ [^\(\w] /gx)
                {
                        $cnt += 1;
                }
                if ($cnt > 0) {
                        push @{$foundAPIsRef}, $api;
                }
        }
}

sub check_snprintf_plus_strlen($$)
{
        my ($fileContentsRef, $filename) = @_;
        my @items;

        # This catches both snprintf() and g_snprint.
        # If we need to do more APIs, we can make this function look more like
        # checkAPIsCalledWithTvbGetPtr().
        @items = (${$fileContentsRef} =~ m/ (snprintf [^;]* ; ) /xsg);
        while (@items) {
                my ($item) = @items;
                shift @items;
                if ($item =~ / strlen\s*\( /xos) {
                        print STDERR "Warning: ".$filename." uses snprintf + strlen to assemble strings.\n";
                        last;
                }
        }
}

#### Regex for use when searching for value-string definitions
my $StaticRegex             = qr/ static \s+                                                            /xs;
my $ConstRegex              = qr/ const  \s+                                                            /xs;
my $Static_andor_ConstRegex = qr/ (?: $StaticRegex $ConstRegex | $StaticRegex | $ConstRegex)            /xs;
my $ValueStringRegex        = qr/ ^ \s* $Static_andor_ConstRegex (?:value|string|range)_string \ + [^;*]+ = [^;]+ [{] .+? [}] \s*? ;  /xms;
my $EnumValRegex            = qr/ $Static_andor_ConstRegex enum_val_t \ + [^;*]+ = [^;]+ [{] .+? [}] \s*? ;  /xs;
my $NewlineStringRegex      = qr/ ["] [^"]* \\n [^"]* ["] /xs;

sub check_value_string_arrays($$$)
{
        my ($fileContentsRef, $filename, $debug_flag) = @_;
        my $cnt = 0;
        # Brute force check for value_string (and string_string or range_string) arrays
        # which are missing {0, NULL} as the final (terminating) array entry

        #  Assumption: definition is of form (pseudo-Regex):
        #    " (static const|static|const) (value|string|range)_string .+ = { .+ ;"
        #  (possibly over multiple lines)
        while (${$fileContentsRef} =~ / ( $ValueStringRegex ) /xsog) {
                # XXX_string array definition found; check if NULL terminated
                my $vs = my $vsx = $1;
                if ($debug_flag) {
                        $vsx =~ / ( .+ (?:value|string|range)_string [^=]+ ) = /xo;
                        printf STDERR "==> %-35.35s: %s\n", $filename, $1;
                        printf STDERR "%s\n", $vs;
                }
                $vs =~ s{ \s } {}xg;
                # README.developer says
                #  "Don't put a comma after the last tuple of an initializer of an array"
                # However: since this usage is present in some number of cases, we'll allow for now
                if ($vs !~ / , NULL [}] ,? [}] ; $/xo) {
                        $vsx =~ /( (?:value|string|range)_string [^=]+ ) = /xo;
                        printf STDERR "Error: %-35.35s: {..., NULL} is required as the last XXX_string array entry: %s\n", $filename, $1;
                        $cnt++;
                }
                if ($vs !~ / (static)? const (?:value|string|range)_string /xo)  {
                        $vsx =~ /( (?:value|string|range)_string [^=]+ ) = /xo;
                        printf STDERR "Error: %-35.35s: Missing 'const': %s\n", $filename, $1;
                        $cnt++;
                }
                if ($vs =~ / $NewlineStringRegex /xo)  {
                        $vsx =~ /( (?:value|string|range)_string [^=]+ ) = /xo;
                        printf STDERR "Error: %-35.35s: XXX_string contains a newline: %s\n", $filename, $1;
                        $cnt++;
                }
        }

        # Brute force check for enum_val_t arrays which are missing {NULL, NULL, ...}
        # as the final (terminating) array entry
        # For now use the same option to turn this and value_string checking on and off.
        # (Is the option even necessary?)

        #  Assumption: definition is of form (pseudo-Regex):
        #    " (static const|static|const) enum_val_t .+ = { .+ ;"
        #  (possibly over multiple lines)
        while (${$fileContentsRef} =~ / ( $EnumValRegex ) /xsog) {
                # enum_val_t array definition found; check if NULL terminated
                my $vs = my $vsx = $1;
                if ($debug_flag) {
                        $vsx =~ / ( .+ enum_val_t [^=]+ ) = /xo;
                        printf STDERR "==> %-35.35s: %s\n", $filename, $1;
                        printf STDERR "%s\n", $vs;
                }
                $vs =~ s{ \s } {}xg;
                # README.developer says
                #  "Don't put a comma after the last tuple of an initializer of an array"
                # However: since this usage is present in some number of cases, we'll allow for now
                if ($vs !~ / NULL, NULL, -?[0-9] [}] ,? [}] ; $/xo) {
                        $vsx =~ /( enum_val_t [^=]+ ) = /xo;
                        printf STDERR "Error: %-35.35s: {NULL, NULL, ...} is required as the last enum_val_t array entry: %s\n", $filename, $1;
                        $cnt++;
                }
                if ($vs !~ / (static)? const enum_val_t /xo)  {
                        $vsx =~ /( enum_val_t [^=]+ ) = /xo;
                        printf STDERR "Error: %-35.35s: Missing 'const': %s\n", $filename, $1;
                        $cnt++;
                }
                if ($vs =~ / $NewlineStringRegex /xo)  {
                        $vsx =~ /( (?:value|string|range)_string [^=]+ ) = /xo;
                        printf STDERR "Error: %-35.35s: enum_val_t contains a newline: %s\n", $filename, $1;
                        $cnt++;
                }
        }

        return $cnt;
}


sub check_included_files($$)
{
        my ($fileContentsRef, $filename) = @_;
        my @incFiles;

        @incFiles = (${$fileContentsRef} =~ m/\#include \s* ([<"].+[>"])/gox);

        # only our wrapper file wsutils/wsgcrypt.h may include gcrypt.h
        # all other files should include the wrapper
        if ($filename !~ /wsgcrypt\.h/) {
                foreach (@incFiles) {
                        if ( m#([<"]|/+)gcrypt\.h[>"]$# ) {
                                print STDERR "Warning: ".$filename.
                                        " includes gcrypt.h directly. ".
                                        "Include wsutil/wsgrypt.h instead.\n";
                                last;
                        }
                }
        }

        # files in the ui/qt directory should include the ui class includes
        # by using #include <>
        # this ensures that Visual Studio picks up these files from the
        # build directory if we're compiling with cmake
        if ($filename =~ m#ui/qt/# ) {
                foreach (@incFiles) {
                        if ( m#"ui_.*\.h"$# ) {
                                # strip the quotes to get the base name
                                # for the error message
                                s/\"//g;

                                print STDERR "$filename: ".
                                        "Please use #include <$_> ".
                                        "instead of #include \"$_\".\n";
                        }
                }
        }
}


sub check_proto_tree_add_XXX_encoding($$)
{
        my ($fileContentsRef, $filename) = @_;
        my @items;
        my $errorCount = 0;

        @items = (${$fileContentsRef} =~ m/ (proto_tree_add_[_a-z0-9]+) \( ([^;]*) \) \s* ; /xsg);

        while (@items) {
                my ($func) = @items;
                shift @items;
                my ($args) = @items;
                shift @items;

                # Remove anything inside parenthesis in the arguments so we
                # don't get false positives when someone calls
                # proto_tree_add_XXX(..., tvb_YYY(..., ENC_ZZZ))
                # and allow there to be newlines inside
                $args =~ s/\(.*\)//sg;

                if ($args =~ /,\s*ENC_/xos) {
                        if (!($func =~ /proto_tree_add_(time|item|bitmask|bits_item|bits_ret_val|item_ret_int|item_ret_uint|bytes_item|checksum)/xos)
                           ) {
                                print STDERR "Error: ".$filename." uses $func with ENC_*.\n";
                                $errorCount++;

                                # Print out the function args to make it easier
                                # to find the offending code.  But first make
                                # it readable by eliminating extra white space.
                                $args =~ s/\s+/ /g;
                                print STDERR "\tArgs: " . $args . "\n";
                        }
                }
        }

        return $errorCount;
}


# Verify that all declared ett_ variables are registered.
# Don't bother trying to check usage (for now)...
sub check_ett_registration($$)
{
        my ($fileContentsRef, $filename) = @_;
        my @ett_declarations;
        my %ett_registrations;
        my @unRegisteredEtts;
        my $errorCount = 0;

        # A pattern to match ett variable names.  Obviously this assumes that
        # they start with ett_
        my $EttVarName = qr{ (?: ett_[a-z0-9_]+ (?:\[[0-9]+\])? ) }xi;

        # Remove macro lines
        my $fileContents = ${$fileContentsRef};
        $fileContents =~ s { ^\s*\#.*$} []xogm;

        # Find all the ett_ variables declared in the file
        @ett_declarations = ($fileContents =~ m{
                ^\s*static              # assume declarations are on their own line
                \s+
                g?int                   # could be int or gint
                \s+
                ($EttVarName)           # variable name
                \s*=\s*
                -1\s*;
        }xgiom);

        if (!@ett_declarations) {
                print STDERR "Found no etts in ".$filename."\n";
                return;
        }

        #print "Found these etts in ".$filename.": ".join(',', @ett_declarations)."\n\n";

        # Find the array used for registering the etts
        # Save off the block of code containing just the variables
        my @reg_blocks;
        @reg_blocks = ($fileContents =~ m{
                static
                \s+
                g?int
                \s*\*\s*                # it's an array of pointers
                [a-z0-9_]+              # array name; usually (always?) "ett"
                \s*\[\s*\]\s*           # array brackets
                =
                \s*\{
                ((?:\s*&\s*             # address of the following variable
                $EttVarName             # variable name
                \s*,?                   # the comma is optional (for the last entry)
                \s*)+)                  # match one or more variable names
                \}
                \s*
                ;
        }xgios);
        #print "Found this ett registration block in ".$filename.": ".join(',', @reg_blocks)."\n";

        if (@reg_blocks == 0) {
                print STDERR "Hmm, found ".@reg_blocks." ett registration blocks in ".$filename."\n";
                # For now...
                return;
        }

        while (@reg_blocks) {
                my ($block) = @reg_blocks;
                shift @reg_blocks;

                # Convert the list returned by the match into a hash of the
                # form ett_variable_name -> 1.  Then combine this new hash with
                # the hash from the last registration block.
                # (Of course) using hashes makes the lookups much faster.
                %ett_registrations = map { $_ => 1 } ($block =~ m{
                        \s*&\s*                 # address of the following variable
                        ($EttVarName)           # variable name
                        \s*,?                   # the comma is optional (for the last entry)
                }xgios, %ett_registrations);
        }
        #print "Found these ett registrations in ".$filename.": ";
        #while( my ($k, $v) = each %ett_registrations ) {
        #          print "$k\n";
        #}

        # Find which declared etts are not registered.
        # XXX - using <@ett_declarations> and $_ instead of $ett_var makes this
        # MUCH slower...  Why?
        while (@ett_declarations) {
                my ($ett_var) = @ett_declarations;
                shift @ett_declarations;

                push(@unRegisteredEtts, $ett_var) if (!$ett_registrations{$ett_var});
        }

        if (@unRegisteredEtts) {
                print STDERR "Error: found these unregistered ett variables in ".$filename.": ".join(',', @unRegisteredEtts)."\n";
                $errorCount++;
        }

        return $errorCount;
}

# Given the file contents and a file name, check all of the hf entries for
# various problems (such as those checked for in proto.c).
sub check_hf_entries($$)
{
        my ($fileContentsRef, $filename) = @_;
        my $errorCount = 0;

        my @items;
        @items = (${$fileContentsRef} =~ m{
                                  \{
                                  \s*
                                  &\s*([A-Z0-9_\[\]-]+)         # &hf
                                  \s*,\s*
                                  \{\s*
                                  ("[A-Z0-9 '\./\(\)_:-]+")     # name
                                  \s*,\s*
                                  (NULL|"[A-Z0-9_\.-]*")        # abbrev
                                  \s*,\s*
                                  (FT_[A-Z0-9_]+)               # field type
                                  \s*,\s*
                                  ([A-Z0-9x\|_]+)               # display
                                  \s*,\s*
                                  ([^,]+?)                      # convert
                                  \s*,\s*
                                  ([A-Z0-9_]+)                  # bitmask
                                  \s*,\s*
                                  (NULL|"[A-Z0-9 '\./\(\)\?_:-]+")      # blurb (NULL or a string)
                                  \s*,\s*
                                  HFILL                         # HFILL
        }xgios);

        #print "Found @items items\n";
        while (@items) {
                ##my $errorCount_save = $errorCount;
                my ($hf, $name, $abbrev, $ft, $display, $convert, $bitmask, $blurb) = @items;
                shift @items; shift @items; shift @items; shift @items; shift @items; shift @items; shift @items; shift @items;

                #print "name=$name, abbrev=$abbrev, ft=$ft, display=$display, convert=>$convert<, bitmask=$bitmask, blurb=$blurb\n";

                if ($abbrev eq '""' || $abbrev eq "NULL") {
                        print STDERR "Error: $hf does not have an abbreviation in $filename\n";
                        $errorCount++;
                }
                if ($abbrev =~ m/\.\.+/) {
                        print STDERR "Error: the abbreviation for $hf ($abbrev) contains two or more sequential periods in $filename\n";
                        $errorCount++;
                }
                if ($name eq $abbrev) {
                        print STDERR "Error: the abbreviation for $hf ($abbrev) matches the field name ($name) in $filename\n";
                        $errorCount++;
                }
                if (lc($name) eq lc($blurb)) {
                        print STDERR "Error: the blurb for $hf ($blurb) matches the field name ($name) in $filename\n";
                        $errorCount++;
                }
                if ($name =~ m/"\s+/) {
                        print STDERR "Error: the name for $hf ($name) has leading space in $filename\n";
                        $errorCount++;
                }
                if ($name =~ m/\s+"/) {
                        print STDERR "Error: the name for $hf ($name) has trailing space in $filename\n";
                        $errorCount++;
                }
                if ($blurb =~ m/"\s+/) {
                        print STDERR "Error: the blurb for $hf ($blurb) has leading space in $filename\n";
                        $errorCount++;
                }
                if ($blurb =~ m/\s+"/) {
                        print STDERR "Error: the blurb for $hf ($blurb) has trailing space in $filename\n";
                        $errorCount++;
                }
                if ($abbrev =~ m/\s+/) {
                        print STDERR "Error: the abbreviation for $hf ($abbrev) has white space in $filename\n";
                        $errorCount++;
                }
                if ("\"".$hf ."\"" eq $name) {
                        print STDERR "Error: name is the hf_variable_name in field $name ($abbrev) in $filename\n";
                        $errorCount++;
                }
                if ("\"".$hf ."\"" eq $abbrev) {
                        print STDERR "Error: abbreviation is the hf_variable_name in field $name ($abbrev) in $filename\n";
                        $errorCount++;
                }
                if ($ft ne "FT_BOOLEAN" && $convert =~ m/^TFS\(.*\)/) {
                        print STDERR "Error: $hf uses a true/false string but is an $ft instead of FT_BOOLEAN in $filename\n";
                        $errorCount++;
                }
                if ($ft eq "FT_BOOLEAN" && $convert =~ m/^VALS\(.*\)/) {
                        print STDERR "Error: $hf uses a value_string but is an FT_BOOLEAN in $filename\n";
                        $errorCount++;
                }
                if (($ft eq "FT_BOOLEAN") && ($bitmask !~ /^(0x)?0+$/) && ($display =~ /^BASE_/)) {
                        print STDERR "Error: $hf: FT_BOOLEAN with a bitmask must specify a 'parent field width' for 'display' in $filename\n";
                        $errorCount++;
                }
                if (($ft eq "FT_BOOLEAN") && ($convert !~ m/^((0[xX]0?)?0$|NULL$|TFS)/)) {
                        print STDERR "Error: $hf: FT_BOOLEAN with non-null 'convert' field missing TFS in $filename\n";
                        $errorCount++;
                }
                if ($convert =~ m/RVALS/ && $display !~ m/BASE_RANGE_STRING/) {
                        print STDERR "Error: $hf uses RVALS but 'display' does not include BASE_RANGE_STRING in $filename\n";
                        $errorCount++;
                }
                if ($convert =~ m/^VALS\(&.*\)/) {
                        print STDERR "Error: $hf is passing the address of a pointer to VALS in $filename\n";
                        $errorCount++;
                }
                if ($convert =~ m/^RVALS\(&.*\)/) {
                        print STDERR "Error: $hf is passing the address of a pointer to RVALS in $filename\n";
                        $errorCount++;
                }
                if ($convert !~ m/^((0[xX]0?)?0$|NULL$|VALS|VALS64|VALS_EXT_PTR|RVALS|TFS|CF_FUNC|FRAMENUM_TYPE|&)/ && $display !~ /BASE_CUSTOM/) {
                        print STDERR "Error: non-null $hf 'convert' field missing 'VALS|VALS64|RVALS|TFS|CF_FUNC|FRAMENUM_TYPE|&' in $filename ?\n";
                        $errorCount++;
                }
## Benign...
##              if (($ft eq "FT_BOOLEAN") && ($bitmask =~ /^(0x)?0+$/) && ($display ne "BASE_NONE")) {
##                      print STDERR "Error: $abbrev: FT_BOOLEAN with no bitmask must use BASE_NONE for 'display' in $filename\n";
##                      $errorCount++;
##              }
                ##if ($errorCount != $errorCount_save) {
                ##        print STDERR "name=$name, abbrev=$abbrev, ft=$ft, display=$display, convert=>$convert<, bitmask=$bitmask, blurb=$blurb\n";
                ##}

        }

        return $errorCount;
}

sub print_usage
{
        print "Usage: checkAPIs.pl [-M] [-h] [-g group1] [-g group2] ... \n";
        print "                    [--build] [-s group1] [-s group2] ... \n";
        print "                    [--sourcedir=srcdir] \n";
        print "                    [--nocheck-value-string-array] \n";
        print "                    [--nocheck-addtext] [--nocheck-hf] [--debug]\n";
        print "                    [--file=/path/to/file_list]\n";
        print "                    file1 file2 ...\n";
        print "\n";
        print "       -M: Generate output for -g in 'machine-readable' format\n";
        print "       -p: used by the git pre-commit hook\n";
        print "       -h: help, print usage message\n";
        print "       -g <group>:  Check input files for use of APIs in <group>\n";
        print "                    (in addition to the default groups)\n";
        print "       -s <group>:  Output summary (count) for each API in <group>\n";
        print "                    (-g <group> also req'd)\n";
        print "       ---nocheck-value-string-array: UNDOCUMENTED\n";
        print "       ---nocheck-addtext: UNDOCUMENTED\n";
        print "       ---nocheck-hf: UNDOCUMENTED\n";
        print "       ---debug: UNDOCUMENTED\n";
        print "       ---build: UNDOCUMENTED\n";
        print "\n";
        print "   Default Groups[-g]: ", join (", ", sort @apiGroups), "\n";
        print "   Available Groups:   ", join (", ", sort keys %APIs), "\n";
}

# -------------
# action:  remove '#if 0'd code from the input string
# args     codeRef, fileName
# returns: codeRef
#
# Essentially: Use s//patsub/meg to pass each line to patsub.
#              patsub monitors #if/#if 0/etc and determines
#               if a particular code line should be removed.
# XXX: This is probably pretty inefficient;
#      I could imagine using another approach such as converting
#       the input string to an array of lines and then making
#       a pass through the array deleting lines as needed.

{  # block begin
my ($if_lvl, $if0_lvl, $if0); # shared vars
my $debug = 0;

    sub remove_if0_code {
        my ($codeRef, $fileName)  = @_;

        my ($preprocRegEx) = qr {
                                    (                                    # $1 [complete line)
                                        ^
                                        (?:                              # non-capturing
                                            \s* \# \s*
                                            (if \s 0| if | else | endif) # $2 (only if #...)
                                        ) ?
                                        .*
                                        $
                                    )
                            }xom;

        ($if_lvl, $if0_lvl, $if0) = (0,0,0);
        $$codeRef =~ s{ $preprocRegEx }{patsub($1,$2)}xegm;

        ($debug == 2) && print "==> After Remove if0: code: [$fileName]\n$$codeRef\n===<\n";
        return $codeRef;
    }

    sub patsub {
        if ($debug == 99) {
            print "-->$_[0]\n";
            (defined $_[1]) && print "  >$_[1]<\n";
        }

        # #if/#if 0/#else/#endif processing
        if (defined $_[1]) {
            my ($if) = $_[1];
            if ($if eq 'if') {
                $if_lvl += 1;
            } elsif ($if eq 'if 0') {
                $if_lvl += 1;
                if ($if0_lvl == 0) {
                    $if0_lvl = $if_lvl;
                    $if0     = 1;  # inside #if 0
                }
            } elsif ($if eq 'else') {
                if ($if0_lvl == $if_lvl) {
                    $if0 = 0;
                }
            } elsif ($if eq 'endif') {
                if ($if0_lvl == $if_lvl) {
                    $if0     = 0;
                    $if0_lvl = 0;
                }
                $if_lvl -= 1;
                if ($if_lvl < 0) {
                    die "patsub: #if/#endif mismatch"
                }
            }
            return $_[0];  # don't remove preprocessor lines themselves
        }

        # not preprocessor line: See if under #if 0: If so, remove
        if ($if0 == 1) {
            return '';  # remove
        }
        return $_[0];
    }
}  # block end

# The below Regexp are based on those from:
# http://aspn.activestate.com/ASPN/Cookbook/Rx/Recipe/59811
# They are in the public domain.

# 1. A complicated regex which matches C-style comments.
my $CComment = qr{ / [*] [^*]* [*]+ (?: [^/*] [^*]* [*]+ )* / }x;

# 1.a A regex that matches C++-style comments.
#my $CppComment = qr{ // (.*?) \n }x;

# 2. A regex which matches double-quoted strings.
#    ?s added so that strings containing a 'line continuation'
#    ( \ followed by a new-line) will match.
my $DoubleQuotedStr = qr{ (?: ["] (?s: \\. | [^\"\\])* ["]) }x;

# 3. A regex which matches single-quoted strings.
my $SingleQuotedStr = qr{ (?: \' (?: \\. | [^\'\\])* [']) }x;

# 4. Now combine 1 through 3 to produce a regex which
#    matches _either_ double or single quoted strings
#    OR comments. We surround the comment-matching
#    regex in capturing parenthesis to store the contents
#    of the comment in $1.
#    my $commentAndStringRegex = qr{(?:$DoubleQuotedStr|$SingleQuotedStr)|($CComment)|($CppComment)};

# 4. Wireshark is strictly a C program so don't take out C++ style comments
#    since they shouldn't be there anyway...
#    Also: capturing the comment isn't necessary.
## my $commentAndStringRegex = qr{ (?: $DoubleQuotedStr | $SingleQuotedStr | $CComment) }x;

#
# MAIN
#
my $errorCount = 0;

# The default list, which can be expanded.
my @apiSummaryGroups = ();
my $check_value_string_array= 1;                        # default: enabled
my $machine_readable_output = 0;                        # default: disabled
my $check_hf = 1;                                       # default: enabled
my $check_addtext = 1;                                  # default: enabled
my $debug_flag = 0;                                     # default: disabled
my $buildbot_flag = 0;
my $source_dir = "";
my $filenamelist = "";
my $help_flag = 0;
my $pre_commit = 0;

my $result = GetOptions(
                        'group=s' => \@apiGroups,
                        'summary-group=s' => \@apiSummaryGroups,
                        'check-value-string-array!' => \$check_value_string_array,
                        'Machine-readable' => \$machine_readable_output,
                        'check-hf!' => \$check_hf,
                        'check-addtext!' => \$check_addtext,
                        'build' => \$buildbot_flag,
                        'sourcedir=s' => \$source_dir,
                        'debug' => \$debug_flag,
                        'pre-commit' => \$pre_commit,
                        'file=s' => \$filenamelist,
                        'help' => \$help_flag
                        );
if (!$result || $help_flag) {
        print_usage();
        exit(1);
}

# the pre-commit hook only calls checkAPIs one file at a time, so this
# is safe to do globally (and easier)
if ($pre_commit) {
    my $filename = $ARGV[0];
    # if the filename is packet-*.c or packet-*.h, then we set the abort and termoutput groups.
    if ($filename =~ /\bpacket-[^\/\\]+\.[ch]$/) {
        push @apiGroups, "abort";
        push @apiGroups, "termoutput";
    }
}

# Add a 'function_count' anonymous hash to each of the 'apiGroup' entries in the %APIs hash.
for my $apiGroup (keys %APIs) {
        my @functions = @{$APIs{$apiGroup}{functions}};

        $APIs{$apiGroup}->{function_counts}   = {};
        @{$APIs{$apiGroup}->{function_counts}}{@functions} = ();  # Add fcn names as keys to the anonymous hash
}

my @filelist;
push @filelist, @ARGV;
if ("$filenamelist" ne "") {
        # We have a file containing a list of files to check (possibly in
        # addition to those on the command line).
        open(FC, $filenamelist) || die("Couldn't open $filenamelist");

        while (<FC>) {
                # file names can be separated by ;
                push @filelist, split(';');
        }
        close(FC);
}

# Read through the files; do various checks
while ($_ = pop @filelist)
{
        my $filename = $_;
        my $fileContents = '';
        my @foundAPIs = ();
        my $line;
        my $prohibit_cpp_comments = 1;

        if ($source_dir and ! -e $filename) {
                $filename = $source_dir . '/' . $filename;
        }
        if (! -e $filename) {
                warn "No such file: \"$filename\"";
                next;
        }

        # delete leading './'
        $filename =~ s{ ^ \. / } {}xo;
        unless (-f $filename) {
                print STDERR "Warning: $filename is not of type file - skipping.\n";
                next;
        }

        # Establish or remove local taboos
        if ($filename =~ m{ ui/qt/ }x) { $prohibit_cpp_comments = 0; }
        if ($filename =~ m{ image/*.rc }x) { $prohibit_cpp_comments = 0; }

        # Read in the file (ouch, but it's easier that way)
        open(FC, $filename) || die("Couldn't open $filename");
        $line = 1;
        while (<FC>) {
                $fileContents .= $_;
                if ($_ =~ m{ [\x80-\xFF] }xo) {
                        print STDERR "Error: Found non-ASCII characters on line " .$line. " of " .$filename."\n";
                        $errorCount++;
                }
                $line++;
        }
        close(FC);

        if (($fileContents =~ m{ \$Id .* \$ }xo))
        {
                print STDERR "Warning: ".$filename." has an SVN Id tag. Please remove it!\n";
        }

        if (($fileContents =~ m{ tab-width:\s*[0-7|9]+ | tabstop=[0-7|9]+ | tabSize=[0-7|9]+ }xo))
        {
                # To quote Icf0831717de10fc615971fa1cf75af2f1ea2d03d :
                # HT tab stops are set every 8 spaces on UN*X; UN*X tools that treat an HT character
                # as tabbing to 4-space tab stops, or that even are configurable but *default* to
                # 4-space tab stops (I'm looking at *you*, Xcode!) are broken. tab-width: 4,
                # tabstop=4, and tabSize=4 are errors if you ever expect anybody to look at your file
                # with a UN*X tool, and every text file will probably be looked at by a UN*X tool at
                # some point, so Don't Do That.
                #
                # Can I get an "amen!"?
                print STDERR "Error: Found modelines with tabstops set to something other than 8 in " .$filename."\n";
                $errorCount++;
        }

        # Remove all the C-comments
        $fileContents =~ s{ $CComment } []xog;

        # optionally check the hf entries (including those under #if 0)
        if ($check_hf) {
            $errorCount += check_hf_entries(\$fileContents, $filename);
        }

        if ($fileContents =~ m{ __func__ }xo)
        {
                print STDERR "Error: Found __func__ (which is not portable, use G_STRFUNC) in " .$filename."\n";
                $errorCount++;
        }
        if ($fileContents =~ m{ %ll }xo)
        {
                # use G_GINT64_MODIFIER instead of ll
                print STDERR "Error: Found %ll in " .$filename."\n";
                $errorCount++;
        }
        if ($fileContents =~ m{ %hh }xo)
        {
                # %hh is C99 and Windows doesn't like it:
                # http://connect.microsoft.com/VisualStudio/feedback/details/416843/sscanf-cannot-not-handle-hhd-format
                # Need to use temporary variables instead.
                print STDERR "Error: Found %hh in " .$filename."\n";
                $errorCount++;
        }

        # check for files that we should not include directly
        # this must be done before quoted strings (#include "file.h") are removed
        check_included_files(\$fileContents, $filename);

        # Check for value_string and enum_val_t errors: NULL termination,
        # const-nes, and newlines within strings
        if ($check_value_string_array) {
                $errorCount += check_value_string_arrays(\$fileContents, $filename, $debug_flag);
        }

        # Remove all the quoted strings
        $fileContents =~ s{ $DoubleQuotedStr | $SingleQuotedStr } []xog;

        #$errorCount += check_ett_registration(\$fileContents, $filename);

        if ($prohibit_cpp_comments && $fileContents =~ m{ \s// }xo)
        {
                print STDERR "Error: Found C++ style comments in " .$filename."\n";
                $errorCount++;
        }

        # Remove all blank lines
        $fileContents =~ s{ ^ \s* $ } []xog;

        # Remove all '#if 0'd' code
        remove_if0_code(\$fileContents, $filename);

        #checkAPIsCalledWithTvbGetPtr(\@TvbPtrAPIs, \$fileContents, \@foundAPIs);
        #if (@foundAPIs) {
        #       print STDERR "Found APIs with embedded tvb_get_ptr() calls in ".$filename." : ".join(',', @foundAPIs)."\n"
        #}

        checkShadowVariable(\@ShadowVariable, \$fileContents, \@foundAPIs);
        if (@foundAPIs) {
               print STDERR "Warning: Found shadow variable(s) in ".$filename." : ".join(',', @foundAPIs)."\n"
        }


        check_snprintf_plus_strlen(\$fileContents, $filename);

        $errorCount += check_proto_tree_add_XXX_encoding(\$fileContents, $filename);


        # Check and count APIs
        for my $apiGroup (@apiGroups) {
                my $pfx = "Warning";
                @foundAPIs = ();

                findAPIinFile($APIs{$apiGroup}, \$fileContents, \@foundAPIs);

                if ($APIs{$apiGroup}->{count_errors}) {
                        # the use of "prohibited" APIs is an error, increment the error count
                        $errorCount += @foundAPIs;
                        $pfx = "Error";
                }

                if (@foundAPIs && ! $machine_readable_output) {
                        print STDERR $pfx . ": Found " . $apiGroup . " APIs in ".$filename.": ".join(',', @foundAPIs)."\n";
                }
                if (@foundAPIs && $machine_readable_output) {
                        for my $api (@foundAPIs) {
                                printf STDERR "%-8.8s %-20.20s %-30.30s %-45.45s\n", $pfx, $apiGroup, $filename, $api;
                        }
                }
        }
}

# Summary: Print Use Counts of each API in each requested summary group

for my $apiGroup (@apiSummaryGroups) {
        printf "\n\nUse Counts\n";
        for my $api (sort {"\L$a" cmp "\L$b"} (keys %{$APIs{$apiGroup}->{function_counts}}   )) {
                printf "%-20.20s %5d  %-40.40s\n", $apiGroup . ':', $APIs{$apiGroup}{function_counts}{$api}, $api;
        }
}

exit($errorCount);

#
# Editor modelines  -  http://www.wireshark.org/tools/modelines.html
#
# Local variables:
# c-basic-offset: 8
# tab-width: 8
# indent-tabs-mode: nil
# End:
#
# vi: set shiftwidth=8 tabstop=8 expandtab:
# :indentSize=8:tabSize=8:noTabs=true:
#
