	.text
//
// Loop calibration
//
	.globl	_calibrate

_calibrate:
	move.l	4(sp),d0
	lea	0x4ba.w,a0
	move.l	(a0),d1
sync:	cmp.l	(a0),d1
	beq.s	sync
	addq.l	#0x01,d1
count:	subq.l	#0x01,d0
	bne.s	count
	move.l	(a0),d2
	sub.l	d1,d2
	move.l	d2,d0
	rts

//
// void delay5(ulong time);
//
// The function gives delays measured in 5 microsecond intervals.
// The faster is the machine, the timings are more accurate.
//
	.globl	_delay5
	.globl	_delayfactor

_delay5:
	move.l	4(sp),d1
	move.l	_delayfactor,d0
mcount:	move.l	d1,d2
dcount:	subq.l	#0x01,d2
	bne.s	dcount
	subq.l	#0x01,d0
	bne.s	mcount
	rts
