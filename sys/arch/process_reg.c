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
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# include "process_reg.h"

# include "libkern/libkern.h"


#define	PSL_C		0x0001		/* carry bit */
#define	PSL_V		0x0002		/* overflow bit */
#define	PSL_Z		0x0004		/* zero bit */
#define	PSL_N		0x0008		/* negative bit */
#define	PSL_X		0x0010		/* extend bit */
#define	PSL_ALLCC	0x001F		/* all cc bits - unlikely */
#define	PSL_IPL0	0x0000		/* interrupt priority level 0 */
#define	PSL_IPL1	0x0100		/* interrupt priority level 1 */
#define	PSL_IPL2	0x0200		/* interrupt priority level 2 */
#define	PSL_IPL3	0x0300		/* interrupt priority level 3 */
#define	PSL_IPL4	0x0400		/* interrupt priority level 4 */
#define	PSL_IPL5	0x0500		/* interrupt priority level 5 */
#define	PSL_IPL6	0x0600		/* interrupt priority level 6 */
#define	PSL_IPL7	0x0700		/* interrupt priority level 7 */
#define	PSL_M		0x1000		/* master (kernel) sp vs intr sp */
#define	PSL_S		0x2000		/* supervisor enable bit */
/*	PSL_T0		0x4000		   ??? T0 on 68020, 8000 is T1 */
#define	PSL_T		0x8000		/* trace enable bit */

#define	PSL_LOWIPL	(PSL_S)
#define	PSL_HIGHIPL	(PSL_S | PSL_IPL7)
#define PSL_IPL		(PSL_IPL7)
#define	PSL_USER	(0)

#define	PSL_MBZ		0xFFFF58E0	/* must be zero bits */

#define	PSL_USERSET	(0)
#define	PSL_USERCLR	(PSL_S | PSL_IPL7 | PSL_MBZ)

#define	USERMODE(ps)	(((ps) & PSL_S) == 0)

long
process_single_step (struct proc *p, int flag)
{
	if (flag)
		p->ctxt[SYSCALL].sr |= PSL_T;
	else
		p->ctxt[SYSCALL].sr &= ~PSL_T;
	
	return 0;
}

long
process_set_pc (struct proc *p, long pc)
{
	p->ctxt[SYSCALL].pc = pc;
	
	return 0;
}

long
process_getregs (struct proc *p, struct reg *reg)
{
	reg->regs[ 0] = p->ctxt[SYSCALL].regs[ 0];
	reg->regs[ 1] = p->ctxt[SYSCALL].regs[ 1];
	reg->regs[ 2] = p->ctxt[SYSCALL].regs[ 2];
	reg->regs[ 3] = p->ctxt[SYSCALL].regs[ 3];
	reg->regs[ 4] = p->ctxt[SYSCALL].regs[ 4];
	reg->regs[ 5] = p->ctxt[SYSCALL].regs[ 5];
	reg->regs[ 6] = p->ctxt[SYSCALL].regs[ 6];
	reg->regs[ 7] = p->ctxt[SYSCALL].regs[ 7];
	reg->regs[ 8] = p->ctxt[SYSCALL].regs[ 8];
	reg->regs[ 9] = p->ctxt[SYSCALL].regs[ 9];
	reg->regs[10] = p->ctxt[SYSCALL].regs[10];
	reg->regs[11] = p->ctxt[SYSCALL].regs[11];
	reg->regs[12] = p->ctxt[SYSCALL].regs[12];
	reg->regs[13] = p->ctxt[SYSCALL].regs[13];
	reg->regs[14] = p->ctxt[SYSCALL].regs[14];
	reg->regs[15] = p->ctxt[SYSCALL].usp;
	reg->sr       = p->ctxt[SYSCALL].sr;
	reg->pc       = p->ctxt[SYSCALL].pc;
	
	return 0;
}

long
process_setregs (struct proc *p, struct reg *reg)
{
	if ((reg->sr & PSL_USERCLR) != 0 ||
	    (reg->sr & PSL_USERSET) != PSL_USERSET)
		return EPERM;
	
	p->ctxt[SYSCALL].regs[ 0] = reg->regs[ 0];
	p->ctxt[SYSCALL].regs[ 1] = reg->regs[ 1];
	p->ctxt[SYSCALL].regs[ 2] = reg->regs[ 2];
	p->ctxt[SYSCALL].regs[ 3] = reg->regs[ 3];
	p->ctxt[SYSCALL].regs[ 4] = reg->regs[ 4];
	p->ctxt[SYSCALL].regs[ 5] = reg->regs[ 5];
	p->ctxt[SYSCALL].regs[ 6] = reg->regs[ 6];
	p->ctxt[SYSCALL].regs[ 7] = reg->regs[ 7];
	p->ctxt[SYSCALL].regs[ 8] = reg->regs[ 8];
	p->ctxt[SYSCALL].regs[ 9] = reg->regs[ 9];
	p->ctxt[SYSCALL].regs[10] = reg->regs[10];
	p->ctxt[SYSCALL].regs[11] = reg->regs[11];
	p->ctxt[SYSCALL].regs[12] = reg->regs[12];
	p->ctxt[SYSCALL].regs[13] = reg->regs[13];
	p->ctxt[SYSCALL].regs[14] = reg->regs[14];
	p->ctxt[SYSCALL].usp      = reg->regs[15];
	p->ctxt[SYSCALL].sr       = reg->sr;
	p->ctxt[SYSCALL].pc       = reg->pc;
	
	return 0;
}

long
process_getfpregs (struct proc *p, struct fpreg *fpreg)
{
	bcopy (p->ctxt[SYSCALL].fregs, fpreg->regs, sizeof (fpreg->regs));
	fpreg->fpcr = p->ctxt[SYSCALL].fctrl[0];
	fpreg->fpsr = p->ctxt[SYSCALL].fctrl[1];
	fpreg->fpiar = p->ctxt[SYSCALL].fctrl[2];
	
	return 0;
}

long
process_setfpregs (struct proc *p, struct fpreg *fpreg)
{
	bcopy (fpreg->regs, p->ctxt[SYSCALL].fregs, sizeof (fpreg->regs));
	p->ctxt[SYSCALL].fctrl[0] = fpreg->fpcr;
	p->ctxt[SYSCALL].fctrl[1] = fpreg->fpsr;
	p->ctxt[SYSCALL].fctrl[2] = fpreg->fpiar;
	
	return 0;
}
