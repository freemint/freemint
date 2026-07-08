| 
| Low level and interrupt routines for the USB mouse driver
|

#include "mint/asmdefs.h"
#include "mint/xbra.h"

#ifdef TOSONLY
	.globl SYM(old_ikbd_int)
	.globl SYM(interrupt_ikbd)
	.extern SYM(mouse_int)

	.text

	dc.l	XBRA_MAGIC
	dc.l	0x554d4f55		| UMOU
SYM(old_ikbd_int):
	ds.l	1
SYM(interrupt_ikbd):
	tas	lock
	bne	skip
#ifdef __mcoldfire__
	lea	-64(sp),sp
	movem.l	a0-a7/d0-d7,(sp)
	move.l	a7,oldSP
	lea	ourSP,a7
	bsr	SYM(mouse_int)
	move.l	oldSP,a7
	movem.l	(sp),a0-a7/d0-d7
	lea	64(sp),sp
#else
	movem.l	a0-a7/d0-d7,registersave
	lea	ourSP,a7
	bsr	SYM(mouse_int)
	movem.l	registersave,a0-a7/d0-d7
#endif
	clr.w	lock
#ifdef __mcoldfire__
	| clr.w is a cached write; ColdFire tas is non-cacheable and reads
	| directly from RAM. Push the cache line containing `lock` so
	| it's visible to the next tas.
	move.l	d0,-(sp)
	move.l	a0,-(sp)
	lea	lock,a0
	move.l	a0,d0
	andi.l	#-16,d0
	move.l	d0,a0
	nop
	cpushl	%dc,(a0)		| way 0
	addq.l	#1,a0
	cpushl	%dc,(a0)		| way 1
	addq.l	#1,a0
	cpushl	%dc,(a0)		| way 2
	addq.l	#1,a0
	cpushl	%dc,(a0)		| way 3
	move.l	(sp)+,a0
	move.l	(sp)+,d0
#endif
skip:	move.l	SYM(old_ikbd_int)(pc),-(sp)
	rts
#endif

	.globl SYM(send_packet)
	.globl SYM(send_data)
	.globl SYM(fake_hwint)

SYM(send_packet):
	lea	-60(sp),sp
	movem.l	d0-a6,(sp)
	move.l	64(sp),a2
	move.l	68(sp),a0
	move.l	72(sp),a1
	jsr	(a2)
	movem.l	(sp),d0-a6
	lea	60(sp),sp
	rts

SYM(send_data):
	lea	-60(sp),sp
	movem.l	d0-a6,(sp)
	move.l	64(sp),a2
	move.l	68(sp),a0
	move.l	72(sp),d0
	jsr	(a2)
	movem.l	(sp),d0-a6
	lea	60(sp),sp
rts

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

SYM(fake_hwint):
#ifdef __mcoldfire__
	move.l	sp,d0
	and.l	#3,d0
	lsl.l	#8,d0			| ColdFire V4e limits LSL.L #imm to 1..8,
	lsl.l	#4,d0			| so shift by 12 in two steps: A7[1:0] -> [13:12]
	or.l	#0x4118,d0		| format[3:2]=01 | VEC=70
					| Build the exception frame.

	| ColdFire V4e RTE requires the exception frame to be at a 4-byte
	| aligned address. On a real exception the CPU takes care of this:
	| it aligns A7 down to a 4-byte boundary before pushing the frame,
	| and stores the discarded low 2 bits of A7 in Format[1:0] so RTE
	| can restore the caller's original alignment on the way out. We
	| have to do the same alignment step here manually.
	move.l	sp,d2
	andi.l	#0xfffffffc,d2		| clear low 2 bits: sp aligned down
	movea.l	d2,sp

	pea	retaddr(pc)		| PC -> [SP+4..7]
	move.w	sr,d1			| SR -> Dn (V4e restriction)
	move.w	d1,-(sp)		| SR -> [SP+2..3]
	move.w	d0,-(sp)		| format word -> [SP+0..1]
#else
	tst.w	0x59e.w
	beq	shortframe
	move.w	#0x118,-(sp)
shortframe:
	pea	retaddr(pc)
	move	sr,d0
	move.w	d0,-(sp)
#endif
	move.l  0x118,a0
	jmp	(a0)
retaddr:
	rts

	.bss
lock:	ds.w 1
oldSP:	ds.l 1
	.balign 4			| ensure ourSP is 4-byte aligned
	ds.l 1024
ourSP:
registersave:
	ds.l 8+8
	.end
