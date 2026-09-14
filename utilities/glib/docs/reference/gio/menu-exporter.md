Title: GMenuModel Exporter
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2011 Matthias Clasen

# GMenuModel Exporter

These functions support exporting a [class@Gio.MenuModel] on D-Bus. The D-Bus
interface that is used is a private implementation detail.

 * [method@Gio.DBusConnection.export_menu_model]
 * [method@Gio.DBusConnection.unexport_menu_model]

To access an exported [class@Gio.MenuModel] remotely, use
[func@Gio.DBusMenuModel.get] to obtain a [class@Gio.DBusMenuModel].

