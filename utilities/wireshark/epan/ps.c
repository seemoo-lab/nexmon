/* DO NOT EDIT
 *
 * Created by rdps.py.
 *
 * ps.c
 * Definitions for generating PostScript(R) packet output.
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

#include <stdio.h>

#include "ps.h"

void print_ps_preamble(FILE *fd) {
	fprintf(fd, "%%!\n");
	fprintf(fd, "%%!PS-Adobe-2.0\n");
	fprintf(fd, "%%\n");
	fprintf(fd, "%% Wireshark - Network traffic analyzer\n");
	fprintf(fd, "%% By Gerald Combs <gerald@wireshark.org>\n");
	fprintf(fd, "%% Copyright 1998 Gerald Combs\n");
	fprintf(fd, "%%\n");
	fprintf(fd, "%%%%Creator: Wireshark\n");
	fprintf(fd, "%%%%Title: Wireshark output\n");
	fprintf(fd, "%%%%DocumentFonts: Helvetica Monaco\n");
	fprintf(fd, "%%%%EndComments\n");
	fprintf(fd, "%%!\n");
	fprintf(fd, "\n");
	fprintf(fd, "%%\n");
	fprintf(fd, "%% Ghostscript http://ghostscript.com/ can convert postscript to pdf files.\n");
	fprintf(fd, "%%\n");
	fprintf(fd, "%% To convert this postscript file to pdf, type (for US letter format):\n");
	fprintf(fd, "%% ps2pdf filename.ps\n");
	fprintf(fd, "%%\n");
	fprintf(fd, "%% or (for A4 format):\n");
	fprintf(fd, "%% ps2pdf -sPAPERSIZE=a4 filename.ps\n");
	fprintf(fd, "%%\n");
	fprintf(fd, "%% ... and of course replace filename.ps by your current filename.\n");
	fprintf(fd, "%%\n");
	fprintf(fd, "%% The pdfmark's below will help converting to a pdf file, and have no\n");
	fprintf(fd, "%% effect when printing the postscript directly.\n");
	fprintf(fd, "%% \n");
	fprintf(fd, "\n");
	fprintf(fd, "%%   This line is necessary if the file should be printable, and not just used\n");
	fprintf(fd, "%%   for distilling into PDF:\n");
	fprintf(fd, "%%\n");
	fprintf(fd, "/pdfmark where {pop} {userdict /pdfmark /cleartomark load put} ifelse\n");
	fprintf(fd, "%%\n");
	fprintf(fd, "%%   This tells PDF viewers to display bookmarks when the document is opened:\n");
	fprintf(fd, "%%\n");
	fprintf(fd, "[/PageMode /UseOutlines /DOCVIEW pdfmark\n");
	fprintf(fd, "\n");
	fprintf(fd, "%% Get the Imagable Area of the page\n");
	fprintf(fd, "clippath pathbbox\n");
	fprintf(fd, "\n");
	fprintf(fd, "%% Set vmax to the vertical size of the page,\n");
	fprintf(fd, "%% hmax to the horizontal size of the page.\n");
	fprintf(fd, "/vmax exch def\n");
	fprintf(fd, "/hmax exch def\n");
	fprintf(fd, "pop pop		%% junk\n");
	fprintf(fd, "\n");
	fprintf(fd, "%% 1/2-inch margins\n");
	fprintf(fd, "/lmargin 36 def					%% left margin\n");
	fprintf(fd, "/tmargin vmax 56 sub def		%% top margin\n");
	fprintf(fd, "/bmargin 36 def					%% bottom margin\n");
	fprintf(fd, "/pagenumtab hmax 36 sub def		%% right margin\n");
	fprintf(fd, "\n");
	fprintf(fd, "%% Counters\n");
	fprintf(fd, "/thispagenum 1 def\n");
	fprintf(fd, "\n");
	fprintf(fd, "%% Strings\n");
	fprintf(fd, "/pagenostr 7 string def\n");
	fprintf(fd, "\n");
	fprintf(fd, "\n");
	fprintf(fd, "/formfeed {\n");
	fprintf(fd, "	printpagedecorations\n");
	fprintf(fd, "	showpage\n");
	fprintf(fd, "	\n");
	fprintf(fd, "	%% we need a new current point after showpage is done\n");
	fprintf(fd, "	lmargin		%% X\n");
	fprintf(fd, "	vpos 		%% Y\n");
	fprintf(fd, "	moveto\n");
	fprintf(fd, "	/vpos tmargin def\n");
	fprintf(fd, "} def\n");
	fprintf(fd, "\n");
	fprintf(fd, "%% Prints text with possible indenting\n");
	fprintf(fd, "/putline_single {\n");
	fprintf(fd, "	exch 10 mul lmargin add		%% X\n");
	fprintf(fd, "	vpos 						%% Y\n");
	fprintf(fd, "	moveto\n");
	fprintf(fd, "	show\n");
	fprintf(fd, "\n");
	fprintf(fd, "	/vpos vpos 10 sub def\n");
	fprintf(fd, "\n");
	fprintf(fd, "	vpos 5 sub bmargin le 		%% is vpos <= bottom margin?\n");
	fprintf(fd, "	{\n");
	fprintf(fd, "		formfeed\n");
	fprintf(fd, "	}\n");
	fprintf(fd, "	if							%% then formfeed and start at top\n");
	fprintf(fd, "} def\n");
	fprintf(fd, "\n");
	fprintf(fd, "\n");
	fprintf(fd, "%% Prints text with possible indenting and line wrap\n");
	fprintf(fd, "/putline {\n");
	fprintf(fd, "	/text exch def\n");
	fprintf(fd, "	/indent exch def\n");
	fprintf(fd, "	\n");
	fprintf(fd, "	%% wrapat = width / sizeof font (remember: monospaced font)\n");
	fprintf(fd, "	/pagewidth pagenumtab lmargin sub def\n");
	fprintf(fd, "	/cwidth (A) stringwidth pop def\n");
	fprintf(fd, "	/wrapat pagewidth cwidth div cvi def\n");
	fprintf(fd, "		\n");
	fprintf(fd, "	text length wrapat le {\n");
	fprintf(fd, "		%% print line\n");
	fprintf(fd, "		indent text 0 text length getinterval putline_single\n");
	fprintf(fd, "	}{\n");
	fprintf(fd, "		%% print the lines first part\n");
	fprintf(fd, "		indent text 0 wrapat getinterval putline_single\n");
	fprintf(fd, "		%% print wrapped rest\n");
	fprintf(fd, "		indent text wrapat text length wrapat sub getinterval putline\n");
	fprintf(fd, "	}\n");
	fprintf(fd, "	ifelse\n");
	fprintf(fd, "} def\n");
	fprintf(fd, "\n");
	fprintf(fd, "\n");
	fprintf(fd, "%% Prints the page number at the top right\n");
	fprintf(fd, "/printpagedecorations {\n");
	fprintf(fd, "	gsave\n");
	fprintf(fd, "		%% Set the font to 8 point\n");
	fprintf(fd, "		/Helvetica findfont 8 scalefont setfont\n");
	fprintf(fd, "\n");
	fprintf(fd, "		%% title\n");
	fprintf(fd, "		lmargin						%% X\n");
	fprintf(fd, "		vmax 36 sub					%% Y\n");
	fprintf(fd, "		moveto\n");
	fprintf(fd, "		ws_pagetitle show\n");
	fprintf(fd, "\n");
	fprintf(fd, "		%% this page number\n");
	fprintf(fd, "		pagenumtab (Page ) stringwidth pop sub thispagenum pagenostr cvs stringwidth pop sub 		%% X\n");
	fprintf(fd, "		vmax 36 sub					%% Y\n");
	fprintf(fd, "		moveto\n");
	fprintf(fd, "		(Page ) show\n");
	fprintf(fd, "		thispagenum pagenostr cvs show\n");
	fprintf(fd, "\n");
	fprintf(fd, "		%% thispagenum++\n");
	fprintf(fd, "		/thispagenum thispagenum 1 add def\n");
	fprintf(fd, "		\n");
	fprintf(fd, "		%% line at top of page\n");
	fprintf(fd, "		lmargin						%% X\n");
	fprintf(fd, "		vmax 38 sub					%% Y\n");
	fprintf(fd, "		moveto\n");
	fprintf(fd, "		\n");
	fprintf(fd, "		pagenumtab					%% X\n");
	fprintf(fd, "		vmax 38 sub					%% Y\n");
	fprintf(fd, "		lineto\n");
	fprintf(fd, "		stroke\n");
	fprintf(fd, "		\n");
	fprintf(fd, "		%% line at bottom of page\n");
	fprintf(fd, "		lmargin						%% X\n");
	fprintf(fd, "		bmargin						%% Y\n");
	fprintf(fd, "		moveto\n");
	fprintf(fd, "		\n");
	fprintf(fd, "		pagenumtab					%% X\n");
	fprintf(fd, "		bmargin						%% Y\n");
	fprintf(fd, "		lineto\n");
	fprintf(fd, "		stroke\n");
	fprintf(fd, "		\n");
	fprintf(fd, "	grestore\n");
	fprintf(fd, "} def\n");
	fprintf(fd, "	\n");
	fprintf(fd, "%% Reset the vertical position\n");
	fprintf(fd, "/vpos tmargin def\n");
	fprintf(fd, "\n");
	fprintf(fd, "%% Set the font to 8 point\n");
	fprintf(fd, "/Monaco findfont 8 scalefont setfont\n");
	fprintf(fd, "\n");
}


void print_ps_finale(FILE *fd) {
	fprintf(fd, "\n");
	fprintf(fd, "printpagedecorations\n");
	fprintf(fd, "showpage\n");
	fprintf(fd, "\n");
	fprintf(fd, "%%%%EOF\n");
	fprintf(fd, "\n");
}


