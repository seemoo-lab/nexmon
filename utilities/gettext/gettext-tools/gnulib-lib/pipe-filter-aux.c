/* Auxiliary code for filtering of data through a subprocess.
   Copyright (C) 2012-2016 Free Software Foundation, Inc.

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

#include <config.h>

#define PIPE_FILTER_AUX_INLINE _GL_EXTERN_INLINE

#include "pipe-filter.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
# include <windows.h>
#else
# include <signal.h>
# include <sys/select.h>
#endif

#include "error.h"
#include "spawn-pipe.h"
#include "wait-process.h"
#include "gettext.h"

#define _(str) gettext (str)

#include "pipe-filter-aux.h"
