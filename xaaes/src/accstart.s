
	.globl	_accstart

	.text

| Accessory startup code poached from oAESis
_accstart:
	move.l	4(sp),a0		| pointer to basepage
	clr.l	36(a0)			| HR: clear pointer to parent basepage
	move.l	16(a0),a1		| data seg
	move.l	a1,8(a0)		| --> text seg (reverse xa_shell action)
	add.l	12(a0),a1		| text + textlen
	move.l	a1,16(a0)		| --> data seg
	move.l	8(a0),a1		| start of text segment
	jmp	(a1)			| jump to text
