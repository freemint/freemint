/*
 *	IP masquerading support for MiNTNet.
 *
 *	Started 05/10/1999 Mario Becroft.
 *      07/01/99 support for ftp protocol by Torsten Lang
 */

# include "masquerade.h"

# include "icmp.h"
# include "inetutil.h"
# include "ip.h"
# include "masqdev.h"
# include "tcputil.h"
# include "udp.h"



# define MBDEBUG TRACE

# define after(seq_num1, seq_num2) (((long)(seq_num1)-(long)(seq_num2))>0 /* seq_num1 after seq_num2? */)
# define offset(seq_num) (after((seq_num), db_record->seq) ? db_record->offs : db_record->prev_offs)


MASQ_GLOBAL_INFO masq =
{
	magic:			MASQ_MAGIC,
	version:		MASQ_VERSION,	
	addr:			0,
	mask:			0,
	flags:			MASQ_DEFRAG_ALL,
	tcp_first_timeout:	200UL * 60 * 1,
	tcp_ack_timeout:	200UL * 60 * 60,
	tcp_fin_timeout:	200UL * 255 * 2,
	udp_timeout:		200UL * 60 * 1,
	icmp_timeout:		200UL * 60 * 1,
	
	redirection_db:		NULL
};

static int ftp_modifier (BUF **buf, ulong localaddr);

typedef struct {
	ushort port;
	int (*data_modify_fn)(BUF **buf, ulong localaddr);
} data_modifier;

static const data_modifier tcp_data_modifier[] =
{
	{ 21, ftp_modifier },
	{  0, NULL }
};

void
masq_init (void)
{
	int i;
	
	masqdev_init ();
	
	for (i = 0; i < MASQ_NUM_PORTS; i++)
		masq.port_db[i] = NULL;
	
	MBDEBUG (("masq_init: initialisation finished"));
	
	c_conws ("IP  masquerading by Mario Becroft, 1999.\r\n");
	c_conws ("FTP masquerading support by Torsten Lang, 1999.\r\n");
}

BUF *
masq_ip_input (struct netif *nif, BUF *buf)
{
	struct ip_dgram *iph = (struct ip_dgram *) buf->dstart;
	struct ip_dgram *orig_iph = iph;
	ushort src_port = 0;
	ushort dst_port = 0;
	PORT_DB_RECORD *db_record = NULL;
	struct tcp_dgram *tcph = (struct tcp_dgram *) IP_DATA (buf);
	struct udp_dgram *uh = (struct udp_dgram *) IP_DATA (buf);
	struct icmp_dgram *icmph = (struct icmp_dgram *) IP_DATA (buf);
	struct route *rt;
	ulong localaddr;
	short addrtype;
	struct sockaddr_in *in;
	
	UNUSED(nif);
	if (!(masq.flags & MASQ_ENABLED))
		return buf;
	
	MBDEBUG (("masq_ip_input: incoming datagram"));
	if (masq.flags & MASQ_DEFRAG_ALL)
		buf = ip_defrag( buf );
	MBDEBUG (("masq_ip_input: done defrag"));
	
	if (!buf)
		return buf;
	MBDEBUG (("masq_ip_input: defragged"));
	
	rt = route_get (iph->daddr);
	if (rt)
	{
		addrtype = ip_chk_addr (iph->daddr, rt);
		in = &(if_af2ifaddr(rt->nif, AF_INET)->adr.in);
		localaddr = in->sin_addr.s_addr;
		route_deref (rt);
	}
	else
	{
		addrtype = 0;
		localaddr = 0x7F000001;	/* If no route found, use local address */
	}
	MBDEBUG (("masq_ip_input: got route"));
	
	if (iph->proto == IPPROTO_TCP)
		src_port = tcph->dstport;
	else if (iph->proto == IPPROTO_UDP)
		src_port = uh->dstport;
	else if (iph->proto == IPPROTO_ICMP)
	{
		switch (icmph->type)
		{
			case ICMPT_ECHORP:
			case ICMPT_TIMERP:
			case ICMPT_INFORP:
			case ICMPT_MASKRP:
				src_port = icmph->u.echo.id;
				break;
			case ICMPT_DSTUR:
			case ICMPT_SRCQ:
			case ICMPT_REDIR:
			case ICMPT_TIMEX:
			case ICMPT_PARAMP:
				orig_iph = (struct ip_dgram *)(icmph+1);
				if (orig_iph->proto == IPPROTO_TCP)
				{
					tcph = (struct tcp_dgram *)orig_iph->data;
					src_port = tcph->srcport;
				}
				else if (orig_iph->proto == IPPROTO_UDP)
				{
					uh = (struct udp_dgram *)orig_iph->data;
					src_port = uh->srcport;
				}
				else
					return buf;
				break;
			default:
				src_port = 0;	/* Other incoming messages should not be modified */
		}
	}
	else
		return buf;	/* We do not understand other protocols */
	
	MBDEBUG (("masq_ip_input: port is %u", src_port));
	if (addrtype == IPADDR_LOCAL && ((db_record = find_redirection (src_port)) || (src_port >= MASQ_BASE_PORT && src_port < MASQ_BASE_PORT + MASQ_NUM_PORTS)))
	{
		/* This is an incoming packet for a masqueraded machine */
		MBDEBUG (("masq_ip_input: datagram is destined to a masqueraded destination"));
		if (!db_record)
			db_record = masq.port_db[src_port - MASQ_BASE_PORT];
		
		if (!db_record)
		{
			/*
			If no record was found for this port, we have no choice but to
			send a "destination unreachable" message. (The record must have
			expired while the connection was still open, or something wierd is
			going on.)
			*/
			MBDEBUG (("masq_ip_input: ALERT: no record for port"));
			icmp_send (ICMPT_DSTUR, ICMPC_PORTUR, iph->saddr, buf, 0);
			return NULL;
		}
		
		db_record->modified = MASQ_TIME;
		
		/* Change destination port to masqueraded port */
		if (iph->proto == IPPROTO_TCP)
		{
			if (!db_record->dst_port)
			{
				db_record->dst_port = tcph->srcport;
				db_record->seq = tcph->ack;
			}
			tcph->dstport = db_record->masq_port;
			tcph->ack -= offset(tcph->ack-db_record->offs);
			tcph->chksum = 0;
			tcph->chksum = tcp_checksum (tcph, (long)buf->dend - (long)tcph, iph->saddr, db_record->masq_addr);
			
			if (tcph->flags & TCPF_SYN)
				db_record->timeout = masq.tcp_ack_timeout;
			if ((tcph->flags & TCPF_FIN) || (tcph->flags & TCPF_RST))
				db_record->timeout = masq.tcp_fin_timeout;
		}
		else if (iph->proto == IPPROTO_UDP)
		{
			uh->dstport = db_record->masq_port;
			uh->chksum = 0;
			uh->chksum = udp_checksum (uh, iph->saddr, db_record->masq_addr);
		}
		else if (iph->proto == IPPROTO_ICMP)
		{
			short datalen;
			switch (icmph->type)
			{
				case ICMPT_ECHORP:
				case ICMPT_TIMERP:
				case ICMPT_INFORP:
				case ICMPT_MASKRP:
					icmph->u.echo.id = db_record->masq_port;
					icmph->chksum = 0;
					datalen = (long)buf->dend - (long)icmph;
					if (datalen & 1) *buf->dend = 0;
					icmph->chksum = chksum (icmph, (datalen+1)/2);
					break;
				case ICMPT_DSTUR:
				case ICMPT_SRCQ:
				case ICMPT_REDIR:
				case ICMPT_TIMEX:
				case ICMPT_PARAMP:
					if (orig_iph->proto == IPPROTO_TCP)
					{
						tcph->srcport = db_record->masq_port;
						tcph->seq    -= offset(tcph->seq-db_record->offs);
						tcph->chksum = 0;
						/*tcph->chksum = tcp_checksum( tcph, (long)buf->dend-(long)tcph, orig_iph->saddr, orig_iph->daddr );*/
						orig_iph->saddr = db_record->masq_addr;
						orig_iph->chksum = 0;
						orig_iph->chksum = chksum (orig_iph, orig_iph->hdrlen * sizeof (short));
						icmph->chksum = 0;
						datalen = (long)buf->dend - (long)icmph;
						if (datalen & 1) *buf->dend = 0;
						icmph->chksum = chksum (icmph, (datalen+1)/2);
					}
					else if (orig_iph->proto == IPPROTO_UDP)
					{
						uh->srcport = db_record->masq_port;
						uh->chksum = 0;
						uh->chksum = udp_checksum ( uh, db_record->masq_addr, orig_iph->daddr );
						orig_iph->saddr = db_record->masq_addr;
						orig_iph->chksum = 0;
						orig_iph->chksum = chksum (orig_iph, orig_iph->hdrlen * sizeof (short));
						icmph->chksum = 0;
						datalen = (long)buf->dend - (long)icmph;
						if (datalen & 1) *buf->dend = 0;
						icmph->chksum = chksum (icmph, (datalen+1)/2);
					}
					break;
			}
		}
		
		/* Change destination address to masqueraded address */
		iph->daddr = db_record->masq_addr;
		
		/* Recalculate IP header checksum */
		iph->chksum = 0;
		iph->chksum = chksum (iph, iph->hdrlen * sizeof (short));
	}
	else if (masq.addr == (iph->saddr & masq.mask))
	{
		ushort lport = 0;	/* To keep gcc happy */
		
		/* This is an outgoing packet from a masqueraded machine */
		MBDEBUG (("masq_ip_input: datagram is from a masqueraded source"));
		
		/* Do not masquerade packets destined to myself */
		if (addrtype == IPADDR_LOCAL && !(masq.flags & MASQ_LOCAL_PACKETS))
			return buf;
		
		if (iph->proto == IPPROTO_TCP)
		{
			int i;
			src_port = tcph->srcport;
			dst_port = tcph->dstport;
			
			/* 07/01/99 TL support for data modification */
			for (i = 0; tcp_data_modifier[i].data_modify_fn; i++)
			{
				if (tcp_data_modifier[i].port == dst_port)
				{
					int delta;
					
					delta = tcp_data_modifier[i].data_modify_fn (&buf, localaddr);
					if (!buf)
						return NULL;
					if (delta)
					{
						/* update info */
						orig_iph = 
						iph      = (struct ip_dgram *) buf->dstart;
						tcph     = (struct tcp_dgram *) IP_DATA (buf);
						uh       = (struct udp_dgram *) IP_DATA (buf);
						icmph    = (struct icmp_dgram *) IP_DATA (buf);
					}
					db_record = find_redirection_by_masq_port (src_port);
					if (!db_record)
						db_record = find_port_record (iph->saddr, src_port, dst_port, iph->proto);
					/* normally we HAVE to find the db_record since the connection */
					/* is already open! So the first part is only for security.    */
					if (db_record && delta && after (tcph->seq, db_record->seq))
					{
						db_record->prev_offs = db_record->offs;
						db_record->offs     += delta;
						db_record->seq       = tcph->seq;
					}
					
					break;
				}
			}
			/* 07/01/99 TL end support for data modification */
		}
		else if (iph->proto == IPPROTO_UDP)
			src_port = uh->srcport;
		else if (iph->proto == IPPROTO_ICMP)
		{
			switch (icmph->type)
			{
				case ICMPT_ECHORQ:
				case ICMPT_TIMERQ:
				case ICMPT_INFORQ:
				case ICMPT_MASKRP:
					src_port = icmph->u.echo.id;
					break;
				default:
					return buf;	/* Other outgoing messages should not be modified */
			}
		}
		
		db_record = find_redirection_by_masq_port (src_port);
		if (db_record)
			lport = db_record->num;
		if (!db_record)
		{
			db_record = find_port_record (iph->saddr, src_port, dst_port, iph->proto);
			if (db_record)
				lport = MASQ_BASE_PORT + db_record->num;
		}
		
		if (!db_record)
		{
			MBDEBUG (("masq_ip_input: no record found, creating one"));
			db_record = new_port_record ();
			if (!db_record)
			{
				/* Panic - no more port database entries
				 * available. We hope that the sender will
				 * resend the packet soon, by which time some
				 * entries may have become available.
				 */
				buf_deref (buf, BUF_NORMAL);
				return NULL;
			}
			db_record->masq_addr = iph->saddr;
			db_record->masq_port = src_port;
			db_record->dst_port = dst_port;
			db_record->proto = iph->proto;
			db_record->seq = 0;
			switch (iph->proto)
			{
				/* 07/05/99 TL "break"s are missing */
				case IPPROTO_TCP:	db_record->timeout = masq.tcp_first_timeout; db_record->seq = tcph->seq; break;
				case IPPROTO_UDP:	db_record->timeout = masq.udp_timeout;       break;
				case IPPROTO_ICMP:	db_record->timeout = masq.icmp_timeout;      break;
			}
			db_record->offs = 0;
			db_record->prev_offs = 0;
			lport = MASQ_BASE_PORT + db_record->num;
		}
		db_record->modified = MASQ_TIME;
		
		/* Change source port to our port */
		if (iph->proto == IPPROTO_TCP)
		{
			tcph->srcport = lport;
			tcph->seq += offset (tcph->seq);
			tcph->chksum = 0;
			tcph->chksum = tcp_checksum (tcph, (long)buf->dend - (long)tcph, localaddr, iph->daddr);
			
			if (tcph->flags & TCPF_SYN)
				db_record->timeout = masq.tcp_ack_timeout;
			if ((tcph->flags & TCPF_FIN) || (tcph->flags & TCPF_RST))
				db_record->timeout = masq.tcp_fin_timeout;
		}
		else if (iph->proto == IPPROTO_UDP)
		{
			uh->srcport = lport;
			uh->chksum = 0;
			uh->chksum = udp_checksum (uh, localaddr, iph->daddr);
		}
		else if (iph->proto == IPPROTO_ICMP)
		{
			short datalen;
			switch (icmph->type)
			{
				case ICMPT_ECHORQ:
				case ICMPT_TIMERQ:
				case ICMPT_INFORQ:
				case ICMPT_MASKRP:
					icmph->u.echo.id = lport;
					icmph->chksum = 0;
					datalen = (long)buf->dend - (long)icmph;
					if (datalen & 1) *buf->dend = 0;
					icmph->chksum = chksum (icmph, (datalen+1)/2);
					break;
				default:
					return buf;	/* Other outgoing messages should not be modified */
			}
		}
		
		/* Change source address to our address */
		iph->saddr = localaddr;
		
		/* Recalculate IP header checksum */
		iph->chksum = 0;
		iph->chksum = chksum (iph, iph->hdrlen * sizeof (short));
	}
	
	MBDEBUG (("masq_ip_input: successfully exited"));
	return buf;
}

PORT_DB_RECORD *
find_port_record (ulong addr, ushort src_port, ushort dst_port, uchar proto)
{
	int i;
	
	for (i = 0; i < MASQ_NUM_PORTS; i++)
		if (addr     == masq.port_db[i]->masq_addr   && 
		     src_port == masq.port_db[i]->masq_port   &&
		    (proto    != IPPROTO_TCP                ||
		     !dst_port                              || 
		     dst_port == masq.port_db[i]->dst_port)   &&
		     proto    == masq.port_db[i]->proto)
			return masq.port_db[i];
	
	return NULL;
}

PORT_DB_RECORD *
new_port_record (void)
{
	int i;
	PORT_DB_RECORD *record;
	
	for (i = 0; i < MASQ_NUM_PORTS; i++)
	{
		if (masq.port_db[i])
		{
			MBDEBUG (("masq_ip_input: deleting an old record to make room for a new one"));
			if ((long)(masq.port_db[i]->modified + masq.port_db[i]->timeout - MASQ_TIME) < 0)
				delete_port_record( masq.port_db[i] );
		}
		if (!masq.port_db[i])
		{
			record = kmalloc (sizeof (*record));
			if (record)
			{
				bzero (record, sizeof (*record));
				record->num = i;
				
				masq.port_db[i] = record;
			} else
			{
				MBDEBUG	(("masq_ip_input: ALERT: could not allocate storage for new record"));
			}

			return record;
		}
	}
	
	return NULL;
}

void
delete_port_record (PORT_DB_RECORD *record)
{
	if (/* record->num >= 0 && */ record->num < MASQ_NUM_PORTS)
		masq.port_db[record->num] = NULL;
	kfree (record);
}

void
purge_port_records (void)
{
	int i;
	
	for (i = 0; i < MASQ_NUM_PORTS; i++)
		if (masq.port_db[i])
			if (masq.port_db[i]->modified + masq.port_db[i]->timeout < MASQ_TIME)
				delete_port_record( masq.port_db[i] );
}

PORT_DB_RECORD *
new_redirection (void)
{
	PORT_DB_RECORD *record;
	
	record = kmalloc (sizeof (*record));
	if (record)
	{
		bzero (record, sizeof (*record));
		
		record->next_port = masq.redirection_db;
		masq.redirection_db = record;
	}
	
	return record;
}

void
delete_redirection (PORT_DB_RECORD *record)
{
	PORT_DB_RECORD *cur;
	
	/* Find previous record */
	cur = masq.redirection_db;
	while (cur)
	{
		if (cur->next_port == record)
			break;
		cur = cur->next_port;
	}
	
	if (cur)		/* If there is a previous record then */
		cur->next_port = record->next_port;	/* set it's next pointer. */
	else										/* Or if this is the first record then */
		masq.redirection_db = record->next_port;	/* adjust base pointer. */
	
	kfree (record);
}

PORT_DB_RECORD *
find_redirection (ushort port)
{
	PORT_DB_RECORD *record;
	
	record = masq.redirection_db;
	while (record)
	{
		if (record->num == port)
			break;
		
		record = record->next_port;
	}
	
	return record;
}

PORT_DB_RECORD *
find_redirection_by_masq_port (ushort port)
{
	PORT_DB_RECORD *record;
	
	record = masq.redirection_db;
	while (record)
	{
		if (record->masq_port == port)
			break;
		
		record = record->next_port;
	}
	
	return record;
}

/* 07/01/99 TL ftp support */
static int
ftp_modifier (BUF **buf, ulong localaddr)
{
	struct ip_dgram *iph = (struct ip_dgram *)(*buf)->dstart;
	struct tcp_dgram *tcph = (struct tcp_dgram *)IP_DATA (*buf);
	PORT_DB_RECORD *db_record = NULL;
	PORT_DB_RECORD *new_db_record;
	char *data       = TCP_DATA(tcph);
	char *data_start = 0; /* Keep gcc happy */
	char tmpbuf[24]; /* "xxx,xxx,xxx,xxx,yyy,yyy" */
	int  tmpbuf_len;
	long delta = 0;
	ulong  ftp_from;
	ushort ftp_port;
	ushort local_port;
	
	/* check if port command is issued */
	while (data < (*buf)->dend-17)
	{
		/* check for "PORT" command */
		if (memcmp (data, "PORT ", 5) && memcmp (data, "port ", 5))
		{
			data ++;
			continue;
		}
		
		/* set data_start to starting address of port info */
		data_start = data+5;
		
		/* check for ip address */
 		ftp_from = strtoul (data_start, &data, 10);
		if (*data != ',')
			continue;
		ftp_from = (ftp_from << 8) | strtoul (data+1, &data, 10);
		if (*data != ',')
			continue;
		ftp_from = (ftp_from << 8) | strtoul (data+1, &data, 10);
		if (*data != ',')
			continue;
		ftp_from = (ftp_from << 8) | strtoul (data+1, &data, 10);
		if (*data != ',')
			continue;
		
		/* check for port number */
		ftp_port = strtoul (data+1, &data, 10);
		if (*data != ',')
			continue;
		ftp_port = (ftp_port << 8) | strtoul (data+1, &data, 10);
		if (*data != '\r' && *data != '\n')
			continue;
		
		db_record = find_port_record (iph->saddr, tcph->srcport, tcph->dstport, iph->proto);
		
		/* There MUST already be an entry! */
		if (!db_record)
		{
			buf_deref (*buf, BUF_NORMAL);
			*buf = NULL;
			return 0;
		}
		
		/* create masquerading db entry for incoming data */
		if (after (tcph->seq, db_record->seq))
		{
			/* This was no retry, so create a new entry */
			new_db_record = new_port_record ();
			if (!new_db_record)
			{
				/* Panic - no more port database entries
				 * available. We hope that the sender will
				 * resend the packet soon, by which time some
				 * entries may have become available.
				 */
				buf_deref (*buf, BUF_NORMAL);
				*buf = NULL;
				return 0;
			}
			
			new_db_record->masq_addr = iph->saddr;
			new_db_record->masq_port = ftp_port;
			new_db_record->dst_port = 0;
			new_db_record->proto = iph->proto;
			
			/* We use the long timeout here since we wait for the first packet from the server */
			new_db_record->timeout = masq.tcp_ack_timeout;
			new_db_record->offs = 0;
			new_db_record->prev_offs=0;
			new_db_record->seq = 0;
			local_port = MASQ_BASE_PORT + new_db_record->num;
			new_db_record->modified = MASQ_TIME;
		}
		else if ((new_db_record = find_port_record (iph->saddr, ftp_port, 0, iph->proto)))
			/* This was a retry, so find the previously created entry */
			local_port = MASQ_BASE_PORT + new_db_record->num;
		else
		{
			/* This should NOT happen */
			buf_deref (*buf, BUF_NORMAL);
			*buf = NULL;
			return 0;
		}
		
		tmpbuf_len = ksprintf (tmpbuf,
		                     "%ld,%ld,%ld,%ld,%ld,%ld",
		                     (localaddr>>24) & 255,
		                     (localaddr>>16) & 255,
		                     (localaddr>> 8) & 255,
		                      localaddr      & 255,
		                     (ulong)((local_port >> 8) & 255),
		                     (ulong)( local_port       & 255));
		
		/* recalculate datagram length */
		delta = tmpbuf_len - ((long) data - (long) data_start);
		iph->length += delta;
		/* adjust other params in the packet */
		if (tmpbuf_len > (long) data - (long) data_start)
		{
			BUF *newbuf;
			long data_offs;
			
			data_offs = (long) data_start - (long) (*buf)->dstart;
			
			/* reallocate buffer */
			newbuf = buf_reserve(*buf, tmpbuf_len-((long)data-(long)data_start), BUF_RESERVE_START);
			
			/* if there's no memory left just drop the packet */
			if (!newbuf)
			{
				buf_deref (*buf, BUF_NORMAL);
				*buf = NULL;
				return 0;
			}
			
			/* move data */
			*buf = newbuf;
			memcpy ((*buf)->dstart - (tmpbuf_len - ((long) data - (long) data_start)), (*buf)->dstart, data_offs);
			
			/* recalculate start of data and pointer to port data */
			(*buf)->dstart -= (tmpbuf_len - ((long) data - (long) data_start));
			data_start = (*buf)->dstart+data_offs;
		}
		else
		{
			memcpy(data_start+tmpbuf_len, data, (long) (*buf)->dend - (long) data);
			(*buf)->dend -= ((long)data - (long) data_start) - tmpbuf_len;
		}
		
		/* now copy our data into the ip data */
		memcpy (data_start, tmpbuf, tmpbuf_len);

		break;
	}
	
	return delta;
}
/* 07/01/99 TL end ftp support */
