|
| genmagic startup module
|
| This file belongs to FreeMiNT,
| it is not a part of the original MiNT distribution
|

	.globl	_init
	.globl	_small_main
	.globl	__base

	.bss

__base:
	ds.l	1
__stack:
	ds.b	4094			| 4k is a minimum
__botst:
	ds.b	2

	.text
_init:
	move.l	0x04(sp),a0		| basepage address
	move.l	a0,__base

	lea	__botst,sp

	move.l	0x18(a0),d0		| base of the BSS segment
	add.l	0x1c(a0),d0		| size of the BSS segment
	sub.l	a0,d0
	
	move.l	d0,-(sp)		| new size
	move.l	a0,-(sp)		| start address
	pea	(0x004a0000).l		| Mshrink()
	trap	#1
	lea	0x0c(sp),sp
	jmp	_small_main
