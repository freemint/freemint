/*
 *	This file contains the functions needed for processing incoming
 *	TCP segments.
 *
 *	04/13/94, Kay Roemer.
 */

# include "tcpin.h"

# include "inetutil.h"
# include "ip.h"
# include "tcpout.h"
# include "tcpsig.h"
# include "tcputil.h"

# include "net.h"
# include "sockutil.h"
# include "util.h"

# include <mint/signal.h>


static void	tcbs_closed		(struct tcb *, BUF *);
static void	tcbs_listen		(struct tcb *, BUF *);
static void	tcbs_synsent		(struct tcb *, BUF *);
static void	tcbs_synrcvd		(struct tcb *, BUF *);
static void	tcbs_established	(struct tcb *, BUF *);
static void	tcbs_finwait1		(struct tcb *, BUF *);
static void	tcbs_finwait2		(struct tcb *, BUF *);
static void	tcbs_closewait		(struct tcb *, BUF *);
static void	tcbs_lastack		(struct tcb *, BUF *);
static void	tcbs_closing		(struct tcb *, BUF *);
static void	tcbs_timewait		(struct tcb *, BUF *);

static long	tcp_rcvdata		(struct tcb *, BUF *);
static long	tcp_addseg		(struct in_dataq *, BUF *);
static short	tcp_sndwnd		(struct tcb *, struct tcp_dgram *);
static long	tcp_ack			(struct tcb *, BUF *, short);
static void	tcp_delack		(struct tcb *);
static void	dodelack		(long);
static short	check_syn		(long, long, long, short, short);


void (*tcb_state[])(struct tcb *, BUF *) =
{
	tcbs_closed,
	tcbs_listen,
	tcbs_synsent,
	tcbs_synrcvd,
	tcbs_established,
	tcbs_finwait1,
	tcbs_finwait2,
	tcbs_closewait,
	tcbs_lastack,
	tcbs_closing,
	tcbs_timewait
};

/*
 * Input finite state machine.
 */

static void
tcbs_closed (struct tcb *tcb, BUF *buf)
{
	tcp_sndrst (buf);
	buf_deref (buf, BUF_NORMAL);
}

static void
tcbs_listen (struct tcb *tcb, BUF *buf)
{
	struct tcp_dgram *tcph = (struct tcp_dgram *) IP_DATA (buf);
	struct socket *so;
	struct in_data *data;
	struct tcb *ntcb;
	long r;
	
	if ((tcph->flags & (TCPF_RST|TCPF_SYN)) != TCPF_SYN)
	{
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	if (tcph->flags & TCPF_ACK)
	{
		tcp_sndrst (buf);
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	if (check_syn (tcph->seq, IP_SADDR (buf), IP_DADDR (buf),
		tcph->srcport, tcph->dstport))
	{
		DEBUG (("tcbs_listen: duplicate syn"));
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	if (ip_is_brdcst_addr (IP_DADDR (buf)))
	{
		DEBUG (("tcbs_listen: syn to broadcast address"));
		buf_deref (buf, BUF_NORMAL);
		return;
	}		
	
	ntcb = tcb_alloc ();
	data = in_data_create ();
	so = so_create ();
	if (!tcb || !data || !so)
	{
		DEBUG (("tcp_listen: cannot create new socket, memory low"));
		if (ntcb) tcb_free (ntcb);
		if (data) in_data_destroy (data, 0);
		if (so) kfree (so);
	}
	
	/*
	 * Init the socket
	 */
	so->state = SS_ISUNCONNECTED;
	so->data = data;
	so->ops = tcb->data->sock->ops;
	so->type = tcb->data->sock->type;
	
	/*
	 * Init the inet data
	 */
	data->protonum = tcb->data->protonum;
	data->proto = tcb->data->proto;
	data->sock = so;
	data->pcb = ntcb;
	data->src.addr = IP_DADDR (buf);
	data->src.port = tcph->dstport;
	data->dst.addr = IP_SADDR (buf);
	data->dst.port = tcph->srcport;
	data->linger = tcb->data->linger;
	data->flags = IN_ISBOUND|IN_ISCONNECTED;
	data->flags |= tcb->data->flags & (IN_KEEPALIVE|IN_OOBINLINE|
		IN_CHECKSUM|IN_DONTROUTE|IN_BROADCAST|IN_LINGER);
	
	/*
	 * Init the TCB
	 */
	ntcb->data = data;
	ntcb->flags |= TCBF_PASSIVE;
	ntcb->state = TCBS_SYNRCVD;
	ntcb->snd_isn =
	ntcb->snd_una =
	ntcb->snd_wndack =
	ntcb->snd_nxt =
	ntcb->snd_max =
	ntcb->snd_urg =
	ntcb->seq_write = tcp_isn ();
	
	ntcb->snd_wnd = TCP_MSS; /* enough for SYN,FIN */
	
	ntcb->rcv_isn =
	ntcb->rcv_nxt =
	ntcb->rcv_wnd =
	ntcb->rcv_urg = tcph->seq;
	ntcb->seq_read =
	ntcb->seq_uread = tcph->seq + 1; /* SEQ of next byte to read() */
	
	ntcb->rcv_mss = tcp_mss (ntcb, data->dst.addr, TCP_MAXMSS);
	
	in_data_put (data);
	
	r = so_connect (tcb->data->sock, so, tcb->data->backlog, 1, 0);
	if (r != EINPROGRESS)
	{
		DEBUG (("tcp_listen: cannot connect to server"));
		ntcb->state = TCBS_CLOSED;
		so_release (so);
		kfree (so);
		tcp_sndrst (buf);
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	r = tcp_options (ntcb, tcph);
	ntcb->snd_mss =
	ntcb->snd_cwnd = tcp_mss (ntcb, data->dst.addr, r);
	if (ntcb->snd_thresh < 2*ntcb->snd_cwnd);
		ntcb->snd_thresh = 2*ntcb->snd_cwnd;
	
	tcp_rcvdata (ntcb, buf);
	tcp_output (ntcb, 0, 0, 0, 0, TCPF_SYN|TCPF_ACK);
}

static void
tcbs_synsent (struct tcb *tcb, BUF *buf)
{
	struct tcp_dgram *tcph = (struct tcp_dgram *) IP_DATA (buf);
	long r;
	
	if (tcph->flags & TCPF_ACK
		&& (SEQLE (tcph->ack, tcb->snd_isn)
			|| SEQGT (tcph->ack, tcb->snd_max)))
	{
		tcp_sndrst (buf);
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	if (tcph->flags & TCPF_RST)
	{
		if (tcph->flags & TCPF_ACK)
		{
			tcb_reset (tcb, ECONNREFUSED);
			tcb->state = TCBS_CLOSED;
			tcb->data->sock->state = SS_ISDISCONNECTING;
		}
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	if (!(tcph->flags & TCPF_SYN))
	{
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	if (ip_is_brdcst_addr (IP_DADDR (buf)))
	{
		DEBUG (("tcbs_synsent: syn to broadcast address"));
		buf_deref (buf, BUF_NORMAL);
		return;
	}		
	
	tcb->rcv_isn =
	tcb->rcv_nxt =
	tcb->rcv_wnd =
	tcb->rcv_urg = tcph->seq;
	tcb->seq_read =
	tcb->seq_uread = tcph->seq + 1; /* SEQ of next byte to read() */
	
	tcp_ack (tcb, buf, 0);
	
	if (SEQGT (tcb->snd_una, tcb->snd_isn))
	{
		/* SYN is acked */
		
		KAYDEBUG (("tcp port %d: SYNSENT -> ESTABLISHED",
				tcb->data->src.port));
		
		tcb->state = TCBS_ESTABLISHED;
		tcb->snd_wnd = tcph->window;
		tcb->snd_wndseq = tcph->seq;
		tcb->snd_wndack = tcph->ack;
		
		tcb->data->sock->state = SS_ISCONNECTED;
		so_wakersel (tcb->data->sock);
		so_wakewsel (tcb->data->sock);
		wake (IO_Q, (long)tcb->data->sock);
	}
	else
	{
		/* SYN not acked */
		
		KAYDEBUG (("tcp port %d: SYNSENT -> SYNRCVD",
				tcb->data->src.port));
		
		tcb->state = TCBS_SYNRCVD;
	}
	
	r = tcp_options (tcb, tcph);
	tcb->snd_mss =
	tcb->snd_cwnd = tcp_mss (tcb, tcb->data->dst.addr, r);
	if (tcb->snd_thresh < 2*tcb->snd_cwnd);
		tcb->snd_thresh = 2*tcb->snd_cwnd;
	
	tcp_rcvdata (tcb, buf);
	tcp_output (tcb, 0, 0, 0, 0, TCPF_ACK);
	
	return;
}

static void
tcbs_synrcvd (struct tcb *tcb, BUF *buf)
{
	struct tcp_dgram *tcph = (struct tcp_dgram *) IP_DATA (buf);
	
	if (tcph->flags & TCPF_RST)
	{
		if (tcb->flags & TCBF_PASSIVE)
		{
			struct socket *so = tcb->data->sock;
			tcb->state = TCBS_CLOSED;
			so_release (so);
			kfree (so);
		}
		else
		{
			tcb_reset (tcb, ECONNRESET);
			tcb->state = TCBS_CLOSED;
			tcb->data->sock->state = SS_ISDISCONNECTING;
		}
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	if (tcph->flags & TCPF_SYN)
	{
		tcp_sndrst (buf);
		if (tcb->flags & TCBF_PASSIVE)
		{
			struct socket *so = tcb->data->sock;
			tcb->state = TCBS_CLOSED;
			so_release (so);
			kfree (so);
		}
		else
		{
			tcb_reset (tcb, ECONNRESET);
			tcb->state = TCBS_CLOSED;
			tcb->data->sock->state = SS_ISDISCONNECTING;
		}
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	if (tcp_ack (tcb, buf, 0))
	{
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	KAYDEBUG (("tcp port %d: SYNRCVD -> ESTABLISHED",
			tcb->data->src.port));
	
	tcb->state = TCBS_ESTABLISHED;
	tcb->snd_wnd = tcph->window;
	tcb->snd_wndseq = tcph->seq;
	tcb->snd_wndack = tcph->ack;
	
	tcp_rcvdata (tcb, buf);
	if (tcb->flags & TCBF_PASSIVE)
	{
		so_wakersel (tcb->data->sock->conn);
		wake (IO_Q, (long)tcb->data->sock->conn);
	}
	else
	{
		tcb->data->sock->state = SS_ISCONNECTED;
		so_wakewsel (tcb->data->sock);
		wake (IO_Q, (long)tcb->data->sock);
	}
	
	if (tcp_finished (tcb))
	{
		KAYDEBUG (("tcp port %d: ESTABLISHED -> CLOSEWAIT",
				tcb->data->src.port));
		
		tcb->state = TCBS_CLOSEWAIT;
		tcb->data->sock->state = SS_ISDISCONNECTING;
		if (!(tcb->flags & TCBF_PASSIVE))
			so_wakersel (tcb->data->sock);
	}
}

static void
tcbs_established (struct tcb *tcb, BUF *buf)
{
	struct tcp_dgram *tcph = (struct tcp_dgram *)IP_DATA (buf);
	
	if (tcph->flags & TCPF_RST)
	{
		tcb_reset (tcb, ECONNRESET);
		tcb->state = TCBS_CLOSED;
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	if (tcph->flags & TCPF_SYN)
	{
		tcp_sndrst (buf);
		tcb_reset (tcb, ECONNRESET);
		tcb->state = TCBS_CLOSED;
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	if (tcp_ack (tcb, buf, 1))
	{
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	tcp_rcvdata (tcb, buf);
	if (tcp_finished (tcb))
	{
		KAYDEBUG (("tcp port %d: ESTABLISHED -> CLOSEWAIT",
				tcb->data->src.port));
		
		tcb->state = TCBS_CLOSEWAIT;
		so_wakersel (tcb->data->sock);
		wake (IO_Q, (long)tcb->data->sock);
	}
	
	return;
}

static void
tcbs_finwait1 (struct tcb *tcb, BUF *buf)
{
	struct tcp_dgram *tcph = (struct tcp_dgram *) IP_DATA (buf);
	
	if (tcph->flags & (TCPF_RST|TCPF_SYN)
		|| (!tcb->data->sock && tcp_datalen (buf, tcph)))
	{
		tcp_sndrst (buf);
		tcb_reset (tcb, ECONNRESET);
		buf_deref (buf, BUF_NORMAL);
		tcb->state = TCBS_CLOSED;
		tcb_delete (tcb);
		return;
	}
	
	if (tcp_ack (tcb, buf, 1))
	{
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	tcp_rcvdata (tcb, buf);
	
	if (tcp_finished (tcb))
	{
		if (SEQGT (tcb->seq_write, tcb->snd_una))
		{
			/*
			 * FIN is not acked
			 */
			
			KAYDEBUG (("tcp port %d: FINWAIT1 -> CLOSING",
					tcb->data->src.port));
			
			tcb->state = TCBS_CLOSING;
		}
		else
		{
			KAYDEBUG (("tcp port %d: FINWAIT1 -> TIMEWAIT",
					tcb->data->src.port));
			
			tcb->state = TCBS_TIMEWAIT;
			tcb->data->flags |= IN_DEAD;
			tcb_wait (tcb);
		}
	}
	else if (SEQLE (tcb->seq_write, tcb->snd_una))
	{
		/*
		 * FIN is acked
		 */
		
		KAYDEBUG (("tcp port %d: FINWAIT1 -> FINWAIT2",
				tcb->data->src.port));
		
		tcb->state = TCBS_FINWAIT2;
		tcb_wait (tcb);
	}
}

static void
tcbs_finwait2 (struct tcb *tcb, BUF *buf)
{
	struct tcp_dgram *tcph = (struct tcp_dgram *) IP_DATA (buf);
	
	if (tcph->flags & (TCPF_RST|TCPF_SYN)
		|| (!tcb->data->sock && tcp_datalen (buf, tcph)))
	{
		tcp_sndrst (buf);
		buf_deref (buf, BUF_NORMAL);
		tcb_reset (tcb, ECONNRESET);
		tcb->state = TCBS_CLOSED;
		tcb_delete (tcb);
		return;
	}
	
	if (tcp_ack (tcb, buf, 0))
	{
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	tcp_rcvdata (tcb, buf);
	
	if (tcp_finished (tcb))
	{
		KAYDEBUG (("tcp port %d: FINWAIT2 -> TIMEWAIT",
				tcb->data->src.port));
		
		tcb->state = TCBS_TIMEWAIT;
		tcb->data->flags |= IN_DEAD;
		tcb_wait (tcb);
	}
}

static void
tcbs_closewait (struct tcb *tcb, BUF *buf)
{
	struct tcp_dgram *tcph = (struct tcp_dgram *) IP_DATA (buf);
	
	if (tcph->flags & (TCPF_RST|TCPF_SYN))
	{
		tcp_sndrst (buf);
		tcb_reset (tcb, ECONNRESET);
		buf_deref (buf, BUF_NORMAL);
		tcb->state = TCBS_CLOSED;
		tcb_delete (tcb);
		return;
	}
	tcp_ack (tcb, buf, 1);
	buf_deref (buf, BUF_NORMAL);
}

static void
tcbs_lastack (struct tcb *tcb, BUF *buf)
{
	struct tcp_dgram *tcph = (struct tcp_dgram *) IP_DATA (buf);
	
	if (tcph->flags & (TCPF_RST|TCPF_SYN)
		|| (!tcb->data->sock && tcp_datalen (buf, tcph)))
	{
		tcp_sndrst (buf);
		tcb_reset (tcb, ECONNRESET);
		buf_deref (buf, BUF_NORMAL);
		tcb->state = TCBS_CLOSED;
		tcb_delete (tcb);
		return;
	}
	
	tcp_ack (tcb, buf, 0);
	
	if (SEQLE (tcb->seq_write, tcb->snd_una))
	{
		/*
		 * FIN is acked
		 */
		KAYDEBUG (("tcp port %d: LASTACK -> CLOSED",
				tcb->data->src.port));
		
		tcb->state = TCBS_CLOSED;
		tcb->data->flags |= IN_DEAD;
		tcb_delete (tcb);
	}
}

static void
tcbs_closing (struct tcb *tcb, BUF *buf)
{
	struct tcp_dgram *tcph = (struct tcp_dgram *) IP_DATA (buf);
	
	if (tcph->flags & (TCPF_RST|TCPF_SYN)
		|| (!tcb->data->sock && tcp_datalen (buf, tcph)))
	{
		tcp_sndrst (buf);
		tcb_reset (tcb, ECONNRESET);
		buf_deref (buf, BUF_NORMAL);
		tcb->state = TCBS_CLOSED;
		tcb_delete (tcb);
		return;
	}
	
	tcp_ack (tcb, buf, 0);
	
	if (SEQLE (tcb->seq_write, tcb->snd_una))
	{
		/* FIN is acked */
		KAYDEBUG (("tcp port %d: CLOSING -> TIMEWAIT",
				tcb->data->src.port));
		
		tcb->state = TCBS_TIMEWAIT;
		tcb->data->flags |= IN_DEAD;
		tcb_wait (tcb);
	}
}

/*
 * Note that in TIMEWAIT state the tcb has no associated socket.
 */
static void
tcbs_timewait (struct tcb *tcb, BUF *buf)
{
	struct tcp_dgram *tcph = (struct tcp_dgram *)IP_DATA (buf);
	
	if (tcph->flags & (TCPF_RST|TCPF_SYN)
		|| (!tcb->data->sock && tcp_datalen (buf, tcph)))
	{
		tcp_sndrst (buf);
		buf_deref (buf, BUF_NORMAL);
		tcb->state = TCBS_CLOSED;
		tcb_delete (tcb);
		return;
	}
	
	tcp_ack (tcb, buf, 0);
	tcp_rcvdata (tcb, buf);
	tcb_wait (tcb);
}

/*
 * Helper routines for the finite state machine
 */

static short
check_syn (long syn, long saddr, long daddr, short sport, short dport)
{
	struct in_data *d = tcp_proto.datas;
	struct tcb *tcb;
	
	while ((d = in_data_lookup_next (d, saddr, sport, daddr, dport, 0)))
	{
		tcb = d->pcb;
		if (d->flags & IN_DEAD && syn == tcb->rcv_isn)
			return 1;
		d = d->next;
	}
	
	return 0;
}

# define TH(b)		((struct tcp_dgram *)(b)->dstart)
# define SEQ1ST(b)	(TH(b)->seq)
# define SEQNXT(b)	(TH(b)->seq + TH(b)->urgptr)

/*
 * Schedule a delayed ack, but ack at least every other packet.
 */
INLINE void
tcp_delack (struct tcb *tcb)
{
# ifdef USE_DELAYED_ACKS
	if (!(tcb->flags & TCBF_DELACK))
	{
		tcb->flags |= TCBF_DELACK;
		event_add (&tcb->ack_evt, TCP_ACKDELAY, dodelack, (long)tcb);
		return;
	}
# endif
	tcp_output (tcb, 0, 0, 0, 0, TCPF_ACK);
}

/*
 * Delayed ack timeout function
 */
static void
dodelack (long arg)
{
	struct tcb *tcb = (struct tcb *)arg;
	
	if (tcb->flags & TCBF_DELACK)
		tcp_output (tcb, 0, 0, 0, 0, TCPF_ACK);
}

/*
 * Handle incoming data and special flags
 */
static long
tcp_rcvdata (struct tcb *tcb, BUF *buf)
{
	struct tcp_dgram *tcph;
	long nxt, onxt, datalen;
	short flags, acknow = 1;
	
	/*
	 * Strip the IP header
	 */
	buf->dstart = IP_DATA (buf);
	tcph = (struct tcp_dgram *) buf->dstart;
	flags = tcph->flags;
	
	/*
	 * Store the segments data length in urgptr
	 */
	datalen = (long)buf->dend - (long)tcph - tcph->hdrlen*4;
	tcph->urgptr = datalen;
# if 0
	DEBUG (("tcp_rcvdata::"));
	tcp_dump (buf);
# endif
	if (flags & TCPF_SYN)
	{
		tcb->rcv_nxt++;
		tcph->seq++;
	}
	
	if (flags & TCPF_FIN)
	{
		tcb->flags |= TCBF_FIN;
		tcb->seq_fin = tcph->seq + datalen;
	}
	if (flags & TCPF_PSH)
	{
		tcb->flags |= TCBF_PSH;
		tcb->seq_psh = tcph->seq + datalen;
	}
	tcph->flags &= ~(TCPF_SYN|TCPF_PSH|TCPF_FIN);
	/*
	 * Add the segment to the recv queue and wake things up
	 */
	onxt = nxt = tcb->rcv_nxt;
	if (!tcp_addseg (&tcb->data->rcv, buf))
	{
		BUF *b = buf;
		/*
		 * Compute the next sequence number beyond the continuous
		 * sequence of received bytes in `nxt'.
		 */
		acknow = (onxt != SEQ1ST (b));
		while (b && SEQLE (SEQ1ST (b), nxt))
		{
			nxt = SEQNXT (b);
			b = b->next;
		}
		if (tcb->data->sock && SEQLT (tcb->seq_read, nxt))
		{
			so_wakersel (tcb->data->sock);
			wake (IO_Q, (long)tcb->data->sock);
		}
		acknow |= (nxt - onxt != datalen);
	}
	if (tcb->flags & TCBF_FIN && SEQEQ (tcb->seq_fin, nxt))
	{
		acknow = 1;
		nxt++;
		/*
		 * Do FIN processing
		 */
	}
	if (tcb->flags & TCBF_PSH && SEQLE (tcb->seq_psh, nxt))
	{
		/*
		 * Do PUSH processing
		 */
	}
	tcb->rcv_nxt = nxt;
	if (datalen > 0 || flags & TCPF_FIN)
	{
		if (tcb->state == TCBS_ESTABLISHED && !acknow)
			tcp_delack (tcb);
		else
			tcp_output (tcb, 0, 0, 0, 0, TCPF_ACK);
	}
	return 0;
}

/*
 * Add the segment in `buf' to the queue `q'.
 * Note that q->curdatalen is the amount of sequence space in between the
 * two bytes with lowest and highest sequence number in the queue.
 * Also note that the `urgptr' field of the segment beeing insertred MUST
 * hold the segment length for urgent AND non urgent segments!
 */
static long
tcp_addseg (struct in_dataq *q, BUF *buf)
{
	struct tcp_dgram *tcph = (struct tcp_dgram *) buf->dstart;
	long seq1st, seqnxt;
	BUF *b;
	
	/*
	 * Check for zero length segments
	 */
	if (tcph->urgptr == 0)
	{
		buf_deref (buf, BUF_NORMAL);
		return -1;
	}
	tcph->flags &= ~TCPF_FREEME;
	
	seq1st = tcph->seq;
	seqnxt = seq1st + tcph->urgptr;
	if (!q->qfirst)
	{
		/*
		 * Queue was empty before.
		 */
		buf->prev = buf->next = 0;
		q->qfirst = q->qlast = buf;
		q->curdatalen = tcph->urgptr;
		return 0;
	}
	else if (SEQLT (SEQNXT (q->qlast), seqnxt))
	{
		/*
		 * Insert at the end of the queue.
		 */
		q->curdatalen += seqnxt - SEQNXT(q->qlast);
		buf->prev = q->qlast;
		buf->next = 0;
		q->qlast->next = buf;
		q->qlast = buf;
	}
	else if (SEQLT (seq1st, SEQ1ST (q->qfirst)))
	{
		/*
		 * Insert at the front of the queue.
		 */
		q->curdatalen += SEQ1ST(q->qfirst) - seq1st;
		buf->prev = 0;
		buf->next = q->qfirst;
		q->qfirst->prev = buf;
		q->qfirst = buf;
	}
	else
	{
		/*
		 * Try to find a hole inbetween two segments which is
		 * (partially) filled by the new segment.
		 * Search backwards for efficiency reasons.
		 */
		for (b = q->qlast; b->prev; b = b->prev)
		{
			if (SEQLT (SEQNXT (b->prev), SEQ1ST (b))
				&& SEQLT (seq1st, SEQ1ST (b))
				&& SEQLT (SEQNXT (b->prev), seqnxt))
			{
				buf->prev = b->prev;
				buf->next = b;
				buf->prev->next = buf;
				buf->next->prev = buf;
				break;
			}
		}
		/*
		 * Now this segment must be a duplicate, free it.
		 */
		if (!b->prev)
		{
			DEBUG (("tcp_addseg: duplicate segment"));
			buf_deref (buf, BUF_NORMAL);
			return -1;
		}
	}
	
	/*
	 * Check for and free segments which are contained in the newly
	 * inserted fragment. Note that we must search forward and backward
	 * here.
	 */
	while ((b = buf->next) && SEQLE (SEQNXT (b), seqnxt))
	{
		if (q->qlast == b) q->qlast = buf;
		buf->next = b->next;
		if (b->next) b->next->prev = buf;
		buf_deref (b, BUF_NORMAL);
	}
	while ((b = buf->prev) && SEQLE (seq1st, SEQ1ST (b)))
	{
		if (q->qfirst == b) q->qfirst = buf;
		buf->prev = b->prev;
		if (b->prev) b->prev->next = buf;
		buf_deref (b, BUF_NORMAL);
	}
	return 0;
}

/*
 * Update the send window using the segment `tcph'
 */
static inline short
tcp_sndwnd (struct tcb *tcb, struct tcp_dgram *tcph)
{
	long owlast, wlast;
	
	if (!(tcph->flags & TCPF_ACK))
		return -1;
	
	if (SEQLT (tcph->seq, tcb->snd_wndseq)
		|| (SEQEQ (tcph->seq, tcb->snd_wndseq)
			&& SEQLT (tcph->ack, tcb->snd_wndack)))
		return -1;
	
	owlast = tcb->snd_wndack + tcb->snd_wnd;
	wlast = tcph->ack + tcph->window;
	if (SEQLT (wlast, owlast))
	{
		DEBUG (("tcp_sndwnd: window has been shrunk by %ld bytes",
			owlast-wlast));
		if (SEQLT (wlast, tcb->snd_nxt))
			tcb->snd_nxt = wlast;
	}
	
	tcb->snd_wnd = tcph->window;
	tcb->snd_wndseq = tcph->seq;
	tcb->snd_wndack = tcph->ack;
	
	if (tcb->snd_wnd > tcb->snd_wndmax)
	{
		tcb->snd_wndmax = tcb->snd_wnd;
		/*
		 * # of mss sized packets that fit into
		 * the max. send window we have seen so far.
		 */
		tcb->snd_ppw = tcb->snd_wndmax/tcb->snd_mss;
		if (tcb->snd_ppw < 2)
			tcb->snd_ppw = 2;
	}
	
	return (SEQLE (owlast, wlast) && tcb->snd_wnd > 0)
		? TCBOE_WNDOPEN : TCBOE_WNDCLOSE;
}

/*
 * Handle incoming acknowledgments
 */
static long
tcp_ack (struct tcb *tcb, BUF *buf, short update_sndwnd)
{
	struct tcp_dgram *tcph = (struct tcp_dgram *) IP_DATA (buf);
	short cmd = -1;
	long osnd_wnd;
	
	if (!(tcph->flags & TCPF_ACK))
		return -1;
	
	/*
	 * Save old send window, we need it later for dupack detection.
	 */
	osnd_wnd = tcb->snd_wnd;
	if (update_sndwnd)
		cmd = tcp_sndwnd (tcb, tcph);
	
	if (SEQGT (tcph->ack, tcb->snd_max))
	{
		/*
		 * Ack for something that we haven't sent yet.
		 */
		DEBUG (("tcp_ack(%d): state %d: ack for data not yet sent",
			tcb->data->src.port, tcb->state));
		KAYDEBUG (("tcp_ack: sndmax %ld, ack %ld",
				tcb->snd_max, tcph->ack));
		if (tcb->state == TCBS_SYNRCVD)
			tcp_sndrst (buf);
		else
			tcp_sndack (tcb, buf);
	}
	else if (SEQLT (tcb->snd_una, tcph->ack))
	{
		/*
		 * Something yet unacked has been acked.
		 */
		tcb->snd_una = tcph->ack;
		TCB_OSTATE (tcb, TCBOE_ACKRCVD);
		tcb->dupacks = 0;
	}
	else if (SEQLT (tcph->ack, tcb->snd_una))
	{
		/*
		 * Out of sequence ack (eg. retransmitted segment).
		 */
		DEBUG (("tcp_ack(%d): duplicate ack",tcb->data->src.port));
	}
	else if (SEQEQ (tcb->snd_una, tcph->ack)
			&& osnd_wnd == tcph->window
			&& tcp_seglen (buf, tcph) == 0)
	{
		/*
		 * Duplicate ack, ie snd_una doesn't change, no window update
		 * and no data in segment.
		 */
		tcb->dupacks++;
		TCB_OSTATE (tcb, TCBOE_DUPACK);
	}
	
	/*
	 * Notify output side of send window changes
	 */
	if (cmd >= 0)
		TCB_OSTATE (tcb, cmd);
	
	return 0;
}

/*
 * Exported for other modules.
 */

long
tcp_rcvurg (struct tcb *tcb, BUF *buf)
{
	struct tcp_dgram *utcph, *tcph;
	long urgptr, urglen, datalen, hdrlen, nxt;
	BUF *ubuf;
	
	tcph = (struct tcp_dgram *)IP_DATA (buf);
	if (!(tcph->flags & TCPF_URG))
		return 0;
	
	/*
	 * Ignore urgent data before connection is established
	 */
	if (tcb->state < TCBS_SYNRCVD
		|| tcb->state > TCBS_CLOSEWAIT
		|| tcph->flags & TCPF_SYN)
	{
		tcph->flags &= ~TCPF_URG;
		return 0;
	}
	
	hdrlen = tcph->hdrlen*4;
	datalen = (long)buf->dend - (long)tcph - hdrlen;
	urglen = tcph->urgptr;
	urgptr = tcph->seq + urglen;
	
	/*
	 * Retransmitted (duplicate) segment ?
	 */
	if (SEQLE (urgptr, tcb->rcv_nxt))
	{
		if (datalen > 0 && urglen <= datalen)
			goto adjust;
		tcph->flags &= ~TCPF_URG;
		return 0;
	}
	
	/*
	 * New urgent pointer ?
	 */
	if (SEQLT (tcb->rcv_urg, urgptr))
	{
		tcb->rcv_urg = urgptr;
		if (tcb->data->sock)
		{
			so_wakexsel (tcb->data->sock);
			tcp_sendsig (tcb, SIGURG);
		}
	}
	if (urglen > datalen || datalen == 0)
	{
		/*
		 * We have just been told the new urgent pointer,
		 * the actual urgent data is not carried within
		 * this segment
		 */
		tcph->flags &= ~TCPF_URG;
		return 0;
	}
	
	/*
	 * Make a new segment containing the urgent data
	 */
	ubuf = buf_alloc (hdrlen + urglen, 0, BUF_NORMAL);
	if (ubuf)
	{
		utcph = (struct tcp_dgram *)ubuf->dstart;
		memcpy (utcph, tcph, hdrlen + urglen);
		utcph->flags = TCPF_URG;
		utcph->urgptr = urglen;
		ubuf->dend += hdrlen + urglen;
# if 0
		DEBUG (("tcp_rcvurg::"));
		tcp_dump (buf);
# endif

		/*
		 * Add the segment to the recv queue and wake things up.
		 */
		if (!tcp_addseg (&tcb->data->rcv, ubuf))
		{
			nxt = tcb->rcv_nxt;
			while (ubuf && SEQLE (SEQ1ST (ubuf), nxt))
			{
				nxt = SEQNXT (ubuf);
				ubuf = ubuf->next;
			}
			if (SEQLT (tcb->rcv_nxt, nxt))
			{
				tcb->rcv_nxt = nxt;
				tcp_output (tcb, 0, 0, 0, 0, TCPF_ACK);
			}
			if (tcb->data->sock)
			{
				so_wakexsel (tcb->data->sock);
				so_wakersel (tcb->data->sock);
				wake (IO_Q, (long)tcb->data->sock);
			}
		}
	}
	else
		DEBUG (("tcp_rcvurg: no space for urgent data"));
	
	/*
	 * Adjust the original segment
	 */
adjust:
	memcpy (TCP_DATA (tcph), TCP_DATA (tcph) + urglen, datalen - urglen);
	tcph->flags &= ~TCPF_URG;
	tcph->seq += urglen;
	buf->dend -= urglen;
	((struct ip_dgram *)buf->dstart)->length -= urglen;
	
	return 0;
}
