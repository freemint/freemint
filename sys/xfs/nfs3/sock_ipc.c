/*
 * Copyright 1993, 1994 by Ulrich K�hn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 * 
 * Modified for FreeMiNT CVS
 * by Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

/*
 * File : sock_ipc.c
 *        inter process communication for the xfs
 */

# include "sock_ipc.h"

# include "arch/timer.h"
# include "mint/ioctl.h"
# include "mint/net.h"
# include "mint/sysctl.h"
# include "sys/param.h"

# include "rpc_xdr.h"


static void free_message_body (MESSAGE *);

static struct socket *nfs_so = NULL;
static short nonblock = 0;


static inline long
bind (struct socket *so, struct sockaddr *addr, short addrlen)
{
	return (*so->ops->bind)(so, addr, addrlen);
}

static inline long
setsockopt (struct socket *so, short level, short optname, void *optval, long optlen)
{
	return (*so->ops->setsockopt)(so, level, optname, optval, optlen);
}

static inline long
so_ioctl (struct socket *so, int cmd, void *buf)
{
	return (*so->ops->ioctl)(so, cmd, buf);
}

static inline long
so_read (struct socket *so, char *buf, long buflen)
{
	struct iovec iov[1] = {{ buf, buflen }};
	
	if (so->state == SS_VIRGIN)
		return EINVAL;
	
	return (*so->ops->recv)(so, iov, 1, nonblock, 0, 0, 0);
}

static inline long
sendmsg (struct socket *so, struct msghdr *msg, short flags)
{
	return (*so->ops->send)(so, msg->msg_iov, msg->msg_iovlen,
				nonblock, flags,
				msg->msg_name, msg->msg_namelen);
}

static inline long
recvmsg (struct socket *so, struct msghdr *msg, short flags)
{
	short namelen = msg->msg_namelen;
	long ret;
	
	ret = (*so->ops->recv)(so, msg->msg_iov, msg->msg_iovlen,
				nonblock, flags,
				msg->msg_name, &namelen);
	
	msg->msg_namelen = namelen;
	return ret;
}


/* In order to make this module more usable we provide the possibility
 * to select the number of the remote program and its version at
 * initialization time. These numbers is stored in this variable for
 * later use.
 */
ulong rpc_program;
ulong rpc_progversion;


/* This is the authentification stuff: we need to store the hostname
 * by ourselfs as MiNT is not capable of providing one for us.
 * We also use a static buffer for the auth_unix structure which is
 * reused for every request.
 *
 * For efficiency reasons, we operate only on the xdred structure,
 * so we set up some pointers for the changing parts of it.
 */
char hostname[MAXHOSTNAMELEN] = "FreeMiNT";  /* the default hostname */
int do_auth_init = 2;  /* retry count to read \etc\hostname */


# define AUTH_UNIX_MAX  340

# ifdef UNEFFICIENT
struct auth_unix
{
	ulong stamp;
	char *machinename;
	ulong uid;
	ulong gid;
	ulong n_gids;
	ulong *gids;
} the_auth;
# endif

char the_xdr_auth[AUTH_UNIX_MAX];
ulong *p_stamp, *p_uid, *p_gid, *p_ngid, *p_gids;
ulong auth_baselen;  /* length of data in the_xdr_auth without supp gids */

opaque_auth unix_auth =
{
	AUTH_UNIX, &the_xdr_auth[0], 0    /* correct the length!! */
};



static int
init_auth (void)
{
	long res, r;
	char *p;
	
	/* try first the new syscall */
	{
		char buf[MAXHOSTNAMELEN];
		long mib[2];
		long size;
		
		mib[0] = CTL_KERN;
		mib[1] = KERN_HOSTNAME;
		size = sizeof(buf);
		
		r = p_sysctl(mib, 2, buf, (unsigned long *)&size, NULL, 0);
		if (r == 0)
		{
			if (strcmp (buf, "(none)"))
				strcpy (hostname, buf);
			else
				r = -1;
		}
		
		/* try /etc/hostname */
		if (r)
		{
			res = f_open ("\\etc\\hostname", O_RDONLY);
			if (res >= 0)
			{
				int fd = res;
				res = f_read (fd, MAXHOSTNAMELEN-1, buf);
				(void) f_close (fd);
				if (res > 0)
				{
					buf[res] = '\0';
					p = buf;
					while (*p)
					{
						if ((*p == '\r') || (*p == '\n'))
							break;
						p += 1;
					}
					*p = '\0';
					
					if (buf[0])
					{
						strcpy (hostname, buf);
						r = 0;
					}
				}
			}
		}
	}
	
	/* set up xdred auth_unix structure */
	bzero (&the_xdr_auth [0], AUTH_UNIX_MAX);
	p = &the_xdr_auth [0];
	p_stamp = (ulong *) p;
	p += sizeof (ulong);
	res = strlen (hostname);
	*(ulong *) p = res;
	p += sizeof (ulong);
	strncpy (p, hostname, res);
	p += res;
	if (res & 0x3)
		p += 4 - (res & 0x3);
	p_uid = (ulong *) p;
	p += sizeof (ulong);
	p_gid = (ulong *) p;
	p += sizeof (ulong);
	*(ulong *) p = 0;  /* no of supplementary group ids. For now, set to 0 */
	p_ngid = (ulong *) p;
	p += sizeof (ulong);
	p_gids = (ulong *) p;
	
	res = p - &the_xdr_auth[0];
	unix_auth.len = res;
	auth_baselen = res;	
	
	return r;
}


static void
setup_auth (ulong stamp)
{
# define NGROUPS_MAX	8
	unsigned short suppgrps [2 * NGROUPS_MAX];
	long ngrp;

	*p_stamp = stamp;
	*p_uid = (ulong) p_geteuid ();
	*p_gid = (ulong) p_getegid ();

	{
		char *p = (char *) & the_xdr_auth[0];

		(void) p; /* suppress warning */
		DEBUG (("setup_auth: machine name has len %ld, `%c%c%c...'",
		          *(long*)(p+sizeof(long)), (p+2*sizeof(long))[0],
		          (p+2*sizeof(long))[1], (p+2*sizeof(long))[2]));
		DEBUG (("setup_auth: setting uid %ld/gid %ld", *p_uid, *p_gid));
	}
	
	ngrp = p_getgroups (2 * NGROUPS_MAX, suppgrps);
	if (ngrp < 0)
		ngrp = 0;
	else if (ngrp > NGROUPS_MAX)
		ngrp = NGROUPS_MAX;
	
	*p_ngid = ngrp;
	unix_auth.len = auth_baselen + sizeof (ulong) * ngrp;
	
	for ( ; ngrp >= 0; ngrp -= 1)
		p_gids [ngrp] = (short)suppgrps [ngrp];
}

/*============================================================*/

/* This structure and the access functions are for throwing away all
 * messages that have xids nobody waits for. All the pending requests
 * are kept in a linked list until they are satisfied or timed out.
 */

/* BUG: how do we deal with processes that have died before we notice?
 *      i.e. a process that was killed with SIGKILL while waiting for
 *      an nfs answer?
 */
typedef struct request
{
	MESSAGE		msg;
	short		have_answer;
	struct request	*next;
	short		pid;
	ulong		xid;
} REQUEST;

REQUEST *pending = NULL;

/* TL: implement locking functions for the request actions.
 *     Note: We will need this when MiNT will be also multitasking
 *           in super mode some day.
 *     Note: The TAS instruction is only useful on multiprocessor
 *           systems, and it is buggy on the FireBee.
 *           The BSET.B #7 instruction does exactly the same thing,
 *           without additional overhead.
 */

volatile char lock = 0;

# define TAS(val) ({ 						\
	char _locked;					\
	long _tstval = (long)&val;				\
	__asm__ __volatile__(					\
		"clrb		%0\n\t"				\
		"movel		%1,%%a0\n\t"			\
		"bsetb		#7,%%a0@\n\t"			\
		"beq		1f\n\t"				\
		"moveb		#1,%0\n\t"			\
		"1:\n\t"					\
		/* outputs  */					\
		: "=d"(_locked)			 		\
		/* inputs   */					\
		: "r"(_tstval)					\
		/* modifies */					\
		: "a0" 						\
	);							\
	_locked;						\
})

static int
insert_request (ulong xid)
{
	REQUEST *p;
	
	while (TAS (lock))
		s_yield ();
	
	p = kmalloc (sizeof (*p));
	if (!p)
	{
		lock = 0;
		return -1;
	}
	
	p->have_answer = 0;
	p->msg.flags = FREE_MSG;
	p->pid = p_getpid ();
	p->xid = xid;
	p->next = pending;
	pending = p;
	
	lock = 0;
	return 0;
}

static int
delete_request (ulong xid)
{
	REQUEST *p, **pp;
	
	while (TAS (lock))
		s_yield ();
	
	for (p = pending, pp = &pending; p; pp = &p->next, p = p->next)
	{
		if (p->xid == xid)
			break;
	}
	
	if (!p)
	{
		lock = 0;
		return -1;
	}
	
	if (p->have_answer)
		free_message_body (&p->msg);
	
	*pp = p->next;
	kfree (p);
	
	lock = 0;
	return 0;
}

static int
remove_request (ulong xid)
{
	REQUEST *p, **pp;

	while (TAS (lock))
		s_yield ();

	for (p = pending, pp = &pending; p; pp = &p->next, p = p->next)
	{
		if (p->xid == xid)
			break;
	}
	
	if (!p)
	{
		lock = 0;
		return -1;
	}
	
	*pp = p->next;
	
	lock = 0;
	return 0;
}	

static REQUEST *
search_request (ulong xid)
{
	REQUEST *p;
	
	while (TAS (lock))
		s_yield ();
	
	for (p = pending;  p;  p = p->next)
	{
		if (p->xid == xid)
		{
			lock = 0;
			return p;
		}
	}
	
	lock = 0;
	return NULL;
}

/*============================================================*/

/* free only the message body
 */
static void
free_message_body (MESSAGE *m)
{
	if (m->flags & FREE_HEADER)
		kfree (m->header);
	if (m->flags & FREE_DATA)
		kfree (m->data);
	if (m->flags & FREE_BUFFER)
		kfree (m->buffer);
	
	m->flags &= ~DATA_FLAGS;
}

/* free only the message header; if it was taken from the pool of free
 * message headers, put it into the list
 */
static void
free_message_header (MESSAGE *m)
{
	if (m->flags & FREE_MSG)
		kfree(m);
}

void
free_message (MESSAGE *m)
{
	if (m->flags & FREE_HEADER)
		kfree (m->header);
	if (m->flags & FREE_DATA)
		kfree (m->data);
	if (m->flags & FREE_BUFFER)
		kfree (m->buffer);
	if (m->flags & FREE_MSG)
		kfree (m);
}

MESSAGE *
alloc_message (MESSAGE *m, char *buf, long buf_len, long data_size)
{
	if (!m)
	{
		m = kmalloc (sizeof (*m));
		if (!m)
			return NULL;
		
		m->flags = FREE_MSG;
	}
	else
	{
		m->flags = 0;
	}
	
	m->data = m->buffer = m->header = NULL;
	m->data_len = m->hdr_len = 0;
	
	if (0 == data_size)
		return m;
	
	if (!buf || (buf_len < data_size))
	{
		m->data = kmalloc (data_size);
		m->buffer = m->data;
		m->buf_len = data_size;
		m->flags |= FREE_BUFFER;
	}
	else
	{
		m->data = m->buffer = buf;
		m->buf_len = buf_len;
	}
	
	if (!m->data)
	{
		if (m->flags & FREE_MSG)
			kfree (m);
		
		return NULL;
	}
	
	m->data_len = data_size;
	return m;
}

/*============================================================*/

static long
bindresvport (struct socket *so)
{
	struct sockaddr_in t;
	short port;
	long r;
	
	t.sin_family = AF_INET;
	t.sin_addr.s_addr = htonl (INADDR_ANY);
	for (port = IPPORT_RESERVED - 1; port > IPPORT_RESERVED / 2; port--)
	{
		t.sin_port = htons (port);
		r = bind (so, (struct sockaddr *) &t, sizeof (t));
		if (r == 0)
			return 0;
		
		if (r < 0 && r != EADDRINUSE)
			return r;
	}
	
	return EADDRINUSE;
}

static long
open_connection (struct socket **resultso)
{
	struct socket *so;
	long arg, ret;
	
	/* Open socket. The file handle will be global as it is specified in the
	 * kernel socket library.
	 */
	ret = so_create (&so, PF_INET, SOCK_DGRAM, 0);
	if (ret < 0)
	{
		DEBUG (("rpcfs: could not open socket -> %ld", ret));
		return ret;
	}
	
	assert (so);
	
	/* Do some settings on the socket so that it becomes usable
	 */
	
	arg = 10240;
	ret = setsockopt (so, SOL_SOCKET, SO_RCVBUF, &arg, sizeof (arg));
	if (ret < 0)
	{
		DEBUG (("rpcfs: could not set socket options -> %ld", ret));
		goto error;
	}
	
	arg = 10240;
	ret = setsockopt (so, SOL_SOCKET, SO_SNDBUF, &arg, sizeof (arg));
	if (ret < 0)
	{
		DEBUG (("rpcfs: could not set socket options -> %ld", ret));
		goto error;
	}
	
	/* Now bind the socket to a local address so that we can use it.
	 * Bind to an priviledged (reserved) port, because most NFS
	 * servers refuse connections to clients on non priviledged
	 * ports for security reasons.
	 * Note that binding to reserved ports is allowed only for
	 * super user processes, so open_connection() MUST be done
	 * at boot time from ipc_init() and the connection CANNOT
	 * be closed and reopened later.
	 * BTW: closing and reopening the socket is pretty useless
	 * for UDP sockets, because they are stateless.
	 */
	ret = bindresvport (so);
	if (ret < 0)
	{
		DEBUG (("rpcfs: could not bind socket to local address -> %ld", ret));
		goto error;
	}
	
	DEBUG (("open_connection: got socket %p", (void *) so));
	
	*resultso = so;
	return ret;
	
error:
	so_free (so);
	return ret;
}

static void
scratch_message (struct socket *so)
{
	char buf[8];
	
	so_read (so, buf, 8);
}

/* This function extracts the xid from the rpc header of a message. For
 * speed reasons it has to know about the xdr representation of the rpc
 * reply header. Indeed we have only to consult the first longword
 * in the message.
 */
static ulong
get_xid (MESSAGE *m)
{
	return *(ulong *) m->data;
}

/* Send a message to the server through a socket, but do not delete the
 * message as we might want to resend it later. The process is blocked
 * until the message has been sent successfully.
 */
static long
rpc_sendmessage (struct socket *so, SERVER_OPT *opt, MESSAGE *mreq)
{
	struct iovec iov[2];
	struct msghdr msg;
	long ret;
	
	/* set up structures for sendmsg() */
	iov[0].iov_base = mreq->header;
	iov[0].iov_len = mreq->hdr_len;
	iov[1].iov_base = mreq->data;
	iov[1].iov_len = mreq->data_len;
	
	msg.msg_name = (void *) &opt->addr;
	msg.msg_namelen = sizeof (opt->addr);
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;
	msg.msg_accrights = NULL;
	msg.msg_accrightslen = 0;
	
	ret = sendmsg (so, &msg, 0);
	if (ret < 0)
	{
		DEBUG (("rpc_sendmessage: error %ld during sending", ret));
		DEBUG (("rpc_sendmessage: handle is %p", (void *) so));
	}
	
	return ret;
}

/* Receive a message from a socket. The message header must be provided as
 * well as the length of the message to be read.
 */
static MESSAGE *
rpc_receivemessage (struct socket *so, MESSAGE *mrep, long toread)
{
	long ret;
	char *buf;
	struct iovec iov[1];
	struct msghdr msg;
	struct sockaddr_in addr;
	
	mrep->buffer = mrep->data = mrep->header = NULL;
	
	/* allocate buffer for message body
	 */
	buf = kmalloc (toread);
	if (!buf)
	{
		scratch_message (so);
		DEBUG (("rpc_receivemessage: failed to allocate receive buffer"));
		return NULL;
	}
	
	iov[0].iov_base = buf;
	iov[0].iov_len = toread;
	
	msg.msg_name = (void *) &addr;
	msg.msg_namelen = sizeof (addr);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_accrights = NULL;
	msg.msg_accrightslen = 0;
	
	/* read message
	 */
	ret = recvmsg (so, &msg, 0);
	if (ret != toread)
	{
		DEBUG (("rpc_receivemessage: could not read message body"));
		kfree (buf);
		return NULL;
	}
	
	/* SECURITY: here we should bother about the address of the sender so that
	 *           we can later check for the correct sender.
	 */
	mrep->buffer = buf;
	mrep->flags |= FREE_BUFFER;
	mrep->data = buf;
	mrep->data_len = toread;
	mrep->next = NULL;
	mrep->xid = 0;
	
	return mrep;
}


/* this function does all the dirty work:
 *  - set up rpc header
 *  - link request into linked list, send it and go to sleep
 *  - receive reply
 *  - break down reply rpc header
 *  - return results of remote function or error message
 * the results (if valid) have to be freed after use
 */
long
rpc_request (SERVER_OPT *opt, MESSAGE *mreq, ulong proc, MESSAGE **mrep)
{
	char req_buf[MAX_RPC_HDR_SIZE];
	rpc_msg hdr;
	xdrs xhdr;
	MESSAGE *reply, mbuf;
	static volatile ulong xid = 0;
	ulong our_xid;
	long r;
	
	
	/* make a header */
	our_xid = xid++;
	hdr.xid = our_xid;
	hdr.mtype = CALL;
	hdr.cbody.rpcvers = RPC_VERSION;
	hdr.cbody.prog = rpc_program;
	hdr.cbody.vers = rpc_progversion;
	hdr.cbody.proc = proc;
	
	if (do_auth_init)
	{
		if (init_auth () == 0)
			do_auth_init = 0;
		else
			do_auth_init -= 1;
	}
	setup_auth (our_xid);
	hdr.cbody.cred = unix_auth;
	hdr.cbody.verf = null_auth;
	
	/* HACK: to prevent xdr_rpc_msg from encoding arguments */
	hdr.cbody.xproc = NULL;
	
	mreq->hdr_len = xdr_size_rpc_msg (&hdr);
	if (mreq->hdr_len > MAX_RPC_HDR_SIZE)
	{
		mreq->header = kmalloc (mreq->hdr_len);
		if (!mreq->header)
		{
			DEBUG (("rpc_request: no memory for rpc header"));
			free_message (mreq);
			return ENOMEM;
		}
		
		mreq->flags |= FREE_HEADER;
	}
	else
		mreq->header = req_buf;
	
	xdr_init (&xhdr, mreq->header, mreq->hdr_len, XDR_ENCODE, NULL);
	if (!xdr_rpc_msg (&xhdr, &hdr))
	{
		DEBUG (("rpc_request: failed to make rpc header"));
		free_message (mreq);
		return EBADARG;
	}
	
	
	/* This is the main send/resend code. We have to send the message, wait
	 * for reply, and if it times out, resend the message. But make sure
	 * that the code is reentrant at some points, as several processes can
	 * use the nfs at the same time. It is also possible, that a process
	 * receives a message that belongs to someone else. In that case, we
	 * check against the list of outstanding requests and store it in a
	 * list if it was waited for. Otherwise silently discard it.
	 */
	{
		struct socket *so = nfs_so;
		long timeout, stamp, toread;
		int retry;
		
		if (!so)
		{
			DEBUG (("rpc_req: no open connection"));
			free_message (mreq);
			return EACCES;
		}
		
		/* Now send the message and wait for answer */
		timeout = opt->timeo;
		stamp = *_hz_200;
		insert_request (our_xid);
		/* TL: we have to increase the timeout by opt->timeo instead of just 
                 *     multiplying it by 2
		 */
		for (retry = 0; retry < opt->retrans; retry++, timeout += opt->timeo)
		{
		    r = rpc_sendmessage (so, opt, mreq);
		    if (r < 0)
		    {
			DEBUG (("rpc_request: could not write message -> %ld", (long)r));
			
			free_message (mreq);
			delete_request (our_xid);
			return r;
		    }
		
		    /* Wait for reply. Any reply for anybody! So we have
		     * to store the message somewhere when it is not for
		     * us. Also we should look there to see if another
		     * process has already received it.
		     * NOTE: the strange 'stamp + timeout - *_hz_200 > 0'
		     * is the same as 'stamp + timeout > *_hz_200' except
		     * that the first works also when *_hz_200 wraps around
		     * while the second method waits `forever' when the
		     * timer wraps around 2^32.
		     */
		    while (stamp + timeout - *_hz_200 > 0)
		    {
			REQUEST *rq;
			MESSAGE *pm;
			
			/* give up CPU */
			s_yield ();
			
			rq = search_request (our_xid);
			if (rq && rq->have_answer)
			{
			    TRACE (("rpc_req: got reply from list"));
			    reply = &rq->msg;

			    /* Remove request from list so that delete_request
			     * doesn't kfree it. The `rq' struct will be
			     * kfreed when doing free_message_header(&rq->msg)
			     * at the end of the function.
			     * NOTE that this works because the `msg' is the
			     * first member of the REQUEST structure.
			     */
			    remove_request (our_xid);
			    goto have_reply;
			}
			/* Now try to drain the socket: read messages from
			 * it until there is nothing more or we found a
			 * reply for our request.
			 */
			while (1)
			{
			    TRACE(("rpc_req: checking socket for reply"));
			    toread = 0;
			    r = so_ioctl (so, FIONREAD, &toread);
			    if (r < 0)
			    {
				DEBUG(("rpc_req: so_ioctl(FIONREAD) failed -> %ld", r));

				free_message(mreq);
				delete_request(our_xid);
				return r;
			    }
			    if (toread == 0) break;
			    else if ((ulong) toread >= 0x7ffffffful)
			    {
				char c;

				/* Fcntl tells us that an asynchronous error
				 * is pending on the socket, caused eg. by
				 * an ICMP error message. The Fread() returns
				 * the error condition. */
				
				free_message (mreq);
				delete_request (our_xid);
				return so_read (so, &c, sizeof(c));
			    }

			    TRACE (("rpc_req: socket has something"));

			    mbuf.flags = 0;
			    reply = rpc_receivemessage (so, &mbuf, toread);
			    if (!reply) break;

			    /* Here we know that reply points to mbuf which
			     * holds a reply message. If we got already the
			     * right xid in the reply, we are ready to finish
			     * the request. Otherwise we have to store the
			     * reply in the list and wait again.
			     */
			    if (get_xid(reply) == our_xid)
			    {
				TRACE(("rpc_req: got a matching reply"));
				goto have_reply;
			    }

			    DEBUG(("rpc_req: wrong xid"));

			    rq = search_request (get_xid(reply));
			    if (!rq || rq->have_answer)
			    {
				DEBUG(("rpc_req: no req/already answer "
					"for this xid"));
				free_message(reply);
				reply = NULL;
				continue;
			    }

			    DEBUG(("rpc_req: adding message to list"));

			    /* TL: set have_answer AFTER storing the message! */
			    pm = &rq->msg;
			    r = pm->flags & ~DATA_FLAGS;
			    *pm = *reply;
			    pm->flags &= DATA_FLAGS;
			    pm->flags |= r;
			    rq->have_answer = 1;
			    reply = NULL;

			} /* while data on socket */

		    }  /* while not timed out */

		}  /* for retry < max_retry */

		DEBUG (("rpc: RPC timed out, no reply"));
		delete_request (our_xid);
		free_message (mreq);
		return EACCES;
	}
	
have_reply:
	
	delete_request (our_xid);
	free_message_body (mreq);    /* for reusing the message header */
	
	/* SECURITY: here we might want to check for the correct sender address
	 *           to not get faked answers.
	 */
	
	xdr_init (&xhdr, reply->data, reply->data_len, XDR_DECODE, NULL);
	
	/* HACK: to prevent xdr_rpc_msg from decoding the results */
	hdr.rbody.rb_arpl.ar_result.xproc = NULL;
	
	if (!xdr_rpc_msg (&xhdr, &hdr))
	{
		DEBUG (("rpc_request: failed to break down rpc header"));
		free_message_header (mreq);  /* free the rest of that */
		free_message (reply);
		return ERPC_GARBAGEARGS;
	}
	
	reply->data += xdr_getpos (&xhdr);
	reply->data_len -= xdr_getpos (&xhdr);
	
	if (MSG_ACCEPTED != hdr.rbody.rb_stat)
	{
		free_message_header (mreq);  /* free the rest of that */
		free_message (reply);
		if (RPC_MISMATCH == hdr.rbody.rb_rrpl.rr_stat)
		{
			DEBUG (("rpc_request -> rpc mismatch"));
			return ERPC_RPCMISMATCH;    /* this must be an internal error! */
		}
		else
		{
			DEBUG (("rpc_request -> auth error"));
			return ERPC_AUTHERROR;
		}
	}
	
	if (hdr.rbody.rb_arpl.ar_stat != SUCCESS)
	{
		free_message_header (mreq);  /* free the rest of that */
		free_message (reply);
		switch (hdr.rbody.rb_arpl.ar_stat)
		{
			case PROG_UNAVAIL:
				DEBUG (("rpc_request -> prog unavail"));
				return ERPC_PROGUNAVAIL;
			case PROG_MISMATCH:
				DEBUG (("rpc_request -> prog mismatch"));
				return ERPC_PROGMISMATCH;
			case PROC_UNAVAIL:
				DEBUG (("rpc_request -> proc unavail"));
				return ERPC_PROCUNAVAIL;
			default:
				DEBUG (("rpc_request -> -1"));
				return -1;
		}
	}
	
	r = mreq->flags & ~DATA_FLAGS;
	*mreq = *reply;
	mreq->flags &= DATA_FLAGS;
	mreq->flags |= r;
	*mrep = mreq;
	
	/* `reply' might or might not point to `mbuf'. So we have to do this
	 */
	free_message_header (reply);
	return 0;
}

int
init_ipc (ulong prog, ulong version)
{
	long ret;
	
	rpc_program = prog;
	rpc_progversion = version;
	
	if (init_auth () == 0)
		do_auth_init = 0;
	else
		do_auth_init -= 1;
	
	ret = open_connection (&nfs_so);
	if (ret < 0)
	{
		nfs_so = NULL;
		DEBUG (("init_ipc: cannot initialize socket -> %ld", ret));
	}
	
	return 0;
}
