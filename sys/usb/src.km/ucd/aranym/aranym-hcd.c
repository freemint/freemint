/*
 * Aranym USB (virtual) Controller Driver.
 *
 * Copyright (c) 2012-2014 David Galvez.
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
 */

#include "mint/mint.h"
#include "mint/dcntl.h"
#include "mint/arch/nf_ops.h"
#include "libkern/libkern.h"

#include "aranym.h"
#include "usbhost_nfapi.h"

#include "../../usb.h"
#include "../../usb_api.h"


#define VER_MAJOR	0
#define VER_MINOR	2
#define VER_STATUS	

#define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS) 
#define MSG_BUILDDATE	__DATE__

#define MSG_BOOT	\
	"\033pAranym USB controller driver " MSG_VERSION " \033q\r\n"

#define MSG_GREET	\
	"(c) 2012-2014 by David Galvez.\r\n" \
	"Compiled " MSG_BUILDDATE ".\r\n\r\n"

/*--- Debug section ---*/

#if 0
# define DEV_DEBUG	1
#endif

#ifdef DEV_DEBUG

# define FORCE(x)	KERNEL_FORCE x
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	KERNEL_DEBUG x
# define TRACE(x)	KERNEL_TRACE x
# define ASSERT(x)	assert x

#else

# define FORCE(x)	KERNEL_FORCE x
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	
# define TRACE(x)	
# define ASSERT(x)	assert x

#endif


/*--- Global variables ---*/

static const char hcd_name[] = "aranym-hcd";
static char lname[] = "Aranym USB driver\0";

/* BEGIN kernel interface */

struct kentry	*kentry;
struct usb_module_api *api;

/* END kernel interface */

static struct usb_device *root_hub_dev = NULL;
unsigned long rh_port_status[NUMBER_OF_PORTS]; 


/*--- Function prototypes ---*/

/*--- Interrupt ---*/

/* old handler */
extern void (*old_interrupt)(void);

/* interrupt wrapper routine */
void my_interrupt (void);

/* interrupt handling - bottom half */
void _cdecl 	nfusb_interrupt	(void);

long		submit_bulk_msg		(struct usb_device *, unsigned long , void *, long, long, unsigned long);
long		submit_control_msg	(struct usb_device *, unsigned long, void *,
					 long, struct devrequest *);
long		submit_int_msg		(struct usb_device *, unsigned long, void *, long, long);

long _cdecl	init			(struct kentry *, struct usb_module_api *, char **);

/* USB controller interface */
static long _cdecl	aranym_open		(struct ucdif *);
static long _cdecl	aranym_close		(struct ucdif *);
static long _cdecl	aranym_ioctl		(struct ucdif *, short, long);


static struct ucdif aranym_uif = 
{
	0,			/* *next */
	USB_API_VERSION,	/* API */
	USB_CONTRLL,		/* class */
	lname,			/* lname */
	"aranym",		/* name */
	0,			/* unit */
	0,			/* flags */
	aranym_open,		/* open */
	aranym_close,		/* close */
	0,			/* resrvd1 */
	aranym_ioctl,		/* ioctl */
	0,			/* resrvd2 */
};

/* ================================================================ */
static unsigned long nfUsbHostID;
long __CDECL (*nf_call)(long id, ...) = 0UL;
/* ================================================================ */


/*--- Functions ---*/

static inline unsigned long
get_nfapi_version()
{
	return nf_call(USBHOST(GET_VERSION));
}


static inline unsigned long
get_int_level()
{
	return nf_call(USBHOST(USBHOST_INTLEVEL));
}


static inline unsigned long
get_rh_port_status(unsigned long *rhportstatus)
{
	return nf_call(USBHOST(USBHOST_RH_PORT_STATUS), rh_port_status);
}


/* --- Transfer functions -------------------------------------------------- */

long
submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		   long len, long interval)
{
	DEBUG(("dev=0x%lx pipe=%lx buf=0x%lx size=%d int=%d",
	    dev, pipe, buffer, len, interval));
	
	long r;

	r = nf_call(USBHOST(USBHOST_SUBMIT_INT_MSG), pipe, buffer, len, interval);

	if(r >= 0) {
		dev->status = 0;
		dev->act_len = r;
		return 0;
	}

	return -1;
}


long
submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		       long len, struct devrequest *setup)
{
	long r;

	r = nf_call(USBHOST(USBHOST_SUBMIT_CONTROL_MSG), pipe, buffer, len, setup);

	if(r >= 0) {
		dev->status = 0;
		dev->act_len = r;
		return 0;
	}

	return -1;
}


long
submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		    long len, long flags, unsigned long timeout)
{
	long r;

	r = nf_call(USBHOST(USBHOST_SUBMIT_BULK_MSG), pipe, buffer, len, flags, timeout);

	if(r >= 0) {
		dev->status = 0;
		dev->act_len = r;
		return 0;
	}

	return -1;
}


/* --- Interrupt functions ----------------------------------------------------- */

static void
int_handle_tophalf (PROC *process, long arg)
{
	unsigned char i;

	get_rh_port_status(rh_port_status);

	for (i = 0; i < NUMBER_OF_PORTS; i++)
	{
		if (rh_port_status[i] & RH_PS_CSC)
		{
			usb_rh_wakeup(&aranym_uif);
		}
	}
}


void _cdecl
nfusb_interrupt(void)
{	
	addroottimeout (0L, int_handle_tophalf, 0x1);
}


/* --- Interface functions -------------------------------------------------- */

static long _cdecl
aranym_open (struct ucdif *u)
{
	return E_OK;
}


static long _cdecl
aranym_close (struct ucdif *u)
{
	return E_OK;
}


static long _cdecl
aranym_ioctl (struct ucdif *u, short cmd, long arg)
{
	long ret = E_OK;

	switch (cmd)
	{
		case FS_INFO:
		{
			*(long *)arg = (((long)VER_MAJOR << 16) | VER_MINOR);
			break;
		}
		case LOWLEVEL_INIT :
		{
			ret = usb_lowlevel_init (u->ucd_priv);
			break;
		}
		case LOWLEVEL_STOP :
		{
			ret = usb_lowlevel_stop (u->ucd_priv);
			break;
		}
		case SUBMIT_CONTROL_MSG :
		{
			struct control_msg *ctrl_msg = (struct control_msg *)arg;
			
			ret = submit_control_msg (ctrl_msg->dev, ctrl_msg->pipe,
			 		    ctrl_msg->data, ctrl_msg->size, ctrl_msg->setup);	
			break;
		}
		case SUBMIT_BULK_MSG :
		{
			struct bulk_msg *bulk_msg = (struct bulk_msg *)arg;

			ret = submit_bulk_msg (bulk_msg->dev, bulk_msg->pipe,
				         bulk_msg->data, bulk_msg->len, bulk_msg->flags, bulk_msg->timeout);

			break;
		}
		case SUBMIT_INT_MSG :
		{
			struct int_msg *int_msg = (struct int_msg *)arg;

			ret = submit_int_msg(int_msg->dev, int_msg->pipe,
				       int_msg->buffer, int_msg->transfer_len, 
				       int_msg->interval);

			break;
		}
		default:
		{
			return ENOSYS;
		}
	}	
	return ret;
}


/* --- Init functions ------------------------------------------------------ */

long 
usb_lowlevel_init(void *dummy)
{
	int r;

	r = nf_call(USBHOST(USBHOST_LOWLEVEL_INIT));

	if (!r) 
		(void) c_conws("Aranym USB Controller Driver init \r\n");
	else
		(void) c_conws("Couldn't init aranym host chip emulator \r\n");

	return 0;
}


long 
usb_lowlevel_stop(void *dummy)
{
	int r;

	r = nf_call(USBHOST(USBHOST_LOWLEVEL_STOP));

	return r;
}


/* Entry function */
long _cdecl
init (struct kentry *k, struct usb_module_api *uapi, char **reason)
{
	long ret;
	char message[100];

	kentry	= k;

	/* get the USBHost NatFeat ID */
	nfUsbHostID = 0;

	if (nf_ops != NULL)
		nfUsbHostID = nf_ops->get_id("USBHOST");

	if ( nfUsbHostID == 0 ) {
		sprintf(message, 100 * sizeof(char), "%s not installed - NatFeat not found\n\r", lname);
		c_conws(message);
		return 1;
	}

	/* safe the nf_call pointer */
	nf_call = nf_ops->call;

	/* compare the version */
	if ( get_nfapi_version() != USBHOST_NFAPI_VERSION ) {
		sprintf(message, 100 * sizeof(char), "%s not installed - version mismatch: %ld != %d\n\r", hcd_name, get_nfapi_version(), USBHOST_NFAPI_VERSION);
		c_conws(message);
		return 1;
	}


	api	= uapi;

	if (check_kentry_version())
		return -1;

	c_conws (MSG_BOOT);
	c_conws (MSG_GREET);
	DEBUG (("%s: enter init", __FILE__));

	ret = ucd_register(&aranym_uif, &root_hub_dev);
	if (ret)
	{
		DEBUG (("%s: ucd register failed!", __FILE__));
		return 1;
	}

	DEBUG (("%s: ucd register ok", __FILE__));

	/* Set handler and interrupt for Root Hub Status Change */
# define vector(x)      (x / 4)
	old_interrupt = (void(*)()) b_setexc(vector(0x60) + get_int_level(), (long) my_interrupt);
	
	return 0;
}
