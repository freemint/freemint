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
 * begin:	2000-04-18
 * last change:	2000-04-18
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _tcputil_h
# define _tcputil_h

# include "tcp.h"


long		tcp_isn		(void);
struct tcb *	tcb_alloc	(void);
void		tcb_free	(struct tcb *);
void		tcb_delete	(struct tcb *);
void		tcb_wait	(struct tcb *);
void		tcb_reset	(struct tcb *, long);
void		tcb_error	(struct tcb *, long);
long		tcp_sndrst	(BUF *);
long		tcp_sndack	(struct tcb *, BUF *);
short		tcp_valid	(struct tcb *, BUF *);
long		tcp_options	(struct tcb *, struct tcp_dgram *);
long		tcp_mss		(struct tcb *, ulong faddr, long);
ushort		tcp_checksum	(struct tcp_dgram *, ushort, ulong, ulong);
void		tcp_dump	(BUF *);

/*
 * Some heavily used one-liners as inlines.
 */

INLINE short
tcp_finished (struct tcb *tcb)
{
	return (tcb->flags & TCBF_FIN && SEQLT (tcb->seq_fin, tcb->rcv_nxt));
}

/* Return the length of the TCP segment in `buf'. */
INLINE long
tcp_seglen (BUF *buf, struct tcp_dgram *tcph)
{
	long len;
	
	len = (long) buf->dend - (long) tcph - tcph->hdrlen * 4;
	
	if (tcph->flags & TCPF_SYN)
		++len;
	if (tcph->flags & TCPF_FIN)
		++len;
	
	return len;
}

INLINE long
tcp_datalen (BUF *buf, struct tcp_dgram *tcph)
{
	return ((long) buf->dend - (long) tcph - tcph->hdrlen * 4);
}


# endif /* _tcputil_h */
