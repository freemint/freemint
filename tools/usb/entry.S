/* ACC startup */

    .globl	__base
	.globl	__start
	.globl	__crtinit

#define STACKSIZE	32768	/* in bytes */

	.bss
__base:
	ds.l	1
	ds.l	STACKSIZE/4
mystack:

	.text

__start:
	move.l	a0,__base
	lea     mystack, sp 
	jmp	_init		// in desk accessory
