/*
 * Filename:     gs.h
 * Project:      GlueSTiK
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list <mint@fishpool.com>.
 * 
 * Copying:      Copyright 1999 Frank Naumann <fnaumann@cs.uni-magdeburg.de>
 * 
 * Portions copyright 1997, 1998, 1999 Scott Bigham <dsb@cs.duke.edu>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# ifndef _gs_h
# define _gs_h

# include <stdlib.h>
# include <stdio.h>
# include <sys/types.h>

# include "transprt.h"


# if 0
# define GS_DEBUG
# endif

# ifndef GS_DEBUG
# define DEBUG(x)
# else
# define DEBUG(x)	{ printf x; printf ("\n"); fflush (stdout); }
# endif

# if __MINTLIB_MAJOR__ == 0 && __MINTLIB_MINOR__ < 57
typedef size_t socklen_t;
# endif

typedef unsigned char	uchar;

# ifndef _cdecl
# define _cdecl		__CDECL
# endif

# define str(x)		_stringify (x)
# define _stringify(x)	#x

# define SOCKDEV	"u:\\dev\\socket"
# define FREECOOKIE	0x46524545L	/* FREE */
# define JAR		0x5A0

/* struct for Pmsg() */
typedef struct
{
	long msg1;
	long msg2;
	short pid;
} PMSG;

# define FLG_SEM		0x4753464CUL	/* 'GSFL' */
# define GS_GETHOSTBYNAME	0x6d676d11UL


# endif /* _gs_h */
