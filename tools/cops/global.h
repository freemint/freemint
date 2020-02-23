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
#undef DEBUG
#include <mint/arch/nf_ops.h>
#define DEBUG(x) nf_debugprintf x
#if defined(__GNUC__)
#define DEBUG_FMT "%s"
#define DEBUG_ARGS __FUNCTION__
#else
#define DEBUG_FMT "%s:%d"
#define DEBUG_ARGS __FILE__, __LINE__
#endif
#else
#define DEBUG(x)
#endif

/* XXX add to mintlib somewhere under include/mint */
#ifndef __XATTR
#define __XATTR
struct xattr
{
	unsigned short st_mode;
	long           st_ino;	/* must be 32 bits */
	unsigned short st_dev;	/* must be 16 bits */
	short          st_rdev;	/* not supported by the kernel */
	unsigned short st_nlink;
	unsigned short st_uid;	/* must be 16 bits */
	unsigned short st_gid;	/* must be 16 bits */
	long           st_size;
	long           st_blksize;
	long           st_blocks;
	unsigned long  st_mtime;
	unsigned long  st_atime;
	unsigned long  st_ctime;
	short          st_attr;
	short res1;		/* reserved for future kernel use */
	long res2[2];
};
#endif

#ifndef BOOLEAN
#define BOOLEAN int
#endif

#ifndef FALSE
#define FALSE 0
#define TRUE  1
#endif

#ifndef __GNUC__
#define __builtin_unreachable()
#endif

#if defined(BROKEN_GEMLIB)
#define wind_get_int(a,b,c) wind_get(a,b,c,NULL,NULL,NULL)
#define wind_set_int(a,b,c) wind_set(a,b,c,0,0,0)
#define form_dial_grect(a,b,c) form_dial(a, (b)->g_x, (b)->g_y, (b)->g_w, (b)->g_h, (c)->g_x, (c)->g_y, (c)->g_w, (c)->g_h)
#define objc_change_grect(a,b,c,d,e,f) objc_change(a,b,c,(d)->g_x, (d)->g_y, (d)->g_w, (d)->g_h, e,f)
#endif

# endif /* _global_h */
