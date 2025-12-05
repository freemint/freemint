|
| Low level and interrupt routines for the PLIP channel 0 driver
|
| 03/13/94, Kay Roemer.
|

#include "mint/asmdefs.h"
#include "mint/xbra.h"

yamaha	  = 0xFFFF8800
mfp_gpip  = 0xFFFFFA01
mfp_aer   = mfp_gpip +  2
mfp_iera  = mfp_gpip +  6
mfp_ierb  = mfp_gpip +  8
mfp_ipra  = mfp_gpip + 10
mfp_iprb  = mfp_gpip + 12
mfp_isra  = mfp_gpip + 14
mfp_isrb  = mfp_gpip + 16
mfp_imra  = mfp_gpip + 18
mfp_imrb  = mfp_gpip + 20

|
| 200 Hz ticks counter
|

hz200     = 0x000004ba

|
| PLIP special chars
|

ESC       = 219
END       = 192
ESC_ESC   = 221
ESC_END   = 220

|
| PSG register numbers
|

IOA       = 14
IOB       = 15
IOCTRL    = 7

|
| Exports
|
	.globl SYM(pl0_set_strobe)
	.globl SYM(pl0_set_direction)
	.globl SYM(pl0_got_ack)
	.globl SYM(pl0_send_ack)
	.globl SYM(pl0_send_pkt)
	.globl SYM(pl0_recv_pkt)
	.globl SYM(pl0_cli)
	.globl SYM(pl0_sti)
	.globl SYM(pl0_old_busy_int)
	.globl SYM(pl0_busy_int)
	.globl SYM(pl0_get_busy)

|
| Imports
|
	.extern SYM(plip_int)


	.text
|
| Get BUSY.
|

SYM(pl0_get_busy):
	btst	#0, mfp_gpip:w
	sne	d0
	extw	d0
	rts

|
| Set STROBE.
|

SYM(pl0_set_strobe):
	movew	sr, d0
	oriw	#0x700, sr
	lea	yamaha:w, a0
	moveb	#IOA, a0@
	moveb	a0@, d1
	tstw	sp@(4)
	beq	L_100
	bset	#5, d1		| Set STROBE.
	bra	L_101
L_100:	bclr	#5, d1		| Clear STROBE.
L_101:	moveb	d1, a0@(2)
	movew	d0, sr
	rts
	
|
| Set the direction of the IO port B. Set to INPUT if argument is zero.
|

SYM(pl0_set_direction):
	movew	sr, d1
	oriw	#0x700, sr
	lea	yamaha:w, a0
	moveb	#IOCTRL, a0@
	moveb	a0@, d0
	tstw	sp@(4)
	beq	L_110
	bset	#7, d0		| Set OUTPUT
	bra	L_111
L_110:	bclr	#7, d0		| Set INPUT
L_111:	moveb	d0, a0@(2)
	movew	d1, sr
	rts

|
| See if we got an acknoledge (BUSY dropped for some us)
|

SYM(pl0_got_ack):
	btst	#0, mfp_iprb:w
	sne	d0
	bne	L_140
	rts
L_140:	moveb	#0xfe, mfp_iprb:w
	rts	

|
| Send an acknowledge (drop STROBE for some us)
|

SYM(pl0_send_ack):
	movew	sr, d1
	oriw	#0x700, sr
	lea	yamaha:w, a0

	moveb	#IOA, a0@	| clear STROBE
	moveb	a0@, d0
	bclr	#5, d0
	moveb	d0, a0@(2)
	nop
	nop
	bset	#5, d0		| set STROBE
	moveb	d0, a0@(2)

	movew	d1, sr
	rts

|
| Disable BUSY interrupt.
|

SYM(pl0_cli):
	bclr	#0, mfp_imrb:w
	rts

|
| Enable BUSY interrupt.
|

SYM(pl0_sti):
	bset	#0, mfp_imrb:w
	rts

|
| BUSY interrupt routine.
|

	.long	XBRA_MAGIC
	.long	0x706c6970	| "plip"
SYM(pl0_old_busy_int):
	.long	0xDEADFACE

SYM(pl0_busy_int):
	moveml	d0-d1/a0-a1, sp@-
	movew	sr, d0
	oriw	#0x700, sr

	movew	sp@(16), d1	| sr before interrupt
	andw	#0x0700, d1	| keep IPL
	cmpiw	#0x0300, d1	| other interrupt in service ?
	bgt	L_120		| if so branch

	andw	#0xf8ff, d0	| clear IPL
	oriw	#0x0300, d0	| set IPL to 3
	movew	d0, sr		| allow other interrupts

	movew	#0, sp@-	| PLIP channel 0
	jsr	SYM(plip_int)	| handle interrupt
	addql	#2, sp

	oriw	#0x700, sr

L_120:	moveml	sp@+, d0-d1/a0-a1
	moveb	#0xfe, mfp_isrb:w
	rte

|
| wait for ack.
|
| in:	d0.l -- timeout
| out:	d0.l -- failure/success
| clob:	d0, d1, a0
|

_pl0_wait_ack:
	moveal	hz200:w, a0
L_702:
	btst	#0, mfp_iprb:w
	bne	L_700

	movel	hz200:w, d1
	subl	a0, d1
	cmpl	d0, d1
	ble	L_702

	clrw	d0
	rts

L_700:
	moveb	#0xfe, mfp_iprb:w
	movew	#-1, d0
	rts	

|
| long pl0_recv_pkt (short timeout, char *cp, long len)
|

SYM(pl0_recv_pkt):
	moveml	d2-d5/a2-a3, sp@-
	clrl	d2
	movew	sp@(7*4+0), d2		| timeout
	moveal	sp@(7*4+2), a2		| cp
	movel	sp@(7*4+6), d3		| len

L_600:
	movel	a2, a3
	movel	d3, d4
	subql	#1, d4

L_601:
	movel	d2, d0
	bsr	_pl0_recv_byte
	tstw	d0
	bpl	L_602			| timeout

	cmpb	#ESC, d0
	beq	L_603
	cmpb	#END, d0
	beq	L_604

	moveb	d0, a3@+
	dbra	d4, L_601
	bra	L_606			| buffer overflow

L_603:
	movel	d2, d0
	bsr	_pl0_recv_byte
	tstw	d0
	bpl	L_602			| timeout

	cmpb	#ESC_END, d0
	bne	L_605
	moveb	#END, a3@+
	dbra	d4, L_601
	bra	L_606			| buffer overflow

L_605:
	cmpb	#ESC_ESC, d0
	bne	L_606			| unknown escaped char
	moveb	#ESC, a3@+
	dbra	d4, L_601
	bra	L_606			| buffer overflow

L_602:
	movel	#-1, d0			| return timeout
	moveml	sp@+, d2-d5/a2-a3
	rts

L_606:
	movel	#-2, d0			| return other error
	moveml	sp@+, d2-d5/a2-a3
	rts

L_604:
	movel	d3, d5
	subl	d4, d5
	subql	#1, d5
	beq	L_600			| zero bytes received, retry

	movel	d5, d0			| return length of packet
	moveml	sp@+, d2-d5/a2-a3
	rts

|
| Read a byte from the parallel port. Return 0xff in the high byte
| if ack received.
|
| in:	d0.l -- timeout
| out:	d0.w -- success/failure
| clob:	d0, d1, a0
|

_pl0_recv_byte:
	moveal	hz200:w, a0
L_165:
	btst	#0, mfp_iprb:w		| data on port valid ?
	bne	L_160

	movel	hz200:w, d1		| check for timeout
	subl	a0, d1
	cmpl	d0, d1
	ble	L_165

	clrw	d0			| data is not valid
	rts

L_160:
	moveb	#0xfe, mfp_iprb:w	| clear pending BUSY ints

	movew	sr, sp@-
	oriw	#0x700, sr
	lea	yamaha:w, a0

	moveb	#IOB, a0@
	movew	#0xff00, d0		| data is valid
	moveb	a0@, d0			| get data

	moveb	#IOA, a0@		| send ack
	moveb	a0@, d1
	bclr	#5, d1
	moveb	d1, a0@(2)
	nop
	nop
	bset	#5, d1
	moveb	d1, a0@(2)

	movew	sp@+, sr
	rts

|
| short pl0_send_pkt (short timeout, char *cp, long len);
|

SYM(pl0_send_pkt):
	moveml	d2-d4/a2, sp@-
	clrl	d2
	movew	sp@(5*4+0), d2		| timeout
	moveal	sp@(5*4+2), a2		| cp
	movel	sp@(5*4+6), d3		| len

	subql	#1, d3			| for dbra
L_500:
	moveb	a2@+, d4
	cmpb	#ESC, d4
	beq	L_501
	cmpb	#END, d4
	beq	L_502
	
	moveb	d4, d0			| send unescaped
	movel	d2, d1
	bsr	_pl0_send_byte
	tstw	d0
	bpl	L_503			| timeout

	dbra	d3, L_500
	bra	L_504			| all sent

L_501:
	moveb	#ESC, d0		| send ESC
	movel	d2, d1
	bsr	_pl0_send_byte
	tstw	d0
	bpl	L_503			| timeout

	moveb	#ESC_ESC, d0		| send ESC_ESC
	movel	d2, d1
	bsr	_pl0_send_byte
	tstw	d0
	bpl	L_503			| timeout

	dbra	d3, L_500
	bra	L_504			| all sent

L_502:
	moveb	#ESC, d0		| send ESC
	movel	d2, d1
	bsr	_pl0_send_byte
	tstw	d0
	bpl	L_503			| timeout

	moveb	#ESC_END, d0		| send ESC_END
	movel	d2, d1
	bsr	_pl0_send_byte
	tstw	d0
	bpl	L_503			| timeout

	dbra	d3, L_500
	bra	L_504			| all sent

L_503:					| return error
	movel	#-1, d0
	moveml	sp@+, d2-d4/a2
	rts

L_504:
	moveb	#END, d0		| send END
	movel	d2, d1
	bsr	_pl0_send_byte
	tstw	d0
	bpl	L_503			| timeout

	movel	d2, d0			| wait for final ack
	bsr	_pl0_wait_ack
	tstw	d0
	bpl	L_503			| timeout

	clrl	d0			| return ok
	moveml	sp@+, d2-d4/a2
	rts

|
| Write a byte to the parallel port and send ack.
|
| in:	d0.b -- byte to send
|	d1.l --	timeout
| out:	d0.w -- success/failure
| clob:	d0, d1, a0, a1
|

_pl0_send_byte:
	moveal	hz200:w, a0
	moveal	d1, a1
L_201:
	btst	#0, mfp_iprb:w		| ack received ?
	bne	L_200

	movel	hz200:w, d1
	subl	a0, d1
	cmpl	a1, d1
	ble	L_201

	clrw	d0			| data is not valid
	rts

L_200:
	moveb	#0xfe, mfp_iprb:w	| clear pending BUSY ints
	movew	sr, d1
	oriw	#0x700, sr
	lea	yamaha:w, a0
	moveb	#IOB, a0@
	moveb	d0, a0@(2)

	moveb	#IOA, a0@		| clear STROBE
	moveb	a0@, d0
	bclr	#5, d0
	moveb	d0, a0@(2)
	nop
	nop
	bset	#5, d0			| set STROBE
	moveb	d0, a0@(2)

	movew	d1, sr
	movew	#0xff00, d0
	rts
