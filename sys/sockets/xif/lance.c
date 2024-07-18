/*
 * Driver for various AM7990 LANCE based boards (C) 1995 - 1998 by T. Lang.
 *
 * Please notice: The driver sources may only distributed unchanged. You're
 * allowed to modify the driver for your needs but when you want your
 * changes to be released in this driver you *MUST* pass me your changes for
 * including. Only Kay Roemer may change the code without my permission since
 * he's resonsible for the major part of the MiNT-Net package and may have
 * to change some code if he does major modifications.
 *
 * I just don't want to be flamed for errors I'm not responsible for. And -
 * programming the ethernet boards is sometimes somewhat tricky because
 * of several design errors.
 * If you ignore this notice and redistribute modified versions without my
 * permission there will be no further source distributions of this driver.
 *
 * Currently supported:
 * PAMs EMEGA
 * PAMs VME (TT)
 * RIEBL MEGA (old and new boards)
 * RIEBL MEGA (old and new boards)
 *            modified for compatibility with TOS 2.0x/3.0x/4.0x/5.x
 *            (b15 and b17 of the MEGA bus connector swapped)
 * RIEBL VME (MEGA STe/TT)
 * Hopefully to be supported soon:
 * TUM board MEGA (since I have two boards over here (thanks Maximilian!)
 * Still missing:
 * NE2100 boards (ISA) (e. g. for use in the Hades)
 *
 * Some of these boards don't have their complete hw address in an NV memory,
 * so you need to pass the hw address BEFORE opening the device.
 *
 * This driver has been developed and tested on a 8MHz MEGA ST and a 32MHz
 * 68030 elrad ST with 12MB ST RAM. If you run it on substantially faster
 * machines a few wait()'s may be needed here and there. Especially the RIEBL
 * cards for MEGA STs may cause some trouble due to a severe design error.
 * Please contact me in case of success or failure...
 *
 * My EMail address (may change in near future):
 * Torsten.Lang@limes.de
 *
 * TO DOs:
 * - Other boards need the full 24 address bits - so handle them correctly!
 * - Better error handling: If MERRs or similar severe errors happen the LANCE
 *   has to be reinitialized!
 *
 * On a 32MHz elrad ST the driver reaches the following throughput:
 * ftp send:  ~ k/sec (Atari <--> Sun Sparc 10)
 * ftp recv:  ~ k/sec (        - "" -         )
 * tcp send:  untested
 * tcp recv:   - "" -
 * nfs read:   - "" -    (Atari <--> Sun Sparc 10 seem to transmit around 100KB/s)
 * nfs write:  - "" -
 *
 * history:
 * v1.2 20.06.1999 fixed a small bug when outputting the interface's name
 * v1.1 07.11.1998 removed a severe bug in multibuffer transmit routines
 *
 * 01/15/94, Kay Roemer.
 * 09/26/95, Torsten Lang.
 */

# include "global.h"
# include "buf.h"
# include "inet4/if.h"
# include "inet4/ifeth.h"

# include "lancemem.h"

# include "netinfo.h"

# include "mint/asm.h"
# include "mint/sockio.h"

# include <stddef.h>
# include <osbind.h>


/*
 * 200 Hz timer
 */
# define HZ200 (*(volatile long *) 0x4baL)

# define MIN_LEN 60

# define TIMEOUTCNT	 400 /* 2s */
# define TIMEOUT	1000 /* 5s */

static PKTBUF *pkt_rcv_ring[RECVBUFFS];
static PKTBUF *pkt_snd_ring[XMITBUFFS];
static short snd_ring_host, snd_ring_lance, snd_ring_full;
static short rcv_ring_host;

void (*ihandler)(void) = 0;
void (*lance_probe_c_)(void);
extern void lance_probe_asm(void);
static volatile int found = 0;
static long last_sent;
static char up = 0;

static struct netif if_lance;

static long	lance_open	(struct netif *);
static long	lance_close	(struct netif *);
static long	lance_output	(struct netif *, BUF *, const char *, short, short);
static long	lance_ioctl	(struct netif *, short, long);
static long	lance_config	(struct netif *, struct ifopt *);

static long	lance_probe	(struct netif *);
static void	lance_probe_c	(void);
static long	lance_reset	(struct netif *);
static void	lance_install_ints (void);
static void	lance_int	(void);
static long	lance_pkt_init	(void);

static long
lance_open (struct netif *nif)
{
	long retcode;
	
	if (!up)
	{
		if (!(retcode = lance_reset (nif)))
			up = 1;

		return (retcode);
	}
	
	return -1;
}

static long
lance_close (struct netif *nif)
{
	register volatile int *rdp;
	
	if (up)
	{
		rdp = RCP_RDP;
		*(rdp+1) = CSR0; /* This normally isn't neccessary, it's just for security */
		*rdp = CSR0_STOP;
		
		ihandler = NULL;
		lance_pkt_init ();
		
		return 0;
	}
	
	return -1;
}

static long
lance_reset (struct netif *nif)
{
	register volatile LNCMEM *mem;
	register volatile TMD	 *md;
	register long i;
	register volatile int *rdp;
	
	lance_pkt_init ();
	
	rdp = RCP_RDP;
	mem = (LNCMEM *) RCP_MEMBOT;
	*(rdp + 1) = CSR0;
	*rdp = CSR0_STOP;
	
	if (!memcmp (nif->hwlocal.adr.bytes, "\377\377\377\377\377\377", ETH_ALEN))
		return -1; /* hw address not set! */
	
	for (i = 0 ; i < ETH_ALEN ; i++)
		mem->init.haddr.haddr[i ^ 1] = nif->hwlocal.adr.bytes[i];
	
	mem->init.rdrp.drp_lo = (ushort) offsetof(LNCMEM,rmd[0]);
# if 0
	/*
	 * receive ring descr. pointer
	 */
	((ulong) &(PRMD[0]));
# endif
	mem->init.rdrp.drp_hi = 0;
	mem->init.rdrp.len = RECVRLEN;
	
	mem->init.tdrp.drp_lo = (ushort) offsetof (LNCMEM,tmd[0]);
# if 0
	/*
	 * transmit ring descr. pointer
	 */
	((ulong) &(PTMD[0]));
# endif
	mem->init.tdrp.drp_hi = 0;
	mem->init.tdrp.len = XMITRLEN;
	
	/*
	 * initialize send ring
	 */
	snd_ring_host = snd_ring_lance = snd_ring_full = 0;
	
	for (i = 0; i < XMITBUFFS; i++)
	{
		md = mem->tmd+i;
		md->ladr = (ushort)(0xFFFFL & (ulong) pkt_snd_ring[i]);
		md->tmd1 = OWN_HOST;
		/*
		 * zero pkt size
		 */
		md->tmd2 = 0;
		md->tmd3 = 0;
	}
	
	rcv_ring_host = 0;
	
	for (i = 0; i < RECVBUFFS; i++)
	{
#if 1
		register volatile RMD *rmd = mem->rmd + i;
		rmd->ladr = (ushort)(0xFFFFL & (ulong) pkt_rcv_ring[i]);
		rmd->rmd1 = OWN_CHIP;
		/*
		 * 2's complement (maximum size)
		 */
		rmd->rmd2 = -MAXPKTLEN;
		rmd->mcnt = 0;
#else
		(RMD *) md = mem->rmd + i;
		((RMD *) md)->ladr = (ushort)(0xFFFFL & (ulong) pkt_rcv_ring[i]);
		((RMD *) md)->rmd1 = OWN_CHIP;
		/*
		 * 2's complement (maximum size)
		 */
		((RMD *) md)->rmd2 = -MAXPKTLEN;
		((RMD *) md)->mcnt = 0;
#endif
	}
	
	*(rdp + 1) = CSR3;
# ifndef PAMs_INTERN
	*rdp = CSR3_BSWP;
# else
	*rdp = CSR3_BSWP | CSR3_ACON;
# endif
	*(rdp+1) = CSR2;
	*rdp = 0;
	*(rdp+1) = CSR1;
	*rdp = 0;
	*(rdp+1) = CSR0;
	*rdp = CSR0_STRT | CSR0_INIT;
	i = HZ200 + TIMEOUTCNT;
	while (!(*rdp & CSR0_IDON) && HZ200 - i < 0)
		;
	if ((*rdp & (CSR0_IDON | CSR0_ERR)) ^ CSR0_IDON)
	{
		*rdp = CSR0_STOP;
		ihandler = NULL;
		return -1;
	}
	ihandler = lance_int;
	/*
	 * clear idon-bit
	 */
	*rdp = CSR0_IDON;
	/*
	 * enable interrupts
	 */
	*rdp = CSR0_INEA;
	/*
	 * RCP_RAP (=*rdp+1) must not be changed after initialition!
	 * Other routines assume that RCP_RAP is set to CSR0 which is
	 * in fact the only register that can be read when LANCE is running.
	 */
	
	return 0;
}

static long
lance_ioctl (struct netif *nif, short cmd, long arg)
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
			 * Interface configuration, handled by lance_config()
			 */
			ifr = (struct ifreq *)arg;
			return lance_config (nif, ifr->ifru.data);
	}
	
	return ENOSYS;
}

/*
 * Interface configuration via SIOCSIFOPT. The ioctl is passed a
 * struct ifreq *ifr. ifr->ifru.data points to a struct ifopt, which
 * we get as the second argument here.
 *
 * If the user MUST configure some parameters before the interface
 * can run make sure that lance_open() fails unless all the necessary
 * parameters are set.
 *
 * Return values	meaning
 * ENOSYS		option not supported
 * ENOENT		invalid option value
 * 0			Ok
 */
static long
lance_config (struct netif *nif, struct ifopt *ifo)
{
# define STRNCMP(s)	(strncmp ((s), ifo->option, sizeof (ifo->option)))

# ifndef PAMs_INTERN
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
		DEBUG (("LANCE: hwaddr is %02x:%02x:%02x:%02x:%02x:%02x",
			cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]));
	}
	else 
# endif
	if (!STRNCMP ("braddr"))
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
		DEBUG (("LANCE: braddr is %02x:%02x:%02x:%02x:%02x:%02x",
			cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]));
	}
	else if (!STRNCMP ("debug"))
	{
		/*
		 * turn debuggin on/off
		 */
		if (ifo->valtype != IFO_INT)
			return ENOENT;
		DEBUG (("LANCE: debug level is %ld", ifo->ifou.v_long));
	}
	else if (!STRNCMP ("log"))
	{
		/*
		 * set log file
		 */
		if (ifo->valtype != IFO_STRING)
			return ENOENT;
		DEBUG (("LANCE: log file is %s", ifo->ifou.v_string));
	}
	
	return ENOSYS;
}

long driver_init(void);
long
driver_init (void)
{
	char *lance_type =
# ifdef PAMs_INTERN
# define __ifname "ple"
	"PAMs EMega/VME"
# else
# ifdef MEGA_STE
# define __ifname "rle"
	"RIEBL VME (MSTE)"
# else
# ifdef ATARI_TT
# define __ifname "rle"
	"RIEBL VME (TT)"
# else
# ifdef SPECIAL
# define __ifname "rle"
	"RIEBL MEGA ST (mod.)"
# else
# ifdef MEGA_ST
# define __ifname "rle"
	"RIEBL MEGA ST"
# endif
# endif
# endif
# endif
# endif
# ifdef HACK
	" (slow)"
# endif
	;
	
	static char message[100];
	
	strcpy (if_lance.name, __ifname);
	if_lance.unit = if_getfreeunit (__ifname);
	if_lance.metric = 0;
	if_lance.flags = IFF_BROADCAST;
	if_lance.mtu = 1500;
	if_lance.timer = 0;
	if_lance.hwtype = HWTYPE_ETH;
	if_lance.hwlocal.len =
	if_lance.hwbrcst.len = ETH_ALEN;
	
	if_lance.rcv.maxqlen = IF_MAXQ;
	if_lance.snd.maxqlen = IF_MAXQ;
	
	if_lance.open = lance_open;
	if_lance.close = lance_close;
	if_lance.output = lance_output;
	if_lance.ioctl = lance_ioctl;
	
	if_lance.timeout = 0;
	
	if_lance.data = 0;
	
	/*
	 * Tell upper layers the max. number of packets we are able to
	 * receive in fast succession.
	 */
	if_lance.maxpackets = 0;
	
	ksprintf (message, "AM7990: driver for %s (C) 1996, 1997, 1998 by T. Lang\r\n", lance_type);
	c_conws(message);
	
	if (lance_probe (&if_lance) < 0)
	{
		ksprintf (message, "AM7990: driver for %s not installed, board not found.\r\n", lance_type);
		c_conws(message);
		return 1;
	}
	lance_pkt_init ();
	lance_install_ints ();
	
	if_register (&if_lance);
	
	if (!memcmp(&if_lance.hwlocal.adr.bytes, "\377\377\377\377\377\377", ETH_ALEN))
		ksprintf (message, "AM7990: driver v%s (%s%d) for %s (no valid hw address!)\r\n",
			"1.2",
			__ifname,
			if_lance.unit,
			lance_type);
	else
		ksprintf (message, "AM7990: driver v%s (%s%d) for %s (%02x:%02x:%02x:%02x:%02x:%02x)\r\n",
			"1.2",
                        __ifname,
			if_lance.unit,
			lance_type,
			if_lance.hwlocal.adr.bytes[0],
			if_lance.hwlocal.adr.bytes[1],
			if_lance.hwlocal.adr.bytes[2],
			if_lance.hwlocal.adr.bytes[3],
			if_lance.hwlocal.adr.bytes[4],
			if_lance.hwlocal.adr.bytes[5]);
	c_conws (message);
	return 0;
}

static long
lance_probe (struct netif *nif)
{
	register volatile LNCMEM *mem;
	register long i;
	register volatile int *rdp;
	short sr;
# ifdef PAMs_INTERN
	ushort volatile *eemem;
# endif
	
	rdp = RCP_RDP;
	mem = (LNCMEM *)RCP_MEMBOT;
	lance_probe_c_ = lance_probe_c;
	
	sr = spl7 ();
	
	(&lance_probe_asm)();
	
	spl (sr);
	
	if (!found)
		return -1;
	
	*(rdp+1) = CSR0;
	*rdp = CSR0_STOP;
	/*
	 * init data structures
	 */
		
# ifdef PAMs_INTERN
	eemem = (ushort *)RCP_MEMBOT;
	*(volatile char *)LANCE_EEPROM;
	memcpy (nif->hwbrcst.adr.bytes, "\377\377\377\377\377\377", ETH_ALEN);
	for (i = 0 ; i < ETH_ALEN; i++)
		nif->hwlocal.adr.bytes[i] = 
			((eemem[i<<1] & 0xf) << 4) | (eemem[(i<<1)+1] & 0xf); 
	*(volatile char *)LANCE_MEM;
# endif
	mem->init.mode = 0;		/* mode word */

/* 
 * Please note: This driver awaits the board's ethernet address in the
 * init block if it is a Riebl card. The address does *NOT* conform to the
 * ANS software what is - to my mind - not a real drawback.
 * In case of old boards you have to set the hw address by hand every time
 * before *OPENING* the device, otherwise only once. This is done by IOCTLs.
 */

# ifdef RIEBL
	/* .. <- This is normally 2 for MEGAs */
        /* ||    and 4 for TTs(/MEGA STes?)   */
	if ((mem->init.haddr.laddr & 0xFFFFF8FFL) != 0x00000036L) {
		/* Set a default hw address that is known as error condition to the */
		/* caller.                                                          */
		for (i = 0; i < ETH_ALEN; i++)
			mem->init.haddr.haddr[i] = '\377';
	}		
	memcpy (nif->hwbrcst.adr.bytes, "\377\377\377\377\377\377", ETH_ALEN);
	for (i = 0 ; i < ETH_ALEN; i++)
		nif->hwlocal.adr.bytes[i ^ 1] = mem->init.haddr.haddr[i];
# endif

    	/*
    	 * logical address filter - receive *NO* logical addresses (no multicasting)
    	 */    	
	mem->init.laf[0] = 0x00000000L;
	mem->init.laf[1] = 0x00000000L;
	
	/*
	 * We return here. The rest of the initialization is done in lance_reset.
	 */
	return 0;
}

static void
lance_probe_c (void)
{
	register union { volatile LNCMEM *v; ushort *s; uchar *c; } ptr;
	register volatile LNCMEM *mem;
	register long i;
	register volatile int *rdp;
	
	rdp = RCP_RDP;
	mem = (LNCMEM *)RCP_MEMBOT;
	
# ifdef PAMs_INTERN
	*(volatile char *)LANCE_MEM;
# endif
	
	*(rdp + 1) = 0;
	
	*rdp = CSR0_STOP;
	if (*rdp != CSR0_STOP)
		return;
	*rdp = ~(CSR0_INIT | CSR0_STRT);
	if (*rdp != CSR0_STOP)
		return;
	
	*(rdp + 1) = 0xFFFF;
	if (*(rdp + 1) != 3)
		return;
	
	*(rdp + 1) = 1;
	
/* For some reason the following test does not work :-( */
# if 0
	for (i = 0; i < 0x10000L; i++)
	{
		*rdp = i;
		if (*rdp != i)
			break;
	}
	if (i != 0x10000L)
		goto err;
# endif
	
	for (i = 0, ptr.v = mem; i < 0x10000L; i++)
	{
		*ptr.s = (ushort)i;
		if (*ptr.s != (ushort)i)
			break;
		*ptr.c = (uchar)(i & 0xFF);
		if (*ptr.c != (uchar)(i & 0xFF))
			break;
		*(ptr.c + 1) = (uchar)(i & 0xff);
		if (*(ptr.c + 1) != (uchar)(i & 0xFF))
			break;
	}
	if (i != 0x10000L)
		return;
	
	found = 1;

}

static long
lance_output (struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	short len;
	BUF *nbuf;
	register volatile int *rdp;
	register volatile TMD *tmd = PTMD;
	ushort sr;
	
	nbuf = eth_build_hdr(buf, nif, hwaddr, pktype);

	if (nbuf == 0)
	{
		nif->out_errors++;
		return ENOMEM;
	}
	
	if (nif->bpf)
		bpf_input (nif, nbuf);
	
	len = nbuf->dend - nbuf->dstart;
	
	tmd = PTMD+snd_ring_host;
	
	/* Alternatively we could check here for (snd_ring_lance-snd_ring_host)%XMITBUFFS==1 */
	if (snd_ring_full)
	{
		if ((HZ200-last_sent) > TIMEOUT)
		{
			buf_deref(nbuf, BUF_NORMAL);
			nif->out_errors++;
			lance_reset (nif);
			return ETIMEDOUT;
		}
		
		if (if_enqueue(&nif->snd, nbuf, nbuf->info))
		{
			nif->out_errors++;
			return EINTERNAL;
		}
		
		return 0;
	}
	
	sr = spl7(); /* New since Nov/07/1998 -> avoid race conditions with TX int */
	
	memcpy (pkt_snd_ring[snd_ring_host], nbuf->dstart, len);
	
	if (len < MIN_LEN)
		len = MIN_LEN;
# ifdef PAMs_INTERN
	len = (len+1) & ~1;
# endif
	
	rdp = RCP_RDP;
	if (!(*rdp & CSR0_TXON))
	{
		spl (sr); /* New since Nov/07/1998 */
		buf_deref (nbuf, BUF_NORMAL);
		nif->out_errors++;
		return EINTERNAL;
	}
	
	tmd->tmd2 = -len;
	tmd->tmd3 = 0;
	
	/* sr = spl7(); */ /* moved a few lines up - look there for comment */
	
	snd_ring_host = (snd_ring_host+1) & (XMITBUFFS-1);
	if (snd_ring_host == snd_ring_lance)
		snd_ring_full = 1;
	
	tmd->tmd1 = (STP | ENP | OWN_CHIP);	/* start/end of packet */
	
	last_sent = HZ200;
	
	*rdp = CSR0_TDMD | CSR0_INEA; /* Force immediate transmit buffer polling */
	
	spl(sr);
	
	buf_deref (nbuf, BUF_NORMAL);
	
	return 0;	
}

/*
 * LANCE interrupt routine
 */
void
lance_int (void)
{
# ifdef HACK
#if 0
	register int		i;
#endif
# endif
	register ushort		csr0;
	
	ushort sr; /* New since Nov/11/1998 */
	
	register volatile int	*rdp;
	register volatile TMD	*tmd;
	register volatile RMD	*rmd;
	register PKTBUF 	*pkt;
	
	struct netif *nif = &if_lance;
	BUF *buf;
	int pktlen;
	
	/*
	 * register data port
	 */
	rdp = RCP_RDP;
	csr0 = *rdp;
	if (!(csr0 & CSR0_INTR))
		return;
	
again:	
	if (csr0 & CSR0_IDON)
		*(rdp) = CSR0_IDON | CSR0_INEA;
	if (csr0 & CSR0_RINT)
	{
		*rdp = CSR0_RINT | CSR0_INEA;
// 		(RMD *)md = PRMD + rcv_ring_host;
		rmd = PRMD + rcv_ring_host;
		while ((rmd->rmd1 & OWN) == OWN_HOST)
		{
			/*
			 * packet ok ?
			 */
			if ((STP|ENP) == (rmd->rmd1 & (ERR|STP|ENP)))
			{
				pkt = pkt_rcv_ring[rcv_ring_host];

				/*
				 * packet length without checksum
				 */
				pktlen = rmd->mcnt - 4;
				buf = buf_alloc (pktlen + 100, 50, BUF_ATOMIC);
				if (buf == 0)
					nif->in_errors++;
				else
				{
					buf->dend = buf->dstart + pktlen;
# ifndef HACK
					memcpy (buf->dstart, pkt, pktlen);
# else
					__asm__
					(
						"loop:\n\t"
						"moveb %1@+,%2@+\n\t"
						"moveb %1@+,%2@+\n\t"
						"dbra  %0,loop\n\t"
						:
						: "d"(((pktlen + 1) >> 1) -1), "a"(pkt), "a"(buf->dstart)
					);
# if 0
					for (i = 0; i < pktlen; i++)
						*(((uchar *) buf->dstart)+i) = *(((uchar *)pkt)+i);
# endif
# endif
					if (nif->bpf)
						bpf_input (nif, buf);
					if (!if_input (nif, buf, 0,
						       eth_remove_hdr (buf)))
						nif->in_packets++;
					else
						nif->in_errors++;
				}
			}
			else
			{
				if (rmd->rmd1 & ERR)
			  	{
			  		nif->in_errors++;
					/* lance_reset (nif); */
					/* spl(sr); */
					/* return; */ /* Not yet... */
				}
			}
			/*
			 * free packet
			 */
			rmd->mcnt = 0;
			/*
			 * give packet back to lance
			 */
			rcv_ring_host = (rcv_ring_host + 1) & (RECVBUFFS - 1);
			rmd->rmd1 = OWN_CHIP;
			rmd = PRMD + rcv_ring_host;
		}
	}
	if (csr0 & CSR0_TINT)
	{
		/*
		 * clear interrupt bit & reenable
		 */ 
		*rdp = CSR0_TINT | CSR0_INEA;
		/*
		 * packet given back to host ?
		 */
		tmd = PTMD + snd_ring_lance;
		sr = spl7(); /* New since Nov/07/1998 -> avoid race conditions with lance_output(); */
		while (((snd_ring_lance != snd_ring_host) || snd_ring_full) && ((tmd->tmd1 & OWN) == OWN_HOST))
		{
			if ((tmd->tmd1 & (ERR | MORE | ONE)) || (tmd->tmd3 & ~0x3FF))
			{
				if ((tmd->tmd1 & (MORE | ONE)) || (tmd->tmd3 & (LCOL | RTRY)))
				{
					nif->collisions++;
					nif->out_packets++;
				}
				else
					nif->out_errors++;
			}
			else
				nif->out_packets++;
			
			snd_ring_lance = (snd_ring_lance + 1) & (XMITBUFFS - 1);
			snd_ring_full = 0;
			tmd = PTMD + snd_ring_lance;
		}
		while (!snd_ring_full)
		{
			if (!(buf = if_dequeue(&nif->snd)))
				break;
			
			tmd = PTMD + snd_ring_host;
			pktlen = buf->dend - buf->dstart;
			memcpy(pkt_snd_ring[snd_ring_host], buf->dstart, pktlen);
			if (pktlen < MIN_LEN)
				pktlen = MIN_LEN;
# ifdef PAMs_INTERN
			pktlen = (pktlen + 1) & ~1;
# endif
			if (!(*rdp & CSR0_TXON))
			{
				buf_deref(buf, BUF_NORMAL);
				nif->out_errors++;
			}
			else
			{
				tmd->tmd2 = -pktlen;
				tmd->tmd3 = 0;
				tmd->tmd1 = STP | ENP | OWN_CHIP;
				
				*rdp = CSR0_TDMD | CSR0_INEA;
				
				last_sent = HZ200;
				
				buf_deref(buf, BUF_NORMAL);
				
				snd_ring_host = (snd_ring_host + 1) & (XMITBUFFS - 1);
				if (snd_ring_host == snd_ring_lance)
					snd_ring_full = 1;
			}
		}
		spl (sr); /* New since Nov/07/1998 -> see above */
	}
	if (csr0 & CSR0_ERR)
	{
		nif->in_errors++;
		*rdp = CSR0_CERR | CSR0_MERR | CSR0_BABL | CSR0_MISS | CSR0_INEA;
	}
	if ((csr0 = *rdp) & CSR0_INTR)
		goto again;
}

extern void lance_hbi_int (void);
extern void lance_v5_int (void);
extern void lance_vbi_int (void);
extern long old_vbi;

/*
 * Initialize the packet ring. This function also clears the packet buffers
 * for security purposes. Since the RIEBL boards have battery back'd static
 * memory as packet buffers you may find things like passwords etc. in the
 * static memory even after turning off and on the machine.
 * This function is also called by lance_reset() and lance_close for better
 * security.
 */
static long
lance_pkt_init (void)
{
	short i, j;

	for (i = 0; i < XMITBUFFS; i++)
	{
		pkt_snd_ring[i] = PPKT+i;
		/* The following loop is just for security purposes (see lance_close). */
		for (j = 0; j < (ETH_MAX_DLEN>>1); j++)
			*(((ushort *)pkt_snd_ring[i])+j) = 0;
	}
	
	for (i = 0; i < RECVBUFFS; i++)
	{
		pkt_rcv_ring[i] = PPKT+(i+XMITBUFFS);
		/* The following loop is just for security purposes (see lance_close). */
		for (j = 0; j < (ETH_MAX_DLEN>>1); j++)
			*(((ushort *)pkt_rcv_ring[i])+j) = 0;
	}
	
	return 0;
}

/*
 * Install interrupt handler
 */
static void
lance_install_ints (void)
{
	ushort sr;
	
	sr = spl7 ();
	(void)Setexc (LANCEIVEC, lance_v5_int);
# ifdef PAMs_INTERN
	/*
         * This is a real dirty hack but seems to be necessary for the MiNT
         * environment. Even with a correct hbi handler installed I found an
         * IPL of 3 quite often. This is definitely a misbehaviour of the
         * MiNT environment since the hbi handler works under plain TOS (what
         * it is supposed to do since Atari guarantees an IPL of 0 at start
         * time of a program).
         * Since the PAMs EMEGA boards run at an unchangeable IPL of 3 it is
         * ABSOLUTELY necessary that the OS and the application programs
         * behave correct!
         * So someone with the knowledge and enough time should take a look
         * at the various MiNT sources, filesystems etc. to find the bug...
         * When these bug(s) are fixed the vbi can stay unchanged!
         */
	
	old_vbi = (long)Setexc (HBI+2, lance_vbi_int);
	
	*(char *)LANCEIVECREG = LANCEIVEC;
	(void)Setexc (HBI, lance_hbi_int);
# endif
# ifdef RIEBL
	*(char *)LANCEIVECREG = LANCEIVEC;
# endif
	spl (sr);
}
