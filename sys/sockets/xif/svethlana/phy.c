/*
 * The Linux PHY library: drivers/net/phy/phy_device.c, phy-core.c,
 * phy.c and mdio_bus.c (Linux 7.3), as far as the generic PHY driver on
 * a 10/100 PHY polled once a second exercises them. Names are kept as in Linux so
 * that fixes can be carried over.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * The kernel is built with -mshort, so Linux "int" is "long" here.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

/* drivers/net/phy/phy_device.c */
// SPDX-License-Identifier: GPL-2.0+
/* Framework for finding and configuring PHYs.
 * Also contains generic PHY driver
 *
 * Author: Andy Fleming
 *
 * Copyright (c) 2004 Freescale Semiconductor, Inc.
 */

/* drivers/net/phy/phy-core.c */
// SPDX-License-Identifier: GPL-2.0+
/*
 * Core PHY library, taken from phy.c
 */

/* drivers/net/phy/phy.c */
// SPDX-License-Identifier: GPL-2.0+
/* Framework for configuring and reading PHY devices
 * Based on code in sungem_phy.c and gianfar_phy.c
 *
 * Author: Andy Fleming
 *
 * Copyright (c) 2004 Freescale Semiconductor, Inc.
 * Copyright (c) 2006, 2007  Maciej W. Rozycki
 */

/* drivers/net/phy/mdio_bus.c */
// SPDX-License-Identifier: GPL-2.0+
/* MDIO Bus interface
 *
 * Author: Andy Fleming
 *
 * Copyright (c) 2004 Freescale Semiconductor, Inc.
 */


# include "global.h"

# include "inet4/if.h"

# include "phy.h"


static long phy_read(struct phy_device *phydev, long regnum)
{
	return phydev->bus->read(phydev->bus, phydev->mdio_addr, regnum);
}

static long phy_write(struct phy_device *phydev, long regnum, u16 val)
{
	return phydev->bus->write(phydev->bus, phydev->mdio_addr, regnum, val);
}

/* returns 1 if the register changed, 0 if not, negative on error */
static long phy_modify_changed(struct phy_device *phydev, long regnum,
			       u16 mask, u16 set)
{
	long new, ret;

	ret = phy_read(phydev, regnum);
	if (ret < 0)
		return ret;

	new = (ret & ~mask) | set;
	if (new == ret)
		return 0;

	ret = phy_write(phydev, regnum, new);

	return ret < 0 ? ret : 1;
}

static long phy_modify(struct phy_device *phydev, long regnum,
		       u16 mask, u16 set)
{
	long ret;

	ret = phy_modify_changed(phydev, regnum, mask, set);

	return ret < 0 ? ret : 0;
}

static long phy_set_bits(struct phy_device *phydev, long regnum, u16 val)
{
	return phy_modify(phydev, regnum, 0, val);
}

static long phy_clear_bits(struct phy_device *phydev, long regnum, u16 val)
{
	return phy_modify(phydev, regnum, val, 0);
}

static inline long phy_is_started(struct phy_device *phydev)
{
	return phydev->state >= PHY_UP;
}

static long get_phy_c22_id(struct mii_bus *bus, long addr, u32 *phy_id)
{
	long phy_reg;

	/* Grab the bits from PHYIR1, and put them in the upper half */
	phy_reg = bus->read(bus, addr, MII_PHYSID1);
	if (phy_reg < 0) {
		/* returning -ENODEV doesn't stop bus scanning */
		return (phy_reg == EIO || phy_reg == ENODEV) ? ENODEV : EIO;
	}

	*phy_id = phy_reg << 16;

	/* Grab the bits from PHYIR2, and put them in the lower half */
	phy_reg = bus->read(bus, addr, MII_PHYSID2);
	if (phy_reg < 0) {
		/* returning -ENODEV doesn't stop bus scanning */
		return (phy_reg == EIO || phy_reg == ENODEV) ? ENODEV : EIO;
	}

	*phy_id |= phy_reg;

	/* If the phy_id is mostly Fs, there is no device there */
	if ((*phy_id & 0x1fffffffUL) == 0x1fffffffUL)
		return ENODEV;

	return 0;
}

/*
 * mdiobus_scan() for one address: the PHY is looked for right here.
 */
struct phy_device *mdiobus_get_phy(struct mii_bus *bus, long addr)
{
	struct phy_device *phydev = &bus->phy;
	u32 phy_id;

	if (get_phy_c22_id(bus, addr, &phy_id) != 0)
		return NULL;

	phydev->bus = bus;
	phydev->mdio_addr = addr;
	phydev->phy_id = phy_id;
	phydev->state = PHY_DOWN;

	return phydev;
}

/**
 * phy_find_next - finds the next PHY device on the bus
 * @bus: the target MII bus
 * @pos: cursor
 *
 * Return: next phy_device on the bus, or NULL
 */
struct phy_device *phy_find_next(struct mii_bus *bus, struct phy_device *pos)
{
	long addr;

	for (addr = pos ? pos->mdio_addr + 1 : 0;
	     addr < PHY_MAX_ADDR; addr++) {
		struct phy_device *phydev = mdiobus_get_phy(bus, addr);

		if (phydev)
			return phydev;
	}
	return NULL;
}

/**
 * genphy_read_abilities - read PHY abilities from Clause 22 registers
 * @phydev: target phy_device struct
 *
 * Description: Reads the PHY's abilities and populates
 * phydev->supported accordingly.
 *
 * Returns: 0 on success, < 0 on failure
 */
static long genphy_read_abilities(struct phy_device *phydev)
{
	long val;

	val = phy_read(phydev, MII_BMSR);
	if (val < 0)
		return val;

	phydev->supported = 0;

	if (val & BMSR_100FULL)
		phydev->supported |= ADVERTISE_100FULL;
	if (val & BMSR_100HALF)
		phydev->supported |= ADVERTISE_100HALF;
	if (val & BMSR_10FULL)
		phydev->supported |= ADVERTISE_10FULL;
	if (val & BMSR_10HALF)
		phydev->supported |= ADVERTISE_10HALF;

	phydev->autoneg = (val & BMSR_ANEGCAPABLE) ?
		AUTONEG_ENABLE : AUTONEG_DISABLE;

	return 0;
}

/**
 * genphy_config_advert - sanitize and advertise auto-negotiation parameters
 * @phydev: target phy_device struct
 * @advert: auto-negotiation parameters to advertise
 *
 * Description: Writes MII_ADVERTISE with the appropriate values,
 *   after sanitizing the values to make sure we only advertise
 *   what is supported.  Returns < 0 on error, 0 if the PHY's advertisement
 *   hasn't changed, and > 0 if it has changed.
 *
 * The gigabit half (MII_CTRL1000) is left out: phy_set_max_speed() limits
 * ethoc to 100 Mbps and the KSZ8041 has no extended status anyway.
 */
static long genphy_config_advert(struct phy_device *phydev, u16 advert)
{
	long err, changed = 0;
	u16 adv;

	adv = advert & (ADVERTISE_ALL | ADVERTISE_100BASE4 |
			ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM);

	/* Setup standard advertisement */
	err = phy_modify_changed(phydev, MII_ADVERTISE,
				 ADVERTISE_ALL | ADVERTISE_100BASE4 |
				 ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM,
				 adv);
	if (err < 0)
		return err;
	if (err > 0)
		changed = 1;

	return changed;
}

/**
 * genphy_setup_forced - configures/forces speed/duplex from @phydev
 * @phydev: target phy_device struct
 *
 * Description: Configures MII_BMCR to force speed/duplex
 *   to the values in phydev. Assumes that the values are valid.
 *   Please see phy_sanitize_settings().
 */
static long genphy_setup_forced(struct phy_device *phydev)
{
	u16 ctl = 0;

	/* mii_bmcr_encode_fixed() */
	if (phydev->speed == SPEED_100)
		ctl |= BMCR_SPEED100;
	if (phydev->duplex == DUPLEX_FULL)
		ctl |= BMCR_FULLDPLX;

	return phy_modify(phydev, MII_BMCR,
			  ~(BMCR_LOOPBACK | BMCR_ISOLATE | BMCR_PDOWN), ctl);
}

/**
 * genphy_restart_aneg - Enable and Restart Autonegotiation
 * @phydev: target phy_device struct
 */
static long genphy_restart_aneg(struct phy_device *phydev)
{
	/* Don't isolate the PHY if we're negotiating */
	return phy_modify(phydev, MII_BMCR, BMCR_ISOLATE,
			  BMCR_ANENABLE | BMCR_ANRESTART);
}

/**
 * genphy_check_and_restart_aneg - Enable and restart auto-negotiation
 * @phydev: target phy_device struct
 * @restart: whether aneg restart is requested
 *
 * Check, and restart auto-negotiation if needed.
 */
static long genphy_check_and_restart_aneg(struct phy_device *phydev, long restart)
{
	long ret;

	if (!restart) {
		/* Advertisement hasn't changed, but maybe aneg was never on to
		 * begin with?  Or maybe phy was isolated?
		 */
		ret = phy_read(phydev, MII_BMCR);
		if (ret < 0)
			return ret;

		if (!(ret & BMCR_ANENABLE) || (ret & BMCR_ISOLATE))
			restart = 1;
	}

	if (restart)
		return genphy_restart_aneg(phydev);

	return 0;
}

/**
 * __genphy_config_aneg - restart auto-negotiation or write BMCR
 * @phydev: target phy_device struct
 * @changed: whether autoneg is requested
 *
 * Description: If auto-negotiation is enabled, we configure the
 *   advertising, and then restart auto-negotiation.  If it is not
 *   enabled, then we write the BMCR.
 */
static long __genphy_config_aneg(struct phy_device *phydev, long changed)
{
	long err;

	if (phydev->autoneg != AUTONEG_ENABLE)
		return genphy_setup_forced(phydev);

	/* Only allow advertising what this PHY supports */
	phydev->advertising &= phydev->supported;

	err = genphy_config_advert(phydev, phydev->advertising);
	if (err < 0) /* error */
		return err;
	else if (err)
		changed = 1;

	return genphy_check_and_restart_aneg(phydev, changed);
}

static inline long genphy_config_aneg(struct phy_device *phydev)
{
	return __genphy_config_aneg(phydev, 0);
}

/**
 * genphy_update_link - update link status in @phydev
 * @phydev: target phy_device struct
 *
 * Description: Update the value in phydev->link to reflect the
 *   current link value.  In order to do this, we need to read
 *   the status register twice, keeping the second value.
 */
static long genphy_update_link(struct phy_device *phydev)
{
	long status = 0, bmcr;

	bmcr = phy_read(phydev, MII_BMCR);
	if (bmcr < 0)
		return bmcr;

	/* Autoneg is being started, therefore disregard BMSR value and
	 * report link as down.
	 */
	if (bmcr & BMCR_ANRESTART)
		goto done;

	/* The link state is latched low so that momentary link
	 * drops can be detected. Do not double-read the status
	 * in polling mode to detect such short link drops except
	 * if the link was already down.
	 */
	if (!phydev->link) {
		status = phy_read(phydev, MII_BMSR);
		if (status < 0)
			return status;
		else if (status & BMSR_LSTATUS)
			goto done;
	}

	/* Read link and autonegotiation status */
	status = phy_read(phydev, MII_BMSR);
	if (status < 0)
		return status;
done:
	phydev->link = status & BMSR_LSTATUS ? 1 : 0;
	phydev->autoneg_complete = status & BMSR_ANEGCOMPLETE ? 1 : 0;

	/* Consider the case that autoneg was started and "aneg complete"
	 * bit has been reset, but "link up" bit not yet.
	 */
	if (phydev->autoneg == AUTONEG_ENABLE && !phydev->autoneg_complete)
		phydev->link = 0;

	return 0;
}

static long genphy_read_lpa(struct phy_device *phydev)
{
	long lpa;

	if (phydev->autoneg == AUTONEG_ENABLE) {
		if (!phydev->autoneg_complete) {
			phydev->lp_advertising = 0;
			return 0;
		}

		lpa = phy_read(phydev, MII_LPA);
		if (lpa < 0)
			return lpa;

		phydev->lp_advertising = lpa;
	} else {
		phydev->lp_advertising = 0;
	}

	return 0;
}

/**
 * phy_resolve_aneg_linkmode - resolve the advertisements into PHY settings
 * @phydev: The phy_device struct
 *
 * Resolve our and the link partner advertisements into their corresponding
 * speed and duplex. If full duplex was negotiated, extract the pause mode
 * from the link partner mask.
 */
static void phy_resolve_aneg_linkmode(struct phy_device *phydev)
{
	u16 common = phydev->lp_advertising & phydev->advertising;

	if (common & LPA_100FULL) {
		phydev->speed = SPEED_100;
		phydev->duplex = DUPLEX_FULL;
	} else if (common & (LPA_100HALF | LPA_100BASE4)) {
		phydev->speed = SPEED_100;
		phydev->duplex = DUPLEX_HALF;
	} else if (common & LPA_10FULL) {
		phydev->speed = SPEED_10;
		phydev->duplex = DUPLEX_FULL;
	} else if (common & LPA_10HALF) {
		phydev->speed = SPEED_10;
		phydev->duplex = DUPLEX_HALF;
	}
}

/**
 * genphy_read_status_fixed - read the link parameters for !aneg mode
 * @phydev: target phy_device struct
 *
 * Read the current duplex and speed state for a PHY operating with
 * autonegotiation disabled.
 */
static long genphy_read_status_fixed(struct phy_device *phydev)
{
	long bmcr = phy_read(phydev, MII_BMCR);

	if (bmcr < 0)
		return bmcr;

	if (bmcr & BMCR_FULLDPLX)
		phydev->duplex = DUPLEX_FULL;
	else
		phydev->duplex = DUPLEX_HALF;

	if (bmcr & BMCR_SPEED100)
		phydev->speed = SPEED_100;
	else
		phydev->speed = SPEED_10;

	return 0;
}

/**
 * genphy_read_status - check the link status and update current link state
 * @phydev: target phy_device struct
 *
 * Description: Check the link, then figure out the current state
 *   by comparing what we advertise with what the link partner
 *   advertises.  Start by checking the gigabit possibilities,
 *   then move on to 10/100.
 */
static long genphy_read_status(struct phy_device *phydev)
{
	long err, old_link = phydev->link;

	/* Update the link, but return if there was an error */
	err = genphy_update_link(phydev);
	if (err)
		return err;

	/* why bother the PHY if nothing can have changed */
	if (phydev->autoneg == AUTONEG_ENABLE && old_link && phydev->link)
		return 0;

	phydev->speed = SPEED_UNKNOWN;
	phydev->duplex = DUPLEX_UNKNOWN;

	err = genphy_read_lpa(phydev);
	if (err < 0)
		return err;

	if (phydev->autoneg == AUTONEG_ENABLE && phydev->autoneg_complete) {
		phy_resolve_aneg_linkmode(phydev);
	} else if (phydev->autoneg == AUTONEG_DISABLE) {
		err = genphy_read_status_fixed(phydev);
		if (err < 0)
			return err;
	}

	return 0;
}

static long genphy_suspend(struct phy_device *phydev)
{
	return phy_set_bits(phydev, MII_BMCR, BMCR_PDOWN);
}

static long genphy_resume(struct phy_device *phydev)
{
	return phy_clear_bits(phydev, MII_BMCR, BMCR_PDOWN);
}

static const char *phy_speed_to_str(long speed)
{
	switch (speed) {
	case SPEED_10:
		return "10Mbps";
	case SPEED_100:
		return "100Mbps";
	case SPEED_UNKNOWN:
		return "Unknown";
	default:
		return "Unsupported (update phy-core.c)";
	}
}

static const char *phy_duplex_to_str(long duplex)
{
	if (duplex == DUPLEX_HALF)
		return "Half";
	if (duplex == DUPLEX_FULL)
		return "Full";
	if (duplex == DUPLEX_UNKNOWN)
		return "Unknown";
	return "Unsupported (update phy-core.c)";
}

/**
 * phy_print_status - Convenience function to print out the current phy status
 * @phydev: the phy_device struct
 *
 * netdev_info() goes to the kernel's alert channel. This is only ever
 * reached from process or root timeout context.
 */
void phy_print_status(struct phy_device *phydev)
{
	struct netif *dev = phydev->attached_dev;

	if (phydev->link) {
		ALERT(("%s%d: Link is Up - %s/%s", dev->name, dev->unit,
			phy_speed_to_str(phydev->speed),
			phy_duplex_to_str(phydev->duplex)));
	} else	{
		ALERT(("%s%d: Link is Down", dev->name, dev->unit));
	}
}

/*
 * phy_link_change(): netif_carrier_on/off() have no counterpart in
 * MiNTNet, only the MAC driver's adjust_link() is called.
 */
static void phy_link_change(struct phy_device *phydev, long up)
{
	UNUSED(up);

	phydev->adjust_link(phydev->attached_dev);
}

static void phy_link_up(struct phy_device *phydev)
{
	phy_link_change(phydev, 1);
}

static void phy_link_down(struct phy_device *phydev)
{
	phy_link_change(phydev, 0);
}

/**
 * phy_check_link_status - check link status and set state accordingly
 * @phydev: the phy_device struct
 *
 * Description: Check for link and whether autoneg was triggered / is running
 * and set state accordingly
 */
static long phy_check_link_status(struct phy_device *phydev)
{
	long err;

	err = genphy_read_status(phydev);
	if (err)
		return err;

	if (phydev->link && phydev->state != PHY_RUNNING) {
		phydev->state = PHY_RUNNING;
		phy_link_up(phydev);
	} else if (!phydev->link && phydev->state != PHY_NOLINK) {
		phydev->state = PHY_NOLINK;
		phy_link_down(phydev);
	}

	return 0;
}

/**
 * phy_sanitize_settings - make sure the PHY is set to supported speed and duplex
 * @phydev: the target phy_device struct
 *
 * Description: Make sure the PHY is set to supported speeds and
 *   duplexes.  Drop down by one in this order:  1000/FULL,
 *   1000/HALF, 100/FULL, 100/HALF, 10/FULL, 10/HALF.
 */
static void phy_sanitize_settings(struct phy_device *phydev)
{
	if (phydev->supported & ADVERTISE_100FULL) {
		phydev->speed = SPEED_100;
		phydev->duplex = DUPLEX_FULL;
	} else if (phydev->supported & ADVERTISE_100HALF) {
		phydev->speed = SPEED_100;
		phydev->duplex = DUPLEX_HALF;
	} else if (phydev->supported & ADVERTISE_10FULL) {
		phydev->speed = SPEED_10;
		phydev->duplex = DUPLEX_FULL;
	} else {
		phydev->speed = SPEED_10;
		phydev->duplex = DUPLEX_HALF;
	}
}

/**
 * _phy_start_aneg - start auto-negotiation for this PHY device
 * @phydev: the phy_device struct
 *
 * Description: Sanitizes the settings (if we're not autonegotiating
 *   them), and then calls the driver's config_aneg function.
 *   If the PHYCONTROL Layer is operating, we change the state to
 *   reflect the beginning of Auto-negotiation or forcing.
 */
static long _phy_start_aneg(struct phy_device *phydev)
{
	long err;

	if (AUTONEG_DISABLE == phydev->autoneg)
		phy_sanitize_settings(phydev);

	err = genphy_config_aneg(phydev);
	if (err < 0)
		return err;

	if (phy_is_started(phydev))
		err = phy_check_link_status(phydev);

	return err;
}

/*
 * The PHY_STATE_WORK_SUSPEND leg is folded in: a halted PHY is powered
 * down right here.
 */
static void _phy_state_machine(struct phy_device *phydev)
{
	long needs_aneg = 0, needs_suspend = 0;
	long err = 0;

	switch (phydev->state) {
	case PHY_DOWN:
	case PHY_READY:
		break;
	case PHY_UP:
		needs_aneg = 1;
		break;
	case PHY_NOLINK:
	case PHY_RUNNING:
		err = phy_check_link_status(phydev);
		break;
	case PHY_HALTED:
		if (phydev->link) {
			if (phydev->autoneg == AUTONEG_ENABLE) {
				phydev->speed = SPEED_UNKNOWN;
				phydev->duplex = DUPLEX_UNKNOWN;
			}
			phydev->lp_advertising = 0;
		}
		/* fall through */
	case PHY_ERROR:
		if (phydev->link) {
			phydev->link = 0;
			phy_link_down(phydev);
		}
		needs_suspend = 1;
		break;
	}

	if (needs_aneg)
		err = _phy_start_aneg(phydev);

	if (err < 0) {
		/* phy_error(): give up on this PHY until the next start */
		ALERT(("%s%d: PHY error %ld", phydev->attached_dev->name,
			phydev->attached_dev->unit, err));
		phydev->state = PHY_ERROR;
	}

	if (needs_suspend)
		genphy_suspend(phydev);
}

/**
 * phy_state_machine - Handle the state machine
 * @phydev: the phy_device struct
 */
void phy_state_machine(struct phy_device *phydev)
{
	if (phy_is_started(phydev))
		_phy_state_machine(phydev);
}

/**
 * phy_start - start or restart a PHY device
 * @phydev: target phy_device struct
 *
 * Description: Indicates the attached device's readiness to
 *   handle PHY-related work.  Used during startup to start the
 *   PHY, and after a call to phy_stop() to resume operation.
 *   Also used to indicate the MDIO bus has cleared an error
 *   condition.
 */
void phy_start(struct phy_device *phydev)
{
	if (phydev->state != PHY_READY && phydev->state != PHY_HALTED) {
		ALERT(("phy_start: called from state %d", phydev->state));
		return;
	}

	/* if phy was suspended, bring the physical link up again */
	genphy_resume(phydev);

	phydev->state = PHY_UP;

	/* phy_start_machine() */
	_phy_state_machine(phydev);
}

/**
 * phy_stop - Bring down the PHY link, and stop checking the status
 * @phydev: target phy_device struct
 */
void phy_stop(struct phy_device *phydev)
{
	if (!phy_is_started(phydev) && phydev->state != PHY_ERROR) {
		ALERT(("phy_stop: called from state %d", phydev->state));
		return;
	}

	phydev->state = PHY_HALTED;

	_phy_state_machine(phydev);
}

/**
 * phy_set_max_speed - Set the maximum speed the PHY should support
 *
 * @phydev: The phy_device struct
 * @max_speed: Maximum speed
 *
 * The PHY might be more capable than the MAC. For example a Fast Ethernet
 * is connected to a 1G PHY. This function allows the MAC to indicate its
 * maximum speed, and so limit what the PHY will advertise.
 *
 * The advertisement is 10/100 only here, so there is nothing above
 * SPEED_100 to drop.
 */
void phy_set_max_speed(struct phy_device *phydev, long max_speed)
{
	UNUSED(phydev);
	UNUSED(max_speed);
}

/**
 * phy_connect_direct - connect an ethernet device to a specific phy_device
 * @dev: the network device to connect
 * @phydev: the pointer to the phy device
 * @handler: callback function for state change notifications
 *
 * The generic PHY driver has no soft_reset, config_init or config_intr,
 * so phy_init_hw() amounts to nothing; phy_probe() reads the abilities
 * and advertises them all.
 */
long phy_connect_direct(struct netif *dev, struct phy_device *phydev,
			void (*handler)(struct netif *))
{
	long err;

	phydev->attached_dev = dev;
	phydev->adjust_link = handler;

	err = genphy_read_abilities(phydev);
	if (err < 0)
		return err;

	/* phy_advertise_supported() */
	phydev->advertising = phydev->supported;

	phydev->link = 0;
	phydev->autoneg_complete = 0;
	phydev->speed = SPEED_UNKNOWN;
	phydev->duplex = DUPLEX_UNKNOWN;
	phydev->lp_advertising = 0;
	phydev->state = PHY_READY;

	return 0;
}
