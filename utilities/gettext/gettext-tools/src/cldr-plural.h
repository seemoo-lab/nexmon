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

#ifndef YY_YY_CLDR_PLURAL_H_INCLUDED
# define YY_YY_CLDR_PLURAL_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    AND = 258,
    OR = 259,
    RANGE = 260,
    ELLIPSIS = 261,
    OTHER = 262,
    AT_INTEGER = 263,
    AT_DECIMAL = 264,
    KEYWORD = 265,
    INTEGER = 266,
    DECIMAL = 267,
    OPERAND = 268
  };
#endif
/* Tokens.  */
#define AND 258
#define OR 259
#define RANGE 260
#define ELLIPSIS 261
#define OTHER 262
#define AT_INTEGER 263
#define AT_DECIMAL 264
#define KEYWORD 265
#define INTEGER 266
#define DECIMAL 267
#define OPERAND 268

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 127 "cldr-plural.y" /* yacc.c:1909  */

  char *sval;
  struct cldr_plural_condition_ty *cval;
  struct cldr_plural_relation_ty *lval;
  struct cldr_plural_expression_ty *eval;
  struct cldr_plural_range_ty *gval;
  struct cldr_plural_operand_ty *oval;
  struct cldr_plural_range_list_ty *rval;
  int ival;

#line 91 "cldr-plural.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int yyparse (struct cldr_plural_parse_args *arg);

#endif /* !YY_YY_CLDR_PLURAL_H_INCLUDED  */
