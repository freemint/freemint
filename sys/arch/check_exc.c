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

# include <arch/cpu.h>		/* get_usp() */

# include <global.h>		/* mcpu */

# include "check_exc.h"

/* Tables that provide information about addresses the user is allowed
 * to access. Size == 0 signalizes the end of the table.
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

static long
handle_68000_bus_frame(MC68000_BUS_FRAME *frame)
{
	return 0;
}

static long
handle_68010_bus_frame(MC68010_BUS_FRAME *frame)
{
	return 0;
}

static long
handle_68030_short_frame(MC68030_SHORT_FRAME *frame)
{
	return 0;
}

static long
handle_68030_long_frame(MC68030_LONG_FRAME *frame)
{
	return 0;
}

static long
handle_68040_frame(MC68040_BUS_FRAME *frame)
{
	return 0;
}

static long
handle_68060_frame(MC68060_BUS_FRAME *frame)
{
	return 0;
}

/* We use the frame_zero type initially
 * to get access to the frame format word.
 */
long
check_bus(struct frame_zero frame)
{
	/* All this below only applies, when the program
	 * has called Super() or Supexec() and thinks that
	 * it is executing in supervisor mode.
	 */
	if ((curproc->p_flag & P_FLAG_SUPER) == 0)
		return 0;

	if (mcpu == 0)
		return handle_68000_bus_frame((MC68000_BUS_FRAME *)&frame);
	else if (mcpu >= 10)
	{
		ushort frame_format = frame.format_word & 0xf000;

		if (frame_format == 0x8000)
			return handle_68010_bus_frame((MC68010_BUS_FRAME *)&frame);
		else if (frame_format == 0xa000)
			return handle_68030_short_frame((MC68030_SHORT_FRAME *)&frame);
		else if (frame_format == 0xb000)
			return handle_68030_long_frame((MC68030_LONG_FRAME *)&frame);
		else if (frame_format == 0x7000)
			return handle_68040_frame((MC68040_BUS_FRAME *)&frame);
		else if (frame_format == 0x4000)
			return handle_68060_frame((MC68060_BUS_FRAME *)&frame);
	}

	/* For all unknown processors and stack frame formats
	 * we let it go.
	 */
	return 0;
}

/* Our extended privilege violation handler.
 *
 * Returns:
 *
 *  0 - handle the exception normally
 * -1 - reload everything and RTE
 *
 * After leaving the function with a negative value, the code must
 * reload registers and cause the execution to continue.
 */

# define ANDI_TO_SR	0x027c
# define EORI_TO_SR	0x0a7c
# define ORI_TO_SR	0x007c

# define MOVE_FROM_SR	0x40c0
# define MOVE_TO_SR	0x46c0
# define MODE_REG_MASK	0xffc0

# define RESET		0x4e70
# define STOP		0x4e72

# define MOVEC_C2G	0x4e7a
# define MOVEC_G2C	0x4e7b

long
check_priv(struct privilege_violation_stackframe frame)
{
	ushort opcode;

	opcode = *frame.pc;

	/* Emulate the "move from sr" instruction,
	 * which is not privileged on 68000, and privileged later.
	 * Thus many programs (even Thing Desktop) execute it in
	 * user mode, if they need an access to the condition codes.
	 */
	if ((opcode & MODE_REG_MASK) == MOVE_FROM_SR)
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
				frame.pc++;

				return -1;	/* reload registers and do RTE */
			}

			/* Here comes unsafe stuff, i.e. writes to the memory.
			 * We simply ignore them, because an attempt to fully
			 * emulate it would be both slow and dangerous.
			 *
			 * Programs should not use the instruction, so we are
			 * doing them a favour anyways, if we silently tolerate
			 * the offension.
			 */
			case 0x2:		/* MOVE SR,(An) */
			{
				frame.pc++;

				return -1;	/* reload registers and do RTE */
			}

			case 0x3:		/* MOVE SR,(An)+ */
			{
				frame.addr_reg[reg] += sizeof(short);
				frame.pc++;

				return -1;	/* reload registers and do RTE */
			}

			case 0x4:		/* MOVE SR,-(An) */
			{
				frame.addr_reg[reg] -= sizeof(short);
				frame.pc++;

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

	/* We mimic the original behaviour: some instructions are only
	 * available after Super(), so we must now emulate them, when
	 * letting programs continue in user mode after Super().
	 */
	if ((curproc->p_flag & P_FLAG_SUPER) == 0)
		return 0;

	/* andi.w #xxxx,SR: we generally do andi.w #xxxx,CCR instead
	 */
	if (opcode == ANDI_TO_SR)
	{
		ushort imm;

		imm = frame.pc[1];
		/* This can switch the CPU back to user mode, it is now
		 * safe to do that, so we emulate it as well.
		 */
		if ((imm & 0x2000) == 0)
			curproc->p_flag &= ~P_FLAG_SUPER;
		imm |= 0xffe0;
		frame.sr &= imm;

		frame.pc += 2;

		return -1;		/* reload everything and RTE */
	}

	/* eori.w #xxxx,SR: we generally do eori.w #xxxx,CCR instead
	 */
	if (opcode == EORI_TO_SR)
	{
		ushort imm;

		imm = frame.pc[1];
		/* This can switch the CPU back to user mode, it is now
		 * safe to do that, so we emulate it as well.
		 */
		if (imm & 0x2000)
			curproc->p_flag &= ~P_FLAG_SUPER;
		imm &= 0x001f;
		frame.sr ^= imm;

		frame.pc += 2;

		return -1;		/* reload everything and RTE */
	}

	/* ori.w #xxxx,SR: we generally do ori.w #xxxx,CCR instead
	 */
	if (opcode == ORI_TO_SR)
	{
		ushort imm;

		imm = frame.pc[1];
		imm &= 0x001f;
		frame.sr |= imm;

		frame.pc += 2;

		return -1;		/* reload everything and RTE */
	}

	/* Ignore this */
	if (opcode == RESET)
	{
		frame.pc++;

		return -1;
	}

	/* Ignore STOP instruction, unless the user wants to set the IPL to
	 * 6 or 7. This means that the user wants to lock the machine up
	 * totally, and it may be that there's not even any valid code
	 * behind the instruction. So we kill the process.
	 */
	if (opcode == STOP)
	{
		ushort imm, ipl;

		imm = frame.pc[1];
		if (imm & 0x2000)
		{
			ipl = imm >> 8;
			ipl &= 3;
			if (ipl > 5)
				return 0;	/* make privilege violation */
		}
		frame.pc += 2;

		return -1;
	}

	/* MOVEC is available as of 68010 */
	if (mcpu)
	{
		/* All data read from the control registers we read as zeros
		 * except user stack pointer.
		 */
		if (opcode == MOVEC_C2G)
		{
			ushort imm, reg, control;
			ulong *register_set;

			imm = frame.pc[1];
			reg = imm >> 12;
			control = imm & 0x0fff;

			register_set = (reg & 0x8) ? frame.addr_reg : frame.data_reg;
			reg &= 7;

			if (mcpu >= 10)
			{
				switch (control)
				{
					case 0x000:	/* Source function code */
					case 0x001:	/* Destination function code */
					case 0x801:	/* Vector base register */
					{
						register_set[reg] = 0L;	/* pretend 0 */

						frame.pc += 2;

						return -1;
					}
					case 0x800:	/* User stack pointer */
					{
						register_set[reg] = get_usp();

						frame.pc += 2;

						return -1;
					}
				}
			}

			if (mcpu >= 20)
			{
				switch (control)
				{
					case 0x002:	/* Cache control register */
					{
						register_set[reg] = 0L;	/* pretend 0 */

						frame.pc += 2;

						return -1;
					}
					case 0x802:	/* Cache address register */
					{
						if (mcpu == 20 || mcpu == 30)
						{
							register_set[reg] = 0;

							frame.pc += 2;

							return -1;
						}
						break;
					}
					case 0x803:
					case 0x804:
					{
						register_set[reg] = get_usp();
						
						frame.pc += 2;

						return -1;
					}
				}			
			}

			if (mcpu >= 40)
			{
				switch (control)
				{
					case 0x003:	/* MMU translation control register */
					case 0x004:	/* Instruction transparent translation register 0 */
					case 0x005:	/* Instruction transparent translation register 0 */
					case 0x006:	/* Data transparent translation register 0 */
					case 0x007:	/* Data transparent translation register 0 */
					case 0x805:	/* MMUSR */
					case 0x806:	/* URP */
					case 0x807:	/* SRP */
					{
						register_set[reg] = 0;

						frame.pc += 2;

						return -1;
					}
				}
			}

			if (mcpu == 60)
			{
			}
		}

		/* We ignore all writes to control registers */
		if (opcode == MOVEC_G2C)
		{
			ushort control = frame.pc[1] & 0x0fff;

			if (mcpu >= 10)
			{
				switch (control)
				{
					case 0x000:	/* Source function code */
					case 0x001:	/* Destination function code */
					case 0x801:	/* Vector base register */
					{
						frame.pc += 2;

						return -1;
					}
				}
			}

			if (mcpu >= 20)
			{
				switch (control)
				{
					case 0x002:	/* Cache control register */
					{
						frame.pc += 2;

						return -1;
					}
					case 0x802:	/* Cache address register */
					{
						if (mcpu == 20 || mcpu == 30)
						{
							frame.pc += 2;

							return -1;
						}
						break;
					}
					case 0x803:
					case 0x804:
					{
						frame.pc += 2;

						return -1;
					}
				}			
			}

			if (mcpu >= 40)
			{
				switch (control)
				{
					case 0x003:	/* MMU translation control register */
					case 0x004:	/* Instruction transparent translation register 0 */
					case 0x005:	/* Instruction transparent translation register 0 */
					case 0x006:	/* Data transparent translation register 0 */
					case 0x007:	/* Data transparent translation register 0 */
					case 0x805:	/* MMUSR */
					case 0x806:	/* URP */
					case 0x807:	/* SRP */
					{
						frame.pc += 2;

						return -1;
					}
				}
			}

			if (mcpu == 60)
			{
			}

			return -1;
		}
	}

	return 0;
}

/* EOF */
