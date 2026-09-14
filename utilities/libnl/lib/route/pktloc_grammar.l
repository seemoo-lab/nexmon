%top{
 #include "nl-default.h"
}
%{
 #include <linux/tc_ematch/tc_em_cmp.h>

 #include <netlink/netlink.h>
 #include <netlink/utils.h>
 #include <netlink/route/pktloc.h>

 #include "pktloc_syntax.h"

 int pktloc_get_column(yyscan_t);
 void pktloc_set_column(int, yyscan_t);
%}

%option 8bit
%option reentrant
%option warn
%option noyywrap
%option noinput
%option nounput
%option bison-bridge
%option bison-locations
%option prefix="pktloc_"

%%

[ \t\r\n]+

"#".*

[[:digit:]]+		|
0[xX][[:xdigit:]]+	{
				yylval->i = strtoul(yytext, NULL, 0);
				return NUMBER;
			}

"+"			{ return yylval->i = yytext[0]; }

[uU]8			{ yylval->i = TCF_EM_ALIGN_U8; return ALIGN; }
[uU]16			{ yylval->i = TCF_EM_ALIGN_U16; return ALIGN; }
[uU]32			{ yylval->i = TCF_EM_ALIGN_U32; return ALIGN; }

[lL][iI][nN][kK]	|
[eE][tT][hH]		{ yylval->i = TCF_LAYER_LINK; return LAYER; }
[nN][eE][tT]		|
[iI][pP]		{ yylval->i = TCF_LAYER_NETWORK; return LAYER; }
[tT][rR][aA][nN][sS][pP][oO][rR][tT] |
[tT][cC][pP]		{ yylval->i = TCF_LAYER_TRANSPORT; return LAYER; }


[^ \t\r\n+]+		{
				yylval->s = strdup(yytext);
				if (yylval->s == NULL)
					return ERROR;
				return NAME;
			}
