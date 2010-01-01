/*
 * $Id$
 *
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
#include "mint/lists.h"

#define MOD_LOADED	1

#define MODCLASS_ROOT		0
#define MODCLASS_BUS		1

#define MODCLASS_XIF		2
#define MODCLASS_XDD		3
#define MODCLASS_XFS		4
#define MODCLASS_KM		5
#define MODCLASS_KMDEF		6
#define MAX_MODCLASS		7

#define	DF_ENABLED	1		/* device should be probed/attached */
#define	DF_FIXEDCLASS	2		/* devclass specified at create time */
#define	DF_WILDCARD	4		/* unit was originally wildcard */
#define	DF_DESCMALLOCED	8		/* description was malloced */
#define	DF_QUIET	16		/* don't print verbose attach message */
#define	DF_DONENOMATCH	32		/* don't execute DEVICE_NOMATCH again */
#define	DF_EXTERNALSOFTC 64		/* softc not allocated by us */
#define	DF_REBID	128		/* Can rebid after attach */

#define DEVICE_ENABLED(x)	(x->flags & DF_ENABLED)
#define set_device_disabled(x)	(x->flags &= ~DF_ENABLED)

struct basepage;
struct dirstruct;
struct kentry;
struct kernel_module;
struct km_device;

typedef struct km_device *device_t;

#define DEVNAMELEN	32
struct device_methods
{
	void _cdecl (*identify)	(device_t dev);
	long _cdecl (*probe)	(device_t dev);
	long _cdecl (*attach)	(device_t dev);
	long _cdecl (*detach)	(device_t dev);
};

struct km_device
{
	TAILQ_ENTRY(km_device)	next;
	TAILQ_ENTRY(km_device)	link;
	TAILQ_ENTRY(km_device)	devlink;

	struct kernel_module *km;
	char name[32];
	unsigned long flags;
	void *softc;	
	struct device_methods *methods;
};

#define KMF_HAVEDEVICE	1

#define KM_HAVEDEVICE(x)	(x->flags & KMF_HAVEDEVICE)

struct kernel_module
{
	struct kernel_module *next;
	struct kernel_module *prev;
	struct kernel_module *parent;
	struct kernel_module *children;
	
	struct basepage *b;
	struct kentry *kentry;
//	void *priv_methods;
	
	TAILQ_HEAD(device_list, km_device) devices;
// 	device_t *dev;

	char name[32];
	char fpath[PATH_MAX];
	char fname[64];
	short class;
	short unused;
	unsigned long flags;
};

static inline void
device_identify(device_t dev)
{
	if (dev->methods->identify)
		(*dev->methods->identify)(dev);
}

static inline long
device_probe(device_t dev)
{
	long error = EINVAL;

	if (dev->methods->probe)
		error = (*dev->methods->probe)(dev);

	return error;
}

static inline long
device_attach(device_t dev)
{
	long error;

	if (dev->methods->attach) {
		error = (*dev->methods->attach)(dev);
		if (!error)
			dev->flags |= DF_ENABLED;
	} else
		error = EINVAL;
	return error;
}

static inline long
device_detach(device_t dev)
{
	long error;

	if (dev->methods->detach) {
		error = (*dev->methods->detach)(dev);
		if (!error)
			dev->flags &= ~DF_ENABLED;
	} else
		error = EINVAL;
	return error;
}
#endif	/* _mint_module_h_ */
