/*
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
 * 	![abcd] One of not (any one of not a,b,c or d)
 *
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
 *
 * extended: longest match for * used, *[], *!, *![], !<whole pattern> possible
 *
 * you can overdo everything ...
 */

#include "global.h"
//#include "debug.h"
#include "matchpat.h"

#define CASESENS 1

#define PATSEP	'|'

/*
 * search for any character of p in s
 *
 * return pointer in s or 0
 */
char *strmchr( char *s, char *p )
{
	char *p1;
	for( ; *s; s++ )
	{
		for( p1 = p; *p1 && *p1 != *s; p1++ );
		if( *p1 )
			return s;	/* found */
	}
	return 0;
}

/*
 * search for any character of p in s from end of s
 *
 * return pointer in s or 0
 */
char *strmrchr( char *s, char *p )
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
#define NO_EXTENDED_PATMATCH	0
#define MAXPATTERNS	8

/*
 * init: t=0
 * de-init t=0,pat=0
 *
 * if auto_wc: append * to pat
 *
 * return true if match else false
 *
 */

int
match_pattern(char *t, char *pat, bool auto_wc)
{
	bool valid = true;
	int len = 0;
	char *p, *tstart;
	int i;
	char *pp, *pp1, *s;
	static short last_magic[MAXPATTERNS] = {-2};
	bool not = false;	/* invert an item: ...![abc]... */
	bool Not;	/* invert whole pattern: !... */

	if (auto_wc)
	{
		if( pat[0] != '*' && !strcmp(t, pat) )
			return 2;
		len = strlen(pat);
		if (pat[len - 1] != '*' && !strchr( pat, PATSEP ) )
		{
			pat[len] = '*';
			pat[len + 1] = '\0';
			last_magic[0] = len;
		}
		else
			auto_wc = false;
	}
	if( auto_wc == false && (!t || last_magic[0] == -2) )
	{
		if( pat )	/* init */
		{
			for( pp1 = pp = pat, i = 0; i < MAXPATTERNS; pp1 = pp+1, i++ )
			{
				if( (pp = strchr( pp1, PATSEP)) )
					*pp = 0;
				s = strmrchr( pp1, "*[!" );
				if( s )
					last_magic[i] = s - pp1;
				else
					last_magic[i] = -1;
				if( pp )
					*pp = PATSEP;
				else
					return false;
			}
		}
		else	/* de-init */
			last_magic[0] = -2;
		return false;
	}

	for( tstart = t, pp1 = pp = pat, i = 0; i < MAXPATTERNS; pp1 = pp+1, i++ )
	{
		pp = strchr( pp1, PATSEP );
		if( pp )
			*pp = 0;

		p = pat = pp1;
		t = tstart;
		valid = true;
		if( *pat == '!' )
		{
			Not = true;
			pat++;
		}
		else
			Not = false;

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
					while( *++pat == '*' );

					if( *pat )
					{
						if( (pat-p-1) == last_magic[i] || (*pat == '!' && (pat-p) == last_magic[i]) )
						{
							char *tp = t + strlen(t) - strlen(pat);
							if( *pat == '!' )
								tp++;
							if( tp >= t )	/* '*x' matches 'x' */
								t = tp;
							else
								valid = false;
						}
						else	/* pat has more magic: search from beg */
						{
#if !NO_EXTENDED_PATMATCH
							char *tp = strmchr( pat, "?[!*");
							if( tp != pat )	/* next is not magic */
							{
								char c = 0;
								if( tp )
								{
									c = *tp;
									*tp = 0;
								}
#endif
								if( !(t = strstr( t, pat )) )
									valid = false;
								else							/* catch last match */
								{
									char *t2;
									for( t2 = t; (t2 = strstr(t2+1, pat)); t = t2 );
								}
#if !NO_EXTENDED_PATMATCH
								if( tp )
									*tp = c;
							}
							/* difficult part: *[], *!, *![] */
							else
							{
								if( *pat == '!' )
								{
									pat++;
									if( *pat != '[' )
									{
										if( !*(pat+1) )
										{
											char *t2 = t + strlen(t) - 1;
											if( *t2 == *pat )
												valid = false;
											else
												t = t2+1;
										}
										else
										{
											for( ; *t && *t == *pat; t++ );
											if( *t )
												t++;
										}
										pat++;
									}
									else
										not = true;
								}
								if( *pat == '[' )
								{
									tp = strchr( ++pat, ']' );
									if( tp )
									{
										*tp = 0;
										if( *pat == '!' )
										{
											not ^= 1;
											pat++;
										}
										if( tp - p > last_magic[i] )	/* only ? and natural left in pat */
										{
											char *t2 = t + strlen(t) - strlen(tp+1) - 1;
											if( t2 < t || not == !!strchr( pat, *t2 ) )
												valid = false;
											else
											{
												t = t2 + 1;
											}
											pat = tp + 1;
											*tp = ']';
											break;
										}
										/* scan up to last match */
										s = strmrchr( t, pat );
										if( not )
										{
											if( s != NULL )
											{
												if( !*(tp+1) )
												{
													if( !*(s+1) )
														valid = false;
													else
														t += strlen(t);
												}
												else
												{
													for( ; *t && strchr( pat, *t ); t++ );
													t++;
												}
											}
											else	/* no match */
											{
												if( *(tp+1) )
													t++;	/* continue matching */
												else
													t += strlen(t);	/* done: match */
											}
										}
										else	/* !not */
										{
											if( s == NULL )
												valid = false;
											else
											{
												t = s+1;
											}
										}
										pat = tp + 1;
										*tp = ']';	/* restore pattern */
									}
									else
										valid = false;	/* invalid pattern: no matching ] */
								}
								not = false;
							}
#endif	//!NO_EXTENDED_PATMATCH
						}
					}
					else
						t += strlen(t);
					break;
				}
				/* !X means any character but X */
				case '!':
				{
					//if (pat[1] && toupper(*t) != toupper(pat[1]))
					if( pat[1] == '[' )
					{
						pat++;
						not = true;
						break;
					}
					if (pat[1] && *t != pat[1])
					{
						t++;
						pat += 2;
					}
					else
						valid = false;
					break;
				}
				/* [<chars>] means any one of <chars> */
				case '[':
				{
					if( *(pat+1) == '!' )
					{
						not ^= 1;
						pat++;
					}
					while (*++pat && (*(pat) != ']') && (*t != *pat))
						;

					if (!*pat || *pat == ']')
						valid = false;
					else
					{
						while (*++pat && *pat != ']')
							;
						if (!*pat)
							valid = false;
					}
					if( *pat )
						pat++;
					t++;
					if( not )
					{
						valid = !valid;
						not = false;
					}
					break;
				}
				default:
				{
					if (*t++ != *pat++)
						valid = false;
					break;
				}
			}
		}
		valid = (valid && (*t == *pat));

		if (auto_wc)
		{
			p[len] = '\0';
			return valid != Not;
		}

		if( pp )
			*pp = PATSEP;
		else
			break;

		if( valid != Not )
			return true;

	}	/*/for( tstart = t, pp1 = pp = pat, i = 0; i < MAXPATTERNS; pp1 = pp+1, i++ )*/

	return valid != Not;
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
