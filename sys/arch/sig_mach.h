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

# ifndef _m68k_sig_mach_h
# define _m68k_sig_mach_h

# include "mint/mint.h"


int		sendsig		(ushort sig);
long	_cdecl	sys_psigreturn	(void);
long	_cdecl	sys_psigintr	(ushort vec, ushort sig);
void		sig_user	(ushort vec);
void		cancelsigintrs	(void);

void		bombs		(ushort sig);
void		exception	(ushort sig);

void		sigbus		(void);
void		sigaddr		(void);
void		sigill		(void);
void		sigpriv		(void);
void		sigfpe		(void);
void		sigtrap		(void);
void		haltformat	(void);
void		haltcpv		(void);


# endif /* _m68k_sig_mach_h */
