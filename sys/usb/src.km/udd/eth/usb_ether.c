/*
 * Modified for the FreeMiNT USB subsystem by Alan Hourihane 2014.
 *
 * Copyright (c) 2011 The Chromium OS Authors.
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "cookie.h"
#include "usbnet.h"
#include "libkern/libkern.h"
#include "mint/dcntl.h"

#include "../../config.h"
#include "../../usb.h"
#include "../udd_defs.h"
#include "../../usb_api.h"


typedef struct
{
        unsigned long  name;
        unsigned long  val;
} COOKIE;

#define COOKIEBASE (*(COOKIE **)0x5a0)

/*
 * Debug section
 */

#if 0
# define DEV_DEBUG	1
#endif

#ifdef DEV_DEBUG
# define FORCE(x)	
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	KERNEL_DEBUG x
# define TRACE(x)	KERNEL_TRACE x
# define ASSERT(x)	assert x
#else
# define FORCE(x)	
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	
# define TRACE(x)	
# define ASSERT(x)	assert x
#endif

#define VER_MAJOR       0
#define VER_MINOR       1
#define VER_STATUS      

#define MSG_VERSION     str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS) 
#define MSG_BUILDDATE   __DATE__

#define MSG_BOOT        \
        "\033p USB ethernet class driver " MSG_VERSION " \033q\r\n"

#define MSG_GREET       \
        "Ported, mixed and shaken by Alan Hourihane.\r\n" \
        "Compiled " MSG_BUILDDATE ".\r\n\r\n"

#define MSG_FAILURE     \
        "\7\r\nSorry, failed!\r\n\r\n"

char *drv_version = MSG_VERSION;


/*
 * USB device interface
 */

static long _cdecl	eth_open		(struct uddif *);
static long _cdecl	eth_close		(struct uddif *);
static long _cdecl	eth_ioctl		(struct uddif *, short, long);

static char lname[] = "USB ethernet class driver\0";

static struct uddif eth_uif = 
{
	0,			/* *next */
	USB_DEVICE,		/* class */
	lname,			/* lname */
	"eth",			/* name */
	0,			/* unit */
	0,			/* flags */
	eth_open,		/* open */
	eth_close,		/* close */
	0,			/* resrvd1 */
	eth_ioctl,		/* ioctl */
	0,			/* resrvd2 */
};

/****************************************************************************/
/* BEGIN kernel interface */

struct kentry   *kentry;
struct usb_module_api   *api;
struct usb_netapi *usbNetAPI = NULL;
long _cdecl init (struct kentry *, struct usb_module_api *, long, long);

/* END kernel interface */
/****************************************************************************/

void usb_ethernet_disconnect(struct usb_device *dev);
long usb_ethernet_probe(struct usb_device *dev);

#include "usb_ether.h"

static struct ueth_data *usb_eth;

/*
 * Given a USB device, ask each driver if it can support it, and attach it
 * to the first driver that says 'yes'
 */
static long probe_valid_drivers(struct usb_device *dev)
{
	long j, devid;
	long numDevices = usbNetAPI->numDevices;

	for (j = 0; j < numDevices; j++) {
		if (usb_eth[j].pusb_dev == NULL)
			break;
	}

	devid = j;

	for (j = 0; j < numDevices; j++) {
		if (!usbNetAPI->usbnet[j].before_probe ||
		    !usbNetAPI->usbnet[j].probe ||
		    !usbNetAPI->usbnet[j].get_info)
			continue;

		usbNetAPI->usbnet[j].before_probe(api);

		if (!usbNetAPI->usbnet[j].probe(dev, 0, &usb_eth[devid]))
			continue;
		/*
		 * ok, it is a supported eth device. Get info and fill it in
		 */
		if (usbNetAPI->usbnet[j].get_info(dev, &usb_eth[devid], &usb_eth[devid].eth_dev)) {
			return 0;
		}
	}

	return -1;
}

static struct usb_driver eth_driver =
{
	name:           "usb-eth",
	probe:          usb_ethernet_probe,
	disconnect:     usb_ethernet_disconnect,
	chain:          {NULL, NULL}
};

static long _cdecl
eth_open (struct uddif *u)
{
	return E_OK;
}

static long _cdecl
eth_close (struct uddif *u)
{
	return E_OK;
}

static long _cdecl
eth_ioctl (struct uddif *u, short cmd, long arg)
{
	return E_OK;
}

long
usb_ethernet_probe(struct usb_device *dev)
{
	int old_async;
	long r;

	if (dev == NULL)
		return -1;

	old_async = usb_disable_asynch(1); /* asynch transfer not allowed */

	/* find valid usb_ether driver for this device, if any */
	r = probe_valid_drivers(dev);

	usb_disable_asynch(old_async); /* restore asynch value */

	DEBUG(("Ethernet Device(s) found"));

	return r;
}

void
usb_ethernet_disconnect(struct usb_device *dev)
{
	long i;

	ALERT(("USB Ethernet Device disconnected: (%ld) %s", dev->devnum, dev->prod));

	for (i = 0; i < usbNetAPI->numDevices; i++)
	{
		if (usb_eth[i].pusb_dev == dev) {
			usb_eth[i].pusb_dev = NULL;
			break;
		};
	}
}

long _cdecl
init (struct kentry *k, struct usb_module_api *uapi, long arg, long reason)
{
        COOKIE *cookie = COOKIEBASE;
	long ret;

	kentry	= k;
	api	= uapi;

	if (check_kentry_version())
		return -1;

	c_conws (MSG_BOOT);
	c_conws (MSG_GREET);
	DEBUG (("%s: enter init", __FILE__));

	/*
	 * Find EUSB cookie.
	 */
        if (cookie)
        {
                while (cookie->name)
                {
                        if (cookie->name == COOKIE_EUSB) {
                                usbNetAPI = (struct usb_netapi *)cookie->val;
				break;
                        }
                        cookie++;
                }
        }

	if (!usbNetAPI) {
		c_conws (MSG_FAILURE);
		return 1;
	}

	usb_eth = kmalloc(sizeof(struct ueth_data) * usbNetAPI->numDevices);
	if (!usb_eth) {
		c_conws (MSG_FAILURE);
		return 1;
	}

	memset(usb_eth, 0, sizeof(usb_eth));

	ret = udd_register(&eth_uif, &eth_driver);
	if (ret)
	{
		c_conws (MSG_FAILURE);
		kfree(usb_eth);
		DEBUG (("%s: udd register failed!", __FILE__));
		return 1;
	}


	DEBUG (("%s: udd register ok", __FILE__));
	
	return 0;
}

