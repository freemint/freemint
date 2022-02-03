/*
 * Copyright (c) 1983, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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

# ifndef NO_FAKE_SUPER

/* This module provides code, that allows old programs to run,
 * even if the Super() system call is modified so that it does
 * not have any real effect, i.e. the CPU remains in the user
 * mode after Super().
 *
 * 1) programs that look into GEMDOS variables ($0-$7ff) should
 *    still work.
 * 2) programs, that directly access hardware, won't.
 * 3) the trick with changing bus error vector temporarily to
 *    check the presence of a hardware register should still work
 *    (indicating that the register does not exist).
 *
 * WARNING: the XHDI vector MUST MUST MUST point to the caller's
 *	    memory area - i.e. to its trampoline. This is to be
 *	    done yet.
 */

/* Tables that provide information about addresses the user is allowed
 * to access. 0 signalizes the end of the table.
 *
 * CAUTION: DO NOT ADD addresses here, which may or may not exist
 * (like e.g. memory mapped FPU CIR $fffffa40)!
 */
static const ulong allow_read[] =
{
	0x00000008L,	/* bus error vector */
	0x00000446L,	/* bootdev */
	0x000004c2L,	/* drvbits */
	0x000005a0L,	/* cookie_p */

	0L		/* address 0 must always cause bus error */
};
static const ulong allow_write[] = { 0L };

static long
prove_fault_address(const ulong *access_table, ulong address)
{
	while (*access_table)
	{
		if (*access_table == address)
			return 1;
		access_table++;
	}

	return 0;
}

/* The routine returns 1 if the program caused a "real" bus fault
 * e.g. it tried to access a nonexistent memory location.
 * This is only important if the program installed own
 * bus error handler.
 */
static long
real_fault(ulong address)
{
 	/* If the fault address is in the hw register
 	 * area, we pretend that nothing exists there.
	 */
	 if ((address >= 0xfff00000L) || \
	 	((address >= 0x00f00000L) && \
	 		(address <= 0x00ffffffL)))
	 {
	 	return 1L;
	 }

	/* Non existent ST RAM address */
	if ((address >= *(unsigned long *)0x42eL /* phystop */) && \
		(address < 0x00e00000L))
	{
		return 1L;
	}

	/* Non existent TT RAM address */
	if (*(long *)0x05a8L == 0x1357bd13L)	/* ramvalid */
	{
		if (address > *(unsigned long *)0x05a4L)	/* ramtop */
			return 1L;
	}

	return 0L;
}

# ifdef M68000
static long
// handle_68000_bus_frame(MC68000_BUS_FRAME *frame)
handle_68000_bus_frame(struct m68k_stack_frames *frame)
{
	return 0;
}
static long
// handle_68010_bus_frame(MC68010_BUS_FRAME *frame)
handle_68010_bus_frame(struct m68k_stack_frames *frame)
{
	return 0;
}
# endif

static long
// handle_68030_short_frame(MC68030_SHORT_FRAME *frame)
handle_68030_short_frame(struct m68k_stack_frames *f)
{
	struct mc68030_bus_frame_short *frame = &f->type.m68030_sbus;

	/* We only handle data faults, instruction
	 * stream faults will cause bus error to be signalled.
	 */
	if (frame->ssw.df == 0)
		return 0;

	/* Read and read-modify-write cycles generate long
	 * frames only. So we only deal here with write cycles (I hope).
	 */

	/* Writes are a problem because programs can do the following:
	 *
	 * 	clr.l	-(sp)
	 *	move.w	#Super,-(sp)
	 *	trap	#1
	 *	addq.l	#6,sp
	 *
	 *	move.l	8.w,a0		; record bus error vector (access 1)
	 *	move.l	sp,a1		; record stack pointer
	 *	move.l	#berr,8.w	; replace the vector (access 2)
	 *	clr.l	d1
	 *	tst.l	address		; check if 'address' exists
	 *	moveq	#$01,d1
	 *
	 * berr:
	 *	move.l	a0,8.w		; restore bus error vector (access 3)
	 *	move.l	a1,sp		; restore stack pointer
	 *
	 *	move.l	d1,_var		; save the result
	 *
	 *	move.l	d0,-(sp)
	 *	move.w	#Super,-(sp)
	 *	trap	#1
	 *	addq.l	#6,sp
	 *
	 * This is less or more what Pure C library does to detect
	 * if there's a memory mapped FPU present. And does this always,
	 * without a regard whether the FPU library is linked or not.
	 * Very funny...
	 *
	 * Obviously, we cannot allow programs to change the REAL
	 * bus error vector, but we can try to emulate things so that
	 * such a practice (above) at least won't kill the program.
	 */

	/* We only allow long transfers */
	if ((frame->fault_address == 0x00000008L) && (frame->ssw.size == 0x02))
	{
		/* Check if program wants to restore the vector
		 * to the original (i.e. present, in fact) state.
		 * We always "allow" this. We understand that this
		 * is the "access 3" as on the listing above.
		 */
		if (frame->data_output_buffer.one_long == *(unsigned long *)0x0008L)
		{
			/* Reset flags in proc */
			curproc->p_flag &= ~P_FLAG_BER;
			curproc->berr = 0L;

			frame->ssw.df = 0;	/* do not rerun the bus cycle */

			return -1;		/* resume */
		}
		else
		{
			/* The program wants to overwrite the bus error
			 * vector with the new value. So this is hopefully
			 * the "access 2" on the above listing, and the
			 * original value of the vector is likely to be
			 * restored soon (if not, we can reset things once
			 * the program leaves the supervisor mode).
			 */
			curproc->berr = frame->data_output_buffer.one_long;

			frame->ssw.df = 0;	/* do not rerun the bus cycle */

			return -1;
		}
	}

	/* If the fault address is 'proved', we can complete
	 * the write cycle requested by program. Or we fall back
	 * to bus error otherwise.
	 *
	 * BUG: we loose, if the accessed location really doesn't
	 * exist (this is bus error handler, and this means double
	 * bus fault then, i.e. CPU halt).
	 *
	 */
	if (prove_fault_address(allow_write, frame->fault_address))
	{
		switch (frame->ssw.size)
		{
			case 0x01:	/* byte */
			{
				*(ulong *)frame->fault_address = frame->data_output_buffer.byte[3];

				frame->ssw.df = 0;	/* do not rerun the faulted bus cycle */

				return -1L;		/* resume */
			}
			case 0x02:	/* long */
			{
				*(ulong *)frame->fault_address = frame->data_output_buffer.word[1];

				frame->ssw.df = 0;	/* do not rerun the faulted bus cycle */

				return -1L;		/* resume */
			}
			case 0x03:	/* word */
			{
				*(ulong *)frame->fault_address = frame->data_output_buffer.one_long;

				frame->ssw.df = 0;	/* do not rerun the faulted bus cycle */

				return -1L;		/* resume */
			}
			default:
			{
				return 0;	/* huh?! */
			}
		}
	}
	else
	{
		/* The write access is not allowed, we must generate
		 * bus error, but it is to be decided, whether this
		 * has to be real bus error (which kills the process)
		 * or simulated bus error (which should cause a jump
		 * inside the program).
		 */
		 if (curproc->berr && (curproc->p_flag & P_FLAG_BER))
		 {
		 	if (real_fault(frame->fault_address))
		 	{
		 		/* Emulate the bus error for the user */
		 		frame->pc = (ushort *)curproc->berr;

				frame->ssw.df = 0;	/* do not rerun */

				return -1L;		/* resume */
		 	}

			/* Pretend success otherwise (writing nothing) */
			frame->ssw.df = 0;	/* do not rerun the faulted bus cycle */

			return -1L;		/* resume */
		 }
	}

	return 0;
}

static long
// handle_68030_long_frame(MC68030_LONG_FRAME *frame)
handle_68030_long_frame(struct m68k_stack_frames *f)
{
	struct mc68030_bus_frame_long *frame = &f->type.m68030_lbus;

	/* We only handle data faults, instruction
	 * stream faults will cause bus error to be signalled.
	 */
	if (frame->ssw.df == 0)
		return 0;

	/* We don't handle data faults caused by CAS, CAS2 or TAS
	 */
	if (frame->ssw.rm)
		return 0;

	/* 1 here means read cycle, 0 write cycle */
	if (frame->ssw.rw)
	{
		if (frame->fault_address == 0x00000008L)
		{
			/* This is special case, the program wants
			 * to read the bus error vector. We record
			 * this fact. See below (in writes).
			 *
			 * BUG: tst.l 0x0008.w will deceive us :(
			 */
			curproc->p_flag |= P_FLAG_BER;
		}

		/* If the fault address is 'proved', we can complete
		 * the read cycle requested by program. Or we fall back
		 * to bus error otherwise.
		 *
		 * BUG: we loose, if the accessed location really doesn't
		 * exist (this is bus error handler, and this means double
		 * bus fault then, i.e. CPU halt).
		 *
		 */
		if (prove_fault_address(allow_read, frame->fault_address))
		{
			switch (frame->ssw.size)
			{
				case 0x01:	/* byte */
				{
					frame->data_input_buffer.byte[3] = *(uchar *)frame->fault_address;

					frame->ssw.df = 0;	/* do not rerun the faulted bus cycle */

					return -1L;		/* resume */
				}
				case 0x02:	/* long */
				{
					frame->data_input_buffer.one_long = *(ulong *)frame->fault_address;

					frame->ssw.df = 0;	/* do not rerun the faulted bus cycle */

					return -1L;		/* resume */
				}
				case 0x03:	/* word */
				{
					frame->data_input_buffer.word[1] = *(ushort *)frame->fault_address;

					frame->ssw.df = 0;	/* do not rerun the faulted bus cycle */

					return -1L;		/* resume */
				}
				default:
				{
					return 0;	/* huh?! */
				}
			}
		}
		else
		{
			/* The read access is not allowed, we must generate
			 * bus error, but it is to be decided, whether this
			 * has to be real bus error (which kills the process)
			 * or simulated bus error (which should cause a jump
			 * inside the program).
			 */
			 if (curproc->berr && (curproc->p_flag & P_FLAG_BER))
			 {
			 	if (real_fault(frame->fault_address))
			 	{
			 		/* Emulate the bus error for the user */
			 		frame->pc = (ushort *)curproc->berr;

					frame->ssw.df = 0;	/* do not rerun */

					return -1L;		/* resume */
			 	}
			}
		}
	}
	else	/* write cycle */
	{
		/* The top part of the long frame is identical with
		 * short frame, so we can handle them both with one routine.
		 */
		return handle_68030_short_frame(f); //handle_68030_short_frame((MC68030_SHORT_FRAME *)frame);
	}

	/* All other cases: raise bus error signal */

	return 0;
}

static long
// handle_68040_frame(MC68040_BUS_FRAME *frame)
handle_68040_frame(struct m68k_stack_frames *f)
{
	struct mc68040_bus_frame *frame = &f->type.m68040_bus;

	/* We don't handle ATC faults */
	if (frame->ssw.atc)
		return 0;

	/* We don't handle the read-modify-write operations either */
	if (frame->ssw.lk)
		return 0;

	/* We only do "user data access" */
	if (frame->ssw.tm != 0x1)
		return 0;

	/* Read is 1, write is 0 */
	if (frame->ssw.rw)
	{
		if (frame->fault_address == 0x00000008L)
		{
			/* This is special case, the program wants
			 * to read the bus error vector. We record
			 * this fact. See below (in writes).
			 *
			 * BUG: tst.l 0x0008.w will deceive us :(
			 */
			curproc->p_flag |= P_FLAG_BER;
		}
	}
	else
	{
	}

	return 0;
}

static long
// handle_68060_frame(MC68060_BUS_FRAME *frame)
handle_68060_frame(struct m68k_stack_frames *f __attribute__((unused)))
{
	return 0;
}

/* We use the frame_zero type initially
 * to get access to the frame format word.
 */
long _cdecl
// check_bus(struct frame_zero frame)
check_bus(struct m68k_stack_frames frame)
{
	ushort frame_format = frame.type.zero.format_word & 0xf000;

	/* All this below only applies, when the program
	 * has called Super() or Supexec() and thinks that
	 * it is executing in supervisor mode.
	 */
	if ((curproc->p_flag & P_FLAG_SUPER) == 0)
		return 0;

# ifdef M68000
	if (mcpu == 0)
		return handle_68000_bus_frame(&frame); //((MC68000_BUS_FRAME *)&frame);
	else if (mcpu >= 10)
	{
		if (frame_format == 0x8000)
			return handle_68010_bus_frame(&frame); //((MC68010_BUS_FRAME *)&frame);
		else if (frame_format == 0xa000)
			return handle_68030_short_frame(&frame); //((MC68030_SHORT_FRAME *)&frame);
		else if (frame_format == 0xb000)
			return handle_68030_long_frame(&frame); //((MC68030_LONG_FRAME *)&frame);
		else if (frame_format == 0x7000)
			return handle_68040_frame(&frame); //((MC68040_BUS_FRAME *)&frame);
		else if (frame_format == 0x4000)
			return handle_68060_frame(&frame); //((MC68060_BUS_FRAME *)&frame);
	}
# else
	if (frame_format == 0xa000)
		return handle_68030_short_frame(&frame); //((MC68030_SHORT_FRAME *)&frame);
	else if (frame_format == 0xb000)
		return handle_68030_long_frame(&frame); //((MC68030_LONG_FRAME *)&frame);
	else if (frame_format == 0x7000)
		return handle_68040_frame(&frame); //((MC68040_BUS_FRAME *)&frame);
	else if (frame_format == 0x4000)
		return handle_68060_frame(&frame); //((MC68060_BUS_FRAME *)&frame);
# endif
	/* For all unknown processors and stack frame formats
	 * we let it go.
	 */
	return 0;
}

# endif /* NO_FAKE_SUPER */

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

long _cdecl
check_priv(struct privilege_violation_stackframe *frame)
{
# ifndef M68000
	/* Backward compatibility with older processors */
	bool emulate_obsolete_instructions = true;
# endif
	ushort opcode;
	opcode = *frame->pc;
	UNUSED(opcode);

# ifdef __mcoldfire__
	if (!coldfire_68k_emulation)
	{
		/* Fortunately, pure ColdFire programs are brand new
		 * and does not contain any obsolete instruction.
		 */
		emulate_obsolete_instructions = false;
	}
# endif

# ifndef M68000
	/* Emulate the "move from sr" instruction,
	 * which is not privileged on 68000, and privileged later.
	 * Thus many programs (even Thing Desktop) execute it in
	 * user mode, if they need an access to the condition codes.
	 */
	if (emulate_obsolete_instructions && (opcode & MODE_REG_MASK) == MOVE_FROM_SR)
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

				temp = frame->data_reg[reg];
				temp &= 0xffff0000L;
				temp |= (frame->sr & 0x20ff);
				frame->data_reg[reg] = temp;
				frame->pc++;

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
				frame->pc++;

				return -1;	/* reload registers and do RTE */
			}

			case 0x3:		/* MOVE SR,(An)+ */
			{
				frame->addr_reg[reg] += sizeof(short);
				frame->pc++;

				return -1;	/* reload registers and do RTE */
			}

			case 0x4:		/* MOVE SR,-(An) */
			{
				frame->addr_reg[reg] -= sizeof(short);
				frame->pc++;

				return -1;	/* reload registers and do RTE */
			}

			case 0x5:		/* MOVE SR,(d16,An) */
			{
				frame->pc += 2;

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
				frame->pc += (reg == 0) ? 2 : 3;

				return -1;	/* reload registers and RTE */
			}

			default:
			{
				return 0;
			}
		}
	}
# endif /* M68000 */

# ifndef NO_FAKE_SUPER

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

		imm = frame->pc[1];
		/* This can switch the CPU back to user mode, it is now
		 * safe to do that, so we emulate it as well.
		 */
		if ((imm & 0x2000) == 0)
			curproc->p_flag &= ~P_FLAG_SUPER;
		imm |= 0xffe0;
		frame->sr &= imm;

		frame->pc += 2;

		return -1;		/* reload everything and RTE */
	}

	/* eori.w #xxxx,SR: we generally do eori.w #xxxx,CCR instead
	 */
	if (opcode == EORI_TO_SR)
	{
		ushort imm;

		imm = frame->pc[1];
		/* This can switch the CPU back to user mode, it is now
		 * safe to do that, so we emulate it as well.
		 */
		if (imm & 0x2000)
			curproc->p_flag &= ~P_FLAG_SUPER;
		imm &= 0x001f;
		frame->sr ^= imm;

		frame->pc += 2;

		return -1;		/* reload everything and RTE */
	}

	/* ori.w #xxxx,SR: we generally do ori.w #xxxx,CCR instead
	 */
	if (opcode == ORI_TO_SR)
	{
		ushort imm;

		imm = frame->pc[1];
		imm &= 0x001f;
		frame->sr |= imm;

		frame->pc += 2;

		return -1;		/* reload everything and RTE */
	}

	/* Ignore this */
	if (opcode == RESET)
	{
		frame->pc++;

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

		imm = frame->pc[1];

		/* We let it go if STOP sets either of the trace bits.
		 */
		if ((imm & 0xe000) == 0x2000)
		{
			ipl = imm >> 8;
			ipl &= 7;
			if (ipl > 5)
				return 0;	/* make privilege violation */
		}
		frame->pc += 2;

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
			volatile ulong *register_set;

			imm = frame->pc[1];
			reg = imm >> 12;
			control = imm & 0x0fff;

			register_set = (reg & 0x8) ? frame->addr_reg : frame->data_reg;
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

						frame->pc += 2;

						return -1;
					}
					case 0x800:	/* User stack pointer */
					{
						register_set[reg] = get_usp();

						frame->pc += 2;

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

						frame->pc += 2;

						return -1;
					}
					case 0x802:	/* Cache address register */
					{
						if (mcpu == 20 || mcpu == 30)
						{
							register_set[reg] = 0;

							frame->pc += 2;

							return -1;
						}
						break;
					}
					case 0x803:
					case 0x804:
					{
						register_set[reg] = get_usp();

						frame->pc += 2;

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

						frame->pc += 2;

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
			ushort control = frame->pc[1] & 0x0fff;

			if (mcpu >= 10)
			{
				switch (control)
				{
					case 0x000:	/* Source function code */
					case 0x001:	/* Destination function code */
					case 0x801:	/* Vector base register */
					{
						frame->pc += 2;

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
						frame->pc += 2;

						return -1;
					}
					case 0x802:	/* Cache address register */
					{
						if (mcpu == 20 || mcpu == 30)
						{
							frame->pc += 2;

							return -1;
						}
						break;
					}
					case 0x803:
					case 0x804:
					{
						frame->pc += 2;

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
					case 0x005:	/* Instruction transparent translation register 1 */
					case 0x006:	/* Data transparent translation register 0 */
					case 0x007:	/* Data transparent translation register 1 */
					case 0x805:	/* MMUSR */
					case 0x806:	/* URP */
					case 0x807:	/* SRP */
					{
						frame->pc += 2;

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

# endif /* NO_FAKE_SUPER */

	return 0;
}
#if 0
long _cdecl
check_priv(volatile struct privilege_violation_stackframe frame)
{
	ushort opcode;
	opcode = *frame.pc;
# ifndef M68000
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
# endif /* M68000 */

# ifndef NO_FAKE_SUPER

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

		/* We let it go if STOP sets either of the trace bits.
		 */
		if ((imm & 0xe000) == 0x2000)
		{
			ipl = imm >> 8;
			ipl &= 7;
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
			volatile ulong *register_set;

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
					case 0x005:	/* Instruction transparent translation register 1 */
					case 0x006:	/* Data transparent translation register 0 */
					case 0x007:	/* Data transparent translation register 1 */
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

# endif /* NO_FAKE_SUPER */

	return 0;
}
#endif

/* EOF */
