|
| genmagic startup module
|
| This file belongs to FreeMiNT,
| it is not a part of the original MiNT distribution
|

#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b
#define SYM(x) CONCAT1 (__USER_LABEL_PREFIX__, x)

	.globl	_init
	.globl	SYM(small_main)
	.globl	SYM(_base)

	.bss

SYM(_base):
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
	jmp	SYM(small_main)
