/*
 * SVEthlana driver for FreeMiNT: the OpenCores 10/100 Mbps Ethernet MAC
 * in the SuperVidel FPGA.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * This is a port of the Linux ethoc driver, drivers/net/ethernet/ethoc.c
 * (Linux 7.3). The parts of the Linux PHY library that ethoc relies on
 * when the generic PHY driver binds to the Micrel KSZ8041 on the
 * SuperVidel are in phy.c. Function, variable and register names are kept as in Linux
 * so that fixes can be carried over; the MiNTNet interface (svethlana_*)
 * at the end of this file takes the place of the Linux net_device, NAPI
 * and platform glue. What is not ported: ethtool, ring size changes, DT/OF
 * probing, clock handling and power management.
 *
 * The kernel is built with -mshort, so Linux "int" is "long" here.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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

// SPDX-License-Identifier: GPL-2.0-only
/*
 * linux/drivers/net/ethernet/ethoc.c
 *
 * Copyright (C) 2007-2008 Avionic Design Development GmbH
 * Copyright (C) 2008-2009 Avionic Design GmbH
 *
 * Written by Thierry Reding <thierry.reding@avionic-design.de>
 */

# include "global.h"

# include "buf.h"
# include "cookie.h"
# include "inet4/if.h"
# include "inet4/ifeth.h"
# include "inet4/igmp.h"
# include "netinfo.h"

# include "mint/delay.h"
# include "mint/fcntl.h"
# include "mint/sockio.h"
# include "mint/time.h"
# include "mint/arch/asm_spl.h"

# include <mint/osbind.h>

# include "sv_regs.h"
# include "svethlana_i6.h"
# include "phy.h"


# define IRQ_NONE	0L
# define IRQ_HANDLED	1L

# define likely(x)	__builtin_expect(!!(x), 1)
# define unlikely(x)	__builtin_expect(!!(x), 0)

/* default NAPI budget, net/core/dev.c */
# define NAPI_POLL_WEIGHT	64


/* register offsets */
#define	MODER		0x00
#define	INT_SOURCE	0x04
#define	INT_MASK	0x08
#define	IPGT		0x0c
#define	IPGR1		0x10
#define	IPGR2		0x14
#define	PACKETLEN	0x18
#define	COLLCONF	0x1c
#define	TX_BD_NUM	0x20
#define	CTRLMODER	0x24
#define	MIIMODER	0x28
#define	MIICOMMAND	0x2c
#define	MIIADDRESS	0x30
#define	MIITX_DATA	0x34
#define	MIIRX_DATA	0x38
#define	MIISTATUS	0x3c
#define	MAC_ADDR0	0x40
#define	MAC_ADDR1	0x44
#define	ETH_HASH0	0x48
#define	ETH_HASH1	0x4c
#define	ETH_TXCTRL	0x50
#define	ETH_END		0x54

/* mode register */
#define	MODER_RXEN	(1UL <<  0) /* receive enable */
#define	MODER_TXEN	(1UL <<  1) /* transmit enable */
#define	MODER_NOPRE	(1UL <<  2) /* no preamble */
#define	MODER_BRO	(1UL <<  3) /* broadcast address */
#define	MODER_IAM	(1UL <<  4) /* individual address mode */
#define	MODER_PRO	(1UL <<  5) /* promiscuous mode */
#define	MODER_IFG	(1UL <<  6) /* interframe gap for incoming frames */
#define	MODER_LOOP	(1UL <<  7) /* loopback */
#define	MODER_NBO	(1UL <<  8) /* no back-off */
#define	MODER_EDE	(1UL <<  9) /* excess defer enable */
#define	MODER_FULLD	(1UL << 10) /* full duplex */
#define	MODER_RESET	(1UL << 11) /* FIXME: reset (undocumented) */
#define	MODER_DCRC	(1UL << 12) /* delayed CRC enable */
#define	MODER_CRC	(1UL << 13) /* CRC enable */
#define	MODER_HUGE	(1UL << 14) /* huge packets enable */
#define	MODER_PAD	(1UL << 15) /* padding enabled */
#define	MODER_RSM	(1UL << 16) /* receive small packets */

/* interrupt source and mask registers */
#define	INT_MASK_TXF	(1UL << 0) /* transmit frame */
#define	INT_MASK_TXE	(1UL << 1) /* transmit error */
#define	INT_MASK_RXF	(1UL << 2) /* receive frame */
#define	INT_MASK_RXE	(1UL << 3) /* receive error */
#define	INT_MASK_BUSY	(1UL << 4)
#define	INT_MASK_TXC	(1UL << 5) /* transmit control frame */
#define	INT_MASK_RXC	(1UL << 6) /* receive control frame */

#define	INT_MASK_TX	(INT_MASK_TXF | INT_MASK_TXE)
#define	INT_MASK_RX	(INT_MASK_RXF | INT_MASK_RXE)

#define	INT_MASK_ALL ( \
		INT_MASK_TXF | INT_MASK_TXE | \
		INT_MASK_RXF | INT_MASK_RXE | \
		INT_MASK_TXC | INT_MASK_RXC | \
		INT_MASK_BUSY \
	)

/* packet length register */
#define	PACKETLEN_MIN(min)		(((min) & 0xffffUL) << 16)
#define	PACKETLEN_MAX(max)		(((max) & 0xffffUL) <<  0)
#define	PACKETLEN_MIN_MAX(min, max)	(PACKETLEN_MIN(min) | \
					PACKETLEN_MAX(max))

/* transmit buffer number register */
#define	TX_BD_NUM_VAL(x)	(((x) <= 0x80) ? (x) : 0x80)

/* control module mode register */
#define	CTRLMODER_PASSALL	(1UL << 0) /* pass all receive frames */
#define	CTRLMODER_RXFLOW	(1UL << 1) /* receive control flow */
#define	CTRLMODER_TXFLOW	(1UL << 2) /* transmit control flow */

/* MII mode register */
#define	MIIMODER_CLKDIV(x)	((x) & 0xfe) /* needs to be an even number */
#define	MIIMODER_NOPRE		(1UL << 8) /* no preamble */

/* MII command register */
#define	MIICOMMAND_SCAN		(1UL << 0) /* scan status */
#define	MIICOMMAND_READ		(1UL << 1) /* read status */
#define	MIICOMMAND_WRITE	(1UL << 2) /* write control data */

/* MII address register */
#define	MIIADDRESS_FIAD(x)		(((x) & 0x1fUL) << 0)
#define	MIIADDRESS_RGAD(x)		(((x) & 0x1fUL) << 8)
#define	MIIADDRESS_ADDR(phy, reg)	(MIIADDRESS_FIAD(phy) | \
					MIIADDRESS_RGAD(reg))

/* MII transmit data register */
#define	MIITX_DATA_VAL(x)	((x) & 0xffffUL)

/* MII receive data register */
#define	MIIRX_DATA_VAL(x)	((x) & 0xffffUL)

/* MII status register */
#define	MIISTATUS_LINKFAIL	(1UL << 0)
#define	MIISTATUS_BUSY		(1UL << 1)
#define	MIISTATUS_INVALID	(1UL << 2)

/* TX buffer descriptor */
#define	TX_BD_CS		(1UL <<  0) /* carrier sense lost */
#define	TX_BD_DF		(1UL <<  1) /* defer indication */
#define	TX_BD_LC		(1UL <<  2) /* late collision */
#define	TX_BD_RL		(1UL <<  3) /* retransmission limit */
#define	TX_BD_RETRY_MASK	(0x00f0UL)
#define	TX_BD_RETRY(x)		(((x) & 0x00f0UL) >>  4)
#define	TX_BD_UR		(1UL <<  8) /* transmitter underrun */
#define	TX_BD_CRC		(1UL << 11) /* TX CRC enable */
#define	TX_BD_PAD		(1UL << 12) /* pad enable for short packets */
#define	TX_BD_WRAP		(1UL << 13)
#define	TX_BD_IRQ		(1UL << 14) /* interrupt request enable */
#define	TX_BD_READY		(1UL << 15) /* TX buffer ready */
#define	TX_BD_LEN(x)		(((x) & 0xffffUL) << 16)
#define	TX_BD_LEN_MASK		(0xffffUL << 16)

#define	TX_BD_STATS		(TX_BD_CS | TX_BD_DF | TX_BD_LC | \
				TX_BD_RL | TX_BD_RETRY_MASK | TX_BD_UR)

/* RX buffer descriptor */
#define	RX_BD_LC	(1UL <<  0) /* late collision */
#define	RX_BD_CRC	(1UL <<  1) /* RX CRC error */
#define	RX_BD_SF	(1UL <<  2) /* short frame */
#define	RX_BD_TL	(1UL <<  3) /* too long */
#define	RX_BD_DN	(1UL <<  4) /* dribble nibble */
#define	RX_BD_IS	(1UL <<  5) /* invalid symbol */
#define	RX_BD_OR	(1UL <<  6) /* receiver overrun */
#define	RX_BD_MISS	(1UL <<  7)
#define	RX_BD_CF	(1UL <<  8) /* control frame */
#define	RX_BD_WRAP	(1UL << 13)
#define	RX_BD_IRQ	(1UL << 14) /* interrupt request enable */
#define	RX_BD_EMPTY	(1UL << 15)
#define	RX_BD_LEN(x)	(((x) & 0xffffUL) << 16)

#define	RX_BD_STATS	(RX_BD_LC | RX_BD_CRC | RX_BD_SF | RX_BD_TL | \
			RX_BD_DN | RX_BD_IS | RX_BD_OR | RX_BD_MISS)

#define	ETHOC_BUFSIZ		1536
#define	ETHOC_ZLEN		64
#define	ETHOC_BD_BASE		0x400
#define	ETHOC_TIMEOUT		(HZ / 2)
#define	ETHOC_MII_TIMEOUT	(1 + (HZ / 5))


/**
 * struct ethoc - driver-private device structure
 * @iobase:	pointer to I/O memory region
 * @membase:	pointer to buffer memory region
 * @num_bd:	number of buffer descriptors
 * @num_tx:	number of send buffers
 * @cur_tx:	last send buffer written
 * @dty_tx:	last buffer actually sent
 * @num_rx:	number of receive buffers
 * @cur_rx:	current receive buffer
 * @vma:        pointer to array of virtual memory addresses for buffers
 * @netdev:	pointer to network device structure
 * @napi:	the pending poll, a root timeout here
 * @napi_scheduled: a poll is due
 * @napi_enabled: polls may be scheduled
 * @queue_stopped: transmit ring full, packets wait in netdev->snd
 * @watchdog:	ticks the queue has been stopped, for ethoc_tx_timeout()
 * @mc_refcnt:	users of each multicast hash bit, the mc list of Linux
 * @mdio:	MDIO bus for PHY access
 * @phydev:	the attached PHY, dev->phydev on Linux
 * @phy_id:	address of attached PHY
 * @old_link:	previous link info
 * @old_duplex: previous duplex info
 */
struct ethoc {
	char *iobase;
	char *membase;

	u32 num_bd;
	u32 num_tx;
	u32 cur_tx;
	u32 dty_tx;

	u32 num_rx;
	u32 cur_rx;

	void **vma;

	struct netif *netdev;

	TIMEOUT *napi;
	short napi_scheduled;
	short napi_enabled;

	short queue_stopped;
	short watchdog;

	u8 mc_refcnt[64];

	struct mii_bus *mdio;
	struct phy_device *phydev;
	s8 phy_id;

	long old_link;
	long old_duplex;
};

/**
 * struct ethoc_bd - buffer descriptor
 * @stat:	buffer statistics
 * @addr:	physical memory address
 */
struct ethoc_bd {
	u32 stat;
	u32 addr;
};


static struct netif if_svethlana;
static struct ethoc ethoc_priv;
static struct mii_bus ethoc_mdio;
static void *ethoc_vma[128];

static long ethoc_mdio_read (struct mii_bus *bus, long phy, long reg);
static long ethoc_mdio_write (struct mii_bus *bus, long phy, long reg, u16 val);
static void ethoc_mdio_poll (struct netif *dev);
static void ethoc_set_multicast_list (struct netif *dev);
static long ethoc_start_xmit (BUF *skb, struct netif *dev);
static void _cdecl ethoc_poll (PROC *p, long arg);


/*
 * Register and buffer access. The SuperVidel is big endian
 * (ethoc_platform_data.big_endian), so ethoc_read() is ioread32be().
 */

static inline u32 ethoc_read(struct ethoc *dev, long offset)
{
	return *(volatile u32 *)(dev->iobase + offset);
}

static inline void ethoc_write(struct ethoc *dev, long offset, u32 data)
{
	*(volatile u32 *)(dev->iobase + offset) = data;
}

/*
 * memcpy_toio() and memcpy_fromio() are memcpy() on Linux/m68k, and
 * libkern's memcpy moves longwords like the Linux one.
 */
#define memcpy_toio(dst, src, count)	memcpy((dst), (src), (count))
#define memcpy_fromio(dst, src, count)	memcpy((dst), (src), (count))
#define memset_io(dst, val, count)	memset((dst), (val), (count))

static inline void ethoc_read_bd(struct ethoc *dev, long index,
		struct ethoc_bd *bd)
{
	long offset = ETHOC_BD_BASE + (index * sizeof(struct ethoc_bd));
	bd->stat = ethoc_read(dev, offset + 0);
	bd->addr = ethoc_read(dev, offset + 4);
}

static inline void ethoc_write_bd(struct ethoc *dev, long index,
		const struct ethoc_bd *bd)
{
	long offset = ETHOC_BD_BASE + (index * sizeof(struct ethoc_bd));
	ethoc_write(dev, offset + 0, bd->stat);
	ethoc_write(dev, offset + 4, bd->addr);
}

static inline void ethoc_enable_irq(struct ethoc *dev, u32 mask)
{
	u32 imask = ethoc_read(dev, INT_MASK);
	imask |= mask;
	ethoc_write(dev, INT_MASK, imask);
}

static inline void ethoc_disable_irq(struct ethoc *dev, u32 mask)
{
	u32 imask = ethoc_read(dev, INT_MASK);
	imask &= ~mask;
	ethoc_write(dev, INT_MASK, imask);
}

static inline void ethoc_ack_irq(struct ethoc *dev, u32 mask)
{
	ethoc_write(dev, INT_SOURCE, mask);
}

static inline void ethoc_enable_rx_and_tx(struct ethoc *dev)
{
	u32 mode = ethoc_read(dev, MODER);
	mode |= MODER_RXEN | MODER_TXEN;
	ethoc_write(dev, MODER, mode);
}

static inline void ethoc_disable_rx_and_tx(struct ethoc *dev)
{
	u32 mode = ethoc_read(dev, MODER);
	mode &= ~(MODER_RXEN | MODER_TXEN);
	ethoc_write(dev, MODER, mode);
}


/*
 * NAPI. On Linux the interrupt handler masks the receive and transmit
 * interrupts and hands the work to ethoc_poll() in softirq context. Here
 * the work goes to a root timeout, which the kernel runs from the
 * scheduler with interrupts enabled, and MiNTNet delivers received
 * packets (if_input) through the very same mechanism, so nothing is
 * added to the delivery latency. Only the interrupt entry runs at IPL 6.
 */

static void napi_enable(struct ethoc *priv)
{
	priv->napi = NULL;
	priv->napi_scheduled = 0;
	priv->napi_enabled = 1;
}

static void napi_disable(struct ethoc *priv)
{
	ushort sr = spl7();

	priv->napi_enabled = 0;
	if (priv->napi)
		cancelroottimeout(priv->napi);
	priv->napi = NULL;
	priv->napi_scheduled = 0;

	spl(sr);
}

/* schedules ethoc_poll(), from the interrupt (flags 1) or not */
static void __napi_schedule(struct ethoc *priv, ushort flags)
{
	priv->napi = addroottimeout(0, ethoc_poll, flags);
	if (priv->napi)
		priv->napi->arg = (long) priv;
	/* else the slow timer retries, see svethlana_timeout() */
}

static void napi_schedule(struct ethoc *priv)
{
	if (!priv->napi_enabled || priv->napi_scheduled)
		return;

	priv->napi_scheduled = 1;
	__napi_schedule(priv, 1);
}

static void napi_complete_done(struct ethoc *priv)
{
	priv->napi_scheduled = 0;
}


/*
 * netif_{start,stop,wake}_queue(). Linux stops calling
 * ethoc_start_xmit() while the queue is stopped and its qdisc keeps the
 * packets. Here svethlana_output() puts them into dev->snd and
 * netif_wake_queue() pushes them into the ring again.
 */

static void netif_start_queue(struct ethoc *priv)
{
	priv->queue_stopped = 0;
	priv->watchdog = 0;
}

static void netif_stop_queue(struct ethoc *priv)
{
	priv->queue_stopped = 1;
}

static void netif_wake_queue(struct netif *dev)
{
	struct ethoc *priv = dev->data;
	BUF *skb;

	netif_start_queue(priv);

	while (!priv->queue_stopped && (skb = if_dequeue(&dev->snd)) != NULL)
		ethoc_start_xmit(skb, dev);
}


static long ethoc_init_ring(struct ethoc *dev, unsigned long mem_start)
{
	struct ethoc_bd bd;
	long i;
	char *vma;

	dev->cur_tx = 0;
	dev->dty_tx = 0;
	dev->cur_rx = 0;

	ethoc_write(dev, TX_BD_NUM, dev->num_tx);

	/* setup transmission buffers */
	bd.addr = mem_start;
	bd.stat = TX_BD_IRQ | TX_BD_CRC;
	vma = dev->membase;

	for (i = 0; i < dev->num_tx; i++) {
		if (i == dev->num_tx - 1)
			bd.stat |= TX_BD_WRAP;

		ethoc_write_bd(dev, i, &bd);
		bd.addr += ETHOC_BUFSIZ;

		dev->vma[i] = vma;
		vma += ETHOC_BUFSIZ;
	}

	bd.stat = RX_BD_EMPTY | RX_BD_IRQ;

	for (i = 0; i < dev->num_rx; i++) {
		if (i == dev->num_rx - 1)
			bd.stat |= RX_BD_WRAP;

		ethoc_write_bd(dev, dev->num_tx + i, &bd);
		bd.addr += ETHOC_BUFSIZ;

		dev->vma[dev->num_tx + i] = vma;
		vma += ETHOC_BUFSIZ;
	}

	return 0;
}

static long ethoc_reset(struct ethoc *dev)
{
	u32 mode;

	/* TODO: reset controller? */

	ethoc_disable_rx_and_tx(dev);

	/* TODO: setup registers */

	/* enable FCS generation and automatic padding */
	mode = ethoc_read(dev, MODER);
	mode |= MODER_CRC | MODER_PAD;
	ethoc_write(dev, MODER, mode);

	/* set full-duplex mode */
	mode = ethoc_read(dev, MODER);
	mode |= MODER_FULLD;
	ethoc_write(dev, MODER, mode);
	ethoc_write(dev, IPGT, 0x15);

	ethoc_ack_irq(dev, INT_MASK_ALL);
	ethoc_enable_irq(dev, INT_MASK_ALL);
	ethoc_enable_rx_and_tx(dev);
	return 0;
}

/*
 * Linux keeps a counter per error type and prints each one; MiNTNet has
 * in_errors and collisions.
 */
static long ethoc_update_rx_stats(struct ethoc *dev,
		struct ethoc_bd *bd)
{
	struct netif *netdev = dev->netdev;
	long ret = 0;

	if (bd->stat & RX_BD_TL) {
		DEBUG(("RX: frame too long"));
		netdev->in_errors++;
		ret++;
	}

	if (bd->stat & RX_BD_SF) {
		DEBUG(("RX: frame too short"));
		netdev->in_errors++;
		ret++;
	}

	if (bd->stat & RX_BD_DN) {
		DEBUG(("RX: dribble nibble"));
		netdev->in_errors++;
	}

	if (bd->stat & RX_BD_CRC) {
		DEBUG(("RX: wrong CRC"));
		netdev->in_errors++;
		ret++;
	}

	if (bd->stat & RX_BD_OR) {
		DEBUG(("RX: overrun"));
		netdev->in_errors++;
		ret++;
	}

	if (bd->stat & RX_BD_MISS)
		netdev->in_errors++;

	if (bd->stat & RX_BD_LC) {
		DEBUG(("RX: late collision"));
		netdev->collisions++;
		ret++;
	}

	return ret;
}

static long ethoc_rx(struct netif *dev, long limit)
{
	struct ethoc *priv = dev->data;
	long count;

	for (count = 0; count < limit; ++count) {
		long entry;
		struct ethoc_bd bd;

		entry = priv->num_tx + priv->cur_rx;
		ethoc_read_bd(priv, entry, &bd);
		if (bd.stat & RX_BD_EMPTY) {
			ethoc_ack_irq(priv, INT_MASK_RX);
			/* If packet (interrupt) came in between checking
			 * BD_EMTPY and clearing the interrupt source, then we
			 * risk missing the packet as the RX interrupt won't
			 * trigger right away when we reenable it; hence, check
			 * BD_EMTPY here again to make sure there isn't such a
			 * packet waiting for us...
			 */
			ethoc_read_bd(priv, entry, &bd);
			if (bd.stat & RX_BD_EMPTY)
				break;
		}

		if (ethoc_update_rx_stats(priv, &bd) == 0) {
			long size = bd.stat >> 16;
			BUF *skb;

			size -= 4; /* strip the CRC */
			/*
			 * netdev_alloc_skb_ip_align(); the frame is copied
			 * to a longword aligned start
			 */
			skb = buf_alloc(size + 4 + 16, 16, BUF_NORMAL);

			if (likely(skb)) {
				void *src = priv->vma[entry];
				short type;

				skb->dstart = (char *)(((ulong) skb->dstart + 3) & ~3UL);
				skb->dend = skb->dstart + size;
				memcpy_fromio(skb->dstart, src, size);

				/* eth_type_trans(): the packet filter sees the
				 * frame before the header goes
				 */
				if (dev->bpf)
					bpf_input(dev, skb);
				type = eth_remove_hdr(skb);

				/* netif_receive_skb(); if_input() frees the
				 * buffer when its queue is full
				 */
				if (if_input(dev, skb, 0, type))
					dev->in_errors++;
				else
					dev->in_packets++;
			} else {
				DEBUG(("low on memory - packet dropped"));

				dev->in_errors++;
				break;
			}
		}

		/* clear the buffer descriptor so it can be reused */
		bd.stat &= ~RX_BD_STATS;
		bd.stat |=  RX_BD_EMPTY;
		ethoc_write_bd(priv, entry, &bd);
		if (++priv->cur_rx == priv->num_rx)
			priv->cur_rx = 0;
	}

	return count;
}

static void ethoc_update_tx_stats(struct ethoc *dev, struct ethoc_bd *bd)
{
	struct netif *netdev = dev->netdev;

	if (bd->stat & TX_BD_LC) {
		DEBUG(("TX: late collision"));
	}

	if (bd->stat & TX_BD_RL) {
		DEBUG(("TX: retransmit limit"));
	}

	if (bd->stat & TX_BD_UR) {
		DEBUG(("TX: underrun"));
	}

	if (bd->stat & TX_BD_CS) {
		DEBUG(("TX: carrier sense lost"));
	}

	if (bd->stat & TX_BD_STATS)
		netdev->out_errors++;

	netdev->collisions += (bd->stat >> 4) & 0xf;
	netdev->out_packets++;
}

static long ethoc_tx(struct netif *dev, long limit)
{
	struct ethoc *priv = dev->data;
	long count;
	struct ethoc_bd bd;

	for (count = 0; count < limit; ++count) {
		long entry;

		entry = priv->dty_tx & (priv->num_tx-1);

		ethoc_read_bd(priv, entry, &bd);

		if (bd.stat & TX_BD_READY || (priv->dty_tx == priv->cur_tx)) {
			ethoc_ack_irq(priv, INT_MASK_TX);
			/* If interrupt came in between reading in the BD
			 * and clearing the interrupt source, then we risk
			 * missing the event as the TX interrupt won't trigger
			 * right away when we reenable it; hence, check
			 * BD_EMPTY here again to make sure there isn't such an
			 * event pending...
			 */
			ethoc_read_bd(priv, entry, &bd);
			if (bd.stat & TX_BD_READY ||
			    (priv->dty_tx == priv->cur_tx))
				break;
		}

		ethoc_update_tx_stats(priv, &bd);
		priv->dty_tx++;
	}

	if ((priv->cur_tx - priv->dty_tx) <= (priv->num_tx / 2))
		netif_wake_queue(dev);

	return count;
}

/* called from svethlana_interrupt at IPL 6, or from ethoc_tx_timeout() */
long _cdecl ethoc_interrupt(void)
{
	struct ethoc *priv = &ethoc_priv;
	struct netif *dev = priv->netdev;
	u32 pending;
	u32 mask;

	/* Figure out what triggered the interrupt...
	 * The tricky bit here is that the interrupt source bits get
	 * set in INT_SOURCE for an event regardless of whether that
	 * event is masked or not.  Thus, in order to figure out what
	 * triggered the interrupt, we need to remove the sources
	 * for all events that are currently masked.  This behaviour
	 * is not particularly well documented but reasonable...
	 */
	mask = ethoc_read(priv, INT_MASK);
	pending = ethoc_read(priv, INT_SOURCE);
	pending &= mask;

	if (unlikely(pending == 0))
		return IRQ_NONE;

	ethoc_ack_irq(priv, pending);

	/* We always handle the dropped packet interrupt */
	if (pending & INT_MASK_BUSY) {
		/* rx_dropped */
		dev->in_errors++;
	}

	/* Handle receive/transmit event by switching to polling */
	if (pending & (INT_MASK_TX | INT_MASK_RX)) {
		ethoc_disable_irq(priv, INT_MASK_TX | INT_MASK_RX);
		napi_schedule(priv);
	}

	return IRQ_HANDLED;
}

static long ethoc_get_mac_address(struct netif *dev, void *addr)
{
	struct ethoc *priv = dev->data;
	u8 *mac = (u8 *)addr;
	u32 reg;

	reg = ethoc_read(priv, MAC_ADDR0);
	mac[2] = (reg >> 24) & 0xff;
	mac[3] = (reg >> 16) & 0xff;
	mac[4] = (reg >>  8) & 0xff;
	mac[5] = (reg >>  0) & 0xff;

	reg = ethoc_read(priv, MAC_ADDR1);
	mac[0] = (reg >>  8) & 0xff;
	mac[1] = (reg >>  0) & 0xff;

	return 0;
}

static void _cdecl ethoc_poll(PROC *p, long arg)
{
	struct ethoc *priv = (struct ethoc *) arg;
	long budget = NAPI_POLL_WEIGHT;
	long rx_work_done = 0;
	long tx_work_done = 0;

	UNUSED(p);

	priv->napi = NULL;

	rx_work_done = ethoc_rx(priv->netdev, budget);
	tx_work_done = ethoc_tx(priv->netdev, budget);

	if (rx_work_done < budget && tx_work_done < budget) {
		napi_complete_done(priv);
		ethoc_enable_irq(priv, INT_MASK_TX | INT_MASK_RX);
	} else {
		/* budget used up, poll again before reenabling interrupts */
		__napi_schedule(priv, 0);
	}
}

static long ethoc_mdio_read(struct mii_bus *bus, long phy, long reg)
{
	struct ethoc *priv = bus->priv;
	long i;

	ethoc_write(priv, MIIADDRESS, MIIADDRESS_ADDR(phy, reg));
	ethoc_write(priv, MIICOMMAND, MIICOMMAND_READ);

	for (i = 0; i < 5; i++) {
		u32 status = ethoc_read(priv, MIISTATUS);
		if (!(status & MIISTATUS_BUSY)) {
			u32 data = ethoc_read(priv, MIIRX_DATA);
			/* reset MII command register */
			ethoc_write(priv, MIICOMMAND, 0);
			return data;
		}
		udelay(100);
	}

	return EBUSY;
}

static long ethoc_mdio_write(struct mii_bus *bus, long phy, long reg, u16 val)
{
	struct ethoc *priv = bus->priv;
	long i;

	ethoc_write(priv, MIIADDRESS, MIIADDRESS_ADDR(phy, reg));
	ethoc_write(priv, MIITX_DATA, val);
	ethoc_write(priv, MIICOMMAND, MIICOMMAND_WRITE);

	for (i = 0; i < 5; i++) {
		u32 stat = ethoc_read(priv, MIISTATUS);
		if (!(stat & MIISTATUS_BUSY)) {
			/* reset MII command register */
			ethoc_write(priv, MIICOMMAND, 0);
			return 0;
		}
		udelay(100);
	}

	return EBUSY;
}


static void ethoc_mdio_poll(struct netif *dev)
{
	struct ethoc *priv = dev->data;
	struct phy_device *phydev = priv->phydev;
	long changed = 0;
	u32 mode;

	if (priv->old_link != phydev->link) {
		changed = 1;
		priv->old_link = phydev->link;
	}

	if (priv->old_duplex != phydev->duplex) {
		changed = 1;
		priv->old_duplex = phydev->duplex;
	}

	if (!changed)
		return;

	mode = ethoc_read(priv, MODER);
	if (phydev->duplex == DUPLEX_FULL)
		mode |= MODER_FULLD;
	else
		mode &= ~MODER_FULLD;
	ethoc_write(priv, MODER, mode);

	phy_print_status(phydev);
}

static long ethoc_mdio_probe(struct netif *dev)
{
	struct ethoc *priv = dev->data;
	struct phy_device *phy;
	long err;

	if (priv->phy_id != -1)
		phy = mdiobus_get_phy(priv->mdio, priv->phy_id);
	else
		phy = phy_find_first(priv->mdio);

	if (!phy) {
		ALERT(("svethlana: no PHY found"));
		return ENXIO;
	}

	priv->old_duplex = -1;
	priv->old_link = -1;

	err = phy_connect_direct(dev, phy, ethoc_mdio_poll);
	if (err) {
		ALERT(("svethlana: could not attach to PHY"));
		return err;
	}
	/* phy_attach_direct() sets dev->phydev */
	priv->phydev = phy;

	phy_set_max_speed(phy, SPEED_100);

	return 0;
}

static long ethoc_open(struct netif *dev)
{
	struct ethoc *priv = dev->data;

	/* request_irq(): the vector is installed once, in driver_init() */

	napi_enable(priv);

	ethoc_init_ring(priv, (unsigned long) priv->membase);
	ethoc_reset(priv);

	if (priv->queue_stopped) {
		DEBUG((" resuming queue"));
		netif_wake_queue(dev);
	} else {
		DEBUG((" starting queue"));
		netif_start_queue(priv);
	}

	priv->old_link = -1;
	priv->old_duplex = -1;

	phy_start(priv->phydev);

	return 0;
}

static long ethoc_stop(struct netif *dev)
{
	struct ethoc *priv = dev->data;

	napi_disable(priv);

	if (priv->phydev)
		phy_stop(priv->phydev);

	ethoc_disable_rx_and_tx(priv);

	/* free_irq(): the vector stays, so the MAC is silenced instead */
	ethoc_disable_irq(priv, INT_MASK_ALL);
	ethoc_ack_irq(priv, INT_MASK_ALL);

	if (!priv->queue_stopped)
		netif_stop_queue(priv);

	return 0;
}

static void ethoc_do_set_mac_address(struct netif *dev)
{
	const unsigned char *mac = dev->hwlocal.adr.bytes;
	struct ethoc *priv = dev->data;

	ethoc_write(priv, MAC_ADDR0, ((u32) mac[2] << 24) | ((u32) mac[3] << 16) |
				     ((u32) mac[4] <<  8) | ((u32) mac[5] <<  0));
	ethoc_write(priv, MAC_ADDR1, ((u32) mac[0] <<  8) | ((u32) mac[1] <<  0));
}

/* is_valid_ether_addr(): not multicast, not all zeros */
static long is_valid_ether_addr(const u8 *addr)
{
	long i;

	if (addr[0] & 0x01)
		return 0;

	for (i = 0; i < ETH_ALEN; i++)
		if (addr[i])
			return 1;

	return 0;
}

/*
 * eth_hw_addr_random(): a locally administered address. The kernel has
 * no entropy source, the 200 Hz counter and the time of day will do for
 * a fallback that is reported at boot.
 */
static void eth_hw_addr_random(struct netif *dev)
{
	u8 *addr = dev->hwlocal.adr.bytes;
	u32 r = *(volatile u32 *) 0x4baUL;
	long i;

	if (KERNEL->xtime)
		r ^= KERNEL->xtime->tv_sec ^ (KERNEL->xtime->tv_usec << 12);

	for (i = 0; i < ETH_ALEN; i++) {
		r = r * 1103515245UL + 12345UL;
		addr[i] = r >> 24;
	}

	addr[0] &= 0xfe;	/* clear multicast bit */
	addr[0] |= 0x02;	/* set local assignment bit (IEEE802) */
}

static long ethoc_set_mac_address(struct netif *dev, const void *p)
{
	const u8 *addr = p;

	if (!is_valid_ether_addr(addr))
		return EADDRNOTAVAIL;
	memcpy(dev->hwlocal.adr.bytes, addr, ETH_ALEN);
	ethoc_do_set_mac_address(dev);
	return 0;
}

/*
 * ether_crc(): the FCS polynomial over the address, bit 26 upwards select
 * the hash bit.
 */
static u32 ether_crc(long length, const u8 *data)
{
	long crc = -1;

	while (--length >= 0) {
		u8 current_octet = *data++;
		long bit;

		for (bit = 0; bit < 8; bit++, current_octet >>= 1) {
			crc = (crc << 1) ^
				((crc < 0) ^ (current_octet & 1) ? 0x04c11db7L : 0);
		}
	}

	return crc;
}

/*
 * The multicast list of Linux is the per-hash-bit reference count kept
 * by svethlana_igmp_mac_filter().
 */
static void ethoc_set_multicast_list(struct netif *dev)
{
	struct ethoc *priv = dev->data;
	u32 mode = ethoc_read(priv, MODER);
	u32 hash[2] = { 0, 0 };

	/* set loopback mode if requested */
	if (dev->flags & IFF_LOOPBACK)
		mode |=  MODER_LOOP;
	else
		mode &= ~MODER_LOOP;

	/* receive broadcast frames if requested */
	if (dev->flags & IFF_BROADCAST)
		mode &= ~MODER_BRO;
	else
		mode |=  MODER_BRO;

	/* enable promiscuous mode if requested */
	if (dev->flags & IFF_PROMISC)
		mode |=  MODER_PRO;
	else
		mode &= ~MODER_PRO;

	ethoc_write(priv, MODER, mode);

	/* receive multicast frames */
	if (dev->flags & IFF_ALLMULTI) {
		hash[0] = 0xffffffffUL;
		hash[1] = 0xffffffffUL;
	} else {
		long bit;

		for (bit = 0; bit < 64; bit++)
			if (priv->mc_refcnt[bit])
				hash[bit >> 5] |= 1UL << (bit & 0x1f);
	}

	ethoc_write(priv, ETH_HASH0, hash[0]);
	ethoc_write(priv, ETH_HASH1, hash[1]);
}

/* ethoc_change_mtu() returns -ENOSYS: the MTU stays at 1500 */

/* the netdev watchdog's tx_timeout, called by svethlana_timeout() */
static void ethoc_tx_timeout(struct netif *dev)
{
	struct ethoc *priv = dev->data;
	u32 pending = ethoc_read(priv, INT_SOURCE);
	if (likely(pending)) {
		ushort sr = spl7();
		ethoc_interrupt();
		spl(sr);
	}
}

/*
 * The buffer is the Linux skb: dstart to dend hold the complete Ethernet
 * frame. It is consumed here, whether it was sent or dropped.
 * skb_put_padto() becomes zero padding in the DMA buffer.
 */
static long ethoc_start_xmit(BUF *skb, struct netif *dev)
{
	struct ethoc *priv = dev->data;
	struct ethoc_bd bd;
	long entry;
	long len = skb->dend - skb->dstart;
	long padded_len = len < ETHOC_ZLEN ? ETHOC_ZLEN : len;
	void *dest;
	ushort sr;

	if (unlikely(len > ETHOC_BUFSIZ)) {
		dev->out_errors++;
		goto out;
	}

	entry = priv->cur_tx % priv->num_tx;
	sr = spl7();
	priv->cur_tx++;

	ethoc_read_bd(priv, entry, &bd);
	if (unlikely(padded_len < ETHOC_ZLEN))
		bd.stat |=  TX_BD_PAD;
	else
		bd.stat &= ~TX_BD_PAD;

	dest = priv->vma[entry];
	memcpy_toio(dest, skb->dstart, len);
	if (padded_len > len)
		memset_io((char *) dest + len, 0, padded_len - len);

	bd.stat &= ~(TX_BD_STATS | TX_BD_LEN_MASK);
	bd.stat |= TX_BD_LEN(padded_len);
	ethoc_write_bd(priv, entry, &bd);

	bd.stat |= TX_BD_READY;
	ethoc_write_bd(priv, entry, &bd);

	if (priv->cur_tx == (priv->dty_tx + priv->num_tx)) {
		DEBUG(("stopping queue"));
		netif_stop_queue(priv);
	}

	spl(sr);
out:
	buf_deref(skb, BUF_NORMAL);
	return 0;
}

/**
 * ethoc_probe - initialize OpenCores ethernet MAC
 * @netdev:	the interface
 * @hwaddr:	the address from SVETHLAN.INF, all zeros if there was none
 *
 * The resources come from sv_regs.h and driver_init() instead of the
 * platform device.
 */
static long ethoc_probe(struct netif *netdev, char *membase, const u8 *hwaddr)
{
	struct ethoc *priv = NULL;
	long num_bd;
	long ret = 0;

	netdev->data = priv = &ethoc_priv;
	priv->netdev = netdev;

	priv->iobase = (char *) ATARI_SVETHLANA_PHYS_ADDR;
	priv->membase = membase;

	/*
	 * The MAC may still be running from before a warm boot, and must not
	 * raise anything before the vector is ours.
	 */
	ethoc_write(priv, MODER, 0);
	ethoc_write(priv, INT_MASK, 0);
	ethoc_ack_irq(priv, INT_MASK_ALL);

	/* calculate the number of TX/RX buffers, maximum 128 supported */
	num_bd = SVETHLANA_BUF_SIZE / ETHOC_BUFSIZ;
	if (num_bd > 128)
		num_bd = 128;
	if (num_bd < 4) {
		ret = ENODEV;
		goto free;
	}
	priv->num_bd = num_bd;
	/* num_tx must be a power of two */
	priv->num_tx = 1;
	while (priv->num_tx * 2 <= (num_bd >> 1))
		priv->num_tx *= 2;
	priv->num_rx = num_bd - priv->num_tx;

	DEBUG(("ethoc: num_tx: %ld num_rx: %ld", priv->num_tx, priv->num_rx));

	priv->vma = ethoc_vma;

	/* Allow the platform setup code to pass in a MAC address. */
	memcpy(netdev->hwlocal.adr.bytes, hwaddr, ETH_ALEN);
	priv->phy_id = -1;

	/* Check that the given MAC address is valid. If it isn't, read the
	 * current MAC from the controller.
	 */
	if (!is_valid_ether_addr(netdev->hwlocal.adr.bytes)) {
		u8 addr[ETH_ALEN];

		ethoc_get_mac_address(netdev, addr);
		memcpy(netdev->hwlocal.adr.bytes, addr, ETH_ALEN);
	}

	/* Check the MAC again for validity, if it still isn't choose and
	 * program a random one.
	 */
	if (!is_valid_ether_addr(netdev->hwlocal.adr.bytes)) {
		eth_hw_addr_random(netdev);
		c_conws("SVEthlana: no address in SVETHLAN.INF, using a random one\r\n");
	}

	ethoc_do_set_mac_address(netdev);

	/* The MII management bus clock is left at its reset value, as on
	 * Linux, where the Atari platform data carries no eth_clkfreq.
	 */

	/* register MII bus */
	priv->mdio = &ethoc_mdio;
	priv->mdio->read = ethoc_mdio_read;
	priv->mdio->write = ethoc_mdio_write;
	priv->mdio->priv = priv;

	ret = ethoc_mdio_probe(netdev);
	if (ret) {
		ALERT(("svethlana: failed to probe MDIO bus"));
		goto free;
	}

	return 0;

free:
	return ret;
}


/*
 * The MiNTNet side: what net_device, its ops and arch/m68k/atari/config.c
 * do on Linux.
 */

static long	svethlana_open		(struct netif *);
static long	svethlana_close		(struct netif *);
static long	svethlana_output	(struct netif *, BUF *, const char *, short, short);
static long	svethlana_ioctl		(struct netif *, short, long);
static long	svethlana_config	(struct netif *, struct ifopt *);
static void	svethlana_igmp_mac_filter (struct netif *, ulong, char);
static void	svethlana_timeout	(struct netif *);

/*
 * This gets called when someone makes an 'ifconfig up' on this interface
 * and the interface was down before.
 */
static long
svethlana_open (struct netif *nif)
{
	long error;

	error = ethoc_open (nif);
	if (error)
		return error;

	/* __dev_open() applies the receive mode after ndo_open */
	ethoc_set_multicast_list (nif);

	return 0;
}

/*
 * Opposite of svethlana_open(), is called when 'ifconfig down' on this interface
 * is done and the interface was up before.
 */
static long
svethlana_close (struct netif *nif)
{
	return ethoc_stop (nif);
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
/* ndo_start_xmit with the qdisc in front of it */
static long
svethlana_output (struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	struct ethoc *priv = nif->data;
	BUF *nbuf;

	nbuf = eth_build_hdr (buf, nif, hwaddr, pktype);
	if (nbuf == NULL)
	{
		nif->out_errors++;
		return ENOMEM;
	}

	if (nif->bpf)
		bpf_input (nif, nbuf);

	if (priv->queue_stopped)
	{
		/* if_enqueue() frees the buffer when the queue is full */
		if (if_enqueue (&nif->snd, nbuf, nbuf->info))
			nif->out_errors++;
		return 0;
	}

	return ethoc_start_xmit (nbuf, nif);
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
svethlana_ioctl (struct netif *nif, short cmd, long arg)
{
	struct ifreq *ifr;

	switch (cmd)
	{
		case SIOCSIFNETMASK:
		case SIOCSIFADDR:
			return 0;

		case SIOCSIFFLAGS:
			/* ndo_set_rx_mode */
			if (nif->flags & IFF_UP)
				ethoc_set_multicast_list (nif);
			return 0;

		case SIOCSIFMTU:
			/*
			 * ethoc_change_mtu() refuses everything, the frame
			 * buffers are ETHOC_BUFSIZ. MintNet has already set
			 * nif->mtu to the new value, we only limit it here.
			 */
			if (nif->mtu > ETH_MAX_DLEN)
				nif->mtu = ETH_MAX_DLEN;
			return 0;

		case SIOCSIFOPT:
			ifr = (struct ifreq *) arg;
			return svethlana_config (nif, ifr->ifru.data);
	}

	return ENOSYS;
}

/*
 * Interface configuration via SIOCSIFOPT. The ioctl is passed a
 * struct ifreq *ifr. ifr->ifru.data points to a struct ifopt, which
 * we get as the second argument here.
 *
 * If the user MUST configure some parameters before the interface
 * can run make sure that svethlana_open() fails unless all the necessary
 * parameters are set.
 *
 * Return values	meaning
 * ENOSYS		option not supported
 * ENOENT		invalid option value
 * 0			Ok
 */
static long
svethlana_config (struct netif *nif, struct ifopt *ifo)
{
# define STRNCMP(s)	(strncmp ((s), ifo->option, sizeof (ifo->option)))

	if (!STRNCMP ("hwaddr"))
	{
		/*
		 * Set hardware address, ndo_set_mac_address
		 */
		if (ifo->valtype != IFO_HWADDR)
			return ENOENT;
		return ethoc_set_mac_address (nif, ifo->ifou.v_string);
	}
	else if (!STRNCMP ("braddr"))
	{
		/*
		 * Set broadcast address
		 */
		if (ifo->valtype != IFO_HWADDR)
			return ENOENT;
		memcpy (nif->hwbrcst.adr.bytes, ifo->ifou.v_string, ETH_ALEN);
		return 0;
	}

	return ENOSYS;
}

/*
 * The IGMP code reports groups one at a time; Linux recomputes the hash
 * from the complete list, so the users of every hash bit are counted.
 */
static void
svethlana_igmp_mac_filter (struct netif *nif, ulong group, char action)
{
	struct ethoc *priv = nif->data;
	u8 addr[ETH_ALEN];
	long bit;

	/* ip_eth_mc_map(): 01:00:5e plus the low 23 bits of the group */
	addr[0] = 0x01;
	addr[1] = 0x00;
	addr[2] = 0x5e;
	addr[3] = (group >> 16) & 0x7f;
	addr[4] = (group >>  8) & 0xff;
	addr[5] = (group >>  0) & 0xff;

	bit = (ether_crc (ETH_ALEN, addr) >> 26) & 0x3f;

	if (action == IGMP_ADD_MAC_FILTER)
	{
		if (priv->mc_refcnt[bit] < 255)
			priv->mc_refcnt[bit]++;
	}
	else
	{
		if (priv->mc_refcnt[bit])
			priv->mc_refcnt[bit]--;
	}

	if (nif->flags & IFF_UP)
		ethoc_set_multicast_list (nif);
}

/*
 * Called every IF_SLOWTIMEOUT (one second) while the interface is up.
 * That is PHY_STATE_TIME, the Linux PHY polling period, and it also
 * stands in for the netdev watchdog.
 */
static void
svethlana_timeout (struct netif *nif)
{
	struct ethoc *priv = nif->data;

	phy_state_machine (priv->phydev);

	/* a poll that could not be scheduled for lack of a timeout */
	if (priv->napi_scheduled && !priv->napi)
		__napi_schedule (priv, 0);

	/* netdev watchdog: the ring stays full only when completions stop */
	if (priv->queue_stopped)
	{
		if (priv->watchdog++)
			ethoc_tx_timeout (nif);
	}
	else
		priv->watchdog = 0;
}

/*
 * SVETHLAN.INF holds the address as 12 hex digits, optionally separated
 * by ':' or '-'. This is the hwaddr the platform data carries on Linux,
 * looked for in the current directory (the sysdir at boot) and then in
 * the root of the boot drive.
 */
# define DriveToLetter(d) ((d) < 26 ? 'A' + (d) : (d) - 26 + '1')

static long
svethlana_read_inf (u8 *hwaddr)
{
	char buf[32];
	char path[] = "A:\\SVETHLAN.INF";
	long fd, n, i, digits = 0;
	u8 value = 0;

	memset (hwaddr, 0, ETH_ALEN);

	fd = f_open ("svethlan.inf", O_RDONLY);
	if (fd < 0)
	{
		path[0] = DriveToLetter (*(short *) 0x446L);
		fd = f_open (path, O_RDONLY);
	}
	if (fd < 0)
		return fd;

	n = f_read (fd, sizeof (buf), buf);
	f_close (fd);
	if (n < 0)
		return n;

	for (i = 0; i < n && digits < 12; i++)
	{
		char c = buf[i];
		u8 nibble;

		if (c >= '0' && c <= '9')
			nibble = c - '0';
		else if (c >= 'a' && c <= 'f')
			nibble = c - 'a' + 10;
		else if (c >= 'A' && c <= 'F')
			nibble = c - 'A' + 10;
		else if (c == ':' || c == '-')
			continue;
		else
			break;

		value = (value << 4) | nibble;
		if (digits & 1)
			hwaddr[digits >> 1] = value;
		digits++;
	}

	if (digits != 12)
	{
		memset (hwaddr, 0, ETH_ALEN);
		return EINVAL;
	}

	return 0;
}

long driver_init (void);

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
	static char message[128];
	struct netif *nif = &if_svethlana;
	struct ethoc *priv;
	u8 hwaddr[ETH_ALEN];
	const u8 *mac;
	u32 version;
	long membase;

	/*
	 * hwreg_present() and the version check of the Linux platform code.
	 * The SuperVidel XBIOS is needed for the buffer memory anyway, and
	 * its cookie proves the hardware is there before its registers are
	 * touched.
	 */
	if (get_toscookie (COOKIE_SupV, NULL) != 0)
	{
		c_conws ("SVEthlana: SuperVidel XBIOS not found (no SupV cookie)\r\n");
		return -1;
	}

	version = *(volatile u32 *) ATARI_SV_VERSION_PHYS_ADDR & ATARI_SV_VERSION_MASK;
	if (version < ATARI_SV_MIN_VERSION)
	{
		ksprintf (message, "SVEthlana: SuperVidel firmware %lu is too old, %d or newer is required\r\n",
			version, ATARI_SV_MIN_VERSION);
		c_conws (message);
		return -1;
	}

	membase = ct60_vmalloc (0, SVETHLANA_BUF_SIZE + SVETHLANA_BUF_ALIGN);
	if (membase == 0 || membase == -1)
	{
		c_conws ("SVEthlana: cannot allocate the packet buffers in SuperVidel RAM\r\n");
		return -1;
	}
	membase = (membase + SVETHLANA_BUF_ALIGN - 1) & ~(SVETHLANA_BUF_ALIGN - 1);

	svethlana_read_inf (hwaddr);

	/*
	 * Set interface name
	 */
	strcpy (nif->name, "en");
	/*
	 * Set interface unit. if_getfreeunit("name") returns a yet
	 * unused unit number for the interface type "name".
	 */
	nif->unit = if_getfreeunit ("en");
	/*
	 * Alays set to zero
	 */
	nif->metric = 0;
	/*
	 * Initial interface flags, should be IFF_BROADCAST for
	 * Ethernet.
	 */
	nif->flags = IFF_BROADCAST;
	/*
	 * Maximum transmission unit, should be >= 46 and <= 1500 for
	 * Ethernet
	 */
	nif->mtu = 1500;
	/*
	 * Time in ms between calls to (*nif->timeout) ();
	 */
	nif->timer = 0;

	/*
	 * Interface hardware type
	 */
	nif->hwtype = HWTYPE_ETH;
	/*
	 * Hardware address length, 6 bytes for Ethernet
	 */
	nif->hwlocal.len =
	nif->hwbrcst.len = ETH_ALEN;

	memcpy (nif->hwbrcst.adr.bytes, "\377\377\377\377\377\377", ETH_ALEN);

	/*
	 * Set length of send and receive queue. IF_MAXQ is a good value.
	 */
	nif->rcv.maxqlen = IF_MAXQ;
	nif->snd.maxqlen = IF_MAXQ;
	/*
	 * Setup pointers to service functions
	 */
	nif->open = svethlana_open;
	nif->close = svethlana_close;
	nif->output = svethlana_output;
	nif->ioctl = svethlana_ioctl;
	nif->igmp_mac_filter = svethlana_igmp_mac_filter;
	/*
	 * Timer function that is called every second.
	 */
	nif->timeout = svethlana_timeout;

	if (ethoc_probe (nif, (char *) membase, hwaddr) != 0)
	{
		c_conws ("SVEthlana: initialization failed\r\n");
		ct60_vmalloc (1, membase);
		return -1;
	}
	priv = nif->data;

	/*
	 * Number of packets the hardware can receive in fast succession,
	 * 0 means unlimited.
	 */
	nif->maxpackets = priv->num_rx;

	/*
	 * Register the interface.
	 */
	if_register (nif);

	svethlana_old_vector = (void (*)(void)) Setexc (ATARI_SVETHLANA_VECTOR, (long) svethlana_interrupt);

	mac = nif->hwlocal.adr.bytes;
	ksprintf (message, "SVEthlana driver v1.0 (%s%d): SuperVidel FW %lu, PHY %08lx at %ld, %02x:%02x:%02x:%02x:%02x:%02x\r\n",
		nif->name, nif->unit, version, priv->phydev->phy_id, priv->phydev->mdio_addr,
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	c_conws (message);

	return 0;
}
