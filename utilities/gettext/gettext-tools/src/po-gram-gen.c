/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 20 "po-gram-gen.y" /* yacc.c:339  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Specification.  */
#include "po-gram.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "str-list.h"
#include "po-lex.h"
#include "po-charset.h"
#include "error.h"
#include "xalloc.h"
#include "gettext.h"
#include "read-catalog-abstract.h"

#define _(str) gettext (str)

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror, etc),
   as well as gratuitiously global symbol names, so we can have multiple
   yacc generated parsers in the same program.  Note that these are only
   the variables produced by yacc.  If other parser generators (bison,
   byacc, etc) produce additional global names that conflict at link time,
   then those parser generators need to be fixed instead of adding those
   names to this list. */

#define yymaxdepth po_gram_maxdepth
#define yyparse po_gram_parse
#define yylex   po_gram_lex
#define yyerror po_gram_error
#define yylval  po_gram_lval
#define yychar  po_gram_char
#define yydebug po_gram_debug
#define yypact  po_gram_pact
#define yyr1    po_gram_r1
#define yyr2    po_gram_r2
#define yydef   po_gram_def
#define yychk   po_gram_chk
#define yypgo   po_gram_pgo
#define yyact   po_gram_act
#define yyexca  po_gram_exca
#define yyerrflag po_gram_errflag
#define yynerrs po_gram_nerrs
#define yyps    po_gram_ps
#define yypv    po_gram_pv
#define yys     po_gram_s
#define yy_yys  po_gram_yys
#define yystate po_gram_state
#define yytmp   po_gram_tmp
#define yyv     po_gram_v
#define yy_yyv  po_gram_yyv
#define yyval   po_gram_val
#define yylloc  po_gram_lloc
#define yyreds  po_gram_reds          /* With YYDEBUG defined */
#define yytoks  po_gram_toks          /* With YYDEBUG defined */
#define yylhs   po_gram_yylhs
#define yylen   po_gram_yylen
#define yydefred po_gram_yydefred
#define yydgoto po_gram_yydgoto
#define yysindex po_gram_yysindex
#define yyrindex po_gram_yyrindex
#define yygindex po_gram_yygindex
#define yytable  po_gram_yytable
#define yycheck  po_gram_yycheck

static long plural_counter;

#define check_obsolete(value1,value2) \
  if ((value1).obsolete != (value2).obsolete) \
    po_gram_error_at_line (&(value2).pos, _("inconsistent use of #~"));

static inline void
do_callback_message (char *msgctxt,
                     char *msgid, lex_pos_ty *msgid_pos, char *msgid_plural,
                     char *msgstr, size_t msgstr_len, lex_pos_ty *msgstr_pos,
                     char *prev_msgctxt,
                     char *prev_msgid, char *prev_msgid_plural,
                     bool obsolete)
{
  /* Test for header entry.  Ignore fuzziness of the header entry.  */
  if (msgctxt == NULL && msgid[0] == '\0' && !obsolete)
    po_lex_charset_set (msgstr, gram_pos.file_name);

  po_callback_message (msgctxt,
                       msgid, msgid_pos, msgid_plural,
                       msgstr, msgstr_len, msgstr_pos,
                       prev_msgctxt, prev_msgid, prev_msgid_plural,
                       false, obsolete);
}

#define free_message_intro(value) \
  if ((value).prev_ctxt != NULL)        \
    free ((value).prev_ctxt);           \
  if ((value).prev_id != NULL)          \
    free ((value).prev_id);             \
  if ((value).prev_id_plural != NULL)   \
    free ((value).prev_id_plural);      \
  if ((value).ctxt != NULL)             \
    free ((value).ctxt);


#line 173 "po-gram-gen.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
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
#line 144 "po-gram-gen.y" /* yacc.c:355  */

  struct { char *string; lex_pos_ty pos; bool obsolete; } string;
  struct { string_list_ty stringlist; lex_pos_ty pos; bool obsolete; } stringlist;
  struct { long number; lex_pos_ty pos; bool obsolete; } number;
  struct { lex_pos_ty pos; bool obsolete; } pos;
  struct { char *ctxt; char *id; char *id_plural; lex_pos_ty pos; bool obsolete; } prev;
  struct { char *prev_ctxt; char *prev_id; char *prev_id_plural; char *ctxt; lex_pos_ty pos; bool obsolete; } message_intro;
  struct { struct msgstr_def rhs; lex_pos_ty pos; bool obsolete; } rhs;

#line 255 "po-gram-gen.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_PO_GRAM_GEN_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 272 "po-gram-gen.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   40

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  19
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  15
/* YYNRULES -- Number of rules.  */
#define YYNRULES  30
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  46

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   271

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    15,     2,    16,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      17,    18
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   169,   169,   171,   172,   173,   174,   179,   187,   195,
     216,   240,   249,   258,   269,   278,   292,   301,   315,   321,
     332,   338,   350,   361,   372,   376,   391,   414,   422,   434,
     442
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "COMMENT", "DOMAIN", "JUNK",
  "PREV_MSGCTXT", "PREV_MSGID", "PREV_MSGID_PLURAL", "PREV_STRING",
  "MSGCTXT", "MSGID", "MSGID_PLURAL", "MSGSTR", "NAME", "'['", "']'",
  "NUMBER", "STRING", "$accept", "po_file", "comment", "domain", "message",
  "message_intro", "prev", "msg_intro", "prev_msg_intro",
  "msgid_pluralform", "prev_msgid_pluralform", "pluralform_list",
  "pluralform", "string_list", "prev_string_list", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,    91,    93,   270,   271
};
# endif

#define YYPACT_NINF -26

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-26)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int8 yypact[] =
{
     -26,     2,   -26,   -26,   -26,    -8,     5,   -26,     0,   -26,
     -26,   -26,   -26,     0,    13,   -26,     5,   -26,   -26,    20,
     -26,    -7,     8,   -26,    24,   -26,   -26,   -26,   -26,     0,
       7,    15,    15,   -26,     5,   -26,    12,    17,    12,    21,
      15,   -26,    26,    22,     0,    12
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,     6,     7,     0,     0,    20,     0,    18,
       3,     4,     5,     0,     0,    14,     0,     8,    29,     0,
      27,     0,    13,    15,    16,    21,    30,    19,    28,     0,
       0,    11,    12,    24,     0,    17,    22,     0,     9,     0,
      10,    25,    23,     0,     0,    26
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -26,   -26,   -26,   -26,   -26,   -26,   -26,    23,   -26,   -26,
     -26,     9,   -25,   -13,   -15
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     1,    10,    11,    12,    13,    14,    15,    16,    31,
      35,    32,    33,    21,    19
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      22,    24,     2,     3,    27,     4,     5,    41,     6,     7,
      17,    28,     8,     9,    18,    41,    36,    38,    20,    42,
      29,    30,    37,     8,     9,    20,    28,    25,    39,    26,
      28,    45,    34,    26,    43,    26,    37,    23,    44,     0,
      40
};

static const yytype_int8 yycheck[] =
{
      13,    16,     0,     1,    11,     3,     4,    32,     6,     7,
      18,    18,    10,    11,     9,    40,    29,    30,    18,    34,
      12,    13,    15,    10,    11,    18,    18,     7,    13,     9,
      18,    44,     8,     9,    17,     9,    15,    14,    16,    -1,
      31
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    20,     0,     1,     3,     4,     6,     7,    10,    11,
      21,    22,    23,    24,    25,    26,    27,    18,     9,    33,
      18,    32,    32,    26,    33,     7,     9,    11,    18,    12,
      13,    28,    30,    31,     8,    29,    32,    15,    32,    13,
      30,    31,    33,    17,    16,    32
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    19,    20,    20,    20,    20,    20,    21,    22,    23,
      23,    23,    23,    23,    24,    24,    25,    25,    26,    26,
      27,    27,    28,    29,    30,    30,    31,    32,    32,    33,
      33
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     1,     2,     4,
       4,     3,     3,     2,     1,     2,     2,     3,     1,     3,
       1,     3,     2,     2,     1,     2,     5,     1,     2,     1,
       2
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 7:
#line 180 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  po_callback_comment_dispatcher ((yyvsp[0].string).string);
                }
#line 1378 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 8:
#line 188 "po-gram-gen.y" /* yacc.c:1646  */
    {
                   po_callback_domain ((yyvsp[0].string).string);
                }
#line 1386 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 9:
#line 196 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  char *string2 = string_list_concat_destroy (&(yyvsp[-2].stringlist).stringlist);
                  char *string4 = string_list_concat_destroy (&(yyvsp[0].stringlist).stringlist);

                  check_obsolete ((yyvsp[-3].message_intro), (yyvsp[-2].stringlist));
                  check_obsolete ((yyvsp[-3].message_intro), (yyvsp[-1].pos));
                  check_obsolete ((yyvsp[-3].message_intro), (yyvsp[0].stringlist));
                  if (!(yyvsp[-3].message_intro).obsolete || pass_obsolete_entries)
                    do_callback_message ((yyvsp[-3].message_intro).ctxt, string2, &(yyvsp[-3].message_intro).pos, NULL,
                                         string4, strlen (string4) + 1, &(yyvsp[-1].pos).pos,
                                         (yyvsp[-3].message_intro).prev_ctxt,
                                         (yyvsp[-3].message_intro).prev_id, (yyvsp[-3].message_intro).prev_id_plural,
                                         (yyvsp[-3].message_intro).obsolete);
                  else
                    {
                      free_message_intro ((yyvsp[-3].message_intro));
                      free (string2);
                      free (string4);
                    }
                }
#line 1411 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 10:
#line 217 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  char *string2 = string_list_concat_destroy (&(yyvsp[-2].stringlist).stringlist);

                  check_obsolete ((yyvsp[-3].message_intro), (yyvsp[-2].stringlist));
                  check_obsolete ((yyvsp[-3].message_intro), (yyvsp[-1].string));
                  check_obsolete ((yyvsp[-3].message_intro), (yyvsp[0].rhs));
                  if (!(yyvsp[-3].message_intro).obsolete || pass_obsolete_entries)
                    {
                      do_callback_message ((yyvsp[-3].message_intro).ctxt, string2, &(yyvsp[-3].message_intro).pos, (yyvsp[-1].string).string,
                                           (yyvsp[0].rhs).rhs.msgstr, (yyvsp[0].rhs).rhs.msgstr_len, &(yyvsp[0].rhs).pos,
                                           (yyvsp[-3].message_intro).prev_ctxt,
                                           (yyvsp[-3].message_intro).prev_id, (yyvsp[-3].message_intro).prev_id_plural,
                                           (yyvsp[-3].message_intro).obsolete);
                      free ((yyvsp[-1].string).string);
                    }
                  else
                    {
                      free_message_intro ((yyvsp[-3].message_intro));
                      free (string2);
                      free ((yyvsp[-1].string).string);
                      free ((yyvsp[0].rhs).rhs.msgstr);
                    }
                }
#line 1439 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 11:
#line 241 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  check_obsolete ((yyvsp[-2].message_intro), (yyvsp[-1].stringlist));
                  check_obsolete ((yyvsp[-2].message_intro), (yyvsp[0].string));
                  po_gram_error_at_line (&(yyvsp[-2].message_intro).pos, _("missing 'msgstr[]' section"));
                  free_message_intro ((yyvsp[-2].message_intro));
                  string_list_destroy (&(yyvsp[-1].stringlist).stringlist);
                  free ((yyvsp[0].string).string);
                }
#line 1452 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 12:
#line 250 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  check_obsolete ((yyvsp[-2].message_intro), (yyvsp[-1].stringlist));
                  check_obsolete ((yyvsp[-2].message_intro), (yyvsp[0].rhs));
                  po_gram_error_at_line (&(yyvsp[-2].message_intro).pos, _("missing 'msgid_plural' section"));
                  free_message_intro ((yyvsp[-2].message_intro));
                  string_list_destroy (&(yyvsp[-1].stringlist).stringlist);
                  free ((yyvsp[0].rhs).rhs.msgstr);
                }
#line 1465 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 13:
#line 259 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  check_obsolete ((yyvsp[-1].message_intro), (yyvsp[0].stringlist));
                  po_gram_error_at_line (&(yyvsp[-1].message_intro).pos, _("missing 'msgstr' section"));
                  free_message_intro ((yyvsp[-1].message_intro));
                  string_list_destroy (&(yyvsp[0].stringlist).stringlist);
                }
#line 1476 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 14:
#line 270 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  (yyval.message_intro).prev_ctxt = NULL;
                  (yyval.message_intro).prev_id = NULL;
                  (yyval.message_intro).prev_id_plural = NULL;
                  (yyval.message_intro).ctxt = (yyvsp[0].string).string;
                  (yyval.message_intro).pos = (yyvsp[0].string).pos;
                  (yyval.message_intro).obsolete = (yyvsp[0].string).obsolete;
                }
#line 1489 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 15:
#line 279 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  check_obsolete ((yyvsp[-1].prev), (yyvsp[0].string));
                  (yyval.message_intro).prev_ctxt = (yyvsp[-1].prev).ctxt;
                  (yyval.message_intro).prev_id = (yyvsp[-1].prev).id;
                  (yyval.message_intro).prev_id_plural = (yyvsp[-1].prev).id_plural;
                  (yyval.message_intro).ctxt = (yyvsp[0].string).string;
                  (yyval.message_intro).pos = (yyvsp[0].string).pos;
                  (yyval.message_intro).obsolete = (yyvsp[0].string).obsolete;
                }
#line 1503 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 16:
#line 293 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  check_obsolete ((yyvsp[-1].string), (yyvsp[0].stringlist));
                  (yyval.prev).ctxt = (yyvsp[-1].string).string;
                  (yyval.prev).id = string_list_concat_destroy (&(yyvsp[0].stringlist).stringlist);
                  (yyval.prev).id_plural = NULL;
                  (yyval.prev).pos = (yyvsp[-1].string).pos;
                  (yyval.prev).obsolete = (yyvsp[-1].string).obsolete;
                }
#line 1516 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 17:
#line 302 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  check_obsolete ((yyvsp[-2].string), (yyvsp[-1].stringlist));
                  check_obsolete ((yyvsp[-2].string), (yyvsp[0].string));
                  (yyval.prev).ctxt = (yyvsp[-2].string).string;
                  (yyval.prev).id = string_list_concat_destroy (&(yyvsp[-1].stringlist).stringlist);
                  (yyval.prev).id_plural = (yyvsp[0].string).string;
                  (yyval.prev).pos = (yyvsp[-2].string).pos;
                  (yyval.prev).obsolete = (yyvsp[-2].string).obsolete;
                }
#line 1530 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 18:
#line 316 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  (yyval.string).string = NULL;
                  (yyval.string).pos = (yyvsp[0].pos).pos;
                  (yyval.string).obsolete = (yyvsp[0].pos).obsolete;
                }
#line 1540 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 19:
#line 322 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  check_obsolete ((yyvsp[-2].pos), (yyvsp[-1].stringlist));
                  check_obsolete ((yyvsp[-2].pos), (yyvsp[0].pos));
                  (yyval.string).string = string_list_concat_destroy (&(yyvsp[-1].stringlist).stringlist);
                  (yyval.string).pos = (yyvsp[0].pos).pos;
                  (yyval.string).obsolete = (yyvsp[0].pos).obsolete;
                }
#line 1552 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 20:
#line 333 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  (yyval.string).string = NULL;
                  (yyval.string).pos = (yyvsp[0].pos).pos;
                  (yyval.string).obsolete = (yyvsp[0].pos).obsolete;
                }
#line 1562 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 21:
#line 339 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  check_obsolete ((yyvsp[-2].pos), (yyvsp[-1].stringlist));
                  check_obsolete ((yyvsp[-2].pos), (yyvsp[0].pos));
                  (yyval.string).string = string_list_concat_destroy (&(yyvsp[-1].stringlist).stringlist);
                  (yyval.string).pos = (yyvsp[0].pos).pos;
                  (yyval.string).obsolete = (yyvsp[0].pos).obsolete;
                }
#line 1574 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 22:
#line 351 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  check_obsolete ((yyvsp[-1].pos), (yyvsp[0].stringlist));
                  plural_counter = 0;
                  (yyval.string).string = string_list_concat_destroy (&(yyvsp[0].stringlist).stringlist);
                  (yyval.string).pos = (yyvsp[-1].pos).pos;
                  (yyval.string).obsolete = (yyvsp[-1].pos).obsolete;
                }
#line 1586 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 23:
#line 362 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  check_obsolete ((yyvsp[-1].pos), (yyvsp[0].stringlist));
                  (yyval.string).string = string_list_concat_destroy (&(yyvsp[0].stringlist).stringlist);
                  (yyval.string).pos = (yyvsp[-1].pos).pos;
                  (yyval.string).obsolete = (yyvsp[-1].pos).obsolete;
                }
#line 1597 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 24:
#line 373 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  (yyval.rhs) = (yyvsp[0].rhs);
                }
#line 1605 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 25:
#line 377 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  check_obsolete ((yyvsp[-1].rhs), (yyvsp[0].rhs));
                  (yyval.rhs).rhs.msgstr = XNMALLOC ((yyvsp[-1].rhs).rhs.msgstr_len + (yyvsp[0].rhs).rhs.msgstr_len, char);
                  memcpy ((yyval.rhs).rhs.msgstr, (yyvsp[-1].rhs).rhs.msgstr, (yyvsp[-1].rhs).rhs.msgstr_len);
                  memcpy ((yyval.rhs).rhs.msgstr + (yyvsp[-1].rhs).rhs.msgstr_len, (yyvsp[0].rhs).rhs.msgstr, (yyvsp[0].rhs).rhs.msgstr_len);
                  (yyval.rhs).rhs.msgstr_len = (yyvsp[-1].rhs).rhs.msgstr_len + (yyvsp[0].rhs).rhs.msgstr_len;
                  free ((yyvsp[-1].rhs).rhs.msgstr);
                  free ((yyvsp[0].rhs).rhs.msgstr);
                  (yyval.rhs).pos = (yyvsp[-1].rhs).pos;
                  (yyval.rhs).obsolete = (yyvsp[-1].rhs).obsolete;
                }
#line 1621 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 26:
#line 392 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  check_obsolete ((yyvsp[-4].pos), (yyvsp[-3].pos));
                  check_obsolete ((yyvsp[-4].pos), (yyvsp[-2].number));
                  check_obsolete ((yyvsp[-4].pos), (yyvsp[-1].pos));
                  check_obsolete ((yyvsp[-4].pos), (yyvsp[0].stringlist));
                  if ((yyvsp[-2].number).number != plural_counter)
                    {
                      if (plural_counter == 0)
                        po_gram_error_at_line (&(yyvsp[-4].pos).pos, _("first plural form has nonzero index"));
                      else
                        po_gram_error_at_line (&(yyvsp[-4].pos).pos, _("plural form has wrong index"));
                    }
                  plural_counter++;
                  (yyval.rhs).rhs.msgstr = string_list_concat_destroy (&(yyvsp[0].stringlist).stringlist);
                  (yyval.rhs).rhs.msgstr_len = strlen ((yyval.rhs).rhs.msgstr) + 1;
                  (yyval.rhs).pos = (yyvsp[-4].pos).pos;
                  (yyval.rhs).obsolete = (yyvsp[-4].pos).obsolete;
                }
#line 1644 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 27:
#line 415 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  string_list_init (&(yyval.stringlist).stringlist);
                  string_list_append (&(yyval.stringlist).stringlist, (yyvsp[0].string).string);
                  free ((yyvsp[0].string).string);
                  (yyval.stringlist).pos = (yyvsp[0].string).pos;
                  (yyval.stringlist).obsolete = (yyvsp[0].string).obsolete;
                }
#line 1656 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 28:
#line 423 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  check_obsolete ((yyvsp[-1].stringlist), (yyvsp[0].string));
                  (yyval.stringlist).stringlist = (yyvsp[-1].stringlist).stringlist;
                  string_list_append (&(yyval.stringlist).stringlist, (yyvsp[0].string).string);
                  free ((yyvsp[0].string).string);
                  (yyval.stringlist).pos = (yyvsp[-1].stringlist).pos;
                  (yyval.stringlist).obsolete = (yyvsp[-1].stringlist).obsolete;
                }
#line 1669 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 29:
#line 435 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  string_list_init (&(yyval.stringlist).stringlist);
                  string_list_append (&(yyval.stringlist).stringlist, (yyvsp[0].string).string);
                  free ((yyvsp[0].string).string);
                  (yyval.stringlist).pos = (yyvsp[0].string).pos;
                  (yyval.stringlist).obsolete = (yyvsp[0].string).obsolete;
                }
#line 1681 "po-gram-gen.c" /* yacc.c:1646  */
    break;

  case 30:
#line 443 "po-gram-gen.y" /* yacc.c:1646  */
    {
                  check_obsolete ((yyvsp[-1].stringlist), (yyvsp[0].string));
                  (yyval.stringlist).stringlist = (yyvsp[-1].stringlist).stringlist;
                  string_list_append (&(yyval.stringlist).stringlist, (yyvsp[0].string).string);
                  free ((yyvsp[0].string).string);
                  (yyval.stringlist).pos = (yyvsp[-1].stringlist).pos;
                  (yyval.stringlist).obsolete = (yyvsp[-1].stringlist).obsolete;
                }
#line 1694 "po-gram-gen.c" /* yacc.c:1646  */
    break;


#line 1698 "po-gram-gen.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
