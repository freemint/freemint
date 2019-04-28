/*
 * USB keyboard driver
 *
 * Copyright (C) 2018 by Christian Zietz
 *
 * LED support by Claude Labelle
 *
 * TOS 1.x detection by Roger Burrows
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See /COPYING.GPL for details.
 */
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
	"\033p USB keyboard class driver " MSG_VERSION " \033q\r\n"

#define MSG_GREET	\
	"By Christian Zietz.\r\n" \
	"Compiled " MSG_BUILDDATE ".\r\n\r\n"


/****************************************************************************/
/*
 * BEGIN kernel interface
 */

#ifndef TOSONLY
struct kentry *kentry;

void kbd_poll_thread (void *);
void kbd_poll (PROC * proc, long dummy);
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

long *vector;
void *iokbd;
void _cdecl send_data (long func, long iorec, long data);
void _cdecl fake_hwint(void);

struct usb_module_api *api;

void kbd_int (void);
void set_led (void);

/*
 * END kernel interface
 */
/****************************************************************************/

/*
 * USB device interface
 */

static long kbd_ioctl (struct uddif *, short, long);
static long kbd_disconnect (struct usb_device *dev);
static long kbd_probe (struct usb_device *dev, unsigned int ifnum);

static unsigned int if_no;

static char lname[] = "USB keyboard class driver\0";

static struct uddif kbd_uif = {
	0,                          /* *next */
	USB_API_VERSION,            /* API */
	USB_DEVICE,                 /* class */
	lname,                      /* lname */
	"keyboard",                    /* name */
	0,                          /* unit */
	0,                          /* flags */
	kbd_probe,                /* probe */
	kbd_disconnect,           /* disconnect */
	0,                          /* resrvd1 */
	kbd_ioctl,                /* ioctl */
	0,                          /* resrvd2 */
};

struct kbd_report
{
	unsigned char mod;
	unsigned char reserved;
	unsigned char keys[6];
};

struct kbd_data
{
	struct usb_device *pusb_dev;        /* this usb_device */
	unsigned char ep_in;        /* in endpoint */
	unsigned char ep_out;       /* out ....... */
	unsigned char ep_int;       /* interrupt . */
	long *irq_handle;            /* for USB int requests */
	unsigned long irqpipe;      /* pipe for release_irq */
	unsigned long outpipe;      /* pipe for output */
	unsigned char irqmaxp;      /* max packed for irq Pipe */
	unsigned char irqinterval;  /* Intervall for IRQ Pipe */
	struct kbd_report olddata;
	struct kbd_report newdata;
};

static struct kbd_data kbd_data;

/*
 * --- Inteface functions
 * --------------------------------------------------
 */

static long _cdecl
kbd_ioctl (struct uddif *u, short cmd, long arg)
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
kbd_disconnect (struct usb_device *dev)
{
	if (dev == kbd_data.pusb_dev)
	{
		kbd_data.pusb_dev = NULL;
#ifndef TOSONLY
		wake (WAIT_Q, (long) &kbd_poll_thread);
#endif
	}

	return 0;
}

static long
usb_kbd_irq (struct usb_device *dev)
{
	return 0;
}

#define SEND_SCAN(x) send_data(vector[-1], (long)iokbd, (x)) /* assumes TOS >= 2 */

static void
handle_modifiers(unsigned char val, unsigned char offset)
{
	if ((val & 1) || (val & 0x10)) /* Left or right control */
		SEND_SCAN(0x1d + offset);

	if (val & 2) /* Left shift */
		SEND_SCAN(0x2a + offset);

	if (val & 0x20) /* Right shift */
		SEND_SCAN(0x36 + offset);

	if ((val & 4) || (val & 0x40)) /* Left or right alt */
		SEND_SCAN(0x38 + offset);
}

static unsigned char
translate_key(unsigned char usbkey)
{
	switch (usbkey)
	{
	case 0x04: return 0x1e; // A
	case 0x05: return 0x30; // B
	case 0x06: return 0x2e; // C
	case 0x07: return 0x20; // D
	case 0x08: return 0x12; // E
	case 0x09: return 0x21; // F
	case 0x0a: return 0x22; // G
	case 0x0b: return 0x23; // H
	case 0x0c: return 0x17; // I
	case 0x0d: return 0x24; // J
	case 0x0e: return 0x25; // K
	case 0x0f: return 0x26; // L
	case 0x10: return 0x32; // M
	case 0x11: return 0x31; // N
	case 0x12: return 0x18; // O
	case 0x13: return 0x19; // P
	case 0x14: return 0x10; // Q
	case 0x15: return 0x13; // R
	case 0x16: return 0x1f; // S
	case 0x17: return 0x14; // T
	case 0x18: return 0x16; // U
	case 0x19: return 0x2f; // V
	case 0x1a: return 0x11; // W
	case 0x1b: return 0x2d; // X
	case 0x1c: return 0x15; // Y
	case 0x1d: return 0x2c; // Z
	case 0x1e: return 0x02; // 1
	case 0x1f: return 0x03; // 2
	case 0x20: return 0x04; // 3
	case 0x21: return 0x05; // 4
	case 0x22: return 0x06; // 5
	case 0x23: return 0x07; // 6
	case 0x24: return 0x08; // 7
	case 0x25: return 0x09; // 8
	case 0x26: return 0x0a; // 9
	case 0x27: return 0x0b; // 0
	case 0x28: return 0x1c; // Return
	case 0x29: return 0x01; // ESC
	case 0x2a: return 0x0e; // Backspace
	case 0x2b: return 0x0f; // TAB
	case 0x2c: return 0x39; // Space
	case 0x2d: return 0x0c; // - (right of to 0)
	case 0x2e: return 0x0d; // = (right of minus)
	case 0x2f: return 0x1a; // [ (right of P, � in Germany)
	case 0x30: return 0x1b; // ] (right of ], + in Germany)
	case 0x31: return   43; // US \ (below delete, ~ in Germany) ????
	case 0x32: return   43; // non-US equivalent of \ (below delete, ~ in Germany) ????
	case 0x33: return 0x27; // ; (right of L, � in Germany)
	case 0x34: return 0x28; // ' (right of L, � in Germany)
	case 0x35: return   41; // ` (left of backspace, # in Germany)
	case 0x36: return 0x33; // , (right of M)
	case 0x37: return 0x34; // . (right of ,)
	case 0x38: return 0x35; // / (right of ., - in Germany)
	case 0x39: return 0x3a; // Caps-Lock
	case 0x3a: return 0x3b; // F1
	case 0x3b: return 0x3c; // F2
	case 0x3c: return 0x3d; // F3
	case 0x3d: return 0x3e; // F4
	case 0x3e: return 0x3f; // F5
	case 0x3f: return 0x40; // F6
	case 0x40: return 0x41; // F7
	case 0x41: return 0x42; // F8
	case 0x42: return 0x43; // F9
	case 0x43: return 0x44; // F10
	case 0x44: return   98; // F11 -> Help
	case 0x45: return   97; // F12 -> Undo
	case 0x46: return   99; // PrintScreen -> KP (

	case 0x48: return  100; // Pause -> KP )
	case 0x49: return   82; // Insert
	case 0x4a: return   71; // Home -> ClrHome
	//case 0x4b: return   99; // PageUp -> KP (
	case 0x4c: return   83; // Delete

	//case 0x4e: return  100; // PageDn -> KP )
	case 0x4f: return 0x4d; // Right arrow
	case 0x50: return 0x4b; // Left arrow
	case 0x51: return 0x50; // Down arrow
	case 0x52: return 0x48; // Up arrow

	case 0x54: return 0x65; // KP /
	case 0x55: return 0x66; // KP *
	case 0x56: return 0x4a; // KP +
	case 0x57: return 0x4e; // KP -
	case 0x58: return 0x72; // KP Enter
	case 0x59: return 0x6d; // KP 1
	case 0x5a: return 0x6e; // KP 2
	case 0x5b: return 0x6f; // KP 3
	case 0x5c: return 0x6a; // KP 4
	case 0x5d: return 0x6b; // KP 5
	case 0x5e: return 0x6c; // KP 6
	case 0x5f: return 0x67; // KP 7
	case 0x60: return 0x68; // KP 8
	case 0x61: return 0x69; // KP 9
	case 0x62: return 0x70; // KP 0
	case 0x63: return 0x71; // KP .
	case 0x64: return   96; // non-US equivalent of \ (left of Y, < in Germany)
	default: return 0;
	}
}

void
kbd_int (void)
{
	int i, j;
	long actlen = 0;
	long r;
	unsigned char temp, temp2 = 0;

	if (kbd_data.pusb_dev == NULL)
		return;

	r = usb_bulk_msg (kbd_data.pusb_dev,
					  kbd_data.irqpipe,
					  &kbd_data.newdata,
					  kbd_data.irqmaxp > 8 ? 8 : kbd_data.irqmaxp,
					  &actlen, USB_CNTL_TIMEOUT * 5, 1);

	if ((r != 0) || (actlen < 3) || (actlen > 8))
	{
		return;
	}

	for (i = actlen; i < 8; i++)
	{
		/* will also zero mod and reserved fields if required */
		kbd_data.newdata.keys[i-2] = 0;
	}

	/* Handle modifier keys first */
	DEBUG(("m: %02x", kbd_data.newdata.mod));
	/* Newly released modifiers */
	temp = (kbd_data.olddata.mod ^ kbd_data.newdata.mod) & kbd_data.olddata.mod;
	handle_modifiers(temp, 0x80);
	/* Newly pressed modifiers */
	temp = (kbd_data.olddata.mod ^ kbd_data.newdata.mod) & kbd_data.newdata.mod;
	handle_modifiers(temp, 0x00);

	/* Check for released keys */
	for (i=0; i<sizeof(kbd_data.olddata.keys); i++) {

		temp = kbd_data.olddata.keys[i];
		if (!temp)
			continue;

		for (j=0; j<sizeof(kbd_data.newdata.keys); j++) {
			if (kbd_data.newdata.keys[j] == temp)
				break;
		}

		/* Key from olddata not found in newdata =>  handle release */
		if (j == sizeof(kbd_data.newdata.keys)) {
			/* Special cases */
			if (temp == 0x4d) {			// End
				SEND_SCAN(0x47 + 0x80); // Home release
				SEND_SCAN(0x2a + 0x80);	// Shift release
			}
			else if (temp == 0x4b) {	// Page Up
				SEND_SCAN(0x48 + 0x80); // Arrow Up release
				SEND_SCAN(0x2a + 0x80);	// Shift release
			}
			else if (temp == 0x4e) {	// Page Down
				SEND_SCAN(0x50 + 0x80); // Arrow Down release
				SEND_SCAN(0x2a + 0x80);	// Shift release
			}
			else {
				temp2 = translate_key(temp);
				if (temp2)
					SEND_SCAN(temp2 + 0x80);
			}
		}
	}

	/* Check for pressed keys */
	for (i=0; i<sizeof(kbd_data.newdata.keys); i++) {

		temp = kbd_data.newdata.keys[i];
		if (!temp)
			continue;

		for (j=0; j<sizeof(kbd_data.olddata.keys); j++) {
			if (kbd_data.olddata.keys[j] == temp)
				break;
		}

		/* Key from newdata not found in olddata =>  handle press */
		if (j == sizeof(kbd_data.olddata.keys)) {
			DEBUG(("p: %02x -> %02x", temp, temp2));
			/* Special cases */
			if (temp == 0x4d) {			// End
				SEND_SCAN(0x2a);		// Shift press
				SEND_SCAN(0x47); 		// Home
			}
			else if (temp == 0x4b) {	// Page Up
				SEND_SCAN(0x2a);		// Shift press
				SEND_SCAN(0x48); 		// Arrow Up
			}
			else if (temp == 0x4e) {	// Page Down
				SEND_SCAN(0x2a);		// Shift press
				SEND_SCAN(0x50); 		// Arrow Down
			}
			else {
				temp2 = translate_key(temp);
				if (temp2)
					SEND_SCAN(temp2);
			}
			if (temp == 0x39)			// Caps Lock
				set_led();
		}
	}

	fake_hwint();

	kbd_data.olddata = kbd_data.newdata;
}

/* Set Caps Lock LED */
#define CAPS_LOCK 0x02
void
set_led (void)
{
	static unsigned char led_keys = 0;
	led_keys = (led_keys)?0:CAPS_LOCK;
	usb_control_msg(kbd_data.pusb_dev, usb_sndctrlpipe(kbd_data.pusb_dev, 0),
					USB_REQ_SET_REPORT,
					USB_TYPE_CLASS | USB_RECIP_INTERFACE,
					0x0200, if_no, &led_keys, 1, USB_CNTL_TIMEOUT * 5);
}

#ifndef TOSONLY
void
kbd_poll (PROC * proc, long dummy)
{
	wake (WAIT_Q, (long) &kbd_poll_thread);
}

void
kbd_poll_thread (void *dummy)
{
	p_setpriority(0,0,-20);

	while (kbd_data.pusb_dev)
	{
		kbd_int ();
		addroottimeout (20, kbd_poll, 0);
		sleep (WAIT_Q, (long) &kbd_poll_thread);
	}

	kthread_exit (0);
}
#endif

/*******************************************************************************
 *
 *
 */
static long
kbd_probe (struct usb_device *dev, unsigned int ifnum)
{
	struct usb_interface *iface;
	struct usb_endpoint_descriptor *ep_desc;

	/*
	 * Only one keyboard at time
	 */
	if (kbd_data.pusb_dev)
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
	if_no = ifnum;
	if (!iface)
	{
		return -1;
	}
	if (iface->desc.bInterfaceClass != USB_CLASS_HID && (iface->desc.bInterfaceSubClass != USB_SUB_HID_BOOT))
	{
		return -1;
	}
	if (iface->desc.bInterfaceProtocol != 1)
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
		kbd_data.ep_int =
			ep_desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
		kbd_data.irqinterval = ep_desc->bInterval;
	}
	else
	{
		return -1;
	}

	kbd_data.pusb_dev = dev;

	kbd_data.irqinterval =
		(kbd_data.irqinterval > 0) ? kbd_data.irqinterval : 255;
	kbd_data.irqpipe =
		usb_rcvintpipe (kbd_data.pusb_dev, (long) kbd_data.ep_int);
	kbd_data.irqmaxp = usb_maxpacket (dev, kbd_data.irqpipe);
	dev->irq_handle = usb_kbd_irq;
	memset (&kbd_data.newdata, 0, sizeof(kbd_data.newdata));
	memset (&kbd_data.olddata, 0, sizeof(kbd_data.olddata));

	usb_set_idle (dev, iface->desc.bInterfaceNumber, 0, 0);     /* report
																 * infinite
																 */
	usb_set_protocol(dev, iface->desc.bInterfaceNumber, 0); /* boot */

#ifndef TOSONLY
	long r = kthread_create (get_curproc (), kbd_poll_thread, NULL, NULL,
							 "keyboardpoll");

	if (r)
	{
		return -1;
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
	unsigned short gemdos;

#ifndef TOSONLY
	kentry = k;
	api = uapi;

	if (check_kentry_version ())
		return -1;
#endif

	c_conws (MSG_BOOT);
	c_conws (MSG_GREET);
	DEBUG (("%s: enter init", __FILE__));

	/*
	 * This driver uses the extended KBDVECS structure, if available.
	 * Since it's undocumented (though present in TOS2/3/4), there is no
	 * Atari-specified method to determine if it is available.  We use
	 * the GEMDOS version reported by Sversion() to discriminate:
	 *  . TOS 1 (which does not have it) reports versions < 0x0019
	 *  . TOS 2/3/4, MagiC, and EmuTOS (which all have it) report versions >= 0x0019
	 */
	gemdos = Sversion();
	gemdos = (gemdos>>8) | (gemdos<<8); /* major|minor */
	if (gemdos < 0x0019)
	{
		c_conws ("This driver does not support TOS 1.x\r\n");
		return -1;
	}

#ifdef TOSONLY
	/*
	 * GET _USB COOKIE to REGISTER
	 */
	if (!getcookie (_USB, (long *)&api))
	{
		(void) Cconws ("KEYBOARD failed to get _USB cookie\r\n");
		return -1;
	}
#endif

	ret = udd_register (&kbd_uif);

	if (ret)
	{
		DEBUG (("%s: udd register failed!", __FILE__));
		return 1;
	}

	DEBUG (("%s: udd register ok", __FILE__));

	vector = (long *) Kbdvbase ();
	iokbd = (void *)Iorec(1);
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
	c_conws ("USB keyboard driver installed.\r\n");

	Ptermres (_PgmSize, 0);
#endif

	return 0;
}
