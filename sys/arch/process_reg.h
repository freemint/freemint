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
 * Started: 2000-10-08
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _m68k_process_reg_h
# define _m68k_process_reg_h

# include "mint/mint.h"
# include "mint/proc.h"

# include "mint/arch/register.h"


long process_single_step	(struct proc *p, int flag);
long process_set_pc		(struct proc *p, long pc);
long process_getregs		(struct proc *p, struct reg *reg);
long process_setregs		(struct proc *p, struct reg *reg);
long process_getfpregs		(struct proc *p, struct fpreg *fpreg);
long process_setfpregs		(struct proc *p, struct fpreg *fpreg);


# endif /* _m68k_process_reg_h */
