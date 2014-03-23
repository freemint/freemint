/*
 * Modified for the FreeMiNT USB subsystem by David Galvez. 2010 - 2011
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright 2003 Odd Skancke <ozk@atari.org>
 * All rights reserved.
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
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

#ifndef _udd_defs_h
#define _udd_defs_h

#define UDD_OPEN	1
#define UDD_NAMSIZ	16	/* maximum ucd name len */

#define USB_DEVICE	0


struct uddif
{
	struct uddif	*next;

	long		class;
	char		*lname;
	char		name[UDD_NAMSIZ];
	short		unit;

	unsigned short	flags;

	long		(*open)		(struct uddif *);
	long		(*close)	(struct uddif *);
	long		resrvd1;	/* (*output)  */
	long		(*ioctl)	(struct uddif *, short cmd, long arg);
	long		resrvd2;	/* (*timeout) */

	long		reserved[24];
};

struct usb_module_api
{
//	short				(*getfreeunit)		(char *);
	long			_cdecl	(*udd_register)		(struct uddif *, struct usb_driver *);
	long			_cdecl	(*udd_unregister)	(struct uddif *, struct usb_driver *);
	long			_cdecl	(*ucd_register)		(struct ucdif *);
	long			_cdecl	(*ucd_unregister)	(struct ucdif *);
//	const char			*fname;


	long			_cdecl	(*usb_init)		(long handle, const struct pci_device_id *ent); /* initialize the USB Controller */
	long 			_cdecl	(*usb_stop)		(void); /* stop the USB Controller */
        long			_cdecl	(*usb_rh_wakeup)	(void);


	long			_cdecl	(*usb_set_protocol)	(struct usb_device *dev, long ifnum, long protocol);
	long			_cdecl	(*usb_set_idle)		(struct usb_device *dev, long ifnum, long duration,
								long report_id);
	struct usb_device *	_cdecl	(*usb_get_dev_index)	(long idx);
	long 			_cdecl	(*usb_control_msg)	(struct usb_device *dev, unsigned long pipe,
								unsigned char request, unsigned char requesttype,
								unsigned short value, unsigned short idx,
								void *data, unsigned short size, long timeout);
	long			_cdecl	(*usb_bulk_msg)		(struct usb_device *dev, unsigned long pipe,
								void *data, long len, long *actual_length, long timeout, long flags);
	long 			_cdecl	(*usb_submit_int_msg)	(struct usb_device *dev, unsigned long pipe,
								void *buffer, long transfer_len, long interval);
	long 			_cdecl	(*usb_disable_asynch)	(long disable);
	long			_cdecl	(*usb_maxpacket)	(struct usb_device *dev, unsigned long pipe);
	long			_cdecl	(*usb_get_configuration_no)	(struct usb_device *dev, unsigned char *buffer,
									long cfgno);
	long			_cdecl	(*usb_get_report)	(struct usb_device *dev, long ifnum, unsigned char type,
								unsigned char id, void *buf, long size);
	long 			_cdecl	(*usb_get_class_descriptor)	(struct usb_device *dev, long ifnum,
									unsigned char type, unsigned char id, void *buf,
									long size);
	long 			_cdecl	(*usb_clear_halt)	(struct usb_device *dev, long pipe);
	long 			_cdecl	(*usb_string)		(struct usb_device *dev, long idx, char *buf, long size);
	long 			_cdecl	(*usb_set_interface)	(struct usb_device *dev, long interface, long alternate);
	long			_cdecl	(*usb_parse_config)	(struct usb_device *dev, unsigned char *buffer, long cfgno);
	long 			_cdecl	(*usb_set_maxpacket)	(struct usb_device *dev);
	long 			_cdecl	(*usb_get_descriptor)	(struct usb_device *dev, unsigned char type,
								unsigned char idx, void *buf, long size);
	long 			_cdecl	(*usb_set_address)	(struct usb_device *dev);
	long 			_cdecl	(*usb_set_configuration)(struct usb_device *dev, long configuration);
	long 			_cdecl	(*usb_get_string)	(struct usb_device *dev, unsigned short langid,
			   					unsigned char idx, void *buf, long size);
	struct usb_device *	_cdecl	(*usb_alloc_new_device) (void *controller);
	long 			_cdecl	(*usb_new_device)	(struct usb_device *dev);
	
	/* For now we leave this hub specific stuff out of the api */
//	long 			_cdecl	(*usb_get_hub_descriptor)	(struct usb_device *dev, void *data, long size);
//	long 			_cdecl	(*usb_clear_port_feature)	(struct usb_device *dev, long port, long feature);
//	long 			_cdecl	(*usb_get_hub_status)	(struct usb_device *dev, void *data);
//	long 			_cdecl	(*usb_set_port_feature)	(struct usb_device *dev, long port, long feature);
//	long 			_cdecl	(*usb_get_port_status)	(struct usb_device *dev, long port, void *data);
//	struct usb_hub_device *	_cdecl	(*usb_hub_allocate)	(void);
//	void 			_cdecl	(*usb_hub_port_connect_change)	(struct usb_device *dev, long port);
//	long 			_cdecl	(*usb_hub_configure)	(struct usb_device *dev);
};

#endif /* _udd_defs_h */
