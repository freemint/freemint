	.globl _lance_hbi_int, _lance_v5_int, _old_vbi, _lance_vbi_int
	.globl _lance_probe_asm, _lance_probe_c_
	.globl _ihandler

_lance_hbi_int:
	andiw	#0xfeff, sp@
	oriw	#0x0200, sp@
	rte

	.long	0x58425241	| "XBRA"	
	.long	0x6c616e63	| "lanc"
_old_vbi:
	.long	0
_lance_vbi_int:
	moveml	d0-a6, sp@-
	movel	_ihandler, d0
	movel	d0, a0
	beq	old_vbi
	jsr	a0@
old_vbi:
	movel	_old_vbi, d0
	movel	d0, a0
	beq	not_inst
	moveml	sp@+,d0-a6
	movel	_old_vbi,sp@-
	rts
not_inst:
	moveml	sp@+,d0-a6
	rte

_lance_v5_int:
	moveml	d0-a6, sp@-
	movel	_ihandler, d0
	movel	d0, a0
	beq	no_handler
	jsr	a0@
no_handler:
	moveml	sp@+, d0-a6
	rte

_lance_probe_asm:
	moveml	d0-d7/a0-a6,sp@-
	movew	sr,sp@-
	movel	8,_ihandler
	lea	berr,a0
	movel	a0,8
	movel	sp,old_stack
	movel	_lance_probe_c_,a0
	jsr	a0@
berr:
	movel	_ihandler,8
	clrl	_ihandler
	movel	old_stack,sp
	movew	sp@+,sr
	moveml	sp@+,d0-d7/a0-a6
	rts
old_stack:
	.long	0
