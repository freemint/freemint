|
| Low level and interrupt routines for the DE600 driver
|
| 01/15/95, Kay Roemer.
|

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
| Exports
|
	.globl _de600_cli, _de600_sti
	.globl _de600_old_busy_int, _de600_busy_int
	.globl _de600_dints

|
| Imports
|
	.globl _de600_int


	.text
|
|
|

_de600_dints:
	moveb	#0xfe, mfp_iprb:w
	rts

|
| Disable BUSY interrupt.
|

_de600_cli:
	bclr	#0, mfp_imrb:w
	rts

|
| Enable BUSY interrupt.
|

_de600_sti:
	bset	#0, mfp_imrb:w
	rts

|
| BUSY interrupt routine.
|

	.long	0x58425241	| "XBRA"
	.long	0x64653030	| "de600"
_de600_old_busy_int:
	.long	0xDEADFACE

_de600_busy_int:
	moveml	d0-d1/a0-a1, sp@-
	movew	sr, d0
	oriw	#0x700, sr

	movew	sp@(16), d1	| sr before interrupt
	andw	#0x0700, d1	| keep IPL

	andw	#0xf8ff, d0	| clear IPL
	orw	d1, d0		| set IPL to value before int
	movew	d0, sr		| allow other interrupts

	jsr	_de600_int	| handle interrupt

	oriw	#0x700, sr

L_120:	moveml	sp@+, d0-d1/a0-a1
	moveb	#0xfe, mfp_isrb:w
	rte
