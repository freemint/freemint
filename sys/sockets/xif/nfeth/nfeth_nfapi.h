/*
 * Native Features ethernet driver - header file.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2002-2006 Standa and Petr of ARAnyM dev team.
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

#ifndef _NFETH_NFAPI_H
#define _NFETH_NFAPI_H

/* 
 * This is the API definition file that needs to be synced with the
 * emulator's side one to ensure no version missmatch.
 */

/* API version number */
#define NFETH_NFAPI_VERSION	0x00000005

enum {
	GET_VERSION = 0,	/* no parameters, return NFAPI_VERSION in d0 */
	XIF_INTLEVEL,		/* no parameters, return Interrupt Level in d0 */
	XIF_IRQ,		/* acknowledge interrupt from host */
	XIF_START,		/* (ethX), called on 'ifup', start receiver thread */
	XIF_STOP,		/* (ethX), called on 'ifdown', stop the thread */
	XIF_READLENGTH,		/* (ethX), return size of network data block to read */
	XIF_READBLOCK,		/* (ethX, buffer, size), read block of network data */
	XIF_WRITEBLOCK,		/* (ethX, buffer, size), write block of network data */
	XIF_GET_MAC,		/* (ethX, buffer, size), return MAC HW addr in buffer */
	XIF_GET_IPHOST,		/* (ethX, buffer, size), return IP address of host */
	XIF_GET_IPATARI,	/* (ethX, buffer, size), return IP address of atari */
	XIF_GET_NETMASK		/* (ethX, buffer, size), return IP netmask */
};

#define ETH(a)	(nfEtherID + a)

#endif /* _NFETH_NFAPI_H */
