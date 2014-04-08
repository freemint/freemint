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

#ifndef _ucd_defs_h
#define _ucd_defs_h

#define UCD_OPEN	1
#define UCD_NAMSIZ	16	/* maximum ucd name len */

#define USB_CONTRLL	0

/*
 * ioctl opcodes
 */
#define LOWLEVEL_INIT		(('U'<< 8) | 0)
#define LOWLEVEL_STOP		(('U'<< 8) | 1)
#define SUBMIT_CONTROL_MSG	(('U'<< 8) | 2)
#define SUBMIT_BULK_MSG		(('U'<< 8) | 3)
#define SUBMIT_INT_MSG		(('U'<< 8) | 4)


struct bulk_msg
{
	struct usb_device 	*dev;
	unsigned long 		pipe;
	void 			*data;
	long 			len;
	long                    flags;
};

struct control_msg
{
	struct usb_device 	*dev;
	unsigned long 		pipe;
	unsigned short 		value;
	void 			*data;
	unsigned short 		size;
	struct devrequest	*setup;
};

struct int_msg
{
	struct usb_device 	*dev;
	unsigned long 		pipe;
	void 			*buffer;
	long 			transfer_len;
	long 			interval;
};

struct ucdif
{
	struct ucdif	*next;

	long		class;
	char		*lname;
	char		name[UCD_NAMSIZ];
	short		unit;

	unsigned short	flags;

	long		(*open)		(struct ucdif *);
	long		(*close)	(struct ucdif *);
	long		resrvd1;	/* (*output)  */
	long		(*ioctl)	(struct ucdif *, short cmd, long arg);
	long		resrvd2;	/* (*timeout) */
	void		*ucd_priv;	/* host controller driver private data */

	long		reserved[20];
};

#endif /* _ucd_defs_h */
