|
|
| Interrupt routines and low-level functions for the slip channel 0 driver
|
| 03/02/94, Kay Roemer.
|
| based on modm0asm.s by T. Bousch
|

yamaha	  = 0xFFFF8800
mfp_gpip  = 0xFFFFFA01
mfp_aer   = mfp_gpip +  2
mfp_isra  = mfp_gpip + 14
mfp_isrb  = mfp_gpip + 16
mfp_imra  = mfp_gpip + 18
mfp_imrb  = mfp_gpip + 20
mfp_rsr	  = mfp_gpip + 42
mfp_tsr	  = mfp_gpip + 44
mfp_udr	  = mfp_gpip + 46

|
| Exports
|
	.globl _sl0_set_dtr, _sl0_set_rts, _sl0_set_brk
	.globl _sl0_get_dcd, _sl0_get_cts

	.globl _sl0_send_byte, _sl0_recv_byte
	.globl _sl0_can_send, _sl0_can_recv

	.globl _sl0_txint, _sl0_rxint
	.globl _sl0_txerror, _sl0_rxerror
	.globl _sl0_dcdint, _sl0_ctsint

	.globl _sl0_old_txint, _sl0_old_rxint
	.globl _sl0_old_txerror, _sl0_old_rxerror
	.globl _sl0_old_dcdint, _sl0_old_ctsint

	.globl _sl0_clear_errors, _sl0_check_errors
	.globl _sl0_on_off

|
| Imports
|
	.globl _slip_txint, _slip_rxint, _slip_start, _slip_stop

rx_errs:	.byte	0
tx_errs:	.byte	0
_sl0_on_off:	.byte	0

	.align 4
	.text
|
| Set DTR state.
|

_sl0_set_dtr:
	movew	sr, d1
	oriw	#0x700, sr
	movel	#yamaha, a0	| soundchip register
	moveb	#14, a0@	| no 14: port A
	moveb	a0@, d0
	tstw	sp@(4)
	beq	L_100
	bclr	#4, d0
	bra	L_101
L_100:	bset	#4, d0
L_101:	moveb	d0, a0@(2)
	movew	d1, sr
	rts

|
| Set RTS state.
|

_sl0_set_rts:
	movew	sr, d1
	oriw	#0x700, sr
	movel	#yamaha, a0	| soundchip register
	moveb	#14, a0@	| no 14: port A
	moveb	a0@, d0
	tstw	sp@(4)
	beq	L_110
	bclr	#3, d0
	bra	L_111
L_110:	bset	#3, d0
L_111:	moveb	d0, a0@(2)
	movew	d1, sr
	rts

|
| Set/clear break condition.
|

_sl0_set_brk:
	tstw	sp@(4)
	beq	L_120
	bset	#3, mfp_tsr:w
	rts
L_120:	bclr	#3, mfp_tsr:w
	rts

|
| Get CTS state.
|

_sl0_get_cts:
	btst	#2, mfp_gpip:w
	seq	d0
	rts

|
| Get DCD state.
|

_sl0_get_dcd:
	btst	#1, mfp_gpip:w
	seq	d0
	rts

|
| Sends a byte to the MFP, interrupts disabled
|

_sl0_send_byte:
	movew	sp@(4), d1
	moveb	d1, mfp_udr:w
	rts

_sl0_can_send:
	btst	#7, mfp_tsr:w
	sne	d0
	rts

|
| Gets a byte from the MFP, interrupts disabled
|

_sl0_recv_byte:
	moveb	mfp_udr:w, d0
	rts

_sl0_can_recv:
	btst	#7, mfp_rsr:w
	sne	d0
	rts

|
| "Transmitter buffer empty" interrupt routine
|

	.long	XBRA_MAGIC
	.long	0x736c6970	| "slip"
_sl0_old_txint:
	.long	0xDEADFACE	| old vector

_sl0_txint:
	tstb	_sl0_on_off
	beq	L_130
	moveml	d0-d1/a0-a1, sp@-
	oriw	#0x700, sr

	movew	#0, sp@-	| slip channel 0
	jsr	_slip_txint
	addql	#2, sp

	moveml	sp@+, d0-d1/a0-a1
	bclr	#2, mfp_isra:w
	rte

L_130:	movel	_sl0_old_txint, sp@-
	rts
	
|
| Receiver interrupt routine
|

	.long	XBRA_MAGIC
	.long	0x736c6970	| "slip"
_sl0_old_rxint:
	.long	0xDEADFACE	| old vector

_sl0_rxint:
	tstb	_sl0_on_off
	beq	L_140
	moveml	d0-d1/a0-a1, sp@-
	oriw	#0x700, sr

	moveb	mfp_rsr, d0
	andb	#0x78, d0	| keep bits 3, 4, 5, and 6
	orb	d0, rx_errs

	movew	#0, sp@-	| slip channel 0
	jsr	_slip_rxint
	addql	#2, sp
	
	moveml	sp@+, d0-d1/a0-a1
	bclr	#4, mfp_isra:w
	rte

L_140:	movel	_sl0_old_rxint, sp@-
	rts

|
| Transmission error routine; the only interesting bit of the TSR is:
|
| bit 6: underrun error
|

	.long	XBRA_MAGIC
	.long	0x736c6970	| "slip"
_sl0_old_txerror:
	.long	0xDEADFACE	| old vector

_sl0_txerror:
	tstb	_sl0_on_off
	beq	L_150
	movel	d0, sp@-
	oriw	#0x700, sr

	moveb	mfp_tsr, d0
	andb	#0x40, d0	| keep bit 6
	orb	d0, tx_errs

	movel	sp@+, d0
	bclr	#1, mfp_isra:w
	rte

L_150:	movel	_sl0_old_txerror, sp@-
	rts

|
| Reception error routine; the interesting bits of the RSR are:
|
| bit 3: break detected
| bit 4: frame error, no rcv interrupt generated.
| bit 5: parity error
| bit 6: overrun error
|

	.long	XBRA_MAGIC
	.long	0x736c6970	| "slip"
_sl0_old_rxerror:
	.long	0xDEADFACE	| old vector

_sl0_rxerror:
	tstb	_sl0_on_off
	beq	L_160
	moveml	d0-d1/a0-a1, sp@-
	oriw	#0x700, sr

	moveb	mfp_rsr, d0
	andb	#0x78, d0	| keep bits 3, 4, 5, and 6
	orb	d0, rx_errs
#	bsr	_sl0_recv_byte	| read data (clears RSR)
	
	moveml	sp@+, d0-d1/a0-a1
	bclr	#3, mfp_isra:w
	rte

L_160:	movel	_sl0_old_rxerror, sp@-
	rts

|
| DCD interrupt routine
|
	.long	XBRA_MAGIC
	.long	0x736c6970	| "slip"
_sl0_old_dcdint:
	.long	0xDEADFACE	| old vector

_sl0_dcdint:
	moveml	d0-d1/a0-a1, sp@-
	oriw	#0x700, sr

	movew	#0, sp@-	| slip channel 0
	bchg	#1, mfp_aer:w	| switch active edge
	bne	L_170
	jsr	_slip_start	| DCD: low  -> high
	bra	L_171
L_170:	jsr	_slip_stop	| DCD: high -> low
L_171:	addql	#2, sp

	moveml	sp@+, d0-d1/a0-a1
	bclr	#1, mfp_isrb:w
	rte
	
|
| CTS interrupt routine
|
	.long	XBRA_MAGIC
	.long	0x736c6970	| "slip"
_sl0_old_ctsint:
	.long	0xDEADFACE	| old vector

_sl0_ctsint:
	moveml	d0-d1/a0-a1, sp@-
	oriw	#0x700, sr

	movew	#0, sp@-	| slip channel 0
	bchg	#2, mfp_aer:w	| switch active edge
	bne	L_180
	jsr	_slip_start	| CTS: low  -> high
	bra	L_181
L_180:	jsr	_slip_stop	| CTS: high -> low
L_181:	addql	#2, sp

	moveml	sp@+, d0-d1/a0-a1
	bclr	#2, mfp_isrb:w
	rte

|
| User routine, return bitmask of accumulated errors
|

UNDERRUN = 0x0001 
OVERRUN  = 0x0002
FRAME    = 0x0004
PARITY   = 0x0008
BREAK    = 0x0010

_sl0_check_errors:
	movel	d2, sp@-
	moveq	#0, d0
	moveb	tx_errs, d2	| any tx errors?
	btst	#6, d2
	beq	L_40
	orw	#UNDERRUN, d0
L_40:
	moveb	rx_errs, d2	| any rx errors?
	beq	L_44		| no, skip all that
	btst	#6, d2
	beq	L_41
	orw	#OVERRUN, d0
L_41:
	btst	#5, d2
	beq	L_42
	orw	#PARITY, d0
L_42:
	btst	#4, d2
	beq	L_43
	orw	#FRAME, d0
L_43:
	btst	#3, d2
	beq	L_44
	orw	#BREAK, d0
L_44:
	movel	sp@+, d2
	bsr	_sl0_clear_errors
	rts

_sl0_clear_errors:
	clrb	rx_errs
	clrb	tx_errs
	rts
