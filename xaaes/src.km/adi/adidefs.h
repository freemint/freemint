/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
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

#ifndef _adidefs_h
#define _adidefs_h

#include "moose.h"

#define ADI_OPEN	1
#define ADI_NAMSIZ	16

#define ADIC_MOUSE	0

struct adif;
struct adiinfo
{
	char		name[16];
	short		(*getfreeunit)		(char *);
	long		(*adi_register)		(struct adif *);
	long		(*adi_unregister)	(struct adif *);

	void _cdecl	(*move)		(short x, short y);
	void _cdecl	(*button)	(struct moose_data *);
	void _cdecl	(*wheel)	(struct moose_data *);
};

struct adif
{
	struct adif		*next;
	struct kernel_module	*km;
	long			class;

	char		*lname;
	char		name[ADI_NAMSIZ];
	short		unit;

	unsigned short	flags;

	long	(*open)		(void);
	long	(*close)	(void);
	long	(*ioctl)	(short, long);
	long	(*timeout)	(void);

	long		reserved[15];
};

#endif /* _adidefs_h */
