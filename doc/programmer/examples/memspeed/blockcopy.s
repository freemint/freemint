| assembler routines for various methods of doing move block
| in memory

	XDEF	_copy256l
	XDEF	_copy256m
	XDEF	_copy512m
	XDEF	_longcopy
	XDEF	_wordcopy
	XDEF	_bytecopy

| this one is just for fun

_bytecopy:
	movem.l	4(sp),a0/a1
	move.l	12(sp),d0
bt:	move.b	(a0)+,(a1)+
	subq.l	#1,d0
	bne.s	bt
	rts

_wordcopy:
	movem.l	4(sp),a0/a1
	move.l	12(sp),d0
wc:	move.w	(a0)+,(a1)+
	subq.l	#1,d0
	bne.s	wc
	rts

_longcopy:
	movem.l	4(sp),a0/a1
	move.l	12(sp),d0
lc:	move.l	(a0)+,(a1)+
	subq.l	#1,d0
	bne.s	lc
	rts

| this one seems to be the best one on falcon

_copy256l:
	movem.l	4(sp),a0/a1
	move.l	12(sp),d0
	bra	scini
sc:	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
scini:	dbra.w	d0,sc
	rts

_copy256m:
	movem.l	4(sp),a0/a1
	move.l	12(sp),d0
	movem.l	d1-d7/a2-a6,-(sp)
	bra.s	lpini
lp:	movem.l	(a0)+,d1-d7/a2-a6
	movem.l	d1-d7/a2-a6,(a1)
	movem.l	(a0)+,d1-d7/a2-a6
	movem.l	d1-d7/a2-a6,48(a1)
	movem.l	(a0)+,d1-d7/a2-a6
	movem.l	d1-d7/a2-a6,96(a1)
	movem.l	(a0)+,d1-d7/a2-a6
	movem.l	d1-d7/a2-a6,144(a1)
	movem.l	(a0)+,d1-d7/a2-a6
	movem.l	d1-d7/a2-a6,192(a1)
	movem.l	(a0)+,d1-d4
	movem.l	d1-d4,240(a1)
	lea	256(a1),a1
lpini:	dbra.w	d0,lp
	movem.l	(sp)+,d1-d7/a2-a6
	rts

| this method is used internally by MiNT

_copy512m:
	movem.l	4(sp),a0/a1
	move.l	12(sp),d0
	movem.l	d1-d7/a2-a6,-(sp)
	bra.s	bleini
ble:	movem.l	(a0)+,d1-d7/a2-a6
	movem.l	d1-d7/a2-a6,(a1)
	movem.l	(a0)+,d1-d7/a2-a6
	movem.l	d1-d7/a2-a6,48(a1)
	movem.l	(a0)+,d1-d7/a2-a6
	movem.l	d1-d7/a2-a6,96(a1)
	movem.l	(a0)+,d1-d7/a2-a6
	movem.l	d1-d7/a2-a6,144(a1)
	movem.l	(a0)+,d1-d7/a2-a6
	movem.l	d1-d7/a2-a6,192(a1)
	movem.l	(a0)+,d1-d7/a2-a6
	movem.l	d1-d7/a2-a6,240(a1)
	movem.l	(a0)+,d1-d7/a2-a6
	movem.l	d1-d7/a2-a6,288(a1)
	movem.l	(a0)+,d1-d7/a2-a6
	movem.l	d1-d7/a2-a6,336(a1)
	movem.l	(a0)+,d1-d7/a2-a6
	movem.l	d1-d7/a2-a6,384(a1)
	movem.l	(a0)+,d1-d7/a2-a6
	movem.l	d1-d7/a2-a6,432(a1)
	movem.l	(a0)+,d1-d7/a2
	movem.l	d1-d7/a2,480(a1)
	lea	512(a1),a1
bleini:	dbra.w	d0,ble
	movem.l	(sp)+,d1-d7/a2-a6
	rts
