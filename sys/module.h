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

struct basepage;
struct dirstruct;

long _cdecl kernel_opendir(struct dirstruct *dirh, const char *name);
long _cdecl kernel_readdir(struct dirstruct *dirh, char *buf, int len);
void _cdecl kernel_closedir(struct dirstruct *dirh);

struct file * _cdecl kernel_open(const char *path, int rwmode, long *err);
long _cdecl kernel_read(struct file *f, void *buf, long buflen);
long _cdecl kernel_write(struct file *f, const void *buf, long buflen);
long _cdecl kernel_lseek(FILEPTR *f, long where, int whence);
void _cdecl kernel_close(struct file *f);

void load_all_modules(const char *curpath, unsigned long mask);
void _cdecl load_modules(const char *extension, long (*loader)(struct basepage *, const char *));

long _cdecl load_km(const char *path);
long _cdecl register_trap2(long _cdecl (*dispatch)(void *), int mode, int flag, long extra);

# endif /* _module_h */
