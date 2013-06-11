/*
 * Modified for the FreeMiNT USB subsystem by David Galvez. 2010 - 2011
 * Modified for Atari by Didier Mequignon 2009
 *
 * Most of this source has been derived from the Linux USB
 * project:
 * (C) Copyright Linus Torvalds 1999
 * (C) Copyright Johannes Erdfelt 1999-2001
 * (C) Copyright Andreas Gal 1999
 * (C) Copyright Gregory P. Smith 1999
 * (C) Copyright Deti Fliegl 1999 (new USB architecture)
 * (C) Copyright Randy Dunlap 2000
 * (C) Copyright David Brownell 2000 (kernel hotplug, usb_device_id)
 * (C) Copyright Yggdrasil Computing, Inc. 2000
 *     (usb_device_id matching changes by Adam J. Richter)
 *
 * Adapted for U-Boot:
 * (C) Copyright 2001 Denis Peter, MPL AG Switzerland
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
 *
 */

#include "global.h"
#include "usb.h"
#include "hub.h"
#include "init.h"
#include "time.h"

#include "mint/signal.h"
#include "mint/proc.h"

/****************************************************************************
 * HUB "Driver"
 * Probes device for being a hub and configurate it
 */

static struct usb_hub_device hub_dev[USB_MAX_HUB];
static long usb_hub_index;
extern struct usb_device usb_dev[USB_MAX_DEVICE];

static void	sigterm			(void);
static void	sigchld			(void);
static void	ignore			(short);
static void	fatal			(short);
static void	setup_common		(void);


long usb_get_hub_descriptor(struct usb_device *dev, void *data, long size)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
		USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_RT_HUB,
		USB_DT_HUB << 8, 0, data, size, USB_CNTL_TIMEOUT);
}

long usb_clear_port_feature(struct usb_device *dev, long port, long feature)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				USB_REQ_CLEAR_FEATURE, USB_RT_PORT, feature,
				port, NULL, 0, USB_CNTL_TIMEOUT);
}

long usb_set_port_feature(struct usb_device *dev, long port, long feature)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				USB_REQ_SET_FEATURE, USB_RT_PORT, feature,
				port, NULL, 0, USB_CNTL_TIMEOUT);
}

long usb_get_hub_status(struct usb_device *dev, void *data)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
			USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_HUB, 0, 0,
			data, sizeof(struct usb_hub_status), USB_CNTL_TIMEOUT);
}

long usb_get_port_status(struct usb_device *dev, long port, void *data)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
			USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_PORT, 0, port,
			data, sizeof(struct usb_hub_status), USB_CNTL_TIMEOUT);
}


static void usb_hub_power_on(struct usb_hub_device *hub)
{
	long i;
	struct usb_device *dev;
	unsigned pgood_delay = hub->desc.bPwrOn2PwrGood * 2;
	struct usb_port_status portsts;
	unsigned short portstatus;
	int ret;

	dev = hub->pusb_dev;

	/*
	 * Enable power to the ports:
	 * Here we Power-cycle the ports: aka,
	 * turning them off and turning on again.
	 */
	DEBUG(("enabling power on all ports\n"));
	for (i = 0; i < dev->maxchild; i++) {
		usb_clear_port_feature(dev, i + 1, USB_PORT_FEAT_POWER);
		DEBUG(("port %d returns %lX\n", i + 1, dev->status));
	}

	/* Wait at least 2*bPwrOn2PwrGood for PP to change */
	mdelay(pgood_delay);

	for (i = 0; i < dev->maxchild; i++) {
		ret = usb_get_port_status(dev, i + 1, &portsts);
		if (ret < 0) {
			DEBUG(("port %d: get_port_status failed\n", i + 1));
			return;
		}

		/*
		 * Check to confirm the state of Port Power:
		 * xHCI says "After modifying PP, s/w shall read
		 * PP and confirm that it has reached the desired state
		 * before modifying it again, undefined behavior may occur
		 * if this procedure is not followed".
		 * EHCI doesn't say anything like this, but no harm in keeping
		 * this.
		 */
		portstatus = le2cpu16(portsts.wPortStatus);
		if (portstatus & (USB_PORT_STAT_POWER << 1)) {
			DEBUG(("port %d: Port power change failed\n", i + 1));
			return;
		}
	}

	for (i = 0; i < dev->maxchild; i++) {
		usb_set_port_feature(dev, i + 1, USB_PORT_FEAT_POWER);
		DEBUG(("port %ld returns %lx", i + 1, dev->status));
		mdelay(hub->desc.bPwrOn2PwrGood * 2);
	}

	/* Wait at least 100 msec for power to become stable */
	mdelay(MAX(pgood_delay, (unsigned)100));
}

void usb_hub_reset(void)
{
	usb_hub_index = 0;
}

struct usb_hub_device *usb_hub_allocate(void)
{
	if (usb_hub_index < USB_MAX_HUB)
		return &hub_dev[usb_hub_index++];

	ALERT(("ERROR: USB_MAX_HUB (%d) reached", USB_MAX_HUB));
	return NULL;
}

#define MAX_TRIES 5

static inline char *portspeed(long portstatus)
{
	char *speed_str;

	switch (portstatus & USB_PORT_STAT_SPEED_MASK) {
	case USB_PORT_STAT_SUPER_SPEED:
		speed_str = "5 Gb/s";
		break;
	case USB_PORT_STAT_HIGH_SPEED:
		speed_str = "480 Mb/s";
		break;
	case USB_PORT_STAT_LOW_SPEED:
		speed_str = "1.5 Mb/s";
		break;
	default:
		speed_str = "12 Mb/s";
		break;
	}

	return speed_str;
}

long hub_port_reset(struct usb_device *dev, long port,
			unsigned short *portstat)
{
	long tries;
	struct usb_port_status portsts;
	unsigned short portstatus, portchange;

	DEBUG(("hub_port_reset: resetting port %ld...", port));
	for (tries = 0; tries < MAX_TRIES; tries++) {
		usb_set_port_feature(dev, port + 1, USB_PORT_FEAT_RESET);
		mdelay(200);

		if (usb_get_port_status(dev, port + 1, &portsts) < 0) {
			DEBUG(("get_port_status failed status %lx",
					dev->status));
			return -1;
		}
		portstatus = le2cpu16(portsts.wPortStatus);
		portchange = le2cpu16(portsts.wPortChange);

		DEBUG(("portstatus %x, change %x, %s",
				portstatus, portchange,
				portspeed(portstatus)));

		DEBUG(("STAT_C_CONNECTION = %d STAT_CONNECTION = %d" \
			       "  USB_PORT_STAT_ENABLE %d",
			(portchange & USB_PORT_STAT_C_CONNECTION) ? 1 : 0,
			(portstatus & USB_PORT_STAT_CONNECTION) ? 1 : 0,
			(portstatus & USB_PORT_STAT_ENABLE) ? 1 : 0));

		if ((portchange & USB_PORT_STAT_C_CONNECTION) ||
		    !(portstatus & USB_PORT_STAT_CONNECTION))
			return -1;

		if (portstatus & USB_PORT_STAT_ENABLE)
			break;

		mdelay(200);
	}

	if (tries == MAX_TRIES) {
		DEBUG(("Cannot enable port %li after %i retries, " \
				"disabling port.", port + 1, MAX_TRIES));
		DEBUG(("Maybe the USB cable is bad?"));
		return -1;
	}

	usb_clear_port_feature(dev, port + 1, USB_PORT_FEAT_C_RESET);
	*portstat = portstatus;
	return 0;
}


void usb_hub_port_connect_change(struct usb_device *dev, long port)
{
	struct usb_device *usb;
	struct usb_port_status portsts;
	unsigned short portstatus;

	/* Check status */
	if (usb_get_port_status(dev, port + 1, &portsts) < 0) {
		DEBUG(("get_port_status failed"));
		return;
	}

	portstatus = le2cpu16(portsts.wPortStatus);
	DEBUG(("portstatus %x, change %x, %s\n",
	      portstatus,
	      le2cpu16(portsts.wPortChange),
	      portspeed(portstatus)));

	/* Clear the connection change status */
	usb_clear_port_feature(dev, port + 1, USB_PORT_FEAT_C_CONNECTION);

	/* Disconnect any existing devices under this port */
	if (((!(portstatus & USB_PORT_STAT_CONNECTION)) &&
	     (!(portstatus & USB_PORT_STAT_ENABLE))) || (dev->children[port])) {
		DEBUG(("usb_disconnect"));
		DEBUG(("devnum %ld", dev->children[port]->devnum));
		usb_disconnect(&dev->children[port]);
		dev->children[port] = NULL;
		/* Return now if nothing is connected */
		if (!(portstatus & USB_PORT_STAT_CONNECTION))
			return;
	}
	mdelay(200);

	/* Reset the port */
	if (hub_port_reset(dev, port, &portstatus) < 0) {
		DEBUG(("cannot reset port %li!?", port + 1));
		return;
	}
	mdelay(200);

	/* Allocate a new device struct for it */
	usb = usb_alloc_new_device(dev->controller);

	switch (portstatus & USB_PORT_STAT_SPEED_MASK) {
	case USB_PORT_STAT_SUPER_SPEED:
		usb->speed = USB_SPEED_SUPER;
		break;
	case USB_PORT_STAT_HIGH_SPEED:
		usb->speed = USB_SPEED_HIGH;
		break;
	case USB_PORT_STAT_LOW_SPEED:
		usb->speed = USB_SPEED_LOW;
		break;
	default:
		usb->speed = USB_SPEED_FULL;
		break;
	}

	dev->children[port] = usb;
	usb->parent = dev;
	/* Run it through the hoops (find a driver, etc) */
	if (usb_new_device(usb)) {
		/* Woops, disable the port */
		usb_free_device();
		dev->children[port] = NULL;
		DEBUG(("hub: disabling port %ld", port + 1));
		usb_clear_port_feature(dev, port + 1, USB_PORT_FEAT_ENABLE);
	}
}


long
usb_hub_configure(struct usb_device *dev)
{
	unsigned char buffer[USB_BUFSIZ], *bitmap;
	struct usb_hub_descriptor *descriptor;
	struct usb_hub_status *hubsts;
	long i;
	struct usb_hub_device *hub;

	/* "allocate" Hub device */
	hub = usb_hub_allocate();
	if (hub == NULL)
		return -1;
	hub->pusb_dev = dev;
	/* Get the the hub descriptor */
	if (usb_get_hub_descriptor(dev, buffer, 4) < 0) {
		DEBUG(("usb_hub_configure: failed to get hub " \
				   "descriptor, giving up %lx", dev->status));
		return -1;
	}
	descriptor = (struct usb_hub_descriptor *)buffer;

	/* silence compiler warning if USB_BUFSIZ is > 256 [= sizeof(char)] */
	i = descriptor->bLength;
	if (i > USB_BUFSIZ) {
		DEBUG(("usb_hub_configure: failed to get hub " \
				"descriptor - too long: %d",
				descriptor->bLength));
		return -1;
	}

	if (usb_get_hub_descriptor(dev, buffer, descriptor->bLength) < 0) {
		DEBUG(("usb_hub_configure: failed to get hub " \
				"descriptor 2nd giving up %lx", dev->status));
		return -1;
	}
	memcpy((unsigned char *)&hub->desc, buffer, descriptor->bLength);
	/* adjust 16bit values */
	hub->desc.wHubCharacteristics =
				le2cpu16(descriptor->wHubCharacteristics);

	/* set the bitmap */
	bitmap = (unsigned char *)&hub->desc.DeviceRemovable[0];
	/* devices not removable by default */
	memset(bitmap, 0xff, (USB_MAXCHILDREN+1+7)/8);
	bitmap = (unsigned char *)&hub->desc.PortPowerCtrlMask[0];
	memset(bitmap, 0xff, (USB_MAXCHILDREN+1+7)/8); /* PowerMask = 1B */

	for (i = 0; i < ((hub->desc.bNbrPorts + 1 + 7)/8); i++)
		hub->desc.DeviceRemovable[i] = descriptor->DeviceRemovable[i];

	for (i = 0; i < ((hub->desc.bNbrPorts + 1 + 7)/8); i++)
		hub->desc.PortPowerCtrlMask[i] = descriptor->PortPowerCtrlMask[i];

	dev->maxchild = descriptor->bNbrPorts;
	DEBUG(("%ld ports detected", dev->maxchild));

	switch (hub->desc.wHubCharacteristics & HUB_CHAR_LPSM) {
	case 0x00:
		DEBUG(("ganged power switching"));
		break;
	case 0x01:
		DEBUG(("individual port power switching"));
		break;
	case 0x02:
	case 0x03:
		DEBUG(("unknown reserved power switching mode"));
		break;
	}

	if (hub->desc.wHubCharacteristics & HUB_CHAR_COMPOUND) {
		DEBUG(("part of a compound device"));
	} else {
		DEBUG(("standalone hub"));
	}

	switch (hub->desc.wHubCharacteristics & HUB_CHAR_OCPM) {
	case 0x00:
		DEBUG(("global over-current protection"));
		break;
	case 0x08:
		DEBUG(("individual port over-current protection"));
		break;
	case 0x10:
	case 0x18:
		DEBUG(("no over-current protection"));
		break;
	}

	DEBUG(("power on to power good time: %dms",
			descriptor->bPwrOn2PwrGood * 2));
	DEBUG(("hub controller current requirement: %dmA",
			descriptor->bHubContrCurrent));

	for (i = 0; i < dev->maxchild; i++)
		DEBUG(("port %ld is%s removable", i + 1,
			hub->desc.DeviceRemovable[(i + 1) / 8] & \
					   (1 << ((i + 1) % 8)) ? " not" : ""));

	if (sizeof(struct usb_hub_status) > USB_BUFSIZ) {
		DEBUG(("usb_hub_configure: failed to get Status - " \
				"too long: %d", descriptor->bLength));
		return -1;
	}

	if (usb_get_hub_status(dev, buffer) < 0) {
		DEBUG(("usb_hub_configure: failed to get Status %lx",
				dev->status));
		return -1;
	}

	hubsts = (struct usb_hub_status *)buffer;
	UNUSED(hubsts);
	DEBUG(("get_hub_status returned status %x, change %x",
			le2cpu16(hubsts->wHubStatus),
			le2cpu16(hubsts->wHubChange)));
	DEBUG(("local power source is %s",
		(le2cpu16(hubsts->wHubStatus) & HUB_STATUS_LOCAL_POWER) ? \
		"lost (inactive)" : "good"));
	DEBUG(("%sover-current condition exists",
		(le2cpu16(hubsts->wHubStatus) & HUB_STATUS_OVERCURRENT) ? \
		"" : "no "));
	usb_hub_power_on(hub);
	
	return 0;
}


static void
usb_hub_events(struct usb_device *dev)
{
	long i;
	struct usb_hub_device *hub;

	hub = dev->privptr;

	for (i = 0; i < dev->maxchild; i++)
	{
		struct usb_port_status portsts;
		unsigned short portstatus, portchange;

		/* GALVEZ: add delay for MiNT/TOS, needed to detect device */
		mdelay(5); /* default 250 */

		if (usb_get_port_status(dev, i + 1, &portsts) < 0)
		{
			DEBUG(("get_port_status failed"));
			continue;
		}

		portstatus = le2cpu16(portsts.wPortStatus);
		portchange = le2cpu16(portsts.wPortChange);
		DEBUG(("Port %ld Status %x Change %x",
				i + 1, portstatus, portchange));

		if (portchange & USB_PORT_STAT_C_CONNECTION)
		{
			DEBUG(("port %ld connection change", i + 1));
			usb_hub_port_connect_change(dev, i);
		}

		if (portchange & USB_PORT_STAT_C_ENABLE)
		{
			DEBUG(("port %ld enable change, status %x",
					i + 1, portstatus));
			usb_clear_port_feature(dev, i + 1,
						USB_PORT_FEAT_C_ENABLE);

			/* EM interference sometimes causes bad shielded USB
			 * devices to be shutdown by the hub, this hack enables
			 * them again. Works at least with mouse driver */
			if (!(portstatus & USB_PORT_STAT_ENABLE) &&
			     (portstatus & USB_PORT_STAT_CONNECTION) &&
			     ((dev->children[i])))
			{
				DEBUG(("already running port %i "  \
						"disabled by hub (EMI?), " \
						"re-enabling...", i + 1);
				usb_hub_port_connect_change(dev, i));
			}
		}
		if (portstatus & USB_PORT_STAT_SUSPEND)
		{
			DEBUG(("port %ld suspend change", i + 1);
			usb_clear_port_feature(dev, i + 1,
						USB_PORT_FEAT_SUSPEND));
		}

		if (portchange & USB_PORT_STAT_C_OVERCURRENT)
		{
			DEBUG(("port %ld over-current change", i + 1));
			usb_clear_port_feature(dev, i + 1,
						USB_PORT_FEAT_C_OVER_CURRENT);
			usb_hub_power_on(hub);
		}

		if (portchange & USB_PORT_STAT_C_RESET)
		{
			DEBUG(("port %ld reset change", i + 1));
			usb_clear_port_feature(dev, i + 1,
						USB_PORT_FEAT_C_RESET);
		}
	} /* end for i all ports */
	return;
}


void
usb_hub_poll(PROC *p, long device)
{
	wake(WAIT_Q, (long)&usb_hub_poll_thread);
#if 0
	struct usb_device *dev = (struct usb_device *)device;

	usb_hub_events(dev);

	TIMEOUT *t;
	t = addtimeout(2000L, usb_hub_poll);
	if (t)
	{
		t->arg = device;
	}
#endif
}


void 
usb_hub_poll_thread(void *dev)
{

	/* join process group of loader, 
	 * otherwise doesn't ends when shutingdown
	 */
	p_setpgrp(0, loader_pgrp);

	for (;;)
	{
//		usb_hub_poll(get_curproc(), (long)dev);
		usb_hub_events(dev);
		addtimeout(2000L, usb_hub_poll);
		sleep(WAIT_Q, (long)&usb_hub_poll_thread);
	}

	kthread_exit(0);
}

long
usb_hub_probe(struct usb_device *dev, long ifnum)
{
	struct usb_interface *iface;
	struct usb_endpoint_descriptor *ep;
	long ret;
	struct usb_hub_device *hub;

	iface = &dev->config.if_desc[ifnum];
	/* Is it a hub? */
	if (iface->desc.bInterfaceClass != USB_CLASS_HUB)
		return 0;
	/* Some hubs have a subclass of 1, which AFAICT according to the */
	/*  specs is not defined, but it works */
	if ((iface->desc.bInterfaceSubClass != 0) &&
	    (iface->desc.bInterfaceSubClass != 1))
		return 0;
	/* Multiple endpoints? What kind of mutant ninja-hub is this? */
	if (iface->desc.bNumEndpoints != 1)
		return 0;
	ep = &iface->ep_desc[0];
	/* Output endpoint? Curiousier and curiousier.. */
	if (!(ep->bEndpointAddress & USB_DIR_IN))
		return 0;
	/* If it's not an interrupt endpoint, we'd better punt! */
	if ((ep->bmAttributes & 3) != 3)
		return 0;
	/* We found a hub */
	DEBUG(("USB hub found"));

	if ((hub = kmalloc(sizeof(*hub))) == NULL)
	{
		DEBUG(("couldn't kmalloc hub struct\n"));
		return -1;
	}

	memset(hub, 0, sizeof(*hub));

	dev->privptr = hub;	

	ret = usb_hub_configure(dev);

	/* 
	 * Galvez: While interrupt transfer in isp116x driver isn't supported,
	 * we poll the hub devices attached to the root hub
	 */
	if (dev->devnum > 1)
	{
		DEBUG(("Installing poll for hub device"));

		long r;
		r = kthread_create(NULL, usb_hub_poll_thread, dev, NULL, "hubpoll");
		
		if (r)
		{
			/* XXX todo -> exit gracefully */
			DEBUG((/*0000000a*/"can't create USB hub kernel thread"));
		}
		
	}

	return ret;
}

void
usb_rh_wakeup(void)
{
	wake(WAIT_Q, (long)usb_hub_thread);
}

/*
 * signal handlers
 */
static void
ignore(short sig)
{
	DEBUG(("USB: ignored signal"));
	DEBUG(("'%s': received signal: %d(ignored)", get_curproc()->name, sig));
}

static void
fatal(short sig)
{
	DEBUG(("'%s': fatal error: %d", get_curproc()->name, sig));
	DEBUG(("'%s': fatal error, trying to clean up", get_curproc()->name ));
	usb_stop();
}


static void
sigterm(void)
{
//	struct proc *p = get_curproc();
	DEBUG(("%s(%ld:USB: ): sigterm received", get_curproc()->name, get_curproc()->pid));

#if 0
	DEBUG(("(ignored)" ));
	return;
#else
//	if( p->pid != C.AESpid )
//	{
//		BLOG((false, "(ignored)" ));
//		return;

//	}
	DEBUG(("shutdown USB" ));
	usb_stop();
#endif
}

static void
sigchld(void)
{
	long r;

	while ((r = p_waitpid(-1, 1, NULL)) > 0)
	{
		DEBUG(("sigchld -> %li (pid %li)", r, ((r & 0xffff0000L) >> 16)));
	}
}

static void
setup_common(void)
{

	/* terminating signals */
	p_signal(SIGHUP,   (long) ignore);
	p_signal(SIGINT,   (long) ignore);
	p_signal(SIGQUIT,  (long) ignore);
	p_signal(SIGPIPE,  (long) ignore);
	p_signal(SIGALRM,  (long) ignore);
	p_signal(SIGSTOP,  (long) ignore);
	p_signal(SIGTSTP,  (long) ignore);
	p_signal(SIGTTIN,  (long) ignore);
	p_signal(SIGTTOU,  (long) ignore);
	p_signal(SIGXCPU,  (long) ignore);
	p_signal(SIGXFSZ,  (long) ignore);
	p_signal(SIGVTALRM,(long) ignore);
	p_signal(SIGPROF,  (long) ignore);
	p_signal(SIGUSR1,  (long) ignore);
	p_signal(SIGUSR2,  (long) ignore);

	/* fatal signals */
	p_signal(SIGILL,   (long) fatal);
	p_signal(SIGTRAP,  (long) fatal);
	p_signal(SIGABRT,  (long) fatal);
	p_signal(SIGFPE,   (long) ignore);//fatal);
	p_signal(SIGBUS,   (long) fatal);
	p_signal(SIGSEGV,  (long) fatal);
	p_signal(SIGSYS,   (long) fatal);

	/* other stuff */
	p_signal(SIGTERM,  (long) sigterm);
	p_signal(SIGCHLD,  (long) sigchld);

//	d_setdrv('u' - 'a');
// 	d_setpath("/");

}

void
usb_hub_thread(void *dummy)
{
	setup_common();
	
	/* join process group of loader, 
	 * otherwise doesn't ends when shutingdown
	 */
	p_setpgrp(0, loader_pgrp);

	while (!(0))
	{
		sleep(WAIT_Q, (long)usb_hub_thread);
		/* only root hub is interupt driven */
		usb_hub_events(&usb_dev[0]);
	}

	kthread_exit(0);
}

/*
 * This should be may be a separate module.
 */
void
usb_hub_init(void)
{
	DEBUG(("Creating USB hub kernel thread"));
	long r;

//	LIST_INIT(&hub_event_list)

	r = kthread_create(get_curproc(), usb_hub_thread, NULL, NULL, "hubd");
		
	if (r)
	{
		/* XXX todo -> exit gracefully */
		DEBUG((/*0000000a*/"can't create USB hub kernel thread"));
	}
		
}
