| This file belongs to FreeMiNT,
| it is not a part of the original MiNT distribution.
| 
| Purpose: handle "unusual" memory structures
| 
| Author: Konrad Kokoszkiewicz <draco@atari.org>
| 
| 1999-02-11
| 
| FAT PEEK:
| a0 - buffer pointer
| d0 - buffer index (number of a 12 bit element to peek)
| 
| returns element value in d0

	.text

	.globl	_fat12_peek
	
_fat12_peek:
	movem.l	4(sp),d0/a0
	exg	d0,a0
	move.l	d0,d2		| save for later...
	lsr.l	#1,d0		| div 2 (to get a 24-bit element index)
	move.l	d0,d1		| mul 3 (to convert the index to an address)
	add.l	d1,d1		| gain two cycles
	add.l	d1,d0
#ifdef ONLY030
	move.l	(a0,d0.l),d1	| fetch the 24-bit element
	lsr.l	#0x8,d1
#else
	clr.l	d1
	lea	(a0,d0.l),a1	| unaligned read
	move.b	(a1)+,d1
	swap	d1
	move.b	(a1)+,d1
	lsl.w	#0x8,d1
	move.b	(a1)+,d1
#endif
	btst.l	#0,d2		| choose a single entry
	bne.s	exit
	lsr.l	#0x8,d1		| even element
	lsr.l	#0x4,d1
exit:	and.l	#0x00000fff,d1
	move.l	d1,d0
	rol.w	#0x8,d0
	rts
