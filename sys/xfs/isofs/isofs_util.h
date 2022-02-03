/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

# ifndef _isofs_util_h
# define _isofs_util_h

int isochar(const uchar *isofn, const uchar *isoend, int joliet_level, uchar *c);

int isofncmp(const uchar *fn, int fnlen, const uchar *isofn, int isolen, int joliet_level);

void isofntrans(uchar *infn, int infnlen, uchar *outfn, ushort *outfnlen,
		int original, int casetrans, int assoc, int joliet_level);

# endif /* _isofs_util_h */
