/*
 *	This file contains some utility functions used both for input
 *	and output.
 *
 *	04/17/94, Kay Roemer.
 */

# include "tcputil.h"

# include "mint/net.h"

# include "inetutil.h"
# include "route.h"
# include "tcpout.h"


long
tcp_isn (void)
{
	static long isn = 0;
	
	if (isn == 0)
		isn = unixtime (t_gettime (), t_getdate ());
	
	isn += 999;
	return isn;
}

struct tcb *
tcb_alloc (void)
{
	struct tcb *tcb;
	
	tcb = kmalloc (sizeof (*tcb));
	if (!tcb)
	{
		DEBUG (("tcb_alloc: out of kernel memory"));
		return NULL;
	}
	bzero (tcb, sizeof (*tcb));
	
	tcb->state = TCBS_CLOSED;
	tcb->ostate = TCBOS_IDLE;
	tcb->snd_mss = TCP_MSS;
	tcb->rcv_mss = TCP_MSS;
	tcb->snd_ppw = 2;
	
	/*
	 * The following settings will result in an initial timeout of
	 * 2 seconds.
	 */
	tcb->rtt = 0;
	tcb->rttdev = (2000/EVTGRAN);
	
	tcb->artt = 0;
	tcb->arttdev = TCP_MINRETRANS;
	
	/*
	 * tcb->snd_cwnd is only an estimate (mss currently can't be
	 * greater than TCP_MSS) and is updated if we receive a max
	 * segment size option.
	 */
	tcb->snd_cwnd = tcb->snd_mss;
	tcb->snd_thresh = 65536;
	
	return tcb;
}

static void
tcb_deltimers (struct tcb *tcb)
{
	event_del (&tcb->timer_evt);
	event_del (&tcb->delete_evt);
	event_del (&tcb->ack_evt);
}

void
tcb_free (struct tcb *tcb)
{
	tcb_deltimers (tcb);
	kfree (tcb);
}

void
tcb_delete (struct tcb *tcb)
{
	struct in_data *data = tcb->data;
	
	tcb->state = TCBS_CLOSED;
	if (data->sock == 0)
	{
		DEBUG (("tcp_delete: removing tcb"));
		
		tcb_free (tcb);
		data->pcb = 0;
		in_data_destroy (data, 0);
	}	
}

static void
deleteme (long arg)
{
	struct tcb *tcb = (struct tcb *) arg;
	struct in_data *data = tcb->data;
	
	UNUSED(arg);
	tcb->state = TCBS_CLOSED;
	if (data->sock == 0)
	{
		DEBUG (("deleteme: removing tcb"));
		
		tcb_free (tcb);
		data->pcb = 0;
		in_data_destroy (data, 0);
	}
}

void
tcb_wait (struct tcb *tcb)
{
	switch (tcb->state)
	{
		case TCBS_FINWAIT2:
			/*
			 * Add a deletion timer for FINWAIT2 sockets, so they
			 * go always away.
			 */
			event_add (&tcb->delete_evt, TCP_CONN_TMOUT, deleteme, (long)tcb);
			break;
			
		default:
			DEBUG (("tcb_wait: called on tcb != FINWAIT2,TIMEWAIT"));
			
		case TCBS_TIMEWAIT:
			tcb_deltimers (tcb);
			event_add (&tcb->delete_evt, 2*TCP_MSL, deleteme, (long)tcb);
			break;
	}
}

void
tcb_reset (struct tcb *tcb, long err)
{
	tcb_deltimers (tcb);
	tcb->ostate = TCBOS_IDLE;
	if (tcb->data)
	{
		tcb->data->err = err;
		in_data_flush (tcb->data);
		if (tcb->data->sock)
		{
			if (tcb->state == TCBS_SYNRCVD
				&& (tcb->flags & TCBF_PASSIVE))
			{
				so_wakersel (tcb->data->sock->conn);
				wake (IO_Q, (long)tcb->data->sock->conn);
			}
			so_wakersel (tcb->data->sock);
			so_wakewsel (tcb->data->sock);
			wake (IO_Q, (long)tcb->data->sock);
		}
	}
}

void
tcb_error (struct tcb *tcb, long err)
{
	if (tcb->data)
	{
		tcb->data->err = err;
		if (tcb->data->sock)
		{
			so_wakersel (tcb->data->sock);
			so_wakewsel (tcb->data->sock);
			wake (IO_Q, (long)tcb->data->sock);
		}
	}
}

/*
 * Send a reset to the sender of the segment in `buf'.
 * NOTE: `buf' holds the IP+TCP dgram.
 */
long
tcp_sndrst (BUF *ibuf)
{
	struct tcp_dgram *otcph, *itcph;
	BUF *obuf;
	
	itcph = (struct tcp_dgram *) IP_DATA (ibuf);
	if (itcph->flags & TCPF_RST)
		return 0;
	
	obuf = buf_alloc (TCP_MINLEN + TCP_RESERVE, TCP_RESERVE, BUF_NORMAL);
	if (!obuf)
	{
		DEBUG (("tcp_reset: no memory for reset segment"));
		return 0;
	}
	
	otcph = (struct tcp_dgram *) obuf->dstart;
	otcph->srcport = itcph->dstport;
	otcph->dstport = itcph->srcport;
	
	if (itcph->flags & TCPF_ACK)
	{
		otcph->seq = itcph->ack;
		otcph->flags = TCPF_RST;
	}
	else
	{
		otcph->seq = 0;
		otcph->flags = TCPF_RST|TCPF_ACK;
	}
	
	otcph->ack = itcph->seq + tcp_seglen (ibuf, itcph);
	otcph->hdrlen = TCP_MINLEN/4;
	otcph->window = 0;
	otcph->urgptr = 0;
	otcph->chksum = 0;
	otcph->chksum = tcp_checksum (otcph, TCP_MINLEN, IP_DADDR (ibuf),
		IP_SADDR (ibuf));
	
	obuf->dend += TCP_MINLEN;
	return ip_send (IP_DADDR (ibuf), IP_SADDR (ibuf), obuf, IPPROTO_TCP,
		0, 0);
}

/*
 * Send an ack to the sender of the segment in `buf'.
 * NOTE: `buf' holds the IP+TCP dgram.
 */
long
tcp_sndack (struct tcb *tcb, BUF *ibuf)
{
	struct tcp_dgram *otcph, *itcph = (struct tcp_dgram *) IP_DATA (ibuf);
	long wndlast;
	BUF *obuf;
	
	if (itcph->flags & TCPF_RST)
		return 0;
	
	wndlast = tcb->snd_wndack + tcb->snd_wnd;
	if (tcb->snd_wnd > 0)
		--wndlast;
	
	obuf = buf_alloc (TCP_MINLEN + TCP_RESERVE, TCP_RESERVE, BUF_NORMAL);
	if (!obuf)
	{
		DEBUG (("tcp_sndack: no memory for ack"));
		return 0;
	}
	
	otcph = (struct tcp_dgram *) obuf->dstart;
	otcph->srcport = itcph->dstport;
	otcph->dstport = itcph->srcport;
	otcph->seq = SEQLE (tcb->snd_nxt, wndlast) ? tcb->snd_nxt : wndlast;
	otcph->ack = tcb->rcv_nxt;
	otcph->hdrlen = TCP_MINLEN/4;
	otcph->flags = TCPF_ACK;
	otcph->window = tcp_rcvwnd (tcb, 1);
	otcph->urgptr = 0;
	otcph->chksum = 0;
	otcph->chksum = tcp_checksum (otcph, TCP_MINLEN, IP_DADDR (ibuf),
		IP_SADDR (ibuf));
	
	obuf->dend += TCP_MINLEN;
	
	/*
	 * Everything acked now
	 */
	tcb->flags &= ~TCBF_DELACK;
	return ip_send (IP_DADDR (ibuf), IP_SADDR (ibuf), obuf, IPPROTO_TCP,
		0, &tcb->data->opts);
}

/*
 * Return nonzero if the TCP segment in `buf' contains anything that must be
 * processed further.
 */
short
tcp_valid (struct tcb *tcb, BUF *buf)
{
	struct tcp_dgram *tcph = (struct tcp_dgram *)IP_DATA (buf);
	long window, wndlast, seglen, seqlast, seq;
	
	if (tcb->state < TCBS_SYNRCVD)
		return 1;
	
	seglen = tcp_seglen (buf, tcph);
	seq = tcph->seq;
	
	window = tcp_rcvwnd (tcb, 0);
	if (window > 0)
	{
		wndlast = tcb->rcv_nxt + window;
		seqlast = seq + seglen;
		if (seglen)
			--seqlast;
		
		if ((tcph->flags & TCPF_SYN) && SEQLT (seq, tcb->rcv_nxt))
		{
			tcph->flags &= ~TCPF_SYN;
			++tcph->seq;
		}
		
		if ((tcph->flags & TCPF_FIN) && SEQGE (seqlast, wndlast))
			tcph->flags &= ~TCPF_FIN;
		
		if (SEQLE (tcb->rcv_nxt, seqlast) && SEQLT (seq, wndlast))
			return 1;
	}
	else if (seglen == 0 && SEQEQ (seq, tcb->rcv_nxt))
		return 1;
	
	return 0;
}

long
tcp_options (struct tcb *tcb, struct tcp_dgram *tcph)
{
	short optlen, len, i;
	uchar *cp;
	long mss = TCP_MSS;
	
	UNUSED(tcb);
	optlen = tcph->hdrlen*4 - TCP_MINLEN;
	cp = (unsigned char *)tcph->data;
	for (i = 0; i < optlen; i += len)
	{
		switch (cp[i])
		{
			case TCPOPT_EOL:
				goto leave;
			
			case TCPOPT_NOP:
				len = 1;
				break;
			
			case TCPOPT_MSS:
				len = cp[i+1];
				if (len != 4)
				{
					DEBUG (("tcp_opt: wrong mss opt len %d", len));
					break;
				}
				if (tcph->flags & TCPF_SYN)
					mss = (((ushort)cp[i+2]) << 8) + cp[i+3];
				break;
			
			default:
				DEBUG (("tcp_options: unknown TCP option %d", cp[i]));
				goto leave;
		}
	}
leave:
	return mss;
}

long
tcp_mss (struct tcb *tcb, ulong faddr, long maxmss)
{
	struct route *rt;
	long win;
	long mss = TCP_MSS;
	
	rt = route_get (faddr);
	if (rt)
	{
		mss = MIN (rt->nif->mtu-20, (ulong)maxmss+20) - TCP_MINLEN;
		
		/*
		 * If destination not on directly attached network then
		 * used default MSS. Otherwise make it as big as mtu
		 * allows.
		 */
		if ((rt->metric > 0 || (rt->flags & RTF_GATEWAY)) && mss > TCP_MSS)
			mss = TCP_MSS;
		if (mss < 32)
			mss = 32;
		
		/*
		 * Limit receive window to 'maxpackets' ful sized segments
		 * if interface wants this.
		 */
		if (rt->nif->maxpackets > 0)
		{
			win = rt->nif->maxpackets * mss;
			if (win < tcb->data->rcv.maxdatalen)
				tcb->data->rcv.maxdatalen = win;
		}
		
		/*
		 * MSS at most half of our send window, please.
		 */
		if (2*mss > tcb->data->snd.maxdatalen)
			mss = tcb->data->snd.maxdatalen/2;
		
		route_deref (rt);
	}
	
	return mss;	
}

ushort
tcp_checksum (struct tcp_dgram *dgram, ushort len, ulong srcadr, ulong dstadr)
{
	ulong sum = 0;
	
	/*
	 * Pseudo IP header checksum
	 */
	__asm__(
		"\tmoveq	#0, d0\n"
		"\tmovel	%3, %0\n"
		"\taddl	%1, %0\n"
		"\taddxl	%2, %0\n"
#ifdef __mcoldfire__
		"\tmvz.w	%4, d1\n"	/* X not affected */
		"\taddxl	d1, %0\n"
		"\taddxl	d0, %0\n"
#else
		"\taddxw	%4, %0\n"
		"\taddxw	d0, %0\n"
#endif
		: "=d"(sum)
		:"g"(srcadr), "d"(dstadr), "i"(IPPROTO_TCP), "d"(len), "0"(sum)
#ifdef __mcoldfire__
		: "d0", "d1", "cc"
#else
		: "d0", "cc"
#endif
		);

	/*
	 * TCP datagram & header checksum
	 */
	__asm__(
		"\tclrl	d0\n"
#ifdef __mcoldfire__
		"\tmvzw	%2, d1\n"
		"\tlsrl	#4, d1\n"
#else
		"\tmovew	%2, d1\n"
		"\tlsrw	#4, d1\n"
#endif
		"\tbeq	4f\n"
#ifdef __mcoldfire__
		"\tsubql	#1, d1\n"		/* clears X bit */
#else
		"\tsubqw	#1, d1\n"		/* clears X bit */
#endif
		"1:\n"
#ifdef __mcoldfire__
		"\tmoveml	%4@, d0/d2-d4\n"	/* 16 byte loop */
		"\tlea	%4@(16), %4\n"
#else
		"\tmoveml	%4@+, d0/d2-d4\n"	/* 16 byte loop */
#endif
		"\taddxl	d0, %0\n"	/* ~5 clock ticks per byte */
		"\taddxl	d2, %0\n"
		"\taddxl	d3, %0\n"
		"\taddxl	d4, %0\n"
#ifdef __mcoldfire__
		"\tmoveq	#0, d0\n"	/* X not affected */
		"\taddxl	d0, %0\n" "subql	#1, d1		\n\t"	/* X cloberred */
		"\tbpls	1b\n"		/* X not affected */
#else
		"\tdbra	d1, 1b\n"
		"\tclrl	d0\n"
		"\taddxl	d0, %0\n"
#endif
		"4:\n"
		"\tmovew	%2, d1\n"
#ifdef __mcoldfire__
		"\tandil	#0xf, d1\n"
		"\tlsrl	#2, d1\n"
#else
		"\tandiw	#0xf, d1\n"
		"\tlsrw	#2, d1\n"
#endif
		"\tbeq	2f\n"
#ifdef __mcoldfire__
		"\tsubql	#1, d1\n"
#else
		"\tsubqw	#1, d1\n"
#endif
		"3:\n"
		"\taddl	%4@+, %0\n"	/* 4 byte loop */
		"\taddxl	d0, %0\n"	/* ~10 clock ticks per byte */
#ifdef __mcoldfire__
		"\tsubql	#1, d1\n"
		"\tbpls	3b\n"
#else
		"\tdbra	d1, 3b\n"
#endif
		"2:\n"
		: "=d"(sum), "=a"(dgram)
		: "g"(len), "0"(sum), "1"(dgram):"d0", "d1", "d2", "d3", "d4", "cc");

	/*
	 * Add in extra word, byte (if len not multiple of 4).
	 * Convert to short
	 */
	__asm__(
		"\tclrl	d0\n"
		"\tbtst	#1, %2\n"
		"\tbeq	5f\n"
#ifdef __mcoldfire__
		"\tmvzw	%4@+, d2\n"
		"\taddl	d2, %0\n"	/* no, add in extra word */
		"\taddxl	d0, %0\n"
#else
		"\taddw	%4@+, %0\n"	/* extra word */
		"\taddxw	d0, %0\n"
#endif
		"5:	\n"
		"\tbtst	#0, %2\n"
		"\tbeq	6f\n"
#ifdef __mcoldfire__
		"\tmvzb	%4@+, d1\n"	/* extra byte */
		"\tlsll	#8, d1\n"
		"\taddl	d1, %0\n"
		"\taddxl	d0, %0\n"
#else
		"\tmoveb	%4@+, d1\n"	/* extra byte */
		"\tlslw	#8, d1\n"
		"\taddw	d1, %0\n"
		"\taddxw	d0, %0\n"
#endif
		"6:\n"
#ifdef __mcoldfire__
		"\tswap	%0\n"		/* convert to short */
		"\tmvzw	%0, d1\n"
		"\tclr.w	%0\n"
		"\tswap	%0\n"
		"\taddl	d1, %0\n"
		"\tswap	%0\n"
		"\tmvzw	%0, d1\n"
		"\tclr.w	%0\n"
		"\tswap	%0\n"
		"\taddl	d1, %0\n"
#else
		"\tmovel	%0, d1\n"	/* convert to short */
		"\tswap	d1\n"
		"\taddw	d1, %0\n"
		"\taddxw	d0, %0\n"
#endif
		: "=d"(sum), "=a"(dgram)
		: "d"(len), "0"(sum), "1"(dgram)
#ifdef __mcoldfire__
		: "d0", "d1", "d2"
#else
		: "d0", "d1"
#endif
		);

	return (short)(~sum & 0xffff);
}

void
tcp_dump (BUF *buf)
{
	struct tcp_dgram *tcph = (struct tcp_dgram *) buf->dstart;
	long datalen;
	
	datalen = (long) buf->dend - (long) TCP_DATA (tcph);
	UNUSED (datalen);
	
	DEBUG (("tcpdump: srcport = %d, dstport = %d, hdrlen = %d",
		tcph->srcport, tcph->dstport, tcph->hdrlen*4));
	DEBUG (("tcpdump: seq = %ld, datalen = %ld, ack = %ld",
		tcph->seq, datalen, tcph->ack));
	DEBUG (("tcpdump: flags = %s%s%s%s%s%s%s",
		tcph->flags & TCPF_URG ? "URG " : "",
		tcph->flags & TCPF_ACK ? "ACK " : "",
		tcph->flags & TCPF_PSH ? "PSH " : "",
		tcph->flags & TCPF_RST ? "RST " : "",
		tcph->flags & TCPF_SYN ? "SYN " : "",
		tcph->flags & TCPF_FIN ? "FIN " : "",
		tcph->flags & TCPF_FREEME ? "FRE " : ""));
	DEBUG (("tcpdump: window = %u, chksum = %u, urgptr = %u",
		tcph->window, tcph->chksum, tcph->urgptr));
}
