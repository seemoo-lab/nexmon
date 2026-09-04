Title: Keyed Data Lists and Datasets
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 1999 Owen Taylor
SPDX-FileCopyrightText: 2000 Red Hat, Inc.
SPDX-FileCopyrightText: 2005 Tim Janik

## Keyed Data Lists

Keyed data lists provide lists of arbitrary data elements which can
be accessed either with a string or with a [type@GLib.Quark] corresponding to
the string.

The [type@GLib.Quark] methods are quicker, since the strings have to be
converted to [type@GLib.Quark]s anyway.

Data lists are used for associating arbitrary data with
[`GObject`](../gobject/class.Object.html)s, using [`g_object_set_data()`](../gobject/method.Object.set_data.html) and related
functions. The data is stored inside opaque [type@GLib.Data] elements.

To create a datalist, use [func@GLib.datalist_init].

To add data elements to a datalist use [func@GLib.datalist_id_set_data],
[func@GLib.datalist_id_set_data_full], [func@GLib.datalist_set_data],
[func@GLib.datalist_set_data_full] and [func@GLib.datalist_id_replace_data].

To get data elements from a datalist use [func@GLib.datalist_id_get_data],
[func@GLib.datalist_get_data] and [func@GLib.datalist_id_dup_data].

To iterate over all data elements in a datalist use
[func@GLib.datalist_foreach] (not thread-safe).

To remove data elements from a datalist use
[func@GLib.datalist_id_remove_data], [func@GLib.datalist_remove_data] and
[func@GLib.datalist_id_remove_multiple]. To remove elements without destroying
them, use [func@GLib.datalist_id_remove_no_notify] and
[func@GLib.datalist_remove_no_notify].

To remove all data elements from a datalist, use [func@GLib.datalist_clear].

A small number of boolean flags can be stored alongside a datalist, using
[func@GLib.datalist_set_flags], [func@GLib.datalist_unset_flags] and
[func@GLib.datalist_get_flags].

## Datasets

Datasets associate groups of data elements with particular memory
locations. These are useful if you need to associate data with a
structure returned from an external library. Since you cannot modify
the structure, you use its location in memory as the key into a
dataset, where you can associate any number of data elements with it.

There are two forms of most of the dataset functions. The first form
uses strings to identify the data elements associated with a
location. The second form uses [type@GLib.Quark] identifiers, which are
created with a call to [func@GLib.quark_from_string] or
[func@GLib.quark_from_static_string]. The second form is quicker, since it
does not require looking up the string in the hash table of [type@GLib.Quark]
identifiers.

There is no function to create a dataset. It is automatically
created as soon as you add elements to it.

To add data elements to a dataset use [func@GLib.dataset_id_set_data],
[func@GLib.dataset_id_set_data_full], [func@GLib.dataset_set_data] and
[func@GLib.dataset_set_data_full].

To get data elements from a dataset use [func@GLib.dataset_id_get_data] and
[func@GLib.dataset_get_data].

To iterate over all data elements in a dataset use
[func@GLib.dataset_foreach] (not thread-safe).

To remove data elements from a dataset use
[func@GLib.dataset_id_remove_data] and [func@GLib.dataset_remove_data]. To
remove data without destroying it, use [func@GLib.dataset_id_remove_no_notify]
and [func@GLib.dataset_remove_no_notify].

To destroy a dataset, use [func@GLib.dataset_destroy].
