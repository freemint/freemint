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
 * begin:	2001-01-13
 * last change:	2001-01-13
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _k_fds_h
# define _k_fds_h

# include "mint/mint.h"
# include "mint/file.h"


long fd_alloc	(struct proc *p, short *fd, short min);
void fd_remove	(struct proc *p, short fd);
long fp_alloc	(struct proc *p, FILEPTR **resultfp);
void fp_done	(struct proc *p, FILEPTR *fp, short fd, char fdflags);
void fp_free_	(FILEPTR *fp, const char *func);
# define fp_free(fp) fp_free_ (fp, __FUNCTION__)
long fp_get	(struct proc **p, short *fd, FILEPTR **fp, const char *func);
long fp_get1	(struct proc *p, short fd, FILEPTR **fp, const char *func);

# define GETFILEPTR(p, fd, fp)	fp_get (p, fd, fp, __FUNCTION__)

long do_dup	(short fd, short min);
long do_open	(FILEPTR **f, const char *name, int rwmode, int attr, XATTR *x);
long do_close	(struct proc *p, FILEPTR *f);


# endif	/* _k_fds_h  */
