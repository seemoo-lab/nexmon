%{
#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/route/pktloc.h>
%}

%locations
%error-verbose
%define api.pure
%name-prefix "pktloc_"

%parse-param {void *scanner}
%lex-param {void *scanner}

%union {
	struct rtnl_pktloc *l;
	uint32_t i;
	char *s;
}

%{
extern int pktloc_lex(YYSTYPE *, YYLTYPE *, void *);
extern void rtnl_pktloc_add(struct rtnl_pktloc *);

static void yyerror(YYLTYPE *locp, void *scanner, const char *msg)
{
	/* FIXME */
}
%}

%token <i> ERROR NUMBER LAYER
%token <s> NAME

%type <i> mask layer
%type <l> location

%destructor { free($$); } NAME

%start input

%%

input:
	def
		{ }
	;

def:
	/* empty */
		{ }
	| location def
		{ }
	;

location:
	NAME NAME layer NUMBER mask
		{
			struct rtnl_pktloc *loc;

			if (!(loc = calloc(1, sizeof(*loc)))) {
				/* FIXME */
			}

			if (!strcasecmp($2, "u8"))
				loc->align = TCF_EM_ALIGN_U8;
			else if (!strcasecmp($2, "h8")) {
				loc->align = TCF_EM_ALIGN_U8;
				loc->flags = TCF_EM_CMP_TRANS;
			} else if (!strcasecmp($2, "u16"))
				loc->align = TCF_EM_ALIGN_U16;
			else if (!strcasecmp($2, "h16")) {
				loc->align = TCF_EM_ALIGN_U16;
				loc->flags = TCF_EM_CMP_TRANS;
			} else if (!strcasecmp($2, "u32"))
				loc->align = TCF_EM_ALIGN_U32;
			else if (!strcasecmp($2, "h32")) {
				loc->align = TCF_EM_ALIGN_U32;
				loc->flags = TCF_EM_CMP_TRANS;
			}
			
			free($2);

			loc->name = $1;
			loc->layer = $3;
			loc->offset = $4;
			loc->mask = $5;

			rtnl_pktloc_add(loc);

			$$ = loc;
		}
	;

layer:
	/* empty */
		{ $$ = TCF_LAYER_NETWORK; }
	| LAYER '+' 
		{ $$ = $1; }
	;

mask:
	/* empty */
		{ $$ = 0; }
	| NUMBER
		{ $$ = $1; }
	;
