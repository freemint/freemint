/*
 *	ARP/RARP structures and definitions.
 *
 *	12/11/94, Kay Roemer.
 */

# ifndef _arp_h
# define _arp_h

# include "mint/socket.h"
# include "global.h"

# include "if.h"
# include "ifeth.h"

# include "buf.h"
# include "timer.h"


/*
 * ARP request/reply packet
 */
struct arp_dgram
{
	ushort	hwtype;
	ushort	prtype;
	uchar	hwlen;
	uchar	prlen;
	ushort	op;
	uchar	data[0];
};

# define ARP_SHW(a)	((a)->data)
# define ARP_SPR(a)	((a)->data + (a)->hwlen)
# define ARP_DHW(a)	((a)->data + (a)->hwlen + (a)->prlen)
# define ARP_DPR(a)	((a)->data + 2*(a)->hwlen + (a)->prlen)
# define ARP_LEN(a)	(sizeof(struct arp_dgram) + 2*((a)->hwlen + (a)->prlen))

/* possible hardware types for arp_dgram.hwtype */
# define ARHWTYPE_ETH	1

/* possible protocol types for arp_dgram.prtype */
# define ARPRTYPE_IP	ETHPROTO_IP

/* possible op-codes for arp_dgram.op */
# define AROP_ARPREQ	1
# define AROP_ARPREP	2
# define AROP_RARPREQ	3
# define AROP_RARPREP	4

/*
 * ARP cache node
 */
struct arp_entry
{
	ushort		links;		/* reference count */
	ushort		flags;		/* ATF_* flags */
	
	ushort		hwtype;		/* hardware type */
	ushort		prtype;		/* protocol type */
	struct hwaddr	praddr;		/* protocol address */
	struct hwaddr	hwaddr;		/* hardware address */

	struct netif	*nif;		/* iface this entry was obtained from*/
	struct ifq	outq;		/* queue of packets for this addr */

	struct event	tmout;		/* retransmit/timeout event */
	ushort		retries;	/* number of retries so far */

	struct arp_entry *prnext;	/* link to next entry in pr chain */
	struct arp_entry *hwnext;	/* link to next entry in hw chain */
};

# define ARP_RETRIES	4			/* 4 retries before giving up*/
# define ARP_RETRANS	(1000L / EVTGRAN)	/* retry after 1 sec */
# define ARP_EXPIRE	(1200000L / EVTGRAN)	/* remove entry after 20 min */
# define ARP_HASHSIZE	47			/* hash table size */
# define ARP_RESERVE	100

/*
 * structure passed on ARP ioctl's
 */
struct arp_req
{
	struct sockaddr	praddr;		/* protocol address */
	struct sockaddr	hwaddr;		/* hardware address */
	ushort		flags;		/* ATF_* flags */
};

/* flags for arp_req.flags and arp_entry.flags */
# define ATF_PRCOM		0x01	/* pr entry resolved */
# define ATF_HWCOM		0x02	/* hw entry resolved */
# define ATF_PERM		0x04	/* static entry */
# define ATF_PUBL		0x08	/* proxy entry */
# define ATF_USETRAILERS	0x10	/* other side wants trailers */
# define ATF_NORARP		0x20	/* don't use this entry for RARP */
# define ATF_COM		(ATF_PRCOM|ATF_HWCOM)
# define ATF_USRMASK		(ATF_PUBL|ATF_PERM|ATF_NORARP)

/* is this entry resolved? */
# define ATF_ISCOM(are)	(((are)->flags & ATF_COM) == ATF_COM)

/*
 * arp hash table
 */
extern struct arp_entry *arptab[ARP_HASHSIZE];

long			arp_init (void);
void			arp_free (struct arp_entry *);
void			arp_input (struct netif *, BUF *);
void			rarp_input (struct netif *, BUF *);
void			arp_flush (struct netif *);
long			arp_ioctl (short, void *);
struct arp_entry *	arp_lookup (short flags, struct netif *, short type, short len, char *);
struct arp_entry *	rarp_lookup (short flags, struct netif *, short type, short len, char *);

/* flags for arp_lookup() */
# define ARLK_NOCREAT	0x01	/* don't create+resolve entry if not found */
# define ARLK_NORESOLV	0x02	/* don't resolve entry if not found */

INLINE void
arp_deref (struct arp_entry *are)
{
	if (--are->links <= 0)
		arp_free (are);
}


# endif /* _arp_h */
