
|-------------------------------------------------------------------------------------
| AES/VDI (Trap 2)  Handler
|-------------------------------------------------------------------------------------
| This mini handler just calls the main handler (written in C) or fields VDI
| commands out to the old vector (which was saved when we hooked trap 2 in the first
| place).
| Perhaps the whole trap handler should be in assembler, but really, it's fast enough
| for me just written in C with this small kludge routine to handle the actual
| exception itself. If anyone wants to recode it totally in assembler, the main Trap
| to pipe interface in in HANDLER.C - I'll happily put the mods into a new release.
| - Self modifying code removed [13/2/96] by Martin koeling.
| - Made XBRA compliant [13/2/96] by Martin Koeling.
| - AES trap code streamlined [980629] by Johan Klockars.
|-------------------------------------------------------------------------------------

	.globl	_XA_handler

	.globl	_old_trap2_vector
	.globl	_handler

	.text

| XBRA structure immediately before the new vector address:
	dc.l    0x58425241		| XBRA magic
	dc.l	0x58614145		| 'XaAE' XBRA id (just a proposal)
_old_trap2_vector:
	dc.l	0

| Exception vector goes to here...
_handler:
	cmp.w	#200,d0         	| Both 0xfffe (vq_gdos) and $73 (vdi calls) are less than 200($c8)
	blt.s	_not_aes
	bgt.s	new_test
__aes:
	movem.l	d0-d2/a0-a2,-(sp)	| for redirection by XaAES
					| Lattice C destroys d0-d1/a0-a1, but AES should keep a1
					| Pure C destroys d2 as well

|	move.l	d1,a0			| Place d1 arg in a0 as Lattice __regargs expects pointer to be
					| in a0 not d1 (same as Pure C)

|	for GNU-C push args to stack
	move.l	d1,-(sp)
	move.w	d0,-(sp)

	jsr 	_XA_handler		| Call the real handler written in C

	addq.w	#6,sp

	tst 	d0
	bge 	done			| XaAES returns -1 here if it is compiled without
					| fileselector and the fileselector is called.
					| This allows fileselectors to be started *Before* Mint & XaAES.
					| E.g. Boxkite
	movem.l	(sp)+,a0-a2/d0-d2	| ******
	bra 	_not_aes	

done:
	movem.l	(sp)+,a0-a2/d0-d2	| ^^^^^^
	clr.w	d0			| Ordinary GEM does this - so we do it as well...
_end_handler:
	rte

| New version of the call to the original VDI/AES vector
| [13/2/96] - Martin Koehling
| This is a jump to the old VDI/AES vector. No self-modifying code
| here - _old_trap_vector is data, not code... :-)
_not_aes:
	move.l	_old_trap2_vector,-(sp)
	rts

new_test:
	cmp.w   #201,d0         	| _appl_yield ?  (NB: sometimes used as vq_aes)
	bne.s   __aes           	| what old code does, but tested in new order
new_appl_yield:
|
|------- Insert code for future real _appl_yield here --------
|
	pea 	(a2)
	move.w	#0xff,-(sp)
	trap 	#1			| Call MiNT's Syield
	addq.w	#2,sp
	move.l	(sp)+,a2

	clr.w   d0			| tell caller AES did something (so it exists)
	rte				| then exit to caller
