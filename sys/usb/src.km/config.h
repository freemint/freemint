/*
 * FreeMiNT USB subsystem by David Galvez. 2010 - 2011
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _CONFIG_H
#define _CONFIG_H

//#define ARCH m68k
//#define COLDFIRE      /* Besides change one(first) .chip in bios.S 68060 or 5200 */
/* Change .chip in detxbios.S 68060 or 5200 */

/*----- USB -----*/
//#define CONFIG_LEGACY_USB_INIT_SEQ
//#define CONFIG_USB_INTERRUPT_POLLING
/*----- ISP116x-HCD ------*/
#define ISP116X_HCD_USE_UDELAY
//#define ISP116X_HCD_USE_EXTRA_DELAY
//#define ISP116X_HCD_SEL15kRES
//#define ISP116X_HCD_OC_ENABLE
//#define ISP116X_HCD_REMOTE_WAKEUP_ENABLE
//#define ISP116X_HCD_INT_EDGE_TRIGGERED
#define ISP116X_HCD_INT_ACT_HIGH
/*----- OHCI-HCI -----*/
#define CONFIG_SYS_USB_OHCI_MAX_ROOT_PORTS	1
//#define PCI_XBIOS

# endif /* _CONFIG_H */
