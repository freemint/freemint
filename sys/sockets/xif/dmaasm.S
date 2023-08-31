#include "mint/asmdefs.h"

	.globl SYM(lance_probe_asm)
	.globl SYM(lance_probe_c_)

	.text

SYM(lance_probe_asm):
	moveml	d0-d7/a0-a6,sp@-
	movew	sr,sp@-
	movel	8,_oldberr
	lea	_berr,a0
	movel	a0,8
	movel	sp,_old_stack
	movel	SYM(lance_probe_c_),a0
	jsr	a0@
_berr:
	movel	_oldberr,8
	clrl	_oldberr
	movel	_old_stack,sp
	movew	sp@+,sr
	moveml	sp@+,d0-d7/a0-a6
	rts

	.data

_old_stack:
	.long	0
_oldberr:
	.long	0
