/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
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
 * 
 * begin:	2000-01-01
 * last change:	2000-03-07
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * changes since last version:
 * 
 * known bugs:
 * 
 * todo:
 * 
 * optimizations:
 * 
 */

%{

# include <string.h>
# include "list.h"
# include "pars.h"

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



"DOS"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _DOS;
}

"BIOS"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _BIOS;
}

"XBIOS"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _XBIOS;
}

"NULL"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _NULL;
}

"RESERVED"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _RESERVED;
}

"MAX"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _MAX;
}

"void"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _VOID;
}

"const"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _CONST;
}

"struct"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _STRUCT;
}

"union"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _UNION;
}

"char"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _CHAR;
}

"short"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _SHORT;
}

"long"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _LONG;
}

"unsigned"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _UNSIGNED;
}

"uchar"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _UCHAR;
}

"ushort"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _USHORT;
}

"ulong"	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
	return _ULONG;
}

{nonzerodigit}{digit}*	{
	yylval.value = strtol (yytext, NULL, 0);
	return Integer;
}

"0"("x"|"X"){hexdigit}+	{
	yylval.value = strtol (yytext, NULL, 0);
	return Integer;
}

{ident}	{
	strncpy (yylval.ident, yytext, STRMAX);
	yylval.ident [STRMAX] = '\0';
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

