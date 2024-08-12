/*
 *  freal.s:
 * 	This file is appended to the top of the 060FPSP package
 *  and contains the entry points into the package. The user, in
 *  effect, branches to one of the branch table entries located
 *  after _060FPSP_TABLE.
 * 	Also, subroutine stubs exist in this file (_fpsp_done for
 *  example) that are referenced by the FPSP package itself in order
 *  to call a given routine. The stub routine actually performs the
 *  callout. The FPSP code does a "bsr" to the stub routine. This
 *  extra layer of hierarchy adds a slight performance penalty but
 *  it makes the FPSP code easier to read and more mainatinable.
 */

#include "mint/asmdefs.h"
#define usp REG(usp)

	_off_bsun = 0x00
	_off_snan = 0x04
	_off_operr = 0x08
	_off_ovfl = 0x0c
	_off_unfl = 0x10
	_off_dz = 0x14
	_off_inex = 0x18
	_off_fline = 0x1c
	_off_fpu_dis = 0x20
	_off_trap = 0x24
	_off_trace = 0x28
	_off_access = 0x2c
	_off_done = 0x30

	_off_imr = 0x40
	_off_dmr = 0x44
	_off_dmw = 0x48
	_off_irw = 0x4c
	_off_irl = 0x50
	_off_drb = 0x54
	_off_drw = 0x58
	_off_drl = 0x5c
	_off_dwb = 0x60
	_off_dww = 0x64
	_off_dwl = 0x68

	.xref _fpsp_snan
	.xref _fpsp_operr
	.xref _fpsp_ovfl
	.xref _fpsp_unfl
	.xref _fpsp_dz
	.xref _fpsp_inex
	.xref _fpsp_fline
	.xref _fpsp_unsupp
	.xref _fpsp_effadd

    .text

	.globl _start
_start:

_060FPSP_TABLE:

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*  Here's the table of ENTRY POINTS for those linking the package. */
	bra.l		_fpsp_snan
	.dc.w		0x0000
	bra.l		_fpsp_operr
	.dc.w		0x0000
	bra.l		_fpsp_ovfl
	.dc.w		0x0000
	bra.l		_fpsp_unfl
	.dc.w		0x0000
	bra.l		_fpsp_dz
	.dc.w		0x0000
	bra.l		_fpsp_inex
	.dc.w		0x0000
	bra.l		_fpsp_fline
	.dc.w		0x0000
	bra.l		_fpsp_unsupp
	.dc.w		0x0000
	bra.l		_fpsp_effadd
	.dc.w		0x0000

	/*  28x trapf, filling to 128 bytes */
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc
	.dc.w 0x51fc

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		_fpsp_done
_fpsp_done:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_done,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_real_ovfl
_real_ovfl:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_ovfl,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_real_unfl
_real_unfl:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_unfl,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_real_inex
_real_inex:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_inex,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_real_bsun
_real_bsun:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_bsun,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_real_operr
_real_operr:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_operr,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_real_snan
_real_snan:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_snan,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_real_dz
_real_dz:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_dz,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_real_fline
_real_fline:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_fline,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_real_fpu_disabled
_real_fpu_disabled:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_fpu_dis,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_real_trap
_real_trap:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_trap,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_real_trace
_real_trace:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_trace,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_real_access
_real_access:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_access,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		_imem_read
_imem_read:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_imr,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_dmem_read
_dmem_read:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_dmr,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_dmem_write
_dmem_write:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_dmw,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_imem_read_word
_imem_read_word:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_irw,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_imem_read_long
_imem_read_long:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_irl,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_dmem_read_byte
_dmem_read_byte:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_drb,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_dmem_read_word
_dmem_read_word:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_drw,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_dmem_read_long
_dmem_read_long:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_drl,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_dmem_write_byte
_dmem_write_byte:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_dwb,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_dmem_write_word
_dmem_write_word:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_dww,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4

	.globl		_dmem_write_long
_dmem_write_long:
	move.l		d0,-(sp)
	move.l		(_060FPSP_TABLE-0x80+_off_dwl,pc),d0
	pea.l		((_060FPSP_TABLE-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd		#0x4


/* ;;;;;;;;;;;; */
/*  CONSTANTS ; */
/* ;;;;;;;;;;;; */
	.globl T1
	.globl T2
T1:	.dc.l		0x40C62D38,0xD3D64634	/*  16381 LOG2 LEAD */
T2:	.dc.l		0x3D6F90AE,0xB1E75CC7	/*  16381 LOG2 TRAIL */

	.globl PI
	.globl PIBY2
PI:	.dc.l		0x40000000,0xC90FDAA2,0x2168C235,0x00000000
PIBY2:	.dc.l		0x3FFF0000,0xC90FDAA2,0x2168C235,0x00000000

	.globl TWOBYPI
TWOBYPI:
	.dc.l		0x3FE45F30,0x6DC9C883

