/*
 * Filename:     
 * Version:      
 * Author:       Jens Heitmann, Frank Naumann
 * Started:      1999-05
 * Last Updated: 1999-08-18
 * Target O/S:   MiNT
 * Description:  
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list <mint@fishpool.com>.
 * 
 * Copying:      Copyright 1999 Frank Naumann <fnaumann@cs.uni-magdeburg.de>
 *               Copyright 1999 Jens Heitmann
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
 * 
 */

# ifndef _global_h
# define _global_h

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <time.h>
# include <unistd.h>
# include <fcntl.h>
# include <netdb.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <errno.h>

# include <mintbind.h>
# include "mintsock.h"

# if __MINTLIB_MAJOR__ == 0 && __MINTLIB_MINOR__ < 57
typedef size_t socklen_t;
# endif

# ifndef _cdecl
# define _cdecl		__CDECL
# endif

# define str(x)		_stringify (x)
# define _stringify(x)	#x


# define SOCKDEV	"u:\\dev\\socket"
# define TCPCOOKIE	0x49434950L	/* ICIP */
# define FREECOOKIE	0x46524545L	/* FREE */
# define JAR		0x5A0

/* struct for Pmsg() */
typedef struct
{
	long msg1;
	long msg2;
	short pid;
} PMSG;

# define MGW_GETHOSTBYNAME	0x6d676d01UL
	

/*
 * Adapted part of STSOCKET.H
 */
# define ST_FDSET_LONGS 8

typedef struct st_fd_set st_fd_set;
struct st_fd_set 
{
	ulong fds_bits [ST_FDSET_LONGS];
};

# define ST_NFDBITS		(8 * sizeof(unsigned long))
# define ST_FD_SETSIZE		(ST_FDSET_LONGS * ST_NFDBITS)

# define ST_FDELT(d)		(d / ST_NFDBITS)
# define ST_FDMASK(d)		(1L << (d % ST_NFDBITS))
# define ST_FD_SET(d, set)	((set)->fds_bits[ST_FDELT(d)] |= ST_FDMASK(d))
# define ST_FD_CLR(d, set)	((set)->fds_bits[ST_FDELT(d)] &= ~ST_FDMASK(d))
# define ST_FD_ISSET(d, set)	((set)->fds_bits[ST_FDELT(d)] & ST_FDMASK(d))
# define ST_FD_ZERO(set) 	memset(set, 0, sizeof(st_fd_set))


# endif /* _global_h */
