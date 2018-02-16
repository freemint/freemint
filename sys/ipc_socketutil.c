/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2000, 2001 Frank Naumann <fnaumann@freemint.de>
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
 * Started: 2001-01-12
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

# include "ipc_socketutil.h"

# include "libkern/libkern.h"

# include "ipc_unix.h"
# include "kmemory.h"
# include "proc.h"
# include "time.h"


/* socket utility functions */


void
domaininit (void)
{
	so_register (AF_UNIX, &unix_ops);
}


/* list of registered domains */
static struct dom_ops *alldomains = NULL;


/* Register a new domain `domain'. Note that one can register several
 * domains with the same `domain' value. When looking up a domain, the
 * one which was last installed is chosen.
 */
void
so_register (short domain, struct dom_ops *ops)
{
	DEBUG (("sockets: registering domain %i (ops %p)", domain, ops));

	ops->domain = domain;
	ops->next = alldomains;
	alldomains = ops;
}

/* Unregister all registered domains whose domain-field equals
 * to `domain'.
 */
void
so_unregister (short domain)
{
	struct dom_ops *ops;

	ops = alldomains;
	while (ops && (ops->domain == domain))
		ops = alldomains = ops->next;

	if (ops)
	{
		do {
			while (ops->next && (ops->next->domain != domain))
				ops = ops->next;
			if (ops->next)
				ops->next = ops->next->next;
		}
		while (ops->next);
	}
}


/* Allocate a new socket struct and initialize it.
 * Returns the address of the newly allocated socket or NULL, indicating
 * `out of memory'.
 */
long _cdecl
so_create (struct socket **resultso, short domain, short type, short protocol)
{
	struct dom_ops *ops;
	struct socket *so;

	for (ops = alldomains; ops; ops = ops->next)
		if (ops->domain == domain)
			break;

	if (!ops)
	{
		DEBUG (("sp_create: domain %i not supported", domain));
		return EAFNOSUPPORT;
	}

	so = kmalloc (sizeof (*so));
	if (so)
	{
		long ret;

		mint_bzero (so, sizeof (*so));

		so->date = datestamp;
		so->time = timestamp;

		switch (type)
		{
			case SOCK_DGRAM:
			case SOCK_STREAM:
			case SOCK_RAW:
			case SOCK_RDM:
			case SOCK_SEQPACKET:
			{
				so->ops = ops;
				so->type = type;

				ret = (*so->ops->attach) (so, protocol);
				if (!ret)
				{
					so->state = SS_ISUNCONNECTED;

					*resultso = so;
					return 0;
				}

				DEBUG (("so_create: failed to attach protocol data (%li)", ret));
				break;
			}
			default:
			{
				ret = ESOCKTNOSUPPORT;
				break;
			}
		}

		kfree (so);
		return ret;
	}

	return ENOMEM;
}

long _cdecl
so_dup (struct socket **resultso, struct socket *so)
{
	struct socket *newso;

	newso = kmalloc (sizeof (*newso));
	if (newso)
	{
		long ret;

		mint_bzero (newso, sizeof (*newso));

		newso->date = datestamp;
		newso->time = timestamp;

		newso->ops = so->ops;
		newso->type = so->type;

		ret = (*so->ops->dup)(newso, so);
		if (ret == 0)
		{
			newso->state = SS_ISUNCONNECTED;
			*resultso = newso;
		}
		else
		{
			DEBUG (("so_dup: failed to dup protocol data"));
			kfree (newso);
		}

		return ret;
	}

	return ENOMEM;
}

void _cdecl
so_free (struct socket *so)
{
	if (so_release (so))
		return; /* XXX - check for EINTR */

	kfree (so);
}

/* Release a socket to disconnected state.
 */
long _cdecl
so_release (struct socket *so)
{
	struct socket *peer;
	short ostate;
	long r = 0;

	ostate = so->state;
	if (ostate != SS_VIRGIN && ostate != SS_ISDISCONNECTED)
	{
		so->state = SS_ISDISCONNECTING;
		so->flags |= SO_CLOSING;

		/* Tell all clients waiting for connections that we are
		 * closing down. This is done by setting there `conn'-field
		 * to zero and waking them up.
		 * NOTE: setting clients state to SS_ISDISCONNECTING here
		 * causes the client not to be able to try a second connect(),
		 * unless somewhere else its state is reset to SS_ISUNCONNECTED
		 */
		if (so->flags & SO_ACCEPTCON)
		{
			while ((peer = so->iconn_q))
			{
				so->iconn_q = peer->next;
				peer->state = SS_ISDISCONNECTING;
				peer->conn = 0;
				(*peer->ops->abort)(peer, SS_ISCONNECTING);
			}
		}

		/* Remove ourselves from the incomplete connection queue of
		 * some server. If we are on any queue, so->state is the
		 * server we are connecting to.
		 * so->state is set to 0 afterwards to indicate that
		 * connect() failed.
		 */
		if (ostate == SS_ISCONNECTING)
		{
			struct socket *last, *server;

			server = so->conn;
			if (server)
			{
				last = server->iconn_q;
				if (last == so)
				{
					server->iconn_q = so->next;
				}
				else
				{
					while (last && (last->next != so))
						last = last->next;
					if (last) last->next = so->next;
				}
				so->conn = 0;
			}
		}

		/* Tell the peer we are closing down, but let the underlying
		 * protocol do its part first.
		 */
		if (ostate == SS_ISCONNECTED && so->conn)
		{
			peer = so->conn;
			so->conn = 0;
			peer->state = SS_ISDISCONNECTING;
			(*so->ops->abort)(peer, SS_ISCONNECTED);
		}

		r = (*so->ops->detach) (so);
		if (r == 0)
		{
			/* No protocol data attached anymore, so we are
			 * disconnected.
			 */
			so->state = SS_ISDISCONNECTED;

			/* Wake anyone waiting for `so', since its state
			 * changed.
			 */
			wake (IO_Q, (long) so);
			so_wakersel (so);
			so_wakewsel (so);
			so_wakexsel (so);
		}
		else
			ALERT ("so_release: so->ops->detach failed!");
	}

	return r;
}

void _cdecl
so_sockpair (struct socket *so1, struct socket *so2)
{
	so1->conn = so2;
	so2->conn = so1;
	so1->state = SS_ISCONNECTED;
	so2->state = SS_ISCONNECTED;
}

/* Put `client' on the queue of incomplete connections of `server'.
 * Blocks until the connection is accepted or it's impossible to
 * establish the connection, unless `nonblock' != 0.
 * `Backlog' is the number of pending connections allowed for `server'.
 * so_connect() will fail if there are already `backlog' clients on the
 * server queue.
 * NOTE: Before using this function to connect `client' to `server',
 * you should do the following:
 *	if (client->state == SS_ISCONNECTING) return EALREADY;
 */
long _cdecl
so_connect (struct socket *server, struct socket *client,
		short backlog, short nonblock, short wakeup)
{
	struct socket *last;
	short clients;

	if (!(server->flags & SO_ACCEPTCON))
	{
		DEBUG (("sockdev: so_connect: server is not listening"));
		return EINVAL;
	}

	/* Put client on the incomplete connection queue of server. */
	client->next = 0;
	last = server->iconn_q;
	if (last)
	{
		for (clients = 1; last->next; last = last->next)
			++clients;
		if (clients >= backlog)
		{
			DEBUG (("sockdev: so_connect: backlog exeeded"));
			return ETIMEDOUT;
		}
		last->next = client;
	}
	else
	{
		if (backlog == 0)
		{
			DEBUG (("sockdev: so_connect: backlog exeeded"));
			return ETIMEDOUT;
		}
		server->iconn_q = client;
	}
	client->state = SS_ISCONNECTING;
	client->conn = server;

	/* Wake proc's selecting for reading on server, which are waiting
	 * for a connection request on the listening server.
	 */
	if (wakeup)
	{
		so_wakersel (server);
		wake (IO_Q, (long) server);
	}

	if (nonblock)
		return EINPROGRESS;

	while (client->state == SS_ISCONNECTING)
	{
		if (sleep (IO_Q, (long) client))
		{
			/* Maybe someone closed the client on us. */
			return EINTR;
		}
		if (!client->conn)
		{
			/* Server rejected us from its queue. */
			DEBUG (("sockdev: so_connect: connection refused"));
			return ECONNREFUSED;
		}
	}

	/* Now we are (at least were) connected to server. */
	return 0;
}

/* Take the first waiting client from `server's incomplete connection
 * queue and connect `newsock' to it.
 * Blocks until a connection request is available, unless `nonblock' != 0.
 */
long _cdecl
so_accept (struct socket *server, struct socket *newso, short nonblock)
{
	struct socket *client;

	/* Remove the first waiting client from the queue of incomplete
	 * connections. Go to sleep if non is waiting, unless nonblocking
	 * mode is set.
	 */
	while (!server->iconn_q)
	{
		if (nonblock)
			return EWOULDBLOCK;

		if (server->flags & SO_CANTRCVMORE)
			return ECONNABORTED;

		if (sleep (IO_Q, (long)server))
			/* may be someone closed the server on us */
			return EINTR;
	}

	client = server->iconn_q;
	server->iconn_q = client->next;

	/* Connect the new socket and the client together. */
	client->next = 0;
	newso->conn = client;
	client->conn = newso;
	newso->state = SS_ISCONNECTED;
	client->state = SS_ISCONNECTED;

	/* Wake proc's selecting for writing on client, which are waiting
	 * for a connect() to finish on a nonblocking socket.
	 */
	so_wakewsel (client);

	wake (IO_Q, (long) client);
	return 0;
}

void _cdecl
so_drop (struct socket *so, short nonblock)
{
	struct socket *newso;
	long ret;

	if (!(so->flags & SO_DROP))
		return;

	DEBUG (("so_drop: dropping incoming connection"));

	ret = so_dup (&newso, so);
	if (ret) return;

	(*so->ops->accept)(so, newso, nonblock);
	so_free (newso);
}
