/*
 * The Linux PHY library as far as the generic PHY driver on a 10/100 PHY
 * needs it: include/linux/phy.h, include/uapi/linux/mii.h and
 * include/uapi/linux/ethtool.h reduced to that. See phy.c.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* include/linux/phy.h */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Framework and drivers for configuring and reading different PHYs
 * Based on code in sungem_phy.c and (long-removed) gianfar_phy.c
 *
 * Author: Andy Fleming
 *
 * Copyright (c) 2004 Freescale Semiconductor, Inc.
 */

/* include/uapi/linux/mii.h */
/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * linux/mii.h: definitions for MII-compatible transceivers
 * Originally drivers/net/sunhme.h.
 *
 * Copyright (C) 1996, 1999, 2001 David S. Miller (davem@redhat.com)
 */

/* include/uapi/linux/ethtool.h */
/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * ethtool.h: Defines for Linux ethtool.
 *
 * Copyright (C) 1998 David S. Miller (davem@redhat.com)
 * Copyright 2001 Jeff Garzik <jgarzik@pobox.com>
 * Portions Copyright 2001 Sun Microsystems (thockin@sun.com)
 * Portions Copyright 2002 Intel (eli.kupermann@intel.com,
 *                                christopher.leech@intel.com,
 *                                scott.feldman@intel.com)
 * Portions Copyright (C) Sun Microsystems 2008
 */


#ifndef _phy_h
#define _phy_h

typedef unsigned long	u32;
typedef unsigned short	u16;
typedef unsigned char	u8;
typedef signed char	s8;

/* Generic MII registers. */
#define MII_BMCR		0x00	/* Basic mode control register */
#define MII_BMSR		0x01	/* Basic mode status register  */
#define MII_PHYSID1		0x02	/* PHYS ID 1                   */
#define MII_PHYSID2		0x03	/* PHYS ID 2                   */
#define MII_ADVERTISE		0x04	/* Advertisement control reg   */
#define MII_LPA			0x05	/* Link partner ability reg    */

/* Basic mode control register. */
#define BMCR_FULLDPLX		0x0100	/* Full duplex                 */
#define BMCR_ANRESTART		0x0200	/* Auto negotiation restart    */
#define BMCR_ISOLATE		0x0400	/* Isolate data paths from MII */
#define BMCR_PDOWN		0x0800	/* Enable low power state      */
#define BMCR_ANENABLE		0x1000	/* Enable auto negotiation     */
#define BMCR_SPEED100		0x2000	/* Select 100Mbps              */
#define BMCR_LOOPBACK		0x4000	/* TXD loopback bits           */
#define BMCR_RESET		0x8000	/* Reset to default state      */

/* Basic mode status register. */
#define BMSR_LSTATUS		0x0004	/* Link status                 */
#define BMSR_ANEGCAPABLE	0x0008	/* Able to do auto-negotiation */
#define BMSR_ANEGCOMPLETE	0x0020	/* Auto-negotiation complete   */
#define BMSR_ESTATEN		0x0100	/* Extended Status in R15      */
#define BMSR_10HALF		0x0800	/* Can do 10mbps, half-duplex  */
#define BMSR_10FULL		0x1000	/* Can do 10mbps, full-duplex  */
#define BMSR_100HALF		0x2000	/* Can do 100mbps, half-duplex */
#define BMSR_100FULL		0x4000	/* Can do 100mbps, full-duplex */
#define BMSR_100BASE4		0x8000	/* Can do 100mbps, 4k packets  */

/* Advertisement control register. */
#define ADVERTISE_CSMA		0x0001	/* Only selector supported     */
#define ADVERTISE_10HALF	0x0020	/* Try for 10mbps half-duplex  */
#define ADVERTISE_10FULL	0x0040	/* Try for 10mbps full-duplex  */
#define ADVERTISE_100HALF	0x0080	/* Try for 100mbps half-duplex */
#define ADVERTISE_100FULL	0x0100	/* Try for 100mbps full-duplex */
#define ADVERTISE_100BASE4	0x0200	/* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP	0x0400	/* Try for pause               */
#define ADVERTISE_PAUSE_ASYM	0x0800	/* Try for asymetric pause     */

#define ADVERTISE_ALL		(ADVERTISE_10HALF | ADVERTISE_10FULL | \
				ADVERTISE_100HALF | ADVERTISE_100FULL)

/* Link partner ability register. */
#define LPA_10HALF		0x0020	/* Can do 10mbps half-duplex   */
#define LPA_10FULL		0x0040	/* Can do 10mbps full-duplex   */
#define LPA_100HALF		0x0080	/* Can do 100mbps half-duplex  */
#define LPA_100FULL		0x0100	/* Can do 100mbps full-duplex  */
#define LPA_100BASE4		0x0200	/* Can do 100mbps 4k packets   */

/* ethtool.h */
#define SPEED_10		10
#define SPEED_100		100
#define SPEED_UNKNOWN		-1

#define DUPLEX_HALF		0x00
#define DUPLEX_FULL		0x01
#define DUPLEX_UNKNOWN		0xff

#define AUTONEG_DISABLE		0x00
#define AUTONEG_ENABLE		0x01

#define PHY_MAX_ADDR		32

/**
 * enum phy_state - PHY state machine states:
 *
 * @PHY_DOWN: PHY device and driver are not ready for anything.  probe
 * should be called if and only if the PHY is in this state,
 * given that the PHY device exists.
 * - PHY driver probe function will set the state to @PHY_READY
 *
 * @PHY_READY: PHY is ready to send and receive packets, but the
 * controller is not.  By default, PHYs which do not implement
 * probe will be set to this state by phy_probe().
 * - start will set the state to UP
 *
 * @PHY_UP: The PHY and attached device are ready to do work.
 * Interrupts should be started here.
 * - timer moves to @PHY_NOLINK or @PHY_RUNNING
 *
 * @PHY_NOLINK: PHY is up, but not currently plugged in.
 * - irq or timer will set @PHY_RUNNING if link comes back
 * - phy_stop moves to @PHY_HALTED
 *
 * @PHY_RUNNING: PHY is currently up, running, and possibly sending
 * and/or receiving packets
 * - irq or timer will set @PHY_NOLINK if link goes down
 * - phy_stop moves to @PHY_HALTED
 *
 * @PHY_HALTED: PHY is up, but no polling or interrupts are done.
 * - phy_start moves to @PHY_UP
 *
 * @PHY_ERROR: PHY is up, but is in an error state.
 * - phy_stop moves to @PHY_HALTED
 */
enum phy_state {
	PHY_DOWN = 0,
	PHY_READY,
	PHY_HALTED,
	PHY_ERROR,
	PHY_UP,
	PHY_RUNNING,
	PHY_NOLINK
};

struct mii_bus;

/**
 * struct phy_device - what ethoc needs of the Linux phy_device
 * @bus:	the MDIO bus
 * @mdio_addr:	address on the bus
 * @phy_id:	PHYSID1/2
 * @state:	state machine state
 * @link:	link up
 * @autoneg_complete: BMSR says autonegotiation is done
 * @autoneg:	AUTONEG_ENABLE or AUTONEG_DISABLE
 * @speed:	SPEED_10, SPEED_100 or SPEED_UNKNOWN
 * @duplex:	DUPLEX_HALF, DUPLEX_FULL or DUPLEX_UNKNOWN
 * @supported:	what the PHY can do, as MII_ADVERTISE bits
 * @advertising: what we advertise, as MII_ADVERTISE bits
 * @lp_advertising: what the link partner advertises, as MII_LPA bits
 * @adjust_link: the MAC driver's link change callback
 * @attached_dev: the interface the PHY serves
 *
 * The link mode masks of Linux are 10/100 only here, so they are kept in
 * the register bit layout of MII_ADVERTISE, which is what
 * linkmode_adv_to_mii_adv_t() produces from them.
 */
struct phy_device {
	struct mii_bus *bus;
	long mdio_addr;
	u32 phy_id;

	enum phy_state state;

	long link;
	long autoneg_complete;
	long autoneg;
	long speed;
	long duplex;

	u16 supported;
	u16 advertising;
	u16 lp_advertising;

	void (*adjust_link)(struct netif *);
	struct netif *attached_dev;
};

/**
 * struct mii_bus - the MDIO bus, as far as one PHY on it is concerned
 * @read:	the MAC driver's register read
 * @write:	the MAC driver's register write
 * @priv:	the MAC driver's data
 * @phy:	the one PHY the bus can hold, Linux keeps 32
 */
struct mii_bus {
	long (*read)(struct mii_bus *bus, long addr, long regnum);
	long (*write)(struct mii_bus *bus, long addr, long regnum, u16 val);
	void *priv;

	struct phy_device phy;
};

struct phy_device *phy_find_next(struct mii_bus *bus, struct phy_device *pos);

static inline struct phy_device *phy_find_first(struct mii_bus *bus)
{
	return phy_find_next(bus, NULL);
}
struct phy_device *mdiobus_get_phy(struct mii_bus *bus, long addr);
long phy_connect_direct(struct netif *dev, struct phy_device *phydev,
			void (*handler)(struct netif *));
void phy_set_max_speed(struct phy_device *phydev, long max_speed);
void phy_start(struct phy_device *phydev);
void phy_stop(struct phy_device *phydev);
void phy_state_machine(struct phy_device *phydev);
void phy_print_status(struct phy_device *phydev);

#endif /* _phy_h */
