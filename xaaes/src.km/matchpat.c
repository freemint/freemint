/*
 * $Id$
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for FreeMiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* HR: isolated completely from the XaAES & dirs environment.
 * 
 * Pattern matching
 * - If you want better filtering of files, put the code here...
 * 
 * Valid patterns are:
 * 	?      Any single char
 * 	*      A string of any char
 * 	!X     Any char except for X
 * 	[abcd] One of (any one of a,b,c or d)
 * Examples:
 * 	*      All files in dir
 * 	a*     All files begining with 'a'
 * 	*.c*   All files with a extension starting with c
 * 	*.o    All '.o' files
 * 	*.!o   All files not ending in '.o' 
 * 	!z*.?  All files not starting with 'z', and having a single character extension
 * 	*.[co] All '.o' and '.c' files
 * 	*.[ch]* All files with a extension starting with c or h
 *
 * Ozk: Added a flag 'auto_wc' which, when true, automatically appends a
 *	wildcard to the pattern. This allows us to use match_pattern for
 *	filenamecompletion.
 *
 * HK: comparing now case-sensitive.
 */

#include "global.h"
#include "matchpat.h"

#define CASESENS 1

static char *strmrchr( char *s, char *p )
{
	char *p1, *sx = s + strlen(s) - 1;
	for( ; sx >= s; sx-- )
	{
		for( p1 = p; *p1 && *p1 != *sx; p1++ );
		if( *p1 )
			return sx;	/* found */
	}
	return 0;
}

#if CASESENS
int
match_pattern(char *t, char *pat, bool auto_wc)
{
	int valid = 1;
	int len = 0;
	char *p = pat;
	char *last_magic = strmrchr( pat, "*[]!" );

	//if (!stricmp(t, pat))
	if( pat[0] != '*' && !strcmp(t, pat) )
		return 2;

	if (auto_wc)
	{
		len = strlen(p);
		if (p[len - 1] != '*')
		{
			p[len] = '*';
			p[len + 1] = '\0';
		}
		else
			auto_wc = false;
	}

	while (valid && ((*t && *pat) || (!*t && *pat == '*')))
	{
		switch (*pat)
		{
			/* Any character */
			case '?':
			{
				t++;
				pat++;
				break;
			}
			/* String of any character */
			case '*':
			{
				while( *++pat == '*' || *pat == '?' );

				if( *pat )
				{
					if( (pat-1) == last_magic )
					{
						char *tp = t + strlen(t) - strlen(pat);
						if( tp >= t )	/* '*x' matches 'x' */
							t = tp;
						else
							valid = false;
					}
					else	/* pat has more magic: search from beg */
						if( !strchr( "[!", *pat ) )	/* next is not magic */
							if( !(t = strchr( t, *pat )) )
								valid = false;
				}
				else
					t += strlen(t);
				break;
			}
			/* !X means any character but X */
			case '!':
			{
				//if (pat[1] && toupper(*t) != toupper(pat[1]))
				if (pat[1] && *t != pat[1])
				{
					t++;
					pat += 2;
				}
				else
					valid = 0;
				break;
			}
			/* [<chars>] means any one of <chars> */
			case '[':
			{
				//while (*++pat && (*(pat) != ']') && (toupper(*t) != toupper(*pat)))
				while (*++pat && (*(pat) != ']') && (*t != *pat))
					;

				if (!*pat || *pat == ']')
					valid = 0;
				else
				{
					while (*++pat && *pat != ']')
						;
					if (!*pat)
						valid = 0;
				}
				if( *pat )
					pat++;
				t++;
				break;
			}
			default:
			{
				//if (toupper(*t++) != toupper(*pat++))
				if (*t++ != *pat++)
					valid = 0;
				break;
			}
		}
	}
	//valid = (valid && (toupper(*t) == toupper(*pat)));
	valid = (valid && (*t == *pat));
	
	if (auto_wc)
		p[len] = '\0';
	
	return valid;
}
#else

int
match_pattern(char *t, char *pat, bool auto_wc)
{
	int valid = 1;
	int len = 0;
	char *p = pat;

	if (!stricmp(t, pat))
		return 2;

	if (auto_wc)
	{
		len = strlen(p);
		if (p[len - 1] != '*')
		{
			p[len] = '*';
			p[len + 1] = '\0';
		}
		else
			auto_wc = false;
	}

	while (valid && ((*t && *pat) || (!*t && *pat == '*')))
	{
		switch (*pat)
		{
			/* Any character */
			case '?':
			{
				t++;
				pat++;
				break;
			}
			/* String of any character */
			case '*':
			{
				char *s = t;
				pat++;
				/* match from end */
				t = t + strlen(t);
				for( t--; t >= s && toupper(*t) != toupper(*pat); t-- );
				break;
			}
			/* !X means any character but X */
			case '!':
			{
				if (pat[1] && toupper(*t) != toupper(pat[1]))
				{
					t++;
					pat += 2;
				}
				else
					valid = 0;
				break;
			}
			/* [<chars>] means any one of <chars> */
			case '[':
			{
				while (*++pat && (*(pat) != ']') && (toupper(*t) != toupper(*pat)))
					;

				if (!*pat || *pat == ']')
					valid = 0;
				else
				{
					while (*++pat && *pat != ']')
						;
					if (!*pat)
						valid = 0;
				}
				pat++;
				t++;
				break;
			}
			default:
			{
				if (toupper(*t++) != toupper(*pat++))
					valid = 0;
				break;
			}
		}
	}
	valid = (valid && (toupper(*t) == toupper(*pat)));
	
	if (auto_wc)
		p[len] = '\0';
	
	return valid;
}
#endif
