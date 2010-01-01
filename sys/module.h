/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000-2004 Frank Naumann <fnaumann@freemint.de>
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

# ifndef _module_h
# define _module_h

# include "mint/mint.h"
# include "mint/module.h"

void init_module_subsys(void);

long _cdecl kernel_opendir(struct dirstruct *dirh, const char *name);
long _cdecl kernel_readdir(struct dirstruct *dirh, char *buf, int len);
void _cdecl kernel_closedir(struct dirstruct *dirh);

struct file * _cdecl kernel_open(const char *path, int rwmode, long *err, XATTR *x);
long _cdecl kernel_read(struct file *f, void *buf, long buflen);
long _cdecl kernel_write(struct file *f, const void *buf, long buflen);
long _cdecl kernel_lseek(FILEPTR *f, long where, int whence);
void _cdecl kernel_close(struct file *f);

/* load all kernel modules */
void load_all_modules(unsigned long mask);
void print_moduleinfo(void);

void _cdecl load_modules_old(const char *ext, long (*loader)(struct basepage *, const char *));
void _cdecl load_modules(const char *path, const char *ext, long (*loader)(struct basepage *, const char *, short *));

void _cdecl load_kmodules(struct kernel_module *parent, const char *path, const char *ext, void *arg, long (*loader)(struct kernel_module *, const char *));
long _cdecl unload_kmodule(struct kernel_module *km);
long _cdecl unload_kmodules(struct kernel_module *parent);
long _cdecl detach_km_devices(struct kernel_module *km, bool free);
void _cdecl detach_child_devices(struct kernel_module *parent);

struct kernel_module * _cdecl find_km_bydevicename(char *name);

long _cdecl register_trap2(long _cdecl (*dispatch)(void *), int mode, int flag, long extra);

extern DEVDRV module_device;

# endif /* _module_h */
