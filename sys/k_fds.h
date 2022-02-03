/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
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
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2001-01-13
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _k_fds_h
# define _k_fds_h

# include "mint/mint.h"
# include "mint/file.h"


long fd_alloc	(struct proc *p, short *fd, short min, const char *func);
void fd_remove	(struct proc *p, short fd, const char *func);

# define FD_ALLOC(p, fd, min)      fd_alloc  (p, fd, min, __FUNCTION__)
# define FD_REMOVE(p, fd)          fd_remove (p, fd, __FUNCTION__)

long fp_alloc	(struct proc *p, FILEPTR **resultfp, const char *func);
void fp_done	(struct proc *p, FILEPTR *fp, short fd, char fdflags, const char *func);
void fp_free	(FILEPTR *fp, const char *func);

# define FP_ALLOC(p, result)       fp_alloc  (p, result, __FUNCTION__)
# define FP_DONE(p, fp, fd, flags) fp_done   (p, fp, fd, flags, __FUNCTION__)
# define FP_FREE(fp)               fp_free   (fp, __FUNCTION__)

long fp_get	(struct proc **p, short *fd, FILEPTR **fp, const char *func);
long fp_get1	(struct proc *p, short fd, FILEPTR **fp, const char *func);

# define FP_GET(p, fd, fp)         fp_get     (p, fd, fp, __FUNCTION__)
# define FP_GET1(p, fd, fp)        fp_get1    (p, fd, fp, __FUNCTION__)
# define GETFILEPTR(p, fd, fp)	   fp_get     (p, fd, fp, __FUNCTION__)

long do_dup	(short fd, short min, int cmd);
long do_open	(FILEPTR **f, const char *name, int rwmode, int attr, XATTR *x);
long do_close	(struct proc *p, FILEPTR *f);


# endif	/* _k_fds_h  */
