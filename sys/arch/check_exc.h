/*
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

# ifndef _check_exc_h
# define _check_exc_h

struct frame_zero
{
	ulong data_reg[8];
	ulong addr_reg[7];
	ushort sr;
	ushort *pc;
	ushort format_word;
};

struct privilege_violation_stackframe
{
	ulong data_reg[8];
	ulong addr_reg[7];
	ushort sr;
	ushort *pc;
	ushort format_word;
};
		

/* Every 68k processor uses different stack frame format for
 * access faults. Very funny...
 */

/* 68000 Special Status Word */
struct mc68000_ssw
{
	unsigned :11;
	unsigned rw:1;		/* Read/write (1 = read) */
	unsigned in:1;		/* Instruction/Not (1 = Not) */
	unsigned fc:3;		/* Function code (address space) */
};

struct mc68010_ssw
{
	unsigned :16;		/* Not yet */
};

/* 68020/68030 Special Status Word */
struct mc68030_ssw
{
	unsigned fc:1;		/* Fault on stage C */
	unsigned fb:1;		/* Fault on stage B */
	unsigned rc:1;		/* Rerun (1) stage C */
	unsigned rb:1;		/* Rerun (1) stage B */
	unsigned :3;		/* CPU internal bits */
	unsigned df:1;		/* Data fault, rerun (1) data cycle */
	unsigned rm:1;		/* Read-modify-write on data cycle */
	unsigned rw:1;		/* Read/write on data cycle (1 = read) */
	unsigned size:2;	/* Size */
	unsigned :1;		/* Internal use */
	unsigned as:3;		/* Address space for data cycle */
};

/* 68040 Special Status Word */
struct mc68040_ssw
{
	unsigned cp:1;		/* Continuation of FP post-instruction exception pending */
	unsigned cu:1;		/* Continuation of unimplemented FP instruction exception pending */
	unsigned ct:1;		/* Continuation of trace exception pending */
	unsigned cm:1;		/* Continuation of MOVEM instruction execution pending */
	unsigned ma:1;		/* Misaligned access */
	unsigned atc:1;		/* ATC fault */
	unsigned lk:1;		/* Locked transfer (read-modify-write) */
	unsigned rw:1;		/* Read/write on data cycle (1 = read) */
	unsigned :1;		/* Undefined */
	unsigned size:2;	/* Size */
	unsigned tt:2;		/* Transfer type */
	unsigned tm:3;		/* Transfer modifier */
};

/* 68060 Fault Status Long Word */
struct mc68060_fslw
{
	unsigned :4;
	unsigned ma:1;
	unsigned :1;
	unsigned lk:1;
	unsigned rw:2;
	unsigned size:2;
	unsigned tt:2;
	unsigned tm:2;
	unsigned io:1;
	unsigned pbe:1;
	unsigned sbe:1;
	unsigned pta:1;
	unsigned ptb:1;
	unsigned il:1;
	unsigned pf:9;
	unsigned sp:1;
	unsigned wp:1;
	unsigned twe:1;
	unsigned re:1;
	unsigned we:1;
	unsigned ttr:1;
	unsigned bpe:1;
	unsigned :1;
	unsigned see:1;
};

typedef struct mc68000_bus_frame MC68000_BUS_FRAME;
typedef struct mc68010_bus_frame MC68010_BUS_FRAME;
typedef struct mc68030_bus_frame_short MC68030_SHORT_FRAME;
typedef struct mc68030_bus_frame_long MC68030_LONG_FRAME;
typedef struct mc68040_bus_frame MC68040_BUS_FRAME;
typedef struct mc68060_bus_frame MC68060_BUS_FRAME;

/* 68000 bus error stack frame */
struct mc68000_bus_frame
{
	ulong data_reg[8];
	ulong addr_reg[7];
	struct mc68000_ssw ssw;
	ulong fault_address;
	ushort instruction_register;
	ushort sr;
	ushort *pc;
};
/* 68010 bus error stack frame, format $8 */
struct mc68010_bus_frame
{
	ulong data_reg[8];
	ulong addr_reg[7];
	ushort sr;
	ushort *pc;
	ushort format_word;
	struct mc68010_ssw ssw;
	ulong fault_address;
	ushort unused_1;
	ushort data_output_buffer;
	ushort unused_2;
	ushort data_input_buffer;
	ushort unused_3;
	ushort instruction_output_buffer;
	ushort version_number;
};

/* 68020/68030 short bus cycle fault stack frame, format $a */
struct mc68030_bus_frame_short
{
	ulong data_reg[8];
	ulong addr_reg[7];
	ushort sr;
	ushort *pc;
	ushort format_word;
	ushort int_1;
	struct mc68030_ssw ssw;
	ushort stage_c;
	ushort stage_b;
	ulong fault_address;
	ushort int_2;
	ushort int_3;

	union
	{
		uchar byte[4];
		ushort word[2];
		ulong one_long;
	} data_output_buffer;

	ushort int_4;
	ushort int_5;
};

/* 68020/68030 long bus cycle fault stack frame, format $b */
struct mc68030_bus_frame_long
{
	ulong data_reg[8];
	ulong addr_reg[7];
	ushort sr;
	ushort *pc;
	ushort format_word;
	ushort int_1;
	struct mc68030_ssw ssw;
	ushort stage_c;
	ushort stage_b;
	ulong fault_address;
	ushort int_2;
	ushort int_3;

	union
	{
		uchar byte[4];
		ushort word[2];
		ulong one_long;
	} data_output_buffer;

	ushort int_4;
	ushort int_5;
	ushort int_6;
	ushort int_7;
	ushort *stage_b_address;
	ushort int_8;
	ushort int_9;

	union
	{
		uchar byte[4];
		ushort word[2];
		ulong one_long;
	} data_input_buffer;

	ushort int_10;
	ushort int_11;
	ushort int_12;
	ushort version;		/* highest 4 bits is version number */
				/* 18 words of internal information follow */
};

/* 68040 access error stack frame */
struct mc68040_bus_frame
{
	ulong data_reg[8];
	ulong addr_reg[7];
	ushort sr;
	ushort *pc;
	ushort format_word;
	void *effective_address;
	struct mc68040_ssw ssw;
	ushort wb3s;			/* pending write-backs, crap */
	ushort wb2s;
	ushort wb1s;
	ulong fault_address;
	void *wb3a;
	ulong wb3d;
	void *wb2a;
	ulong wb2d;
	void *wb1a;
	ulong wb1d;
	ulong pd1;
	ulong pd2;
	ulong pd3;
};

/* 68060 access error stack frame */
struct mc68060_bus_frame
{
	ulong data_reg[8];
	ulong addr_reg[7];
	ushort sr;
	ushort *pc;
	ushort format_word;
	ulong fault_address;

	union
	{
		ushort *fault_pc;
		struct mc68060_fslw fslw;
	} bottom;
};

/* ColdFire exception stack frame */
struct coldfire_frame
{
	ulong data_reg[8];
	ulong addr_reg[7];
	ushort format_word;
	ushort sr;
	ushort *pc;
};

struct m68k_stack_frames
{
	union {
		struct frame_zero 			zero;
		struct privilege_violation_stackframe	privviol;
		struct mc68000_bus_frame		m68000_bus;
		struct mc68010_bus_frame		m68010_bus;
		struct mc68030_bus_frame_short		m68030_sbus;
		struct mc68030_bus_frame_long		m68030_lbus;
		struct mc68040_bus_frame		m68040_bus;
		struct mc68060_bus_frame		m68060_bus;
		struct coldfire_frame			coldfire;
	} type;
};

// long check_bus(struct frame_zero frame);
long _cdecl check_bus(struct m68k_stack_frames);
// long _cdecl check_priv(volatile struct privilege_violation_stackframe frame);
long _cdecl check_priv(struct privilege_violation_stackframe *frame);
# endif /* _check_exc_h */
