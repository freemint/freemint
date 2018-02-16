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
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2000-06-28
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _ipc_unix_h
# define _ipc_unix_h

# include "mint/mint.h"
# include "mint/net.h"
# include "mint/un.h"


struct un_data
{
	short		proto;		/* protocol, only 0 is allowed */
	short		flags;		/* unix socket flags */
	struct socket	*sock;		/* socket this un_data belongs to */
	long		index;
	long		index2;		/* index of the peer address (dgram) */
	short		head;		/* buffer head */
	short		tail;		/* buffer tail */
	short		buflen;		/* current buffer size */
	char		*buf;		/* buffer data */
	struct un_data	*next;		/* link to next un_data */
	short		backlog;	/* max. # of pending connections */
	struct sockaddr_un addr;	/* local address */
	short		addrlen;	/* length of local address */
};

/* datagram header */
struct dgram_hdr
{
	short	nbytes;			/* # of bytes in the datagram */
	long	sender;			/* index of sender of this dgram */
};

INLINE void
un_store_header (struct un_data *un, struct dgram_hdr *hdr)
{
	short i, tail;
	char *d, *s;
	
	tail = un->tail;
	s = (char *) hdr;
	d = un->buf;
	i = sizeof (*hdr);
	
	while (i--)
	{
		d[tail++] = *s++;
		if (tail >= un->buflen)
			tail = 0;
	}
	
	un->tail = tail;
}

INLINE void
un_read_header (struct un_data *un, struct dgram_hdr *hdr, short modify)
{
	short head, i;
	char *s, *d;
	
	head = un->head;
	s = un->buf;
	d = (char *) hdr;
	i = sizeof (*hdr);

	while (i--)
	{
		*d++ = s[head++];
		if (head >= un->buflen)
			head = 0;
	}
	
	if (modify)
		un->head = head;
}

INLINE short
UN_USED (struct un_data *un)
{
	register short space;
	
	space = un->tail - un->head;
	if (space < 0)
		space += un->buflen;
	
	return space;
}

# define UN_FREE(un)	((un)->buflen - UN_USED (un) - 1)

# define UN_HASH(i)	(((i) ^ ((i)>>8) ^ ((i)>>16) ^ ((i)>>24)) & 0xff)
# define UN_HASH_SIZE	256

# define UN_INDEX(u)	((u)->index)


struct un_data *	un_lookup (long, enum so_type);
void			un_put (struct un_data *);
void			un_remove (struct un_data *);
long			un_resize (struct un_data *, long);
long			un_namei (struct sockaddr *, short, long *);	


extern struct dom_ops unix_ops;
extern DEVDRV unixdev;


# endif /* _ipc_unix_h */
