/*
 *	This file contains the functions needed for processing outgoing
 *	TCP segments.
 *
 *	04/15/94, Kay Roemer.
 */

# include "tcpout.h"

# include "iov.h"
# include "tcputil.h"


/*
 * Some macros to extract some tcp header fields given a BUF holding
 * a TCP segment.
 */
# define TH(b)		((struct tcp_dgram *)(b)->dstart)
# define SEQ1ST(b)	(TH(b)->seq)
# define DATLEN(b)	(TH(b)->urgptr)

static void	tcbos_idle	(struct tcb *, short);
static void	tcbos_xmit	(struct tcb *, short);
static void	tcbos_retrans	(struct tcb *, short);
static void	tcbos_persist	(struct tcb *, short);

static BUF *	tcp_mkseg	(struct tcb *, ulong);
static long	tcp_sndseg	(struct tcb *, BUF *, short, long w1, long w2);
static short	tcp_retrans	(struct tcb *);
static short	tcp_probe	(struct tcb *);
static long	tcp_dropdata	(struct tcb *);
static long	tcp_sndhead	(struct tcb *);
static void	tcp_rtt		(struct tcb *, BUF *);
static long	tcp_atimeout	(struct tcb *);

static short	canretrans	(struct tcb *);
static void	wakeme		(long);

void (*tcb_ostate[])(struct tcb *, short) =
{
	tcbos_idle,
	tcbos_retrans,
	tcbos_persist,
	tcbos_xmit
};

# ifdef DEV_DEBUG
static char *ostate_names[] =
{
	"IDLE",
	"RETRANS",
	"PERSIST",
	"XMIT"
};
# endif

/*
 * Output finite state machine.
 */

static void
tcbos_idle (struct tcb *tcb, short event)
{
	long tmout;
	
	switch (event)
	{
		case TCBOE_SEND:
		{
			if (SEQGE (tcb->snd_nxt, tcb->seq_write))
				break;
			
			if (DIFTIME (tcb->last_recv, GETTIME()) > tcp_timeout (tcb))
			{
				if (tcb->snd_cwnd > tcb->snd_thresh)
					tcb->snd_thresh = tcb->snd_cwnd;
				tcb->snd_cwnd = tcb->snd_mss;
			}
			
			/*
			 * Update time of last receive, because we did not send
			 * something for some time.
			 */
			tcb->last_recv = GETTIME ();
			tcp_sndhead (tcb);
			tcb->persist_tmo =
			tcb->retrans_tmo = tmout = tcp_timeout (tcb);
			
			if (canretrans (tcb))
			{
				tcb->ostate = TCBOS_XMIT;
				tmout -= DIFTIME (tcb->data->snd.qfirst->info,
					GETTIME ());
				if (tmout < 1)
					tmout = 1;
			}
			else
			{
				tcb->ostate = TCBOS_PERSIST;
				tcb->flags &= ~(TCBF_DORTT|TCBF_ACKVALID);
			}
			
			event_add (&tcb->timer_evt, tmout, wakeme, (long)tcb);
			
			DEBUG (("tcpout: port %d: IDLE -> %s", tcb->data->src.port,
					ostate_names[tcb->ostate]));
			
			break;
		}
	}
}

static void
tcbos_xmit (struct tcb *tcb, short event)
{
	long tmout;
#ifdef DEV_DEBUG
	long delay;
	long inq = 0;
#endif

	switch (event)
	{
		case TCBOE_SEND:
		{
			if (SEQLT (tcb->snd_nxt, tcb->seq_write))
				tcp_sndhead (tcb);
			break;
		}
		case TCBOE_WNDOPEN:
		{
			if (SEQLT (tcb->snd_nxt, tcb->seq_write))
				tcp_sndhead (tcb);
		}
		/* FALLTHROUGH */
		case TCBOE_WNDCLOSE:
		{
			if (!canretrans (tcb))
			{
				tcb->persist_tmo = tcb->retrans_tmo;
				tcb->ostate = TCBOS_PERSIST;
				tcb->flags &= ~(TCBF_DORTT|TCBF_ACKVALID);
				
				DEBUG (("tcpout: port %d: XMIT -> PERSIST",
						tcb->data->src.port));
			}
			break;
		}
		case TCBOE_ACKRCVD:
		{
#ifdef DEV_DEBUG
	  		delay = DIFTIME(tcb->data->snd.qfirst->info, GETTIME());
#endif
			
			tcp_dropdata (tcb);
			if (SEQGE (tcb->snd_una, tcb->seq_write))
			{
				event_del (&tcb->timer_evt);
				tcb->ostate = TCBOS_IDLE;
				tcb->flags &= ~TCBF_ACKVALID;
				
				DEBUG (("tcpout: port %d: XMIT -> IDLE",
						tcb->data->src.port));
				break;
			}
			
			tcb->retrans_tmo = tmout = tcp_timeout (tcb);
			if (canretrans (tcb))
			{
				long atmout = tcp_atimeout (tcb);
				tmout -= DIFTIME (tcb->data->snd.qfirst->info,
						  GETTIME());
				if (tmout < atmout)
					tmout = atmout;
#ifdef DEV_DEBUG
				inq = DIFTIME (tcb->data->snd.qfirst->info, GETTIME());
#endif
			}
			
			DEBUG (("a %4ld ms; t %4ld ms; q %4ld ms; mt %4ld ms",
					delay*EVTGRAN, tmout*EVTGRAN, inq*EVTGRAN,
					tcp_atimeout (tcb) * EVTGRAN));
			
			event_reset (&tcb->timer_evt, tmout);
			break;
		}
		case TCBOE_TIMEOUT:
		{
			DEBUG (("tcpout: port %d: XMIT -> RETRANS",
					tcb->data->src.port));
			
#ifdef DEV_DEBUG
	  		delay = DIFTIME (tcb->data->snd.qfirst->info, GETTIME());
#endif
			DEBUG (("tcpout: timeout after %4ld ms", delay*EVTGRAN));
			
			tcb->ostate = TCBOS_RETRANS;
			tcb->flags &= ~(TCBF_DORTT|TCBF_ACKVALID);
			if (tcb->dupacks >= TCP_DUPTHRESH)
				tcb->dupacks = TCP_DUPTHRESH-1;
			/*
			 * be careful, tcp_retrans () may destroy tcb.
			 */
			tcp_retrans (tcb);
			break;
		}
	}
}

static void
tcbos_retrans (struct tcb *tcb, short event)
{
	struct in_dataq *q;
	long nsegs;
	long tmout;
	
	switch (event)
	{
		case TCBOE_TIMEOUT:
		{
			/*
			 * be careful, tcp_retrans () may destroy tcb.
			 */
			tcp_retrans (tcb);
			break;
		}
		case TCBOE_WNDCLOSE:
		{
			if (!canretrans (tcb))
			{
				tcb->persist_tmo = tcb->retrans_tmo;
				tcb->ostate = TCBOS_PERSIST;
				
				DEBUG (("tcpout: port %d: RETRANS -> PERSIST",
						tcb->data->src.port));
			}
			break;
		}
		case TCBOE_DUPACK:
		{
# ifdef USE_DUPLICATE_ACKS
			/*
			 * Duplicate acks are a good measurement for how many
			 * segments have arrived at the receiver (but out of
			 * order).
			 * We use this information to resend some segments and
			 * keep up the congestion window this way.
			 */
			if (tcb->dupacks >= TCP_DUPTHRESH && canretrans (tcb))
			{
				BUF *b = tcb->data->snd.qfirst;
				short i = tcb->dupacks;
				
				while (b && --i >= TCP_DUPTHRESH)
					b = b->next;
				
				if (b && SEQLE (SEQ1ST (b) + tcp_seglen (b, TH(b)),
								tcb->snd_nxt))
				{
					i = (tcb->dupacks == TCP_DUPTHRESH)
						? tcb->nretrans : 0;
					DEBUG (("tcbos_retrans: resending..."));
					tcp_sndseg (tcb, b, i, tcb->snd_una,
						tcb->snd_nxt);
				}
			}
# endif /* USE_DUPLICATE_ACKS */
			break;
		}
		case TCBOE_ACKRCVD:
		{
			nsegs = tcp_dropdata (tcb);
			if (SEQGE (tcb->snd_una, tcb->seq_write))
			{
				event_del (&tcb->timer_evt);
				tcb->ostate = TCBOS_IDLE;
				
				DEBUG (("tcpout: port %d: RETRANS -> IDLE",
						tcb->data->src.port));
				break;
			}
# ifdef USE_DROPPED_SEGMENT_DETECTION
			/*
			 * Since we append the number of retransmits so far for
			 * this segment in bytes to every retransmitted segment 
			 * we can determine the reason of the timeout from the
			 * first ack for this segment, ie. if
			 * 1) the timeout was to short or
			 * 2) the segment or its ack was dropped
			 *
			 * In case 2) we want to reset the backoff, in case 1)
			 * we keep the backoff until the next rtt measurement.
			 */
			if (nsegs == 1 && (q = &tcb->data->snd)->qfirst &&
			    tcb->snd_una - SEQ1ST (q->qfirst) >= TCP_DROPTHRESH)
			{
				DEBUG (("tcbos_retrans: resetting backoff"));
				tcb->backoff = 0;
			}
# endif /* USE_DROPPED_SEGMENT_DETECTION */
		
			if (SEQGE (tcb->snd_una, tcb->snd_nxt))
			{
				/*
				 * everything sent has been acked:
				 * restart transmitting normaly
				 */
				if (tcb->snd_cwnd > tcb->snd_thresh)
					tcb->snd_thresh = tcb->snd_cwnd;
				tcb->snd_cwnd = tcb->snd_mss;
				
				if (SEQLT (tcb->snd_nxt, tcb->seq_write))
					tcp_sndhead (tcb);
				
				DEBUG (("tcpout: port %d: RETRANS -> XMIT",
						tcb->data->src.port));
				
				tcb->ostate = TCBOS_XMIT;
				tcb->retrans_tmo = tcp_timeout (tcb);
				event_reset (&tcb->timer_evt, tcb->retrans_tmo);
			}
			else
			{
				/*
				 * something has been acked, but not everything.
				 */
				tcb->retrans_tmo = tmout = tcp_timeout (tcb);
				if (canretrans (tcb))
				{
					tmout -= DIFTIME (tcb->data->snd.qfirst->info,
							  GETTIME());
					if (tmout < 1)
						tmout = 1;
				}
				event_reset (&tcb->timer_evt, tmout);
			}
			break;
		}
	}
}

static void
tcbos_persist (struct tcb *tcb, short event)
{
	switch (event)
	{
		case TCBOE_TIMEOUT:
		{
			/*
			 * sender SWS: receiver might have shrunk his
			 * receivce buffers...
			 */
			if (tcb->snd_wnd > 0)
			{
				DEBUG (("tcbos_persists: SSWS override timeout"));
				
				tcb->snd_wndmax = tcb->snd_wnd;
				tcp_sndhead (tcb);
				if (canretrans (tcb))
				{
					tcb->ostate = TCBOS_XMIT;
 					tcb->retrans_tmo = tcp_timeout (tcb);
					event_add (&tcb->timer_evt, tcb->retrans_tmo,
						wakeme, (long)tcb);
					
					DEBUG (("tcpout: port %d: PERSIST -> XMIT",
							tcb->data->src.port));
					break;
				}
				else
					tcb->flags &= ~(TCBF_DORTT|TCBF_ACKVALID);
			}
			/*
			 * be careful, tcp_probe () may destroy tcb.
			 */
			tcp_probe (tcb);
			break;
		}
		case TCBOE_ACKRCVD:
		{
			tcp_dropdata (tcb);
			if (SEQGE (tcb->snd_una, tcb->seq_write))
			{
				event_del (&tcb->timer_evt);
				tcb->ostate = TCBOS_IDLE;
				
				DEBUG (("tcpout: port %d: PERSIST -> IDLE",
						tcb->data->src.port));
			}
			break;
		}
		case TCBOE_WNDOPEN:
		{
			if (SEQLT (tcb->snd_nxt, tcb->seq_write))
				tcp_sndhead (tcb);
			
			if (canretrans (tcb))
			{
				DEBUG (("tcpout: port %d: PERSIST -> XMIT",
						tcb->data->src.port));
				
				tcb->ostate = TCBOS_XMIT;
 				tcb->retrans_tmo = tcp_timeout (tcb);
				event_reset (&tcb->timer_evt, tcb->retrans_tmo);
			}
			else
				tcb->flags &= ~(TCBF_DORTT|TCBF_ACKVALID);
			break;
		}
	}
}

/*
 * Helper routines for the output state machine.
 */

/*
 * Make a TCP segment of size 'size' an fill in some header fields.
 */
static BUF *
tcp_mkseg (struct tcb *tcb, ulong size)
{
	struct tcp_dgram *tcph;
	BUF *b;
	
	b = buf_alloc (TCP_RESERVE + size, TCP_RESERVE/2, BUF_NORMAL);
	if (!b)
	{
		DEBUG (("tcp_mkseg: no space for segment"));
		return NULL;
	}
	
	tcph = (struct tcp_dgram *)b->dstart;
	tcph->srcport = tcb->data->src.port;
	tcph->dstport = tcb->data->dst.port;
	tcph->flags = 0;
	tcph->seq = tcb->snd_nxt;
	tcph->ack = tcb->rcv_nxt;
	tcph->hdrlen = TCP_MINLEN/4;
	tcph->window = 0;
	tcph->urgptr = 0;
	tcph->chksum = 0;
	b->dend += size;
	
	return b;
}

/*
 * Send a copy of the segment in 'b' to the net.
 */
static long
tcp_sndseg (struct tcb *tcb, BUF *b, short nretrans, long wnd1st, long wndnxt)
{
	struct tcp_dgram *tcph, *tcph2;
	long seq1st, seqnxt = 0, offs;
	ulong todo;
	BUF *nb, *b2;
	short cut = 0;
	
	todo = (ulong)(b->dend - b->dstart);
	nb = buf_alloc (TCP_RESERVE + todo, TCP_RESERVE/2, BUF_NORMAL);
	if (!nb)
	{
		DEBUG (("tcp_sndseg: no mem to send"));
		return ENOMEM;
	}
	tcph = (struct tcp_dgram *)nb->dstart;
	
	seq1st = SEQ1ST (b);
	seqnxt = seq1st + tcp_seglen (b, TH (b));
	
#if 0
	if (SEQLE (wndnxt, seq1st) || SEQLE (seqnxt, wnd1st))
		FATAL ("tcp_sndseg: seg (%ld %ld) outside wnd (%ld %ld)",
			seq1st, seqnxt, wnd1st, wndnxt);
#endif

	if (!SEQLT (seq1st, wnd1st))
	{
		/*
		 * seg:  |...
		 * win: |...
		 */
		if (SEQLT (wndnxt, seqnxt))
		{
			/*
			 * seg:  |....|
			 * win: |....|
			 */
			if (TH(b)->flags & TCPF_FIN)
				--seqnxt;
			todo -= seqnxt - wndnxt;
			cut |= TCPF_FIN;
		}
		memcpy (nb->dstart, b->dstart, todo);
		nb->dend += todo;
	}
	else
	{
		/*
		 * seg: |...
		 * win:  |...
		 */
		cut |= TCPF_SYN;
		memcpy (nb->dstart, b->dstart, TCP_HDRLEN (TH (b)));
		nb->dend += TCP_HDRLEN (TH (b));
		
		if (TH(b)->flags & TCPF_SYN)
			seq1st++;
		
		offs = wnd1st - seq1st;
		todo = DATLEN (b) - offs;
		if (SEQLT (wndnxt, seqnxt))
		{
			/*
			 * seg: |.....|
			 * win:  |...|
			 */
			if (TH(b)->flags & TCPF_FIN)
				--seqnxt;
			todo -= seqnxt - wndnxt;
			cut |= TCPF_FIN;
		}
		memcpy (nb->dend, TCP_DATA (TH (b)) + offs, todo);
		nb->dend += todo;
		
		tcph->seq = wnd1st;
	}
	tcph->flags &= ~cut;
	
	if (SEQGT (tcph->seq + (nb->dend - nb->dstart) - TCP_HDRLEN (tcph), wndnxt))
		FATAL ("tcp_sndseg: seg (%ld) exceed wnd (%ld)",
			tcph->seq + (nb->dend - nb->dstart) - TCP_HDRLEN (tcph),
			wndnxt);
	
# if 0
	DEBUG (("tcp_sndseg: cut=%x (%ld,%ld,%ld) seq %ld",
		cut,
		MAX (wnd1st - seq1st, 0L),
		(long)(nb->dend - nb->dstart) - TCP_HDRLEN (tcph),
		MAX (seqnxt - wndnxt, 0L),
		seq1st));
# endif
	
# ifdef USE_DROPPED_SEGMENT_DETECTION
	/*
	 * The next part adds if possible `nretrans' bytes from the
	 * next segment to the end of the segment that is beeing sent.
	 *
	 * We use this in tcp_retrans() to add one byte for every
	 * retransmission.
	 *
	 * As a result of this technique we can determine from the
	 * ack which of the retransmits got trough and caused the
	 * ack (provided the segments arrive in order at the receiver).
	 *
	 * Note that tcp_output() has to make sure we don't exceed
	 * the mss by adding up to TCP_MAXRETRY bytes to the segment.
	 */
	if (nretrans > 0 && !(cut & TCPF_FIN) && (b2 = b->next)
		&& (nretrans = MIN (DATLEN(b2), (ushort)nretrans)) > 0
		&& ((tcph2 = TH(b2)), 1)
		&& ((seqnxt = tcph2->seq + nretrans), 1)
		&& SEQLE (seqnxt, tcb->snd_wndack + tcb->snd_wnd)
		&& !((tcph->flags ^ tcph2->flags) & (TCPF_URG|TCPF_SYN|TCPF_FIN)))
	{
		/*
		 * Ok, add the bytes and advance the send
		 * sequence pointer if necessary.
		 */
		DEBUG (("tcp_sndseg: adding %d bytes", nretrans));
		memcpy (nb->dend, TCP_DATA (tcph2), nretrans);
		nb->dend += nretrans;
		if (SEQLT (tcb->snd_max, seqnxt))
			tcb->snd_max = seqnxt;
	}
# endif /* USE_DROPPED_SEGMENT_DETECTION */
	
	tcph->ack = tcb->rcv_nxt;
	tcph->window = tcp_rcvwnd (tcb, 1);
	tcph->chksum = 0;
	tcph->urgptr = 0;
	
	if (SEQGT (tcb->snd_urg, tcph->seq))
	{
		BUF *buf = b;
		
		while (buf && !(TH(buf)->flags & TCPF_URG))
			buf = buf->next;
		
		if (buf)
		{
			tcph->flags |= TCPF_URG;
			tcph->urgptr = (ushort)
				(SEQ1ST(buf) + DATLEN(buf) - tcph->seq);
		}
	}
	
	tcph->chksum = tcp_checksum (tcph, nb->dend - nb->dstart,
		tcb->data->src.addr,
		tcb->data->dst.addr);
	
	/*
	 * Everything acked now
	 */
	tcb->flags &= ~TCBF_DELACK;
	return ip_send (tcb->data->src.addr, tcb->data->dst.addr,
			nb, IPPROTO_TCP, 0, &tcb->data->opts);
}

/*
 * Update the round trip time mean and deviation. Note that tcb->rtt is scaled
 * by 8 and tcb->rttdev by 4.
 */
INLINE void
tcp_rtt (struct tcb *tcb, BUF *buf)
{
	long err;
	long seqnxt = SEQ1ST (buf) + tcp_seglen (buf, TH (buf));
	
	if (tcb->flags & TCBF_DORTT && SEQLT (tcb->rttseq, seqnxt))
	{
		err = DIFTIME (buf->info, GETTIME ());
		err -= tcb->rtt >> 3;
		tcb->rtt += err;
		if (err < 0)
			err = -err;
		tcb->rttdev += err - (tcb->rttdev >> 2);
		tcb->backoff = 0;
		tcb->rttseq = seqnxt;
		
		if (!(tcb->flags & TCBF_ACKVALID))
		{
			tcb->last_ack = GETTIME();
			tcb->flags |= TCBF_ACKVALID;
		}
	}
}

INLINE void
tcp_artt (struct tcb *tcb)
{
	long err;
	
	if (tcb->flags & TCBF_DORTT)
	{
		if (tcb->flags & TCBF_ACKVALID)
		{
			err = DIFTIME (tcb->last_ack, GETTIME ());
			err -= tcb->artt >> 3;
			tcb->artt += err;
			if (err < 0)
				err = -err;
			tcb->arttdev += err - (tcb->arttdev >> 2);
		}
		tcb->last_ack = GETTIME();
		tcb->flags |= TCBF_ACKVALID;
	}
}

/*
 * Drop acked data from the segment retransmission queue.
 */
static long
tcp_dropdata (struct tcb *tcb)
{
	struct in_dataq *q = &tcb->data->snd;
	BUF *b, *nxtb;
	long una;
	short n = 0;
	
	una = tcb->snd_una;
	for (b = q->qfirst; b; ++n, b = nxtb)
	{
		if (SEQLE (SEQ1ST (b) + tcp_seglen (b, TH (b)), una))
		{
			if ((nxtb = b->next))
			{
				nxtb->prev = 0;
				q->qfirst = nxtb;
				q->curdatalen -= DATLEN (b);
			}
			else
			{
				q->qfirst = 0;
				q->qlast = 0;
				q->curdatalen = 0;
			}
			tcp_rtt (tcb, b);
			buf_deref (b, BUF_NORMAL);
		}
		else
			break;
	
	}
	if (tcb->data->sock && n > 0 && tcp_canwrite (tcb->data) > 0)
	{
		so_wakewsel (tcb->data->sock);
		wake (IO_Q, (long)tcb->data->sock);
	}
	
	/*
	 * If packets were acked then reset retransmission counter and
	 * update congestion window.
	 */
	if (n > 0)
	{
		tcp_artt (tcb);
		tcb->nretrans = 0;
		tcb->snd_cwnd += n * ((tcb->snd_cwnd < tcb->snd_thresh)
			? tcb->snd_mss
			: ((tcb->snd_mss * tcb->snd_mss) / tcb->snd_cwnd));
	}
	
	/*
	 * Drag the urgent pointer along with the left window edge
	 */
	if (SEQLT (tcb->snd_urg, tcb->snd_una))
		tcb->snd_urg = tcb->snd_una;
	
	return n;
}

/*
 * Send all unsent urgent segments and all non urgent segments that fall
 * into the receivers window.
 */
static long
tcp_sndhead (struct tcb *tcb)
{
	long wndnxt, seqnxt, rttseq = 0, stamp;
	struct in_dataq *q = &tcb->data->snd;
	short sent = 0;
	long r;
	BUF *b = q->qlast;
	
	if (SEQGE (tcb->snd_nxt, tcb->seq_write))
		return 0;
	
	r = tcb->snd_cwnd;
	
# ifdef USE_NAGLE
	/*
	 * Kludge cwnd so that the last segment is not sent if
	 * last segment is not full sized and
	 * 1) Something unacked is outstanding, or
	 * 2) Nothing unacked is outstanding but there is more than
	 *    one segment in the queue.
	 * Otherwise send what we've got (or what cwnd allows).
	 */
	if (!(tcb->flags & TCBF_NDELAY) && b->info > DATLEN (b) &&
	    (SEQLT (tcb->snd_una, tcb->snd_nxt) || q->qfirst != b))
		r = MIN (SEQ1ST (b) - tcb->snd_wndack, r);
# endif /* USE_NAGLE */
	
	wndnxt = tcb->snd_wndack + MIN (tcb->snd_wnd, r);
	
	/*
	 * Skip already sent segments
	 */
	for (b = q->qfirst; b; b = b->next)
	{
		seqnxt = SEQ1ST (b) + tcp_seglen (b, TH (b));
		if (SEQLT (tcb->snd_nxt, seqnxt))
			break;
	}
	
	/*
	 * Send the segments that fall into the window
	 */
	for ( ; b; b = b->next)
	{
		seqnxt = SEQ1ST (b) + tcp_seglen (b, TH (b));
		stamp = GETTIME ();

		if (SEQLE (wndnxt, SEQ1ST(b)) || SEQLE (seqnxt, tcb->snd_nxt))
			break;

		/*
		 * See if the segment should be sent:
		 * 1) send whole seg if it fits into window
		 * 2) send part if window is larger than
		 *    half the max. send window we have seen.
		 */
		if (!(SEQLE (seqnxt, wndnxt) ||
		      (SEQLT (tcb->snd_nxt, wndnxt) &&
		       2*(wndnxt - tcb->snd_nxt) > tcb->snd_wndmax)))
			break;
		/*
		 * If no outstanding data and nothing can be sent, reduce
		 * backoff. This works ok because we know this `lost'
		 * segment will cause a retransmit and the backoff is
		 * increased in tcp_retrans(), ie stays the same when
		 * decremented here.
		 */
		if ((r = tcp_sndseg (tcb, b, 0, tcb->snd_nxt, wndnxt)) < 0)
		{
			if (r != ENOMEM)
				tcb_error (tcb, r);
			if (SEQEQ (tcb->snd_una, tcb->snd_nxt))
			{
				tcb->snd_nxt = seqnxt;
				if (SEQLT (tcb->snd_max, tcb->snd_nxt))
					tcb->snd_max = tcb->snd_nxt;
				b->info = stamp;
				if (r == ENOMEM && tcb->backoff > 0)
					--tcb->backoff;
				return 1;
			}
			break;
		}
		if (++sent == 1)
			rttseq = tcb->snd_nxt;
		
		tcb->snd_nxt = SEQLE (seqnxt, wndnxt) ? seqnxt : wndnxt;
		if (SEQLT (tcb->snd_max, tcb->snd_nxt))
			tcb->snd_max = tcb->snd_nxt;
		
		b->info = stamp;
	}
	
	DEBUG (("sw %5ld; cw %5ld; ew %5ld; uw %5ld; av %5ld",
			tcb->snd_wnd, tcb->snd_cwnd,
			wndnxt - tcb->snd_wndack,
			tcb->snd_nxt - tcb->snd_wndack,
			tcb->seq_write - tcb->snd_wndack));
	
	/*
	 * Schedule an RTT measurement
	 */
	if (sent && !(tcb->flags & TCBF_DORTT))
	{
		tcb->rttseq = rttseq;
		tcb->flags |= TCBF_DORTT;
	}
	
	return sent;
}

/*
 * Probe the other TCP's window
 */
static short
tcp_probe (struct tcb *tcb)
{
	struct tcp_dgram *tcph;
	BUF *b;
	
	DEBUG (("tcp_probe: port %d: sending probe", tcb->data->src.port));
	
	/*
	 * If peer does not respond then close connection
	 */
	if (DIFTIME (tcb->last_recv, GETTIME ()) > TCP_CONN_TMOUT)
	{
		DEBUG (("tcbos_persist: no response, closing conn."));
		tcb_reset (tcb, ETIMEDOUT);
		tcb->state = TCBS_CLOSED;
		tcb_delete (tcb);
		return 1;
	}
	
	/*
	 * Make the probe contain one bogus data byte outside the window.
	 * Otherwise a 4.2 BSD (and derivate) host won't respond to the
	 * probe.
	 */
	b = tcp_mkseg (tcb, TCP_MINLEN + 1);
	if (b)
	{
		tcph = (struct tcp_dgram *)b->dstart;
		tcph->flags = TCPF_ACK;
		
		/*
		 * was tcb->snd_una - 1
		 */
		tcph->seq = tcb->snd_una - 2;
		tcph->window = tcp_rcvwnd (tcb, 0);
		if (SEQGT (tcb->snd_urg, tcb->snd_una))
		{
			BUF *buf = tcb->data->snd.qfirst;
			while (buf && !(TH(buf)->flags & TCPF_URG))
				buf = buf->next;
			
			if (buf)
			{
				tcph->flags |= TCPF_URG;
				tcph->urgptr = (ushort)
					(SEQ1ST(buf) + DATLEN(buf) - tcph->seq);
			}
		}
		tcph->chksum = tcp_checksum (tcph, TCP_MINLEN+1,
			tcb->data->src.addr,
			tcb->data->dst.addr);
		
		ip_send (tcb->data->src.addr, tcb->data->dst.addr, b,
			IPPROTO_TCP, 0, &tcb->data->opts);
	} else
	{
		DEBUG (("do_probe: no memory to probe"));
	}

	tcb->persist_tmo = MIN (TCP_MAXPROBE, tcb->persist_tmo << 1);
	if (tcb->persist_tmo < TCP_MINRETRANS)
	{
		DEBUG (("do_probe: probe timeout < TCP_MINRETRANS"));
		tcb->persist_tmo = TCP_MINRETRANS;
	}
	
	event_add (&tcb->timer_evt, tcb->persist_tmo, wakeme, (long)tcb);
	
	return 0;
}

/*
 * Retransmit segments after timeout. 
 */
static short
tcp_retrans (struct tcb *tcb)
{
	long r;
	BUF *b;
	
	DEBUG (("tcp_retrans: port %d: sending %dnd retransmit",
			tcb->data->src.port, tcb->nretrans+1));
	
	if (++tcb->nretrans > TCP_MAXRETRY)
	{
		DEBUG (("do_retrans: connection timed out"));
		tcb_reset (tcb, ETIMEDOUT);
		tcb->state = TCBS_CLOSED;
		tcb_delete (tcb);
		return 1;
	}
	
	b = tcb->data->snd.qfirst;
	if (!b)
	{
		ALERT (("do_retrans: queue is empty ?!"));
		tcb->ostate = TCBOS_IDLE;
		return 0;
	}
	
	/*
	 * Shrink threshold to half of its former size on every retransmit
	 * but don't make it smaller than two full sized packets so reopening
	 * the cong. window is fast enough.
	 * Drop cong. window to one full sized packet.
	 */
	if (tcb->snd_cwnd > tcb->snd_thresh)
		tcb->snd_thresh = tcb->snd_cwnd;
	tcb->snd_thresh = MIN (tcb->snd_thresh, tcb->snd_wnd) >> 1;
	if (tcb->snd_thresh < 2*tcb->snd_mss)
		tcb->snd_thresh = 2*tcb->snd_mss;
	tcb->snd_cwnd = tcb->snd_mss;
	
	/*
	 * If memory is low then cause no backoff.
	 */
	r = tcp_sndseg (tcb, b, tcb->nretrans, tcb->snd_una, tcb->snd_nxt);
	switch (r)
	{
		default:
			tcb_error (tcb, r);
		case 0:
			tcb->backoff++;
		case ENOMEM:
			break;
	}
	
	tcb->retrans_tmo = tcp_timeout (tcb);
	event_add (&tcb->timer_evt, tcb->retrans_tmo, wakeme, (long)tcb);
	
	return 0;
}

static void
wakeme (long arg)
{
	TCB_OSTATE ((struct tcb *) arg, TCBOE_TIMEOUT);
}

/*
 * Is there something to retransmit, ie. something that has been
 * transmitted, is not yet acked and still fits into the window?
 */
static short
canretrans (struct tcb *tcb)
{
	BUF *b;
	
	return (tcb->snd_wnd > 0 &&
		(b = tcb->data->snd.qfirst) &&
		SEQLT (tcb->snd_una, tcb->snd_nxt) &&
		SEQLT (SEQ1ST (b), tcb->snd_nxt));
}

/*
 * Exported functions for other modules.
 */

/*
 * Can the segment in `buf` be concatenated with more data?
 */
INLINE long
cancombine (struct tcb *tcb, BUF *buf, short flags)
{
	if (buf && SEQLE (tcb->snd_max, SEQ1ST (buf)) &&
            !((flags ^ TH(buf)->flags) & TCPF_URG) &&
	    !(flags & TCPF_SYN))
		return buf->info - DATLEN (buf);
	
	return 0;
}

/*
 * TCP maximum segment size option we send with every SYN segment.
 */
static struct
{
	char	code;
	char	len;
	short	mss;
}
mss_opt =
{
	code:	TCPOPT_MSS,
	len:	4,
	mss:	0
};

/*
 * Generate and send TCP segments from the data in `iov' and/or with
 * the flags in `flags'.
 */
long
tcp_output (struct tcb *tcb, const struct iovec *iov, short niov, long len,
		long offset, short flags)
{
	struct tcp_dgram *tcph = NULL;
	struct in_dataq *q = &tcb->data->snd;
	short first = 1;
	long ret = 0, r = 0, effmss;
	BUF *b;
	
	/*
	 * Check if ACK only
	 */
	if (len == 0 && (flags & (TCPF_SYN|TCPF_FIN)) == 0)
	{
		b = tcp_mkseg (tcb, TCP_MINLEN);
		if (!b)
		{
			DEBUG (("tcp_out: cannot send, memory low"));
			return ENOMEM;
		}
		
		tcph = TH (b);
		tcph->flags = TCPF_ACK | (flags & TCPF_PSH);
		tcph->window = tcp_rcvwnd (tcb, 1);
		
		tcph->chksum = tcp_checksum (tcph, TCP_MINLEN,
			tcb->data->src.addr,
			tcb->data->dst.addr);
		
		/*
		 * Everything acked now
		 */
		tcb->flags &= ~TCBF_DELACK;
		return ip_send (tcb->data->src.addr, tcb->data->dst.addr,
			b, IPPROTO_TCP, 0, &tcb->data->opts);
	}
	
	/*
	 * Do the hard part
	 */
	do {
		b = q->qlast;
		if ((r = cancombine (tcb, b, flags)) > 0)
		{
			tcph = TH (b);
			if (r > len)
				r = len;
			q->curdatalen += r;
		}
		else
		{
			effmss = tcb->snd_mss;
			
			/*
			 * Leave TCP_MAXRETRY bytes for the technique
			 * described in tcp_sndseg().
			 */
			r = MIN (effmss - TCP_MAXRETRY, len);
			b = tcp_mkseg (tcb, effmss);
			if (!b)
			{
				DEBUG (("tcp_out: cannot send, memory low"));
				if (first)
					return ENOMEM;
				b = q->qlast;
				ret = ENOMEM;
				break;
			}
			tcph = TH (b);
			tcph->flags = flags & ~(TCPF_SYN|TCPF_FIN);
			tcph->seq = tcb->seq_write;
			tcph->urgptr = 0;
			
			/*
			 * We store the space left in segment here.
			 */
			b->info = effmss - TCP_MAXRETRY;
			b->dend = tcph->data;
			
			if (first && flags & TCPF_SYN)
			{
				tcph->flags |= TCPF_SYN;
				tcb->seq_write++;
				/*
				 * Add maximum segment size TCP option
				 */
				tcph->hdrlen += sizeof (mss_opt) / 4;
				mss_opt.mss = tcb->rcv_mss;
				memcpy (b->dend, &mss_opt, sizeof (mss_opt));
				b->dend += sizeof (mss_opt);
			}
			else
				tcph->flags |= TCPF_ACK;
			
			/*
			 * Enqueue the segment.
			 */
			if (q->qlast == 0)
			{
				b->next = b->prev = 0;
				q->qlast = q->qfirst = b;
				q->curdatalen = r;
			}
			else
			{
				b->next = 0;
				b->prev = q->qlast;
				q->qlast->next = b;
				q->qlast = b;
				q->curdatalen += r;
			}
		}
		if (r > 0)
		{
			r = iov2buf_cpy (b->dend, r, iov, niov, offset);
			offset += r;
			b->dend += r;
			len -= r;
			
			/*
			 * Store segment length in urgptr field.
			 */
			tcph->urgptr += r;
			tcb->seq_write += r;
			tcph->flags |= TCPF_PSH;
		}
		if (len == 0 && flags & TCPF_FIN)
		{
			tcph->flags |= TCPF_FIN;
			tcb->seq_write++;
			
			/*
			 * Make this segment appear full sized in order
			 * to send it immediatly.
			 */
			b->info = tcph->urgptr;
		}
		first = 0;
	}
	while (len > 0);
	
	if (flags & TCPF_URG)
	{
		tcb->snd_urg = tcb->seq_write;
		/*
		 * In PERSIST state we force a probe to be sent
		 * immediatly which communicates our new urgent
		 * pointer.
		 */
		if (tcb->ostate == TCBOS_PERSIST)
		{
			event_del (&tcb->timer_evt);
			tcb->persist_tmo = tcp_timeout (tcb);
			/*
			 * be careful, tcp_probe () may destroy tcb.
			 */
			if (tcp_probe (tcb))
				return ret;
		}
	}
	TCB_OSTATE (tcb, TCBOE_SEND);
	return ret;
}

/*
 * Return the initial timeout to use for retransmission and persisting.
 */
long
tcp_timeout (struct tcb *tcb)
{
	ulong tmout;
	
	tmout = (tcb->rtt >> 3) + tcb->rttdev;
	if (tmout > (0x7ffffffful >> TCP_MAXRETRY))
		tmout = TCP_MAXRETRANS;
	else
	{
		tmout <<= tcb->backoff;
		if (tmout < TCP_MINRETRANS)
			tmout = TCP_MINRETRANS;
		else if (tmout > TCP_MAXRETRANS)
			tmout = TCP_MAXRETRANS;
	}
	
	return tmout;
}

static long
tcp_atimeout (struct tcb *tcb)
{
	long tmout;
	
	tmout = (tcb->artt >> 3) + tcb->arttdev;
	if (tmout < TCP_MINRETRANS)
		tmout = TCP_MINRETRANS;
	else if (tmout > TCP_MAXRETRANS)
		tmout = TCP_MAXRETRANS;
	
	return tmout;
}

/*
 * Return the size of our window we should advertise to the remote TCP.
 * For now only return the current free buffer space, later we have
 * to take into account congestion control.
 */
long
tcp_rcvwnd (struct tcb *tcb, short wnd_update)
{
	long space, minwnd;
	
	space = tcb->data->rcv.maxdatalen - tcb->data->rcv.curdatalen;
	if (space < tcb->rcv_mss && space*4 < tcb->data->rcv.maxdatalen)
		space = 0;
	
	if (tcb->state >= TCBS_SYNRCVD)
	{
		minwnd = tcb->rcv_wnd - tcb->rcv_nxt;
		if (space < minwnd)
			return minwnd;
		
		if (wnd_update)
			tcb->rcv_wnd = tcb->rcv_nxt + space;
	}
	
	return space;
}
