/*
 * Native Features ethernet driver.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2002-2006 Standa and Petr of ARAnyM dev team.
 *
 * based on dummy.xif skeleton 12/14/94, Kay Roemer.
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
 */

# include <osbind.h>

# include "buf.h"
# include "inet4/if.h"
# include "inet4/ifeth.h"
# include "netinfo.h"

# include "mint/sockio.h"
# include "mint/arch/nf_ops.h"

# include "global.h"
# include "nfeth_nfapi.h"


#define XIF_NAME	"Native Features Ethernet driver v0.7"

#define MAX_ETH		4

/* old handler */
extern void (*old_interrupt)(void);

/* interrupt wrapper routine */
void my_interrupt (void);

/* the C routine handling the interrupt */
void _cdecl nfeth_interrupt(void);

long driver_init (void);

unsigned long inet_aton(const char *cp, long *addr);

/*
 * Our interface structure
 */
static struct netif ifaces[MAX_ETH];



/* ================================================================ */
static unsigned long nfEtherID;
long __CDECL (*nf_call)(long id, ...) = 0UL;

/* ================================================================ */

static inline unsigned long get_nfapi_version(void)
{
	return nf_call(ETH(GET_VERSION));
}

static inline unsigned long get_int_level(void)
{
	return nf_call(ETH(XIF_INTLEVEL));
}

static inline unsigned long
get_hw_addr( int ethX, char *buffer, int len )
{
	return nf_call(ETH(XIF_GET_MAC), (unsigned long)ethX, buffer, (unsigned long)len);
}

static inline unsigned long
nfInterrupt( short bit_mask )
{
	return nf_call(ETH(XIF_IRQ), (unsigned long)bit_mask);
}

static inline short
read_packet_len( int ethX )
{
	return nf_call(ETH(XIF_READLENGTH), (unsigned long)ethX);
}

static inline void
read_block( int ethX, char *cp, short len )
{
	nf_call(ETH(XIF_READBLOCK), (unsigned long)ethX, cp, (unsigned long)len);
}

static inline void
send_block( int ethX, char *cp, short len )
{
	nf_call(ETH(XIF_WRITEBLOCK), (unsigned long)ethX, cp, (unsigned long)len);
}

static void
nfeth_install_int(void)
{
# define vector(x)      (x / 4)
	old_interrupt = Setexc(vector(0x60) + get_int_level(), (long) my_interrupt);
}


/*
 * This gets called when someone makes an 'ifconfig up' on this interface
 * and the interface was down before.
 */
static long neth_open(struct netif *nif)
{
	int ethX = nif->unit;
	DEBUG (("nfeth: open (nif = %08lx)", (long)nif));
	return nf_call(ETH(XIF_START), (unsigned long)ethX);
}

/*
 * Opposite of neth_open(), is called when 'ifconfig down' on this interface
 * is done and the interface was up before.
 */
static long neth_close(struct netif *nif)
{
	int ethX = nif->unit;
	return nf_call(ETH(XIF_STOP), (unsigned long)ethX);
}

/*
 * This routine is responsible for enqueing a packet for later sending.
 * The packet it passed in `buf', the destination hardware address and
 * length in `hwaddr' and `hwlen' and the type of the packet is passed
 * in `pktype'.
 *
 * `hwaddr' is guaranteed to be of type nif->hwtype and `hwlen' is
 * garuanteed to be equal to nif->hwlocal.len.
 *
 * `pktype' is currently one of (definitions in if.h):
 *	PKTYPE_IP for IP packets,
 *	PKTYPE_ARP for ARP packets,
 *	PKTYPE_RARP for reverse ARP packets.
 *
 * These constants are equal to the ethernet protocol types, ie. an
 * Ethernet driver may use them directly without prior conversion to
 * write them into the `proto' field of the ethernet header.
 *
 * If the hardware is currently busy, then you can use the interface
 * output queue (nif->snd) to store the packet for later transmission:
 *	if_enqueue (&nif->snd, buf, buf->info).
 *
 * `buf->info' specifies the packet's delivering priority. if_enqueue()
 * uses it to do some priority queuing on the packets, ie. if you enqueue
 * a high priority packet it may jump over some lower priority packets
 * that were already in the queue (ie that is *no* FIFO queue).
 *
 * You can dequeue a packet later by doing:
 *	buf = if_dequeue (&nif->snd);
 *
 * This will return NULL is no more packets are left in the queue.
 *
 * The buffer handling uses the structure BUF that is defined in buf.h.
 * Basically a BUF looks like this:
 *
 * typedef struct {
 *	long buflen;
 *	char *dstart;
 *	char *dend;
 *	...
 *	char data[0];
 * } BUF;
 *
 * The structure consists of BUF.buflen bytes. Up until BUF.data there are
 * some header fields as shown above. Beginning at BUF.data there are
 * BUF.buflen - sizeof (BUF) bytes (called userspace) used for storing the
 * packet.
 *
 * BUF.dstart must always point to the first byte of the packet contained
 * within the BUF, BUF.dend points to the first byte after the packet.
 *
 * BUF.dstart should be word aligned if you pass the BUF to any MintNet
 * functions! (except for the buf_* functions itself).
 *
 * BUF's are allocated by
 *	nbuf = buf_alloc (space, reserve, mode);
 *
 * where `space' is the size of the userspace of the BUF you need, `reserve'
 * is used to set BUF.dstart = BUF.dend = BUF.data + `reserve' and mode is
 * one of
 *	BUF_NORMAL for calls from kernel space,
 *	BUF_ATOMIC for calls from interrupt handlers.
 *
 * buf_alloc() returns NULL on failure.
 *
 * Usually you need to pre- or postpend some headers to the packet contained
 * in the passed BUF. To make sure there is enough space in the BUF for this
 * use
 *	nbuf = buf_reserve (obuf, reserve, where);
 *
 * where `obuf' is the BUF where you want to reserve some space, `reserve'
 * is the amount of space to reserve and `where' is one of
 *	BUF_RESERVE_START for reserving space before BUF.dstart
 *	BUF_RESERVE_END for reserving space after BUF.dend
 *
 * Note that buf_reserve() returns pointer to a new buffer `nbuf' (possibly
 * != obuf) that is a clone of `obuf' with enough space allocated. `obuf'
 * is no longer existant afterwards.
 *
 * However, if buf_reserve() returns NULL for failure then `obuf' is
 * untouched.
 *
 * buf_reserve() does not modify the BUF.dstart or BUF.dend pointers, it
 * only makes sure you have the space to do so.
 *
 * In the worst case (if the BUF is to small), buf_reserve() allocates a new
 * BUF and copies the old one to the new one (this is when `nbuf' != `obuf').
 *
 * To avoid this you should reserve enough space when calling buf_alloc(), so
 * buf_reserve() does not need to copy. This is what MintNet does with the BUFs
 * passed to the output function, so that copying is never needed. You should
 * do the same for input BUFs, ie allocate the packet as eg.
 *	buf = buf_alloc (nif->mtu+sizeof (eth_hdr)+100, 50, BUF_ATOMIC);
 *
 * Then up to nif->mtu plus the length of the ethernet header bytes long
 * frames may ne received and there are still 50 bytes after and before
 * the packet.
 *
 * If you have sent the contents of the BUF you should free it by calling
 *	buf_deref (`buf', `mode');
 *
 * where `buf' should be freed and `mode' is one of the modes described for
 * buf_alloc().
 *
 * Functions that can be called from interrupt:
 *	buf_alloc (..., ..., BUF_ATOMIC);
 *	buf_deref (..., BUF_ATOMIC);
 *	if_enqueue ();
 *	if_dequeue ();
 *	if_input ();
 *	eth_remove_hdr ();
 *	addroottimeout (..., ..., 1);
 */

static long neth_output(struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	BUF *nbuf;

	/*
	 * Attach eth header. MintNet provides you with the eth_build_hdr
	 * function that attaches an ethernet header to the packet in
	 * buf. It takes the BUF (buf), the interface (nif), the hardware
	 * address (hwaddr) and the packet type (pktype).
	 *
	 * Returns NULL if the header could not be attached (the passed
	 * buf is thrown away in this case).
	 *
	 * Otherwise a pointer to a new BUF with the packet and attached
	 * header is returned and the old buf pointer is no longer valid.
	 */
	nbuf = eth_build_hdr (buf, nif, hwaddr, pktype);
	if (nbuf == 0)
	{
		nif->out_errors++;
		return ENOMEM;
	}
	nif->out_packets++;

	/*
	 * Here you should either send the packet to the hardware or
	 * enqueue the packet and send the next packet as soon as
	 * the hardware is finished.
	 *
	 * If you are done sending the packet free it with buf_deref().
	 *
	 * Before sending it pass it to the packet filter.
	 */
	if (nif->bpf)
		bpf_input (nif, nbuf);

	/*
	 * Send packet
	 */
	{
		short len = nbuf->dend - nbuf->dstart;
		int ethX = nif->unit;
		DEBUG (("nfeth: send %d (0x%x) bytes", len, len));
		len = MAX (len, 60);

		send_block (ethX, nbuf->dstart, len);
	}

	buf_deref (nbuf, BUF_NORMAL);

	return 0;
}


/*
 * Interface configuration via SIOCSIFOPT. The ioctl is passed a
 * struct ifreq *ifr. ifr->ifru.data points to a struct ifopt, which
 * we get as the second argument here.
 *
 * If the user MUST configure some parameters before the interface
 * can run make sure that neth_open() fails unless all the necessary
 * parameters are set.
 *
 * Return values	meaning
 * ENOSYS		option not supported
 * ENOENT		invalid option value
 * 0			Ok
 */
static long neth_config(struct netif *nif, struct ifopt *ifo)
{
# define STRNCMP(s)	(strncmp ((s), ifo->option, sizeof (ifo->option)))

	if (!STRNCMP ("hwaddr"))
	{
		uchar *cp;
		/*
		 * Set hardware address
		 */
		if (ifo->valtype != IFO_HWADDR)
			return ENOENT;
		memcpy (nif->hwlocal.adr.bytes, ifo->ifou.v_string, ETH_ALEN);
		cp = nif->hwlocal.adr.bytes;
		UNUSED (cp);
		DEBUG (("dummy: hwaddr is %x:%x:%x:%x:%x:%x",
				cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]));
	}
	else if (!STRNCMP ("braddr"))
	{
		uchar *cp;
		/*
		 * Set broadcast address
		 */
		if (ifo->valtype != IFO_HWADDR)
			return ENOENT;
		memcpy (nif->hwbrcst.adr.bytes, ifo->ifou.v_string, ETH_ALEN);
		cp = nif->hwbrcst.adr.bytes;
		UNUSED (cp);
		DEBUG (("dummy: braddr is %x:%x:%x:%x:%x:%x",
				cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]));
	}
	else if (!STRNCMP ("debug"))
	{
		/*
		 * turn debuggin on/off
		 */
		if (ifo->valtype != IFO_INT)
			return ENOENT;
		DEBUG (("dummy: debug level is %ld", ifo->ifou.v_long));
	}
	else if (!STRNCMP ("log"))
	{
		/*
		 * set log file
		 */
		if (ifo->valtype != IFO_STRING)
			return ENOENT;
		DEBUG (("dummy: log file is %s", ifo->ifou.v_string));
	}

	return ENOSYS;
}


/*
 * MintNet notifies you of some noteable IOCLT's. Usually you don't
 * need to act on them because MintNet already has done so and only
 * tells you that an ioctl happened.
 *
 * One useful thing might be SIOCGLNKFLAGS and SIOCSLNKFLAGS for setting
 * and getting flags specific to your driver. For an example how to use
 * them look at slip.c
 */
static long neth_ioctl(struct netif *nif, short cmd, long arg)
{
	char buffer[128];
	struct ifreq *ifr = (struct ifreq *)arg;
	long *data = ifr->ifru.data;
	int ethX = nif->unit;

	DEBUG (("nfeth: ioctl cmd = %d \"('%c'<<8)|%d\" bytes", cmd, cmd>>8, cmd&0xff));

	switch (cmd)
	{
		case SIOCGLNKSTATS:
			nf_call(ETH(XIF_GET_IPATARI), (unsigned long)ethX, buffer, sizeof(buffer));
			inet_aton(buffer, data++);
			nf_call(ETH(XIF_GET_IPHOST), (unsigned long)ethX, buffer, sizeof(buffer));
			inet_aton(buffer, data++);
			nf_call(ETH(XIF_GET_NETMASK), (unsigned long)ethX, buffer, sizeof(buffer));
			inet_aton(buffer, data++);
			return 0;

		case SIOCSIFNETMASK:
		case SIOCSIFFLAGS:
		case SIOCSIFADDR:
			return 0;

		case SIOCSIFMTU:
			/*
			 * Limit MTU to 1500 bytes. MintNet has already set nif->mtu
			 * to the new value, we only limit it here.
			 */
			if (nif->mtu > ETH_MAX_DLEN)
				nif->mtu = ETH_MAX_DLEN;
			return 0;

		case SIOCSIFOPT:
			/*
			 * Interface configuration, handled by neth_config()
			 */
			return neth_config (nif, ifr->ifru.data);
	}

	return ENOSYS;
}


/*
 * Initialization. This is called when the driver is loaded. If you
 * link the driver with main.o and init.o then this must be called
 * driver_init() because main() calles a function with this name.
 *
 * You should probe for your hardware here, setup the interface
 * structure and register your interface.
 *
 * This function should return 0 on success and != 0 if initialization
 * fails.
 */
long
driver_init (void)
{
	char message[100];
	/* static char my_file_name[128]; */
	int ethX;

	/* get the Ethernet NatFeat ID */
	nfEtherID = 0;
	if (MINT_KVERSION >= 2 && KERNEL->nf_ops != NULL)
		nfEtherID = KERNEL->nf_ops->get_id("ETHERNET");

	if ( nfEtherID == 0 ) {
		c_conws(XIF_NAME " not installed - NatFeat not found\r\n");
		return 1;
	}

	/* safe the nf_call pointer */
	nf_call = KERNEL->nf_ops->call;

	/* compare the version */
	if ( get_nfapi_version() != NFETH_NFAPI_VERSION ) {
		ksprintf (message, XIF_NAME " not installed - version mismatch: %ld != %d\r\n", get_nfapi_version(), NFETH_NFAPI_VERSION);
		c_conws(message);
		return 1;
	}

	/*
	 * registering ETHx
	 */
	strcpy(message, XIF_NAME " ("); /* for 'we are alive' message, see below */
	for(ethX=0; ethX<MAX_ETH; ethX++)
	{
		struct netif *iface = &(ifaces[ethX]);
		/*
	 	 * Set interface name
	 	 */
		strcpy (iface->name, "eth");
		/*
		 * Set interface unit to the ethX index and hope that
		 * it was unused (if_getfreeunit("name") returns a yet
		 * unused unit number for the interface type "name" but
		 * we really need the unit to be the ethX)
		 */
		iface->unit = ethX; /* required */
		/*
		 * Always set to zero
		 */
		iface->metric = 0;
		/*
		 * Initial interface flags, should be IFF_BROADCAST for
		 * Ethernet.
		 */
		iface->flags = IFF_BROADCAST;
		/*
		 * Maximum transmission unit, should be >= 46 and <= 1500 for
		 * Ethernet
		 */
		iface->mtu = 1500;
		/*
		 * Time in ms between calls to (*iface.timeout) ();
		 */
		iface->timer = 0;

		/*
		 * Interface hardware type
		 */
		iface->hwtype = HWTYPE_ETH;
		/*
		 * Hardware address length, 6 bytes for Ethernet
		 */
		iface->hwlocal.len = ETH_ALEN;
		iface->hwbrcst.len = ETH_ALEN;

		/*
		 * Set interface hardware and broadcast addresses.
		 */
		if (! get_hw_addr(ethX, (char *)iface->hwlocal.adr.bytes, ETH_ALEN))
			continue; /* if XIF_GET_MAC returns false then skip this interface */

		memcpy (iface->hwbrcst.adr.bytes, "\377\377\377\377\377\377", ETH_ALEN);

		/*
		 * Set length of send and receive queue. IF_MAXQ is a good value.
		 */
		iface->rcv.maxqlen = IF_MAXQ;
		iface->snd.maxqlen = IF_MAXQ;
		/*
		 * Setup pointers to service functions
		 */
		iface->open = neth_open;
		iface->close = neth_close;
		iface->output = neth_output;
		iface->ioctl = neth_ioctl;
		/*
		 * Optional timer function that is called every 200ms.
		 */
		iface->timeout = 0;

		/*
		 * Here you could attach some more data your driver may need
		 */
		iface->data = 0;

		/*
		 * Number of packets the hardware can receive in fast succession,
		 * 0 means unlimited.
		 */
		iface->maxpackets = 0;

		/*
		 * Register the interface.
		 */
		if_register (iface);

		ksprintf(message + strlen(message), " eth%d", ethX); /* for 'alive' msg */
	}

	/*
	 * Install the interface interrupt.
	 */
	nfeth_install_int();

	/*
	 * NETINFO->fname is a pointer to the drivers file name
	 * (without leading path), eg. "dummy.xif".
	 * NOTE: the file name will be overwritten when you leave the
	 * init function. So if you need it later make a copy!
	 */
/*
	if (NETINFO->fname)
	{
		strncpy (my_file_name, NETINFO->fname, sizeof (my_file_name));
		my_file_name[sizeof (my_file_name) - 1] = '\0';
	}
*/

	/*
	 * And say we are alive...
	 */
	strcat(message, " )\r\n");
	c_conws (message);
	return 0;
}


/*
 * Read a packet out of the adapter and pass it to the upper layers
 */
INLINE void
recv_packet (struct netif *nif)
{
	ushort pktlen;
	BUF *b;
	int ethX = nif->unit;

	/* read packet length (excluding 32 bit crc) */
	pktlen = read_packet_len(ethX);

	DEBUG (("nfeth: recv_packet: %i", pktlen));

	//if (pktlen < 32)
	if (!pktlen)
	{
		DEBUG (("nfeth: recv_packet: pktlen == 0"));
		nif->in_errors++;
		return;
	}

	b = buf_alloc (pktlen+100, 50, BUF_ATOMIC);
	if (!b)
	{
		DEBUG (("nfeth: recv_packet: out of mem (buf_alloc failed)"));
		nif->in_errors++;
		return;
	}
	b->dend += pktlen;

	read_block(ethX, b->dstart, pktlen);

	/* Pass packet to upper layers */
	if (nif->bpf)
		bpf_input (nif, b);

	/* and enqueue packet */
	if (!if_input (nif, b, 0, eth_remove_hdr (b)))
		nif->in_packets++;
	else
		nif->in_errors++;
}


/*
 * interrupt routine
 */
void _cdecl
nfeth_interrupt (void)
{
	static int in_use = 0;
	int ethX;

	if (in_use)
		return; /* primitive locking */
	in_use++;

	for(ethX = 0; ethX < MAX_ETH; ethX++) {
		int this_dev_irq_bit = 1 << ethX;
		int irq_for_eth_bitmask = nfInterrupt(0);
		if (this_dev_irq_bit & irq_for_eth_bitmask) {
			recv_packet (&ifaces[ethX]);
			nfInterrupt(this_dev_irq_bit);
		}
	}
	in_use = 0;
}

/*
 * Check whether "cp" is a valid ascii representation
 * of an Internet address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 * This replaces inet_addr, the return value from which
 * cannot distinguish between failure and a local broadcast address.
 */

unsigned long inet_aton(const char *cp, long *addr)
{
	register unsigned long val;
	int base;
	register int n;
	register char c;
	unsigned long parts[4];
	register unsigned long *pp = parts;

	c = *cp;
	for (;;) {
		/*
		 * Collect number up to ``.''.
		 * Values are specified as for C:
		 * 0x=hex, 0=octal, isdigit=decimal.
		 */
		if (!isdigit(c))
			goto ret_0;
		base = 10;
		if (c == '0') {
			c = *++cp;
			if (c == 'x' || c == 'X')
				base = 16, c = *++cp;
			else
				base = 8;
		}
		val = 0;
		for (;;) {
			if (isascii(c) && isdigit(c)) {
				val = (val * base) + (c - '0');
				c = *++cp;
			} else if (base == 16 && isascii(c) && isxdigit(c)) {
				val = (val << 4) |
					(c + 10 - (islower(c) ? 'a' : 'A'));
				c = *++cp;
			} else
				break;
		}
		if (c == '.') {
			/*
			 * Internet format:
			 *	a.b.c.d
			 *	a.b.c	(with c treated as 16 bits)
			 *	a.b	(with b treated as 24 bits)
			 */
			if (pp >= parts + 3)
				goto ret_0;
			*pp++ = val;
			c = *++cp;
		} else
			break;
	}
	/*
	 * Check for trailing characters.
	 */
	if (c != '\0' && (!isascii(c) || !isspace(c)))
		goto ret_0;
	/*
	 * Concoct the address according to
	 * the number of parts specified.
	 */
	n = pp - parts + 1;
	switch (n) {

	case 0:
		goto ret_0;		/* initial nondigit */

	case 1:				/* a -- 32 bits */
		break;

	case 2:				/* a.b -- 8.24 bits */
		if (parts[0] > 0xff || val > 0xffffff)
			goto ret_0;
		val |= parts[0] << 24;
		break;

	case 3:				/* a.b.c -- 8.8.16 bits */
		if (parts[0] > 0xff || parts[1] > 0xff || val > 0xffff)
			goto ret_0;
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;

	case 4:				/* a.b.c.d -- 8.8.8.8 bits */
		if (parts[0] > 0xff || parts[1] > 0xff || parts[2] > 0xff
		    || val > 0xff)
			goto ret_0;
		val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	}
	if (addr)
		*addr = val;

	return (1);

ret_0:
	return (0);
}
