%{
 #include <netlink-local.h>
 #include <netlink-tc.h>
 #include <netlink/netlink.h>
 #include <netlink/utils.h>
 #include <netlink/route/pktloc.h>
 #include "pktloc_syntax.h"
%}

%option 8bit
%option reentrant
%option warn
%option noyywrap
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

[lL][iI][nN][kK]	{ yylval->i = TCF_LAYER_LINK; return LAYER; }
[nN][eE][tT]		{ yylval->i = TCF_LAYER_NETWORK; return LAYER; }
[tT][cC][pP]		{ yylval->i = TCF_LAYER_TRANSPORT; return LAYER; }

[^ \t\r\n+]+		{
				yylval->s = strdup(yytext);
				if (yylval->s == NULL)
					return ERROR;
				return NAME;
			}
