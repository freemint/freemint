/* This file belongs to FreeMiNT
 * it has been stolen from the MiNT Library
 * and modified as it seemed necessary.
 */
/* from Henry Spencer's stringlib */
/* Bugfix by Guido: Handle empty strings correctly.  */

#include "libkern.h"

/*
 * strstr - find first occurrence of wanted in s
 */

char * _cdecl				/* found string, or NULL if none */
_mint_strstr(const char *s, const char *wanted)
{
	register const char *scan;
	register long len;
	register char firstc;

	if (!*s) {
		if (*wanted)
			return 0;
		else
			return s;
	} else if (!*wanted) {
		return s;
	}
	
	/*
	 * The odd placement of the two tests is so "" is findable.
	 * Also, we inline the first char for speed.
	 * The ++ on scan has been moved down for optimization.
	 */
	firstc = *wanted;
	len = strlen(wanted);
	for (scan = s; *scan != firstc || strncmp(scan, wanted, len) != 0; )
		if (*scan++ == '\0')
			return(0);
	return((char *)scan);
}
