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

#ifdef __MINT__
#include "global.h"
#include "config.h" 
#include "usb.h"
#include "hub.h"
#include "ucd.h"
#include "ucdload.h"
#include "uddload.h"
#else
#include <common.h>
#include <command.h>
#include <asm/processor.h>
#include <linux/compiler.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>

#include <usb.h>
#ifdef CONFIG_4xx
#include <asm/4xx_pci.h>
#endif
#endif

/*static*/ struct usb_device usb_dev[USB_MAX_DEVICE];
static long dev_index;
static long asynch_allowed;
static struct devrequest setup_packet;
char usb_started; /* flag for the started/stopped USB status */

static long 	usb_find_interface_driver	(struct usb_device *dev, unsigned ifnum);

/***************************************************************************
 * Init USB Device
 */
void
usb_main(void *dummy)
{
	/* load driver for usb host controller */
	ucd_load(1);

	if (usb_init() >= 0)
	{	
		/* load device driver modules */
		udd_load(1);
		return;
	}
	DEBUG(("Failing with USB init"));
}

long usb_init(void)
{
	struct ucdif *a;
	struct usb_device *dev;
	int i, start_index = 0;

	DEBUG(("usb_init"));

	long result;

	dev_index = 0;
	asynch_allowed = 1;
	usb_hub_reset();

        /* first make all devices unknown */
        for (i = 0; i < USB_MAX_DEVICE; i++) {
                memset(&usb_dev[i], 0, sizeof(struct usb_device));
                usb_dev[i].devnum = -1;
        }

	/* init low_level USB */
	for (a = allucdifs; a; a = a->next) {
		ucd_open (a);
	
		result = ucd_ioctl(allucdifs, LOWLEVEL_INIT, 0);
		if (result)
		{
			DEBUG (("%s: ucd low level init failed!", __FILE__));
			return -1;
		}

		/* if lowlevel init is OK, scan the bus for devices
		 * i.e. search HUBs and configure them 
		 */
		start_index = dev_index;
		DEBUG(("scanning bus %d for devices... ", a->unit));

		dev = usb_alloc_new_device(a);
		/*
		 * device 0 is always present
		 * (root hub, so let it analyze)
		 */
		if (dev)
			usb_new_device(dev);

		if (start_index == dev_index)
			DEBUG(("No USB Device found"));
		else
			DEBUG(("%ld USB Device(s) found",
				dev_index - start_index));

		usb_started = 1;
	}

	DEBUG(("scan end"));
	/* if we were not able to find at least one working bus, bail out */
	if (!usb_started) {
		DEBUG(("USB error: all controllers failed lowlevel init"));
		return -1;
	}

	return 0;
}

/******************************************************************************
 * Stop USB this stops the LowLevel Part and deregisters USB devices.
 */
long usb_stop(void)
{
	struct ucdif *a;

	if (usb_started)
	{
		asynch_allowed = 1;
		usb_started = 0;
		usb_hub_reset();

		for (a = allucdifs; a; a = a->next) {
			ucd_ioctl(allucdifs, LOWLEVEL_STOP, 0);
		}
	}

	return 0;
}

/*
 * disables the asynch behaviour of the control message. This is used for data
 * transfers that uses the exclusiv access to the control and bulk messages.
 * Returns the old value so it can be restored later.
 */
long usb_disable_asynch(long disable)
{
	int old_value = asynch_allowed;

	asynch_allowed = !disable;
	return old_value;
}


/*-------------------------------------------------------------------
 * Message wrappers.
 *
 */

/*
 * submits an Interrupt Message
 */
long usb_submit_int_msg(struct usb_device *dev, unsigned long pipe,
			void *buffer, long transfer_len, long interval)
{
	struct int_msg arg;
	
	arg.dev = dev;
	arg.pipe = pipe;
	arg.buffer = buffer;
	arg.transfer_len = transfer_len;
	arg.interval =  interval;
	
	return (ucd_ioctl(allucdifs, SUBMIT_INT_MSG, (long)&arg));
}

/*
 * submits a control message and waits for completion (at least timeout * 1ms)
 * If timeout is 0, we don't wait for completion (used as example to set and
 * clear keyboards LEDs). For data transfers, (storage transfers) we don't
 * allow control messages with 0 timeout, by previousely resetting the flag
 * asynch_allowed (usb_disable_asynch(1)).
 * returns the transfered length if OK or -1 if error. The transfered length
 * and the current status are stored in the dev->act_len and dev->status.
 */
long usb_control_msg(struct usb_device *dev, unsigned long pipe,
			unsigned char request, unsigned char requesttype,
			unsigned short value, unsigned short index,
			void *data, unsigned short size, long timeout)
{
	struct control_msg arg;

	if ((timeout == 0) && (!asynch_allowed)) {
		/* request for a asynch control pipe is not allowed */
		return -1;
	}

	/* set setup command */
	setup_packet.requesttype = requesttype;
	setup_packet.request = request;
	setup_packet.value = cpu2le16(value);
	setup_packet.index = cpu2le16(index);
	setup_packet.length = cpu2le16(size);
	DEBUG(("usb_control_msg: request: 0x%x, requesttype: 0x%x, value 0x%x idx 0x%x length 0x%x",
		   request, requesttype, value, index, size));
	dev->status = USB_ST_NOT_PROC; /* not yet processed */

	arg.dev = dev;
	arg.pipe = pipe;
	arg.data = data;
	arg.size = size;
	arg.setup = &setup_packet;

	ucd_ioctl(allucdifs, SUBMIT_CONTROL_MSG, (long)&arg);
	if (timeout == 0)
	{
		DEBUG(("size %d \r", size));
		return (long)size;
	}

       /*
        * Wait for status to update until timeout expires, USB driver
        * interrupt handler may set the status when the USB operation has
        * been completed.
        */
	while (timeout--) {
		if (!((volatile unsigned long)dev->status & USB_ST_NOT_PROC))
			break;
		mdelay(1);
	}

	if (dev->status)
	{
		return -1;
	}

	return dev->act_len;
}

/*-------------------------------------------------------------------
 * submits bulk message, and waits for completion. returns 0 if Ok or
 * -1 if Error.
 * synchronous behavior
 */
long usb_bulk_msg(struct usb_device *dev, unsigned long pipe,
			void *data, long len, long *actual_length, long timeout)
{
	struct bulk_msg arg;

	if (len < 0)
		return -1;

	dev->status = USB_ST_NOT_PROC; /*not yet processed */
	
	arg.dev = dev;
	arg.pipe = pipe;
	arg.data = data;
	arg.len = len;
	ucd_ioctl(allucdifs, SUBMIT_BULK_MSG, (long)&arg);

	while (timeout--) {
		if (!((volatile unsigned long)dev->status & USB_ST_NOT_PROC))
			break;
		mdelay(1);
	}

	*actual_length = dev->act_len;
	if (dev->status == 0)
		return 0;
	else
		return -1;
}


/*-------------------------------------------------------------------
 * Max Packet stuff
 */

/*
 * returns the max packet size, depending on the pipe direction and
 * the configurations values
 */
long usb_maxpacket(struct usb_device *dev, unsigned long pipe)
{
	/* direction is out -> use emaxpacket out */
	if ((pipe & USB_DIR_IN) == 0)
		return dev->epmaxpacketout[((pipe>>15) & 0xf)];
	else
		return dev->epmaxpacketin[((pipe>>15) & 0xf)];
}

/* The routine usb_set_maxpacket_ep() is extracted from the loop of routine
 * usb_set_maxpacket(), because the optimizer of GCC 4.x chokes on this routine
 * when it is inlined in 1 single routine. What happens is that the register r3
 * is used as loop-count 'i', but gets overwritten later on.
 * This is clearly a compiler bug, but it is easier to workaround it here than
 * to update the compiler (Occurs with at least several GCC 4.{1,2},x
 * CodeSourcery compilers like e.g. 2007q3, 2008q1, 2008q3 lite editions on ARM)
 *
 * NOTE: Similar behaviour was observed with GCC4.6 on ARMv5.
 */
static void  __attribute__((noinline))
usb_set_maxpacket_ep(struct usb_device *dev, int if_idx, int ep_idx)
{
	int b;
	struct usb_endpoint_descriptor *ep;

	ep = &dev->config.if_desc[if_idx].ep_desc[ep_idx];

	b = ep->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;

	if ((ep->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
						USB_ENDPOINT_XFER_CONTROL) {
		/* Control => bidirectional */
		dev->epmaxpacketout[b] = ep->wMaxPacketSize;
		dev->epmaxpacketin[b] = ep->wMaxPacketSize;
		DEBUG(("##Control EP epmaxpacketout/in[%d] = %ld",
			   b, dev->epmaxpacketin[b]));
	} else {
		if ((ep->bEndpointAddress & 0x80) == 0) {
			/* OUT Endpoint */
			if (ep->wMaxPacketSize > dev->epmaxpacketout[b]) {
				dev->epmaxpacketout[b] = ep->wMaxPacketSize;
				DEBUG(("##EP epmaxpacketout[%d] = %ld",
					   b, dev->epmaxpacketout[b]));
			}
		} else {
			/* IN Endpoint */
			if (ep->wMaxPacketSize > dev->epmaxpacketin[b]) {
				dev->epmaxpacketin[b] = ep->wMaxPacketSize;
				DEBUG(("##EP epmaxpacketin[%d] = %ld",
					   b, dev->epmaxpacketin[b]));
			}
		} /* if out */
	} /* if control */
}

/*
 * set the max packed value of all endpoints in the given configuration
 */
long usb_set_maxpacket(struct usb_device *dev)
{
	long i, ii;

	for (i = 0; i < dev->config.desc.bNumInterfaces; i++)
		for (ii = 0; ii < dev->config.if_desc[i].desc.bNumEndpoints; ii++)
			usb_set_maxpacket_ep(dev, i, ii);

	return 0;
}

/*******************************************************************************
 * Parse the config, located in buffer, and fills the dev->config structure.
 * Note that all little/big endian swapping are done automatically.
 */
long usb_parse_config(struct usb_device *dev, unsigned char *buffer, long cfgno)
{
	struct usb_descriptor_header *head;
	long index, ifno, epno, curr_if_num;
	struct usb_interface *if_desc = NULL;

	ifno = -1;
	epno = -1;
	curr_if_num = -1;

	dev->configno = cfgno;
	head = (struct usb_descriptor_header *) &buffer[0];
	if (head->bDescriptorType != USB_DT_CONFIG) {
		DEBUG((" ERROR: NOT USB_CONFIG_DESC %x",
			head->bDescriptorType));
		return -1;
	}
	memcpy(&dev->config, buffer, buffer[0]);
	le16_to_cpus(&(dev->config.desc.wTotalLength));
	dev->config.no_of_if = 0;

	index = dev->config.desc.bLength;
	/* Ok the first entry must be a configuration entry,
	 * now process the others */
	head = (struct usb_descriptor_header *) &buffer[index];
	while (index + 1 < dev->config.desc.wTotalLength) {
		switch (head->bDescriptorType) {
		case USB_DT_INTERFACE:
			if (((struct usb_interface_descriptor *) \
			     &buffer[index])->bInterfaceNumber != curr_if_num) {
				/* this is a new interface, copy new desc */
				ifno = dev->config.no_of_if;
				if_desc = &dev->config.if_desc[ifno];
				dev->config.no_of_if++;
				memcpy(if_desc,	&buffer[index], buffer[index]);
				if_desc->no_of_ep = 0;
				if_desc->num_altsetting = 1;
				curr_if_num =
				     if_desc->desc.bInterfaceNumber;
			} else {
				/* found alternate setting for the interface */
				if (ifno >= 0) {
					if_desc = &dev->config.if_desc[ifno];
					if_desc->num_altsetting++;
				}
			}
			break;

		case USB_DT_ENDPOINT:
			epno = dev->config.if_desc[ifno].no_of_ep;
			if_desc = &dev->config.if_desc[ifno];
			/* found an endpoint */
			if_desc->no_of_ep++;
			memcpy(&if_desc->ep_desc[epno],
				&buffer[index], buffer[index]);
			le16_to_cpus(&(if_desc->ep_desc[epno].wMaxPacketSize));
			DEBUG(("if %ld, ep %ld", ifno, epno));
			break;
		case USB_DT_SS_ENDPOINT_COMP:
			if_desc = &dev->config.if_desc[ifno];
			memcpy(&if_desc->ss_ep_comp_desc[epno],
				&buffer[index], buffer[index]);
			break;
		default:
			if (head->bLength == 0)
				return 1;

			DEBUG(("unknown Description Type : %x", head->bDescriptorType));
			{
				unsigned char *ch = (unsigned char *)head;
				int i;
			
				/* Debug stuff */
				char build_str[4];
				char buf[4 * head->bLength];
				sprintf(buf, sizeof(buf),"\0");
				for (i = 0; i < head->bLength; i++)
				{
					sprintf(build_str, sizeof(build_str), "%02x ", *ch++);
					strcat(buf, build_str);
				}
				DEBUG((buf));
			}

			break;
		}
		index += head->bLength;
		head = (struct usb_descriptor_header *)&buffer[index];
	}
	return 1;
}

/***********************************************************************
 * Clears an endpoint
 * endp: endpoint number in bits 0-3;
 * direction flag in bit 7 (1 = IN, 0 = OUT)
 */
long usb_clear_halt(struct usb_device *dev, long pipe)
{
	long result;
	long endp = usb_pipeendpoint(pipe)|(usb_pipein(pipe)<<7);
	
	result = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				 USB_REQ_CLEAR_FEATURE, USB_RECIP_ENDPOINT, 0,
				 endp, NULL, 0, USB_CNTL_TIMEOUT * 3);

	/* don't clear if failed */
	if (result < 0)
		return result;

	/*
	 * NOTE: we do not get status and verify reset was successful
	 * as some devices are reported to lock up upon this check..
	 */

	usb_endpoint_running(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe));

	/* toggle is reset on clear */
	usb_settoggle(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe), 0);
	return 0;
}


/**********************************************************************
 * get_descriptor type
 */
long usb_get_descriptor(struct usb_device *dev, unsigned char type,
			unsigned char index, void *buf, long size)
{
	long res;

	res = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
			USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
			(type << 8) + index, 0,
			buf, size, USB_CNTL_TIMEOUT);
	return res;
}

/**********************************************************************
 * gets configuration cfgno and store it in the buffer
 */
long usb_get_configuration_no(struct usb_device *dev, unsigned char *buffer, long cfgno)
{
	long result;
	unsigned long tmp;
	struct usb_config_descriptor *config;


	config = (struct usb_config_descriptor *)&buffer[0];
	result = usb_get_descriptor(dev, USB_DT_CONFIG, cfgno, buffer, 9);
	if (result < 9) {
		if (result < 0)
			DEBUG(("unable to get descriptor, error %lx",
				dev->status));
		else
			DEBUG(("config descriptor too short " \
				"(expected %i, got %i)", 9, result));
		return -1;
	}
	tmp = le2cpu16(config->wTotalLength);

	if (tmp > USB_BUFSIZ) {
		DEBUG(("usb_get_configuration_no: failed to get " \
			   "descriptor - too long: %ld", tmp));
		return -1;
	}

	result = usb_get_descriptor(dev, USB_DT_CONFIG, cfgno, buffer, tmp);
	DEBUG(("get_conf_no %ld Result %ld, wLength %ld",
		   cfgno, result, tmp));
	return result;
}

/********************************************************************
 * set address of a device to the value in dev->devnum.
 * This can only be done by addressing the device via the default address (0)
 */
long usb_set_address(struct usb_device *dev)
{
	long res;

	DEBUG(("set address %ld", dev->devnum));
	res = usb_control_msg(dev, usb_snddefctrl(dev),
				USB_REQ_SET_ADDRESS, 0,
				(dev->devnum), 0,
				NULL, 0, USB_CNTL_TIMEOUT);
	return res;
}

/********************************************************************
 * set interface number to interface
 */
long usb_set_interface(struct usb_device *dev, long interface, long alternate)
{
	struct usb_interface *if_face = NULL;
	long ret, i;

	for (i = 0; i < dev->config.desc.bNumInterfaces; i++) {
		if (dev->config.if_desc[i].desc.bInterfaceNumber == interface) {
			if_face = &dev->config.if_desc[i];
			break;
		}
	}
	if (!if_face) {
		DEBUG(("selecting invalid interface %ld", interface));
		return -1;
	}
	/*
	 * We should return now for devices with only one alternate setting.
	 * According to 9.4.10 of the Universal Serial Bus Specification
	 * Revision 2.0 such devices can return with a STALL. This results in
	 * some USB sticks timeouting during initialization and then being
	 * unusable in U-Boot.
	 */
	if (if_face->num_altsetting == 1)
		return 0;

	ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				USB_REQ_SET_INTERFACE, USB_RECIP_INTERFACE,
				alternate, interface, NULL, 0,
				USB_CNTL_TIMEOUT * 5);
	if (ret < 0)
		return ret;

	return 0;
}

/********************************************************************
 * set configuration number to configuration
 */
long usb_set_configuration(struct usb_device *dev, long configuration)
{
	long res;
	DEBUG(("set configuration %ld", configuration));
	/* set setup command */
	res = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				USB_REQ_SET_CONFIGURATION, 0,
				configuration, 0,
				NULL, 0, USB_CNTL_TIMEOUT);
	if (res == 0) {
		dev->toggle[0] = 0;
		dev->toggle[1] = 0;
		return 0;
	} else
		return -1;
}

/********************************************************************
 * set protocol to protocol
 */
long usb_set_protocol(struct usb_device *dev, long ifnum, long protocol)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
		USB_REQ_SET_PROTOCOL, USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		protocol, ifnum, NULL, 0, USB_CNTL_TIMEOUT);
}

/********************************************************************
 * set idle
 */
long usb_set_idle(struct usb_device *dev, long ifnum, long duration, long report_id)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
		USB_REQ_SET_IDLE, USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		(duration << 8) | report_id, ifnum, NULL, 0, USB_CNTL_TIMEOUT);
}

/********************************************************************
 * get report
 */
long usb_get_report(struct usb_device *dev, long ifnum, unsigned char type,
		   unsigned char id, void *buf, long size)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
			USB_REQ_GET_REPORT,
			USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			(type << 8) + id, ifnum, buf, size, USB_CNTL_TIMEOUT);
}

/********************************************************************
 * get class descriptor
 */
long
usb_get_class_descriptor(struct usb_device *dev, long ifnum,
		unsigned char type, unsigned char id, void *buf, long size)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
		USB_REQ_GET_DESCRIPTOR, USB_RECIP_INTERFACE | USB_DIR_IN,
		(type << 8) + id, ifnum, buf, size, USB_CNTL_TIMEOUT);
}

/********************************************************************
 * get string index in buffer
 */
long usb_get_string(struct usb_device *dev, unsigned short langid,
		   unsigned char index, void *buf, long size)
{
	long i;
	long result;

	for (i = 0; i < 3; ++i) {
		/* some devices are flaky */
		result = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
			USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
			(USB_DT_STRING << 8) + index, langid, buf, size,
			USB_CNTL_TIMEOUT);

		if (result > 0)
			break;
	}
	return result;
}


static void usb_try_string_workarounds(unsigned char *buf, long *length)
{
	long newlength, oldlength = *length;

	for (newlength = 2; newlength + 1 < oldlength; newlength += 2) {
		char c = buf[newlength];
		if ((c < ' ') || (c >= 127) || buf[newlength + 1])
			break;
	}
	if (newlength > 2) {
		buf[0] = newlength;
		*length = newlength;
	}
}


static long usb_string_sub(struct usb_device *dev, unsigned long langid,
		unsigned long index, unsigned char *buf)
{
	long rc;

	/* Try to read the string descriptor by asking for the maximum
	 * possible number of bytes */
	rc = usb_get_string(dev, langid, index, buf, 255);

	/* If that failed try to read the descriptor length, then
	 * ask for just that many bytes */
	if (rc < 2) {
		rc = usb_get_string(dev, langid, index, buf, 2);
		if (rc == 2)
			rc = usb_get_string(dev, langid, index, buf, buf[0]);
	}

	if (rc >= 2) {
		if (!buf[0] && !buf[1])
			usb_try_string_workarounds(buf, &rc);

		/* There might be extra junk at the end of the descriptor */
		if (buf[0] < rc)
			rc = buf[0];

		rc = rc - (rc & 1); /* force a multiple of two */
	}

	if (rc < 2)
		rc = -1;

	return rc;
}


/********************************************************************
 * usb_string:
 * Get string index and translate it to ascii.
 * returns string length (> 0) or error (< 0)
 */
long usb_string(struct usb_device *dev, long index, char *buf, size_t size)
{
	unsigned char mybuf[USB_BUFSIZ];
	unsigned char *tbuf;
	long err;
	unsigned long u, idx;

	if (size <= 0 || !buf || !index)
		return -1;
	buf[0] = 0;
	tbuf = &mybuf[0];

	/* get langid for strings if it's not yet known */
	if (!dev->have_langid) {
		err = usb_string_sub(dev, 0, 0, tbuf);
		if (err < 0) {
			DEBUG(("error getting string descriptor 0 " \
				   "(error=%lx)", dev->status));
			return -1;
		} else if (tbuf[0] < 4) {
			DEBUG(("string descriptor 0 too short"));
			return -1;
		} else {
			dev->have_langid = -1;
			dev->string_langid = tbuf[2] | (tbuf[3] << 8);
				/* always use the first langid listed */
			DEBUG(("USB device number %ld default " \
				   "language ID 0x%lx",
				   dev->devnum, dev->string_langid));
		}
	}

	err = usb_string_sub(dev, dev->string_langid, index, tbuf);
	if (err < 0)
		return err;

	size--;		/* leave room for trailing NULL char in output buffer */
	for (idx = 0, u = 2; u < err; u += 2) {
		if (idx >= size)
			break;
		if (tbuf[u+1])			/* high byte */
			buf[idx++] = '?';  /* non-ASCII character */
		else
			buf[idx++] = tbuf[u];
	}
	buf[idx] = 0;
	err = idx;
	return err;
}


/********************************************************************
 * USB device handling:
 * the USB device are static allocated [USB_MAX_DEVICE].
 */


/*
 * Something got disconnected. Get rid of it, and all of its children.
 */
void
usb_disconnect(struct usb_device **pdev)
{
	struct usb_device *dev = *pdev;

	if (dev)
	{
		long i;

		ALERT(("USB disconnect on device %ld", dev->parent->devnum));
		DEBUG(("USB device disconnect on device %s", dev->parent->prod));

		DEBUG(("USB disconnected, device number %ld", dev->devnum));
		DEBUG(("USB device disconnected, device %s", dev->prod));

		if(dev->driver)
			dev->driver->disconnect(dev);

		/* Free up all the children.. */
		for (i = 0; i < USB_MAXCHILDREN; i++)
		{
			if (dev->children[i] != NULL)
			{
				DEBUG(("Disconnect children %ld", dev->children[i]->devnum));
				struct usb_device **child = &dev->children[i];
				DEBUG(("child %lx", *child));
				usb_disconnect(child);
				dev->children[i] = NULL;
			}
		}

		if (dev->devnum > 0)
		{
			memset(dev, 0, sizeof(struct usb_device));
			dev->devnum = -1;
			dev_index--;
		}
		
	}	
	DEBUG(("Number of devices attached %ld", dev_index - 1));
	DEBUG(("Number of devices in the bus %ld", dev_index));
}


/* returns a pointer to the device with the index [index].
 * if the device is not assigned (dev->devnum==-1) returns NULL
 */
struct usb_device *usb_get_dev_index(long index)
{
	if (usb_dev[index].devnum == -1)
		return NULL;
	else
		return &usb_dev[index];
}


/* returns a pointer of a new device structure or NULL, if
 * no device struct is available
 */
struct usb_device *usb_alloc_new_device(void *controller)
{
	long i = 0;
	DEBUG(("New Device %ld\n", dev_index));
	if (dev_index == USB_MAX_DEVICE) {
		ALERT(("ERROR, too many USB Devices, max=%d", USB_MAX_DEVICE));
		return NULL;
	}

	/* default Address is 0, real addresses start with 1 */
	usb_dev[dev_index].devnum = dev_index + 1;
	usb_dev[dev_index].maxchild = 0;
	for (i = 0; i < USB_MAXCHILDREN; i++)
		usb_dev[dev_index].children[i] = NULL;
	usb_dev[dev_index].parent = NULL;
	usb_dev[dev_index].controller = controller;
	dev_index++;

	return &usb_dev[dev_index - 1];
}

/*
 * Free the newly created device node.
 * Called in error cases where configuring a newly attached
 * device fails for some reason.
 */
void usb_free_device(void)
{
	dev_index--;
	DEBUG(("Freeing device node: %ld\n", dev_index));
	memset(&usb_dev[dev_index], 0, sizeof(struct usb_device));
	usb_dev[dev_index].devnum = -1;
}


/*
 * By the time we get here, the device has gotten a new device ID
 * and is in the default state. We need to identify the thing and
 * get the ball rolling..
 *
 * Returns 0 for success, != 0 for error.
 */
long usb_new_device(struct usb_device *dev)
{
	DEBUG(("usb_new_device: "));
	long addr, err;
	long tmp;
	unsigned char tmpbuf[USB_BUFSIZ];
 
	/* We still haven't set the Address yet */
	addr = dev->devnum;
	dev->devnum = 0;

#ifdef CONFIG_LEGACY_USB_INIT_SEQ
	/* this is the old and known way of initializing devices, it is
	 * different than what Windows and Linux are doing. Windows and Linux
	 * both retrieve 64 bytes while reading the device descriptor
	 * Several USB stick devices report ERR: CTL_TIMEOUT, caused by an
	 * invalid header while reading 8 bytes as device descriptor. */
	dev->descriptor.bMaxPacketSize0 = 8;	    /* Start off at 8 bytes  */
	dev->maxpacketsize = PACKET_SIZE_8;
	dev->epmaxpacketin[0] = 8;
	dev->epmaxpacketout[0] = 8;

	err = usb_get_descriptor(dev, USB_DT_DEVICE, 0, tmpbuf, 8);
	if (err < 8) {
		DEBUG(("\n      USB device not responding, " \
		       "giving up (status=%lX)\n", dev->status));
		return 1;
	}
	memcpy(&dev->descriptor, tmpbuf, 8);
#else
	/* This is a Windows scheme of initialization sequence, with double
	 * reset of the device (Linux uses the same sequence)
	 * Some equipment is said to work only with such init sequence; this
	 * patch is based on the work by Alan Stern:
	 * http://sourceforge.net/mailarchive/forum.php?
	 * thread_id=5729457&forum_id=5398
	 */
	struct usb_device_descriptor *desc;
	long port = -1;
	struct usb_device *parent = dev->parent;
	unsigned short portstatus;

	/* send 64-byte GET-DEVICE-DESCRIPTOR request.  Since the descriptor is
	 * only 18 bytes long, this will terminate with a short packet.  But if
	 * the maxpacket size is 8 or 16 the device may be waiting to transmit
	 * some more, or keeps on retransmitting the 8 byte header. */

	desc = (struct usb_device_descriptor *)tmpbuf;
	dev->descriptor.bMaxPacketSize0 = 64;	    /* Start off at 64 bytes  */
	/* Default to 64 byte max packet size */
	dev->maxpacketsize = PACKET_SIZE_64;
	dev->epmaxpacketin[0] = 64;
	dev->epmaxpacketout[0] = 64;

	err = usb_get_descriptor(dev, USB_DT_DEVICE, 0, desc, 64);
	if (err < 0) {
		DEBUG(("usb_new_device: usb_get_descriptor() failed"));
		return 1;
	}

	dev->descriptor.bMaxPacketSize0 = desc->bMaxPacketSize0;
	/*
	 * Fetch the device class, driver can use this info
	 * to differentiate between HUB and DEVICE.
	 */
	dev->descriptor.bDeviceClass = desc->bDeviceClass;

	/* find the port number we're at */
	if (parent) {
		long j;

		for (j = 0; j < parent->maxchild; j++) {
			if (parent->children[j] == dev) {
				port = j;
				break;
			}
		}
		if (port < 0) {
			DEBUG(("usb_new_device:cannot locate device's port."));
			return 1;
		}

		/* reset the port for the second time */
		err = hub_port_reset(dev->parent, port, &portstatus);
		if (err < 0) {
			DEBUG(("Couldn't reset port %li", port));
			return 1;
		}
	}
#endif

	dev->epmaxpacketin[0] = dev->descriptor.bMaxPacketSize0;
	dev->epmaxpacketout[0] = dev->descriptor.bMaxPacketSize0;
	switch (dev->descriptor.bMaxPacketSize0) {
	case 8:
		dev->maxpacketsize  = PACKET_SIZE_8;
		break;
	case 16:
		dev->maxpacketsize = PACKET_SIZE_16;
		break;
	case 32:
		dev->maxpacketsize = PACKET_SIZE_32;
		break;
	case 64:
		dev->maxpacketsize = PACKET_SIZE_64;
		break;
	}
	dev->devnum = addr;

	err = usb_set_address(dev); /* set address */

	if (err < 0) {
		DEBUG(("USB device not accepting new address " \
			"(error=%lx)", dev->status));
		return 1;
	}

	mdelay(10);	/* Let the SET_ADDRESS settle */

	tmp = sizeof(dev->descriptor);

	err = usb_get_descriptor(dev, USB_DT_DEVICE, 0,
				 tmpbuf, sizeof(dev->descriptor));
	if (err < tmp) {
		if (err < 0)
			DEBUG(("unable to get device descriptor (error=%ld)",
			       err));
		else
			DEBUG(("USB device descriptor short read " \
				"(expected %li, got %li)", tmp, err));
		return 1;
	}
	memcpy(&dev->descriptor, tmpbuf, sizeof(dev->descriptor));
	/* correct le values */
	le16_to_cpus(&dev->descriptor.bcdUSB);
	le16_to_cpus(&dev->descriptor.idVendor);
	le16_to_cpus(&dev->descriptor.idProduct);
	le16_to_cpus(&dev->descriptor.bcdDevice);
	/* only support for one config for now */
	err = usb_get_configuration_no(dev, tmpbuf, 0);
	if (err < 0) {
		DEBUG(("usb_new_device: Cannot read configuration, " \
		       "skipping device %04x:%04x\n",
		       dev->descriptor.idVendor, dev->descriptor.idProduct));
		return -1;
	}
	usb_parse_config(dev, tmpbuf, 0);
	usb_set_maxpacket(dev);
	/* we set the default configuration here */
	if (usb_set_configuration(dev, dev->config.desc.bConfigurationValue)) {
		DEBUG(("failed to set default configuration " \
			"len %ld, status %lx", dev->act_len, dev->status));
		return -1;
	}
	DEBUG(("new device strings: Mfr=%d, Product=%d, SerialNumber=%d",
		   dev->descriptor.iManufacturer, dev->descriptor.iProduct,
		   dev->descriptor.iSerialNumber));
	memset(dev->mf, 0, sizeof(dev->mf));
	memset(dev->prod, 0, sizeof(dev->prod));
	memset(dev->serial, 0, sizeof(dev->serial));
	if (dev->descriptor.iManufacturer)
		usb_string(dev, dev->descriptor.iManufacturer,
			   dev->mf, sizeof(dev->mf));
	if (dev->descriptor.iProduct)
		usb_string(dev, dev->descriptor.iProduct,
			   dev->prod, sizeof(dev->prod));
	if (dev->descriptor.iSerialNumber)
		usb_string(dev, dev->descriptor.iSerialNumber,
			   dev->serial, sizeof(dev->serial));
	DEBUG(("Manufacturer %s", dev->mf));
	DEBUG(("Product      %s", dev->prod));
	DEBUG(("SerialNumber %s", dev->serial));
	ALERT(("New USB device (%ld) %s", dev->devnum, dev->prod));
	/* now probe if the device is a hub */
	tmp = usb_hub_probe(dev, 0);

	if (tmp < 1)
		/* assign driver if possible */
		usb_find_interface_driver(dev, 0);

	return 0;
}

/********************************************************************
 * USB device driver handling:
 * 
 */

extern LIST_HEAD(,usb_driver) usb_driver_list;

/*
 * This entrypoint gets called for each new device.
 *
 * We now walk the list of registered USB drivers,
 * looking for one that will accept this interface.
 *
 * The probe return value is changed to be a private pointer.  This way
 * the drivers don't have to dig around in our structures to set the
 * private pointer if they only need one interface. 
 *
 * Returns: 0 if a driver accepted the interface, -1 otherwise
 */
static long 
usb_find_interface_driver(struct usb_device *dev, unsigned ifnum)
{
	struct usb_driver *tmp;

	if ((!dev) || (ifnum >= dev->config.desc.bNumInterfaces)) {
		DEBUG(("bad find_interface_driver params"));
		return -1;
	}
	
	if (dev->driver)
		return -1;

	LIST_FOREACH (tmp, &usb_driver_list, chain)
	{
		struct usb_driver *driver = tmp;
	
		DEBUG(("LIST_FOREACH. tmp %lx", tmp ));

		if (!driver->probe(dev))
		{
			DEBUG(("driver->probe(dev) = 0"));

			dev->driver = driver;
			
			return 0;
		}
	
		if (!(tmp = LIST_NEXT(tmp, chain)))
			break;
	}
	return -1;
}

