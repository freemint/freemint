/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
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
 * 
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2001-03-01
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_sockio_h
# define _mint_sockio_h


/*
 * Socket ioctls
 */

/* socket-level I/O control calls */
# define SIOCGLOWAT	(('S' << 8) | 1)	/* get low watermark */
# define SIOCSLOWAT	(('S' << 8) | 2)	/* set low watermark */
# define SIOCGHIWAT	(('S' << 8) | 3)	/* get high watermark */
# define SIOCSHIWAT	(('S' << 8) | 4)	/* set high watermark */
# define SIOCSPGRP	(('S' << 8) | 5)	/* set process group */
# define SIOCGPGRP	(('S' << 8) | 6)	/* get process group */
# define SIOCATMARK	(('S' << 8) | 7)	/* at oob mark? */

/* socket configuration controls */
# define SIOCGIFNAME	(('S' << 8) | 10)	/* get iface name */
# define SIOCSIFLINK	(('S' << 8) | 11)	/* connect iface to device */
# define SIOCGIFCONF	(('S' << 8) | 12)	/* get iface list */
# define SIOCGIFFLAGS	(('S' << 8) | 13)	/* get flags */
# define SIOCSIFFLAGS	(('S' << 8) | 14)	/* set flags */
# define SIOCGIFADDR	(('S' << 8) | 15)	/* get iface address */
# define SIOCSIFADDR	(('S' << 8) | 16)	/* set iface address */
# define SIOCGIFDSTADDR	(('S' << 8) | 17)	/* get iface remote address */
# define SIOCSIFDSTADDR	(('S' << 8) | 18)	/* set iface remote address */
# define SIOCGIFBRDADDR	(('S' << 8) | 19)	/* get iface broadcast address */
# define SIOCSIFBRDADDR	(('S' << 8) | 20)	/* set iface broadcast address */
# define SIOCGIFNETMASK	(('S' << 8) | 21)	/* get iface network mask */
# define SIOCSIFNETMASK	(('S' << 8) | 22)	/* set iface network mask */
# define SIOCGIFMETRIC	(('S' << 8) | 23)	/* get metric */
# define SIOCSIFMETRIC	(('S' << 8) | 24)	/* set metric */
# define SIOCSLNKFLAGS	(('S' << 8) | 25)	/* set link level flags */
# define SIOCGLNKFLAGS	(('S' << 8) | 26)	/* get link level flags */
# define SIOCGIFMTU	(('S' << 8) | 27)	/* get MTU size */
# define SIOCSIFMTU	(('S' << 8) | 28)	/* set MTU size */
# define SIOCGIFSTATS	(('S' << 8) | 29)	/* get interface statistics */
# define SIOCADDRT	(('S' << 8) | 30)	/* add routing table entry */
# define SIOCDELRT	(('S' << 8) | 31)	/* delete routing table entry */

# define SIOCGIFNAME_ETH	(('S' << 8) | 32)	/* return the name of the interface */
# define SIOCGIFINDEX	(('S' << 8) | 33)	/* retrieve the interface index */

# define SIOCDARP	(('S' << 8) | 40)	/* delete ARP table entry */
# define SIOCGARP	(('S' << 8) | 41)	/* get ARP table entry */
# define SIOCSARP	(('S' << 8) | 42)	/* set ARP table entry */

# define SIOCSIFHWADDR	(('S' << 8) | 49)	/* set hardware address */
# define SIOCGIFHWADDR	(('S' << 8) | 50)	/* get hardware address */
# define SIOCGLNKSTATS	(('S' << 8) | 51)	/* get link statistics */
# define SIOCSIFOPT	(('S' << 8) | 52)	/* set interface option */


# endif /* _mint_sockio_h */
