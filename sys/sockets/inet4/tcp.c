/*
 *	Transmission Control Protocol, see RFCs 793, 1122 for details.
 *
 *	04/01/94, kay roemer.
 *	07/01/99, land bug hack by Mario Becroft
 */

# include "tcp.h"

# include "icmp.h"
# include "if.h"
# include "in.h"
# include "inet.h"
# include "inetutil.h"
# include "ip.h"
# include "tcpin.h"
# include "tcpout.h"
# include "tcpsig.h"
# include "tcputil.h"

# include "sockutil.h"
# include "util.h"

# include <mint/dcntl.h>
# include <mint/signal.h>


# define TH(b)		((struct tcp_dgram *)(b)->dstart)
# define SEGLEN(b)	(TH(b)->urgptr)
# define SEQ1ST(b)	(TH(b)->seq)
# define SEQNXT(b)	(SEQ1ST(b) + SEGLEN(b))


static long	tcp_attach	(struct in_data *);
static long	tcp_abort	(struct in_data *, short);
static long	tcp_detach	(struct in_data *, short);
static long	tcp_connect	(struct in_data *, struct sockaddr_in *, short, short);
static long	tcp_listen	(struct in_data *);
static long	tcp_accept	(struct in_data *, struct in_data *, short);
static long	tcp_ioctl	(struct in_data *, short, void *);
static long	tcp_select	(struct in_data *, short, long);
static long	tcp_send	(struct in_data *, struct iovec *, short, short, short, struct sockaddr_in *, short);
static long	tcp_recv	(struct in_data *, struct iovec *, short, short, short, struct sockaddr_in *, short *);
static long	tcp_shutdown	(struct in_data *, short);
static long	tcp_setsockopt	(struct in_data *, short, short, char *, long);
static long	tcp_getsockopt	(struct in_data *, short, short, char *, long *);

static long	tcp_error	(short, short, BUF *, ulong, ulong);
static long	tcp_input	(struct netif *, BUF *, ulong, ulong);

static void	tcp_dropsegs	(struct tcb *);
static long	tcp_canreadurg	(struct in_data *, long *);
static long	tcp_canaccept	(struct in_data *);

struct in_proto tcp_proto =
{
	proto:		IPPROTO_TCP,
	flags:		0,
	next:		NULL,
	soops:		{
				attach:		tcp_attach,
				abort:		tcp_abort,
				detach:		tcp_detach,
				connect:	tcp_connect,
				listen:		tcp_listen,
				accept:		tcp_accept,
				ioctl:		tcp_ioctl,
				select:		tcp_select,
				send:		tcp_send,
				recv:		tcp_recv,
				shutdown:	tcp_shutdown,
				setsockopt:	tcp_setsockopt,
				getsockopt:	tcp_getsockopt
			},
	ipops:		{
				proto:		IPPROTO_TCP,
				next:		NULL,
				error:		tcp_error,
				input:		tcp_input
			},
	datas:		NULL
};


void
tcp_init (void)
{
	in_proto_register (IPPROTO_TCP, &tcp_proto);
}


static long
tcp_attach (struct in_data *data)
{
	struct tcb *tcb;
	
	tcb = tcb_alloc ();
	if (!tcb)
	{
		DEBUG (("tcp_attach: cannot alloc tcb"));
		return ENOMEM;
	}
	
	data->pcb = tcb;
	tcb->data = data;
	
	return 0;
}

static long
tcp_abort (struct in_data *data, short ostate)
{
	if (ostate == SS_ISCONNECTING)
	{
		DEBUG (("tcp_abort: freeing connecting socket"));
		so_free (data->sock);
	}
	
	return 0;
}

static long
tcp_detach (struct in_data *data, short wait)
{
	struct tcb *tcb = data->pcb;
	BUF *b;
	
	switch (tcb->state)
	{
		case TCBS_CLOSED:
		case TCBS_LISTEN:
		case TCBS_SYNSENT:
			tcb_free (tcb);
			data->pcb = 0;
			return in_data_destroy (data, wait);
		
		case TCBS_SYNRCVD:
		case TCBS_ESTABLISHED:
			tcb->state = TCBS_FINWAIT1;
			tcp_output (tcb, 0, 0, 0, 0, TCPF_FIN|TCPF_ACK);
			data->sock = 0;
			break;
		
		case TCBS_CLOSEWAIT:
			tcb->state = TCBS_LASTACK;
			tcp_output (tcb, 0, 0, 0, 0, TCPF_FIN|TCPF_ACK);
			data->sock = 0;
			break;
		
		default:
			data->sock = 0;
			break;
	}
	
	tcb->seq_read = tcb->seq_uread = tcb->rcv_nxt;
	for (b = data->rcv.qfirst; b; b = b->next)
		TH(b)->flags |= TCPF_FREEME;
	tcp_dropsegs (tcb);
	
	return 0;
}

static long
tcp_connect (struct in_data *data, struct sockaddr_in *addr, short addrlen, short nonblock)
{
	struct in_data *data2;
	struct tcb *tcb = data->pcb;
	ulong laddr, faddr;
	
	if (tcb->state != TCBS_CLOSED)
	{
		DEBUG (("tcp_connect: already connected"));
		return EISCONN;
	}
	
	if (addr->sin_port == 0)
	{
		DEBUG (("tcp_connect: port == 0"));
		return EADDRNOTAVAIL;
	}
	
	faddr = ip_dst_addr (addr->sin_addr.s_addr);
	if (ip_is_brdcst_addr (faddr))
	{
		DEBUG (("tcp_connect: connect to broadcast addr ?!"));
		return EINVAL;
	}
	
	laddr = ip_local_addr (faddr);
	if (laddr == INADDR_ANY)
	{
		DEBUG (("tcp_connect: no route to destination 0x%lx", faddr));
		return ENETUNREACH;
	}
	
	data->src.addr = laddr;
	data2 = in_data_lookup (data->proto->datas,
		data->src.addr, data->src.port,
		faddr, addr->sin_port);
	if (data2 && data2->flags & IN_ISCONNECTED)
	{
		DEBUG (("tcp_connect: duplicate association"));
		return EADDRINUSE;
	}
	
	data->dst.addr = faddr;
	data->dst.port = addr->sin_port;
	data->flags |= IN_ISCONNECTED;
	data->sock->state = SS_ISCONNECTING;
	
	tcb->snd_wnd = TCP_MSS;	/* enough for SYN,FIN */
	tcb->snd_isn =
	tcb->snd_una =
	tcb->snd_wndack =
	tcb->snd_nxt =
	tcb->snd_max =
	tcb->snd_urg =
	tcb->seq_write = tcp_isn ();
	
	tcb->rcv_mss = tcp_mss (tcb, data->dst.addr, TCP_MAXMSS);
	
	tcp_output (tcb, 0, 0, 0, 0, TCPF_SYN);
	tcb->state = TCBS_SYNSENT;
	
	if (nonblock)
		return EINPROGRESS;
	
	while (tcb->state == TCBS_SYNSENT || tcb->state == TCBS_SYNRCVD)
	{
		if (isleep (IO_Q, (long)data->sock))
		{
			DEBUG (("tcp_connect: interrupted"));
			return EINTR;
		}
	}
	
	if (tcb->state != TCBS_ESTABLISHED && data->err)
	{
		long r = data->err;
		DEBUG (("tcp_connect: connecting failed"));
		data->err = 0;
		return r;
	}
	
	return 0;
}

static long
tcp_listen (struct in_data *data)
{
	struct tcb *tcb = data->pcb;
	
	if (data->flags & IN_ISCONNECTED)
	{
		DEBUG (("tcp_listen: connected sockets cannot listen"));
		return EINVAL;
	}
	if (tcb->state > TCBS_LISTEN)
	{
		DEBUG (("tcp_listen: tcb already connected"));
		return EALREADY;
	}
	tcb->state = TCBS_LISTEN;
	return 0;
}

static long
tcp_accept (struct in_data *data, struct in_data *newdata, short nonblock)
{
	struct socket *so, **prev, *newso;
	struct tcb *cltcb, *tcb = data->pcb;
	
	while (1)
	{
		if (tcb->state != TCBS_LISTEN)
		{
			DEBUG (("tcp_accept: tcb not listening"));
			return EINVAL;
		}
		prev = &data->sock->iconn_q;
		for (so = *prev; so; prev = &so->next, so = *prev)
		{
			cltcb = ((struct in_data *)so->data)->pcb;
			if (cltcb->state != TCBS_SYNRCVD)
			{
				*prev = so->next;
				goto found;
			}
		}
		
		if (nonblock)
		{
			DEBUG (("tcp_accept: EAGAIN"));
			return EAGAIN;
		}
		
		if (isleep (IO_Q, (long)data->sock))
		{
			DEBUG (("tcp_accept: interrupted"));
			return EINTR;
		}
		if (data->err)
		{
			long r = data->err;
			data->err = 0;
			return r;
		}
	}
found:
	newso = newdata->sock;
	tcp_detach (newdata, 0);
	newdata = so->data;
	kfree (so);
	newdata->sock = newso;
	newso->data = newdata;
	newso->conn = 0;
	newso->state = SS_ISCONNECTED;
	
	return 0;
}

static long
tcp_ioctl (struct in_data *data, short cmd, void *buf)
{
	struct tcb *tcb = data->pcb;
	struct socket *so = data->sock;
	long space, urgseq;
	
	switch (cmd)
	{
		case SIOCATMARK:
			*(long *) buf = tcp_canreadurg (data, &urgseq) &&
				SEQLE (urgseq, tcb->seq_read);
			return 0;
		
		case FIONREAD:
			if (data->err || so->flags & SO_CANTRCVMORE)
				space = NO_LIMIT;
			else if (tcb->state == TCBS_LISTEN)
			{
				*(long *)buf = tcp_canaccept (data);
				return EINVAL;
			}
			else
			{
				space = tcp_canread (data);
				if (space <= 0 && tcb->state != TCBS_ESTABLISHED)
					space = NO_LIMIT;
			}
			*(long *) buf = space;
			return 0;
		
		case FIONWRITE:
			if (data->err || so->flags & SO_CANTSNDMORE)
				space = NO_LIMIT;
			else switch (tcb->state)
			{
				case TCBS_LISTEN:
					*(long *)buf = 0;
					return EINVAL;
				
				case TCBS_ESTABLISHED:
				case TCBS_CLOSEWAIT:
					space = tcp_canwrite (data);
					break;
				
				default:
					space = NO_LIMIT;
					break;
			}
			*(long *)buf = space;
			return 0;
		
		case FIOEXCEPT:
			if (data->err || tcp_canreadurg (data, &urgseq)
				|| SEQGT (tcb->rcv_urg, tcb->rcv_nxt))
				space = 1;
			else
				space = 0;
			*(long *) buf = space;
			return 0;
	}
	
	return ENOSYS;
}

static long
tcp_select (struct in_data *data, short mode, long proc)
{
	struct socket *so = data->sock;
	struct tcb *tcb = data->pcb;
	long urgseq;
	
	switch (mode)
	{
		case O_RDONLY:
			if (data->err || so->flags & SO_CANTRCVMORE)
				return 1;
			
			if (tcb->state == TCBS_LISTEN)
				return tcp_canaccept (data)
					? 1 : so_rselect (so, proc);
			
			if (tcp_canread (data) > 0)
				return 1;
		
			if (tcb->state != TCBS_ESTABLISHED)
				return 1;
			
			return so_rselect (so, proc);
			
		case O_WRONLY:
			switch (tcb->state)
			{
				case TCBS_SYNSENT:
				case TCBS_SYNRCVD:
					return data->err ? 1 : so_wselect (so, proc);
				
				case TCBS_ESTABLISHED:
				case TCBS_CLOSEWAIT:
					if (data->err || so->flags & SO_CANTSNDMORE)
						return 1;
					return tcp_canwrite (data) ? 1 : so_wselect (so, proc);
				default:
					return 1;
			}
		
		case O_RDWR:
			if (data->err || tcp_canreadurg (data, &urgseq)
			    || SEQGT (tcb->rcv_urg, tcb->rcv_nxt))
				return 1;
			else
				return so_xselect (so, proc);
	}
	
	DEBUG (("tcp_select: select called with invalid mode"));
	return 0;
}

# define CONNECTING(tcb) \
	((tcb)->state == TCBS_SYNSENT || (tcb)->state == TCBS_SYNRCVD)
# define CONNECTED(tcb) \
	((tcb)->state == TCBS_ESTABLISHED || (tcb)->state == TCBS_CLOSEWAIT)

static long
tcp_send (struct in_data *data, struct iovec *iov, short niov, short nonblock,
		short flags, struct sockaddr_in *addr, short addrlen)
{
	struct tcb *tcb = data->pcb;
	long size, offset, avail, r, seq_write = tcb->seq_write;
	
	offset = 0;
	size = iov_size (iov, niov);
	if (size == 0)
		return 0;
	
	if (size < 0)
	{
		DEBUG (("tcp_send: invalid iovec"));
		return EINVAL;
	}
	
	if (tcb->state <= TCBS_LISTEN)
	{
		DEBUG (("tcp_send: not connected"));
		return ENOTCONN;
	}
	
	while (offset < size)
	{
		avail = tcp_canwrite (data);
		while ((CONNECTED (tcb) && avail <= 0) || CONNECTING (tcb))
		{
			if (nonblock)
			{
				if (offset == 0)
				{
					DEBUG (("tcp_send: EAGAIN"));
					offset = EAGAIN;
				}
				
				return offset;
			}
			
			if (isleep (IO_Q, (long)data->sock))
			{
				DEBUG (("tcp_send: interrupted"));
				return offset ? offset : EINTR;
			}
			
			if (data->err)
			{
				r = data->err;
				data->err = 0;
				return r;
			}
			
			if (data->sock && data->sock->flags & SO_CANTSNDMORE)
			{
				DEBUG (("tcp_send: shut down"));
				p_kill (p_getpid (), SIGPIPE);
				return EPIPE;
			}
			
			avail = tcp_canwrite (data);
		}
		
		if (!CONNECTED (tcb))
		{
			DEBUG (("tcp_send: broken connection"));
			p_kill (p_getpid (), SIGPIPE);
			return EPIPE;
		}
		avail = MIN (avail, size - offset);
		
		r = tcp_output (tcb, iov, niov, avail, offset,
			flags & MSG_OOB ? TCPF_URG : 0);
		
		if (r < 0)
		{
			DEBUG (("tcp_send: tcp_output() returned %ld", r));
			return SEQLT (seq_write, tcb->seq_write)
				? (tcb->seq_write - seq_write)
				: r;
		}
		
		offset += avail;
	}
	
	return offset;
}

static void
tcp_dropsegs (struct tcb *tcb)
{
	struct in_dataq *q = &tcb->data->rcv;
	BUF *b, *nextb;
	long min, win;
	
	/*
	 * 'min' is the lowest sequence number in the queue.
	 */
	min = tcb->seq_uread;
	if (SEQLT (tcb->seq_read, min))
		min = tcb->seq_read;
	
	for (b = q->qfirst; b && TH(b)->flags & TCPF_FREEME; b = nextb)
	{
		min = SEQNXT (b);
		if ((nextb = b->next) != 0)
		{
			q->curdatalen -= SEQ1ST (nextb) - SEQ1ST (b);
			nextb->prev = 0;
		}
		else
		{
			q->curdatalen = 0;
			q->qlast = 0;
		}
		buf_deref (b, BUF_NORMAL);
	}
	q->qfirst = b;
	
	/*
	 * Drag the read and urgent pointers and receive urgent pointer
	 * along with the lowest sequence number in the queue.
	 */
	if (SEQLT (tcb->seq_read, min))
		tcb->seq_read = min;
	if (SEQLT (tcb->seq_uread, min))
		tcb->seq_uread = min;
	if (SEQLT (tcb->rcv_urg, min))
		tcb->rcv_urg = min;
	
	win = tcp_rcvwnd (tcb, 0) - (tcb->rcv_wnd - tcb->rcv_nxt);
	if (win >= 2*tcb->rcv_mss || 2*win >= tcb->data->rcv.maxdatalen)
	{
		/*
		 * Send window update if our window opens up by >= 2 segments
		 * or by >= half of our receive window.
		 */
		tcp_output (tcb, 0, 0, 0, 0, TCPF_ACK);
	}
}

static long
tcp_recv (struct in_data *data, struct iovec *iov, short niov, short nonblock,
		short flags, struct sockaddr_in *addr, short *addrlen)
{
	struct tcb *tcb = data->pcb;
	long size, offset, copied, cando, readnxt, *seq;
	struct in_dataq *q;
	short urgflag, oobinline, mask;
	BUF *b;
	
	size = iov_size (iov, niov);
	if (size == 0)
		return 0;
	
	if (size < 0)
	{
		DEBUG (("tcp_recv: invalid iovec"));
		return EINVAL;
	}
	
	if (tcb->state <= TCBS_SYNSENT)
	{
		DEBUG (("tcp_recv: not connected"));
		return ENOTCONN;
	}
	
	if (flags & MSG_OOB)
	{
		seq = &tcb->seq_uread;
		urgflag = TCPF_URG;
	}
	else
	{
		seq = &tcb->seq_read;
		urgflag = 0;
	}
	
	if (data->flags & IN_OOBINLINE)
	{
		mask = TCPF_FREEME;
		oobinline = 1;
	}
	else
	{
		mask = TCPF_FREEME|TCPF_URG;
		oobinline = 0;
	}
	
	for (q = &data->rcv;; )
	{
		readnxt = *seq;
		b = q->qfirst;
		/*
		 * Search the segment `readnxt' lies in
		 */
		while (b && SEQLE (SEQNXT (b), readnxt))
			b = b->next;
		
		/*
		 * Search the next fitting segment (ie next urgent segment
		 * when reading URGENT; next nonurgent segment when reading
		 * NORMAL and not in OOBINLINE mode; next unread segment
		 * when reading NORMAL and OOBINLINE).
		 */
		while (b && (TH(b)->flags ^ urgflag) & mask &&
			SEQLE (SEQ1ST (b), readnxt))
		{
			readnxt = SEQNXT (b);
			b = b->next;
		}
		if (b && SEQLE (SEQ1ST (b), readnxt))
			break;
		
		if (tcb->state != TCBS_ESTABLISHED)
		{
			DEBUG (("tcp_recv: connection closed -- EOF"));
			return 0;
		}
		
		if (urgflag && SEQGT (tcb->rcv_urg, tcb->rcv_nxt))
		{
			DEBUG (("tcp_recv: urgent data not yet, EAGAIN"));
			return EAGAIN;
		}
		
		if (nonblock)
		{
			DEBUG (("tcp_recv: EAGAIN"));
			return EAGAIN;
		}
		
		if (isleep (IO_Q, (long)data->sock))
		{
			DEBUG (("tcp_recv: interrupted -> %li", EINTR));
			return EINTR;
		}
		
		if (data->err)
		{
			long r = data->err;
			data->err = 0;
			return r;
		}
		
		if (data->sock && data->sock->flags & SO_CANTRCVMORE)
		{
			DEBUG (("tcp_recv: shut down"));
			return 0;
		}
	}
	
	urgflag = TH(b)->flags & TCPF_URG;
	for (copied = offset = 0; b && size > 0; b = b->next)
	{
		/*
		 * Don't pass the boundary between nonurgent and
		 * urgent data
		 */
		if ((TH(b)->flags & TCPF_URG) != urgflag)
			break;
		
		/*
		 * Check for holes in the data stream
		 */
		if (SEQLT (readnxt, SEQ1ST (b)))
			break;
		
		/*
		 * Skip already read urgent data
		 */
		if (TH(b)->flags & TCPF_FREEME)
		{
			readnxt = SEQNXT (b);
			continue;
		}
		
		/*
		 * Skip already read urgent data (via MSG_OOB) when reading
		 * "normal" (ie without MSG_OOB) in OOB-INLINE mode.
		 */
		if (oobinline && urgflag && !(flags & MSG_OOB) &&
		    SEQLT (readnxt, tcb->seq_uread) &&
		    SEQLT (tcb->seq_uread, SEQNXT (b)))
			readnxt = tcb->seq_uread;
		
		/*
		 * Copy the datagram contents to the user buffer
		 */
		offset = readnxt - SEQ1ST (b);
		cando = MIN (size, SEGLEN (b) - offset);
		buf2iov_cpy (TCP_DATA (TH(b)) + offset,cando,iov,niov,copied);
		size -= cando;
		copied += cando;
		readnxt += cando;
		
		/*
		 * Mark the segment for deletion
		 */
		if (!(flags & MSG_PEEK) && SEQLE (SEQNXT (b), readnxt))
			TH(b)->flags |= TCPF_FREEME;
	}
	
	if (!(flags & MSG_PEEK))
	{
		*seq = readnxt;
		if (oobinline)
			tcb->seq_uread = readnxt;
		tcp_dropsegs (tcb);
	}
	
	if (addr)
	{
		struct sockaddr_in in;
		
		*addrlen = MIN (*addrlen, sizeof (in));
		in.sin_family = AF_INET;
		in.sin_addr.s_addr = data->dst.addr;
		in.sin_port = data->dst.port;
		memcpy (addr, &in, *addrlen);
	}
	
	return copied;
}

static long
tcp_shutdown (struct in_data *data, short how)
{
	struct tcb *tcb = data->pcb;
	
	if (how == 1 || how == 2) switch (tcb->state)
	{
		case TCBS_SYNRCVD:
		case TCBS_ESTABLISHED:
			tcb->state = TCBS_FINWAIT1;
			tcp_output (tcb, 0, 0, 0, 0, TCPF_FIN|TCPF_ACK);
			break;
		
		case TCBS_CLOSEWAIT:
			tcb->state = TCBS_LASTACK;
			tcp_output (tcb, 0, 0, 0, 0, TCPF_FIN|TCPF_ACK);
			break;
	}
	return 0;
}

static long
tcp_setsockopt (struct in_data *data, short level, short optname, char *optval, long optlen)
{
	struct tcb *tcb = data->pcb;
	
	if (level != IPPROTO_TCP)
		return EOPNOTSUPP;
	
	if (optlen != sizeof (long) || !optval)
		return EINVAL;
	
	switch (optname)
	{
		case TCP_NODELAY:
			if (*(long *)optval)
				tcb->flags |= TCBF_NDELAY;
			else
				tcb->flags &= ~TCBF_NDELAY;
			return 0;
	}
	
	return EOPNOTSUPP;
}

static long
tcp_getsockopt (struct in_data *data, short level, short optname, char *optval, long *optlen)
{
	struct tcb *tcb = data->pcb;

	if (level != IPPROTO_TCP)
		return EOPNOTSUPP;
	
	if (!optval || !optlen || *optlen < sizeof (long))
		return EINVAL;
	
	switch (optname)
	{
		case TCP_NODELAY:
			*(long *)optval = !!(tcb->flags & TCBF_NDELAY);
			*optlen = sizeof (long);
			return 0;
	}
	
	return EOPNOTSUPP;
}

static long
tcp_input (struct netif *iface, BUF *buf, ulong saddr, ulong daddr)
{
	struct tcp_dgram *tcph = (struct tcp_dgram *) IP_DATA (buf);
	struct in_data *data;
	struct tcb *tcb;
	long pktlen;
	
	pktlen = (long)buf->dend - (long)tcph;
	if (pktlen < TCP_MINLEN)
	{
		DEBUG (("tcp_input: invalid packet length"));
		buf_deref (buf, BUF_NORMAL);
		return 0;
	}
	
	if (tcp_checksum (tcph, pktlen, saddr, daddr))
	{
		DEBUG (("tcp_input: bad checksum"));
		buf_deref (buf, BUF_NORMAL);
		return 0;
	}
	
	data = in_data_lookup (tcp_proto.datas, saddr, tcph->srcport,
		daddr, tcph->dstport);
	if (!data)
	{
		DEBUG (("tcp_input: destination port %d doesn't exist",
			tcph->dstport));
		tcp_sndrst (buf);
		return -1;
	}
	tcb = (struct tcb *)data->pcb;
	
	/* 07/01/99 MB land bug hack (the real reason for the crash is yet unknown) */
	if (tcb->state == TCBS_LISTEN && saddr == daddr && tcph->srcport == tcph->dstport)
	{
		DEBUG (("tcp_input: Mario Becroft's land bug hack"));
		buf_deref (buf, BUF_NORMAL);
		return 0;
	}
	/* 07/01/99 MB end */
	
	/*
	 * update time of last receive on this connection
	 */
	tcb->last_recv = GETTIME ();
	if (tcph->flags & TCPF_URG)
		tcp_rcvurg (tcb, buf);
	if (tcp_valid (tcb, buf))
	{
		if (tcph->hdrlen > 5 && tcb->state >= TCBS_SYNRCVD)
			tcp_options (tcb, tcph);
		(*tcb_state[tcb->state]) (tcb, buf);
		return 0;
	}
	DEBUG (("tcp_input: port %d:state %d: not acceptable input segment",
		data->src.port, tcb->state));
	KAYDEBUG (("tcp_input: w %ld, s %ld, snxt %ld, wlast %ld",
		tcb->rcv_nxt, tcph->seq,
		tcph->seq + tcp_seglen (buf, tcph),
		tcb->rcv_nxt + tcp_rcvwnd (tcb, 0)));
	tcp_sndack (tcb, buf);
	buf_deref (buf, BUF_NORMAL);
	
	return 0;
}

static long
tcp_error (short type, short code, BUF *buf, ulong saddr, ulong daddr)
{
	struct tcp_dgram *tcph = (struct tcp_dgram *) IP_DATA (buf);
	struct in_data *data;
	struct tcb *tcb;
	
	data = in_data_lookup (tcp_proto.datas, daddr, tcph->dstport,
		saddr, tcph->srcport);
	if (!data)
	{
		DEBUG (("tcp_error: no local port %x", tcph->srcport));
		buf_deref (buf, BUF_NORMAL);
		return -1;
	}
	tcb = data->pcb;
	switch (type)
	{
		case ICMPT_SRCQ:
			/*
			 * Source quench. Cause slow start.
			 */
			if (tcb->snd_cwnd > tcb->snd_thresh)
				tcb->snd_thresh = tcb->snd_cwnd;
			tcb->snd_thresh = MIN (tcb->snd_thresh, tcb->snd_wnd) >> 1;
			if (tcb->snd_thresh < 2*tcb->snd_mss)
				tcb->snd_thresh = 2*tcb->snd_mss;
			tcb->snd_cwnd = tcb->snd_mss;
			break;
		
		case ICMPT_DSTUR:
			if (code >= ICMPC_PROTOUR && code <= ICMPC_FNDF)
			{
				/*
				 * Hard error. Abort connection.
				 */
				tcb_reset (tcb, icmp_errno (type, code));
				tcb->state = TCBS_CLOSED;
				/*
				 * Free tcb if socket has gone. Must be careful to
				 * not touch the tcb after the call to tcb_delete ().
				 */
				tcb_delete (tcb);
				break;
			}
		/* FALLTHROUGH */
		
		case ICMPT_TIMEX:
		case ICMPT_PARAMP:
			/*
			 * Soft error. Notify but don't abort.
			 */
			if (data->sock)
			{
				data->err = icmp_errno (type, code);	
				wake (IO_Q, (long)data->sock);
				so_wakersel (data->sock);
				so_wakewsel (data->sock);
			}
			break;
	}
	buf_deref (buf, BUF_NORMAL);
	return 0;
}

/*
 * Some helper routines for ioctl(), select(), read() and write().
 */

static long
tcp_canreadurg (struct in_data *data, long *urgseq)
{
	BUF *b;
	
	b = data->rcv.qfirst;
	while (b && (TH(b)->flags & (TCPF_URG|TCPF_FREEME)) != TCPF_URG)
		b = b->next;
	
	if (!b) return 0;
	
	*urgseq = SEQ1ST (b);
	return 1;
}

/*
 * Return nonzero if there is a client waiting to be accepted
 */
static long
tcp_canaccept (struct in_data *data)
{
	struct tcb *tcb = data->pcb;
	struct socket *so;
	
	if (tcb->state != TCBS_LISTEN)
		return 0;
	
	for (so = data->sock->iconn_q; so; so = so->next)
	{
		struct tcb *ntcb = ((struct in_data *)so->data)->pcb;
		if (ntcb->state != TCBS_SYNRCVD)
			return 1;
	}
	
	return 0;
}

/*
 * Return the number of bytes that can be read from `data'
 */
long
tcp_canread (struct in_data *data)
{
	struct tcb *tcb = data->pcb;
	long readnxt, nbytes, cando;
	short urgflag;
	BUF *b;
	
	if (tcb->state == TCBS_LISTEN)
		return 0;
	
	readnxt = tcb->seq_read;
	b = data->rcv.qfirst;
	while (b && SEQLE (SEQNXT (b), readnxt))
		b = b->next;
	
	if (!(data->flags & IN_OOBINLINE))
	{
		while (b && TH(b)->flags & (TCPF_URG|TCPF_FREEME) &&
			SEQLE (SEQ1ST (b), readnxt))
		{
			readnxt = SEQNXT (b);
			b = b->next;
		}
	}
	if (!b || SEQGT (SEQ1ST (b), readnxt))
		return 0;
	
	urgflag = TH(b)->flags & TCPF_URG;
	for (nbytes = 0; b; b = b->next)
	{
		if ((TH(b)->flags & TCPF_URG) != urgflag)
			break;
		
		if (SEQLT (readnxt, SEQ1ST (b)))
			break;
		
		if (TH(b)->flags & TCPF_FREEME)
		{
			readnxt = SEQNXT (b);
			continue;
		}
		
		if (urgflag && data->flags & IN_OOBINLINE &&
		    SEQLT (readnxt, tcb->seq_uread) &&
		    SEQLT (tcb->seq_uread, SEQNXT (b)))
			readnxt = tcb->seq_uread;
		
		cando = SEGLEN (b) - (readnxt - SEQ1ST (b));
		nbytes += cando;
		readnxt += cando;
	}
	
	return nbytes;
}

/*
 * Return the number of bytes that can be written
 */
long
tcp_canwrite (struct in_data *data)
{
	struct tcb *tcb = data->pcb;
	long space;
	
	if (!CONNECTED (tcb))
		return 0;
	
	space = data->snd.maxdatalen - data->snd.curdatalen;
	if (space < tcb->snd_mss && 2*tcb->snd_mss <= data->snd.maxdatalen)
		space = 0;
	
	return MAX (space, 0);
}
