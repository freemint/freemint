/*
 *	Inet utility function. They deal with creating, destroing,
 *	putting into, removing from and looking up inet socket datas.
 *
 *	At the momenet all datas belonging to one proto are kept in
 *	a list. This slows down searching for a special one, perhaps
 *	we should use some hashing scheme ?
 *
 *	01/21/94, kay roemer.
 */

# include "inetutil.h"

# include "if.h"
# include "in.h"
# include "ip.h"
# include "port.h"

# include "buf.h"

# include "mint/file.h"
# include "mint/socket.h"


struct in_proto *allinetprotos = NULL;

void
in_proto_register (short protonum, struct in_proto *proto)
{
	UNUSED(protonum);
	proto->next = allinetprotos;
	allinetprotos = proto;
	ip_register (&proto->ipops);
}

struct in_proto *
in_proto_lookup (short protocol)
{
	struct in_proto *p;
	
	for (p = allinetprotos; p; p = p->next)
	{
		if (p->proto == protocol)
			break;
	}
	
	return p;
}

struct in_data *
in_data_create (void)
{
	struct in_data *data;
	
	data = kmalloc (sizeof (*data));
	if (!data)
	{
		DEBUG (("in_data_create: Out of mem"));
		return 0;
	}
	bzero (data, sizeof (*data));
	
	// data->protonum = 0;
	// data->proto = 0;
	// data->sock = 0;
	// data->next = 0;
	// data->pcb = 0;
	
	// bzero (&data->opts, sizeof (data->opts));
	data->opts.ttl = IP_DEFAULT_TTL;
	data->opts.tos = IP_DEFAULT_TOS;
	
	data->snd.maxdatalen = IN_DEFAULT_WSPACE;
	// data->snd.curdatalen = 0;
	// data->snd.lowat = 0;
	// data->snd.hiwat = 0;
	// data->snd.flags = 0;
	// data->snd.qfirst = 0;
	// data->snd.qlast = 0;
	
	data->rcv.maxdatalen = IN_DEFAULT_RSPACE;
	// data->rcv.curdatalen = 0;
	// data->rcv.lowat = 0;
	// data->rcv.hiwat = 0;
	// data->rcv.flags = 0;
	// data->rcv.qfirst = 0;
	// data->rcv.qlast = 0;
	
	// data->src.port = 0;
	// data->src.addr = 0;
	
	// data->dst.port = 0;
	// data->dst.addr = 0;
	
	data->flags = IN_BROADCAST|IN_CHECKSUM;
	// data->backlog = 0;
	// data->linger = 0;
	// data->err = 0;
	
	return data;
}

long
in_data_destroy (struct in_data *data, short wait)
{
	short proto;
	
	if (!data)
	{
		DEBUG (("in_data_destroy: data == NULL"));
		return 0;
	}
	
	proto = data->protonum;
	if (wait && data->flags & IN_LINGER)
	{
		while (data->snd.curdatalen > 0)
		{
			if (isleep (IO_Q, (long)data->sock))
			{
				DEBUG (("in_data_destroy: interrupted."));
				if (!in_data_find (proto, data))
					return EINTR;
			}
		}
	}
	
	if (data->proto)
		in_data_remove (data);
	
	in_data_flush (data);
	kfree (data);
	return 0;
}

void
in_data_flush (struct in_data *data)
{
	BUF *buf, *nextbuf;
	
	for (buf = data->rcv.qfirst; buf; buf = nextbuf)
	{
		nextbuf = buf->next;
		buf_deref (buf, BUF_NORMAL);
	}
	
	data->rcv.qfirst = data->rcv.qlast = 0;
	data->rcv.curdatalen = 0;
	
	for (buf = data->snd.qfirst; buf; buf = nextbuf)
	{
		nextbuf = buf->next;
		buf_deref (buf, BUF_NORMAL);
	}
	
	data->snd.qfirst = data->snd.qlast = 0;
	data->snd.curdatalen = 0;
}

void
in_data_put (struct in_data *data)
{
	data->next = data->proto->datas;
	data->proto->datas = data;
}

void
in_data_remove (struct in_data *data)
{
	struct in_data *d = data->proto->datas;
	
	if (d == data)
		data->proto->datas = data->next;
	else
	{
		while (d && d->next != data)
			d = d->next;
		
		if (d)
			d->next = data->next;
		else
		{
			DEBUG (("in_remove: Data not found"));
		}
	}
	
	data->next = 0;
}

short
in_data_find (short proto, struct in_data *data)
{
	struct in_proto *p = in_proto_lookup (proto);
	struct in_data *d;
	
	if (!p) return 0;
	
	for (d = p->datas; d; d = d->next)
		if (data == d)
			return 1;
	
	return 0;
}

struct in_data *
in_data_lookup (struct in_data *datas, ulong srcaddr, ushort srcport,
					ulong dstaddr, ushort dstport)
{
	struct in_data *deflt, *dead, *d = datas;
	
	for (dead = deflt = 0; d; d = d->next)
	{
		if (d->flags & IN_ISBOUND && d->src.port == dstport
			&& ip_same_addr (d->src.addr, dstaddr))
		{
			if (d->flags & IN_ISCONNECTED)
			{
				if (d->dst.port == srcport
					&& ip_same_addr (srcaddr, d->dst.addr))
				{
					if (!(d->flags & IN_DEAD))
				    		break;
					dead = d;
				}
			}
			else if (!deflt || deflt->src.addr == INADDR_ANY)
			{
				/*
				 * prefer exact matches over INADDR_ANY
				 */
				deflt = d;
			}
		}
	}
	
	return (d ? d : (deflt ? deflt : dead));
}

/*
 * Incrementally search the next matching data
 */
struct in_data *
in_data_lookup_next (struct in_data *datas, ulong srcaddr, ushort srcport,
						ulong dstaddr, ushort dstport, short wildcard)
{
	struct in_data *d;
	
	for (d = datas; d; d = d->next)
	{
		if (d->flags & IN_ISBOUND && d->src.port == dstport
			&& ip_same_addr (d->src.addr, dstaddr))
		{
			if (d->flags & IN_ISCONNECTED)
			{
				if (d->dst.port == srcport
					&& ip_same_addr (srcaddr, d->dst.addr))
				{
					break;
				}
			}
			else if (wildcard)
				break;
		}
	}
	
	return d;
}

/*
 * Generic checksum computation routine.
 * Returns the one's complement of the 16 bit one's complement sum over
 * the `nwords' words starting at `buf'.
 */
short
chksum (void *buf, short nwords)
{
	ulong sum = 0;

	__asm__(
		"\tclrl	%%d0\n"
#ifdef __mcoldfire__
		"\tmvzw	%2, %%d1\n"
		"\tlsrl	#1, %%d1\n"	/* # of longs in buf */
#else
		"\tmovew	%2, %%d1\n"
		"\tlsrw	#1, %%d1\n"	/* # of longs in buf */
#endif
		"\tbcc	l1\n"		/* multiple of 4 ? */
#ifdef __mcoldfire__
		"\tmvz.w	%1@+, %%d2\n"
		"\taddl	%%d2, %0\n"	/* no, add in extra word */
		"\taddxl	%%d0, %0\n"
#else
		"\taddw	%1@+, %0\n"	/* no, add in extra word */
		"\taddxw	%%d0, %0\n"
#endif
		"l1:\n"
#ifdef __mcoldfire__
		"\tsubql	#1, %%d1\n"	/* decrement for dbeq */
#else
		"\tsubqw	#1, %%d1\n"	/* decrement for dbeq */
#endif
		"\tbmi	l3\n"
		"l2:\n"
		"\taddl	%1@+, %0\n"
		"\taddxl	%%d0, %0\n"
#ifdef __mcoldfire__
		"\tsubql	#1, %%d1\n"
		"\tbpls	l2\n"	/* loop over all longs */
#else
		"\tdbra	%%d1, l2\n"		/* loop over all longs */
#endif
		"l3:\n"
#ifdef __mcoldfire__
		"\tswap	%0\n"		/* convert to short */
		"\tmvzw	%0, %%d1\n"
		"\tclr.w	%0\n"
		"\tswap	%0\n"
		"\taddl	%%d1, %0\n"
		"\tswap	%0\n"
		"\tmvzw	%0, %%d1\n"
		"\tclr.w	%0\n"
		"\tswap	%0\n"
		"\taddl	%%d1, %0\n"
#else
		"\tmovel	%0, %%d1\n"	/* convert to short */
		"\tswap	%%d1\n"
		"\taddw	%%d1, %0\n"
		"\taddxw	%%d0, %0\n"
#endif
		: "=d"(sum), "=a"(buf)
		: "g"(nwords), "1"(buf), "0"(sum)
#ifdef __mcoldfire__
		: "d0", "d1", "d2"
#else
		: "d0", "d1"
#endif
		);
	
	return (short)(~sum & 0xffff);
}

void
sa_copy (struct sockaddr *sa1, struct sockaddr *sa2)
{
	short todo;
	
	switch (sa2->sa_family)
	{
		case AF_INET:
			todo = sizeof (struct sockaddr_in);
			break;
		default:	
			todo = sizeof (struct sockaddr);
	}
	
	memcpy (sa1, sa2, todo);
}
