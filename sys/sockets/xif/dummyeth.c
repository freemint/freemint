/*
 *	Dummy loopback eth driver for testing purposes. Also sceleton
 *	for ethernet packet drivers.
 *
 *	Usage:
 *		ifconfig eth0 addr u.v.w.x
 *		route add u.v.w.x eth0
 *	Then you can do ping u.v.w.x and somesuch and see what eg. ARP
 *	does...
 *
 *	12/14/94, Kay Roemer.
 */

# include "global.h"

# include "buf.h"
# include "inet4/if.h"
# include "inet4/ifeth.h"

# include "netinfo.h"

# include "mint/sockio.h"


/*
 * Our interface structure
 */
static struct netif if_dummy;

/*
 * Prototypes for our service functions
 */
static long	dummy_open	(struct netif *);
static long	dummy_close	(struct netif *);
static long	dummy_output	(struct netif *, BUF *, const char *, short, short);
static long	dummy_ioctl	(struct netif *, short, long);
static long	dummy_config	(struct netif *, struct ifopt *);

/*
 * This gets called when someone makes an 'ifconfig up' on this interface
 * and the interface was down before.
 */
static long
dummy_open (struct netif *nif)
{
	return 0;
}

/*
 * Opposite of dummy_open(), is called when 'ifconfig down' on this interface
 * is done and the interface was up before.
 */
static long
dummy_close (struct netif *nif)
{
	return 0;
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
 
static long
dummy_output (struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	BUF *nbuf;
	short type;
	long r;

	/*
	 * This is not needed in real hardware drivers. We test
	 * only if the destination hardware address is either our
	 * hw or our broadcast address, because we loop the packets
	 * back only then.
	 */
	if (memcmp (hwaddr, nif->hwlocal.adr.bytes, ETH_ALEN) &&
	    memcmp (hwaddr, nif->hwbrcst.adr.bytes, ETH_ALEN))
	{
		/*
		 * Not for me.
		 */
		buf_deref (buf, BUF_NORMAL);
		return 0;
	}
	
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
	 * Now follows the input side code of the driver. This is
	 * only part of the output function, because this example
	 * is a loopback driver.
	 */
	
	/*
	 * Before passing it to if_input pass it to the packet filter.
	 * (but before stripping the ethernet header).
	 *
	 * For the loopback driver this doesn't make sense... We
	 * would see all packets twice!
	 *
	 * if (nif->bpf)
	 *	bpf_input (nif, buf);
	 */
	
	/*
	 * Strip eth header and get packet type. MintNet provides you
	 * with the function eth_remove_hdr(buf) for this purpose where
	 * `buf' contains an ethernet frame. eth_remove_hdr strips the
	 * ethernet header and returns the packet type.
	 */
	type = eth_remove_hdr (nbuf);
	
	/*
	 * Then you should pass the buf to MintNet for further processing,
	 * using
	 *	if_input (nif, buf, 0, type);
	 *
	 * where `nif' is the interface the packet was received on, `buf'
	 * contains the packet and `type' is the packet type, which must
	 * be a valid ethernet protcol identifier.
	 *
	 * if_input takes `buf' over, so after calling if_input() on it
	 * you can no longer access it.
	 */
	r = if_input (nif, nbuf, 0, type);
	if (r)
		nif->in_errors++;
	else
		nif->in_packets++;
	
	return r;
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
static long
dummy_ioctl (struct netif *nif, short cmd, long arg)
{
	struct ifreq *ifr;
	
	switch (cmd)
	{
		case SIOCSIFNETMASK:
		case SIOCSIFFLAGS:
		case SIOCSIFADDR:
			return 0;
		
		case SIOCSIFMTU:
			/*
			 * Limit MTU to 1500 bytes. MintNet has alraedy set nif->mtu
			 * to the new value, we only limit it here.
			 */
			if (nif->mtu > ETH_MAX_DLEN)
				nif->mtu = ETH_MAX_DLEN;
			return 0;
		
		case SIOCSIFOPT:
			/*
			 * Interface configuration, handled by dummy_config()
			 */
			ifr = (struct ifreq *) arg;
			return dummy_config (nif, ifr->ifru.data);
	}
	
	return ENOSYS;
}

/*
 * Interface configuration via SIOCSIFOPT. The ioctl is passed a
 * struct ifreq *ifr. ifr->ifru.data points to a struct ifopt, which
 * we get as the second argument here.
 *
 * If the user MUST configure some parameters before the interface
 * can run make sure that dummy_open() fails unless all the necessary
 * parameters are set.
 *
 * Return values	meaning
 * ENOSYS		option not supported
 * ENOENT		invalid option value
 * 0			Ok
 */
static long
dummy_config (struct netif *nif, struct ifopt *ifo)
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
long driver_init(void);
long
driver_init (void)
{
	static char message[100];
	static char my_file_name[128];
	
	/*
	 * Set interface name
	 */
	strcpy (if_dummy.name, "en");
	/*
	 * Set interface unit. if_getfreeunit("name") returns a yet
	 * unused unit number for the interface type "name".
	 */
	if_dummy.unit = if_getfreeunit ("en");
	/*
	 * Alays set to zero
	 */
	if_dummy.metric = 0;
	/*
	 * Initial interface flags, should be IFF_BROADCAST for
	 * Ethernet.
	 */
	if_dummy.flags = IFF_BROADCAST;
	/*
	 * Maximum transmission unit, should be >= 46 and <= 1500 for
	 * Ethernet
	 */
	if_dummy.mtu = 1500;
	/*
	 * Time in ms between calls to (*if_dummy.timeout) ();
	 */
	if_dummy.timer = 0;
	
	/*
	 * Interface hardware type
	 */
	if_dummy.hwtype = HWTYPE_ETH;
	/*
	 * Hardware address length, 6 bytes for Ethernet
	 */
	if_dummy.hwlocal.len =
	if_dummy.hwbrcst.len = ETH_ALEN;
	
	/*
	 * Set interface hardware and broadcast addresses. For real ethernet
	 * drivers you must get them from the hardware of course!
	 */
	memcpy (if_dummy.hwlocal.adr.bytes, "\001\002\003\004\005\006", ETH_ALEN);
	memcpy (if_dummy.hwbrcst.adr.bytes, "\377\377\377\377\377\377", ETH_ALEN);
	
	/*
	 * Set length of send and receive queue. IF_MAXQ is a good value.
	 */
	if_dummy.rcv.maxqlen = IF_MAXQ;
	if_dummy.snd.maxqlen = IF_MAXQ;
	/*
	 * Setup pointers to service functions
	 */
	if_dummy.open = dummy_open;
	if_dummy.close = dummy_close;
	if_dummy.output = dummy_output;
	if_dummy.ioctl = dummy_ioctl;
	/*
	 * Optional timer function that is called every 200ms.
	 */
	if_dummy.timeout = 0;
	
	/*
	 * Here you could attach some more data your driver may need
	 */
	if_dummy.data = 0;
	
	/*
	 * Number of packets the hardware can receive in fast succession,
	 * 0 means unlimited.
	 */
	if_dummy.maxpackets = 0;
	
	/*
	 * Register the interface.
	 */
	if_register (&if_dummy);
	
	/*
	 * NETINFO->fname is a pointer to the drivers file name
	 * (without leading path), eg. "dummy.xif".
	 * NOTE: the file name will be overwritten when you leave the
	 * init function. So if you need it later make a copy!
	 */
	if (NETINFO->fname)
	{
		strncpy (my_file_name, NETINFO->fname, sizeof (my_file_name));
		my_file_name[sizeof (my_file_name) - 1] = '\0';
# if 0
		ksprintf (message, "My file name is '%s'\n\r", my_file_name);
		c_conws (message);
# endif
	}	
	/*
	 * And say we are alive...
	 */
	ksprintf (message, "Dummy Eth driver v0.0 (en%d)\n\r", if_dummy.unit);
	c_conws (message);
	return 0;
}
