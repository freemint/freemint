/*
 *	10 Mbit Ethernet support functions, exported to packet drivers
 *	through struct netinfo.
 *
 *	12/14/94, Kay Roemer.
 */

# include "ifeth.h"

# include "arp.h"
# include "ip.h"


BUF *
eth_build_hdr (BUF *buf, struct netif *nif, char *addr, short type)
{
	struct eth_dgram *ep;
	BUF *nbuf;
	long len;

	DEBUG (("eth_build_hdr( buf=0x%lx, nif='%s', type=%d )", buf, nif->name, type));
	
	len = buf->dend - buf->dstart;
	if (len > ETH_MAX_DLEN)
	{
		buf_deref (buf, BUF_NORMAL);
		return NULL;
	}
	
	nbuf = buf_reserve (buf, sizeof (*ep), BUF_RESERVE_START);
	if (!nbuf)
	{
		buf_deref (buf, BUF_NORMAL);
		return NULL;
	}
	
	nbuf->dstart -= sizeof (*ep);
	ep = (struct eth_dgram *) nbuf->dstart;
	memcpy (ep->saddr, nif->hwlocal.addr, ETH_ALEN);
	memcpy (ep->daddr, addr, ETH_ALEN);
	ep->proto = (type == ETHPROTO_8023) ? (short) len : type;
	
	return nbuf;
}

short
eth_remove_hdr (BUF *buf)
{
	struct eth_dgram *ep = (struct eth_dgram *) buf->dstart;
	long len, nlen;
	short type;
	
	type = (ep->proto >= 1536) ? ep->proto : ETHPROTO_8023;
	buf->dstart += sizeof (*ep);
	
	DEBUG (("eth_remove_hdr( buf=0x%lx, type=%d )", buf, type));

	/*
	 * Correct packet length for to short packets. (Ethernet
	 * requires all packets to be padded to at least 60 bytes)
	 */
	len = buf->dend - buf->dstart;
	switch (type)
	{
		case ETHPROTO_IP:
		{
			struct ip_dgram *i = (struct ip_dgram *) buf->dstart;
			nlen = i->length;
			break;
		}
		case ETHPROTO_ARP:
		case ETHPROTO_RARP:
		{
			struct arp_dgram *a = (struct arp_dgram *) buf->dstart;
			nlen = ARP_LEN (a);
			break;
		}
		default:
		{
			nlen = len;
			break;
		}
	}
	
	if (nlen < len)
		buf->dend = buf->dstart + nlen;
	
	return type;
}
