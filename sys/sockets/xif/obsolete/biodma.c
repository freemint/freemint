/*
 **********************************************************************
 *                                                                    *
 * Driver for BIODATA DMA ethernet adaptors (C) 1995 by Torsten Lang. *
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
 * Important: The driver must be loaded to ST RAM, otherwise weird things
 *            will happen!!!
 *
 * TO DOs:
 * - I don't know how the adaptor reacts when heavy traffic floods the 
 *   receive buffers. The final driver should handle this.
 * - Multicasting may never work since it is not implemented in the firmware.
 *   So it would have to be done by using the promiscuous mode.
 * - Cache management isn't implemented yet - so you may run into trouble on
 *   processors > 68020 - but it seems that MiNT is clearing the caches when
 *   performing context switching.
 *
 * Thanks to...
 * - Kay Roemer for his great networking package and his support.
 * - Lisa GmbH
 *   Hans-Peter Jansen
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
 * 01/15/94, Kay Roemer,
 * 01/18/96, Torsten Lang.
 */

#include <string.h>
#include "config.h"
#include "netinfo.h"
#include "kerbind.h"
#include "atarierr.h"
#include "sockios.h"
#include "sockerr.h"
#include "buf.h"
#include "if.h"
#include "ifeth.h"
#include "biodma.h"
#include "util.h"

void (*ihandler) (void) = 0;

static struct netif if_lance;

static long	lance_open	(struct netif *);
static long	lance_close	(struct netif *);
static long	lance_output	(struct netif *, BUF *, char *, short, short);
static long	lance_ioctl	(struct netif *, short, long);

static long	lance_probe	(struct netif *);
static long	lance_reset	(struct netif *);
static void	lance_install_ints (void);
static void	lance_int	(struct netif *);
static void	timeout_int(void);

static int	sendpkt (int target, u_char *buffer, int length);
static int	receivepkt (int target, u_char *buffer);
static HADDR	*read_hw_addr(int target);
static int	set_status(int target, u_char status);
static int	inquiry (int target);
static int	timeout (long timeoutcounter);
static void	setup_dma (void *address, u_short rw_flag, int num_blocks);
static int	send_first (int target, u_char byte, int biomode);
static int	send_other (u_char byte, int dma);
static int	send_1_5 (int lun, u_char *command, int dma);
static int	get_1_6 (u_char *buffer);
static int	get_status (void);
static int	calc_received (void *start_address);

DMABUFFER dma_buffer;
u_short rw;
int lance_target = -1;
int up = 0;
int pending = 0;
int send = 0;

static long
lance_open (nif)
	struct netif *nif;
{
	int i;

	if (!LOCK)
		return -1;
	up = 1;
	i = set_status(lance_target, BROADCAST);
	send = 1;
	lance_int(nif);

	return i;
}

static long
lance_close (nif)
	struct netif *nif;
{
	up = 0;

	return 0;
}

static long
lance_output (nif, buf, hwaddr, hwlen, pktype)
	struct netif *nif;
	BUF *buf;
	char *hwaddr;
	short hwlen, pktype;
{
	short len;
	BUF *nbuf;
	int i, j;
	long r;

	if (0 == (nbuf = eth_build_hdr (buf, nif, hwaddr, pktype))) {
		++nif->out_errors;
		return ENSMEM;
	}
	if (nif->bpf)
		bpf_input (nif, nbuf);

	if (!up) {
		buf_deref (nbuf, BUF_NORMAL);
		return -1;
	}

	if (!LOCK) {
		if ((r = if_enqueue (&nif->snd, buf, buf->info))) {
			return r;
		}
		return 0;
	}

	len = nbuf->dend - nbuf->dstart;
	memcpy (dma_buffer.data, nbuf->dstart, len);
	if (len < MIN_LEN)
		len = MIN_LEN;
	buf_deref (nbuf, BUF_NORMAL);
	if ((j = sendpkt (lance_target, dma_buffer.data, len)))
		++nif->out_errors;
	else {
		for (i = 1; i < 5; i++)
			if (addroottimeout(TIMEOUTCOUNTER>>i, timeout_int, 0))
				pending++;
	++nif->out_packets;
	}

	send = 1;
	lance_int(nif);

	return(j);
}

static long
lance_ioctl (nif, cmd, arg)
	struct netif *nif;
	short cmd;
	long arg;
{
	switch (cmd) {
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
	return EINVFN;
}

long
driver_init (void)
{
	static char message[100];

	strcpy (if_lance.name, "de");
	if_lance.unit = if_getfreeunit ("de");
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

	if (lance_probe (&if_lance) < 0) {
		c_conws ("BIODATA DMA adaptor: driver not installed.\r\n");
		return 1;
	}
	lance_install_ints ();

	if_register (&if_lance);

	sprintf (message, "DP8390 driver for BIODATA DMA v%s (de%d) "
		 "(%02x:%02x:%02x:%02x:%02x:%02x) on target %d\r\n",
		"0.3á",
		if_lance.unit,
		if_lance.hwlocal.addr[0],
		if_lance.hwlocal.addr[1],
		if_lance.hwlocal.addr[2],
		if_lance.hwlocal.addr[3],
		if_lance.hwlocal.addr[4],
		if_lance.hwlocal.addr[5],
		lance_target);
	c_conws (message);
	return 0;
}

static long
lance_reset (nif)
	struct netif *nif;
{
	int i;

	if (!LOCK)
		return -1;
	i = set_status(lance_target, BROADCAST);
	send = 1;
	lance_int(nif);

	return i;
}

static long
lance_probe (nif)
	struct netif *nif;
{
	int i;
	HADDR *hwaddr;
	
	if (!LOCK)
		return -1;

	for (i=0; i<8; i++)
		if (!inquiry(i)) {
			lance_target = i;
			break;
		}
	if (lance_target < 0) {
		UNLOCK;
		return -1;
	}
	if (!(hwaddr = read_hw_addr(lance_target))) {
		send = 1;
		lance_int(nif);
		return -1;
	}
	
	memcpy (nif->hwbrcst.addr, "\377\377\377\377\377\377", ETH_ALEN);
	memcpy (nif->hwlocal.addr, hwaddr, ETH_ALEN);

	send = 1;
	lance_int(nif);

	return(0);
}

/*
 * LANCE interrupt routine
 */
static void
timeout_int(void)
{
	pending--;
	lance_int (&if_lance);
}

static void
lance_int (nif)
	struct netif *nif;
{
	int len, i;
	BUF *buf;

	if (!up)
		return;

	if (!LOCK & !send)
		goto bad;

	send = 0;

	i = 0;
	while ((buf = if_dequeue(&nif->snd))) {
		i = 1;
		len = buf->dend - buf->dstart;
		memcpy (dma_buffer.data, buf->dstart, len);
		if (len < MIN_LEN)
			len = MIN_LEN;
		buf_deref (buf, BUF_NORMAL);
		if (sendpkt (lance_target, dma_buffer.data, len))
			++nif->out_errors;
		else
			++nif->out_packets;
	}
	while ((len = receivepkt(lance_target, (u_char *)&dma_buffer)) > 0) {
		buf = buf_alloc (len+100, 50, BUF_NORMAL);
		buf->dend = buf->dstart + len;
		memcpy (buf->dstart, dma_buffer.data, len);
		if (nif->bpf)
			bpf_input (nif, buf);
		if (!if_input (nif, buf, 0, eth_remove_hdr (buf)))
			++nif->in_packets;
		else
			++nif->in_errors;
	}
	if (i) {
		for (i = 0 ; i < 5 ; i++)
			if (addroottimeout(TIMEOUTCOUNTER>>i, timeout_int, 0))
				pending++;
	}
	UNLOCK;
bad:
	/*
	 * This can be removed when the if_lance.timer works (at the moment 
	 * if_lance.timer is called every second independent of the value of
	 * if_lance.timer).
	 */
	if (!pending && addroottimeout(TIMEOUTCOUNTER, timeout_int, 0))
		++pending;
}

extern void lance_vbi_int (void);
extern long old_vbi;

/*
 * Install interrupt handler
 */

static void
lance_install_ints (void)
{
}

/* sendpkt() sends a packet and returns a value of zero when the packet was sent
             successfully */

static int
sendpkt (target, buffer, length)
	int target;
	u_char *buffer;
	int length;
{
	int ret = -1;

	if (send_first(target, WRITEPKT, BIOMODE))
		goto bad;
	setup_dma(buffer, WRITE, 4);
	if (send_other(length & 0xFF, NODMA) ||
	    send_other(length >> 8, DMA) ||
	    timeout(TIMEOUTDMA))
		goto bad;
	if (send_other(0, NODMA))
		goto bad;
	if (send_other(0, NODMA))
		goto bad;
	WRITEMODE(REG_ACSI | DMA_WINDOW | DMA_FDC | A1);
	DACCESS = 0;
	WRITEMODE(REG_ACSI | DMA_WINDOW | DMA_FDC);
	DACCESS;
	WRITEMODE(REG_ACSI | DMA_WINDOW | DMA_FDC | A1);
	ret = 0;
bad:
	return (ret);
}

/* receivepkt() loads a packet to a given buffer and returns its length */

static int
receivepkt (target, buffer)
	int target;
	u_char *buffer;
{
	int ret = -1;

	if (send_first(target, READPKT, BIOMODE))
		goto bad;
	setup_dma(buffer, READ, 4);
	if (send_other(0, DMA) ||
	    timeout(TIMEOUTDMA))
		goto bad;
	get_status();
	ret = calc_received(buffer);
bad:
	WRITEMODE(REG_ACSI | DMA_WINDOW | DMA_FDC);
	DACCESS;
	WRITEMODE(REG_ACSI | DMA_WINDOW | DMA_FDC | A1);

	return (ret);
}

/* inquiry() returns 0 when BIODATA DMA found, -1 when timeout, -2 otherwise */

static int 
inquiry (target)
	int target;
{
	int ret = -1;

	if (send_first(target, TUR, NORMAL))
		goto hwaddr;
	dma_buffer.data[0] = dma_buffer.data[1] = dma_buffer.data[2] = dma_buffer.data[3] = dma_buffer.data[4] = 0;
	if (send_1_5(0, dma_buffer.data, NODMA) ||
	    timeout(TIMEOUTDMA) ||
	    (get_status() == 0xFF))
		goto hwaddr;
	else
		goto bad;
hwaddr:
	if (!read_hw_addr(target))
		goto bad;
	ret = 0;
bad:
	return (ret);
}

/* 
 * readhwaddr() reads the hwaddr and returns
 * a pointer to it or 0 in case of an error
 */
static HADDR 
*read_hw_addr(target)
	int target;
{
	HADDR *ret = 0;

	if (send_first(target, GETETH, BIOMODE))
		goto bad;
	if (!get_1_6(dma_buffer.data))
		ret = (HADDR *)dma_buffer.data;
bad:
	return (ret);
}

static int
set_status(target, status)
	int    target;
	u_char status;
{
	int i = -1;

	if (send_first(target, SETSTAT, BIOMODE))
		{c_conws("s1");goto bad;}
	WRITEBOTH(status, DMA_FDC | DMA_WINDOW | REG_ACSI | A1);
	if ((i = timeout(TIMEOUTCMD)))
		{c_conws("s2");goto bad;}
	DACCESS;
bad:
	return i;
}

static int
timeout (timeoutcounter)
	long timeoutcounter;
{
	long i;
	
	i = HZ200 + timeoutcounter;
	while (HZ200 - i < 0)
		if (INT) return (0);

	return (-1);
}

static void 
setup_dma (address, rw_flag, num_blocks)
	void *address;
	u_short rw_flag;
	int num_blocks;
{
	WRITEMODE((u_short) rw_flag          | DMA_FDC | SEC_COUNT | REG_ACSI |
		  A1);
	WRITEMODE((u_short)(rw_flag ^ WRITE) | DMA_FDC | SEC_COUNT | REG_ACSI |
		  A1);
	WRITEMODE((u_short) rw_flag          | DMA_FDC | SEC_COUNT | REG_ACSI |
		  A1);
	DMALOW  = (u_char)((u_long)address & 0xFF);
	DMAMID  = (u_char)(((u_long)address >>  8) & 0xFF);
	DMAHIGH = (u_char)(((u_long)address >> 16) & 0xFF);
	WRITEBOTH((u_short)num_blocks & 0xFF,
		  rw_flag | DMA_FDC | DMA_WINDOW | REG_ACSI | A1);
	rw = rw_flag;
}

static int 
send_first (target, byte, biomode)
	int    target;
	u_char byte;
	int    biomode;
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
/*	i = timeout(TIMEOUTCMD);
	if (biomode == BIOMODE)
		DACCESS; */
	if (biomode == BIOMODE) {
		for (i = 0 ; i < 500 ; i++) {
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
	else
		i = timeout(TIMEOUTCMD);

	return i;
}

static int
send_other(byte, dma)
	u_char byte;
	int    dma;
{
	WRITEBOTH(byte, rw | REG_ACSI | DMA_WINDOW | DMA_FDC | A1);
	if (timeout(TIMEOUTCMD))
		return -1;
	else
		DACCESS;
	if (dma == DMA)
		WRITEMODE(rw | REG_ACSI | DMA_WINDOW | DMA_ACSI | A1);
	return 0;
}

static int 
send_1_5 (lun, command, dma)
	int lun;
	u_char *command;
	int dma;
{
	int i, j;
	
	for (i=0; i<5; i++) {
		WRITEBOTH((!i ? (((lun & 0x7) << 5) | (command[i] & 0x1F))
			      : command[i]),
			  rw | REG_ACSI | DMA_WINDOW |
			   ((i < 4) ? DMA_FDC
				    : (dma == DMA ? DMA_ACSI
					   : DMA_FDC)) | A1);
		if (i < 4 && (j = timeout(TIMEOUTCMD)))
			return (j);
	}
	return 0;
}

static int
get_1_6 (buffer)
	u_char *buffer;
{
	int i, j;

	WRITEMODE(DMA_FDC | DMA_WINDOW | REG_ACSI | A1);

	for (i=0; i<6; i++) {
		DACCESS = 0;
		if ((j = timeout(TIMEOUTCMD)))
			return (j);
		buffer[i] = (u_char)(DACCESS & 0xFF);
	}
	return (0);
}

static int 
get_status (void)
{
	WRITEMODE(DMA_FDC | DMA_WINDOW | REG_ACSI | A1);
	return ((int)(DACCESS & 0xFF));
}

static int 
calc_received (start_address)
	void *start_address;
{
	register int i;

	i = ((DMABUFFER *)start_address)->length;
	__asm__ volatile("rolw #8,%0" : : "d"(i));
	return i;
}
