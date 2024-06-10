/*
 * V4SA ethernet driver for FreeMiNT.
 *
 * Author:
 *  Michael Grunditz
 * 
 * Contributors:
 *  Kay Roemer, Anders Granlund
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
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

#include "global.h"
#include "buf.h"
#include "cookie.h"
#include "inet4/if.h"
#include "inet4/ifeth.h"
#include "netinfo.h"
#include "v4net.h"
#include "arch/tosbind.h"
#include "mint/sockio.h"
#include "mint/osbind.h"
#include "mint/ssystem.h"

#define V4NET_DEBUG	0

static char dbgbuf[256];
#define DPRINT(...)			{ ksprintf(dbgbuf, __VA_ARGS__); c_conws("v4net: "); c_conws(dbgbuf); c_conws("\n\r"); }
#if V4NET_DEBUG > 0
#define DWARN(...)			DPRINT(__VA_ARGS__)
#else
#define DWARN(...)			
#endif
#if V4NET_DEBUG > 1
#define DDBG(...)			DPRINT(__VA_ARGS__)
#else
#define DDBG(...)			
#endif


#define V4_INTENA2			(*(volatile u_int16_t*)0xdff29a)
#define V4_INTENA2R			(*(volatile u_int16_t*)0xdff21c)
#define V4_INTREQ2			(*(volatile u_int16_t*)0xdff29c)
#define V4_INTREQ2R			(*(volatile u_int16_t*)0xdff21e)

#define V4_ETH_MAC1			(*(volatile u_int32_t*)0xde0020)
#define V4_ETH_MAC2			(*(volatile u_int32_t*)0xde0024)
#define V4_ETH_MULTI1		(*(volatile u_int32_t*)0xde0028)
#define V4_ETH_MULTI2		(*(volatile u_int32_t*)0xde002c)
#define V4_ETH_DMA_START	(*(volatile u_int32_t*)0xde0030)
#define V4_ETH_DMA_END		(*(volatile u_int32_t*)0xde0034)
#define V4_ETH_SW_WRITE		(*(volatile u_int32_t*)0xde0038)
#define V4_ETH_HW_WRITE		(*(volatile u_int32_t*)0xde003c)
#define V4_ETH_TXFIFO		(*(volatile u_int32_t*)0xde0040)
#define V4_ETH_TXQUEUE		(*(volatile u_int32_t*)0xde0044)


#define MEMSIZE 204800UL

/*
 * Our interface structure
 */
typedef struct _v4_ethernet_t
{
#if 1
	u_int16_t dma;	
	u_int8_t  mac[6];
 #else
	u_int32_t mac1;         /* DMA_ON<<31 | MAC(0:15)                   */
	u_int32_t mac2;         /* MAC(16:47)                               */
#endif
	u_int32_t multicast1;   /* high order bits multicast hash           */
	u_int32_t multicast2;   /* low  order bits multicast hash           */
	u_int32_t rx_start;     /* DMA buffer start (aligned to 2048 bytes) */
	u_int32_t rx_stop;      /* DMA buffer end (aligned to 2048 bytes)   */
	u_int32_t rx_swpos;     /* RX software position (barrier)           */
	u_int32_t rx_hwpos;     /* hardware RX write position               */
	u_int32_t txword;       /* write port for TX FIFO (32 Bits)         */
	u_int32_t txqueuelen;   /* number of words in TX FIFO               */
} v4_ethernet_t;


/*
 * Prototypes for our service functions
 */
static long	v4net_open(struct netif *);
static long	v4net_close(struct netif *);
static long	v4net_output(struct netif *, BUF *, const char *, short, short);
static long	v4net_ioctl(struct netif *, short, long);
static long	v4net_config(struct netif *, struct ifopt *);
void _cdecl v4net_recv(void);


volatile v4_ethernet_t * v4e = (v4_ethernet_t*)0xde0020;
static struct netif if_v4net;
static u_int8_t hwtmp[6];
volatile u_int32_t v4net_swposcopy = 0;
static volatile u_int32_t alignmem = 0;
static volatile u_int32_t upcfg = 0;

static long
v4net_open (struct netif *nif)
{
	upcfg = (1L << 31);
	V4_ETH_MAC1 = (upcfg | (hwtmp[1]<<8) | hwtmp[0]);
#if !V4NET_200HZ
	V4_INTENA2 = 0xA000;
#endif
	return 0;
}

static long
v4net_close (struct netif *nif)
{
	upcfg = 0;
	V4_ETH_MAC1 = (upcfg | (hwtmp[1]<<8) | hwtmp[0]);
#if !V4NET_200HZ
	V4_INTENA2 = 0x2000;
#endif
	return 0;
}

void _cdecl
v4net_recv(void)
{
	BUF *buf; 
	short type, frame_len;
	u_int32_t lenlong, tmp, tmp2;
	u_int32_t *dst,*src;
	struct netif *nif = &if_v4net;

	while(1)
	{
		tmp = v4net_swposcopy;
		tmp2 = (v4net_swposcopy + 2048);
		if (tmp2 >= (alignmem + MEMSIZE))
		{
			DDBG("v4net_swposcopy wrap : 0x%x", v4net_swposcopy);
			tmp2=alignmem;
		}
		v4net_swposcopy = tmp2;

		frame_len = (short) *((short*)tmp2+3);
		DDBG("swposcopy: 0x%lx, len: %d", tmp2, frame_len);
		*((short*)tmp2+3)=(short)0;

		frame_len = frame_len & 2047;
		if (frame_len < 14 || frame_len > 1535)
		{
			DWARN("Size error: %d", frame_len);
			nif->in_errors++;
			if (V4_ETH_HW_WRITE != v4net_swposcopy)
				continue;
			break;
		}

		buf = buf_alloc(frame_len + 200, 100, BUF_ATOMIC);
		if (!buf)
		{
			DWARN("Buf alloc failed");
			nif->in_errors++;
			break;
		}

		lenlong = (frame_len + 3UL) >> 2;
		buf->dstart = (char*)((((u_int32_t)(buf->dstart)) + 3) & 0xFFFFFFFCUL);
		buf->dend = buf->dstart + (lenlong << 2);

		src = (u_int32_t*)(tmp2 + 8);
		dst = (u_int32_t*)(buf->dstart);

		while (lenlong >= 16)
		{
			*dst++ = *src++; *dst++ = *src++; *dst++ = *src++; *dst++ = *src++;
			*dst++ = *src++; *dst++ = *src++; *dst++ = *src++; *dst++ = *src++;
			*dst++ = *src++; *dst++ = *src++; *dst++ = *src++; *dst++ = *src++;
			*dst++ = *src++; *dst++ = *src++; *dst++ = *src++; *dst++ = *src++;
			lenlong -= 16;
		}
		while (lenlong)
		{
			*dst++ = *src++;
			lenlong--;
		}

		if (nif->bpf)
			bpf_input (nif, buf);

		type = eth_remove_hdr(buf);
		if(!if_input(nif, buf, 0UL, type))
			nif->in_packets++;
		else
			nif->in_errors++;

		if (tmp != alignmem)
			V4_ETH_SW_WRITE = tmp;

		if (V4_ETH_HW_WRITE == v4net_swposcopy)
			break;
	}
}


static long
v4net_output(struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	BUF *nbuf;
	u_int32_t i,tlen,rounded_len,stufflen;
	u_int32_t* p;

	nbuf = eth_build_hdr(buf, nif, hwaddr, pktype);
	if (nbuf == 0)
	{
		DWARN("send nonbuf");
		nif->out_errors++;
		return ENOMEM;
	}
	nif->out_packets++;
	
	if (nif->bpf)
		bpf_input (nif, nbuf);

	tlen = (nbuf->dend) - (nbuf->dstart);
	rounded_len = ((tlen + 3UL) & 0xFFFC);
	//DDBG("tx q:%ld, tlen:%ld, rlen:%ld", V4_ETH_TXQUEUE, tlen, rounded_len);
	if (rounded_len > 1536UL)
	{
		DWARN("rounded len error: %ld", rounded_len);
		buf_deref (nbuf, BUF_NORMAL);
		return -2;
	}

	/* todo: enqueue packet and dequeue + send on service interrupt instead of blocking */
	if(V4_ETH_TXQUEUE)
	{
		volatile u_int32_t d2 = 0;
		volatile u_int32_t d1 = (0xfffffffdUL - (rounded_len >> 2)) + 512;
		for(d2 = 800000UL; d2 > 0; d2--)
		{
			//DDBG("tx q : %lx", *txqtr);
			if(V4_ETH_TXQUEUE <= d1)
				goto freebuffer;

			for (i=0;i<50;i++)
			{
				asm("nop;");
				asm("nop;");
				asm("nop;");
				asm("nop;");
				asm("nop;");
			}
		}
		DWARN("tx full");
		buf_deref (nbuf, BUF_NORMAL);
		return ETIMEDOUT;
	}
freebuffer:

	if (rounded_len<64)
	{
		stufflen = 64 - rounded_len;
		DDBG("stuff len: %ld",stufflen);
		V4_ETH_TXFIFO = 64;
	}
	else
	{
		stufflen = 0;
		V4_ETH_TXFIFO = rounded_len;
	}

	rounded_len >>= 2;
	p = (u_int32_t*)nbuf->dstart;
	while (rounded_len >= 16)
	{
		V4_ETH_TXFIFO = *p++; V4_ETH_TXFIFO = *p++; V4_ETH_TXFIFO = *p++; V4_ETH_TXFIFO = *p++;
		V4_ETH_TXFIFO = *p++; V4_ETH_TXFIFO = *p++; V4_ETH_TXFIFO = *p++; V4_ETH_TXFIFO = *p++;
		V4_ETH_TXFIFO = *p++; V4_ETH_TXFIFO = *p++; V4_ETH_TXFIFO = *p++; V4_ETH_TXFIFO = *p++;
		V4_ETH_TXFIFO = *p++; V4_ETH_TXFIFO = *p++; V4_ETH_TXFIFO = *p++; V4_ETH_TXFIFO = *p++;
		rounded_len -= 16;
	}
	while (rounded_len)
	{
		V4_ETH_TXFIFO = *p++;
		rounded_len--;
	}

	stufflen >>= 2;
	while (stufflen)
	{
		V4_ETH_TXFIFO = 0;
		stufflen--;		
	}

	buf_deref (nbuf, BUF_NORMAL);
	return (0);
}

static long
v4net_ioctl (struct netif *nif, short cmd, long arg)
{
	struct ifreq *ifr;
	struct sockaddr_hw *shw;

	switch (cmd)
	{
		case SIOCSIFNETMASK:
		case SIOCSIFFLAGS:
		case SIOCSIFADDR:
			return 0;
		case SIOCSIFHWADDR:
			ifr = (struct ifreq*) arg;
			shw = &ifr->ifru.adr.hw;
			memcpy(hwtmp, shw->shw_adr.bytes, ETH_ALEN);
			V4_ETH_MAC1 = (upcfg | (hwtmp[1]<<8) | hwtmp[0]);
			V4_ETH_MAC2 = (((u_int32_t)hwtmp[2]) << 0) | (((u_int32_t)hwtmp[3]) << 8) | (((u_int32_t)hwtmp[4]) << 16) | (((u_int32_t)hwtmp[5]) << 24);
			memcpy(if_v4net.hwlocal.adr.bytes, hwtmp, ETH_ALEN);
			return 0;
		case SIOCSIFMTU:
			/* Limit MTU to 1500 bytes. MintNet has alraedy set nif->mtu to the new value, we only limit it here. */
			if (nif->mtu > ETH_MAX_DLEN)
				nif->mtu = ETH_MAX_DLEN;
			return 0;
		case SIOCSIFOPT:
			/* Interface configuration, handled by v4net_config() */
			ifr = (struct ifreq *) arg;
			return v4net_config (nif, ifr->ifru.data);
	}

	return ENOSYS;
}

static long
v4net_config (struct netif *nif, struct ifopt *ifo)
{
# define STRNCMP(s)	(strncmp ((s), ifo->option, sizeof (ifo->option)))
	
	if (!STRNCMP ("hwaddr"))
	{
		/* Set hardware address */
		uchar *cp;
		if (ifo->valtype != IFO_HWADDR)
			return ENOENT;
		memcpy (nif->hwlocal.adr.bytes, ifo->ifou.v_string, ETH_ALEN);
		cp = nif->hwlocal.adr.bytes;
		UNUSED (cp);
		DDBG("v4net: hwaddr is %x:%x:%x:%x:%x:%x", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
	}
	else if (!STRNCMP ("braddr"))
	{
		/* Set broadcast address */
		uchar *cp;
		if (ifo->valtype != IFO_HWADDR)
			return ENOENT;
		memcpy (nif->hwbrcst.adr.bytes, ifo->ifou.v_string, ETH_ALEN);
		cp = nif->hwbrcst.adr.bytes;
		UNUSED (cp);
		DDBG("v4net: braddr is %x:%x:%x:%x:%x:%x", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
	}
	else if (!STRNCMP ("debug"))
	{
		/* turn debuggin on/off */
		if (ifo->valtype != IFO_INT)
			return ENOENT;
		DDBG("v4net: debug level is %ld", ifo->ifou.v_long);
	}
	else if (!STRNCMP ("log"))
	{
		/* set log file */
		if (ifo->valtype != IFO_STRING)
			return ENOENT;
		DDBG("v4net: log file is %s", ifo->ifou.v_string);
	}
	
	return ENOSYS;
}

long driver_init(void)
{
	/* default mac: 06:80:11:04:04 */
	/* can be changed using ifconfig hwaddr xx:xx:xx:xx:xx:xx when bringing it up */
	hwtmp[0]=0x06;
	hwtmp[1]=0x80;
	hwtmp[2]=0x11;
	hwtmp[3]=0x04;
	hwtmp[4]=0x04;
	hwtmp[5]=0x04;

	/* hardware setup */
	alignmem=TRAP_Mxalloc(2048+(MEMSIZE),1);
	if (!alignmem)
		return(0);
	memset((void*)alignmem,0,2048+MEMSIZE);
	//alignmem = ((alignmem+2047) & (ulong)0xfffff800);
	V4_ETH_MULTI1 = 0;
	V4_ETH_MULTI2 = 0;
	V4_ETH_DMA_START = alignmem;
	V4_ETH_DMA_END = alignmem + MEMSIZE;
	v4net_swposcopy=((alignmem+MEMSIZE)-2048);
	V4_ETH_HW_WRITE = v4net_swposcopy;
	V4_ETH_SW_WRITE = v4net_swposcopy;
	V4_ETH_MAC1 = (((u_int32_t)hwtmp[0]) << 0) | (((u_int32_t)hwtmp[1]) << 8);
	V4_ETH_MAC2 = (((u_int32_t)hwtmp[2]) << 0) | (((u_int32_t)hwtmp[3]) << 8) | (((u_int32_t)hwtmp[4]) << 16) | (((u_int32_t)hwtmp[5]) << 24);

	/* interface setup */
	strcpy (if_v4net.name, "en");
	if_v4net.unit = if_getfreeunit ("en");
	if_v4net.metric = 0;
	if_v4net.flags = IFF_BROADCAST;
	if_v4net.mtu = 1500;
	if_v4net.timer = 0;
	if_v4net.hwtype = HWTYPE_ETH;
	if_v4net.hwlocal.len = ETH_ALEN;
	if_v4net.hwbrcst.len = ETH_ALEN;
	if_v4net.rcv.maxqlen = IF_MAXQ;
	if_v4net.snd.maxqlen = IF_MAXQ;

	/* Setup pointers to service functions */
	if_v4net.open = v4net_open;
	if_v4net.close = v4net_close;
	if_v4net.output = v4net_output;
	if_v4net.ioctl = v4net_ioctl;
	/* Optional timer function that is called every 200ms. */
	if_v4net.timeout = 0;
	/* Here you could attach some more data your driver may need */
	if_v4net.data = 0;
	/* Number of packets the hardware can receive in fast succession, 0 means unlimited. */
	if_v4net.maxpackets = 0;

	memcpy (if_v4net.hwlocal.adr.bytes, hwtmp, ETH_ALEN);
	//memcpy (if_v4net.hwbrcst.adr.bytes, hwbtmp, ETH_ALEN);
	memcpy (if_v4net.hwbrcst.adr.bytes, "\377\377\377\377\377\377", ETH_ALEN);
	
	if_register (&if_v4net);
	
#if V4NET_200HZ
	/* hook into 200hz interrupt */
	u_int32_t* vec = (u_int32_t*)0x114UL;
	s_system (S_GETCOOKIE, COOKIE__5MS, (long) &vec);
	v4net_int_old = (void*) *vec;
	*vec = (u_int32_t) v4net_int;
#else
	/* install ethernet interrupt handler */
	u_int32_t vbr = 0;
	__asm__ volatile ( \
		"	dc.w  0x4e7a,0x0801\n" /* movec VBR,d0 */ \
		"	move.l %%d0,%0\n" \
		: "=d"(vbr) : : "d0", "cc"
	);
	u_int32_t vec = vbr + 0x6C;
	v4net_int_old = Setexc (vec>>2, (long) v4net_int);
#endif
	DDBG("int installed: %08x -> %08x (old=%08x)", (u_int32_t)vec, v4net_int, v4net_int_old);

	/* done */
	DPRINT("v4net Eth driver v0.1 (en%d)", if_v4net.unit);
	return 0;
}
