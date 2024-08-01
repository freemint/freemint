/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2004 Holger Schulz <HolgerSchulz@t-online.de>
 * All rights reserved.
 *
 * Modified for FreeMiNT by Frank Naumann <fnaumann@freemint.de>
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 *
 * This file is free software  you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation  either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY  without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program  if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 * emulator for unimplemented FPU and integer instructions of the 68060
 * version 0.05 from 15.03.2004
 */

#include "mint/asmdefs.h"
#define fpsr REG(fpsr)
#define fpcr REG(fpcr)

	.text

	.globl _init

/*
 * not a C symbol: this is directly referenced in the Makefile
 */
_init:
	movea.l	4(sp),a5
	move.l	12(a5),d6		//   Groesse des TEXT-Segment
	add.l	20(a5),d6		// + Groesse des DATA-Segments
	add.l	28(a5),d6		// + Groesse des BSS-Segment
	addi.l	#0x100,d6		// + Groesse der BASEPAGE

	// nicht benoetigten Speicher frei geben
 
	move.l	d6,-(sp)
	move.l	a5,-(sp)
	clr.w	-(sp)
	move.w	#74,-(sp)		// Mshrink
	trap	#1
	add.l	#12,sp

	clr.l	-(sp)			// in Supervisormodus umschalten
	move.w	#32,-(sp)		// Super
	trap	#1
	addq.l	#6,sp
	move.l	d0,d7			// Super-Stack-Pointer merken

// nach vorhandener CPU suchen
// kein 68060 -> raus

	movea.l	0x5A0,a5		// cookie jar
cpu_ident:
	move.l	(a5)+,d0		// id
	beq.b	no_cpu060
	move.l	(a5)+,d1		// value
	cmp.l	#0x5F435055,d0		// '_CPU' ?
	bne.b	cpu_ident
	cmp.l	#60,d1			// 68060 ?
	beq.b	cpu_060

// keinen 68060 mit CPU gefunden

no_cpu060:
	move.l	d7,-(sp)		// alter SSP
	move.w	#32,-(sp)		// Super
	trap	#1
	addq.l	#6,sp

	clr.w	-(sp)			// Pterm(0)
	trap	#1

// ----------------------------------------------------------------------------------
// entry for the 68060 CPU
// ----------------------------------------------------------------------------------
cpu_060:

	move.l	0xF4,old_vec1		// alten Vektor sichern
	move.l	#_top_isp+0x80,0xf4	// Unimplemented Integer Instruction

	dc.l	0x4e7a0808		// movec.l  PCR, d0
	btst	#16,d0
	bne	set_res			// EC or LC

	// die FPU-Vektoren werden nur gesetzt, wenn auch eine da ist!

	move.l	0x2c,old_vec		// alten LINE-F-Vektor merken

	move.l	#_top_fpsp+0x80+0x30,0x2c	// Floating-Point Unimplemented Instruction
	move.l	#_top_fpsp+0x80+0x00,0xd8	// Floating-Point SNAN
	move.l	#_top_fpsp+0x80+0x08,0xD0	// Floating-Point Operand Error
	move.l	#_top_fpsp+0x80+0x10,0xD4	// Floating-Point Overflow
	move.l	#_top_fpsp+0x80+0x18,0xCC	// Floating-Point Underflow
	move.l	#_top_fpsp+0x80+0x20,0xC8	// Floating-Point Divide-by-Zero
	move.l	#_top_fpsp+0x80+0x28,0xC4	// Floating-Point Inexact Result
	move.l	#_top_fpsp+0x80+0x38,0xDC	// Floating-Point Unimplemented Data Type
	move.l	#_top_fpsp+0x80+0x40,0xF0	// Unimplemented Effective Address

	fmove.l	#0,fpcr

set_res:				// Programm resident halten und beenden

	lea	text_01,a0		// Startmeldung
	bsr	print			// ausgeben

	move.l	d7,-(sp)		// in alten Modus zurueckschalten
	move.w	#32,-(sp)		// Super
	trap	#1
	addq.l	#6,sp

	clr.w	-(sp)			// kein Fehler
	move.l	d6,-(sp)		// Anzahl residenter Bytes
	move.w	#49,-(sp)		// Ptermres
	trap	#1

// -------------------------------------------------------------------------------
// Meldung ausgeben
// IN  : a0    Adresse des Textes
// -------------------------------------------------------------------------------
print:
	move.l	a0,-(sp)
	move.w	#9,-(sp)		// Cconws
	trap	#1
	addq.l	#6,sp
	rts

text_01:
	.ascii	"*********************************************\15\12"
	.ascii	" Unimplemented Integer Instruction and       \15\12"
	.ascii	" Floating Point Handler Version 0.05         \15\12"
	.ascii	" Production Release P1.01 - October 10, 1994 \15\12"
	.ascii	" M68060 Software Package                     \15\12"
	.ascii	" Copyright (c) 1993, 1994 Motorola Inc.      \15\12"
	.ascii	" All rights reserved                         \15\12"
	.ascii	"*********************************************\15\12"
	.ascii	"\0"

	.even

	dc.l	0x58425241	// XBRA
	dc.l	0x4D435350	// MCSP
old_vec:
	dc.l	0

	dc.l	0x58425241	// XBRA
	dc.l	0x4D435350	// MCSP
old_vec1:
	dc.l	0

// -------------------------------------------------------------------------------------------
// Hier beginnen die Routinen zum Zugriff aus dem ISP
// -------------------------------------------------------------------------------------------

_060_real_cas:
	bra.l	_top_isp+0x80+0x08

_060_real_cas2:
	bra.l	_top_isp+0x80+0x10


// -------------------------------------------------------------------------------------------
// memory access routines for the FPSP
// -------------------------------------------------------------------------------------------

// _060_imem_write/_060_dmem_write()
//
// Writes to instruction/data memory.
//
// INPUTS:
//	a0 - source address	
//	a1 - destination address
//	d0 - number of bytes to write	
//
// OUTPUTS:
//	d1 - 0 = success, !0 = failure
//
// _060_imem_read(), _060_dmem_read():
//
// Reads from instruction/data memory.
//
// INPUTS:
//	a0 - source address
//	a1 - destination address
//	d0 - number of bytes to read
//
// OUTPUTS:
//	d1 - 0 = success, !0 = failure
//

_060_imem_read:
_060_dmem_read:
_060_imem_write:
_060_dmem_write:
	jmp	([copy_tab, pc, d0*4], 0.w)

copy_tab:
	dc.l	_copy00
	dc.l	_copy01
	dc.l	_copy02
	dc.l	_copy03
	dc.l	_copy04
	dc.l	_copy05
	dc.l	_copy06
	dc.l	_copy07
	dc.l	_copy08
	dc.l	_copy09
	dc.l	_copy10
	dc.l	_copy11
	dc.l	_copy12

_copy01:
	move.b	(a0)+,(a1)+		// 1 Byte kopieren

_copy00:				// nichts zu kopieren
	clr.l	d1
	rts

_copy03:
	move.b	(a0)+,(a1)+		// 3 Byte kopieren

_copy02:
	move.w	(a0)+,(a1)+		// 2 Byte kopieren
	clr.l	d1
	rts

_copy05:
	move.b	(a0)+,(a1)+

_copy04:
	move.l	(a0)+,(a1)+
	clr.l	d1
	rts

_copy07:
	move.b	(a0)+,(a1)+

_copy06:
	move.w	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	clr.l	d1
	rts

_copy09:
	move.b	(a0)+,(a1)+

_copy08:
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	clr.l	d1
	rts

_copy11:
	move.b	(a0)+,(a1)+

_copy10:
	move.w	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	clr.l	d1
	rts

_copy12:
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	clr.l	d1
	rts


// _060_dmem_read_byte():
//
// Read a data byte from memory.
//
// INPUTS:
//	a0 - source address
//
// OUTPUTS:
//	d0 - data byte in d0
//	d1 - 0 = success, !0 = failure
//
_060_dmem_read_byte:
	clr.l	d0			// clear whole longword
	move.b	(a0),d0			// fetch byte
	clr.l	d1			// return success
	rts

// _060_dmem_read_word():
//
// Read a data word from memory.
//
// INPUTS:
//	a0 - source address
//
// OUTPUTS:
//	d0 - data word in d0
//	d1 - 0 = success, !0 = failure
//
_060_dmem_read_word:
	clr.l	d0			// clear whole longword
	move.w	(a0),d0			// fetch super word
	clr.l	d1			// return success
	rts

// _060_dmem_read_long():
//
// INPUTS:
//	a0 - source address
//
// OUTPUTS:
//	d0 - data longword in d0
//	d1 - 0 = success, !0 = failure
//
_060_dmem_read_long:
	move.l	(a0),d0			// fetch super longword
	clr.l	d1			// return success
	rts

// _060_dmem_write_byte():
//
// Write a data byte to memory.
//
// INPUTS:
//	a0 - destination address
// 	d0 - data byte in d0
//
// OUTPUTS:
//	d1 - 0 = success, !0 = failure
//
_060_dmem_write_byte:
	move.b	d0,(a0)			// store super byte
	clr.l	d1			// return success
	rts

// _060_dmem_write_word():
//
// Write a data word to memory.
//
// INPUTS:
//	a0 - destination address
// 	d0 - data word in d0
//
// OUTPUTS:
//	d1 - 0 = success, !0 = failure
//
_060_dmem_write_word:
	move.w	d0,(a0)			// store super word
	clr.l	d1			// return success
	rts

// _060_dmem_write_long():
//
// Write a data longword to memory.
//
// INPUTS:
//	a0 - user destination address
// 	d0 - data longword in d0
//
// OUTPUTS:
//	d1 - 0 = success, !0 = failure
//
_060_dmem_write_long:
	move.l	d0,(a0)			// store super longword
	clr.l	d1			// return success
	rts

// _060_imem_read_word():
//
// Read an instruction word from memory.
//
// INPUTS:
//	a0 - source address
//
// OUTPUTS:
//	d0 - instruction word in d0
//	d1 - 0 = success, !0 = failure
//
_060_imem_read_word:
	move.w	(a0),d0			// fetch super word
	clr.l	d1			// return success
	rts

// _060_imem_read_long():
//
// Read an instruction longword from memory.
//
// INPUTS:
//	a0 - source address
//
// OUTPUTS:
//	d0 - instruction longword in d0
//	d1 - 0 = success, !0 = failure
//
_060_imem_read_long:
	move.l	(a0),d0			// fetch super longword
	clr.l	d1			// return success
	rts

_060_real_trace:
	move.l	0x24,-(sp)
	rts

_060_real_access:
	move.l	0x08,-(sp)
	rts

// _060_real_fline():
//
// This is the exit point for the 060FPSP when an F-Line Illegal exception
// is encountered. Three different types of exceptions can enter the F-Line 
// exception vector number 11: FP Unimplemented Instructions, FP implemented
// instructions when the FPU is disabled, and F-Line Illegal instructions.
// The 060FPSP module _fpsp_fline() distinguishes between the three and acts
// appropriately. F-Line
// Illegals branch here.
//
_060_real_fline:
	move.l	old_vec,-(sp)
	rts

// _060_real_trap():
//
// This is the exit point for the 060FPSP when an emulated 'ftrapcc'
// instruction discovers that the trap condition is true and it should
// branch to the operating system handler for the trap exception vector
// number 7.
//
// The sample code below simply executes an 'rte'.
//
_060_real_trap:
	rte

// _060_fpsp_done():
//
// This is the main exit point for the 68060 Floating-Point
// Software Package. For a normal exit, all 060FPSP routines call this
// routine. The operating system can do system dependent clean-up or
// simply execute an 'rte' as with the sample code below.
//
_060_fpsp_done:
	rte

// _060_real_bsun():
//
// This is the exit point for the 060FPSP when an enabled bsun exception
// is present. The routine below should point to the operating system handler
// for enabled bsun exceptions. The exception stack frame is a bsun
// stack frame.
//
// The sample routine below clears the exception status bit, clears the NaN
// bit in the FPSR, and does an rte. The instruction that caused the
// bsun will now be re-executed but with the NaN FPSR bit cleared.
//
_060_real_bsun:
	fsave		-(sp)
	fmovem.l	fpsr,-(sp)
	andi.b		#0xfe,(sp)
	fmovem.l	(sp)+,fpsr
	lea		12(sp),sp
	fmovem.l	#0,fpcr
	rte

// _060_real_snan():
//
// This is the exit point for the 060FPSP when an enabled signalling NaN exception
// is present. The routine below should point to the operating system handler
// for enabled signalling NaN exceptions. The exception stack frame is a signalling NaN
// stack frame. The FP state frame holds the source operand of the faulting
// instruction.
//
// The sample routine below simply clears the exception status bit and
// does an 'rte'.
//
_060_real_snan:
	fsave		-(sp)
	move.w		#0x6000,2(sp)
	frestore	(sp)+
	fmovem.l	#0,fpcr
	rte

// _060_real_operr():
//
// This is the exit point for the 060FPSP when an enabled operand error exception
// is present. The routine below should point to the operating system handler
// for enabled operand error exceptions. The exception stack frame is an operand error
// stack frame. The FP state frame holds the source operand of the faulting
// instruction.
//
// The sample routine below simply clears the exception status bit and
// does an 'rte'.
//
_060_real_operr:
	fsave		-(sp)
	move.w		#0x6000,2(sp)
	frestore	(sp)+
	fmovem.l	#0,fpcr
	rte

// _060_real_ovfl():
//
// This is the exit point for the 060FPSP when an enabled overflow exception
// is present. The routine below should point to the operating system handler
// for enabled overflow conditions. The exception stack frame is an overflow
// stack frame. The FP state frame holds the EXCEPTIONAL OPERAND.
//
// The sample routine below simply clears the exception status bit and
// does an 'rte'.
//
_060_real_ovfl:
	fsave		-(sp)
	move.w		#0x6000,2(sp)
	frestore	(sp)+
	fmovem.l	#0,fpcr
	rte

// _060_real_unfl():
//
// This is the exit point for the 060FPSP when an enabled underflow exception
// is present. The routine below should point to the operating system handler
// for enabled underflow conditions. The exception stack frame is an underflow
// stack frame. The FP state frame holds the EXCEPTIONAL OPERAND.
//
// The sample routine below simply clears the exception status bit and
// does an 'rte'.
//
_060_real_unfl:
	fsave		-(sp)
	move.w		#0x6000,2(sp)
	frestore	(sp)+
	fmovem.l	#0,fpcr
	rte

// _060_real_dz():
//
// This is the exit point for the 060FPSP when an enabled divide-by-zero exception
// is present. The routine below should point to the operating system handler
// for enabled divide-by-zero exceptions. The exception stack frame is a divide-by-zero
// stack frame. The FP state frame holds the source operand of the faulting
// instruction.
//
// The sample routine below simply clears the exception status bit and
// does an 'rte.'
//
_060_real_dz:
	fsave		-(sp)
	move.w		#0x6000,2(sp)
	frestore	(sp)+
	fmovem.l	#0,fpcr
	rte

// _060_real_inex():
//
// This is the exit point for the 060FPSP when an enabled inexact exception
// is present. The routine below should point to the operating system handler
// for enabled inexact exceptions. The exception stack frame is an inexact
// stack frame. The FP state frame holds the source operand of the faulting
// instruction.
//
// The sample routine below simply clears the exception status bit and
// does an 'rte'.
//
_060_real_inex:
	fsave		-(sp)
	move.w		#0x6000,2(sp)
	frestore	(sp)+
	fmovem.l	#0,fpcr
	rte

// _060_real_fpu_disabled():
//
// This is the exit point for the 060FPSP when an FPU disabled exception is
// encountered. Three different types of exceptions can enter the F-Line exception
// vector number 11: FP Unimplemented Instructions, FP implemented  instructions when
// the FPU is disabled, and F-Line Illegal instructions. The 060FPSP module
// _fpsp_fline() distinguishes between the three and acts appropriately. FPU disabled
// exceptions branch here.
//
// The sample code below enables the FPU, sets the PC field in the exception stack
// frame to the PC of the instruction causing the exception, and does an 'rte'.
// The execution of the instruction then proceeds with an enabled floating-point
// unit.
//
_060_real_fpu_disabled:
	move.l	d0,-(sp)
	dc.l	0x4e7a0808		// movec.l pcr, d0
	bclr	#1,d0
	dc.l	0x4e7b0808		// movec.l d0, pcr
	move.l	(sp)+,d0
	move.l	12(sp),2(sp)
	fmovem.l	#0,fpcr
	rte

_060_real_chk:
	tst.b	(sp)			// is tracing enabled?
	bpl.b	real_chk_end		// no
	move.b	#0x24,0x7(sp)		// set trace vecno
	bra.l	_060_real_trace
real_chk_end:
	rte

_060_real_divbyzero:
	tst.b	(sp)			// is tracing enabled?
	bpl.b	real_divbyzero_end	// no
	move.b	#0x24,0x7(sp)		// set trace vecno
	bra.l	_060_real_trace
real_divbyzero_end:
	rte

_060_real_lock_page:
_060_real_unlock_page:
	clr.l	d0
	rts

_060_isp_done:
	rte

// -----------------------------------------------------------------------------
// jump entry table into the ISP functions
// This area must be exactly 128 bytes!
// -----------------------------------------------------------------------------
_top_isp:
	dc.l	_060_real_chk         - _top_isp	// or 0xf4
	dc.l	_060_real_divbyzero   - _top_isp
	dc.l	_060_real_trace       - _top_isp
	dc.l	_060_real_access      - _top_isp
	dc.l	_060_isp_done         - _top_isp
	dc.l	_060_real_cas         - _top_isp
	dc.l	_060_real_cas2        - _top_isp
	dc.l	_060_real_lock_page   - _top_isp
	dc.l	_060_real_unlock_page - _top_isp
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	_060_imem_read        - _top_isp
	dc.l	_060_dmem_read        - _top_isp
	dc.l	_060_dmem_write       - _top_isp
	dc.l	_060_imem_read_word   - _top_isp
	dc.l	_060_imem_read_long   - _top_isp
	dc.l	_060_dmem_read_byte   - _top_isp
	dc.l	_060_dmem_read_word   - _top_isp
	dc.l	_060_dmem_read_long   - _top_isp
	dc.l	_060_dmem_write_byte  - _top_isp
	dc.l	_060_dmem_write_word  - _top_isp
	dc.l	_060_dmem_write_long  - _top_isp
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0

#include "isp.S"

// -----------------------------------------------------------------------------
// jump entry table into the FPSP functions
// This area must be exactly 128 bytes!
// -----------------------------------------------------------------------------
_top_fpsp:
	dc.l	_060_real_bsun         - _top_fpsp
	dc.l	_060_real_snan         - _top_fpsp
	dc.l	_060_real_operr        - _top_fpsp
	dc.l	_060_real_ovfl         - _top_fpsp
	dc.l	_060_real_unfl         - _top_fpsp
	dc.l	_060_real_dz           - _top_fpsp
	dc.l	_060_real_inex         - _top_fpsp
	dc.l	_060_real_fline        - _top_fpsp	// or 0x2C
	dc.l	_060_real_fpu_disabled - _top_fpsp
	dc.l	_060_real_trap         - _top_fpsp
	dc.l	_060_real_trace        - _top_fpsp
	dc.l	_060_real_access       - _top_fpsp
	dc.l	_060_fpsp_done         - _top_fpsp
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	_060_imem_read         - _top_fpsp
	dc.l	_060_dmem_read         - _top_fpsp
	dc.l	_060_dmem_write        - _top_fpsp
	dc.l	_060_imem_read_word    - _top_fpsp
	dc.l	_060_imem_read_long    - _top_fpsp
	dc.l	_060_dmem_read_byte    - _top_fpsp
	dc.l	_060_dmem_read_word    - _top_fpsp
	dc.l	_060_dmem_read_long    - _top_fpsp
	dc.l	_060_dmem_write_byte   - _top_fpsp
	dc.l	_060_dmem_write_word   - _top_fpsp
	dc.l	_060_dmem_write_long   - _top_fpsp
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0

#include "fpsp.S"
