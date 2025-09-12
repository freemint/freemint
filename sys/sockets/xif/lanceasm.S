#include "mint/asmdefs.h"
#include "mint/xbra.h"

	.globl SYM(lance_hbi_int)
	.globl SYM(lance_v5_int)
	.globl SYM(old_vbi)
	.globl SYM(lance_vbi_int)
	.globl SYM(lance_probe_asm)
	.globl SYM(lance_probe_c_)
	.extern SYM(ihandler)


SYM(lance_hbi_int):
	andiw	#0xfeff, sp@
	oriw	#0x0200, sp@
	rte

	.long	XBRA_MAGIC	
	.long	0x6c616e63	| "lanc"
SYM(old_vbi):
	.long	0
SYM(lance_vbi_int):
	moveml	d0-a6, sp@-
	movel	SYM(ihandler), d0
	movel	d0, a0
	beq	go_old_vbi
	jsr	a0@
go_old_vbi:
	movel	SYM(old_vbi), d0
	movel	d0, a0
	beq	not_inst
	moveml	sp@+,d0-a6
	movel	SYM(old_vbi),sp@-
	rts
not_inst:
	moveml	sp@+,d0-a6
	rte

SYM(lance_v5_int):
	moveml	d0-a6, sp@-
	movel	SYM(ihandler), d0
	movel	d0, a0
	beq	no_handler
	jsr	a0@
no_handler:
	moveml	sp@+, d0-a6
	rte

SYM(lance_probe_asm):
	moveml	d0-d7/a0-a6,sp@-
	movew	sr,sp@-
	movel	8,SYM(ihandler)
	lea	berr,a0
	movel	a0,8
	movel	sp,old_stack
	movel	SYM(lance_probe_c_),a0
	jsr	a0@
berr:
	movel	SYM(ihandler),8
	clrl	SYM(ihandler)
	movel	old_stack,sp
	movew	sp@+,sr
	moveml	sp@+,d0-d7/a0-a6
	rts
old_stack:
	.long	0
