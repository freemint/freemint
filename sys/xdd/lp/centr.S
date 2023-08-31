|
| centr.s: Centronics interrupt routine. It is raised every time
| the printer is ready to write one more character
|
| Also a few low-level functions

#include "mint/asmdefs.h"

	.globl SYM(new_centr_vector)
	.globl SYM(print_byte)
	.extern SYM(print_head)
	.extern SYM(wake_up)

SYM(new_centr_vector):
#ifdef __mcoldfire__
	lea	sp@(-16), sp
	moveml	d0-d1/a0-a1, sp@
#else
	moveml	d0-d1/a0-a1, sp@-
#endif
	jsr	SYM(print_head)		| prints the pending byte(s)
	jsr	SYM(wake_up)		| has someone selected the printer?
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

#ifdef __mcoldfire__
|
| stupid delay for Coldfires to keep the strobe signal
| active long enough according to the standards
|
strobe_delay:
	move.l	(sp)+,d0		| 1
1:
	nop				| 6
	subq.l	#1,d0			| 1
	bne	1b			| 0 = 8 clocks at best
	rts
#endif

SYM(print_byte):
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

#ifdef __mcoldfire__
	move.l	d0,-(sp)
	move.l	#21,-(sp)		| 773 ns according to my calculations
	bsr	strobe_delay		| ieee 1284 requires at least 750 ns
	move.l	(sp)+,d0
#endif

	moveb	#14, a0@		| select port A
	moveb	a0@, d0			| read it
	bclr	#5, d0
	moveb	d0, a0@(2)		| strobe low
#ifdef __mcoldfire__
	move.l	d0,-(sp)
	move.l	#21,-(sp)		| see above
	bsr	strobe_delay
	move.l	(sp)+,d0
#else
	nop
	nop
#endif
	bset	#5, d0
	moveb	d0, a0@(2)		| strobe high
#ifdef __mcoldfire__
	movew	sp@+, d1
	movew	d1, sr
#else
	movew	sp@+, sr
#endif
	rts
