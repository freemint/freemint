/*
 * $Id$
 *
 * Copyright (c) 1983, 1992, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Author: Konrad M. Kokoszkiewicz <draco@atari.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

# include <mint/mint.h>
# include <mint/proc.h>		/* struct proc */

# include <global.h>		/* mcpu */

# ifdef SUPER_TESTING
short safe_super = 0;
# endif

struct stackframe
{
	ulong data_reg[8];	/* it would be better to define a data register as union */
	ulong addr_reg[7];
	ushort sr;
	ushort *pc;
	ushort format_word;
	void *address;
};

struct access
{
	long address;
	long size;
};

/* Tables that provide information about addresses the user is allowed
 * to access. Size = 0 signalizes the end of the table.
 */
static struct access allow_read[] =
{
	{ 0x00000446L, sizeof(short) },	/* bootdev */
	{ 0x000004c2L, sizeof(long) },	/* drvbits */
	{ 0x000005a0L, sizeof(long) },	/* cookie_p */
	{ 0L, 0L }
};

static struct access allow_write[] =
{
	{ 0L, 0L }
};

long check_bus(struct stackframe frame);
long check_priv(struct stackframe frame);

/* Returns:
 *
 *  0 - handle the exception normally
 * -1 - reload everything and RTE
 *
 * After leaving the function with a negative value, the code must
 * reload registers and cause the execution to continue.
 */

long
check_priv(struct stackframe frame)
{
	ushort opcode;

	opcode = *frame.pc;

	/* Emulate the "move from sr" instruction,
	 * which is not privileged on 68000, and privileged later.
	 * Thus many programs (even Thing Desktop) execute it in
	 * user mode, if they need an access to the condition codes.
	 */
	if ((opcode & 0xffc0) == 0x40c0)	/* MOVE from SR */
	{
		ushort mode, reg;

		reg = opcode & 0x0007;
		mode = opcode >> 3;
		mode &= 0x0007;

		switch (mode)
		{
			/* Hide the IPL mask and trace bits, reveal the S bit */
			case 0x0:		/* MOVE SR,Dn */
			{
				ulong temp;

				temp = frame.data_reg[reg];
				temp &= 0xffff0000L;
				temp |= (frame.sr & 0x20ff);
				frame.data_reg[reg] = temp;
				frame.pc += 1;

				return -1;	/* reload registers and do RTE */
			}

			/* Here comes unsafe stuff, i.e. writes to the memory.
			 * We simply ignore them, because an attempt to fully
			 * emulate it would be both slow and dangerous.
			 *
			 * Programs should not use te instruction, so we are
			 * doing them a favour anyways, if we silently tolerate
			 * the offension.
			 */
			case 0x2:		/* MOVE SR,(An) */
			{
				frame.pc += 1;

				return -1;	/* reload registers and do RTE */
			}

			case 0x3:		/* MOVE SR,(An+) */
			{
				frame.addr_reg[reg] += sizeof(short);
				frame.pc += 1;

				return -1;	/* reload registers and do RTE */
			}

			case 0x4:		/* MOVE SR,-(An) */
			{
				frame.addr_reg[reg] -= sizeof(short);
				frame.pc += 1;

				return -1;	/* reload registers and do RTE */
			}

			case 0x5:		/* MOVE SR,(d16,An) */
			{
				frame.pc += 2;

				return -1;	/* reload registers and do RTE */
			}

			/* MOVE SR,(d8,An,Xn) */
			/* MOVE SR,(bd,An,Xn) - 68020+ */
			/* MOVE SR,([bd,An,Xn],od) - 68020+ */
			/* MOVE SR,([bd,An],Xn,od) - 68020+ */
			case 0x6:
			{
				return 0;	/* This makes them "really" privileged */
			}

			case 0x7:
			{
				/* "MOVE SR,x.w" and "MOVE SR,x.l" */
				frame.pc += (reg == 0) ? 2 : 3;

				return -1;	/* reload registers and RTE */
			}

			default:
			{
				return 0;
			}
		}
	}

	return 0;
}

/* EOF */
