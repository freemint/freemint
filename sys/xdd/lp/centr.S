|
| centr.s: Centronics interrupt routine. It is raised every time
| the printer is ready to write one more character
|
| Also a few low-level functions

	.globl _new_centr_vector
	.globl _print_byte

_new_centr_vector:
#ifdef __mcoldfire__
	lea	sp@(-16), sp
	moveml	d0-d1/a0-a1, sp@
#else
	moveml	d0-d1/a0-a1, sp@-
#endif
	jsr	_print_head		| prints the pending byte(s)
	jsr	_wake_up		| has someone selected the printer?
#ifdef __mcoldfire__
	moveql	#0, d0
	bclr	d0, 0xFFFFFA11
	moveml	sp@, d0-d1/a0-a1
	lea	sp@(16), sp
#else
	moveml	sp@+, d0-d1/a0-a1
	bclr	#0, 0xFFFFFA11
#endif
	rte
	
_print_byte:
	movew	sp@(4), d0		| character to print
#ifdef __mcoldfire__
	movew	sr, d1
	movew	d1, sp@-
	oril	#0x700, d1		| disable all interrupts
	movew	d1, sr
#else
	movew	sr, sp@-
	oriw	#0x700, sr		| disable all interrupts
#endif
	movel	#0xFFFF8800, a0
	moveb	#15, a0@		| select port B
	moveb	d0, a0@(2)		| and write the character
	moveb	#14, a0@		| select port A
	moveb	a0@, d0			| read it
	bclr	#5, d0
	moveb	d0, a0@(2)		| strobe low
	nop
	nop
	bset	#5, d0
	moveb	d0, a0@(2)		| strobe high
#ifdef __mcoldfire__
	movew	sp@+, d1
	movew	d1, sr
#else
	movew	sp@+, sr
#endif
	rts
