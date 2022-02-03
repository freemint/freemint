/*
 **********************************************************************
 *                                                                    *
 * Driver for BIODATA DMA ethernet adaptors (C) 1996 by Torsten Lang. *
 *                                                                    *
 **********************************************************************
 *                                                                    *
 * The DMA code of this ethernet packet driver is based on assembler  *
 * source code (C) by Hans-Peter Jansen, LISA GmbH, Berlin            *
 *                                                                    *
 **********************************************************************
 *                                                                    *
 * Ethernet driver skeleton (C) Kay Roemer                            *
 *                                                                    *
 **********************************************************************
 *
 * This driver has been developed and tested on a 32MHz elrad ST. If you
 * run it on substantially faster machines a few wait()'s may be needed
 * here and there. Please contact me in case of success or failure...
 *
 * Please note: The driver uses the interface name "de?" now
 *              (for dma ethernet with ? from 0..9).
 *
 * Some users reported problems on very fast machines like a MEGA ST
 * with a PAK68/3/48MHz and FRAK (TT RAM board). I know of this problem
 * but don't have the time to work on it yet (and I don't know if I ever
 * will since only very few users seem to use this driver...).
 *
 * Important: - The driver must be loaded to ST RAM, otherwise weird things
 *              will happen!!!
 *            - The driver needs some mirrored ST RAM since there is no cache
 *              management. So this version will NO LONGER work on a TT! 
 *
 * TO DOs:
 * - I don't know how the adaptor reacts when heavy traffic floods the 
 *   receive buffers. The final driver should handle this.
 * - Multicasting may never work since it is not implemented in the firmware.
 *   So it would have to be done by using the promiscuous mode.
 * - Cache management isn't implemented yet. I used a trick to avoid
 *   problems. The buffer address is ORed with $FF000000 when the driver
 *   is accessing the buffer. A drawback is that this version of the driver
 *   no longer runs on machines not mirroring the ST RAM (e. g. Atari TT).
 * - At the moment the driver doesn't support more than one adaptor. You may NOT
 *   use a second adaptor by installing the driver twice!
 * - Apply some patches I've already done in pamsdma.c/pamsdma.h
 *
 * Thanks to...
 * - Kay Roemer for his great networking package and his support.
 * - Hans-Peter Jansen
 *   Lisa GmbH
 *   Berlin
 *   for his support concerning the BIODATA ethernet adaptor.
 *   (please don't ask there for anything concerning THIS software)
 *
 * On a 32MHz ST the driver reaches the following throughput:
 * ftp send:  ~17 k/sec (ncftp 1.7.5 -> wu-ftpd 2.0)
 * ftp recv:  ~26 k/sec (ncftp 1.7.5 <- wu-ftpd 2.0)
 * tcp send:  untested  (tcpcl -> tcpsv)
 * tcp recv:   - "" -   (tcpsv <- tcpcl)
 * nfs read:   - "" -
 * nfs write:  - "" -
 *
 * Since some important functions are missing don't expect me to write some
 * magic routines increasing the performance. The BIODATA adaptor was NEVER
 * designed for an environment where you can't explicitly wait for packets.
 *
 * History:
 *
 * V0.1beta     First released version
 *
 * V0.3beta	Using mirrored ST RAM for accessing the DMA buffers.
 *
 * V1.0		Cleaned up the code
 *
 * 01/15/94, Kay Roemer,
 * 10/13/96, Torsten Lang.
 */

# include "global.h"
# include "buf.h"
# include "inet4/if.h"
# include "inet4/ifeth.h"

# include "biodma.h"

# include "netinfo.h"

# include "mint/asm.h"
# include "mint/sockio.h"


static struct netif if_lance;

int ram_ok   = 0;
int has_acsi = 0;

void (*lance_probe_c_)(void);

extern void	lance_probe_asm	(void);
static long	lance_open	(struct netif *);
static long	lance_close	(struct netif *);
static long	lance_output	(struct netif *, BUF *, const char *, short, short);
static long	lance_ioctl	(struct netif *, short, long);

static long	lance_probe	(struct netif *);
static void	lance_probe_c	(void);
// static long	lance_reset	(struct netif *);
static void	lance_int	(struct netif *);
static void	timeout_int	(PROC *proc, long arg);

static int	sendpkt (int target, uchar *buffer, int length);
static int	receivepkt (int target, uchar *buffer);
static HADDR	*read_hw_addr(int target);
static int	set_status(int target, uchar status);
static int	inquiry (int target);
static int	timeout (long timeoutcounter);
static void	setup_dma (void *address, ushort rw_flag, int num_blocks);
static int	send_first (int target, uchar byte, int biomode);
static int	send_other (uchar byte, int dma);
static int	send_1_5 (int lun, uchar *command, int dma);
static int	get_1_6 (uchar *buffer);
static int	get_status (void);
static int	calc_received (void *start_address);

DMABUFFER dma_buffer;
ushort rw;
int lance_target = -1;
int up = 0;
int pending = 0;
int send = 0;

static long
lance_open (struct netif *nif)
{
	int i;
	
	if (!LOCK)
		return -1;
	up = 1;
	i = set_status (lance_target, BROADCAST);
	send = 1;
	lance_int (nif);
	
	return i;
}

static long
lance_close (struct netif *nif)
{
	up = 0;
	
	return 0;
}

static long
lance_output (struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	short len;
	BUF *nbuf;
	int i, j;
	long r;
	
	nbuf = eth_build_hdr (buf, nif, hwaddr, pktype);
	if (!nbuf)
	{
		nif->out_errors++;
		return ENOMEM;
	}
	
	if (nif->bpf)
		bpf_input (nif, nbuf);
	
	if (!up)
	{
		buf_deref (nbuf, BUF_NORMAL);
		return -1;
	}
	
	if (!LOCK)
	{
		r = if_enqueue (&nif->snd, buf, buf->info);
		if (r)
		{
			buf_deref (nbuf, BUF_NORMAL);
			nif->out_errors++;
			return r;
		}
		return 0;
	}
	
	len = nbuf->dend - nbuf->dstart;
	memcpy ((uchar *)((ulong)dma_buffer.data.bytes | 0xFF000000l), nbuf->dstart, len);
	if (len < MIN_LEN)
		len = MIN_LEN;
	buf_deref (nbuf, BUF_NORMAL);
	if ((j = sendpkt (lance_target, dma_buffer.data.bytes, len)))
		nif->out_errors++;
	else
	{
		for (i = 1; i < 6; i++)
			if (addroottimeout (TIMEOUTCOUNTER>>i, timeout_int, 0))
				pending++;
		nif->out_packets++;
	}
	
	send = 1;
	lance_int (nif);
	
	return j;
}

static long
lance_ioctl (struct netif *nif, short cmd, long arg)
{
	switch (cmd)
	{
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
	}
	
	return ENOSYS;
}

long driver_init(void);
long
driver_init (void)
{
	static char message[100];
	
	strcpy (if_lance.name, "de");
	if_lance.unit = if_getfreeunit ((char *)"de");
	if_lance.metric = 0;
	if_lance.flags = IFF_BROADCAST;
	if_lance.mtu = 1500;
	if_lance.timer = TIMEOUTCOUNTER;
	if_lance.hwtype = HWTYPE_ETH;
	if_lance.hwlocal.len =
	if_lance.hwbrcst.len = ETH_ALEN;
	
	if_lance.rcv.maxqlen = IF_MAXQ;
	if_lance.snd.maxqlen = IF_MAXQ;
	
	if_lance.open = lance_open;
	if_lance.close = lance_close;
	if_lance.output = lance_output;
	if_lance.ioctl = lance_ioctl;
	
	if_lance.timeout = lance_int ;
	
	if_lance.data = 0;
	
	/*
	 * Tell upper layers the max. number of packets we are able to
	 * receive in fast succession.
	 */
	if_lance.maxpackets = 4;
	
	c_conws ("DP8390: DMA adaptor driver (C) 1996, 1997, 1998 by T. Lang\r\n");
	
	if (lance_probe (&if_lance) < 0)
	{
		c_conws ("DP8390: driver (v1.1) for BIODATA DMA adaptor not installed.\r\n");
		return 1;
	}
	
	if_register (&if_lance);
	
	ksprintf (message, "DP8390: driver for BIODATA DMA v%s (de%d) "
		 "(%02x:%02x:%02x:%02x:%02x:%02x) on target %d\r\n",
		"1.1",
		if_lance.unit,
		if_lance.hwlocal.adr.bytes[0],
		if_lance.hwlocal.adr.bytes[1],
		if_lance.hwlocal.adr.bytes[2],
		if_lance.hwlocal.adr.bytes[3],
		if_lance.hwlocal.adr.bytes[4],
		if_lance.hwlocal.adr.bytes[5],
		lance_target);
	c_conws (message);
	
	return 0;
}
#if 0
static long
lance_reset (struct netif *nif)
{
	int i;
	
	if (!LOCK)
		return -1;
	i = set_status (lance_target, BROADCAST);
	send = 1;
	lance_int (nif);
	
	return i;
}
#endif
static long
lance_probe (struct netif *nif)
{
	int i;
	HADDR *hwaddr;
	ushort sr;
	
	lance_probe_c_ = lance_probe_c;
	
	sr = spl7 ();
	
	(&lance_probe_asm)();
	
	spl (sr);
	
	if (!ram_ok)
	{
		c_conws ("DP8390: Mirrored ST ram not found!\r\n");
		return -1;
	}
	
	if (!LOCK)
	{
		c_conws ("DP8390: DMA locked!\r\n");
		return -1;
	}
	
	if (!has_acsi)
	{
		c_conws ("DP8390: This machine has no ACSI port!\r\n");
		return -1;
	}
	
	for (i = 0; i < 8; i++)
	{
		if (!inquiry (i))
		{
			/* If inquiry is OK it's not a BIODATA! */
			lance_target = i;
			break;
		}
	}
	
	if (lance_target < 0)
	{
		UNLOCK;
		c_conws ("DP8390: No device found!\n");
		return -1;
	}
	
	if (!(hwaddr = read_hw_addr(lance_target)))
	{
		send = 1;
		lance_int (nif);
		c_conws ("DP8390: Unable to read HW address!\n");
		return -1;
	}
	
	memcpy (nif->hwbrcst.adr.bytes, "\377\377\377\377\377\377", ETH_ALEN);
	memcpy (nif->hwlocal.adr.bytes, hwaddr, ETH_ALEN);
	
	send = 1;
	lance_int (nif);
	
	return 0;
}

/*
 * probe for mirrored ST RAM
 */

static void
lance_probe_c (void)
{
	register volatile ulong saveval;
	register volatile ulong *address;
	COOKIE *cookie = COOKIEBASE;
	
	address = dma_buffer.data.longs;
	saveval = *address;
	*address = 0xFFAA5500l;
	if (*(ulong *)((ulong)address | 0xFF000000l) == 0xFFAA5500l)
	{
		*address = 0xAA5500FFl;
		if (*(ulong *)((ulong)address | 0xFF000000l) == 0xAA5500FFl)
		{
			*address = 0x5500FFAAl;
			if (*(ulong *)((ulong)address | 0xFF000000l) == 0x5500FFAAl)
			{
				*address = 0x00FFAA55l;
				if (*(ulong *)((ulong)address | 0xFF000000l) == 0x00FFAA55l)
					ram_ok = -1;
			}
		}
	}
	*address = saveval;
	
	if (cookie)
	{
		while (cookie->name)
		{
			if ((cookie->name == 0x5f4d4348 /* _MCH */) && ((cookie->val>>16) == 3 /* Falcon */))
				return;
			cookie++;
		}
	}
		
	if (LOCK)
	{
		DBOTH;
		DMALOW;
		DMAMID;
		DMAHIGH;
		UNLOCK;
		has_acsi = -1;
	}
}

/*
 * LANCE interrupt routine
 */
static void
timeout_int(PROC *proc, long arg)
{
	pending--;
	lance_int (&if_lance);
}

static void
lance_int (struct netif *nif)
{
	int len, i;
	BUF *buf;
	
	if (!up)
		return;
	
	if (LOCK || send)
	{
		send = 0;
		
		i = 0;
		
again:
		while ((len = receivepkt (lance_target, (uchar *)&dma_buffer)) > 0)
		{
			i = 1;
			if ((buf = buf_alloc (len+100, 50, BUF_NORMAL)))
			{
				buf->dend = buf->dstart + len;
				memcpy (buf->dstart, (uchar *)((ulong)dma_buffer.data.bytes | 0xFF000000l), len);
				if (nif->bpf)
					bpf_input (nif, buf);
				if (!if_input (nif, buf, 0, eth_remove_hdr (buf)))
					nif->in_packets++;
				else
					nif->in_errors++;
			}
			else
				nif->in_errors++;
		}
		if ((buf = if_dequeue (&nif->snd)))
		{
			len = buf->dend - buf->dstart;
			memcpy ((uchar *)((ulong)dma_buffer.data.bytes | 0xFF000000l), buf->dstart, len);
			if (len < MIN_LEN)
				len = MIN_LEN;
			buf_deref (buf, BUF_NORMAL);
			if (sendpkt (lance_target, dma_buffer.data.bytes, len))
				nif->out_errors++;
			else
			{
				nif->out_packets++;
				i = 1;
			}
			goto again;
		}
		if (i)
		{
			for (i = 0 ; i < 6 ; i++)
				if (addroottimeout (TIMEOUTCOUNTER>>i, timeout_int, 0))
					pending++;
		}
		UNLOCK;
	}
	
	/*
	 * This can be removed when the if_lance.timer works (at the moment 
	 * if_lance.timer is called every second independent of the value of
	 * if_lance.timer).
	 */
	if (!pending)
		if (addroottimeout (TIMEOUTCOUNTER, timeout_int, 0))
			pending++;
}

/* sendpkt() sends a packet and returns a value of zero when the packet was sent
             successfully */

static int
sendpkt (int target, uchar *buffer, int length)
{
	if (!send_first(target, WRITEPKT, BIOMODE))
	{
		setup_dma (buffer, WRITE, 4);
		if (!send_other (length & 0xFF, NODMA) &&
		    !send_other (length >> 8, DMA)     &&
		    !timeout (TIMEOUTDMA)              &&
		    !send_other (0, NODMA)             &&
		    !send_other (0, NODMA))
		{
			WRITEMODE(REG_ACSI | DMA_WINDOW | DMA_FDC | A1);
			DACCESS = 0;
			WRITEMODE(REG_ACSI | DMA_WINDOW | DMA_FDC);
			DACCESS;
			WRITEMODE(REG_ACSI | DMA_WINDOW | DMA_FDC | A1);
			return 0;
		}
	}
	return -1;
}

/* receivepkt() loads a packet to a given buffer and returns its length */

static int
receivepkt (int target, uchar *buffer)
{
	int ret = -1;
	
	if (!send_first (target, READPKT, BIOMODE))
	{
		setup_dma (buffer, READ, 4);
		if (!send_other (0, DMA) &&
		    !timeout (TIMEOUTDMA))
		{
			get_status ();
			ret = calc_received (buffer);
		}
	}
	WRITEMODE(REG_ACSI | DMA_WINDOW | DMA_FDC);
	DACCESS;
	WRITEMODE(REG_ACSI | DMA_WINDOW | DMA_FDC | A1);
	
	return ret;
}

/* inquiry() returns 0 when BIODATA DMA found, -1 when timeout, -2 otherwise */

static int 
inquiry (int target)
{
	int ret = -1;
	
	if (send_first (target, TUR, NORMAL))
		goto hwaddr;
	dma_buffer.data.longs[0] = dma_buffer.data.longs[1] = 0;
	if (send_1_5 (0, dma_buffer.data.bytes, NODMA) ||
	    timeout (TIMEOUTDMA) ||
	    (get_status () == 0xFF))
		goto hwaddr;
	else
		goto bad;
hwaddr:
	if (!read_hw_addr (target))
		goto bad;
	ret = 0;
bad:
	return ret;
}

/* 
 * readhwaddr() reads the hwaddr and returns
 * a pointer to it or 0 in case of an error
 */
static HADDR *
read_hw_addr (int target)
{
	if (!send_first (target, GETETH, BIOMODE))
	{
		if (!get_1_6 (dma_buffer.data.bytes))
			return ((HADDR *)dma_buffer.data.bytes);
	}
	return 0;
}

static int
set_status (int target, uchar status)
{
	int i = -1;
	
	if (!send_first (target, SETSTAT, BIOMODE))
	{
		WRITEBOTH(status, DMA_FDC | DMA_WINDOW | REG_ACSI | A1);
		i = timeout(TIMEOUTCMD);
		DACCESS;
	}
	return i;
}

static int
timeout (long timeoutcounter)
{
	long i;
	
	i = HZ200 + timeoutcounter;
	while (HZ200 - i < 0)
		if (INT)
			return 0;
	
	return -1;
}

static void 
setup_dma (void *address, ushort rw_flag, int num_blocks)
{
	WRITEMODE((ushort) rw_flag          | DMA_FDC | SEC_COUNT | REG_ACSI |
		  A1);
	WRITEMODE((ushort)(rw_flag ^ WRITE) | DMA_FDC | SEC_COUNT | REG_ACSI |
		  A1);
	WRITEMODE((ushort) rw_flag          | DMA_FDC | SEC_COUNT | REG_ACSI |
		  A1);
	DMALOW  = (uchar)((ulong)address & 0xFF);
	DMAMID  = (uchar)(((ulong)address >>  8) & 0xFF);
	DMAHIGH = (uchar)(((ulong)address >> 16) & 0xFF);
	WRITEBOTH((ushort)num_blocks & 0xFF,
		  rw_flag | DMA_FDC | DMA_WINDOW | REG_ACSI | A1);
	rw = rw_flag;
}

static int 
send_first (int target, uchar byte, int biomode)
{
	int i = 0;
	
	rw = READ;
	/*
	 * wake up ACSI
	 */
	WRITEMODE(DMA_FDC | DMA_WINDOW | REG_ACSI);
	/*
	 * write command byte
	 */
	WRITEBOTH((target << 5) | (byte & 0x1F), DMA_FDC |
	          DMA_WINDOW | REG_ACSI | A1);
# if STANDARD_SEND_FIRST
	i = timeout(TIMEOUTCMD);
	if (biomode == BIOMODE)
		DACCESS; 
# else
	LONGDELAY;
	if (biomode == BIOMODE)
	{
		for (i = 0 ; i < 500 ; i++)
		{
			DELAY;
			if (INT)
				break;
		}
		if (i < 500)
			i = 0;
		else
			i = -1;
		DACCESS;
	}
# endif
	else
		i = timeout(TIMEOUTCMD);
	
	return i;
}

static int
send_other (uchar byte, int dma)
{
	WRITEBOTH (byte, rw | REG_ACSI | DMA_WINDOW | DMA_FDC | A1);
	
	if (dma != DMA)
	{
		if (timeout (TIMEOUTCMD))
			return -1;
		else
			DACCESS;
	}
	else
		DACCESS;
	
	if (dma == DMA)
		WRITEMODE (rw | REG_ACSI | DMA_WINDOW | DMA_ACSI | A1);
	
	return 0;
}

static int 
send_1_5 (int lun, uchar *command, int dma)
{
	int i, j;
	
	for (i = 0; i < 5; i++)
	{
		WRITEBOTH ((!i ? (((lun & 0x7) << 5) | (command[i] & 0x1F))
			      : command[i]),
			  rw | REG_ACSI | DMA_WINDOW |
			   ((i < 4) ? DMA_FDC
				    : (dma == DMA ? DMA_ACSI
					   : DMA_FDC)) | A1);
		if (i < 4 && (j = timeout (TIMEOUTCMD)))
			return j;
	}
	return 0;
}

static int
get_1_6 (uchar *buffer)
{
	int i, j;
	
	WRITEMODE (DMA_FDC | DMA_WINDOW | REG_ACSI | A1);
	
	for (i = 0; i < 6; i++)
	{
		DACCESS = 0;
		if ((j = timeout (TIMEOUTCMD)))
			return j;
		buffer[i] = (uchar)(DACCESS & 0xFF);
	}
	return 0;
}

static int 
get_status (void)
{
	WRITEMODE (DMA_FDC | DMA_WINDOW | REG_ACSI | A1);
	return ((int)(DACCESS & 0xFF));
}

static int 
calc_received (void *start_address)
{
	register int i;
	
	i = ((DMABUFFER *)start_address)->length;
	__asm__ volatile
	(
		"rolw #8,%0"
		:
		: "d"(i)
	);
	
	return i;
}
