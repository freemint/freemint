/*
 * Modified for the FreeMiNT USB subsystem by David Galvez. 2010 - 2011
 * Modified for Atari by Didier Mequignon 2009
 *	
 * (C) Copyright 2001
 * Denis Peter, MPL AG Switzerland
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Note: Part of this code has been derived from linux
 *
 */

#ifndef _HUB_H_
#define _HUB_H_

/*************************************************************************
 * Hub Stuff
 */
struct usb_port_status
{
	unsigned short wPortStatus;
	unsigned short wPortChange;
} __attribute__ ((packed));

struct usb_hub_status
{
	unsigned short wHubStatus;
	unsigned short wHubChange;
} __attribute__ ((packed));


/* Hub descriptor */
struct usb_hub_descriptor
{
	unsigned char  bLength;
	unsigned char  bDescriptorType;
	unsigned char  bNbrPorts;
	unsigned short wHubCharacteristics;
	unsigned char  bPwrOn2PwrGood;
	unsigned char  bHubContrCurrent;
	unsigned char  DeviceRemovable[(USB_MAXCHILDREN+1+7)/8];
	unsigned char  PortPowerCtrlMask[(USB_MAXCHILDREN+1+7)/8];
	/* DeviceRemovable and PortPwrCtrlMask want to be variable-length
	   bitmaps that hold max 255 entries. (bit0 is ignored) */
} __attribute__ ((packed));


struct usb_hub_device
{
	struct usb_device *pusb_dev;
	struct usb_hub_descriptor desc;
};


long 		usb_get_hub_descriptor	(struct usb_device *dev, void *data, long size);
long 		usb_clear_hub_feature	(struct usb_device *dev, long feature);
long 		usb_clear_port_feature	(struct usb_device *dev, long port, long feature);
long 		usb_get_hub_status	(struct usb_device *dev, void *data);
long 		usb_set_port_feature	(struct usb_device *dev, long port, long feature);
long 		usb_get_port_status	(struct usb_device *dev, long port, void *data);
struct usb_hub_device *	usb_hub_allocate(void);
void 		usb_hub_port_connect_change	(struct usb_device *dev, long port);
long 		usb_hub_configure	(struct usb_device *dev);
long 		usb_hub_probe		(struct usb_device *dev, long ifnum);
void 		usb_hub_reset		(void);
long 		hub_port_reset		(struct usb_device *dev, long port,
			  		 unsigned short *portstat);
void		usb_rh_wakeup		(void);
void		usb_hub_init		(void);
void		usb_hub_thread		(void *);
void		usb_hub_poll		(PROC *, long);
void 		usb_hub_poll_thread	(void *);

#endif /*_HUB_H_ */
