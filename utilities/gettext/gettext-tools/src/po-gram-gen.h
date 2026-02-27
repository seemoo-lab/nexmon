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

#ifndef YY_YY_PO_GRAM_GEN_H_INCLUDED
# define YY_YY_PO_GRAM_GEN_H_INCLUDED
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
    COMMENT = 258,
    DOMAIN = 259,
    JUNK = 260,
    PREV_MSGCTXT = 261,
    PREV_MSGID = 262,
    PREV_MSGID_PLURAL = 263,
    PREV_STRING = 264,
    MSGCTXT = 265,
    MSGID = 266,
    MSGID_PLURAL = 267,
    MSGSTR = 268,
    NAME = 269,
    NUMBER = 270,
    STRING = 271
  };
#endif
/* Tokens.  */
#define COMMENT 258
#define DOMAIN 259
#define JUNK 260
#define PREV_MSGCTXT 261
#define PREV_MSGID 262
#define PREV_MSGID_PLURAL 263
#define PREV_STRING 264
#define MSGCTXT 265
#define MSGID 266
#define MSGID_PLURAL 267
#define MSGSTR 268
#define NAME 269
#define NUMBER 270
#define STRING 271

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 144 "po-gram-gen.y" /* yacc.c:1909  */

  struct { char *string; lex_pos_ty pos; bool obsolete; } string;
  struct { string_list_ty stringlist; lex_pos_ty pos; bool obsolete; } stringlist;
  struct { long number; lex_pos_ty pos; bool obsolete; } number;
  struct { lex_pos_ty pos; bool obsolete; } pos;
  struct { char *ctxt; char *id; char *id_plural; lex_pos_ty pos; bool obsolete; } prev;
  struct { char *prev_ctxt; char *prev_id; char *prev_id_plural; char *ctxt; lex_pos_ty pos; bool obsolete; } message_intro;
  struct { struct msgstr_def rhs; lex_pos_ty pos; bool obsolete; } rhs;

#line 96 "po-gram-gen.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_PO_GRAM_GEN_H_INCLUDED  */
