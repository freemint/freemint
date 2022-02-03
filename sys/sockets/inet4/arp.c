/*
 *	ARP (rfc 826) and RARP (rfc 903) implementation.
 *
 *	12/13/94, Kay Roemer.
 */

# include "arp.h"

# include "arpdev.h"
# include "in.h"
# include "route.h"

# include "mint/sockio.h"


static void	arp_timeout (long);

/*
 * Hash tables for ARP and RARP.
 */
struct arp_entry *arptab[ARP_HASHSIZE];
struct arp_entry *rarptab[ARP_HASHSIZE];


static short
arp_hash (uchar *addr, short len)
{
	ulong v;
	
	v = *(ulong *) addr;
	if (len < 4)
		v >>= (32 - (len << 3));
	
	return v % ARP_HASHSIZE;
}

/*
 * Remove an arp cache entry from the internal hash tables and free it
 * if no longer referenced afterwards.
 */
static void
arp_remove (struct arp_entry *are)
{
	struct arp_entry *curr, **prev;
	short idx;
	
	if (!(are->flags & ATF_PRCOM))
		return;
	
	idx = arp_hash (are->praddr.adr.bytes, are->praddr.len);
	prev = &arptab[idx];
	for (curr = *prev; curr; prev = &curr->prnext, curr = *prev)
	{
		if (curr == are)
		{
			*prev = curr->prnext;
			arp_deref (are);
			break;
		}
	}
}

static void
rarp_remove (struct arp_entry *are)
{
	struct arp_entry *curr, **prev;
	short idx;
	
	if (!(are->flags & ATF_HWCOM))
		return;
	
	idx = arp_hash (are->hwaddr.adr.bytes, are->hwaddr.len);
	prev = &rarptab[idx];
	for (curr = *prev; curr; prev = &curr->hwnext, curr = *prev)
	{
		if (curr == are)
		{
			*prev = curr->hwnext;
			arp_deref (are);
			break;
		}
	}
}

/*
 * Insert an entry into internal hash table.
 */
# ifdef notused
static void
arp_put (struct arp_entry *are)
{
	short idx;
	
	are->links++;
	idx = arp_hash(are->praddr.addr, are->praddr.len);
	are->prnext = arptab[idx];
	arptab[idx] = are;
}
# endif

static void
rarp_put (struct arp_entry *are)
{
	short idx;
	
	are->links++;
	idx = arp_hash (are->hwaddr.adr.bytes, are->hwaddr.len);
	are->hwnext = rarptab[idx];
	rarptab[idx] = are;
}

/*
 * Flush all ARP entries belonging to interface 'nif'
 */
void
arp_flush (struct netif *nif)
{
	struct arp_entry **prev, *curr, *next;
	short i;
	
	for (i = 0; i < ARP_HASHSIZE; i++)
	{
		/*
		 * delete from arptab
		 */
		prev = &arptab[i];
		for (curr = *prev; curr; curr = next)
		{
			next = curr->prnext;
			if (curr->nif == nif)
			{
				*prev = next;
				arp_deref (curr);
			}
			else
				prev = &curr->prnext;
		}
		
		/*
		 * delete from rarptab
		 */
		prev = &rarptab[i];
		for (curr = *prev; curr; curr = next)
		{
			next = curr->hwnext;
			if (curr->nif == nif)
			{
				*prev = next;
				arp_deref (curr);
			}
			else
				prev = &curr->hwnext;
		}
	}
}

static struct arp_entry *
arp_alloc (void)
{
	struct arp_entry *are;
	
	are = kmalloc (sizeof (*are));
	if (are)
	{
		mint_bzero (are, sizeof (*are));
		are->outq.maxqlen = IF_MAXQ;
	} else
	{
		DEBUG (("arp_alloc: out of memory"));
	}

	return are;
}

void
arp_free (struct arp_entry *are)
{
	are->nif->out_errors += are->outq.qlen;
	if_flushq (&are->outq);
	event_del (&are->tmout);
	kfree (are);
}

/*
 * Get the local protocol address. This function must know how
 * to convert betweem ARP protocol type and address family.
 */
static char *
arp_myaddr (struct netif *nif, short type)
{
	switch (type)
	{
		case ARPRTYPE_IP:
		{
			struct ifaddr *ifa;
			
			ifa = if_af2ifaddr (nif, AF_INET);
			if (!ifa)
				break;
			
			return (char *)&ifa->adr.in.sin_addr.s_addr;
		}
	}
	
	DEBUG (("arp_myaddr: unkown ARP protocol type: %d", type));
	return 0;
}

static void
arp_dosend (struct netif *nif, BUF *buf, short pktype)
{
	struct arp_dgram *arph = (struct arp_dgram *) buf->dstart;
	
	if ((nif->flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
	{
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	(*nif->output)(nif, buf, (char *)ARP_DHW(arph), arph->hwlen, pktype);
}

/*
 * Send an ARP request to resolve the entry 'are'
 */
static void
arp_sendreq (struct arp_entry *are)
{
	struct arp_dgram *arphdr;
	long hwlen, prlen, size;
	char *myaddr;
	BUF *arpbuf;
	
	if (++are->retries > ARP_RETRIES)
	{
		short flags = are->flags;
		DEBUG (("arp_send: timeout after %d retries", are->retries-1));
		arp_remove (are);
		if (flags & ATF_HWCOM)
			rarp_remove (are);
		return;
	}
	myaddr = arp_myaddr (are->nif, are->prtype);
	if (!myaddr)
		return;
	
	hwlen = are->hwaddr.len;
	prlen = are->praddr.len;
	size = sizeof (*arphdr) + 2 * (prlen + hwlen);
	
	arpbuf = buf_alloc (size + ARP_RESERVE, ARP_RESERVE/2, BUF_NORMAL);
	if (!arpbuf)
	{
		DEBUG (("arp_send: no memory for arp request"));
		return;
	}
	
	arpbuf->dend += size;
	arphdr = (struct arp_dgram *)arpbuf->dstart;
	arphdr->hwtype = are->hwtype;
	arphdr->prtype = are->prtype;
	arphdr->hwlen = hwlen;
	arphdr->prlen = prlen;
	arphdr->op = AROP_ARPREQ;
	
	memcpy (ARP_SPR(arphdr), myaddr, prlen);
	memcpy (ARP_SHW(arphdr), are->nif->hwlocal.adr.bytes, hwlen);
	memcpy (ARP_DPR(arphdr), are->praddr.adr.bytes, prlen);
	memcpy (ARP_DHW(arphdr), are->nif->hwbrcst.adr.bytes, hwlen);
	
	event_del (&are->tmout);
	event_add (&are->tmout, ARP_RETRANS, arp_timeout, (long)are);
	arp_dosend (are->nif, arpbuf, PKTYPE_ARP);
}

void
arp_timeout (long arg)
{
	struct arp_entry *are = (struct arp_entry *) arg;
	
	if (ATF_ISCOM (are))
	{
		/*
		 * This is an resolved entry which has timed out.
		 * Delete it.
		 * Double check whether this was a static entry.
		 * (The check should never fail).
		 */
		if (!(are->flags & ATF_PERM))
		{
			arp_remove (are);
			rarp_remove (are);
		}
	}
	else if (are->flags & ATF_PRCOM)
	{
		/*
		 * This is an unresolved ARP entry for which no
		 * answer arrived yet. Send a retransmit.
		 */
		arp_sendreq (are);
	}
}

/*
 * Look for a protocol address in the ARP cache and return it.
 * Depending on 'flags' create a new entry when not found and resolve
 * it.
 *
 * Note: if nif == 0 then don't check whether the interfaces match. Used
 * for looking up proxy entries.
 */
struct arp_entry *
arp_lookup (short flags, struct netif *nif, short type, short len, char *addr)
{
	struct arp_entry *are;
	short idx;
	
	idx = arp_hash ((unsigned char *)addr, len);
	for (are = arptab[idx]; are; are = are->prnext)
	{
		if (are->flags & ATF_PRCOM &&
		    are->prtype == (ushort)type &&
		    (!nif || are->nif == nif) &&
		    !memcmp (addr, are->praddr.adr.bytes, len))
			break;
	}
	
	if ((are && (++are->links, 1)) || flags & ARLK_NOCREAT || !nif)
		return are;
	
	are = arp_alloc ();
	if (!are)
		return NULL;
	
	are->links = 2;
	are->flags = ATF_PRCOM;
	
	are->hwtype = nif->hwtype;
	are->hwaddr.len = nif->hwlocal.len;
	
	are->prtype = type;
	are->praddr.len = len;
	memcpy (are->praddr.adr.bytes, addr, len);
	
	are->nif = nif;
	
	are->prnext = arptab[idx];
	arptab[idx] = are;
	
	if (!(flags & ARLK_NORESOLV))
		arp_sendreq (are);
	
	return are;
}

/*
 * Lookup hardware address in the RARP cache and return found entry.
 * If 'nif' is passed as zero then don't match interfaces.
 */
struct arp_entry *
rarp_lookup (short flags, struct netif *nif, short type, short len, char *addr)
{
	struct arp_entry *are;
	short idx;
	
	UNUSED(flags);
	idx = arp_hash ((unsigned char *)addr, len);
	for (are = rarptab[idx]; are; are = are->hwnext)
	{
		if (are->flags & ATF_HWCOM
			&& are->hwtype == (ushort)type
			&& (!nif || are->nif == nif)
			&& !memcmp (addr, are->hwaddr.adr.bytes, len))
		{
			are->links++;
			break;
		}
	}
	
	return are;
}

/*
 * Send the packets on the ARP queue of a newly resolved entry
 */
static void
arp_sendq (struct arp_entry *are)
{
	BUF *b;
	
	if ((are->nif->flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
	{
		DEBUG (("arp_sendq: if down, flushing arp queue"));
		if_flushq (&are->outq);
		return;
	}
	
	/*
	 * XXX: We should not assume PKTYPE_IP here ...
	 */
	while ((b = if_dequeue (&are->outq)))
	{
		(*are->nif->output)(are->nif, b, (char *)are->hwaddr.adr.bytes,
			are->hwaddr.len, PKTYPE_IP);
	}
}

/*
 * Deal with incoming ARP packets.
 */
void
arp_input (struct netif *nif, BUF *buf)
{
	char *myaddr, *spr, *dpr, *shw, *dhw;
	struct arp_dgram *arphdr;
	struct arp_entry *are;
	short forme;
	unsigned long len;
	
	len = buf->dend - buf->dstart;
	arphdr = (struct arp_dgram *)buf->dstart;
	
	DEBUG (("arp_input..."));
	/*
	 * Validate incoming packet.
	 */
	if (len < sizeof (*arphdr) || len != ARP_LEN (arphdr))
	{
		DEBUG (("arp_input: bad length"));
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	/*
	 * Check if we can handle this packet, if not discard.
	 */
	myaddr = arp_myaddr (nif, arphdr->prtype);
	if (arphdr->hwtype != (ushort)nif->hwtype
		|| arphdr->hwlen != nif->hwlocal.len
		|| (nif->flags & IFF_NOARP)
		|| myaddr == 0)
	{
	    	DEBUG (("arp_input: not for me..."));
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	spr = (char *)ARP_SPR (arphdr);
	dpr = (char *)ARP_DPR (arphdr);
	shw = (char *)ARP_SHW (arphdr);
	dhw = (char *)ARP_DHW (arphdr);
	forme = !memcmp (myaddr, dpr, arphdr->prlen);
	
	/*
	 * If my address is requested then create an entry if non is found,
	 * otherwise only look for a pending entry but don't create a new
	 * one if not found.
	 */
	are = arp_lookup (forme ? ARLK_NORESOLV : ARLK_NOCREAT,
		nif, arphdr->prtype, arphdr->prlen, spr);
	if (are)
	{
		/*
		 * Update sender's hardware address
		 */
		rarp_remove (are);
		memcpy (are->hwaddr.adr.bytes, shw, arphdr->hwlen);
		are->flags |= ATF_HWCOM;
		rarp_put (are);
		event_del (&are->tmout);
		if (!(are->flags & ATF_PERM))
			event_add (&are->tmout, ARP_EXPIRE, arp_timeout,
				(long)are);
		/*
		 * Send all accumulated packets
		 */
		arp_sendq (are);
		arp_deref (are);
	}
	if (arphdr->op == AROP_ARPREQ)
	{
		arphdr->op = AROP_ARPREP;
		memcpy (dhw, shw, arphdr->hwlen);
		if (forme)
		{
			/*
			 * My address was requested, answer it.
			 */
			memcpy (dpr, spr, arphdr->prlen);
			memcpy (spr, myaddr, arphdr->prlen);
			memcpy (shw, nif->hwlocal.adr.bytes, arphdr->hwlen);
			buf_ref (buf);
			arp_dosend (nif, buf, PKTYPE_ARP);
		}
		else
		{
			/*
			 * See if we have proxy entries for the requested
			 * address and answer if so.
			 */
			are = arp_lookup (ARLK_NOCREAT, 0, arphdr->prtype,
				arphdr->prlen, dpr);
			if (are && are->flags & ATF_PUBL && ATF_ISCOM (are))
			{
				memcpy (dpr, spr, arphdr->prlen);
				memcpy (spr, are->praddr.adr.bytes, arphdr->prlen);
				memcpy (shw, are->hwaddr.adr.bytes, arphdr->hwlen);
				buf_ref (buf);
				arp_dosend (nif, buf, PKTYPE_ARP);
			}
			
			if (are)
				arp_deref (are);
		}
	}
	
	buf_deref (buf, BUF_NORMAL);
}

/*
 * Deal with incoming RARP packets.
 */
void
rarp_input (struct netif *nif, BUF *buf)
{
	struct arp_dgram *arphdr;
	struct arp_entry *are;
	char *myaddr;
	unsigned long len;
	
	len = buf->dend - buf->dstart;
	arphdr = (struct arp_dgram *) buf->dstart;
	
	/*
	 * Validate incoming packet
	 */
	if (len < sizeof (*arphdr) || len != ARP_LEN (arphdr))
	{
		DEBUG (("rarp_input: invalid length"));
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	/*
	 * Check if we can handle it
	 */
	myaddr = arp_myaddr (nif, arphdr->prtype);
	if (arphdr->op != AROP_RARPREQ
		|| arphdr->hwlen != nif->hwlocal.len
		|| arphdr->hwtype != (ushort)nif->hwtype
		|| (nif->flags & IFF_NOARP)
		|| myaddr == 0)
	{
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	/*
	 * See if we have a matching entry.
	 */
	are = rarp_lookup (0, nif, arphdr->hwtype, arphdr->hwlen,
		(char *)ARP_SHW (arphdr));
	if (are && (are->flags & (ATF_PRCOM|ATF_NORARP)) == ATF_PRCOM
		&& are->prtype == arphdr->prtype)
	{
		/*
		 * Source is our interface hw/pr address, destination is
		 * senders hw and the looked up pr address.
		 */
		arphdr->op = AROP_RARPREP;
		memcpy (ARP_DPR (arphdr), are->praddr.adr.bytes, arphdr->prlen);
		memcpy (ARP_DHW (arphdr), ARP_SHW (arphdr), arphdr->hwlen);
		memcpy (ARP_SPR (arphdr), myaddr, arphdr->prlen);
		memcpy (ARP_SHW (arphdr), nif->hwlocal.adr.bytes, arphdr->hwlen);
		buf_ref (buf);
		arp_dosend (nif, buf, PKTYPE_RARP);
	}
	
	if (are)
		arp_deref (are);
	
	buf_deref (buf, BUF_NORMAL);
}

/*
 * Helper functions for arp_ioctl ()
 */
static long
get_praddr (struct hwaddr *hwaddr, struct sockaddr *sockaddr, short *type)
{
	switch (sockaddr->sa_family)
	{
		case AF_INET:
		{
			struct sockaddr_in *in;
			
			in = (struct sockaddr_in *)sockaddr;
			hwaddr->len = 4;
			memcpy (hwaddr->adr.bytes, &in->sin_addr.s_addr, 4);
			*type = ARPRTYPE_IP;
			
			return 0;
		}
	}
	
	return EINVAL;
}

# ifdef notused
static long
put_praddr (struct hwaddr *hwaddr, struct sockaddr *sockaddr, short type)
{
	switch (type)
	{
		case ARPRTYPE_IP:
		{
			struct sockaddr_in *sin;
			
			sin = (struct sockaddr_in *)sockaddr;
			sin->sin_family = AF_INET;
			sin->sin_port = 0;
			sin->sin_addr.s_addr = *(ulong *)hwaddr->addr;
			
			return 0;
		}
	}
	
	return EINVAL;
}
# endif

static long
get_hwaddr (struct hwaddr *hwaddr, struct sockaddr *sockaddr, short *type)
{
	struct sockaddr_hw *shw = (struct sockaddr_hw *) sockaddr;
	
	if (shw->shw_family == AF_LINK &&
	    shw->shw_len <= sizeof (hwaddr->adr.bytes))
	{
		hwaddr->len = shw->shw_len;
		memcpy (hwaddr->adr.bytes, shw->shw_adr.bytes, shw->shw_len);
		*type = shw->shw_type;
		
		return 0;
	}
	
	return EINVAL;
}

static long
put_hwaddr (struct hwaddr *hwaddr, struct sockaddr *sockaddr, short type)
{
	struct sockaddr_hw *shw = (struct sockaddr_hw *) sockaddr;
	
	shw->shw_family = AF_LINK;
	shw->shw_type = type;
	shw->shw_len = hwaddr->len;
	memcpy (shw->shw_adr.bytes, hwaddr->adr.bytes, MIN (sizeof (shw->shw_adr), (unsigned short)hwaddr->len));
	
	return 0;
}

/*
 * Handle ARP ioctl's
 */
long
arp_ioctl (short cmd, void *arg)
{
	struct arp_req *areq = (struct arp_req *) arg;
	
	switch (cmd)
	{
		case SIOCSARP:
		{
			struct arp_entry *are;
			struct netif *nif;
			struct route *rt;
			struct hwaddr praddr, hwaddr;
			short prtype, hwtype;
			
			if (p_geteuid ())
				return EACCES;
			
			if (get_praddr (&praddr, &areq->praddr, &prtype) ||
			    get_hwaddr (&hwaddr, &areq->hwaddr, &hwtype))
				return EINVAL;
			
			if (prtype != ARPRTYPE_IP)
				return EAFNOSUPPORT;
			/*
			 * Get interface to attach arp entry to
			 */
			rt = route_get(praddr.adr.longs[0]);// (*(long *)praddr.addr);
			if (rt == 0 || rt->flags & RTF_GATEWAY)
			{
				if (rt) route_deref (rt);
				return ENETUNREACH;
			}
			nif = rt->nif;
			route_deref (rt);
			
			/*
			 * Lookup/create arp entry
			 */
			are = arp_lookup (ARLK_NORESOLV, nif, ARPRTYPE_IP, praddr.len, (char *)praddr.adr.bytes);
			if (!are)
				return ENOMEM;
			
			rarp_remove (are);
			are->hwtype = hwtype;
			are->hwaddr = hwaddr;
			are->flags |= ATF_HWCOM;
			are->flags &= ~ATF_USRMASK;
			are->flags |= areq->flags & ATF_USRMASK;
			rarp_put (are);
			event_del (&are->tmout);
			if (!(are->flags & ATF_PERM))
				event_add (&are->tmout, ARP_EXPIRE, arp_timeout,
					(long)are);
			/*
			 * Send all accumulated packets (if any)
			 */
			arp_sendq (are);
			arp_deref (are);
			
			return 0;
		}
		case SIOCDARP:
		{
			struct arp_entry *are;
			struct hwaddr praddr;
			short prtype;
			
			if (p_geteuid ())
				return EACCES;
			
			if (get_praddr (&praddr, &areq->praddr, &prtype))
				return EINVAL;		
			
			are = arp_lookup (ARLK_NOCREAT, 0, ARPRTYPE_IP, praddr.len, (char *)praddr.adr.bytes);
			if (!are)
				return ENOENT;
			
			arp_remove (are);
			rarp_remove (are);
			arp_deref (are);
			
			return 0;
		}
		case SIOCGARP:
		{
			struct arp_entry *are;
			struct hwaddr praddr;
			short prtype;
			
			if (get_praddr (&praddr, &areq->praddr, &prtype))
				return EINVAL;
			
			are = arp_lookup (ARLK_NOCREAT, 0, ARPRTYPE_IP, praddr.len, (char *)praddr.adr.bytes);
			if (!are || !(are->flags & ATF_HWCOM))
			{
				if (are) arp_deref (are);
				return ENOENT;
			}
			
			put_hwaddr (&are->hwaddr, &areq->hwaddr, are->hwtype);
			areq->flags = are->flags;
			arp_deref (are);
			
			return 0;
		}
	}
	
	return ENOSYS;
}

long
arp_init (void)
{
	return arpdev_init ();
}

/*
 * Debugging stuff
 */
# ifdef notused
static void
arp_dump (struct arp_entry *are)
{
	uchar *cp;
	
	DEBUG (("flags = 0x%x, links = %d", are->flags, are->links));
	DEBUG (("pr = 0x%x, hw = 0x%x", are->prtype, are->hwtype));
	
	if (are->flags & ATF_PRCOM)
	{
		cp = are->praddr.addr;
		DEBUG (("praddr = %x:%x:%x:%x:%x:%x", cp[0],cp[1],cp[2],cp[3],
			cp[4],cp[5]));
	}
	
	if (are->flags & ATF_HWCOM)
	{
		cp = are->hwaddr.addr;
		DEBUG (("hwaddr = %x:%x:%x:%x:%x:%x", cp[0],cp[1],cp[2],cp[3],
			cp[4],cp[5]));
	}
}
# endif
