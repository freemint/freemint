/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2001 Frank Naumann <fnaumann@freemint.de>
 * Copyright 1993, 1994, 1995, 1996 Kay Roemer
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
 * Started: 2000-01-12
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

/*
 * Definitions for the socket layer.
 */

# ifndef _mint_net_h
# define _mint_net_h


/* possible socket states */
enum so_state
{
	SS_VIRGIN = 0,
	SS_ISUNCONNECTED,
	SS_ISCONNECTING,
	SS_ISCONNECTED,
	SS_ISDISCONNECTING,
	SS_ISDISCONNECTED
};

/* possible socket flags */
# define SO_ACCEPTCON	0x0001		/* socket is accepting connections */
# define SO_RCVATMARK	0x0002		/* in-band and oob data are in sync */
# define SO_CANTRCVMORE	0x0004		/* shut down for receives */
# define SO_CANTSNDMORE	0x0008		/* shut down for sends */
# define SO_CLOSING	0x0010		/* socket is close()ing */
# define SO_DROP	0x0020		/* drop connecting socket when accept()
					   fails due to lacking file handles */


# ifdef __KERNEL__

# include "socket.h"

struct socket
{
	enum so_type	type;		/* socket type: SOCK_* */
	enum so_state	state;		/* socket state: SS_* */
	short		flags;		/* socket flags: SO_* */
	struct socket	*conn;		/* peer socket */
	struct socket	*iconn_q;	/* queue of imcomplete connections */
	struct socket	*next;		/* next connecting socket in list */
	struct dom_ops	*ops;		/* domain specific operations */
	void		*data;		/* domain specific data */
	short		error;		/* async. error */
	short		pgrp;		/* process group to send sinals to */
	long		rsel;		/* process selecting for reading */
	long		wsel;		/* process selecting for writing */
	long		xsel;		/* process selecting for exec. cond. */
	short		date;		/* date stamp */
	short		time;		/* time stamp */
	short		lockpid;	/* pid of locking process */
};

/* domain, as the socket level sees it */
struct dom_ops
{
	short	domain;
	struct dom_ops *next;
	
	long	(*attach)	(struct socket *s, short proto);
	long	(*dup)		(struct socket *news, struct socket *olds);
	long	(*abort)	(struct socket *s, enum so_state ostate);
	long	(*detach)	(struct socket *s);
	long	(*bind)		(struct socket *s, struct sockaddr *addr,
				 short addrlen);
	
	long	(*connect)	(struct socket *s, const struct sockaddr *addr,
				 short addrlen, short flags);
	
	long	(*socketpair)	(struct socket *s1, struct socket *s2);
	long	(*accept)	(struct socket *s, struct socket *new,
			 	 short flags);
	
	long	(*getname)	(struct socket *s, struct sockaddr *addr,
			 	 short *addrlen, short peer);
# define PEER_ADDR	0
# define SOCK_ADDR	1
	long	(*select)	(struct socket *s, short sel_type, long proc);
	long	(*ioctl)	(struct socket *s, short cmd, void *arg);
	long	(*listen)	(struct socket *s, short backlog);
	
	long	(*send)		(struct socket *s, const struct iovec *iov,
				 short niov, short block, short flags,
				 const struct sockaddr *addr, short addrlen);
	
	long	(*recv)		(struct socket *s, const struct iovec *iov,
				 short niov, short block, short flags,
				 struct sockaddr *addr, short *addrlen);
	
	long	(*shutdown)	(struct socket *s, short flags);
	long	(*setsockopt)	(struct socket *s, short level, short optname,
			 	 char *optval, long optlen);
	
	long	(*getsockopt)	(struct socket *s, short level, short optname,
			 	 char *optval, long *optlen);
};

# endif /* __KERNEL__ */

# endif /* _mint_net_h */
