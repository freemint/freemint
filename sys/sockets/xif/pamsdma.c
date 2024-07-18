/*
 * Driver for PAMs DMA ethernet adaptors (C) 1995 by Torsten Lang.
 * Ethernet driver skeleton (C) Kay Roemer
 *
 * This driver has been developed and tested on a 8MHz ST. If you run in
 * on substantially faster machines a few wait()'s may be needed here and
 * there. Please contact me in case of success or failure...
 *
 * Important: The driver must be loaded to ST RAM, otherwise weird things
 *            will happen!!!
 *
 * The driver uses the interface name "de?" (dma ethernet with ? from 0..9)
 *
 * History:
 * V0.1�	The first quite stable released version.
 *		- enqueueing of send packets if DMA locked
 *		- cache management still missing
 *		- problems on fast computers with heavy ACSI load (many
 *		  attached devices)
 * V0.1� (net-090)
 *		- minor changes against the other V0.1�
 * V0.3�	- CPU transfers from/to the DMA buffer are done through
 *		  mirrored ST RAM. This is not a real good replacement for
 *		  cache management but causes less overhead and at least is
 *		  better than nothing at all!
 * V0.4		Never released...
 *		- kicked off the beta state since there aren't any
 *		  complaints, at least not concerning unaccelerated STs
 *		- reworked some comments...
 *		- changed TIMEOUTCMD constant. Maybe this caused trouble
 *		  sometimes...
 * V0.5		Never released...
 *		- tried to fix a problem concerning the IRQ line: With heavy
 *		  load on the ACSI bus it seems that the IRQ line stays active
 *		  too long and causes misinterpretations in the next command
 *		  phase. So we wait until it is not active any longer before
 *		  sending a command byte...
 * V0.6		Never released...
 *		- Ooops: Forget to count outgoing packets! Did noone recognize
 *		  that error???
 * V1.0		After some more work I finally decided to release a V1.0
 *		- Aieeeee! Fixed a bug concerning the command handling: At least
 *		  when writing data to the adaptor the dma_buffer is already used
 *		  by packet data so that we shouldn't copy our command bytes over it.
 *		  Why didn't anyone see that???
 *		- Fixed two bugs concerning the buffer handling in send_packet()
 *		  and receive_packet().
 * V1.2         Cleaned up the code, tried to fix timing probs on fast machines.
 *
 * TO DOs:
 * - I don't know how the adaptor reacts when heavy traffic floods the 
 *   receive buffers. The final driver should handle this.
 * - At the moment the adaptor is polled for received packets. It should be
 *   able to generate interrupts when selected. So the max. receive rate is
 *   somewhat limited which may be a problem with heavy broadcasting on the
 *   net.
 * - I don't know how to get the exact length of received packets. At the
 *   moment the DMA counter is checked to determine the packet length.
 * - Multicasting may never work since I don't know how to configure these
 *   internal things of the adaptor.
 * - Cache management isn't implemented yet. I used a trick to avoid
 *   problems. The buffer address is ORed with $FF000000 when the driver
 *   is accessing the buffer. A drawback is that this version of the driver
 *   no longer runs on machines not mirroring the ST RAM (e. g. Atari TT).
 * - At the moment the driver supports only one adaptor. It is NOT possible
 *   to start the driver under another name to support a second adaptor!
 *
 * Thanks to...
 * I have to thank Kay Roemer for his great networking package and his support.
 *
 * No thanks to...
 * I'm waiting for months to get support (information about the PAM's boards)
 * from:
 * GK Computer
 * M. V�lkel
 * 7800 Freiburg
 * Phone: 0761/409061
 * Fax:   0761/408617
 * who developed the PAM's DMA ethernet adaptors.
 *
 * Between a MEGA ST (PAK/3/48MHz/FPU/FRAK/4MB ST RAM/16MB TT RAM) and an
 * Amiga 4000 (CyberStorm 060/50MHz/102MB RAM) the driver reaches the 
 * following throughput:
 * ftp send:  ~ 200k/sec (Atari (/ram)      -> Amiga (ram:))
 * ftp recv:  ~ 170k/sec (Atari (/dev/null) <- Amiga (ram:))
 * tcp send:  ~ k/sec (tcpcl -> tcpsv)
 * tcp recv:  ~ k/sec (tcpsv <- tcpcl)
 * nfs read:  ~ k/sec
 * nfs write: ~ k/sec
 *
 * 01/15/94, Kay Roemer,
 * 09/14/95, Torsten Lang.
 */

# include "global.h"
# include "buf.h"
# include "inet4/if.h"
# include "inet4/ifeth.h"

# include "pamsdma.h"
# include "netinfo.h"

# include "mint/asm.h"
# include "mint/sockio.h"


static struct netif if_lance;

int ram_ok   = 0;
int has_acsi = 0;

void		(*lance_probe_c_)	(void);
extern void	lance_probe_asm		(void);
static long	lance_open		(struct netif *);
static long	lance_close		(struct netif *);
static long	lance_output		(struct netif *, BUF *, const char *, short, short);
static long	lance_ioctl		(struct netif *, short, long);

static long	lance_probe		(struct netif *);
static void	lance_probe_c		(void);
static long	lance_reset		(struct netif *);
static void	lance_int		(struct netif *);
static void	timeout_int		(PROC *proc, long arg);

static void	start		(int target);
static int	stop 		(int target);
static int	testpkt		(int target);
static int	sendpkt		(int target, uchar *buffer, int length);
static int	receivepkt	(int target, uchar *buffer);
static int	inquiry		(int target);
static HADDR	*read_hw_addr	(int target);
static int	timeout		(long timeoutcounter);
static int	wait_for_no_irq	(long timeoutcounter);
static void	setup_dma	(void *address, ushort rw_flag, int num_blocks);
static int	send_first	(int target, uchar byte);
static int	send_1_5	(int lun, uchar *command, int dma);
static int	get_status	(void);
static int	calc_received	(void *start_address);

uchar dma_buffer[2048];
uchar cmd_buffer[6];
ushort rw;
int lance_target = -1;
int up = 0;
int pending = 0;

static long
lance_open (struct netif *nif)
{
	if (!LOCK)
		return -1;
	if (!up)
		start (lance_target);
	up = 1;
	UNLOCK;
	return 0;
}

static long
lance_close (struct netif *nif)
{
	int i;
	if (!LOCK)
		return -1;
	up = 0;
	i = stop (lance_target);
	UNLOCK;
	return i;
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
			nif->out_errors++;
			return r;
		}
		
		return 0;
	}
	
	len = nbuf->dend - nbuf->dstart;
	memcpy ((uchar *)((ulong)dma_buffer | 0xFF000000l), nbuf->dstart, len);
	if (len < MIN_LEN)
		len = MIN_LEN;
	buf_deref (nbuf, BUF_NORMAL);
	i = 0;
	send_again:
	if ((j = sendpkt (lance_target, dma_buffer, len)))
	{
		nif->out_errors++;
		lance_reset (nif);
		if (++i < 3)
			goto send_again;
	}
	else
	{
		nif->out_packets++;
		for (i = 1; i < 7; i++)
			if (addroottimeout (TIMEOUTCOUNTER>>i, timeout_int, 0))
				pending++;
	}
	
	UNLOCK;
	
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
	if_lance.maxpackets = 0;
	
	c_conws ("DP8390: DMA adaptor driver (C) 1996, 1997, 1998 by T. Lang\r\n");
	
	if (lance_probe (&if_lance) < 0)
	{
		c_conws ("DP8390: DMA adaptor driver not installed.\r\n");
		return 1;
	}
	
	if_register (&if_lance);
	
	ksprintf (message, "DP8390: driver for PAM's DMA v%s (de%d) "
		 "(%02x:%02x:%02x:%02x:%02x:%02x) on target %d\r\n",
		"1.2",
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

/*
 * probe for mirrored ST RAM
 */

static void
lance_probe_c (void)
{
	register volatile ulong saveval;
	register volatile ulong *address;
	COOKIE *cookie = COOKIEBASE;
	
	address = (ulong *)dma_buffer;
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
		has_acsi=-1;
	}
}

static long
lance_reset (struct netif *nif)
{
	if (!LOCK)
		return -1;
	stop (lance_target);
	start (lance_target);
	UNLOCK;
	return 0;
}

static long
lance_probe (struct netif *nif)
{
	int i, j;
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
	
	for (i = 0; i < 16; i++)
	{
		/* Scan the bus twice */
		for (j = 0; j < 12; j++)
			get_status();	/* Clean up the ACSI bus for the case that a previously */
		inquiry(i & 7);		/* inquired target behaved strange.                     */
		if (!inquiry(i & 7))
		{
			lance_target = i & 7;
			break;
		}
	}
	if (lance_target < 0)
	{
		UNLOCK;
		c_conws ("DP8390: No device found!\r\n");
		return -1;
	}
	if (!(hwaddr = read_hw_addr (lance_target)))
	{
		UNLOCK;
		c_conws ("DP8390: Unable to read HW address!\r\n");
		return -1;
	}
	
	memcpy (nif->hwbrcst.adr.bytes, "\377\377\377\377\377\377", ETH_ALEN);
	memcpy (nif->hwlocal.adr.bytes, hwaddr, ETH_ALEN);
	
	UNLOCK;
	return(0);
}

/*
 * LANCE interrupt routine
 */
static void
timeout_int (PROC *proc, long arg)
{
	pending--;
	lance_int (&if_lance);
}

static void
lance_int (struct netif *nif)
{
	int len, i, j;
	BUF *buf;
	
	if (!up)
		return;
	
	if (LOCK)
	{
		i = 0;
		
		while ((buf = if_dequeue(&nif->snd)))
		{
			len = buf->dend - buf->dstart;
			memcpy ((uchar *)((ulong)dma_buffer | 0xFF000000l), buf->dstart, len);
			if (len < MIN_LEN)
				len = MIN_LEN;
			buf_deref (buf, BUF_NORMAL);
			for (j = 0; j < 3; j++)
			{
				if (sendpkt (lance_target, dma_buffer, len))
				{
					nif->out_errors++; /* errors may actually only be caused by hardware failure! */
					lance_reset (nif); /* so let's reset the adaptor for max. two times...        */
				}
				else
				{
					nif->out_packets++;
					i = 1;
					break;
				}
			}
		}
		while (testpkt(lance_target) > 0)
		{
			len = receivepkt(lance_target, dma_buffer);
			if ((buf = buf_alloc (len+100, 50, BUF_NORMAL)))
			{
				buf->dend = buf->dstart + len;
				memcpy (buf->dstart, (uchar *)((ulong)dma_buffer | 0xFF000000l), len);
				if (nif->bpf)
					bpf_input (nif, buf);
				if (!if_input (nif, buf, 0, eth_remove_hdr (buf))) {
					i = 1;
					nif->in_packets++;
				}
				else
					nif->in_errors++;
			}
			else
				nif->in_errors++;
		}
		if (i)
		{
			for (i = 0 ; i < 7 ; i++)
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

/* start() starts the PAM's DMA adaptor */

static void
start (int target)
{
	send_first (target, START);
}

/* stop() stops the PAM's DMA adaptor and returns a value of zero in case of success */

static int
stop (int target)
{
	if (!send_first(target, STOP))
	{
		cmd_buffer[0] = cmd_buffer[1] = cmd_buffer[2] =
		cmd_buffer[3] = cmd_buffer[4] = 0;
		if (!send_1_5(lance_target, cmd_buffer, 0) &&
		    !timeout(TIMEOUTDMA) &&
		    !get_status())
		return 0;
	}
	
	return -1;
}

/* testpkt() returns the number of received packets waiting in the queue */

static int
testpkt (int target)
{
	if (!send_first (target, NUMPKTS))
		return get_status();
	
	return -1;
}

/* sendpkt() sends a packet and returns a value of zero when the packet was sent
             successfully */

static int
sendpkt (int target, uchar *buffer, int length)
{
	if (!send_first (target, WRITEPKT))
	{
		setup_dma (buffer, WRITE, 3);
		cmd_buffer[0] = cmd_buffer[1] = cmd_buffer[4] = 0;
		cmd_buffer[2] = length >> 8;
		cmd_buffer[3] = length & 0xFF;
		if (!send_1_5 (7, cmd_buffer, 1) && 
		    !timeout (TIMEOUTDMA))
			return get_status();
	}
	return -1;
}

/* receivepkt() loads a packet to a given buffer and returns its length */

static int
receivepkt (int target, uchar *buffer)
{
	if (!send_first(target, READPKT))
	{
		setup_dma (buffer, READ, 3);
		cmd_buffer[0] = cmd_buffer[1] = cmd_buffer[2] = cmd_buffer[4] = 0;
		cmd_buffer[3] = 3;
		if (!send_1_5 (7, cmd_buffer, 1) &&
		    !timeout (TIMEOUTDMA)        &&
		    !get_status ())
			return calc_received (dma_buffer);
	}
	return -1;
}

/* inquiry() returns 0 when PAM's DMA found, -1 when timeout, -2 otherwise */

static int 
inquiry (int target)
{
	long i;
	
	if (!send_first (target, INQUIRY))
	{
		setup_dma (dma_buffer, READ, 1);
		cmd_buffer[0] = cmd_buffer[1] = cmd_buffer[2] = cmd_buffer[4] = 0;
		cmd_buffer[3] = 48;
		bzero ((uchar *)(((ulong)dma_buffer | 0xFF000000l) + 8), 20);
		if (!send_1_5 (5, cmd_buffer, 1) && !timeout(TIMEOUTDMA) &&
		    /*
		     * The following delay is needed because:
		     * When reading data and the target is assigning IRQ the last
		     * 16 bytes to be transfered may still hang around somewhere in
		     * the DMA's fifo. If you now go on without explicitly waiting
		     * for these last bytes you may miss them!!!
		     */
		    !({i = HZ200 + 2; while (HZ200 < i) {}; (calc_received (dma_buffer) < 32);}) &&
		    !get_status () &&
		    !memcmp (inquire8, (uchar *)(((ulong)dma_buffer + 8) | 0xFF000000l), 20))
			return 0;
	}
	
	return -1;
}

/* 
 * readhwaddr() reads the sector containing the hwaddr and returns
 * a pointer to it or 0 in case of an error
 */
static HADDR *
read_hw_addr (int target)
{
	if (!send_first (target, READSECTOR))
	{
		setup_dma (dma_buffer, READ, 1);
		cmd_buffer[0] = cmd_buffer[1] = cmd_buffer[2] = cmd_buffer[4] = 0;
		cmd_buffer[3] = 1;
		if (!send_1_5 (5, cmd_buffer, 1) &&
		    !timeout (TIMEOUTDMA)        &&
		    !get_status ())
			return ((HADDR *)&(((DMAHWADDR *)((ulong)dma_buffer | 0xFF000000l))->hwaddr));
	}
	return NULL;
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

static int
wait_for_no_irq (long timeoutcounter)
{
	long i;
	
	i = HZ200 + timeoutcounter;
	while (HZ200 - i < 0)
		if (!INT)
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
send_first (int target, uchar byte)
{
	rw = READ;
	/*
	 * wait until IRQ is not active any longer (maybe active from last access ?!)...
	 * we don't leave in case of failure since the only way to reanimate a frozen ACSI
	 * is to try again and again (we just want to avoid any misinterpretation of the IRQ
	 * status)...
	 */
	wait_for_no_irq (TIMEOUTDMA);
	/*
	 * wake up ACSI
	 */
	WRITEMODE (DMA_FDC | DMA_WINDOW | REG_ACSI);
	/*
	 * write command byte
	 */
	WRITEBOTH ((target << 5) | (byte & 0x1F), DMA_FDC |
	          DMA_WINDOW | REG_ACSI | A1);
	DELAY; DELAY; DELAY; /* wait ~36us */
	
	return (timeout (TIMEOUTCMD));
}

static int 
send_1_5 (int lun, uchar *command, int dma)
{
	int i, j;
	
	for (i = 0; i < 5; i++)
	{
		WRITEBOTH((!i ? (((lun & 0x7) << 5) | (command[i] & 0x1F))
			      : command[i]),
			  rw | REG_ACSI | DMA_WINDOW |
			   ((i < 4) ? DMA_FDC
				    : (dma ? DMA_ACSI
					   : DMA_FDC)) | A1);
		DELAY; DELAY; DELAY; /* wait ~36us */
		if (i < 4 && (j = timeout(TIMEOUTCMD)))
			return j;
	}
	return 0;
}

static int 
get_status (void)
{
	WRITEMODE(DMA_FDC | DMA_WINDOW | REG_ACSI | A1);
	return ((int)(DACCESS & 0xFF));
}

static int 
calc_received (void *start_address)
{
	return (int)(
		(((ulong)DMAHIGH << 16) | ((ushort)DMAMID << 8) | DMALOW)
	      - (ulong)start_address);
}
