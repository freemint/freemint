
.text
	.even
.globl __mint_strlen

__mint_strlen:
	movel	sp@(4),a0
	movel	a0,d0
	notl	d0
	.even
loop:
	tstb	a0@+
	jne	loop
	addl	a0,d0
	rts
