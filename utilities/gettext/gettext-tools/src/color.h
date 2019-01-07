/* Color and styling handling.
   Copyright (C) 2006, 2015-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2006.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _COLOR_H
#define _COLOR_H

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Whether to output a test page.  */
extern DLL_VARIABLE bool color_test_mode;

/* Color option.  */
enum color_option { color_no, color_tty, color_yes, color_html };
extern DLL_VARIABLE enum color_option color_mode;

/* Style to use when coloring.  */
extern DLL_VARIABLE const char *style_file_name;

/* --color argument handling.  Return an error indicator.  */
extern bool handle_color_option (const char *option);

/* --style argument handling.  */
extern void handle_style_option (const char *option);

/* Print a color test page.  */
extern void print_color_test (void);

/* Assign a default value to style_file_name if necessary.  */
extern void style_file_prepare (void);


#ifdef __cplusplus
}
#endif


#endif /* _COLOR_H */
