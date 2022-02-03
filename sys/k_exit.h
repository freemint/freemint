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

# ifndef _k_exit_h
# define _k_exit_h

# include "mint/mint.h"
# include "mint/proc.h"


long 		terminate	(struct proc *p, short code, short que);
long _cdecl	sys_pterm	(short code);
long		kernel_pterm	(struct proc *p, short code);
long _cdecl	sys_pterm0	(void);
long _cdecl	sys_ptermres	(long save, short code);

long		pwaitpid	(short pid, short nohang, long *rusage, short *retval);
long _cdecl	sys_pwaitpid	(short pid, short nohang, long *rusage);
long _cdecl	sys_pwait3	(short nohang, long *rusage);
long _cdecl	sys_pwait	(void);


# endif /* _k_exit_h */
