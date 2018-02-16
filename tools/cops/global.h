/*
 * COPS (c) 1995 - 2003 Sven & Wilfried Behne
 *                 2004 F.Naumann & O.Skancke
 *
 * A XCONTROL compatible CPX manager.
 *
 * This file is part of COPS.
 *
 * COPS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * COPS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

# ifndef _global_h
# define _global_h

# include <sys/types.h>
# include <mint/mintbind.h>

# if __MINTLIB_MAJOR__ == 0 && __MINTLIB_MINOR__ < 57
# error Require at least MiNTLib 0.57
# endif

# ifndef _cdecl
# define _cdecl __CDECL
# endif

# define str(x)		_stringify (x)
# define _stringify(x)	#x

#define min(a,b)	((a) < (b) ? (a) : (b))
#define max(a,b)	((a) > (b) ? (a) : (b))
#define between(x,a,b)	((x >= a) && (x <= b))
#define	ABS(a)		((a) < 0 ? -(a) : (a))

#if 0
#define DEBUG(x) printf x, fflush(stdout)
#else
#define DEBUG(x)
#endif

/* XXX add to mintlib somewhere under include/mint */
struct xattr
{
	unsigned short mode;
	long           ino;	/* must be 32 bits */
	unsigned short dev;	/* must be 16 bits */
	short          rdev;	/* not supported by the kernel */
	unsigned short nlink;
	unsigned short uid;	/* must be 16 bits */
	unsigned short gid;	/* must be 16 bits */
	long           size;
	long           blksize;
	long           blocks;
	unsigned long  mtime;
	unsigned long  atime;
	unsigned long  ctime;
	short          attr;
	short res1;		/* reserved for future kernel use */
	long res2[2];
};

# endif /* _global_h */
