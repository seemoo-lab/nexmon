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
#line 2 "./grammar.lemon"

#include "config.h"

#include <assert.h>

#include "dfilter-int.h"
#include "syntax-tree.h"
#include "sttype-range.h"
#include "sttype-test.h"
#include "sttype-function.h"
#include "sttype-set.h"
#include "drange.h"

#include "grammar.h"

#ifdef _WIN32
#pragma warning(disable:4671)
#endif

/* End of C code */
#line 49 "grammar.c"
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
**    DfilterTOKENTYPE     is the data type used for minor type for terminal
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
**                       which is DfilterTOKENTYPE.  The entry in the union
**                       for terminal symbols is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    DfilterARG_SDECL     A static variable declaration for the %extra_argument
**    DfilterARG_PDECL     A parameter declaration for the %extra_argument
**    DfilterARG_STORE     Code to store %extra_argument into yypParser
**    DfilterARG_FETCH     Code to extract %extra_argument from yypParser
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
#define YYNOCODE 42
#define YYACTIONTYPE unsigned char
#define DfilterTOKENTYPE stnode_t*
typedef union {
  int yyinit;
  DfilterTOKENTYPE yy0;
  test_op_t yy26;
  GSList* yy69;
  drange_node* yy79;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define DfilterARG_SDECL dfwork_t *dfw;
#define DfilterARG_PDECL ,dfwork_t *dfw
#define DfilterARG_FETCH dfwork_t *dfw = yypParser->dfw
#define DfilterARG_STORE yypParser->dfw = dfw
#define YYNSTATE             29
#define YYNRULE              42
#define YY_MAX_SHIFT         28
#define YY_MIN_SHIFTREDUCE   58
#define YY_MAX_SHIFTREDUCE   99
#define YY_MIN_REDUCE        100
#define YY_MAX_REDUCE        141
#define YY_ERROR_ACTION      142
#define YY_ACCEPT_ACTION     143
#define YY_NO_ACTION         144
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
#define YY_ACTTAB_COUNT (116)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */    83,   84,   87,   88,   85,   86,   90,   91,   89,  101,
 /*    10 */     7,    4,    3,   75,  143,   15,   12,   60,   61,   20,
 /*    20 */    25,    2,   27,   26,   68,   24,    5,    2,   27,   26,
 /*    30 */    68,   27,   26,   68,   19,    1,    6,   99,   17,   21,
 /*    40 */    19,    1,   78,   19,   77,   96,   14,   12,   60,   61,
 /*    50 */    76,   25,   64,   12,   60,   61,   24,   25,   63,   12,
 /*    60 */    60,   61,   24,   25,   23,   22,   11,   82,   24,   10,
 /*    70 */    25,   28,   12,   60,   61,   24,   25,   27,   26,   68,
 /*    80 */     3,   24,  114,   97,  100,    4,    3,   25,   92,   19,
 /*    90 */    16,  102,   24,   27,   26,   68,   74,   18,   94,  113,
 /*   100 */    93,  102,   25,    9,   25,   19,   98,   24,    8,   24,
 /*   110 */    25,  112,   95,   73,   13,   24,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     3,    4,    5,    6,    7,    8,    9,   10,   11,    0,
 /*    10 */    34,    1,    2,   36,   29,   30,   31,   32,   33,   22,
 /*    20 */    35,   12,   13,   14,   15,   40,   26,   12,   13,   14,
 /*    30 */    15,   13,   14,   15,   25,   26,   23,   27,   19,   20,
 /*    40 */    25,   26,   19,   25,   19,   27,   30,   31,   32,   33,
 /*    50 */    19,   35,   30,   31,   32,   33,   40,   35,   30,   31,
 /*    60 */    32,   33,   40,   35,   20,   21,   31,   32,   40,   16,
 /*    70 */    35,   30,   31,   32,   33,   40,   35,   13,   14,   15,
 /*    80 */     2,   40,   16,   31,    0,    1,    2,   35,   24,   25,
 /*    90 */    38,   41,   40,   13,   14,   15,   36,   37,   31,   16,
 /*   100 */    31,   41,   35,   18,   35,   25,   31,   40,   39,   40,
 /*   110 */    35,   16,   27,   17,   18,   40,
};
#define YY_SHIFT_USE_DFLT (-4)
#define YY_SHIFT_COUNT (28)
#define YY_SHIFT_MIN   (-3)
#define YY_SHIFT_MAX   (96)
static const signed char yy_shift_ofst[] = {
 /*     0 */     9,   15,   15,   15,   15,   18,   80,   80,   64,   80,
 /*    10 */    19,   -3,   -3,   19,   10,   84,   85,   44,   96,    0,
 /*    20 */    13,   23,   25,   31,   53,   66,   83,   95,   78,
};
#define YY_REDUCE_USE_DFLT (-25)
#define YY_REDUCE_COUNT (13)
#define YY_REDUCE_MIN   (-24)
#define YY_REDUCE_MAX   (75)
static const signed char yy_reduce_ofst[] = {
 /*     0 */   -15,   16,   22,   28,   41,   52,   69,   35,   67,   75,
 /*    10 */    60,  -24,  -24,  -23,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   142,  142,  142,  142,  142,  142,  142,  142,  142,  142,
 /*    10 */   142,  123,  107,  142,  142,  142,  142,  122,  142,  142,
 /*    20 */   142,  142,  142,  121,  142,  111,  109,  108,  104,
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
  DfilterARG_SDECL                /* A place to hold %extra_argument */
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
void DfilterTrace(FILE *TraceFILE, char *zTracePrompt){
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
  "$",             "TEST_AND",      "TEST_OR",       "TEST_EQ",     
  "TEST_NE",       "TEST_LT",       "TEST_LE",       "TEST_GT",     
  "TEST_GE",       "TEST_CONTAINS",  "TEST_MATCHES",  "TEST_BITWISE_AND",
  "TEST_NOT",      "FIELD",         "STRING",        "UNPARSED",    
  "LBRACKET",      "RBRACKET",      "COMMA",         "INTEGER",     
  "COLON",         "HYPHEN",        "TEST_IN",       "LBRACE",      
  "RBRACE",        "FUNCTION",      "LPAREN",        "RPAREN",      
  "error",         "sentence",      "expr",          "entity",      
  "relation_test",  "logical_test",  "rel_op2",       "range",       
  "drnode",        "drnode_list",   "funcparams",    "setnode_list",
  "range_body",  
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "sentence ::= expr",
 /*   1 */ "sentence ::=",
 /*   2 */ "expr ::= relation_test",
 /*   3 */ "expr ::= logical_test",
 /*   4 */ "logical_test ::= expr TEST_AND expr",
 /*   5 */ "logical_test ::= expr TEST_OR expr",
 /*   6 */ "logical_test ::= TEST_NOT expr",
 /*   7 */ "logical_test ::= entity",
 /*   8 */ "entity ::= FIELD",
 /*   9 */ "entity ::= STRING",
 /*  10 */ "entity ::= UNPARSED",
 /*  11 */ "entity ::= range",
 /*  12 */ "range_body ::= FIELD",
 /*  13 */ "range_body ::= STRING",
 /*  14 */ "range_body ::= range",
 /*  15 */ "range ::= range_body LBRACKET drnode_list RBRACKET",
 /*  16 */ "drnode_list ::= drnode",
 /*  17 */ "drnode_list ::= drnode_list COMMA drnode",
 /*  18 */ "drnode ::= INTEGER COLON INTEGER",
 /*  19 */ "drnode ::= INTEGER HYPHEN INTEGER",
 /*  20 */ "drnode ::= COLON INTEGER",
 /*  21 */ "drnode ::= INTEGER COLON",
 /*  22 */ "drnode ::= INTEGER",
 /*  23 */ "relation_test ::= entity rel_op2 entity",
 /*  24 */ "relation_test ::= entity rel_op2 relation_test",
 /*  25 */ "rel_op2 ::= TEST_EQ",
 /*  26 */ "rel_op2 ::= TEST_NE",
 /*  27 */ "rel_op2 ::= TEST_GT",
 /*  28 */ "rel_op2 ::= TEST_GE",
 /*  29 */ "rel_op2 ::= TEST_LT",
 /*  30 */ "rel_op2 ::= TEST_LE",
 /*  31 */ "rel_op2 ::= TEST_BITWISE_AND",
 /*  32 */ "rel_op2 ::= TEST_CONTAINS",
 /*  33 */ "rel_op2 ::= TEST_MATCHES",
 /*  34 */ "relation_test ::= entity TEST_IN LBRACE setnode_list RBRACE",
 /*  35 */ "setnode_list ::= entity",
 /*  36 */ "setnode_list ::= setnode_list entity",
 /*  37 */ "entity ::= FUNCTION LPAREN funcparams RPAREN",
 /*  38 */ "entity ::= FUNCTION LPAREN RPAREN",
 /*  39 */ "funcparams ::= entity",
 /*  40 */ "funcparams ::= funcparams COMMA entity",
 /*  41 */ "expr ::= LPAREN expr RPAREN",
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
** second argument to DfilterAlloc() below.  This can be changed by
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
** to Dfilter and DfilterFree.
*/
void *DfilterAlloc(void *(*mallocProc)(YYMALLOCARGTYPE)){
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
  DfilterARG_FETCH;
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
    case 1: /* TEST_AND */
    case 2: /* TEST_OR */
    case 3: /* TEST_EQ */
    case 4: /* TEST_NE */
    case 5: /* TEST_LT */
    case 6: /* TEST_LE */
    case 7: /* TEST_GT */
    case 8: /* TEST_GE */
    case 9: /* TEST_CONTAINS */
    case 10: /* TEST_MATCHES */
    case 11: /* TEST_BITWISE_AND */
    case 12: /* TEST_NOT */
    case 13: /* FIELD */
    case 14: /* STRING */
    case 15: /* UNPARSED */
    case 16: /* LBRACKET */
    case 17: /* RBRACKET */
    case 18: /* COMMA */
    case 19: /* INTEGER */
    case 20: /* COLON */
    case 21: /* HYPHEN */
    case 22: /* TEST_IN */
    case 23: /* LBRACE */
    case 24: /* RBRACE */
    case 25: /* FUNCTION */
    case 26: /* LPAREN */
    case 27: /* RPAREN */
{
#line 31 "./grammar.lemon"

	(void) dfw; /* Mark unused, similar to Q_UNUSED */
	stnode_free((yypminor->yy0));

#line 542 "grammar.c"
}
      break;
    case 30: /* expr */
    case 31: /* entity */
    case 32: /* relation_test */
    case 33: /* logical_test */
    case 35: /* range */
{
#line 40 "grammar.lemon"
stnode_free((yypminor->yy0));
#line 553 "grammar.c"
}
      break;
    case 36: /* drnode */
{
#line 57 "grammar.lemon"
drange_node_free((yypminor->yy79));
#line 560 "grammar.c"
}
      break;
    case 37: /* drnode_list */
{
#line 60 "grammar.lemon"
drange_node_free_list((yypminor->yy69));
#line 567 "grammar.c"
}
      break;
    case 38: /* funcparams */
{
#line 63 "grammar.lemon"
st_funcparams_free((yypminor->yy69));
#line 574 "grammar.c"
}
      break;
    case 39: /* setnode_list */
{
#line 66 "grammar.lemon"
set_nodelist_free((yypminor->yy69));
#line 581 "grammar.c"
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
void DfilterFree(
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
int DfilterStackPeak(void *p){
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
   DfilterARG_FETCH;
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
   DfilterARG_STORE; /* Suppress warning about unused %extra_argument var */
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
  { 29, 1 },
  { 29, 0 },
  { 30, 1 },
  { 30, 1 },
  { 33, 3 },
  { 33, 3 },
  { 33, 2 },
  { 33, 1 },
  { 31, 1 },
  { 31, 1 },
  { 31, 1 },
  { 31, 1 },
  { 40, 1 },
  { 40, 1 },
  { 40, 1 },
  { 35, 4 },
  { 37, 1 },
  { 37, 3 },
  { 36, 3 },
  { 36, 3 },
  { 36, 2 },
  { 36, 2 },
  { 36, 1 },
  { 32, 3 },
  { 32, 3 },
  { 34, 1 },
  { 34, 1 },
  { 34, 1 },
  { 34, 1 },
  { 34, 1 },
  { 34, 1 },
  { 34, 1 },
  { 34, 1 },
  { 34, 1 },
  { 32, 5 },
  { 39, 1 },
  { 39, 2 },
  { 31, 4 },
  { 31, 3 },
  { 38, 1 },
  { 38, 3 },
  { 30, 3 },
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
  DfilterARG_FETCH;
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
      case 0: /* sentence ::= expr */
#line 139 "grammar.lemon"
{ dfw->st_root = yymsp[0].minor.yy0; }
#line 906 "grammar.c"
        break;
      case 1: /* sentence ::= */
#line 140 "grammar.lemon"
{ dfw->st_root = NULL; }
#line 911 "grammar.c"
        break;
      case 2: /* expr ::= relation_test */
      case 3: /* expr ::= logical_test */ yytestcase(yyruleno==3);
      case 8: /* entity ::= FIELD */ yytestcase(yyruleno==8);
      case 9: /* entity ::= STRING */ yytestcase(yyruleno==9);
      case 10: /* entity ::= UNPARSED */ yytestcase(yyruleno==10);
      case 11: /* entity ::= range */ yytestcase(yyruleno==11);
      case 12: /* range_body ::= FIELD */ yytestcase(yyruleno==12);
      case 13: /* range_body ::= STRING */ yytestcase(yyruleno==13);
      case 14: /* range_body ::= range */ yytestcase(yyruleno==14);
#line 142 "grammar.lemon"
{ yygotominor.yy0 = yymsp[0].minor.yy0; }
#line 924 "grammar.c"
        break;
      case 4: /* logical_test ::= expr TEST_AND expr */
#line 148 "grammar.lemon"
{
	yygotominor.yy0 = stnode_new(STTYPE_TEST, NULL);
	sttype_test_set2(yygotominor.yy0, TEST_OP_AND, yymsp[-2].minor.yy0, yymsp[0].minor.yy0);
  yy_destructor(yypParser,1,&yymsp[-1].minor);
}
#line 933 "grammar.c"
        break;
      case 5: /* logical_test ::= expr TEST_OR expr */
#line 154 "grammar.lemon"
{
	yygotominor.yy0 = stnode_new(STTYPE_TEST, NULL);
	sttype_test_set2(yygotominor.yy0, TEST_OP_OR, yymsp[-2].minor.yy0, yymsp[0].minor.yy0);
  yy_destructor(yypParser,2,&yymsp[-1].minor);
}
#line 942 "grammar.c"
        break;
      case 6: /* logical_test ::= TEST_NOT expr */
#line 160 "grammar.lemon"
{
	yygotominor.yy0 = stnode_new(STTYPE_TEST, NULL);
	sttype_test_set1(yygotominor.yy0, TEST_OP_NOT, yymsp[0].minor.yy0);
  yy_destructor(yypParser,12,&yymsp[-1].minor);
}
#line 951 "grammar.c"
        break;
      case 7: /* logical_test ::= entity */
#line 166 "grammar.lemon"
{
	yygotominor.yy0 = stnode_new(STTYPE_TEST, NULL);
	sttype_test_set1(yygotominor.yy0, TEST_OP_EXISTS, yymsp[0].minor.yy0);
}
#line 959 "grammar.c"
        break;
      case 15: /* range ::= range_body LBRACKET drnode_list RBRACKET */
#line 185 "grammar.lemon"
{
	yygotominor.yy0 = stnode_new(STTYPE_RANGE, NULL);
	sttype_range_set(yygotominor.yy0, yymsp[-3].minor.yy0, yymsp[-1].minor.yy69);

	/* Delete the list, but not the drange_nodes that
	 * the list contains. */
	g_slist_free(yymsp[-1].minor.yy69);
  yy_destructor(yypParser,16,&yymsp[-2].minor);
  yy_destructor(yypParser,17,&yymsp[0].minor);
}
#line 973 "grammar.c"
        break;
      case 16: /* drnode_list ::= drnode */
#line 195 "grammar.lemon"
{
	yygotominor.yy69 = g_slist_append(NULL, yymsp[0].minor.yy79);
}
#line 980 "grammar.c"
        break;
      case 17: /* drnode_list ::= drnode_list COMMA drnode */
#line 200 "grammar.lemon"
{
	yygotominor.yy69 = g_slist_append(yymsp[-2].minor.yy69, yymsp[0].minor.yy79);
  yy_destructor(yypParser,18,&yymsp[-1].minor);
}
#line 988 "grammar.c"
        break;
      case 18: /* drnode ::= INTEGER COLON INTEGER */
#line 206 "grammar.lemon"
{
	yygotominor.yy79 = drange_node_new();
	drange_node_set_start_offset(yygotominor.yy79, stnode_value(yymsp[-2].minor.yy0));
	drange_node_set_length(yygotominor.yy79, stnode_value(yymsp[0].minor.yy0));

	stnode_free(yymsp[-2].minor.yy0);
	stnode_free(yymsp[0].minor.yy0);
  yy_destructor(yypParser,20,&yymsp[-1].minor);
}
#line 1001 "grammar.c"
        break;
      case 19: /* drnode ::= INTEGER HYPHEN INTEGER */
#line 217 "grammar.lemon"
{
	yygotominor.yy79 = drange_node_new();
	drange_node_set_start_offset(yygotominor.yy79, stnode_value(yymsp[-2].minor.yy0));
	drange_node_set_end_offset(yygotominor.yy79, stnode_value(yymsp[0].minor.yy0));

	stnode_free(yymsp[-2].minor.yy0);
	stnode_free(yymsp[0].minor.yy0);
  yy_destructor(yypParser,21,&yymsp[-1].minor);
}
#line 1014 "grammar.c"
        break;
      case 20: /* drnode ::= COLON INTEGER */
#line 229 "grammar.lemon"
{
	yygotominor.yy79 = drange_node_new();
	drange_node_set_start_offset(yygotominor.yy79, 0);
	drange_node_set_length(yygotominor.yy79, stnode_value(yymsp[0].minor.yy0));

	stnode_free(yymsp[0].minor.yy0);
  yy_destructor(yypParser,20,&yymsp[-1].minor);
}
#line 1026 "grammar.c"
        break;
      case 21: /* drnode ::= INTEGER COLON */
#line 239 "grammar.lemon"
{
	yygotominor.yy79 = drange_node_new();
	drange_node_set_start_offset(yygotominor.yy79, stnode_value(yymsp[-1].minor.yy0));
	drange_node_set_to_the_end(yygotominor.yy79);

	stnode_free(yymsp[-1].minor.yy0);
  yy_destructor(yypParser,20,&yymsp[0].minor);
}
#line 1038 "grammar.c"
        break;
      case 22: /* drnode ::= INTEGER */
#line 249 "grammar.lemon"
{
	yygotominor.yy79 = drange_node_new();
	drange_node_set_start_offset(yygotominor.yy79, stnode_value(yymsp[0].minor.yy0));
	drange_node_set_length(yygotominor.yy79, 1);

	stnode_free(yymsp[0].minor.yy0);
}
#line 1049 "grammar.c"
        break;
      case 23: /* relation_test ::= entity rel_op2 entity */
#line 261 "grammar.lemon"
{
	yygotominor.yy0 = stnode_new(STTYPE_TEST, NULL);
	sttype_test_set2(yygotominor.yy0, yymsp[-1].minor.yy26, yymsp[-2].minor.yy0, yymsp[0].minor.yy0);
}
#line 1057 "grammar.c"
        break;
      case 24: /* relation_test ::= entity rel_op2 relation_test */
#line 268 "grammar.lemon"
{
	stnode_t *L, *F;
	/* for now generate it like yymsp[-2].minor.yy0 yymsp[-1].minor.yy26 F  TEST_OP_AND  F P G, later it could be optimized
	   or semantically checked (to make a <= b >= c or a == b != c invalid)?
	 */

	F = yymsp[0].minor.yy0;
	do {
		g_assert(F != NULL && stnode_type_id(F) == STTYPE_TEST);
		sttype_test_get(F, NULL, &F, NULL);
	} while (stnode_type_id(F) == STTYPE_TEST);

	L = stnode_new(STTYPE_TEST, NULL);
	sttype_test_set2(L, yymsp[-1].minor.yy26, yymsp[-2].minor.yy0, stnode_dup(F));

	yygotominor.yy0 = stnode_new(STTYPE_TEST, NULL);
	sttype_test_set2(yygotominor.yy0, TEST_OP_AND, L, yymsp[0].minor.yy0);
}
#line 1079 "grammar.c"
        break;
      case 25: /* rel_op2 ::= TEST_EQ */
#line 287 "grammar.lemon"
{ yygotominor.yy26 = TEST_OP_EQ;   yy_destructor(yypParser,3,&yymsp[0].minor);
}
#line 1085 "grammar.c"
        break;
      case 26: /* rel_op2 ::= TEST_NE */
#line 288 "grammar.lemon"
{ yygotominor.yy26 = TEST_OP_NE;   yy_destructor(yypParser,4,&yymsp[0].minor);
}
#line 1091 "grammar.c"
        break;
      case 27: /* rel_op2 ::= TEST_GT */
#line 289 "grammar.lemon"
{ yygotominor.yy26 = TEST_OP_GT;   yy_destructor(yypParser,7,&yymsp[0].minor);
}
#line 1097 "grammar.c"
        break;
      case 28: /* rel_op2 ::= TEST_GE */
#line 290 "grammar.lemon"
{ yygotominor.yy26 = TEST_OP_GE;   yy_destructor(yypParser,8,&yymsp[0].minor);
}
#line 1103 "grammar.c"
        break;
      case 29: /* rel_op2 ::= TEST_LT */
#line 291 "grammar.lemon"
{ yygotominor.yy26 = TEST_OP_LT;   yy_destructor(yypParser,5,&yymsp[0].minor);
}
#line 1109 "grammar.c"
        break;
      case 30: /* rel_op2 ::= TEST_LE */
#line 292 "grammar.lemon"
{ yygotominor.yy26 = TEST_OP_LE;   yy_destructor(yypParser,6,&yymsp[0].minor);
}
#line 1115 "grammar.c"
        break;
      case 31: /* rel_op2 ::= TEST_BITWISE_AND */
#line 293 "grammar.lemon"
{ yygotominor.yy26 = TEST_OP_BITWISE_AND;   yy_destructor(yypParser,11,&yymsp[0].minor);
}
#line 1121 "grammar.c"
        break;
      case 32: /* rel_op2 ::= TEST_CONTAINS */
#line 294 "grammar.lemon"
{ yygotominor.yy26 = TEST_OP_CONTAINS;   yy_destructor(yypParser,9,&yymsp[0].minor);
}
#line 1127 "grammar.c"
        break;
      case 33: /* rel_op2 ::= TEST_MATCHES */
#line 295 "grammar.lemon"
{ yygotominor.yy26 = TEST_OP_MATCHES;   yy_destructor(yypParser,10,&yymsp[0].minor);
}
#line 1133 "grammar.c"
        break;
      case 34: /* relation_test ::= entity TEST_IN LBRACE setnode_list RBRACE */
#line 298 "grammar.lemon"
{
	stnode_t *S;
	yygotominor.yy0 = stnode_new(STTYPE_TEST, NULL);
	S = stnode_new(STTYPE_SET, yymsp[-1].minor.yy69);
	sttype_test_set2(yygotominor.yy0, TEST_OP_IN, yymsp[-4].minor.yy0, S);
  yy_destructor(yypParser,22,&yymsp[-3].minor);
  yy_destructor(yypParser,23,&yymsp[-2].minor);
  yy_destructor(yypParser,24,&yymsp[0].minor);
}
#line 1146 "grammar.c"
        break;
      case 35: /* setnode_list ::= entity */
      case 39: /* funcparams ::= entity */ yytestcase(yyruleno==39);
#line 306 "grammar.lemon"
{
	yygotominor.yy69 = g_slist_append(NULL, yymsp[0].minor.yy0);
}
#line 1154 "grammar.c"
        break;
      case 36: /* setnode_list ::= setnode_list entity */
#line 311 "grammar.lemon"
{
	yygotominor.yy69 = g_slist_append(yymsp[-1].minor.yy69, yymsp[0].minor.yy0);
}
#line 1161 "grammar.c"
        break;
      case 37: /* entity ::= FUNCTION LPAREN funcparams RPAREN */
#line 319 "grammar.lemon"
{
	yygotominor.yy0 = yymsp[-3].minor.yy0;
	sttype_function_set_params(yygotominor.yy0, yymsp[-1].minor.yy69);
  yy_destructor(yypParser,26,&yymsp[-2].minor);
  yy_destructor(yypParser,27,&yymsp[0].minor);
}
#line 1171 "grammar.c"
        break;
      case 38: /* entity ::= FUNCTION LPAREN RPAREN */
#line 326 "grammar.lemon"
{
	yygotominor.yy0 = yymsp[-2].minor.yy0;
  yy_destructor(yypParser,26,&yymsp[-1].minor);
  yy_destructor(yypParser,27,&yymsp[0].minor);
}
#line 1180 "grammar.c"
        break;
      case 40: /* funcparams ::= funcparams COMMA entity */
#line 336 "grammar.lemon"
{
	yygotominor.yy69 = g_slist_append(yymsp[-2].minor.yy69, yymsp[0].minor.yy0);
  yy_destructor(yypParser,18,&yymsp[-1].minor);
}
#line 1188 "grammar.c"
        break;
      case 41: /* expr ::= LPAREN expr RPAREN */
#line 343 "grammar.lemon"
{
	yygotominor.yy0 = yymsp[-1].minor.yy0;
	stnode_set_bracket(yygotominor.yy0, TRUE);
  yy_destructor(yypParser,26,&yymsp[-2].minor);
  yy_destructor(yypParser,27,&yymsp[0].minor);
}
#line 1198 "grammar.c"
        break;
      default:
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
  DfilterARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
/************ Begin %parse_failure code ***************************************/
#line 126 "./grammar.lemon"

	dfw->syntax_error = TRUE;
#line 1251 "grammar.c"
/************ End %parse_failure code *****************************************/
  DfilterARG_STORE; /* Suppress warning about unused %extra_argument variable */
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
  DfilterARG_FETCH;
#define TOKEN (yyminor.yy0)
/************ Begin %syntax_error code ****************************************/
#line 70 "./grammar.lemon"


	header_field_info	*hfinfo;

	if (!TOKEN) {
		dfilter_fail(dfw, "Unexpected end of filter string.");
		dfw->syntax_error = TRUE;
		return;
	}

	switch(stnode_type_id(TOKEN)) {
		case STTYPE_UNINITIALIZED:
			dfilter_fail(dfw, "Syntax error.");
			break;
		case STTYPE_TEST:
			dfilter_fail(dfw, "Syntax error, TEST.");
			break;
		case STTYPE_STRING:
			dfilter_fail(dfw, "The string \"%s\" was unexpected in this context.",
				(char *)stnode_data(TOKEN));
			break;
		case STTYPE_UNPARSED:
			dfilter_fail(dfw, "\"%s\" was unexpected in this context.",
				(char *)stnode_data(TOKEN));
			break;
		case STTYPE_INTEGER:
			dfilter_fail(dfw, "The integer %d was unexpected in this context.",
				stnode_value(TOKEN));
			break;
		case STTYPE_FIELD:
			hfinfo = (header_field_info *)stnode_data(TOKEN);
			dfilter_fail(dfw, "Syntax error near \"%s\".", hfinfo->abbrev);
			break;
		case STTYPE_FUNCTION:
			dfilter_fail(dfw, "The function s was unexpected in this context.");
			break;
		case STTYPE_SET:
			dfilter_fail(dfw, "Syntax error, SET.");
			break;

		/* These aren't handed to use as terminal tokens from
		   the scanner, so was can assert that we'll never
		   see them here. */
		case STTYPE_NUM_TYPES:
		case STTYPE_RANGE:
		case STTYPE_FVALUE:
			g_assert_not_reached();
			break;
	}
	dfw->syntax_error = TRUE;
#line 1319 "grammar.c"
/************ End %syntax_error code ******************************************/
  DfilterARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  DfilterARG_FETCH;
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
  DfilterARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "DfilterAlloc" which describes the current state of the parser.
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
void Dfilter(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  DfilterTOKENTYPE yyminor       /* The value for the token */
  DfilterARG_PDECL               /* Optional %extra_argument parameter */
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
  DfilterARG_STORE;

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
