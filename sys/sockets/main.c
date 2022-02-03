/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000, 2001 Frank Naumann <fnaumann@freemint.de>
 * Copyright 1997, 1998, 1999 Torsten Lang
 * Copyright 1993, 1994, 1995, 1996 Kay Roemer
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
 * begin:	2000-06-28
 * last change:	2000-06-28
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# include "global.h"

# include "buf.h"
# include "inet4/init.h"
# include "usbnet.h"
# include "version.h"
# include "cookie.h"
# include "mint/dcntl.h"
# include "mint/file.h"


# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR)
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p MiNT-Net TCP/IP " MSG_VERSION " PL " str (VER_PL) ", " VER_STATUS " \033q\r\n"

# define MSG_GREET	\
	"\xbd 1993-1996 by Kay Roemer.\r\n" \
	"\xbd 1997-1999 by Torsten Lang.\r\n" \
	"\xbd 2000-2010 by Frank Naumann.\r\n"

# define MSG_ALPHA	\
	"\033p WARNING: This is an unstable version - ALPHA! \033q\7\r\n"

# define MSG_BETA	\
	"\033p WARNING: This is a test version - BETA! \033q\7\r\n"

# define MSG_OLDMINT	\
	"\033pMiNT is not the correct version, this module requires FreeMiNT 1.19!\033q\r\n"

# define MSG_FAILURE	\
	"\7Sorry, module NOT installed!\r\n\r\n"


/*
 * For USB ethernet devices.
 */
#define MAX_USB_ETHERNET_DEVICES 4
static struct usb_netapi usbNetAPI;

static long
set_cookie (ulong tag, ulong val)
{
	ushort n = 0;
	struct cookie *cjar = *CJAR;

	while (cjar->tag)
	{
		n++;
		if (cjar->tag == tag)
		{
			cjar->value = val;
			return E_OK;
		}
		cjar++;
	}

	n++;
	if (n < cjar->value)
	{
		n = cjar->value;
		cjar->tag = tag;
		cjar->value = val;

		cjar++;
		cjar->tag = 0L;
		cjar->value = n;

		return E_OK;
	}

	return ENOMEM;
}

static long usb_eth_register(struct usb_eth_prob_dev *ethdev)
{
	long i;

	for (i = 0; i < usbNetAPI.numDevices; i++) {
		if (!usbNetAPI.usbnet[i].before_probe) {
			usbNetAPI.usbnet[i] = *ethdev;

			return 0;
		}
	}

	return -1;
}

static void usb_eth_deregister(long i)
{
	memset(&usbNetAPI.usbnet[i], 0, sizeof(struct usb_eth_prob_dev));
}

static void (*init_func[])(void) =
{
	inet4_init,
	NULL
};

DEVDRV * init (struct kerinfo *k);

DEVDRV *
init (struct kerinfo *k)
{
	long r;
	
	KERNEL = k;
	
	c_conws (MSG_BOOT);
	c_conws (MSG_GREET);
# ifdef ALPHA
	c_conws (MSG_ALPHA);
# endif
# ifdef BETA
	c_conws (MSG_BETA);
# endif
	c_conws ("\r\n");
	
	if (MINT_MAJOR != 1 || MINT_MINOR != 19 || MINT_KVERSION != 2 || !so_register)
	{
		c_conws (MSG_OLDMINT);
		return NULL;
	}
	
	if (buf_init ())
	{
		c_conws ("Cannot initialize buf allocator\n\r");
		c_conws (MSG_FAILURE);
		
		return NULL;
	}
 
 	/* Set the EUSB cookie so that USB ethernet devices can be probed. */
	memset(&usbNetAPI, 0, sizeof(struct usb_netapi));

	usbNetAPI.majorVersion = 0;
	usbNetAPI.minorVersion = 0;
	usbNetAPI.numDevices = MAX_USB_ETHERNET_DEVICES;
	usbNetAPI.usb_eth_register = usb_eth_register;
	usbNetAPI.usb_eth_deregister = usb_eth_deregister;
	usbNetAPI.usbnet = kmalloc(usbNetAPI.numDevices * sizeof(struct usb_eth_prob_dev));
	if (!usbNetAPI.usbnet) {
		return NULL;
	}
	memset(usbNetAPI.usbnet, 0, usbNetAPI.numDevices * sizeof(struct usb_eth_prob_dev));

	set_cookie (COOKIE_EUSB, (long) &usbNetAPI);

	for (r = 0; init_func[r]; r++)
		(*init_func[r])();
	
	c_conws ("\r\n");
	return (DEVDRV *) 1;
}
