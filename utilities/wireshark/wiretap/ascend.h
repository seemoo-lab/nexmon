/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_ASCEND_ASCEND_H_INCLUDED
# define YY_ASCEND_ASCEND_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int ascenddebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    STRING = 258,
    KEYWORD = 259,
    WDD_DATE = 260,
    WDD_CHUNK = 261,
    COUNTER = 262,
    SLASH_SUFFIX = 263,
    WDS_PREFIX = 264,
    ISDN_PREFIX = 265,
    ETHER_PREFIX = 266,
    DECNUM = 267,
    HEXNUM = 268,
    HEXBYTE = 269
  };
#endif
/* Tokens.  */
#define STRING 258
#define KEYWORD 259
#define WDD_DATE 260
#define WDD_CHUNK 261
#define COUNTER 262
#define SLASH_SUFFIX 263
#define WDS_PREFIX 264
#define ISDN_PREFIX 265
#define ETHER_PREFIX 266
#define DECNUM 267
#define HEXNUM 268
#define HEXBYTE 269

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 172 "./ascend.y" /* yacc.c:1909  */

gchar  *s;
guint32 d;
guint8  b;

#line 88 "ascend.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int ascendparse (void *yyscanner, ascend_state_t *parser_state, FILE_T fh);

#endif /* !YY_ASCEND_ASCEND_H_INCLUDED  */
