/* inotify-sub.h - GVFS Directory Monitor using inotify

   Copyright (C) 2006 John McCutchan

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   see <http://www.gnu.org/licenses/>.

   Author: John McCutchan <john@johnmccutchan.com>
*/


#ifndef __INOTIFY_SUB_H
#define __INOTIFY_SUB_H

typedef struct
{
	gchar*   dirname;
	gchar*   filename;
	gboolean cancelled;
	gpointer user_data;
	gboolean pair_moves;
	gboolean hardlinks;
} inotify_sub;

inotify_sub *_ih_sub_new (const gchar  *dirname,
			  const gchar  *filename,
			  gboolean      watch_hardlinks,
			  gpointer      user_data);
void         _ih_sub_free (inotify_sub *sub);

#endif /* __INOTIFY_SUB_H */
