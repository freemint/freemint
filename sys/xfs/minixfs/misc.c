/*
 * This file is part of 'minixfs' Copyright 1991,1992,1993 S.N.Henson
 * 
 * Modified for FreeMiNT CVS
 * by Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

# include "misc.h"

/* inode_busy() scans the list for a given inode, this is used to stop
 * unlink blowing away a busy inode. In addition f->aux flag is set if 'flag'
 * is non zero. This is to allow a subsequent close to recognise that the inode
 * is marked for zapping.
 */

int
inode_busy (unsigned int inum, int drive, int flag)
/* flag - Propagate AUX_DEL */
{
	FILEPTR *f;
	int found = 0;

	for (f = firstptr; f; f = f->next)
	{
		if ((f->fc.index == inum) && (f->fc.dev == drive))
		{
			if (flag) f->fc.aux |= AUX_DEL;
			if (found != 2) found++;
		}
		else if (found)
			break; /* Since same files consecutive, halt search */
	}
	return found;
}


/* Copy characters from 'from' to 'to' translating backslashes to slashes */

void
btos_cpy (char *to, const char *from)
{
	char c;
	do {
		c = *from++;	
		if (c == '\\') *to++ = '/';
		else *to++ = c;
	}
	while (c);
}

/* Translate slashes to backslashes, return zero if all characters copied */

int
stob_ncpy (char *to, const char *from, long int n)
{
	char c;

	if(n == 0) return 0;

	do
	{
		c = *from++;
		if (c == '/') *to++ = '\\';
		else *to++ = c;
	}
	while (--n && c);

	if (c)
	{
		*--to = '\0';
		return 1;
	}
	
	return 0;
}
