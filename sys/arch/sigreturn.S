/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. Its not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This file is free software| you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation| either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY| without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program| if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2000-11-08
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

| sig_return: user signal handlers return to us. At that point, the
| stack looks like this:
|  -4(sp)	(long) sfmt
|    (sp)	(long) signal number -- was a parameter for user routine
|
|
	.globl	_sig_return
	.globl	_pc_valid_return

_sig_return:	addq.w	#8,sp		| pop signal number and sfmt
	move.w	#0x11a,-(sp)		| Psigreturn() system call
	trap	#1
_pc_valid_return:			| tells kernel its us
| we had better not come back
| if we did, something terrible
| happened, and we might as well terminate
	move.w	#-998,-(sp)
	move.w	#0x4c,-(sp)		| Pterm()
	trap	#1
