
#include "mint/asmdefs.h"

.text
	.even
.globl SYM(_mint_strlen)

SYM(_mint_strlen):
	movel	sp@(4),a0
	movel	a0,d0
	notl	d0
	.even
loop:
	tstb	a0@+
	jne	loop
	addl	a0,d0
	rts
