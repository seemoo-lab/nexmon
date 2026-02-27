/*
** 2000-05-29
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Driver template for the LEMON parser generator.
**
** The "lemon" program processes an LALR(1) input grammar file, then uses
** this template to construct a parser.  The "lemon" program inserts text
** at each "%%" line.  Also, any "P-a-r-s-e" identifer prefix (without the
** interstitial "-" characters) contained in this template is changed into
** the value of the %name directive from the grammar.  Otherwise, the content
** of this template is copied straight through into the generate parser
** source file.
**
** The following is the concatenation of all %include directives from the
** input grammar file:
*/
#include <stdio.h>
/************ Begin %include sections from the grammar ************************/
#line 1 "./dtd_grammar.lemon"


/* dtd_parser.lemon
* XML dissector for wireshark 
* XML's DTD grammar
*
* Copyright 2005, Luis E. Garcia Ontanon <luis@ontanon.org>
*
* Wireshark - Network traffic analyzer
* By Gerald Combs <gerald@wireshark.org>
* Copyright 1998 Gerald Combs
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "config.h"

#include <stdio.h>
#include <glib.h>
#include <assert.h>
#include "dtd.h"
#include "dtd_parse.h"

static dtd_named_list_t* dtd_named_list_new(gchar* name, GPtrArray* list) {
	dtd_named_list_t* nl = g_new(dtd_named_list_t,1);

	nl->name = name;
	nl->list = list;

	return nl;
}

static GPtrArray* g_ptr_array_join(GPtrArray* a, GPtrArray* b){

	while(b->len > 0) {
		g_ptr_array_add(a,g_ptr_array_remove_index_fast(b,0));
	}

	g_ptr_array_free(b,TRUE);

	return a;
}

#line 84 "dtd_grammar.c"
/**************** End of %include directives **********************************/
/* These constants specify the various numeric values for terminal symbols
** in a format understandable to "makeheaders".  This section is blank unless
** "lemon" is run with the "-m" command-line option.
***************** Begin makeheaders token definitions *************************/
/**************** End makeheaders token definitions ***************************/

/* The next sections is a series of control #defines.
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used to store the integer codes
**                       that represent terminal and non-terminal symbols.
**                       "unsigned char" is used if there are fewer than
**                       256 symbols.  Larger types otherwise.
**    YYNOCODE           is a number of type YYCODETYPE that is not used for
**                       any terminal or nonterminal symbol.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       (also known as: "terminal symbols") have fall-back
**                       values which should be used if the original symbol
**                       would not parse.  This permits keywords to sometimes
**                       be used as identifiers, for example.
**    YYACTIONTYPE       is the data type used for "action codes" - numbers
**                       that indicate what to do in response to the next
**                       token.
**    DtdParseTOKENTYPE     is the data type used for minor type for terminal
**                       symbols.  Background: A "minor type" is a semantic
**                       value associated with a terminal or non-terminal
**                       symbols.  For example, for an "ID" terminal symbol,
**                       the minor type might be the name of the identifier.
**                       Each non-terminal can have a different minor type.
**                       Terminal symbols all have the same minor type, though.
**                       This macros defines the minor type for terminal
**                       symbols.
**    YYMINORTYPE        is the data type used for all minor types.
**                       This is typically a union of many types, one of
**                       which is DtdParseTOKENTYPE.  The entry in the union
**                       for terminal symbols is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    DtdParseARG_SDECL     A static variable declaration for the %extra_argument
**    DtdParseARG_PDECL     A parameter declaration for the %extra_argument
**    DtdParseARG_STORE     Code to store %extra_argument into yypParser
**    DtdParseARG_FETCH     Code to extract %extra_argument from yypParser
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YY_MAX_SHIFT       Maximum value for shift actions
**    YY_MIN_SHIFTREDUCE Minimum value for shift-reduce actions
**    YY_MAX_SHIFTREDUCE Maximum value for shift-reduce actions
**    YY_MIN_REDUCE      Maximum value for reduce actions
**    YY_ERROR_ACTION    The yy_action[] code for syntax error
**    YY_ACCEPT_ACTION   The yy_action[] code for accept
**    YY_NO_ACTION       The yy_action[] code for no-op
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/************* Begin control #defines *****************************************/
#define YYCODETYPE unsigned char
#define YYNOCODE 41
#define YYACTIONTYPE unsigned char
#define DtdParseTOKENTYPE  dtd_token_data_t* 
typedef union {
  int yyinit;
  DtdParseTOKENTYPE yy0;
  dtd_named_list_t* yy29;
  gchar* yy44;
  GPtrArray* yy59;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define DtdParseARG_SDECL  dtd_build_data_t *bd ;
#define DtdParseARG_PDECL , dtd_build_data_t *bd 
#define DtdParseARG_FETCH  dtd_build_data_t *bd  = yypParser->bd 
#define DtdParseARG_STORE yypParser->bd  = bd 
#define YYNSTATE             33
#define YYNRULE              44
#define YY_MAX_SHIFT         32
#define YY_MIN_SHIFTREDUCE   71
#define YY_MAX_SHIFTREDUCE   114
#define YY_MIN_REDUCE        115
#define YY_MAX_REDUCE        158
#define YY_ERROR_ACTION      159
#define YY_ACCEPT_ACTION     160
#define YY_NO_ACTION         161
/************* End control #defines *******************************************/

/* The yyzerominor constant is used to initialize instances of
** YYMINORTYPE objects to zero. */
static const YYMINORTYPE yyzerominor = { 0 };

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N <= YY_MAX_SHIFT             Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   N between YY_MIN_SHIFTREDUCE       Shift to an arbitrary state then
**     and YY_MAX_SHIFTREDUCE           reduce by rule N-YY_MIN_SHIFTREDUCE.
**
**   N between YY_MIN_REDUCE            Reduce by rule N-YY_MIN_REDUCE
**     and YY_MAX_REDUCE

**   N == YY_ERROR_ACTION               A syntax error has occurred.
**
**   N == YY_ACCEPT_ACTION              The parser accepts its input.
**
**   N == YY_NO_ACTION                  No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
**
*********** Begin parsing tables **********************************************/
#define YY_ACTTAB_COUNT (92)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   160,   32,    9,   76,   77,   85,   30,   87,   88,   89,
 /*    10 */    17,   17,   20,   16,   17,   95,    7,   76,   77,   19,
 /*    20 */   107,  106,    1,    1,   96,   15,    1,    2,   28,   26,
 /*    30 */   104,  104,   27,   79,  104,   93,   21,   92,   25,   97,
 /*    40 */    98,   99,   22,   31,   29,   97,   98,   99,  112,  114,
 /*    50 */   113,  103,    4,   18,   94,   22,   91,    5,   83,   23,
 /*    60 */     8,  105,   74,   75,    2,   11,    1,   84,  116,   22,
 /*    70 */    13,   82,   81,    8,  104,   14,   78,   80,   90,    6,
 /*    80 */    31,   29,   73,    3,   24,  102,   12,  101,  100,   86,
 /*    90 */    10,  115,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    25,   26,   27,   28,   29,   10,   11,   12,   13,   14,
 /*    10 */     3,    3,   31,   31,    3,    3,   27,   28,   29,   38,
 /*    20 */    39,   39,   15,   15,   12,    1,   15,   15,   21,   21,
 /*    30 */    23,   23,   21,    6,   23,   35,   36,   37,    2,   18,
 /*    40 */    19,   20,    1,    7,    8,   18,   19,   20,   18,   19,
 /*    50 */    20,   16,   17,   31,   35,    1,   37,   22,    9,    5,
 /*    60 */     3,   39,   28,   29,   15,   33,   15,   35,    0,    1,
 /*    70 */    30,   34,   32,    3,   23,   31,    6,   32,   16,   17,
 /*    80 */     7,    8,    6,    4,    3,   16,    3,   16,   16,   12,
 /*    90 */     3,    0,
};
#define YY_SHIFT_USE_DFLT (-6)
#define YY_SHIFT_COUNT (32)
#define YY_SHIFT_MIN   (-5)
#define YY_SHIFT_MAX   (91)
static const signed char yy_shift_ofst[] = {
 /*     0 */    24,    7,   12,   41,    8,   11,   12,   54,   49,   68,
 /*    10 */    57,   -5,   51,   70,   27,   36,   21,   30,   21,   35,
 /*    20 */    21,   62,   73,   76,   79,   81,   69,   71,   72,   83,
 /*    30 */    77,   87,   91,
};
#define YY_REDUCE_USE_DFLT (-26)
#define YY_REDUCE_COUNT (13)
#define YY_REDUCE_MIN   (-25)
#define YY_REDUCE_MAX   (45)
static const signed char yy_reduce_ofst[] = {
 /*     0 */   -25,  -19,    0,  -11,  -18,   22,   19,   34,   32,   34,
 /*    10 */    40,   37,   44,   45,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   159,  159,  159,  159,  159,  159,  159,  159,  159,  159,
 /*    10 */   159,  159,  159,  159,  159,  159,  154,  155,  153,  159,
 /*    20 */   152,  159,  159,  159,  159,  159,  159,  159,  159,  159,
 /*    30 */   159,  159,  159,
};
/********** End of lemon-generated parsing tables *****************************/

/* The next table maps tokens (terminal symbols) into fallback tokens.
** If a construct like the following:
**
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
**
** This feature can be used, for example, to cause some keywords in a language
** to revert to identifiers if they keyword does not apply in the context where
** it appears.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
**
** After the "shift" half of a SHIFTREDUCE action, the stateno field
** actually contains the reduce action for the second half of the
** SHIFTREDUCE.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number, or reduce action in SHIFTREDUCE */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyidxMax;                 /* Maximum value of yyidx */
#endif
  int yyerrcnt;                 /* Shifts left before out of the error */
  DtdParseARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/*
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void DtdParseTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = {
  "$",             "TAG_START",     "DOCTYPE_KW",    "NAME",        
  "OPEN_BRACKET",  "CLOSE_BRACKET",  "TAG_STOP",      "ATTLIST_KW",  
  "ELEMENT_KW",    "ATT_TYPE",      "ATT_DEF",       "ATT_DEF_WITH_VALUE",
  "QUOTED",        "IMPLIED_KW",    "REQUIRED_KW",   "OPEN_PARENS", 
  "CLOSE_PARENS",  "PIPE",          "STAR",          "PLUS",        
  "QUESTION",      "ELEM_DATA",     "COMMA",         "EMPTY_KW",    
  "error",         "dtd",           "doctype",       "dtd_parts",   
  "element",       "attlist",       "attrib_list",   "sub_elements",
  "attrib",        "att_type",      "att_default",   "enumeration", 
  "enum_list",     "enum_item",     "element_list",  "element_child",
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "dtd ::= doctype",
 /*   1 */ "dtd ::= dtd_parts",
 /*   2 */ "doctype ::= TAG_START DOCTYPE_KW NAME OPEN_BRACKET dtd_parts CLOSE_BRACKET TAG_STOP",
 /*   3 */ "dtd_parts ::= dtd_parts element",
 /*   4 */ "dtd_parts ::= dtd_parts attlist",
 /*   5 */ "dtd_parts ::= element",
 /*   6 */ "dtd_parts ::= attlist",
 /*   7 */ "attlist ::= TAG_START ATTLIST_KW NAME attrib_list TAG_STOP",
 /*   8 */ "element ::= TAG_START ELEMENT_KW NAME sub_elements TAG_STOP",
 /*   9 */ "attrib_list ::= attrib_list attrib",
 /*  10 */ "attrib_list ::= attrib",
 /*  11 */ "attrib ::= NAME att_type att_default",
 /*  12 */ "att_type ::= ATT_TYPE",
 /*  13 */ "att_type ::= enumeration",
 /*  14 */ "att_default ::= ATT_DEF",
 /*  15 */ "att_default ::= ATT_DEF_WITH_VALUE QUOTED",
 /*  16 */ "att_default ::= QUOTED",
 /*  17 */ "att_default ::= IMPLIED_KW",
 /*  18 */ "att_default ::= REQUIRED_KW",
 /*  19 */ "enumeration ::= OPEN_PARENS enum_list CLOSE_PARENS",
 /*  20 */ "enum_list ::= enum_list PIPE enum_item",
 /*  21 */ "enum_list ::= enum_item",
 /*  22 */ "enum_list ::= enumeration",
 /*  23 */ "enum_list ::= enum_list PIPE enumeration",
 /*  24 */ "enum_item ::= NAME",
 /*  25 */ "enum_item ::= QUOTED",
 /*  26 */ "sub_elements ::= sub_elements STAR",
 /*  27 */ "sub_elements ::= sub_elements PLUS",
 /*  28 */ "sub_elements ::= sub_elements QUESTION",
 /*  29 */ "sub_elements ::= OPEN_PARENS ELEM_DATA CLOSE_PARENS",
 /*  30 */ "sub_elements ::= OPEN_PARENS element_list COMMA ELEM_DATA CLOSE_PARENS",
 /*  31 */ "sub_elements ::= OPEN_PARENS element_list PIPE ELEM_DATA CLOSE_PARENS",
 /*  32 */ "sub_elements ::= OPEN_PARENS element_list CLOSE_PARENS",
 /*  33 */ "sub_elements ::= EMPTY_KW",
 /*  34 */ "element_list ::= element_list COMMA element_child",
 /*  35 */ "element_list ::= element_list PIPE element_child",
 /*  36 */ "element_list ::= element_child",
 /*  37 */ "element_list ::= sub_elements",
 /*  38 */ "element_list ::= element_list COMMA sub_elements",
 /*  39 */ "element_list ::= element_list PIPE sub_elements",
 /*  40 */ "element_child ::= NAME",
 /*  41 */ "element_child ::= NAME STAR",
 /*  42 */ "element_child ::= NAME QUESTION",
 /*  43 */ "element_child ::= NAME PLUS",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(yyParser *p){
  int newSize;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  if( pNew ){
    p->yystack = pNew;
    p->yystksz = newSize;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, p->yystksz);
    }
#endif
  }
}
#endif

/* Datatype of the argument to the memory allocated passed as the
** second argument to DtdParseAlloc() below.  This can be changed by
** putting an appropriate #define in the %include section of the input
** grammar.
*/
#ifndef YYMALLOCARGTYPE
# define YYMALLOCARGTYPE size_t
#endif

/*
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to DtdParse and DtdParseFree.
*/
void *DtdParseAlloc(void *(*mallocProc)(YYMALLOCARGTYPE)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (YYMALLOCARGTYPE)sizeof(yyParser) );
  if( pParser ){
    pParser->yyidx = -1;
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyidxMax = 0;
#endif
#if YYSTACKDEPTH<=0
    pParser->yystack = NULL;
    pParser->yystksz = 0;
    yyGrowStack(pParser);
#endif
  }
  return pParser;
}

/* The following function deletes the "minor type" or semantic value
** associated with a symbol.  The symbol can be either a terminal
** or nonterminal. "yymajor" is the symbol code, and "yypminor" is
** a pointer to the value to be deleted.  The code used to do the
** deletions is derived from the %destructor and/or %token_destructor
** directives of the input grammar.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  DtdParseARG_FETCH;
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are *not* used
    ** inside the C code.
    */
/********* Begin destructor definitions ***************************************/
      /* TERMINAL Destructor */
    case 1: /* TAG_START */
    case 2: /* DOCTYPE_KW */
    case 3: /* NAME */
    case 4: /* OPEN_BRACKET */
    case 5: /* CLOSE_BRACKET */
    case 6: /* TAG_STOP */
    case 7: /* ATTLIST_KW */
    case 8: /* ELEMENT_KW */
    case 9: /* ATT_TYPE */
    case 10: /* ATT_DEF */
    case 11: /* ATT_DEF_WITH_VALUE */
    case 12: /* QUOTED */
    case 13: /* IMPLIED_KW */
    case 14: /* REQUIRED_KW */
    case 15: /* OPEN_PARENS */
    case 16: /* CLOSE_PARENS */
    case 17: /* PIPE */
    case 18: /* STAR */
    case 19: /* PLUS */
    case 20: /* QUESTION */
    case 21: /* ELEM_DATA */
    case 22: /* COMMA */
    case 23: /* EMPTY_KW */
{
#line 62 "./dtd_grammar.lemon"

	(void) bd; /* Mark unused, similar to Q_UNUSED */
	if ((yypminor->yy0)) {
		if ((yypminor->yy0)->text) g_free((yypminor->yy0)->text);
		if ((yypminor->yy0)->location) g_free((yypminor->yy0)->location);
		g_free((yypminor->yy0));
	}

#line 576 "dtd_grammar.c"
}
      break;
/********* End destructor definitions *****************************************/
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
*/
static void yy_pop_parser_stack(yyParser *pParser){
  yyStackEntry *yytos;
  assert( pParser->yyidx>=0 );
  yytos = &pParser->yystack[pParser->yyidx--];
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yy_destructor(pParser, yytos->major, &yytos->minor);
}

/*
** Deallocate and destroy a parser.  Destructors are called for
** all stack elements before shutting the parser down.
**
** If the YYPARSEFREENEVERNULL macro exists (for example because it
** is defined in a %include section of the input grammar) then it is
** assumed that the input pointer is never NULL.
*/
void DtdParseFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
#ifndef YYPARSEFREENEVERNULL
  if( pParser==0 ) return;
#endif
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int DtdParseStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyidxMax;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
*/
static int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;

  if( stateno>=YY_MIN_REDUCE ) return stateno;
  assert( stateno <= YY_SHIFT_COUNT );
  do{
    i = yy_shift_ofst[stateno];
    if( i==YY_SHIFT_USE_DFLT ) return yy_default[stateno];
    assert( iLookAhead!=YYNOCODE );
    i += iLookAhead;
    if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
      if( iLookAhead>0 ){
#ifdef YYFALLBACK
        YYCODETYPE iFallback;            /* Fallback token */
        if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
               && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
          }
#endif
          assert( yyFallback[iFallback]==0 ); /* Fallback loop must terminate */
          iLookAhead = iFallback;
          continue;
        }
#endif
#ifdef YYWILDCARD
        {
          int j = i - iLookAhead + YYWILDCARD;
          if(
#if YY_SHIFT_MIN+YYWILDCARD<0
            j>=0 &&
#endif
#if YY_SHIFT_MAX+YYWILDCARD>=YY_ACTTAB_COUNT
            j<YY_ACTTAB_COUNT &&
#endif
            yy_lookahead[j]==YYWILDCARD
          ){
#ifndef NDEBUG
            if( yyTraceFILE ){
              fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
                 yyTracePrompt, yyTokenName[iLookAhead],
                 yyTokenName[YYWILDCARD]);
            }
#endif /* NDEBUG */
            return yy_action[j];
          }
        }
#endif /* YYWILDCARD */
      }
      return yy_default[stateno];
    }else{
      return yy_action[i];
    }
  }while(1);
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_COUNT ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_COUNT );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_ACTTAB_COUNT );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser, YYMINORTYPE *yypMinor _U_){
   DtdParseARG_FETCH;
   yypParser->yyidx--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
/******** Begin %stack_overflow code ******************************************/
/******** End %stack_overflow code ********************************************/
   DtdParseARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Print tracing information for a SHIFT action
*/
#ifndef NDEBUG
static void yyTraceShift(yyParser *yypParser, int yyNewState){
  if( yyTraceFILE ){
    if( yyNewState<YYNSTATE ){
      fprintf(yyTraceFILE,"%sShift '%s', go to state %d\n",
         yyTracePrompt,yyTokenName[yypParser->yystack[yypParser->yyidx].major],
         yyNewState);
    }else{
      fprintf(yyTraceFILE,"%sShift '%s'\n",
         yyTracePrompt,yyTokenName[yypParser->yystack[yypParser->yyidx].major]);
    }
  }
}
#else
# define yyTraceShift(X,Y)
#endif

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer to the minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( yypParser->yyidx>yypParser->yyidxMax ){
    yypParser->yyidxMax = yypParser->yyidx;
  }
#endif
#if YYSTACKDEPTH>0
  if( yypParser->yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(yypParser, yypMinor);
    return;
  }
#else
  if( yypParser->yyidx>=yypParser->yystksz ){
    yyGrowStack(yypParser);
    if( yypParser->yyidx>=yypParser->yystksz ){
      yyStackOverflow(yypParser, yypMinor);
      return;
    }
  }
#endif
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor = *yypMinor;
  yyTraceShift(yypParser, yyNewState);
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 25, 1 },
  { 25, 1 },
  { 26, 7 },
  { 27, 2 },
  { 27, 2 },
  { 27, 1 },
  { 27, 1 },
  { 29, 5 },
  { 28, 5 },
  { 30, 2 },
  { 30, 1 },
  { 32, 3 },
  { 33, 1 },
  { 33, 1 },
  { 34, 1 },
  { 34, 2 },
  { 34, 1 },
  { 34, 1 },
  { 34, 1 },
  { 35, 3 },
  { 36, 3 },
  { 36, 1 },
  { 36, 1 },
  { 36, 3 },
  { 37, 1 },
  { 37, 1 },
  { 31, 2 },
  { 31, 2 },
  { 31, 2 },
  { 31, 3 },
  { 31, 5 },
  { 31, 5 },
  { 31, 3 },
  { 31, 1 },
  { 38, 3 },
  { 38, 3 },
  { 38, 1 },
  { 38, 1 },
  { 38, 3 },
  { 38, 3 },
  { 39, 1 },
  { 39, 2 },
  { 39, 2 },
  { 39, 2 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  DtdParseARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno>=0
        && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    yysize = yyRuleInfo[yyruleno].nrhs;
    fprintf(yyTraceFILE, "%sReduce [%s], go to state %d.\n", yyTracePrompt,
      yyRuleName[yyruleno], yymsp[-yysize].stateno);
  }
#endif /* NDEBUG */
  yygotominor = yyzerominor;

  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
/********** Begin reduce actions **********************************************/
      case 2: /* doctype ::= TAG_START DOCTYPE_KW NAME OPEN_BRACKET dtd_parts CLOSE_BRACKET TAG_STOP */
#line 89 "dtd_grammar.lemon"
{
	dtd_named_list_t* root;
	GPtrArray* root_elems = g_ptr_array_new();
	guint i;
	gchar *name;

	if(! bd->proto_name) {
		bd->proto_name = yymsp[-4].minor.yy0->text;
	}

	if(bd->proto_root)
		g_free(bd->proto_root);

	bd->proto_root = yymsp[-4].minor.yy0->text;

	name = g_ascii_strdown(bd->proto_name, -1);
	g_free(bd->proto_name);
	bd->proto_name = name;

	for( i = 0; i< bd->elements->len; i++) {
		dtd_named_list_t* el = (dtd_named_list_t*)g_ptr_array_index(bd->elements,i);

		g_ptr_array_add(root_elems,g_strdup(el->name));
	}

	root = dtd_named_list_new(g_strdup(yymsp[-4].minor.yy0->text),root_elems);

	g_ptr_array_add(bd->elements,root);

	g_free(yymsp[-4].minor.yy0->location);
	g_free(yymsp[-4].minor.yy0);

  yy_destructor(yypParser,1,&yymsp[-6].minor);
  yy_destructor(yypParser,2,&yymsp[-5].minor);
  yy_destructor(yypParser,4,&yymsp[-3].minor);
  yy_destructor(yypParser,5,&yymsp[-1].minor);
  yy_destructor(yypParser,6,&yymsp[0].minor);
}
#line 940 "dtd_grammar.c"
        break;
      case 3: /* dtd_parts ::= dtd_parts element */
      case 5: /* dtd_parts ::= element */ yytestcase(yyruleno==5);
#line 123 "dtd_grammar.lemon"
{ g_ptr_array_add(bd->elements,yymsp[0].minor.yy29); }
#line 946 "dtd_grammar.c"
        break;
      case 4: /* dtd_parts ::= dtd_parts attlist */
      case 6: /* dtd_parts ::= attlist */ yytestcase(yyruleno==6);
#line 124 "dtd_grammar.lemon"
{ g_ptr_array_add(bd->attributes,yymsp[0].minor.yy29); }
#line 952 "dtd_grammar.c"
        break;
      case 7: /* attlist ::= TAG_START ATTLIST_KW NAME attrib_list TAG_STOP */
#line 129 "dtd_grammar.lemon"
{
	yygotominor.yy29 = dtd_named_list_new(g_ascii_strdown(yymsp[-2].minor.yy0->text, -1),yymsp[-1].minor.yy59);
	g_free(yymsp[-2].minor.yy0->text);
	g_free(yymsp[-2].minor.yy0->location);
	g_free(yymsp[-2].minor.yy0);
  yy_destructor(yypParser,1,&yymsp[-4].minor);
  yy_destructor(yypParser,7,&yymsp[-3].minor);
  yy_destructor(yypParser,6,&yymsp[0].minor);
}
#line 965 "dtd_grammar.c"
        break;
      case 8: /* element ::= TAG_START ELEMENT_KW NAME sub_elements TAG_STOP */
#line 137 "dtd_grammar.lemon"
{
	yygotominor.yy29 = dtd_named_list_new(g_ascii_strdown(yymsp[-2].minor.yy0->text, -1),yymsp[-1].minor.yy59);
	g_free(yymsp[-2].minor.yy0->text);
	g_free(yymsp[-2].minor.yy0->location);
	g_free(yymsp[-2].minor.yy0);
  yy_destructor(yypParser,1,&yymsp[-4].minor);
  yy_destructor(yypParser,8,&yymsp[-3].minor);
  yy_destructor(yypParser,6,&yymsp[0].minor);
}
#line 978 "dtd_grammar.c"
        break;
      case 9: /* attrib_list ::= attrib_list attrib */
#line 145 "dtd_grammar.lemon"
{ g_ptr_array_add(yymsp[-1].minor.yy59,yymsp[0].minor.yy44); yygotominor.yy59 = yymsp[-1].minor.yy59; }
#line 983 "dtd_grammar.c"
        break;
      case 10: /* attrib_list ::= attrib */
#line 146 "dtd_grammar.lemon"
{ yygotominor.yy59 = g_ptr_array_new(); g_ptr_array_add(yygotominor.yy59,yymsp[0].minor.yy44);  }
#line 988 "dtd_grammar.c"
        break;
      case 11: /* attrib ::= NAME att_type att_default */
#line 149 "dtd_grammar.lemon"
{
	yygotominor.yy44 = g_ascii_strdown(yymsp[-2].minor.yy0->text, -1);
	g_free(yymsp[-2].minor.yy0->text);
	g_free(yymsp[-2].minor.yy0->location);
	g_free(yymsp[-2].minor.yy0);
}
#line 998 "dtd_grammar.c"
        break;
      case 12: /* att_type ::= ATT_TYPE */
#line 156 "dtd_grammar.lemon"
{
  yy_destructor(yypParser,9,&yymsp[0].minor);
}
#line 1005 "dtd_grammar.c"
        break;
      case 14: /* att_default ::= ATT_DEF */
#line 159 "dtd_grammar.lemon"
{
  yy_destructor(yypParser,10,&yymsp[0].minor);
}
#line 1012 "dtd_grammar.c"
        break;
      case 15: /* att_default ::= ATT_DEF_WITH_VALUE QUOTED */
#line 160 "dtd_grammar.lemon"
{
  yy_destructor(yypParser,11,&yymsp[-1].minor);
  yy_destructor(yypParser,12,&yymsp[0].minor);
}
#line 1020 "dtd_grammar.c"
        break;
      case 16: /* att_default ::= QUOTED */
      case 25: /* enum_item ::= QUOTED */ yytestcase(yyruleno==25);
#line 161 "dtd_grammar.lemon"
{
  yy_destructor(yypParser,12,&yymsp[0].minor);
}
#line 1028 "dtd_grammar.c"
        break;
      case 17: /* att_default ::= IMPLIED_KW */
#line 162 "dtd_grammar.lemon"
{
  yy_destructor(yypParser,13,&yymsp[0].minor);
}
#line 1035 "dtd_grammar.c"
        break;
      case 18: /* att_default ::= REQUIRED_KW */
#line 163 "dtd_grammar.lemon"
{
  yy_destructor(yypParser,14,&yymsp[0].minor);
}
#line 1042 "dtd_grammar.c"
        break;
      case 19: /* enumeration ::= OPEN_PARENS enum_list CLOSE_PARENS */
#line 165 "dtd_grammar.lemon"
{
  yy_destructor(yypParser,15,&yymsp[-2].minor);
  yy_destructor(yypParser,16,&yymsp[0].minor);
}
#line 1050 "dtd_grammar.c"
        break;
      case 20: /* enum_list ::= enum_list PIPE enum_item */
      case 23: /* enum_list ::= enum_list PIPE enumeration */ yytestcase(yyruleno==23);
#line 167 "dtd_grammar.lemon"
{
  yy_destructor(yypParser,17,&yymsp[-1].minor);
}
#line 1058 "dtd_grammar.c"
        break;
      case 24: /* enum_item ::= NAME */
#line 172 "dtd_grammar.lemon"
{
  yy_destructor(yypParser,3,&yymsp[0].minor);
}
#line 1065 "dtd_grammar.c"
        break;
      case 26: /* sub_elements ::= sub_elements STAR */
#line 177 "dtd_grammar.lemon"
{yygotominor.yy59=yymsp[-1].minor.yy59;  yy_destructor(yypParser,18,&yymsp[0].minor);
}
#line 1071 "dtd_grammar.c"
        break;
      case 27: /* sub_elements ::= sub_elements PLUS */
#line 178 "dtd_grammar.lemon"
{yygotominor.yy59=yymsp[-1].minor.yy59;  yy_destructor(yypParser,19,&yymsp[0].minor);
}
#line 1077 "dtd_grammar.c"
        break;
      case 28: /* sub_elements ::= sub_elements QUESTION */
#line 179 "dtd_grammar.lemon"
{yygotominor.yy59=yymsp[-1].minor.yy59;  yy_destructor(yypParser,20,&yymsp[0].minor);
}
#line 1083 "dtd_grammar.c"
        break;
      case 29: /* sub_elements ::= OPEN_PARENS ELEM_DATA CLOSE_PARENS */
#line 180 "dtd_grammar.lemon"
{ yygotominor.yy59 = g_ptr_array_new();   yy_destructor(yypParser,15,&yymsp[-2].minor);
  yy_destructor(yypParser,21,&yymsp[-1].minor);
  yy_destructor(yypParser,16,&yymsp[0].minor);
}
#line 1091 "dtd_grammar.c"
        break;
      case 30: /* sub_elements ::= OPEN_PARENS element_list COMMA ELEM_DATA CLOSE_PARENS */
#line 181 "dtd_grammar.lemon"
{ yygotominor.yy59 = yymsp[-3].minor.yy59;   yy_destructor(yypParser,15,&yymsp[-4].minor);
  yy_destructor(yypParser,22,&yymsp[-2].minor);
  yy_destructor(yypParser,21,&yymsp[-1].minor);
  yy_destructor(yypParser,16,&yymsp[0].minor);
}
#line 1100 "dtd_grammar.c"
        break;
      case 31: /* sub_elements ::= OPEN_PARENS element_list PIPE ELEM_DATA CLOSE_PARENS */
#line 182 "dtd_grammar.lemon"
{ yygotominor.yy59 = yymsp[-3].minor.yy59;   yy_destructor(yypParser,15,&yymsp[-4].minor);
  yy_destructor(yypParser,17,&yymsp[-2].minor);
  yy_destructor(yypParser,21,&yymsp[-1].minor);
  yy_destructor(yypParser,16,&yymsp[0].minor);
}
#line 1109 "dtd_grammar.c"
        break;
      case 32: /* sub_elements ::= OPEN_PARENS element_list CLOSE_PARENS */
#line 183 "dtd_grammar.lemon"
{ yygotominor.yy59 = yymsp[-1].minor.yy59;   yy_destructor(yypParser,15,&yymsp[-2].minor);
  yy_destructor(yypParser,16,&yymsp[0].minor);
}
#line 1116 "dtd_grammar.c"
        break;
      case 33: /* sub_elements ::= EMPTY_KW */
#line 184 "dtd_grammar.lemon"
{ yygotominor.yy59 = g_ptr_array_new();   yy_destructor(yypParser,23,&yymsp[0].minor);
}
#line 1122 "dtd_grammar.c"
        break;
      case 34: /* element_list ::= element_list COMMA element_child */
#line 187 "dtd_grammar.lemon"
{ g_ptr_array_add(yymsp[-2].minor.yy59,yymsp[0].minor.yy44); yygotominor.yy59 = yymsp[-2].minor.yy59;   yy_destructor(yypParser,22,&yymsp[-1].minor);
}
#line 1128 "dtd_grammar.c"
        break;
      case 35: /* element_list ::= element_list PIPE element_child */
#line 188 "dtd_grammar.lemon"
{ g_ptr_array_add(yymsp[-2].minor.yy59,yymsp[0].minor.yy44); yygotominor.yy59 = yymsp[-2].minor.yy59;   yy_destructor(yypParser,17,&yymsp[-1].minor);
}
#line 1134 "dtd_grammar.c"
        break;
      case 36: /* element_list ::= element_child */
#line 189 "dtd_grammar.lemon"
{ yygotominor.yy59 = g_ptr_array_new(); g_ptr_array_add(yygotominor.yy59,yymsp[0].minor.yy44); }
#line 1139 "dtd_grammar.c"
        break;
      case 37: /* element_list ::= sub_elements */
#line 190 "dtd_grammar.lemon"
{ yygotominor.yy59 = yymsp[0].minor.yy59; }
#line 1144 "dtd_grammar.c"
        break;
      case 38: /* element_list ::= element_list COMMA sub_elements */
#line 191 "dtd_grammar.lemon"
{ yygotominor.yy59 = g_ptr_array_join(yymsp[-2].minor.yy59,yymsp[0].minor.yy59);   yy_destructor(yypParser,22,&yymsp[-1].minor);
}
#line 1150 "dtd_grammar.c"
        break;
      case 39: /* element_list ::= element_list PIPE sub_elements */
#line 192 "dtd_grammar.lemon"
{ yygotominor.yy59 = g_ptr_array_join(yymsp[-2].minor.yy59,yymsp[0].minor.yy59);   yy_destructor(yypParser,17,&yymsp[-1].minor);
}
#line 1156 "dtd_grammar.c"
        break;
      case 40: /* element_child ::= NAME */
#line 195 "dtd_grammar.lemon"
{
	yygotominor.yy44 = g_ascii_strdown(yymsp[0].minor.yy0->text, -1);
	g_free(yymsp[0].minor.yy0->text);
	g_free(yymsp[0].minor.yy0->location);
	g_free(yymsp[0].minor.yy0);
}
#line 1166 "dtd_grammar.c"
        break;
      case 41: /* element_child ::= NAME STAR */
#line 202 "dtd_grammar.lemon"
{
	yygotominor.yy44 = g_ascii_strdown(yymsp[-1].minor.yy0->text, -1);
	g_free(yymsp[-1].minor.yy0->text);
	g_free(yymsp[-1].minor.yy0->location);
	g_free(yymsp[-1].minor.yy0);
  yy_destructor(yypParser,18,&yymsp[0].minor);
}
#line 1177 "dtd_grammar.c"
        break;
      case 42: /* element_child ::= NAME QUESTION */
#line 209 "dtd_grammar.lemon"
{
	yygotominor.yy44 = g_ascii_strdown(yymsp[-1].minor.yy0->text, -1);
	g_free(yymsp[-1].minor.yy0->text);
	g_free(yymsp[-1].minor.yy0->location);
	g_free(yymsp[-1].minor.yy0);
  yy_destructor(yypParser,20,&yymsp[0].minor);
}
#line 1188 "dtd_grammar.c"
        break;
      case 43: /* element_child ::= NAME PLUS */
#line 216 "dtd_grammar.lemon"
{
	yygotominor.yy44 = g_ascii_strdown(yymsp[-1].minor.yy0->text, -1);
	g_free(yymsp[-1].minor.yy0->text);
	g_free(yymsp[-1].minor.yy0->location);
	g_free(yymsp[-1].minor.yy0);
  yy_destructor(yypParser,19,&yymsp[0].minor);
}
#line 1199 "dtd_grammar.c"
        break;
      default:
      /* (0) dtd ::= doctype */ yytestcase(yyruleno==0);
      /* (1) dtd ::= dtd_parts */ yytestcase(yyruleno==1);
      /* (13) att_type ::= enumeration */ yytestcase(yyruleno==13);
      /* (21) enum_list ::= enum_item */ yytestcase(yyruleno==21);
      /* (22) enum_list ::= enumeration */ yytestcase(yyruleno==22);
        break;
/********** End reduce actions ************************************************/
  };
  assert( yyruleno>=0 && yyruleno<(int)(sizeof(yyRuleInfo)/sizeof(yyRuleInfo[0])) );
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact <= YY_MAX_SHIFTREDUCE ){
    if( yyact>YY_MAX_SHIFT ) yyact += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
    /* If the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      yypParser->yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = (YYACTIONTYPE)yyact;
      yymsp->major = (YYCODETYPE)yygoto;
      yymsp->minor = yygotominor;
      yyTraceShift(yypParser, yyact);
    }else{
      yy_shift(yypParser,yyact,yygoto,&yygotominor);
    }
  }else{
    assert( yyact == YY_ACCEPT_ACTION );
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  DtdParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
/************ Begin %parse_failure code ***************************************/
#line 78 "./dtd_grammar.lemon"

	g_string_append_printf(bd->error,"DTD parsing failure\n");
#line 1257 "dtd_grammar.c"
/************ End %parse_failure code *****************************************/
  DtdParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor _U_,               /* The major type of the error token */
  YYMINORTYPE yyminor            /* The minor type of the error token */
){
  DtdParseARG_FETCH;
#define TOKEN (yyminor.yy0)
/************ Begin %syntax_error code ****************************************/
#line 71 "./dtd_grammar.lemon"

	if (!TOKEN)
		g_string_append_printf(bd->error,"syntax error at end of file");
	else 
		g_string_append_printf(bd->error,"syntax error in %s at or before '%s': \n", TOKEN->location,TOKEN->text);
#line 1280 "dtd_grammar.c"
/************ End %syntax_error code ******************************************/
  DtdParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  DtdParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
/*********** Begin %parse_accept code *****************************************/
/*********** End %parse_accept code *******************************************/
  DtdParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "DtdParseAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void DtdParse(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  DtdParseTOKENTYPE yyminor       /* The value for the token */
  DtdParseARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
#if YYSTACKDEPTH<=0
    if( yypParser->yystksz <=0 ){
      /*memset(&yyminorunion, 0, sizeof(yyminorunion));*/
      yyminorunion = yyzerominor;
      yyStackOverflow(yypParser, &yyminorunion);
      return;
    }
#endif
    yypParser->yyidx = 0;
    yypParser->yyerrcnt = -1;
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sInitialize. Empty stack. State 0\n",
              yyTracePrompt);
    }
#endif
  }
  yyminorunion.yy0 = yyminor;
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  yyendofinput = (yymajor==0);
#endif
  DtdParseARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput '%s'\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact <= YY_MAX_SHIFTREDUCE ){
      if( yyact > YY_MAX_SHIFT ) yyact += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
      yy_shift(yypParser,yyact,yymajor,&yyminorunion);
      yypParser->yyerrcnt--;
      yymajor = YYNOCODE;
    }else if( yyact <= YY_MAX_REDUCE ){
      yy_reduce(yypParser,yyact-YY_MIN_REDUCE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YY_MIN_REDUCE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor,yyminorunion);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      yymajor = YYNOCODE;

#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
#ifndef NDEBUG
  if( yyTraceFILE ){
    int i;
    fprintf(yyTraceFILE,"%sReturn. Stack=",yyTracePrompt);
    for(i=1; i<=yypParser->yyidx; i++)
      fprintf(yyTraceFILE,"%c%s", i==1 ? '[' : ' ',
              yyTokenName[yypParser->yystack[i].major]);
    fprintf(yyTraceFILE,"]\n");
  }
#endif
  return;
}
