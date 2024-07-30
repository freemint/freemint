/*
 * USB DATA driver
 *
 * Copyright (C) 2023 by Claude Labelle
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See /COPYING.GPL for details.
 *
 */
#include "../../global.h"

#include "../../usb.h"
#include "../../usb_api.h"

#ifdef TOSONLY
#define MSG_VERSION "TOS DRIVERS"
#else
#define MSG_VERSION "FreeMiNT DRIVERS"
#endif

#define MSG_BUILDDATE	__DATE__

#ifdef TOSONLY
#define MSG_BOOT	\
		"\r\n\033p USB DATA class driver " MSG_VERSION " \033q\r\n"
#define MSG_GREET	\
		"by Claude Labelle.\r\n" \
		"Compiled " MSG_BUILDDATE ".\r\n"
#else
#define MSG_BOOT	\
		"\033p USB DATA class driver " MSG_VERSION " \033q\r\n"
#define MSG_GREET	\
		"by Claude Labelle.\r\n" \
		"Compiled " MSG_BUILDDATE ".\r\n\r\n"
#endif

//#define TEST
//#define DEBUGTOS

/****************************************************************************/
/*
 * BEGIN kernel interface
 */

#ifndef TOSONLY
struct kentry *kentry;
#else
extern unsigned long _PgmSize;
int isHddriverModule(void); /* in entry.S */
#endif

struct usb_module_api *api;

#define BCOSTAT0 ((volatile unsigned long *)0x55eL)
#define BCONOUT0 ((volatile unsigned long *)0x57eL)

/*
 * END kernel interface
 */
/****************************************************************************/

/*
 * USB device interface
 */

static long data_ioctl (struct uddif *, short, long);
static long data_disconnect (struct usb_device *dev);
static long data_probe (struct usb_device *dev, unsigned int ifnum);

static char lname[] = "USB DATA class driver\0";

static struct uddif data_uif = {
	0,                          /* *next */
	USB_API_VERSION,            /* API */
	USB_DEVICE,                 /* class */
	lname,                      /* lname */
	"data",                    /* name */
	0,                          /* unit */
	0,                          /* flags */
	data_probe,                /* probe */
	data_disconnect,           /* disconnect */
	0,                          /* resrvd1 */
	data_ioctl,                /* ioctl */
	0,                          /* resrvd2 */
};

struct data_data
{
	struct usb_device *pusb_dev;	/* this usb_device */
	unsigned int if_no;				/* interface number */
	unsigned char ep_out;			/* endpoint out */
	unsigned long bulk_out_pipe;	/* pipe for sending data to data */
	unsigned char max_packet_size;	/* max packet for pipe */
	unsigned char protocol;
};

static struct data_data data_data;

/*
 * --- Interface functions
 * --------------------------------------------------
 */

static long _cdecl
data_ioctl (struct uddif *u, short cmd, long arg)
{
	return E_OK;
}

/*******************************************************************************
 *
 *
 */
static long
data_disconnect (struct usb_device *dev)
{
	if (dev == data_data.pusb_dev)
	{
		data_data.pusb_dev = NULL;
	}

	return 0;
}

/*******************************************************************************
 *
 *
 */

static long
data_probe (struct usb_device *dev, unsigned int ifnum)
{
	struct usb_interface *iface;
	struct usb_endpoint_descriptor *ep_desc;
	short i;

	/*
	 * Only one data at a time
	 */
	if (data_data.pusb_dev)
	{
		return -1;
	}

	if (dev == NULL)
	{
		return -1;
	}

	/*
	 * let's examine the device now
	 */
#ifdef DEBUGTOS
	c_conws ("\r\ndev device class should be 0: ");
	hex_byte(dev->descriptor.bDeviceClass);
#endif

	iface = &dev->config.if_desc[ifnum];
	if (!iface)
	{
		return -1;
	}

	// for debugging with printer; replace printer driver.
	//if (iface->desc.bInterfaceClass != USB_CLASS_PRINTER)
	if (iface->desc.bInterfaceClass != USB_CLASS_DATA)
	{
		return -1;
	}

	/* Protocol code. Should be 0. */
#ifdef DEBUGTOS
	c_conws ("\r\n protocol: ");
	hex_byte(iface->desc.bInterfaceProtocol);
#endif

	/*
	 Enumerate endpoints. ep_desc is endpoint_descriptor.
	 We just care about the bulk-out endpoint, to which we will write data
	 */
	short found_endpoint = 0;
	for (i = 0; i < iface->desc.bNumEndpoints; i++) {
		ep_desc = &iface->ep_desc[i];
#ifdef DEBUGTOS
		c_conws("\r\n Found Endpoint ");
#endif
		if ((ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK) {
#ifdef DEBUGTOS
			c_conws(" of type bulk ");
#endif
			if (ep_desc->bEndpointAddress & USB_DIR_IN) {
#ifdef DEBUGTOS
				c_conws(" in.");
#endif
			}
			else {
#ifdef DEBUGTOS
				c_conws(" out.");
#endif
				data_data.ep_out = ep_desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
				found_endpoint = 1;
			}
		}else {
#ifdef DEBUGTOS
			c_conws(" of other type.");
#endif
		}
	}
	//Bconin(2);

	if (! found_endpoint)
	{
		return -1;
	}

	data_data.pusb_dev = dev;
	data_data.if_no = ifnum;
	data_data.protocol = iface->desc.bInterfaceProtocol;
	data_data.bulk_out_pipe =
	usb_sndbulkpipe (data_data.pusb_dev, (long) data_data.ep_out);
	data_data.max_packet_size = usb_maxpacket (data_data.pusb_dev, data_data.bulk_out_pipe);
#ifdef DEBUGTOS
	c_conws ("\r\n max packet size: 0x");
	hex_byte(data_data.max_packet_size);
#endif
	return 0;
}

#ifdef TOSONLY
int init (void);
int
init (void)
#else
long _cdecl init_udd (struct kentry *, struct usb_module_api *, long, long);
long _cdecl
init_udd (struct kentry *k, struct usb_module_api *uapi, long arg, long reason)
#endif
{
	long ret;

#ifndef TOSONLY
	kentry = k;
	api = uapi;

	if (check_kentry_version ())
		return -1;
#endif

	c_conws (MSG_BOOT);
	c_conws (MSG_GREET);
	DEBUG (("%s: enter init", __FILE__));

#ifdef TOSONLY
	/*
	 * GET _USB COOKIE to REGISTER
	 */
	if (!getcookie (_USB, (long *)&api))
	{
		(void) Cconws ("data failed to get _USB cookie\r\n");
		return -1;
	}

#endif
	/* setup_data_module_api(); */
	ret = udd_register (&data_uif);

	if (ret)
	{
		DEBUG (("%s: udd register failed!", __FILE__));
		return 1;
	}

	DEBUG (("%s: udd register ok", __FILE__));

#ifdef TOSONLY
	c_conws ("USB data driver installed");
	/* terminate and stay resident */
	if (isHddriverModule()) {
		c_conws(" as HDDRIVER module.\r\n");
		return 0;
	} else {
		c_conws(".\r\n");
		Ptermres(_PgmSize, 0);
	}
#endif

	return 0;
}
