/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
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

#ifndef _mint_module_h_
#define _mint_module_h_

#include "mint/mint.h"
#include "mint/config.h"

#define MOD_LOADED	1

#define MODCLASS_XIF	1
#define MODCLASS_XDD	2
#define MODCLASS_XFS	3
#define MODCLASS_KM	4
#define MODCLASS_KMDEF	5

struct basepage;
struct dirstruct;
struct kentry;

struct km_api
{
	long _cdecl (*install)(struct kentry *, const char *path);
	long _cdecl (*uninstall)(void);
};

struct kernel_module
{
	struct kernel_module *next;
	struct kernel_module *prev;
	struct basepage *b;
	short class;
	short subclass;
	unsigned long flags;
	struct km_api *kmapi;
	char path[PATH_MAX];
	char name[64];
	struct proc *caller;
};

#endif	/* _mint_module_h_ */
