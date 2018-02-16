/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 1999, 2000 Guido Flohr <guido@freemint.de>
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
 * Author: Guido Flohr <guido@freemint.de>
 * Started: 1999-10-24
 *
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
 *
 * Kernel system info filesystem, information retrieval routines.
 *
 */

# ifndef _kernget_h
# define _kernget_h

# include "mint/mint.h"


# if WITH_KERNFS

long kern_get_unimplemented	(SIZEBUF **buffer);

long kern_get_bootlog (SIZEBUF **buffer);
long kern_get_buildinfo		(SIZEBUF **buffer);
long kern_get_cookiejar		(SIZEBUF **buffer);
long kern_get_filesystems	(SIZEBUF **buffer);
long kern_get_hz		(SIZEBUF **buffer);
long kern_get_loadavg		(SIZEBUF **buffer);
long kern_get_meminfo		(SIZEBUF **buffer);
long kern_get_stat              (SIZEBUF **buffer);
long kern_get_sysdir		(SIZEBUF **buffer);
long kern_get_time		(SIZEBUF **buffer);
long kern_get_uptime		(SIZEBUF **buffer);
long kern_get_version		(SIZEBUF **buffer);
long kern_get_welcome		(SIZEBUF **buffer);

long kern_procdir_get_cmdline	(SIZEBUF **buffer, const struct proc *p);
long kern_procdir_get_environ	(SIZEBUF **buffer,       struct proc *p);
long kern_procdir_get_fname	(SIZEBUF **buffer, const struct proc *p);
long kern_procdir_get_meminfo	(SIZEBUF **buffer, const struct proc *p);
long kern_procdir_get_stat	(SIZEBUF **buffer,       struct proc *p);
long kern_procdir_get_statm     (SIZEBUF **buffer,	 struct proc *p);
long kern_procdir_get_status	(SIZEBUF **buffer, const struct proc *p);

# endif


# endif /* _kernget_h */
