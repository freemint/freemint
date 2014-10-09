|
| audioasm.s: DMA sound interrupt routine. It is raised at the end
| of each sample frame.
|
| Also a neat PSG sample player
|
| 11/03/95, Kay Roemer.
|
| 9705, added gpi7 routines: John Blakeley
|

mfp_gpip = 0xfffffa01
mfp_isra = mfp_gpip + 14
mfp_iera = mfp_gpip +  6
mfp_imra = mfp_gpip + 18
mfp_ipra = mfp_gpip + 10

	.globl _new_timera_vector
	.globl _new_gpi7_vector
	.globl _timer_func

_timer_func:
	.long 0

_new_gpi7_vector:
#ifdef __mcoldfire__
	lea	sp@(-16), sp
	moveml	d0-d1/a0-a1, sp@
#else
	moveml	d0-d1/a0-a1, sp@-
#endif
	moveal	_timer_func, a0
	jsr a0@			| do thangs...
#ifdef __mcoldfire__
	lea	mfp_isra:w, a0
	moveb	#0x7f, a0@
	moveml	sp@, d0-d1/a0-a1
	lea	sp@(16), sp
#else
	moveml	sp@+, d0-d1/a0-a1
	moveb	#0x7f, mfp_isra:w
#endif
	rte

_new_timera_vector:
#ifdef __mcoldfire__
	lea	sp@(-16), sp
	moveml	d0-d1/a0-a1, sp@
#else
	moveml	d0-d1/a0-a1, sp@-
#endif
	moveal	_timer_func, a0
	jsr	a0@			| do things...
#ifdef __mcoldfire__
	lea	mfp_isra:w, a0
	moveb	#0xdf, a0@
	moveml	sp@, d0-d1/a0-a1
	lea	sp@(16), sp
#else
	moveml	sp@+, d0-d1/a0-a1
	moveb	#0xdf, mfp_isra:w
#endif
	rte

	.globl _digitab, _playptr, _playlen, _psg_player

_psg_player:
#ifdef __mcoldfire__
	lea	sp@(-16), sp
	moveml	d0-d1/a0-a1, sp@
#else
	moveml	d0-d1/a0-a1, sp@-
#endif
	tstl	_playlen
	bgt	_psg_player_1
	moveal	_timer_func, a0
	jsr	a0@		| switch buffers ...
_psg_player_1:
	tstl	_playlen
	ble	_psg_player_2
	subql	#1, _playlen
	moveal	_playptr, a0
	moveq	#0, d0
	moveb	a0@+, d0
	movel	a0, _playptr
	lsll	#3, d0
	lea	_digitab, a0
#ifdef __mcoldfire__
	addl	d0, a0
	lea	0xffff8800:w, a1
	moveb	a0@+, a1@
	moveb	a0@+, a1@(2)
	moveb	a0@+, a1@
	moveb	a0@+, a1@(2)
	moveb	a0@+, a1@
	moveb	a0@, a1@(2)
#else
	movel	a0@(0,d0:w), d1
	movew	a0@(4,d0:w), d0
	lea	0xffff8800:w, a0
	movepl	d1, a0@(0)
	movepw	d0, a0@(0)
#endif
_psg_player_2:
#ifdef __mcoldfire__
	lea	mfp_isra:w, a0
	moveb	#0xdf, a0@
	moveml	sp@, d0-d1/a0-a1
	lea	sp@(16), sp
#else
	moveml	sp@+, d0-d1/a0-a1
	moveb	#0xdf, mfp_isra:w
#endif
	rte
