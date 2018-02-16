/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000-2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

%{

#include <string.h>
#include "syscalldefs.h"
#include "pars.h"

int yylex (void);
int yylinecount = 1;

%}

digit		[0-9]
nonzerodigit	[1-9]
hexdigit	[0-9abcdefABCDEF]

letter		[a-zA-Z_]
ident		{letter}({letter}|{digit})*

space		[ \t\n]


%x comment cppcomment

%%

"/*"			BEGIN (comment);
<comment>[^*\n]*	/* eat anything that's not a '*' */
<comment>"*"+[^*/\n]*	/* eat up '*'s not followed by '/'s */
<comment>\n		yylinecount++;
<comment>"*"+"/"	BEGIN (INITIAL);

"//"			BEGIN (cppcomment);
<cppcomment>\n		yylinecount++; BEGIN (INITIAL);
<cppcomment>.		



"GEMDOS"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_GEMDOS;
}

"BIOS"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_BIOS;
}

"XBIOS"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_XBIOS;
}

"MAX"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_MAX;
}

"void"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_VOID;
}

"const"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_CONST;
}

"struct"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_STRUCT;
}

"union"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_UNION;
}

"char"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_CHAR;
}

"short"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_SHORT;
}

"long"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_LONG;
}

"unsigned"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_UNSIGNED;
}

"uchar"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_UCHAR;
}

"ushort"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_USHORT;
}

"ulong"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_ULONG;
}

"undefined"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_UNDEFINED;
}

"unsupported"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_UNSUPPORTED;
}

"unimplemented"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_UNIMPLEMENTED;
}

"passthrough"	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return _IDENT_PASSTHROUGH;
}

{nonzerodigit}{digit}*	{
	yylval.value = strtol(yytext, NULL, 0);
	return Integer;
}

"0"("x"|"X"){hexdigit}+	{
	yylval.value = strtol(yytext, NULL, 0);
	return Integer;
}

{ident}	{
	strncpy(yylval.ident, yytext, STRMAX);
	yylval.ident[STRMAX] = '\0';
	return Identifier;
}

{space}	{
	if (*yytext == '\n')
		yylinecount++;
}

.	{
	return *yytext;
}

%%

