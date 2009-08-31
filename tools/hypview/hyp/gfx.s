EXPORT reduce
EXPORT mono_bitmap

; void reduce(void *addr,long planesize,int dither);
; A0 - A3 Pointer auf 1. bis 4. Plane
; A4 Ende der ersten Plane
; D0 - D3 entsprechendes Wort aus entsprechender Plane
; D4 Dithermuster
; D5 temp. Dithermuster
; D6 Pixelcounter
; D7 Pixelwert
MODULE reduce
	movem.l	A2-A4/D3-D7,-(SP)
	movea.l	A0,A1
	adda.l	D0,A1
	movea.l	A1,A4
	movea.l	A1,A2
	adda.l	D0,A2
	movea.l	A2,A3
	adda.l	D0,A3
	
	move.w	D1,D4
rt4loop:
	move.w	(A0),D0
	move.w	(A1)+,D1
	move.w	(A2)+,D2
	move.w	(A3)+,D3
	
	moveq.l	#15,D6
rt4pixloop:
	moveq.l	#0,D7
	add.w		D3,D3
	addx.w	D7,D7
	add.w		D2,D2
	addx.w	D7,D7
	add.w		D1,D1
	addx.w	D7,D7
	add.w		D0,D0
	addx.w	D7,D7

	move.w	D4,D5
	ror.w		D7,D5
	and.w		#1,D5
	add.w		D5,D0
	
	dbra		D6,rt4pixloop

	move.w	D0,(A0)+

	cmpa.l	A0,A4
	bne		rt4loop
	
	movem.l	(SP)+,A2-A4/D3-D7
	rts
ENDMOD

; void mono_bitmap(void *src,void *dst,long planesize,int color);
; A0 - A3 Pointer auf 1. bis 4. Plane
; A4 - Pointer auf Zielbereich
; A5 - Endadresse fr Plane 1
; D0 - D3 entsprechendes Wort aus entsprechender Plane
; D4 Pixelcounter
; D5 Pixelwert
; D6 aktuelle mono bitmap (16 Pixel)
; D7 gesuchte Farbe (=Pixelwert)
MODULE mono_bitmap
	movem.l	A2-A5/D3-D7,-(SP)
	movea.l	A1,A4			*	A4 = Zieladresse
	movea.l	A0,A1
	adda.l	D0,A1			*	A1 = Zeiger auf 2te Plane
	movea.l	A1,A5			*	A5 = Ende der 1ten Plane
	movea.l	A1,A2	
	adda.l	D0,A2			*	A2 = Zeiger auf 3te Plane
	movea.l	A2,A3
	adda.l	D0,A3			*	A3 = Zeiger auf 4te Plane

	move.w	D1,D7			*	D7 = gesuchter Pixelwert
.loop:
	move.w	(A0)+,D0		*	Planes auslesen
	move.w	(A1)+,D1
	move.w	(A2)+,D2
	move.w	(A3)+,D3
	
	moveq.l	#0,D6			*	D6 = Mono Bitmap initialisieren
	moveq.l	#15,D4		*	D4 = 15 = Pixelcounter
.pixloop:
	moveq.l	#0,D5			*	D5 = Pixelwert initialisieren

	add.w	D6,D6			*	mono bitmap shiften

	add.w	D3,D3			*	Pixelwert extrahieren
	addx.w	D5,D5
	add.w	D2,D2
	addx.w	D5,D5
	add.w	D1,D1
	addx.w	D5,D5
	add.w	D0,D0
	addx.w	D5,D5

	cmp.w	D5,D7			*	gesuchter Pixelwert?
	bne	.cont

	addq.w	#1,D6			*	Bit im Monochromen Bitmap setzen

.cont:
	dbra		D4,.pixloop	*	naechstes Pixel

	move.w	D6,(A4)+		*	Monochrom Bitmap speichern

	cmpa.l	A5,A0
	blt	.loop			*	Loop solange Ende nicht erreicht
	
	movem.l	(SP)+,A2-A5/D3-D7
	rts
ENDMOD


