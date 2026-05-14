| Fake an IKBD ACIA interrupt by vectoring through $118.
|
| Used by drivers that hold the CPU at IPL >= 6 long enough for the
| ACIA's UART to drop characters.  Polls the ACIA RDRF bit (handled
| by the caller via ikbd_int_pending()) and, when set, invokes the
| installed IKBD handler so the byte is read out before it is lost.
|
| Originally lifted from the Lightning VME/ST USB driver by
| Ingo Uhlemann / Christian Zietz; ported to the NetUSBee/EtherNAT
| USB drivers by Roger Burrows and to Svethlana by Joni Valtanen.
| The ColdFire variant comes from the USB mouse and OHCI drivers.
|
| Clobbers d0, d1 and a0 only.

#include "mint/asmdefs.h"

	.globl	SYM(fake_ikbd_int)

	.text

| Classic 680x0 frame (8 bytes):
|   [SP+0..1]  SR
|   [SP+2..5]  PC
|   [SP+6..7]  Format (bits 15-12) | Vector offset (bits 11-0)
| Format 0 = normal 4-word frame - always accepted by RTE.
| VBR = 0; the vector table lives at absolute low memory.
|
| ColdFire V4e frame (8 bytes):
|   [SP+0..3]  one longword packed as
|                bits 31-28  Format[3:0]  - bits [3:2] must be 01
|                                           (2-longword frame), otherwise
|                                           RTE raises Format Error; bits
|                                           [1:0] carry A7[1:0] at exception
|                                           time so RTE can restore the
|                                           caller's stack alignment
|                bits 27-26  FS[3:2]      - upper half of fault status
|                                           (0 when not simulating a fault)
|                bits 25-18  VEC[7:0]     - vector number (70 here, same as
|                                           classic ACIA MFP source 6); in
|                                           top-word bits this occupies
|                                           [9:2], so VEC=70 encodes as
|                                           (70 << 2) = 0x118 - the same
|                                           low 12 bits as the classic path
|                bits 17-16  FS[1:0]      - lower half of fault status; 0
|                bits 15-0   SR
|   [SP+4..7]  PC
|

SYM(fake_ikbd_int):
#ifdef __mcoldfire__
	| ColdFire RTE expects a 4-byte aligned frame of format/vector word,
	| SR and PC. A real exception aligns A7 down and keeps the dropped
	| low two bits in format[1:0], so RTE can undo the alignment; do
	| the same here.
	move.l	sp,d0
	and.l	#3,d0
	lsl.l	#8,d0			| V4e limits LSL.L #imm to 1..8,
	lsl.l	#4,d0			|   shift by 12 in two steps
	or.l	#0x4118,d0		| format 4, vector offset 0x118
	move.l	sp,d1
	andi.l	#0xfffffffc,d1		| align sp down to 4 bytes
	movea.l	d1,sp
	pea	2f(pc)			| return address
	move.w	sr,d1			| V4e cannot push SR directly
	move.w	d1,-(sp)		|   SR
	move.w	d0,-(sp)		|   format/vector word
#else
	tst.w	0x59e.w			| handle 68000/68030 stack-frame difference
	beq.b	1f
	move.w	#0x118,-(sp)		| push vector offset
1:
	pea	2f(pc)			| return address
	move	sr,-(sp)		|   and SR
#endif
	move.l	0x118,a0
	jmp	(a0)			| go to interrupt handler
2:
	rts
