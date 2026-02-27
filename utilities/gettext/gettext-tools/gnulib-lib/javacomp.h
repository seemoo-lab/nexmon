/* Compile a Java program.
   Copyright (C) 2001-2002, 2006, 2009-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

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

#ifndef _JAVACOMP_H
#define _JAVACOMP_H

#include <stdbool.h>

/* Compile a Java source file to bytecode.
   java_sources is an array of source file names.
   classpaths is a list of pathnames to be prepended to the CLASSPATH.

   source_version can be:    support for
             1.3             inner classes
             1.4             assert keyword
             1.5             generic classes and methods
             1.6             (not yet supported)
   target_version can be:  classfile version:
             1.1                 45.3
             1.2                 46.0
             1.3                 47.0
             1.4                 48.0
             1.5                 49.0
             1.6                 50.0
   target_version can also be given as NULL. In this case, the required
   target_version is determined from the found JVM (see javaversion.h).
   Specifying target_version is useful when building a library (.jar) that is
   useful outside the given package. Passing target_version = NULL is useful
   when building an application.
   It is unreasonable to ask for:
     - target_version < 1.4 with source_version >= 1.4, or
     - target_version < 1.5 with source_version >= 1.5, or
     - target_version < 1.6 with source_version >= 1.6,
   because even Sun's javac doesn't support these combinations.
   It is redundant to ask for a target_version > source_version, since the
   smaller target_version = source_version will also always work and newer JVMs
   support the older target_versions too. Except for the case
   target_version = 1.4, source_version = 1.3, which allows gcj versions 3.0
   to 3.2 to be used.

   directory is the target directory. The .class file for class X.Y.Z is
   written at directory/X/Y/Z.class. If directory is NULL, the .class
   file is written in the source's directory.
   use_minimal_classpath = true means to ignore the user's CLASSPATH and
   use a minimal one. This is likely to reduce possible problems if the
   user's CLASSPATH contains garbage or a classes.zip file of the wrong
   Java version.
   If verbose, the command to be executed will be printed.
   Return false if OK, true on error.  */
extern bool compile_java_class (const char * const *java_sources,
                                unsigned int java_sources_count,
                                const char * const *classpaths,
                                unsigned int classpaths_count,
                                const char *source_version,
                                const char *target_version,
                                const char *directory,
                                bool optimize, bool debug,
                                bool use_minimal_classpath,
                                bool verbose);

#endif /* _JAVACOMP_H */
