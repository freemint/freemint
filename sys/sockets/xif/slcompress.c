/*-
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)slcompress.c	7.7 (Berkeley) 5/7/91
 */

/*
 * Routines to compress and uncompess tcp packets (for transmission
 * over low speed serial lines.
 *
 * Van Jacobson (van@helios.ee.lbl.gov), Dec 31, 1989:
 *	- Initial distribution.
 */

# include "slcompress.h"
# include "netinfo.h"

# include "inet4/in.h"
# include "inet4/tcp.h"


# ifndef SL_NO_STATS
# define INCR(counter) ++comp->counter;
# else
# define INCR(counter)
# endif

# define BCMP(p1,p2,n)		memcmp ((p2), (p1), (ulong)(n))
# define BCOPY(p1,p2,n)		memcpy ((p2), (p1), (ulong)(n))
# define ovbcopy(p1,p2,n)	memcpy ((p2), (p1), (ulong)(n))

struct slcompress *
slc_init (void)
{
	struct slcompress *comp;
	struct cstate *tstate;
	ushort i;
	
	comp = kmalloc (sizeof (*comp));
	if (!comp)
	{
		DEBUG (("sl_compress_init: no mem"));
		return NULL;
	}
	
	tstate = comp->tstate;
	bzero (comp, sizeof (*comp));
	for (i = MAX_STATES - 1; i > 0; --i)
	{
		tstate[i].cs_id = i;
		tstate[i].cs_next = &tstate[i - 1];
	}
	tstate[0].cs_next = &tstate[MAX_STATES - 1];
	tstate[0].cs_id = 0;
	comp->last_cs = &tstate[0];
	comp->last_recv = 255;
	comp->last_xmit = 255;
	
	return comp;
}

void
slc_free (struct slcompress *comp)
{
	if (comp)
		kfree (comp);
}

/* ENCODE encodes a number that is known to be non-zero.  ENCODEZ
 * checks for zero (since zero has to be encoded in the long, 3 byte
 * form).
 */
# define ENCODE(n) { \
	if ((ushort)(n) >= 256) { \
		*cp++ = 0; \
		cp[1] = (n); \
		cp[0] = (n) >> 8; \
		cp += 2; \
	} else { \
		*cp++ = (n); \
	} \
}
# define ENCODEZ(n) { \
	if ((ushort)(n) >= 256 || (ushort)(n) == 0) { \
		*cp++ = 0; \
		cp[1] = (n); \
		cp[0] = (n) >> 8; \
		cp += 2; \
	} else { \
		*cp++ = (n); \
	} \
}
# define DECODEL(f) { \
	if (*cp == 0) {\
		(f) = htonl(ntohl(f) + ((cp[1] << 8) | cp[2])); \
		cp += 3; \
	} else { \
		(f) = htonl(ntohl(f) + (ulong)*cp++); \
	} \
}
# define DECODES(f) { \
	if (*cp == 0) {\
		(f) = htons(ntohs(f) + ((cp[1] << 8) | cp[2])); \
		cp += 3; \
	} else { \
		(f) = htons(ntohs(f) + (ulong)*cp++); \
	} \
}
# define DECODEU(f) { \
	if (*cp == 0) {\
		(f) = htons((cp[1] << 8) | cp[2]); \
		cp += 3; \
	} else { \
		(f) = htons((ulong)*cp++); \
	} \
}

uchar
slc_compress (BUF *b, struct slcompress *comp, long compress_cid)
{
	struct cstate *cs = comp->last_cs->cs_next;
	struct ip_dgram *ip = (struct ip_dgram *) b->dstart;
	ulong hlen = ip->hdrlen;
	struct tcp_dgram *oth;
	struct tcp_dgram *th;
	ulong deltaS, deltaA;
	ulong changes = 0;
	uchar new_seq[16];
	uchar *cp = new_seq;
	
	/*
	 * Bail if this is an IP fragment or if the TCP packet isn't
	 * `compressible' (i.e., ACK isn't set or some other control bit is
	 * set).  (We assume that the caller has already made sure the
	 * packet is IP proto TCP).
	 */
	if (ip->fragoff & (IP_MF|IP_FRAGOFF) || b->dend - b->dstart < 40)
		return (TYPE_IP);
	
	th = (struct tcp_dgram *) IP_DATA (b);
	if ((th->flags & (TCPF_SYN|TCPF_FIN|TCPF_RST|TCPF_ACK)) != TCPF_ACK)
		return (TYPE_IP);
	/*
	 * Packet is compressible -- we're going to send either a
	 * COMPRESSED_TCP or UNCOMPRESSED_TCP packet.  Either way we need
	 * to locate (or create) the connection state.  Special case the
	 * most recently used connection since it's most likely to be used
	 * again & we don't have to do any reordering if it's used.
	 */
	INCR (sls_packets);
	if (ip->saddr != cs->cs_ip.saddr
		|| ip->daddr != cs->cs_ip.daddr
		|| *(long *) th != ((long *) &cs->cs_ip)[cs->cs_ip.hdrlen])
	{
		/*
		 * Wasn't the first -- search for it.
		 *
		 * States are kept in a circularly linked list with
		 * last_cs pointing to the end of the list.  The
		 * list is kept in lru order by moving a state to the
		 * head of the list whenever it is referenced.  Since
		 * the list is short and, empirically, the connection
		 * we want is almost always near the front, we locate
		 * states via linear search.  If we don't find a state
		 * for the datagram, the oldest state is (re-)used.
		 */
		struct cstate *lcs;
		struct cstate *lastcs = comp->last_cs;
		
		do {
			lcs = cs; cs = cs->cs_next;
			INCR (sls_searches);
			if (ip->saddr == cs->cs_ip.saddr &&
			    ip->daddr == cs->cs_ip.daddr &&
			    *(long *)th == ((long *)&cs->cs_ip)[cs->cs_ip.hdrlen])
				goto found;
		}
		while (cs != lastcs);
		
		/*
		 * Didn't find it -- re-use oldest cstate.  Send an
		 * uncompressed packet that tells the other side what
		 * connection number we're using for this conversation.
		 * Note that since the state list is circular, the oldest
		 * state points to the newest and we only need to set
		 * last_cs to update the lru linkage.
		 */
		INCR (sls_misses);
		comp->last_cs = lcs;
		hlen += th->hdrlen;
		hlen <<= 2;
		goto uncompressed;
		
found:
		/*
		 * Found it -- move to the front on the connection list.
		 */
		if (cs == lastcs)
			comp->last_cs = lcs;
		else
		{
			lcs->cs_next = cs->cs_next;
			cs->cs_next = lastcs->cs_next;
			lastcs->cs_next = cs;
		}
	}
	
	/*
	 * Make sure that only what we expect to change changed. The first
	 * line of the `if' checks the IP protocol version, header length &
	 * type of service.  The 2nd line checks the "Don't fragment" bit.
	 * The 3rd line checks the time-to-live and protocol (the protocol
	 * check is unnecessary but costless).  The 4th line checks the TCP
	 * header length.  The 5th line checks IP options, if any.  The 6th
	 * line checks TCP options, if any.  If any of these things are
	 * different between the previous & current datagram, we send the
	 * current datagram `uncompressed'.
	 */
	oth = (struct tcp_dgram *) &((long *) &cs->cs_ip)[hlen];
	deltaS = hlen;
	hlen += th->hdrlen;
	hlen <<= 2;
	
	if (((ushort *)ip)[0] != ((ushort *)&cs->cs_ip)[0] ||
	    ((ushort *)ip)[3] != ((ushort *)&cs->cs_ip)[3] ||
	    ((ushort *)ip)[4] != ((ushort *)&cs->cs_ip)[4] ||
	    th->hdrlen != oth->hdrlen ||
	    (deltaS > 5 &&
	     BCMP(ip + 1, &cs->cs_ip + 1, (deltaS - 5) << 2)) ||
	    (th->hdrlen > 5 &&
	     BCMP(th + 1, oth + 1, (th->hdrlen - 5) << 2)))
		goto uncompressed;
	
	/*
	 * Figure out which of the changing fields changed.  The
	 * receiver expects changes in the order: urgent, window,
	 * ack, seq (the order minimizes the number of temporaries
	 * needed in this section of code).
	 */
	if (th->flags & TCPF_URG)
	{
		deltaS = ntohs (th->urgptr);
		ENCODEZ (deltaS);
		changes |= NEW_U;
	}
	else if (th->urgptr != oth->urgptr)
		/* argh! URG not set but urp changed -- a sensible
		 * implementation should never do this but RFC793
		 * doesn't prohibit the change so we have to deal
		 * with it. */
		 goto uncompressed;
	
	if ((deltaS = (ushort)(ntohs (th->window) - ntohs (oth->window))))
	{
		ENCODE (deltaS);
		changes |= NEW_W;
	}
	
	if ((deltaA = ntohl (th->ack) - ntohl (oth->ack)))
	{
		if (deltaA > 0xffff)
			goto uncompressed;
		ENCODE (deltaA);
		changes |= NEW_A;
	}
	
	if ((deltaS = ntohl (th->seq) - ntohl (oth->seq)))
	{
		if (deltaS > 0xffff)
			goto uncompressed;
		ENCODE (deltaS);
		changes |= NEW_S;
	}
	
	switch (changes)
	{
		case 0:
			/*
			 * Nothing changed. If this packet contains data and the
			 * last one didn't, this is probably a data packet following
			 * an ack (normal on an interactive connection) and we send
			 * it compressed.  Otherwise it's probably a retransmit,
			 * retransmitted ack or window probe.  Send it uncompressed
			 * in case the other side missed the compressed version.
			 */
			if (ip->length != cs->cs_ip.length &&
			    ntohs (cs->cs_ip.length) == hlen)
				break;
			
		/* (fall through) */
		
		case SPECIAL_I:
		case SPECIAL_D:
			/*
			 * actual changes match one of our special case encodings --
			 * send packet uncompressed.
			 */
			goto uncompressed;
		
		case NEW_S|NEW_A:
			if (deltaS == deltaA &&
			    deltaS == ntohs (cs->cs_ip.length) - hlen)
			{
				/* special case for echoed terminal traffic */
				changes = SPECIAL_I;
				cp = new_seq;
			}
			break;
		
		case NEW_S:
			if (deltaS == ntohs (cs->cs_ip.length) - hlen)
			{
				/* special case for data xfer */
				changes = SPECIAL_D;
				cp = new_seq;
			}
			break;
	}
	
	deltaS = ntohs (ip->id) - ntohs (cs->cs_ip.id);
	if (deltaS != 1)
	{
		ENCODEZ(deltaS);
		changes |= NEW_I;
	}
	
	if (th->flags & TCPF_PSH)
		changes |= TCP_PUSH_BIT;
	/*
	 * Grab the cksum before we overwrite it below.  Then update our
	 * state with this packet's header.
	 */
	deltaA = ntohs (th->chksum);
	BCOPY (ip, &cs->cs_ip, hlen);
	
	/*
	 * We want to use the original packet as our compressed packet.
	 * (cp - new_seq) is the number of bytes we need for compressed
	 * sequence numbers.  In addition we need one byte for the change
	 * mask, one for the connection id and two for the tcp checksum.
	 * So, (cp - new_seq) + 4 bytes of header are needed.  hlen is how
	 * many bytes of the original packet to toss so subtract the two to
	 * get the new packet size.
	 */
	deltaS = cp - new_seq;
	cp = (uchar *)ip;
	if (compress_cid == 0 || comp->last_xmit != cs->cs_id)
	{
		comp->last_xmit = cs->cs_id;
		hlen -= deltaS + 4;
		cp += hlen;
		*cp++ = changes | NEW_C;
		*cp++ = cs->cs_id;
	}
	else
	{
		hlen -= deltaS + 3;
		cp += hlen;
		*cp++ = changes;
	}
	
	b->dstart += hlen;
	*cp++ = deltaA >> 8;
	*cp++ = deltaA;
	BCOPY (new_seq, cp, deltaS);
	INCR (sls_compressed);
	return (TYPE_COMPRESSED_TCP);
	
	/*
	 * Update connection state cs & send uncompressed packet ('uncompressed'
	 * means a regular ip/tcp packet but with the 'conversation id' we hope
	 * to use on future compressed packets in the protocol field).
	 */
uncompressed:
	BCOPY (ip, &cs->cs_ip, hlen);
	ip->proto = cs->cs_id;
	comp->last_xmit = cs->cs_id;
	return (TYPE_UNCOMPRESSED_TCP);
}

long
slc_uncompress (BUF *b, uchar type, struct slcompress *comp)
{
	uchar *cp;
	ulong hlen, changes;
	struct tcp_dgram *th;
	struct cstate *cs;
	struct ip_dgram *ip;
	long len;
	
	len = b->dend - b->dstart;
	
	switch (type)
	{
		case TYPE_UNCOMPRESSED_TCP:
			ip = (struct ip_dgram *) b->dstart;
			if (len < 40 || ip->proto >= MAX_STATES)
				goto bad;
			/*
			 * clear out TYPE_UNCOMPRESSED_TCP
			 */
			ip->version &= 0x4;
			cs = &comp->rstate[comp->last_recv = ip->proto];
			comp->flags &= ~SLF_TOSS;
			ip->proto = IPPROTO_TCP;
			hlen = ip->hdrlen;
			hlen += ((struct tcp_dgram *) &((long *) ip)[hlen])->hdrlen;
			hlen <<= 2;
			BCOPY(ip, &cs->cs_ip, hlen);
			cs->cs_ip.chksum = 0;
			cs->cs_hlen = hlen;
			INCR (sls_uncompressedin);
			return (len);
		
		default:
			goto bad;
		
		case TYPE_COMPRESSED_TCP:
			break;
	}
	
	/* We've got a compressed packet. */
	INCR (sls_compressedin);
	cp = b->dstart;
	changes = *cp++;
	if (changes & NEW_C)
	{
		/* Make sure the state index is in range, then grab the state.
		 * If we have a good state index, clear the 'discard' flag. */
		if (*cp >= MAX_STATES)
			goto bad;
		
		comp->flags &= ~SLF_TOSS;
		comp->last_recv = *cp++;
	}
	else
	{
		/* this packet has an implicit state index.  If we've
		 * had a line error since the last time we got an
		 * explicit state index, we have to toss the packet. */
		if (comp->flags & SLF_TOSS)
		{
			INCR (sls_tossed);
			return (0);
		}
	}
	
	cs = &comp->rstate[comp->last_recv];
	hlen = cs->cs_ip.hdrlen << 2;
	th = (struct tcp_dgram *)&((uchar *)&cs->cs_ip)[hlen];
	th->chksum = htons((*cp << 8) | cp[1]);
	cp += 2;
	if (changes & TCP_PUSH_BIT)
		th->flags |= TCPF_PSH;
	else
		th->flags &= ~TCPF_PSH;

	switch (changes & SPECIALS_MASK)
	{
		case SPECIAL_I:
		{
			ulong i = ntohs (cs->cs_ip.length) - cs->cs_hlen;
			th->ack = htonl (ntohl (th->ack) + i);
			th->seq = htonl (ntohl (th->seq) + i);
			
			break;
		}
		case SPECIAL_D:
		{
			th->seq = htonl(ntohl(th->seq) + ntohs(cs->cs_ip.length)
				   - cs->cs_hlen);
			break;
		}
		default:
		{
			if (changes & NEW_U)
			{
				th->flags |= TCPF_URG;
				DECODEU (th->urgptr);
			}
			else
				th->flags &= ~TCPF_URG;
			if (changes & NEW_W)
				DECODES (th->window);
			if (changes & NEW_A)
				DECODEL (th->ack);
			if (changes & NEW_S)
				DECODEL (th->seq);
			break;
		}
	}
	
	if (changes & NEW_I)
	{
		DECODES (cs->cs_ip.id);
	}
	else
		cs->cs_ip.id = htons (ntohs (cs->cs_ip.id) + 1);

	/*
	 * At this point, cp points to the first byte of data in the
	 * packet.  If we're not aligned on a 4-byte boundary, copy the
	 * data down so the ip & tcp headers will be aligned.  Then back up
	 * cp by the tcp/ip header length to make room for the reconstructed
	 * header (we assume the packet we were handed has enough space to
	 * prepend 128 bytes of header).  Adjust the length to account for
	 * the new header & fill in the IP total length.
	 */
	len -= (cp - (uchar *) b->dstart);
	if (len < 0)
		/* we must have dropped some characters (crc should detect
		 * this but the old slip framing won't) */
		goto bad;
	
	if ((long) cp & 1)
	{
		if (len > 0)
			ovbcopy (cp, cp-1, len);
		--cp; --b->dend;
	}
	cp -= cs->cs_hlen;
	len += cs->cs_hlen;
	cs->cs_ip.length = htons (len);
	BCOPY (&cs->cs_ip, cp, cs->cs_hlen);
	b->dstart = cp;
	
	/* recompute the ip header checksum */
	((struct ip_dgram *)cp)->chksum = in_chksum (cp, hlen >> 1);
	return (len);
bad:
	comp->flags |= SLF_TOSS;
	INCR (sls_errorin);
	return (0);
}

void
slc_getstats (struct slcompress *comp, long *l)
{
	memcpy (l, &comp->sls_packets, 8 * sizeof (long));
}
