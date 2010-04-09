/*
 * Driver for Dlink's DE600 pocket lan adaptor connected to the cartridge
 * port via the TUW hardware (adapter hardware description can be found
 * at fortec.tuwien.ac.at).
 * 
 * This driver REQUIRES you to connect the interrupt wire to pin 11 (BUSY)
 * of your centronics port, ie it does not poll for incomming packets during
 * the VBL interrupt like the tuw-tcp driver.
 * 
 * This driver has been developed and tested on a 16MHz ST. If you run in
 * on substantially faster machines a few wait()'s may be needed here and
 * there. Please contact me in case of success or failure...
 * 
 * On a 16MHz ST (TT) the driver reaches the following throughput:
 * ftp send:  ~88 (~180) k/sec (ncftp 1.7.5 -> wu-ftpd 2.0)
 * ftp recv:  ~85 (~150) k/sec (ncftp 1.7.5 <- wu-ftpd 2.0)
 * tcp send:  ~90 (~200) k/sec (tcpcl -> tcpsv)
 * tcp recv:  ~90 (~160) k/sec (tcpsv <- tcpcl)
 * nfs read:  ~50 (    ) k/sec
 * nfs write: ~45 (    ) k/sec
 * 
 * A note to NFS. When you mount a filesystem you must tell 'mount' to
 * use a maximum receive block size of MTU-300 bytes! Otherwise the
 * UDP datagrams must be fragmented and the lots of fragments will flood
 * the DE600's receive buffers wich leads to *very* low performance, ie
 * ALWAYS mount like this:
 *	mount -o rsize=1200 remote:/dir /nfs/dir
 * 
 * 01/15/95, Kay Roemer.
 */

# include "global.h"
# include "buf.h"
# include "inet4/if.h"
# include "inet4/ifeth.h"

# include "de600.h"
# include "netinfo.h"

# include "mint/asm.h"
# include "mint/sockio.h"
# include "arch/timer.h"

# include <osbind.h>


# define DE600_VERSION	"0.4"

/* there must be a better place for that */
volatile long *hz_200 = _hz_200;

/*
 * Some macros for managing the output queue.
 */
# define Q_EMPTY(p)	((p)->tx_used <= 0)
# define Q_FULL(p)	((p)->tx_used >= TX_PAGES)
# define Q_LAST(p)	((p)->tx_addr[(p)->tx_tail])
# define Q_DEL(p)	{ --(p)->tx_used; (p)->tx_tail ^= 1; }
# define Q_ADD(p, addr)	{ ++(p)->tx_used; (p)->tx_addr[(p)->tx_head] = (addr);\
			  (p)->tx_head ^= 1; }

struct de600
{
	ushort	rx_page;	/* high nibble of nic's command reg */
	ushort	tx_addr[2];	/* queue of page start addresses */
	ushort	tx_head;	/* queue head */
	ushort	tx_tail;	/* queue tail */
	ushort	tx_used;	/* no of used queue entries */
};

static struct netif if_de600;
static struct de600 de600_priv;

static long	de600_open	(struct netif *);
static long	de600_close	(struct netif *);
static long	de600_output	(struct netif *, BUF *, const char *, short, short);
static long	de600_ioctl	(struct netif *, short, long);

static long	de600_probe	(struct netif *);
static long	de600_reset	(struct netif *);
static void	de600_install_ints (void);
static void	de600_recv_packet (struct netif *);

static long
de600_open (struct netif *nif)
{
	de600_reset (nif);
	return 0;
}

static long
de600_close (struct netif *nif)
{
	return 0;
}

static long
de600_output (struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	struct de600 *pr = nif->data;
	short stat, addr, len, sr;
	long ticks;
	BUF *nbuf;
	
	nbuf = eth_build_hdr (buf, nif, hwaddr, pktype);
	if (!nbuf)
	{
		nif->out_errors++;
		return ENOMEM;
	}
	
	if (nif->bpf)
		bpf_input (nif, nbuf);
	
	/*
	 * These spl()'s are absolutely necesarry to protect de600_cli().
	 * If you remove them somehow the return address on the stack
	 * gets corrupted when a BUSY interrupt happens during the
	 * jsr _de600_cli.
	 */
	sr = spl7 ();
	de600_cli ();
	de600_dints ();
	spl (sr);
	
	select_nic ();
	if (Q_FULL (pr))
	{
		c_conws ("de600: output queue full!\r\n");
		
		buf_deref (nbuf, BUF_NORMAL);
		select_prn ();
		de600_sti ();
		de600_reset (nif);
		
		return ENOMEM;
	}
	
	/*
	 * Enet packets must at least have 60 bytes.
	 */
	len = nbuf->dend - nbuf->dstart;
	addr = (pr->tx_head + 1) * MEM_2K - MAX (len, MIN_LEN);
	
	/*
	 * Setup address and send packet
	 */
	send_addr (RW_ADDR, addr);
	send_block (nbuf->dstart, len);
	
	/*
	 * If nic busy sending something than wait for it to finish
	 */
	if (!Q_EMPTY (pr))
	{
		ticks = *hz_200+1;
		do {
			stat = recv_stat () & (TX_BUSY|TX_FAILED16|RX_GOOD);
			if (stat & RX_GOOD)
			{
				stat &= ~RX_GOOD;
				de600_recv_packet (nif);
			}
			if (stat == 0)
			{
				Q_DEL (pr);
			}
			else if (stat == TX_FAILED16)
			{
				nif->collisions++;
				send_addr (TX_ADDR, Q_LAST (pr));
				send_cmd (pr->rx_page, TX_ENABLE);
				ticks = *hz_200+1;
			}
		}
		while (!Q_EMPTY (pr) && *hz_200 - ticks <= 0);
		
		/*
		 * transmission timed out
		 */
		if (!Q_EMPTY (pr))
		{
			c_conws ("de600: timeout, check network cable!\r\n");
			
			buf_deref (nbuf, BUF_NORMAL);
			nif->out_errors++;
			select_prn ();
			de600_sti ();
			de600_reset (nif);
			
			return ETIMEDOUT;
		}
	}
	
	Q_ADD (pr, addr);
	
	/*
	 * Tell nic where to send from and enable transmitter
	 */
	send_addr (TX_ADDR, addr);
	send_cmd (pr->rx_page, TX_ENABLE);
	
	select_prn ();
	de600_sti ();
	nif->out_packets++;
	buf_deref (nbuf, BUF_NORMAL);
	
	return 0;
}

static long
de600_ioctl (struct netif *nif, short cmd, long arg)
{
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
	}
	
	return ENOSYS;
}
long _cdecl driver_init(void);
long
driver_init (void)
{
	static char message[100];
	ushort sr;
	
	strcpy (if_de600.name, "en");
	if_de600.unit = if_getfreeunit ("en");
	if_de600.metric = 0;
	if_de600.flags = IFF_BROADCAST;
	if_de600.mtu = 1500;
	if_de600.timer = 0;
	if_de600.hwtype = HWTYPE_ETH;
	if_de600.hwlocal.len =
	if_de600.hwbrcst.len = ETH_ALEN;
	
	if_de600.rcv.maxqlen = IF_MAXQ;
	if_de600.snd.maxqlen = IF_MAXQ;
	
	if_de600.open = de600_open;
	if_de600.close = de600_close;
	if_de600.output = de600_output;
	if_de600.ioctl = de600_ioctl;
	
	if_de600.timeout = 0;
	
	if_de600.data = &de600_priv;
	
	/*
	 * Tell upper layers the max. number of packets we are able to
	 * receive in fast succession.
	 */
	if_de600.maxpackets = 1;
	
	sr = spl7 ();
	de600_cli ();
	de600_dints ();
	spl (sr);
	
	if (de600_probe (&if_de600) < 0)
	{
		de600_sti ();
		c_conws ("de600: driver not installed.\r\n");
		return 1;
	}
	
	de600_install_ints ();
	de600_sti ();
	
	if_register (&if_de600);
	
	ksprintf (message, "DE600 driver v%s (en%d) (%x:%x:%x:%x:%x:%x)\r\n",
		DE600_VERSION,
		if_de600.unit,
		if_de600.hwlocal.adr.bytes[0],
		if_de600.hwlocal.adr.bytes[1],
		if_de600.hwlocal.adr.bytes[2],
		if_de600.hwlocal.adr.bytes[3],
		if_de600.hwlocal.adr.bytes[4],
		if_de600.hwlocal.adr.bytes[5]);
	c_conws (message);
	
	return 0;
}

static long
de600_reset (struct netif *nif)
{
	struct de600 *pr = nif->data;
	ushort sr;
	int i;
	
	sr = spl7 ();
	de600_cli ();
	de600_dints ();
	spl (sr);
	
	select_nic ();
	/*
	 * Reset adapter
	 */
	send_cmd (0, RESET_ON);
	wait ();
	send_cmd (0, RESET_OFF);
	wait ();
	
	/*
	 * Set enet address
	 */
	send_addr (RW_ADDR, MEM_ROM);
	for (i = 0; i < ETH_ALEN; i++)
		send_byte (WRITE_DATA, nif->hwlocal.adr.bytes[i]);
	
	bzero (pr, sizeof (*pr));
	pr->rx_page = RX_BP | RX_PAGE1;
	
	/*
	 * Enable receiver
	 */
	send_cmd (pr->rx_page, RX_ENABLE);
	select_prn ();
	de600_sti ();
	
	return 0;
}

static long
de600_probe (struct netif *nif)
{
	uchar *cp;
	int i;
	
	select_nic ();
	
	/*
	 * Reset the adapter
	 */
	recv_stat ();
	send_cmd (0, RESET_ON);
	wait ();
	send_cmd (0, RESET_OFF);
	wait ();
	if (recv_stat () & 0xf0)
	{
		c_conws ("de600: no DE600 detected.\r\n");
		return -1;
	}
	
	/*
	 * Try to read magic code and last 3 bytes of hw
	 * address
	 */
	send_addr (RW_ADDR, MEM_ROM);
	wait ();
	for (i = 0; i < ETH_ALEN; ++i)
	{
		nif->hwlocal.adr.bytes[i] = recv_byte (READ_DATA);
		nif->hwbrcst.adr.bytes[i] = 0xff;
	}
	
	/*
	 * Check magic
	 */
	cp = nif->hwlocal.adr.bytes;
	if (cp[1] != 0xde || cp[2] != 0x15)
	{
		c_conws ("de600: magic check failed.\r\n");
		return -1;
	}
	
	/*
	 * Install real address
	 */
	cp[0] = 0x00;
	cp[1] = 0x80;
	cp[2] = 0xc8;
	cp[3] &= 0x0f;
	cp[3] |= 0x70;
	select_prn ();
	
	return 0;
}

/*
 * Busy interrupt routine
 */
void de600_int(void);
void
de600_int (void)
{
	struct de600 *pr = if_de600.data;
	unsigned char stat;
	ushort sr;
	
	sr = spl7 ();
	de600_cli ();
	spl (sr);
	
	select_nic ();
	stat = recv_stat ();
	do {
		/*
		 * See if packet arrived. Reenable receiver
		 */
		if (stat & RX_GOOD)
			de600_recv_packet (&if_de600);
		else if (!(stat & RX_BUSY))
			send_cmd (pr->rx_page, RX_ENABLE);
		/*
		 * If something has been sent then either delete it
		 * from send queue or resend in case of collision.
		 */
		if (!Q_EMPTY (pr) && !(stat & TX_BUSY))
		{
			if (stat & TX_FAILED16)
			{
				if_de600.collisions++;
				send_addr (TX_ADDR, Q_LAST (pr));
				send_cmd (pr->rx_page, TX_ENABLE);
			}
			else
			{
				Q_DEL (pr);
			}
		}
		stat = recv_stat ();
	}
	while (stat & RX_GOOD);
	
	select_prn ();
	de600_sti ();
}

/*
 * Read a packet out of the adapter and pass it to the upper layers
 */
static void
de600_recv_packet (struct netif *nif)
{
	struct de600 *pr = nif->data;
	ushort pktlen, addr;
	BUF *b;
	
	/*
	 * Read packet length (excluding 32 bit crc)
	 */
	pktlen =  (ushort) recv_byte (RX_LEN);
	pktlen |= (ushort) recv_byte (RX_LEN) << 8;
	pktlen -= 4;
	
	/*
	 * Switch to next recv page and enable receiver
	 */
	pr->rx_page ^= RX_PAGE2;
	send_cmd (pr->rx_page, RX_ENABLE);

	if (pktlen < 32 || pktlen > 1535)
	{
		nif->in_errors++;
		if (pktlen > 10000)
			de600_reset (nif);
		return;
	}
	
	b = buf_alloc (pktlen+100, 50, BUF_ATOMIC);
	if (!b)
	{
		nif->in_errors++;
		return;
	}
	
	b->dend += pktlen;
	addr = (pr->rx_page & RX_PAGE2) ? MEM_4K : MEM_6K;
	
	/*
	 * Set receive address and read the packet
	 */
	send_addr (RW_ADDR, addr);
	recv_block (b->dstart, pktlen);
	
	/*
	 * Pass packet to upper layers
	 */
	if (nif->bpf)
		bpf_input (nif, b);
	
	if (!if_input (nif, b, 0, eth_remove_hdr (b)))
		nif->in_packets++;
	else
		nif->in_errors++;
}

/*
 * Install interrupt handler
 */
static void
de600_install_ints (void)
{
	de600_old_busy_int = (long)Setexc (0x40, de600_busy_int);
	
	__asm__
	(
		"bset	#0, 0xfffffa09:w;"	/* IERB */
		"bset	#0, 0xfffffa03:w;"	/* AER */
	);
}
