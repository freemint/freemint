#include "../../global.h"

#include "../../usb.h"
#include "../../usb_api.h"

#include <mint/osbind.h>

#ifdef TOSONLY
#define MSG_VERSION "TOS DRIVERS"
#else
#define MSG_VERSION "FreeMiNT DRIVERS"
#endif

#define MSG_BUILDDATE	__DATE__

#define MSG_BOOT	\
	"\033p USB mouse class driver " MSG_VERSION " \033q\r\n"

#define MSG_GREET	\
	"Ported, mixed and shaken by Alan Hourihane.\r\n" \
	"Compiled " MSG_BUILDDATE ".\r\n\r\n"


/****************************************************************************/
/*
 * BEGIN kernel interface 
 */

#ifndef TOSONLY
struct kentry *kentry;

void mouse_poll_thread (void *);
void mouse_poll (PROC * proc, long dummy);
#else
extern unsigned long _PgmSize;

/*
 * old handler 
 */
extern void (*old_ikbd_int) (void);

/*
 * interrupt wrapper routine 
 */
extern void interrupt_ikbd (void);
#endif

struct kbdvbase
{
	long midivec;
	long vkbderr;
	long vmiderr;
	long statvec;
	long mousevec;
	long clockvec;
	long joyvec;
	long midisys;
	long ikbdsys;
	short drvstat;			/* Non-zero if a packet is currently
							 * transmitted. */
};
typedef struct kbdvbase KBDVEC;

KBDVEC *vector;
static char mouse_packet[6];
void _cdecl send_packet (long func, char *buf, char *bufend);
void _cdecl fake_hwint(void);

struct usb_module_api *api;

void mouse_int (void);

/*
 * END kernel interface 
 */
/****************************************************************************/

/*
 * USB device interface
 */

static long mouse_ioctl (struct uddif *, short, long);
static long mouse_disconnect (struct usb_device *dev);
static long mouse_probe (struct usb_device *dev, unsigned int ifnum);

static char lname[] = "USB mouse class driver\0";

static struct uddif mouse_uif = {
	0,                          /* *next */
	USB_API_VERSION,            /* API */
	USB_DEVICE,                 /* class */
	lname,                      /* lname */
	"mouse",                    /* name */
	0,                          /* unit */
	0,                          /* flags */
	mouse_probe,                /* probe */
	mouse_disconnect,           /* disconnect */
	0,                          /* resrvd1 */
	mouse_ioctl,                /* ioctl */
	0,                          /* resrvd2 */
};

struct mse_data
{
	struct usb_device *pusb_dev;        /* this usb_device */
	unsigned char ep_in;        /* in endpoint */
	unsigned char ep_out;       /* out ....... */
	unsigned char ep_int;       /* interrupt . */
	long *irq_handle;            /* for USB int requests */
	unsigned long irqpipe;      /* pipe for release_irq */
	unsigned char irqmaxp;      /* max packed for irq Pipe */
	unsigned char irqinterval;  /* Intervall for IRQ Pipe */
	char data[8];
	char new[8];
};

static struct mse_data mse_data;

/*
 * --- Inteface functions
 * -------------------------------------------------- 
 */

static long _cdecl
mouse_ioctl (struct uddif *u, short cmd, long arg)
{
	return E_OK;
}

/*
 * ------------------------------------------------------------------------- 
 */

/*******************************************************************************
 * 
 * 
 */
static long
mouse_disconnect (struct usb_device *dev)
{
	if (dev == mse_data.pusb_dev)
	{
		mse_data.pusb_dev = NULL;
#ifndef TOSONLY
		wake (WAIT_Q, (long) &mouse_poll_thread);
#endif
	}

	return 0;
}

static long
usb_mouse_irq (struct usb_device *dev)
{
	return 0;
}

void
mouse_int (void)
{
	long i, change = 0;
	long actlen = 0;
	long r;

	if (mse_data.pusb_dev == NULL)
		return;

#if 0
	usb_submit_int_msg (mse_data.pusb_dev,
						mse_data.irqpipe,
						mse_data.data,
						mse_data.irqmaxp > 8 ? 8 : mse_data.irqmaxp,
						USB_CNTL_TIMEOUT * 5);
#else
	r = usb_bulk_msg (mse_data.pusb_dev,
					  mse_data.irqpipe,
					  mse_data.new,
					  mse_data.irqmaxp > 8 ? 8 : mse_data.irqmaxp,
					  &actlen, USB_CNTL_TIMEOUT * 5, 1);

	if ((r != 0) || (actlen < 3) || (actlen > 8))
	{
		return;
	}
	for (i = 0; i < actlen; i++)
	{
		if (mse_data.new[i] != mse_data.data[i])
		{
			change = 1;
			break;
		}
	}
	if (change)
	{
		char wheel = 0, buttons, old_buttons;

		(void) wheel;
		(void) buttons;
		(void) old_buttons;
		if ((actlen >= 6) && (mse_data.new[0] == 1))
		{					   /* report-ID */
			buttons = mse_data.new[1];
			old_buttons = mse_data.data[1];
			mouse_packet[0] =
				((mse_data.new[1] & 1) << 1) +
				((mse_data.new[1] & 2) >> 1) + 0xF8;
			mouse_packet[1] = mse_data.new[2];
			mouse_packet[2] = mse_data.new[3];
			wheel = mse_data.new[4];
		}
		else
		{					   /* boot report */

			buttons = mse_data.new[0];
			old_buttons = mse_data.data[0];
			mouse_packet[0] =
				((mse_data.new[0] & 1) << 1) +
				((mse_data.new[0] & 2) >> 1) + 0xF8;
			mouse_packet[1] = mse_data.new[1];
			mouse_packet[2] = mse_data.new[2];
			if (actlen >= 3)
				wheel = mse_data.new[3];
		}
#ifdef EIFFELMODE
		if ((buttons ^ old_buttons) & 4)
		{					   /* 3rd button */
			if (buttons & 4)
			{
				usb_kbd_send_code (0x72);	   /* ENTER */
				usb_kbd_send_code (0xF2);
			}
		}

		if (wheel != 0)
		{					   /* actually like Eiffel */
#define REPEAT_WHEEL 3
			int i;

			if (wheel > 0)
			{
				for (i = 0; i < REPEAT_WHEEL; i++)
				{
					usb_kbd_send_code (0x48);   /* UP */
					usb_kbd_send_code (0xC8);
				}
			}
			else
			{
				for (i = 0; i < REPEAT_WHEEL; i++)
				{
					usb_kbd_send_code (0x50);   /* DOWN */
					usb_kbd_send_code (0xD0);
				}
			}
		}
#endif
		fake_hwint();
		send_packet (vector->mousevec, mouse_packet, mouse_packet + 3);
		mse_data.data[0] = mse_data.new[0];
		mse_data.data[1] = mse_data.new[1];
		mse_data.data[2] = mse_data.new[2];
		mse_data.data[3] = mse_data.new[3];
		mse_data.data[4] = mse_data.new[4];
		mse_data.data[5] = mse_data.new[5];
	}
#endif
}

#ifndef TOSONLY
void
mouse_poll (PROC * proc, long dummy)
{
	wake (WAIT_Q, (long) &mouse_poll_thread);
}

void
mouse_poll_thread (void *dummy)
{
	p_setpriority(0,0,-20);

	while (mse_data.pusb_dev)
	{
		mouse_int ();
		addroottimeout (20, mouse_poll, 0);
		sleep (WAIT_Q, (long) &mouse_poll_thread);
	}

	kthread_exit (0);
}
#endif

/*******************************************************************************
 * 
 * 
 */
static long
mouse_probe (struct usb_device *dev, unsigned int ifnum)
{
	struct usb_interface *iface;
	struct usb_endpoint_descriptor *ep_desc;


	/*
	 * Only one mouse at time 
	 */
	if (mse_data.pusb_dev)
	{
		return -1;
	}

	if (dev == NULL)
	{
		return -1;
	}

	usb_disable_asynch (1);     /* asynch transfer not allowed */

	/*
	 * let's examine the device now 
	 */
	iface = &dev->config.if_desc[ifnum];
	if (!iface)
	{
		return -1;
	}

	if (iface->desc.bInterfaceClass != USB_CLASS_HID)
	{
		return -1;
	}

	if (iface->desc.bInterfaceSubClass != USB_SUB_HID_BOOT)
	{
		return -1;
	}

	if (iface->desc.bInterfaceProtocol != 2)
	{
		return -1;
	}

	if (iface->desc.bNumEndpoints != 1)
	{
		return -1;
	}

	ep_desc = &iface->ep_desc[0];
	if (!ep_desc)
	{
		return -1;
	}

	if ((ep_desc->bmAttributes &
		 USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT)
	{
		mse_data.ep_int =
			ep_desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
		mse_data.irqinterval = ep_desc->bInterval;
	}
	else
	{
		return -1;
	}

	mse_data.pusb_dev = dev;

	mse_data.irqinterval =
		(mse_data.irqinterval > 0) ? mse_data.irqinterval : 255;
	mse_data.irqpipe =
		usb_rcvintpipe (mse_data.pusb_dev, (long) mse_data.ep_int);
	mse_data.irqmaxp = usb_maxpacket (dev, mse_data.irqpipe);
	dev->irq_handle = usb_mouse_irq;
	memset (mse_data.data, 0, 8);
	memset (mse_data.new, 0, 8);

	// if(mse_data.irqmaxp < 6)
	// usb_set_protocol(dev, iface->desc.bInterfaceNumber, 0); /* boot */
	// else
	usb_set_protocol (dev, iface->desc.bInterfaceNumber, 1);    /* report */

	usb_set_idle (dev, iface->desc.bInterfaceNumber, 0, 0);     /* report
                                                                 * infinite 
                                                                 */

#ifndef TOSONLY
	long r = kthread_create (get_curproc (), mouse_poll_thread, NULL, NULL,
							 "mousepoll");

	if (r)
	{
		return 0;
	}
#endif

	return 0;
}

#ifdef TOSONLY
int init (void);
int
init (void)
#else
long _cdecl init (struct kentry *, struct usb_module_api *, long, long);
long _cdecl
init (struct kentry *k, struct usb_module_api *uapi, long arg, long reason)
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
		(void) Cconws ("MOUSE failed to get _USB cookie\r\n");
		return -1;
	}
#endif

	ret = udd_register (&mouse_uif);

	if (ret)
	{
		DEBUG (("%s: udd register failed!", __FILE__));
		return 1;
	}

	DEBUG (("%s: udd register ok", __FILE__));

	vector = (KBDVEC *) Kbdvbase ();

#ifdef TOSONLY
#if 0
	old_ikbd_int = Setexc (0x114/4, (long) interrupt_ikbd);
#else
	{
		ret = Super (0L);
		old_ikbd_int = (void *) *(volatile unsigned long *) 0x400;
		*(volatile unsigned long *) 0x400 = (unsigned long) interrupt_ikbd;
		SuperToUser (ret);
	}
#endif
	c_conws ("USB mouse driver installed.\r\n");

	Ptermres (_PgmSize, 0);
#endif

	return 0;
}
