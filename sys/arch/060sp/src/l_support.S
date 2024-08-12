;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_fsins_
_fsins_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l	fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x	fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L0_2s
	bsr.l		ssin			; operand is a NORM
	bra.b		_L0_6s
_L0_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L0_3s			; no
	bsr.l		src_zero			; yes
	bra.b		_L0_6s
_L0_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L0_4s			; no
	bsr.l		t_operr			; yes
	bra.b		_L0_6s
_L0_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L0_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L0_6s
_L0_5s:
	bsr.l		ssind			; operand is a DENORM
_L0_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fsind_
_fsind_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L0_2d
	bsr.l		ssin			; operand is a NORM
	bra.b		_L0_6d
_L0_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L0_3d			; no
	bsr.l		src_zero			; yes
	bra.b		_L0_6d
_L0_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L0_4d			; no
	bsr.l		t_operr			; yes
	bra.b		_L0_6d
_L0_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L0_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L0_6d
_L0_5d:
	bsr.l		ssind			; operand is a DENORM
_L0_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fsinx_
_fsinx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L0_2x
	bsr.l		ssin			; operand is a NORM
	bra.b		_L0_6x
_L0_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L0_3x			; no
	bsr.l		src_zero			; yes
	bra.b		_L0_6x
_L0_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L0_4x			; no
	bsr.l		t_operr			; yes
	bra.b		_L0_6x
_L0_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L0_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L0_6x
_L0_5x:
	bsr.l		ssind			; operand is a DENORM
_L0_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_fcoss_
_fcoss_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L1_2s
	bsr.l		scos			; operand is a NORM
	bra.b		_L1_6s
_L1_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L1_3s			; no
	bsr.l		ld_pone			; yes
	bra.b		_L1_6s
_L1_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L1_4s			; no
	bsr.l		t_operr			; yes
	bra.b		_L1_6s
_L1_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L1_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L1_6s
_L1_5s:
	bsr.l		scosd			; operand is a DENORM
_L1_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fcosd_
_fcosd_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L1_2d
	bsr.l		scos			; operand is a NORM
	bra.b		_L1_6d
_L1_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L1_3d			; no
	bsr.l		ld_pone			; yes
	bra.b		_L1_6d
_L1_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L1_4d			; no
	bsr.l		t_operr			; yes
	bra.b		_L1_6d
_L1_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L1_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L1_6d
_L1_5d:
	bsr.l		scosd			; operand is a DENORM
_L1_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fcosx_
_fcosx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L1_2x
	bsr.l		scos			; operand is a NORM
	bra.b		_L1_6x
_L1_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L1_3x			; no
	bsr.l		ld_pone			; yes
	bra.b		_L1_6x
_L1_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L1_4x			; no
	bsr.l		t_operr			; yes
	bra.b		_L1_6x
_L1_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L1_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L1_6x
_L1_5x:
	bsr.l		scosd			; operand is a DENORM
_L1_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_fsinhs_
_fsinhs_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L2_2s
	bsr.l		ssinh			; operand is a NORM
	bra.b		_L2_6s
_L2_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L2_3s			; no
	bsr.l		src_zero			; yes
	bra.b		_L2_6s
_L2_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L2_4s			; no
	bsr.l		src_inf			; yes
	bra.b		_L2_6s
_L2_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L2_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L2_6s
_L2_5s:
	bsr.l		ssinhd			; operand is a DENORM
_L2_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fsinhd_
_fsinhd_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L2_2d
	bsr.l		ssinh			; operand is a NORM
	bra.b		_L2_6d
_L2_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L2_3d			; no
	bsr.l		src_zero			; yes
	bra.b		_L2_6d
_L2_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L2_4d			; no
	bsr.l		src_inf			; yes
	bra.b		_L2_6d
_L2_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L2_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L2_6d
_L2_5d:
	bsr.l		ssinhd			; operand is a DENORM
_L2_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fsinhx_
_fsinhx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L2_2x
	bsr.l		ssinh			; operand is a NORM
	bra.b		_L2_6x
_L2_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L2_3x			; no
	bsr.l		src_zero			; yes
	bra.b		_L2_6x
_L2_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L2_4x			; no
	bsr.l		src_inf			; yes
	bra.b		_L2_6x
_L2_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L2_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L2_6x
_L2_5x:
	bsr.l		ssinhd			; operand is a DENORM
_L2_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_flognp1s_
_flognp1s_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L3_2s
	bsr.l		slognp1			; operand is a NORM
	bra.b		_L3_6s
_L3_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L3_3s			; no
	bsr.l		src_zero			; yes
	bra.b		_L3_6s
_L3_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L3_4s			; no
	bsr.l		sopr_inf			; yes
	bra.b		_L3_6s
_L3_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L3_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L3_6s
_L3_5s:
	bsr.l		slognp1d			; operand is a DENORM
_L3_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_flognp1d_
_flognp1d_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L3_2d
	bsr.l		slognp1			; operand is a NORM
	bra.b		_L3_6d
_L3_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L3_3d			; no
	bsr.l		src_zero			; yes
	bra.b		_L3_6d
_L3_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L3_4d			; no
	bsr.l		sopr_inf			; yes
	bra.b		_L3_6d
_L3_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L3_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L3_6d
_L3_5d:
	bsr.l		slognp1d			; operand is a DENORM
_L3_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_flognp1x_
_flognp1x_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L3_2x
	bsr.l		slognp1			; operand is a NORM
	bra.b		_L3_6x
_L3_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L3_3x			; no
	bsr.l		src_zero			; yes
	bra.b		_L3_6x
_L3_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L3_4x			; no
	bsr.l		sopr_inf			; yes
	bra.b		_L3_6x
_L3_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L3_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L3_6x
_L3_5x:
	bsr.l		slognp1d			; operand is a DENORM
_L3_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_fetoxm1s_
_fetoxm1s_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L4_2s
	bsr.l		setoxm1			; operand is a NORM
	bra.b		_L4_6s
_L4_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L4_3s			; no
	bsr.l		src_zero			; yes
	bra.b		_L4_6s
_L4_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L4_4s			; no
	bsr.l		setoxm1i			; yes
	bra.b		_L4_6s
_L4_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L4_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L4_6s
_L4_5s:
	bsr.l		setoxm1d			; operand is a DENORM
_L4_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fetoxm1d_
_fetoxm1d_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L4_2d
	bsr.l		setoxm1			; operand is a NORM
	bra.b		_L4_6d
_L4_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L4_3d			; no
	bsr.l		src_zero			; yes
	bra.b		_L4_6d
_L4_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L4_4d			; no
	bsr.l		setoxm1i			; yes
	bra.b		_L4_6d
_L4_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L4_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L4_6d
_L4_5d:
	bsr.l		setoxm1d			; operand is a DENORM
_L4_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fetoxm1x_
_fetoxm1x_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L4_2x
	bsr.l		setoxm1			; operand is a NORM
	bra.b		_L4_6x
_L4_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L4_3x			; no
	bsr.l		src_zero			; yes
	bra.b		_L4_6x
_L4_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L4_4x			; no
	bsr.l		setoxm1i			; yes
	bra.b		_L4_6x
_L4_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L4_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L4_6x
_L4_5x:
	bsr.l		setoxm1d			; operand is a DENORM
_L4_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_ftanhs_
_ftanhs_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L5_2s
	bsr.l		stanh			; operand is a NORM
	bra.b		_L5_6s
_L5_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L5_3s			; no
	bsr.l		src_zero			; yes
	bra.b		_L5_6s
_L5_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L5_4s			; no
	bsr.l		src_one			; yes
	bra.b		_L5_6s
_L5_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L5_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L5_6s
_L5_5s:
	bsr.l		stanhd			; operand is a DENORM
_L5_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_ftanhd_
_ftanhd_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L5_2d
	bsr.l		stanh			; operand is a NORM
	bra.b		_L5_6d
_L5_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L5_3d			; no
	bsr.l		src_zero			; yes
	bra.b		_L5_6d
_L5_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L5_4d			; no
	bsr.l		src_one			; yes
	bra.b		_L5_6d
_L5_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L5_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L5_6d
_L5_5d:
	bsr.l		stanhd			; operand is a DENORM
_L5_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_ftanhx_
_ftanhx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L5_2x
	bsr.l		stanh			; operand is a NORM
	bra.b		_L5_6x
_L5_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L5_3x			; no
	bsr.l		src_zero			; yes
	bra.b		_L5_6x
_L5_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L5_4x			; no
	bsr.l		src_one			; yes
	bra.b		_L5_6x
_L5_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L5_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L5_6x
_L5_5x:
	bsr.l		stanhd			; operand is a DENORM
_L5_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_fatans_
_fatans_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L6_2s
	bsr.l		satan			; operand is a NORM
	bra.b		_L6_6s
_L6_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L6_3s			; no
	bsr.l		src_zero			; yes
	bra.b		_L6_6s
_L6_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L6_4s			; no
	bsr.l		spi_2			; yes
	bra.b		_L6_6s
_L6_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L6_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L6_6s
_L6_5s:
	bsr.l		satand			; operand is a DENORM
_L6_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fatand_
_fatand_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L6_2d
	bsr.l		satan			; operand is a NORM
	bra.b		_L6_6d
_L6_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L6_3d			; no
	bsr.l		src_zero			; yes
	bra.b		_L6_6d
_L6_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L6_4d			; no
	bsr.l		spi_2			; yes
	bra.b		_L6_6d
_L6_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L6_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L6_6d
_L6_5d:
	bsr.l		satand			; operand is a DENORM
_L6_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fatanx_
_fatanx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L6_2x
	bsr.l		satan			; operand is a NORM
	bra.b		_L6_6x
_L6_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L6_3x			; no
	bsr.l		src_zero			; yes
	bra.b		_L6_6x
_L6_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L6_4x			; no
	bsr.l		spi_2			; yes
	bra.b		_L6_6x
_L6_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L6_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L6_6x
_L6_5x:
	bsr.l		satand			; operand is a DENORM
_L6_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_fasins_
_fasins_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L7_2s
	bsr.l		sasin			; operand is a NORM
	bra.b		_L7_6s
_L7_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L7_3s			; no
	bsr.l		src_zero			; yes
	bra.b		_L7_6s
_L7_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L7_4s			; no
	bsr.l		t_operr			; yes
	bra.b		_L7_6s
_L7_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L7_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L7_6s
_L7_5s:
	bsr.l		sasind			; operand is a DENORM
_L7_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fasind_
_fasind_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L7_2d
	bsr.l		sasin			; operand is a NORM
	bra.b		_L7_6d
_L7_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L7_3d			; no
	bsr.l		src_zero			; yes
	bra.b		_L7_6d
_L7_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L7_4d			; no
	bsr.l		t_operr			; yes
	bra.b		_L7_6d
_L7_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L7_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L7_6d
_L7_5d:
	bsr.l		sasind			; operand is a DENORM
_L7_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fasinx_
_fasinx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L7_2x
	bsr.l		sasin			; operand is a NORM
	bra.b		_L7_6x
_L7_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L7_3x			; no
	bsr.l		src_zero			; yes
	bra.b		_L7_6x
_L7_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L7_4x			; no
	bsr.l		t_operr			; yes
	bra.b		_L7_6x
_L7_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L7_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L7_6x
_L7_5x:
	bsr.l		sasind			; operand is a DENORM
_L7_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_fatanhs_
_fatanhs_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L8_2s
	bsr.l		satanh			; operand is a NORM
	bra.b		_L8_6s
_L8_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L8_3s			; no
	bsr.l		src_zero			; yes
	bra.b		_L8_6s
_L8_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L8_4s			; no
	bsr.l		t_operr			; yes
	bra.b		_L8_6s
_L8_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L8_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L8_6s
_L8_5s:
	bsr.l		satanhd			; operand is a DENORM
_L8_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fatanhd_
_fatanhd_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L8_2d
	bsr.l		satanh			; operand is a NORM
	bra.b		_L8_6d
_L8_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L8_3d			; no
	bsr.l		src_zero			; yes
	bra.b		_L8_6d
_L8_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L8_4d			; no
	bsr.l		t_operr			; yes
	bra.b		_L8_6d
_L8_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L8_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L8_6d
_L8_5d:
	bsr.l		satanhd			; operand is a DENORM
_L8_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fatanhx_
_fatanhx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L8_2x
	bsr.l		satanh			; operand is a NORM
	bra.b		_L8_6x
_L8_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L8_3x			; no
	bsr.l		src_zero			; yes
	bra.b		_L8_6x
_L8_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L8_4x			; no
	bsr.l		t_operr			; yes
	bra.b		_L8_6x
_L8_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L8_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L8_6x
_L8_5x:
	bsr.l		satanhd			; operand is a DENORM
_L8_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_ftans_
_ftans_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L9_2s
	bsr.l		stan			; operand is a NORM
	bra.b		_L9_6s
_L9_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L9_3s			; no
	bsr.l		src_zero			; yes
	bra.b		_L9_6s
_L9_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L9_4s			; no
	bsr.l		t_operr			; yes
	bra.b		_L9_6s
_L9_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L9_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L9_6s
_L9_5s:
	bsr.l		stand			; operand is a DENORM
_L9_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_ftand_
_ftand_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L9_2d
	bsr.l		stan			; operand is a NORM
	bra.b		_L9_6d
_L9_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L9_3d			; no
	bsr.l		src_zero			; yes
	bra.b		_L9_6d
_L9_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L9_4d			; no
	bsr.l		t_operr			; yes
	bra.b		_L9_6d
_L9_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L9_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L9_6d
_L9_5d:
	bsr.l		stand			; operand is a DENORM
_L9_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_ftanx_
_ftanx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L9_2x
	bsr.l		stan			; operand is a NORM
	bra.b		_L9_6x
_L9_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L9_3x			; no
	bsr.l		src_zero			; yes
	bra.b		_L9_6x
_L9_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L9_4x			; no
	bsr.l		t_operr			; yes
	bra.b		_L9_6x
_L9_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L9_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L9_6x
_L9_5x:
	bsr.l		stand			; operand is a DENORM
_L9_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_fetoxs_
_fetoxs_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L10_2s
	bsr.l		setox			; operand is a NORM
	bra.b		_L10_6s
_L10_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L10_3s			; no
	bsr.l		ld_pone			; yes
	bra.b		_L10_6s
_L10_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L10_4s			; no
	bsr.l		szr_inf			; yes
	bra.b		_L10_6s
_L10_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L10_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L10_6s
_L10_5s:
	bsr.l		setoxd			; operand is a DENORM
_L10_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fetoxd_
_fetoxd_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L10_2d
	bsr.l		setox			; operand is a NORM
	bra.b		_L10_6d
_L10_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L10_3d			; no
	bsr.l		ld_pone			; yes
	bra.b		_L10_6d
_L10_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L10_4d			; no
	bsr.l		szr_inf			; yes
	bra.b		_L10_6d
_L10_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L10_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L10_6d
_L10_5d:
	bsr.l		setoxd			; operand is a DENORM
_L10_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fetoxx_
_fetoxx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L10_2x
	bsr.l		setox			; operand is a NORM
	bra.b		_L10_6x
_L10_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L10_3x			; no
	bsr.l		ld_pone			; yes
	bra.b		_L10_6x
_L10_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L10_4x			; no
	bsr.l		szr_inf			; yes
	bra.b		_L10_6x
_L10_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L10_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L10_6x
_L10_5x:
	bsr.l		setoxd			; operand is a DENORM
_L10_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_ftwotoxs_
_ftwotoxs_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L11_2s
	bsr.l		stwotox			; operand is a NORM
	bra.b		_L11_6s
_L11_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L11_3s			; no
	bsr.l		ld_pone			; yes
	bra.b		_L11_6s
_L11_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L11_4s			; no
	bsr.l		szr_inf			; yes
	bra.b		_L11_6s
_L11_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L11_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L11_6s
_L11_5s:
	bsr.l		stwotoxd			; operand is a DENORM
_L11_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_ftwotoxd_
_ftwotoxd_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L11_2d
	bsr.l		stwotox			; operand is a NORM
	bra.b		_L11_6d
_L11_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L11_3d			; no
	bsr.l		ld_pone			; yes
	bra.b		_L11_6d
_L11_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L11_4d			; no
	bsr.l		szr_inf			; yes
	bra.b		_L11_6d
_L11_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L11_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L11_6d
_L11_5d:
	bsr.l		stwotoxd			; operand is a DENORM
_L11_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_ftwotoxx_
_ftwotoxx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L11_2x
	bsr.l		stwotox			; operand is a NORM
	bra.b		_L11_6x
_L11_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L11_3x			; no
	bsr.l		ld_pone			; yes
	bra.b		_L11_6x
_L11_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L11_4x			; no
	bsr.l		szr_inf			; yes
	bra.b		_L11_6x
_L11_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L11_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L11_6x
_L11_5x:
	bsr.l		stwotoxd			; operand is a DENORM
_L11_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_ftentoxs_
_ftentoxs_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L12_2s
	bsr.l		stentox			; operand is a NORM
	bra.b		_L12_6s
_L12_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L12_3s			; no
	bsr.l		ld_pone			; yes
	bra.b		_L12_6s
_L12_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L12_4s			; no
	bsr.l		szr_inf			; yes
	bra.b		_L12_6s
_L12_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L12_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L12_6s
_L12_5s:
	bsr.l		stentoxd			; operand is a DENORM
_L12_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_ftentoxd_
_ftentoxd_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L12_2d
	bsr.l		stentox			; operand is a NORM
	bra.b		_L12_6d
_L12_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L12_3d			; no
	bsr.l		ld_pone			; yes
	bra.b		_L12_6d
_L12_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L12_4d			; no
	bsr.l		szr_inf			; yes
	bra.b		_L12_6d
_L12_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L12_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L12_6d
_L12_5d:
	bsr.l		stentoxd			; operand is a DENORM
_L12_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_ftentoxx_
_ftentoxx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L12_2x
	bsr.l		stentox			; operand is a NORM
	bra.b		_L12_6x
_L12_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L12_3x			; no
	bsr.l		ld_pone			; yes
	bra.b		_L12_6x
_L12_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L12_4x			; no
	bsr.l		szr_inf			; yes
	bra.b		_L12_6x
_L12_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L12_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L12_6x
_L12_5x:
	bsr.l		stentoxd			; operand is a DENORM
_L12_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_flogns_
_flogns_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L13_2s
	bsr.l		slogn			; operand is a NORM
	bra.b		_L13_6s
_L13_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L13_3s			; no
	bsr.l		t_dz2			; yes
	bra.b		_L13_6s
_L13_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L13_4s			; no
	bsr.l		sopr_inf			; yes
	bra.b		_L13_6s
_L13_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L13_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L13_6s
_L13_5s:
	bsr.l		slognd			; operand is a DENORM
_L13_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_flognd_
_flognd_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L13_2d
	bsr.l		slogn			; operand is a NORM
	bra.b		_L13_6d
_L13_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L13_3d			; no
	bsr.l		t_dz2			; yes
	bra.b		_L13_6d
_L13_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L13_4d			; no
	bsr.l		sopr_inf			; yes
	bra.b		_L13_6d
_L13_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L13_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L13_6d
_L13_5d:
	bsr.l		slognd			; operand is a DENORM
_L13_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_flognx_
_flognx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L13_2x
	bsr.l		slogn			; operand is a NORM
	bra.b		_L13_6x
_L13_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L13_3x			; no
	bsr.l		t_dz2			; yes
	bra.b		_L13_6x
_L13_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L13_4x			; no
	bsr.l		sopr_inf			; yes
	bra.b		_L13_6x
_L13_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L13_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L13_6x
_L13_5x:
	bsr.l		slognd			; operand is a DENORM
_L13_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_flog10s_
_flog10s_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L14_2s
	bsr.l		slog10			; operand is a NORM
	bra.b		_L14_6s
_L14_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L14_3s			; no
	bsr.l		t_dz2			; yes
	bra.b		_L14_6s
_L14_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L14_4s			; no
	bsr.l		sopr_inf			; yes
	bra.b		_L14_6s
_L14_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L14_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L14_6s
_L14_5s:
	bsr.l		slog10d			; operand is a DENORM
_L14_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_flog10d_
_flog10d_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L14_2d
	bsr.l		slog10			; operand is a NORM
	bra.b		_L14_6d
_L14_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L14_3d			; no
	bsr.l		t_dz2			; yes
	bra.b		_L14_6d
_L14_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L14_4d			; no
	bsr.l		sopr_inf			; yes
	bra.b		_L14_6d
_L14_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L14_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L14_6d
_L14_5d:
	bsr.l		slog10d			; operand is a DENORM
_L14_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_flog10x_
_flog10x_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L14_2x
	bsr.l		slog10			; operand is a NORM
	bra.b		_L14_6x
_L14_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L14_3x			; no
	bsr.l		t_dz2			; yes
	bra.b		_L14_6x
_L14_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L14_4x			; no
	bsr.l		sopr_inf			; yes
	bra.b		_L14_6x
_L14_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L14_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L14_6x
_L14_5x:
	bsr.l		slog10d			; operand is a DENORM
_L14_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_flog2s_
_flog2s_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L15_2s
	bsr.l		slog2			; operand is a NORM
	bra.b		_L15_6s
_L15_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L15_3s			; no
	bsr.l		t_dz2			; yes
	bra.b		_L15_6s
_L15_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L15_4s			; no
	bsr.l		sopr_inf			; yes
	bra.b		_L15_6s
_L15_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L15_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L15_6s
_L15_5s:
	bsr.l		slog2d			; operand is a DENORM
_L15_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_flog2d_
_flog2d_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L15_2d
	bsr.l		slog2			; operand is a NORM
	bra.b		_L15_6d
_L15_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L15_3d			; no
	bsr.l		t_dz2			; yes
	bra.b		_L15_6d
_L15_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L15_4d			; no
	bsr.l		sopr_inf			; yes
	bra.b		_L15_6d
_L15_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L15_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L15_6d
_L15_5d:
	bsr.l		slog2d			; operand is a DENORM
_L15_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_flog2x_
_flog2x_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L15_2x
	bsr.l		slog2			; operand is a NORM
	bra.b		_L15_6x
_L15_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L15_3x			; no
	bsr.l		t_dz2			; yes
	bra.b		_L15_6x
_L15_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L15_4x			; no
	bsr.l		sopr_inf			; yes
	bra.b		_L15_6x
_L15_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L15_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L15_6x
_L15_5x:
	bsr.l		slog2d			; operand is a DENORM
_L15_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_fcoshs_
_fcoshs_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L16_2s
	bsr.l		scosh			; operand is a NORM
	bra.b		_L16_6s
_L16_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L16_3s			; no
	bsr.l		ld_pone			; yes
	bra.b		_L16_6s
_L16_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L16_4s			; no
	bsr.l		ld_pinf			; yes
	bra.b		_L16_6s
_L16_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L16_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L16_6s
_L16_5s:
	bsr.l		scoshd			; operand is a DENORM
_L16_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fcoshd_
_fcoshd_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L16_2d
	bsr.l		scosh			; operand is a NORM
	bra.b		_L16_6d
_L16_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L16_3d			; no
	bsr.l		ld_pone			; yes
	bra.b		_L16_6d
_L16_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L16_4d			; no
	bsr.l		ld_pinf			; yes
	bra.b		_L16_6d
_L16_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L16_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L16_6d
_L16_5d:
	bsr.l		scoshd			; operand is a DENORM
_L16_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fcoshx_
_fcoshx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L16_2x
	bsr.l		scosh			; operand is a NORM
	bra.b		_L16_6x
_L16_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L16_3x			; no
	bsr.l		ld_pone			; yes
	bra.b		_L16_6x
_L16_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L16_4x			; no
	bsr.l		ld_pinf			; yes
	bra.b		_L16_6x
_L16_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L16_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L16_6x
_L16_5x:
	bsr.l		scoshd			; operand is a DENORM
_L16_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_facoss_
_facoss_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L17_2s
	bsr.l		sacos			; operand is a NORM
	bra.b		_L17_6s
_L17_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L17_3s			; no
	bsr.l		ld_ppi2			; yes
	bra.b		_L17_6s
_L17_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L17_4s			; no
	bsr.l		t_operr			; yes
	bra.b		_L17_6s
_L17_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L17_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L17_6s
_L17_5s:
	bsr.l		sacosd			; operand is a DENORM
_L17_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_facosd_
_facosd_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L17_2d
	bsr.l		sacos			; operand is a NORM
	bra.b		_L17_6d
_L17_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L17_3d			; no
	bsr.l		ld_ppi2			; yes
	bra.b		_L17_6d
_L17_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L17_4d			; no
	bsr.l		t_operr			; yes
	bra.b		_L17_6d
_L17_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L17_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L17_6d
_L17_5d:
	bsr.l		sacosd			; operand is a DENORM
_L17_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_facosx_
_facosx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L17_2x
	bsr.l		sacos			; operand is a NORM
	bra.b		_L17_6x
_L17_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L17_3x			; no
	bsr.l		ld_ppi2			; yes
	bra.b		_L17_6x
_L17_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L17_4x			; no
	bsr.l		t_operr			; yes
	bra.b		_L17_6x
_L17_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L17_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L17_6x
_L17_5x:
	bsr.l		sacosd			; operand is a DENORM
_L17_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_fgetexps_
_fgetexps_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L18_2s
	bsr.l		sgetexp			; operand is a NORM
	bra.b		_L18_6s
_L18_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L18_3s			; no
	bsr.l		src_zero			; yes
	bra.b		_L18_6s
_L18_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L18_4s			; no
	bsr.l		t_operr			; yes
	bra.b		_L18_6s
_L18_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L18_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L18_6s
_L18_5s:
	bsr.l		sgetexpd			; operand is a DENORM
_L18_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fgetexpd_
_fgetexpd_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L18_2d
	bsr.l		sgetexp			; operand is a NORM
	bra.b		_L18_6d
_L18_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L18_3d			; no
	bsr.l		src_zero			; yes
	bra.b		_L18_6d
_L18_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L18_4d			; no
	bsr.l		t_operr			; yes
	bra.b		_L18_6d
_L18_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L18_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L18_6d
_L18_5d:
	bsr.l		sgetexpd			; operand is a DENORM
_L18_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fgetexpx_
_fgetexpx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L18_2x
	bsr.l		sgetexp			; operand is a NORM
	bra.b		_L18_6x
_L18_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L18_3x			; no
	bsr.l		src_zero			; yes
	bra.b		_L18_6x
_L18_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L18_4x			; no
	bsr.l		t_operr			; yes
	bra.b		_L18_6x
_L18_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L18_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L18_6x
_L18_5x:
	bsr.l		sgetexpd			; operand is a DENORM
_L18_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_fgetmans_
_fgetmans_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L19_2s
	bsr.l		sgetman			; operand is a NORM
	bra.b		_L19_6s
_L19_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L19_3s			; no
	bsr.l		src_zero			; yes
	bra.b		_L19_6s
_L19_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L19_4s			; no
	bsr.l		t_operr			; yes
	bra.b		_L19_6s
_L19_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L19_5s			; no
	bsr.l		src_qnan			; yes
	bra.b		_L19_6s
_L19_5s:
	bsr.l		sgetmand			; operand is a DENORM
_L19_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fgetmand_
_fgetmand_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L19_2d
	bsr.l		sgetman			; operand is a NORM
	bra.b		_L19_6d
_L19_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L19_3d			; no
	bsr.l		src_zero			; yes
	bra.b		_L19_6d
_L19_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L19_4d			; no
	bsr.l		t_operr			; yes
	bra.b		_L19_6d
_L19_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L19_5d			; no
	bsr.l		src_qnan			; yes
	bra.b		_L19_6d
_L19_5d:
	bsr.l		sgetmand			; operand is a DENORM
_L19_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fgetmanx_
_fgetmanx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L19_2x
	bsr.l		sgetman			; operand is a NORM
	bra.b		_L19_6x
_L19_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L19_3x			; no
	bsr.l		src_zero			; yes
	bra.b		_L19_6x
_L19_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L19_4x			; no
	bsr.l		t_operr			; yes
	bra.b		_L19_6x
_L19_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L19_5x			; no
	bsr.l		src_qnan			; yes
	bra.b		_L19_6x
_L19_5x:
	bsr.l		sgetmand			; operand is a DENORM
_L19_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MONADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_fsincoss_
_fsincoss_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L20_2s
	bsr.l		ssincos			; operand is a NORM
	bra.b		_L20_6s
_L20_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L20_3s			; no
	bsr.l		ssincosz			; yes
	bra.b		_L20_6s
_L20_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L20_4s			; no
	bsr.l		ssincosi			; yes
	bra.b		_L20_6s
_L20_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L20_5s			; no
	bsr.l		ssincosqnan			; yes
	bra.b		_L20_6s
_L20_5s:
	bsr.l		ssincosd			; operand is a DENORM
_L20_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		fp0-fp1,-(sp)		; store off fp0/fp1
	fmovem.x		(sp)+,fp1		; fp0 now in fp1
	fmovem.x		(sp)+,fp0		; fp1 now in fp0
	unlk		a6
	rts

	.globl		_fsincosd_
_fsincosd_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl input
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	move.b		d1,STAG(a6)
	tst.b		d1
	bne.b		_L20_2d
	bsr.l		ssincos			; operand is a NORM
	bra.b		_L20_6d
_L20_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L20_3d			; no
	bsr.l		ssincosz			; yes
	bra.b		_L20_6d
_L20_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L20_4d			; no
	bsr.l		ssincosi			; yes
	bra.b		_L20_6d
_L20_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L20_5d			; no
	bsr.l		ssincosqnan			; yes
	bra.b		_L20_6d
_L20_5d:
	bsr.l		ssincosd			; operand is a DENORM
_L20_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		fp0-fp1,-(sp)		; store off fp0/fp1
	fmovem.x		(sp)+,fp1		; fp0 now in fp1
	fmovem.x		(sp)+,fp0		; fp1 now in fp0
	unlk		a6
	rts

	.globl		_fsincosx_
_fsincosx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_SRC(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext input
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.b		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	tst.b		d1
	bne.b		_L20_2x
	bsr.l		ssincos			; operand is a NORM
	bra.b		_L20_6x
_L20_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L20_3x			; no
	bsr.l		ssincosz			; yes
	bra.b		_L20_6x
_L20_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L20_4x			; no
	bsr.l		ssincosi			; yes
	bra.b		_L20_6x
_L20_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L20_5x			; no
	bsr.l		ssincosqnan			; yes
	bra.b		_L20_6x
_L20_5x:
	bsr.l		ssincosd			; operand is a DENORM
_L20_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		fp0-fp1,-(sp)		; store off fp0/fp1
	fmovem.x		(sp)+,fp1		; fp0 now in fp1
	fmovem.x		(sp)+,fp0		; fp1 now in fp0
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; DYADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_frems_
_frems_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl dst
	fmove.x		fp0,FP_DST(a6)
	lea		FP_DST(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,DTAG(a6)

	fmove.s		0xc(a6),fp0		; load sgl src
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.l		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	lea		FP_SRC(a6),a0		; pass ptr to src
	lea		FP_DST(a6),a1		; pass ptr to dst

	tst.b		d1
	bne.b		_L21_2s
	bsr.l		srem_snorm			; operand is a NORM
	bra.b		_L21_6s
_L21_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L21_3s			; no
	bsr.l		srem_szero			; yes
	bra.b		_L21_6s
_L21_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L21_4s			; no
	bsr.l		srem_sinf			; yes
	bra.b		_L21_6s
_L21_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L21_5s			; no
	bsr.l		sop_sqnan			; yes
	bra.b		_L21_6s
_L21_5s:
	bsr.l		srem_sdnrm			; operand is a DENORM
_L21_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fremd_
_fremd_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl dst
	fmove.x		fp0,FP_DST(a6)
	lea		FP_DST(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,DTAG(a6)

	fmove.d		0x10(a6),fp0		; load dbl src
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.l		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	lea		FP_SRC(a6),a0		; pass ptr to src
	lea		FP_DST(a6),a1		; pass ptr to dst

	tst.b		d1
	bne.b		_L21_2d
	bsr.l		srem_snorm			; operand is a NORM
	bra.b		_L21_6d
_L21_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L21_3d			; no
	bsr.l		srem_szero			; yes
	bra.b		_L21_6d
_L21_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L21_4d			; no
	bsr.l		srem_sinf			; yes
	bra.b		_L21_6d
_L21_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L21_5d			; no
	bsr.l		sop_sqnan			; yes
	bra.b		_L21_6d
_L21_5d:
	bsr.l		srem_sdnrm			; operand is a DENORM
_L21_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fremx_
_fremx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_DST(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext dst
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,DTAG(a6)

	lea		FP_SRC(a6),a0
	move.l		0x14+0x0(a6),0x0(a0)	; load ext src
	move.l		0x14+0x4(a6),0x4(a0)
	move.l		0x14+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.l		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	lea		FP_SRC(a6),a0		; pass ptr to src
	lea		FP_DST(a6),a1		; pass ptr to dst

	tst.b		d1
	bne.b		_L21_2x
	bsr.l		srem_snorm			; operand is a NORM
	bra.b		_L21_6x
_L21_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L21_3x			; no
	bsr.l		srem_szero			; yes
	bra.b		_L21_6x
_L21_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L21_4x			; no
	bsr.l		srem_sinf			; yes
	bra.b		_L21_6x
_L21_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L21_5x			; no
	bsr.l		sop_sqnan			; yes
	bra.b		_L21_6x
_L21_5x:
	bsr.l		srem_sdnrm			; operand is a DENORM
_L21_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; DYADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_fmods_
_fmods_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl dst
	fmove.x		fp0,FP_DST(a6)
	lea		FP_DST(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,DTAG(a6)

	fmove.s		0xc(a6),fp0		; load sgl src
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.l		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	lea		FP_SRC(a6),a0		; pass ptr to src
	lea		FP_DST(a6),a1		; pass ptr to dst

	tst.b		d1
	bne.b		_L22_2s
	bsr.l		smod_snorm			; operand is a NORM
	bra.b		_L22_6s
_L22_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L22_3s			; no
	bsr.l		smod_szero			; yes
	bra.b		_L22_6s
_L22_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L22_4s			; no
	bsr.l		smod_sinf			; yes
	bra.b		_L22_6s
_L22_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L22_5s			; no
	bsr.l		sop_sqnan			; yes
	bra.b		_L22_6s
_L22_5s:
	bsr.l		smod_sdnrm			; operand is a DENORM
_L22_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fmodd_
_fmodd_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl dst
	fmove.x		fp0,FP_DST(a6)
	lea		FP_DST(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,DTAG(a6)

	fmove.d		0x10(a6),fp0		; load dbl src
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.l		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	lea		FP_SRC(a6),a0		; pass ptr to src
	lea		FP_DST(a6),a1		; pass ptr to dst

	tst.b		d1
	bne.b		_L22_2d
	bsr.l		smod_snorm			; operand is a NORM
	bra.b		_L22_6d
_L22_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L22_3d			; no
	bsr.l		smod_szero			; yes
	bra.b		_L22_6d
_L22_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L22_4d			; no
	bsr.l		smod_sinf			; yes
	bra.b		_L22_6d
_L22_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L22_5d			; no
	bsr.l		sop_sqnan			; yes
	bra.b		_L22_6d
_L22_5d:
	bsr.l		smod_sdnrm			; operand is a DENORM
_L22_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fmodx_
_fmodx_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_DST(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext dst
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,DTAG(a6)

	lea		FP_SRC(a6),a0
	move.l		0x14+0x0(a6),0x0(a0)	; load ext src
	move.l		0x14+0x4(a6),0x4(a0)
	move.l		0x14+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.l		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	lea		FP_SRC(a6),a0		; pass ptr to src
	lea		FP_DST(a6),a1		; pass ptr to dst

	tst.b		d1
	bne.b		_L22_2x
	bsr.l		smod_snorm			; operand is a NORM
	bra.b		_L22_6x
_L22_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L22_3x			; no
	bsr.l		smod_szero			; yes
	bra.b		_L22_6x
_L22_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L22_4x			; no
	bsr.l		smod_sinf			; yes
	bra.b		_L22_6x
_L22_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L22_5x			; no
	bsr.l		sop_sqnan			; yes
	bra.b		_L22_6x
_L22_5x:
	bsr.l		smod_sdnrm			; operand is a DENORM
_L22_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; DYADIC TEMPLATE							
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		_fscales_
_fscales_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.s		0x8(a6),fp0		; load sgl dst
	fmove.x		fp0,FP_DST(a6)
	lea		FP_DST(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,DTAG(a6)

	fmove.s		0xc(a6),fp0		; load sgl src
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.l		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	lea		FP_SRC(a6),a0		; pass ptr to src
	lea		FP_DST(a6),a1		; pass ptr to dst

	tst.b		d1
	bne.b		_L23_2s
	bsr.l		sscale_snorm			; operand is a NORM
	bra.b		_L23_6s
_L23_2s:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L23_3s			; no
	bsr.l		sscale_szero			; yes
	bra.b		_L23_6s
_L23_3s:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L23_4s			; no
	bsr.l		sscale_sinf			; yes
	bra.b		_L23_6s
_L23_4s:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L23_5s			; no
	bsr.l		sop_sqnan			; yes
	bra.b		_L23_6s
_L23_5s:
	bsr.l		sscale_sdnrm			; operand is a DENORM
_L23_6s:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fscaled_
_fscaled_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	fmove.d		0x8(a6),fp0		; load dbl dst
	fmove.x		fp0,FP_DST(a6)
	lea		FP_DST(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,DTAG(a6)

	fmove.d		0x10(a6),fp0		; load dbl src
	fmove.x		fp0,FP_SRC(a6)
	lea		FP_SRC(a6),a0
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.l		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	lea		FP_SRC(a6),a0		; pass ptr to src
	lea		FP_DST(a6),a1		; pass ptr to dst

	tst.b		d1
	bne.b		_L23_2d
	bsr.l		sscale_snorm			; operand is a NORM
	bra.b		_L23_6d
_L23_2d:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L23_3d			; no
	bsr.l		sscale_szero			; yes
	bra.b		_L23_6d
_L23_3d:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L23_4d			; no
	bsr.l		sscale_sinf			; yes
	bra.b		_L23_6d
_L23_4d:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L23_5d			; no
	bsr.l		sop_sqnan			; yes
	bra.b		_L23_6d
_L23_5d:
	bsr.l		sscale_sdnrm			; operand is a DENORM
_L23_6d:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts

	.globl		_fscalex_
_fscalex_:
	link		a6,#-LOCAL_SIZE

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	; save d0-d1/a0-a1
	fmovem.l		fpcr/fpsr,USER_FPCR(a6) ; save ctrl regs
	fmovem.x		fp0-fp1,EXC_FP0(a6)	; save fp0/fp1

	fmove.l		#0x0,fpcr		; zero FPCR

;
;	copy, convert, and tag input argument
;
	lea		FP_DST(a6),a0
	move.l		0x8+0x0(a6),0x0(a0)	; load ext dst
	move.l		0x8+0x4(a6),0x4(a0)
	move.l		0x8+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,DTAG(a6)

	lea		FP_SRC(a6),a0
	move.l		0x14+0x0(a6),0x0(a0)	; load ext src
	move.l		0x14+0x4(a6),0x4(a0)
	move.l		0x14+0x8(a6),0x8(a0)
	bsr.l		tag			; fetch operand type
	move.b		d0,STAG(a6)
	move.l		d0,d1

	andi.l		#0x00ff00ff,USER_FPSR(a6)

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	; pass rnd mode,prec

	lea		FP_SRC(a6),a0		; pass ptr to src
	lea		FP_DST(a6),a1		; pass ptr to dst

	tst.b		d1
	bne.b		_L23_2x
	bsr.l		sscale_snorm			; operand is a NORM
	bra.b		_L23_6x
_L23_2x:
	cmpi.b		#ZERO,d1		; is operand a ZERO?
	bne.b		_L23_3x			; no
	bsr.l		sscale_szero			; yes
	bra.b		_L23_6x
_L23_3x:
	cmpi.b		#INF,d1		; is operand an INF?
	bne.b		_L23_4x			; no
	bsr.l		sscale_sinf			; yes
	bra.b		_L23_6x
_L23_4x:
	cmpi.b		#QNAN,d1		; is operand a QNAN?
	bne.b		_L23_5x			; no
	bsr.l		sop_sqnan			; yes
	bra.b		_L23_6x
_L23_5x:
	bsr.l		sscale_sdnrm			; operand is a DENORM
_L23_6x:

;
;	Result is now in FP0
;
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	; restore d0-d1/a0-a1
	fmovem.l		USER_FPCR(a6),fpcr/fpsr ; restore ctrl regs
	fmovem.x		EXC_FP1(a6),fp1	; restore fp1
	unlk		a6
	rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; XDEF ****************************************************************	
;	tag(): return the optype of the input ext fp number		
;									
;	This routine is used by the 060FPLSP.				
;									
; XREF ****************************************************************	
;	None								
;									
; INPUT ***************************************************************	
;	a0 = pointer to extended precision operand			
;									
; OUTPUT **************************************************************	
;	d0 = value of type tag						
;		one of: NORM, INF, QNAN, SNAN, DENORM, ZERO		
;									
; ALGORITHM ***********************************************************	
;	Simply test the exponent, j-bit, and mantissa values to		
; determine the type of operand.					
;	If it's an unnormalized zero, alter the operand and force it	
; to be a normal zero.							
;									
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.globl		tag
tag:
	move.w		FTEMP_EX.w(a0), d0	; extract exponent
	andi.w		#0x7fff, d0		; strip off sign
	cmpi.w		#0x7fff,d0		; is (EXP == MAX)?
	beq.b		inf_or_nan_x
not_inf_or_nan_x:
	btst		#0x7,FTEMP_HI(a0)
	beq.b		not_norm_x
is_norm_x:
	move.b		#NORM, d0
	rts
not_norm_x:
	tst.w		d0			; is exponent = 0?
	bne.b		is_unnorm_x
not_unnorm_x:
	tst.l		FTEMP_HI(a0)
	bne.b		is_denorm_x
	tst.l		FTEMP_LO(a0)
	bne.b		is_denorm_x
is_zero_x:
	move.b		#ZERO, d0
	rts
is_denorm_x:
	move.b		#DENORM, d0
	rts
is_unnorm_x:
	bsr.l		unnorm_fix		; convert to norm,denorm,or zero
	rts
is_unnorm_reg_x:
	move.b		#UNNORM, d0
	rts
inf_or_nan_x:
	tst.l		FTEMP_LO(a0)
	bne.b		is_nan_x
	move.l		FTEMP_HI(a0), d0
	andi.l		#0x7fffffff, d0	; msb is a don't care!
	bne.b		is_nan_x
is_inf_x:
	move.b		#INF, d0
	rts
is_nan_x:
	move.b		#QNAN, d0
	rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

qnan:	.dc.l		0x7fff0000, 0xffffffff, 0xffffffff

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; XDEF ****************************************************************	
;	t_dz(): Handle DZ exception for "flogn" emulation.	
;	t_dz2(): Handle 060FPLSP dz exception for "fatanh" emulation.	
;									
;	These routines are used by the 060FPLSP package.		
;									
; XREF ****************************************************************	
;	None								
;									
; INPUT ***************************************************************	
;	a0 = pointer to extended precision source operand.		
;									
; OUTPUT **************************************************************	
;	fp0 = default DZ result.					
;									
; ALGORITHM ***********************************************************	
;	Transcendental emulation for the 060FPLSP has detected that	
; a DZ exception should occur for the instruction. If DZ is disabled,	
; return the default result.						
;	If DZ is enabled, the dst operand should be returned unscathed	
; in fp0 while fp1 is used to create a DZ exception so that the		
; operating system can log that such an event occurred.			
;									
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.globl		t_dz
t_dz:
	tst.b		SRC_EX.w(a0)		; check sign for neg or pos
	bpl.b		dz_pinf			; branch if pos sign

	.globl		t_dz2
t_dz2:
	ori.l		#dzinf_mask+neg_mask,USER_FPSR(a6) ; set N/I/DZ/ADZ

	btst		#dz_bit,FPCR_ENABLE(a6)
	bne.b		dz_minf_ena

; dz is disabled. return a -INF.
	fmove.s		#0xff800000,fp0	; return -INF
	rts

; dz is enabled. create a dz exception so the user can record it
; but use fp1 instead. return the dst operand unscathed in fp0.
dz_minf_ena:
	fmovem.x		EXC_FP0(a6),fp0	; return fp0 unscathed
	fmove.l		USER_FPCR(a6),fpcr
	fmove.s		#0xbf800000,fp1	; load -1
	fdiv.s		#0x00000000,fp1	; -1 / 0
	rts

dz_pinf:
	ori.l		#dzinf_mask,USER_FPSR(a6) ; set I/DZ/ADZ

	btst		#dz_bit,FPCR_ENABLE(a6)
	bne.b		dz_pinf_ena

; dz is disabled. return a +INF.
	fmove.s		#0x7f800000,fp0	; return +INF
	rts

; dz is enabled. create a dz exception so the user can record it
; but use fp1 instead. return the dst operand unscathed in fp0.
dz_pinf_ena:
	fmovem.x		EXC_FP0(a6),fp0	; return fp0 unscathed
	fmove.l		USER_FPCR(a6),fpcr
	fmove.s		#0x3f800000,fp1	; load +1
	fdiv.s		#0x00000000,fp1	; +1 / 0
	rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; XDEF ****************************************************************	
;	t_operr(): Handle 060FPLSP OPERR exception during emulation.	
;									
;	This routine is used by the 060FPLSP package.			
;									
; XREF ****************************************************************	
;	None.								
;									
; INPUT ***************************************************************	
;	fp1 = source operand						
;									
; OUTPUT **************************************************************	
;	fp0 = default result						
;	fp1 = unchanged							
;									
; ALGORITHM ***********************************************************	
;	An operand error should occur as the result of transcendental	
; emulation in the 060FPLSP. If OPERR is disabled, just return a NAN	
; in fp0. If OPERR is enabled, return the dst operand unscathed in fp0	
; and the source operand in fp1. Use fp2 to create an OPERR exception	
; so that the operating system can log the event.			
;									
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.globl		t_operr
t_operr:
	ori.l		#opnan_mask,USER_FPSR(a6) ; set NAN/OPERR/AIOP

	btst		#operr_bit,FPCR_ENABLE(a6)
	bne.b		operr_ena

; operr is disabled. return a QNAN in fp0
	fmovem.x		qnan(pc),fp0		; return QNAN
	rts

; operr is enabled. create an operr exception so the user can record it
; but use fp2 instead. return the dst operand unscathed in fp0.
operr_ena:
	fmovem.x		EXC_FP0(a6),fp0	; return fp0 unscathed
	fmove.l		USER_FPCR(a6),fpcr
	fmovem.x		fp2,-(sp)		; save fp2
	fmove.s		#0x7f800000,fp2	; load +INF
	fmul.s		#0x00000000,fp2	; +INF x 0
	fmovem.x		(sp)+,fp2		; restore fp2
	rts

pls_huge:
	.dc.l		0x7ffe0000,0xffffffff,0xffffffff
mns_huge:
	.dc.l		0xfffe0000,0xffffffff,0xffffffff
pls_tiny:
	.dc.l		0x00000000,0x80000000,0x00000000
mns_tiny:
	.dc.l		0x80000000,0x80000000,0x00000000

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; XDEF ****************************************************************	
;	t_unfl(): Handle 060FPLSP underflow exception during emulation.	
;	t_unfl2(): Handle 060FPLSP underflow exception during		
;	           emulation. result always positive.			
;									
;	This routine is used by the 060FPLSP package.			
;									
; XREF ****************************************************************	
;	None.								
;									
; INPUT ***************************************************************	
;	a0 = pointer to extended precision source operand		
;									
; OUTPUT **************************************************************	
;	fp0 = default underflow result					
;									
; ALGORITHM ***********************************************************	
;	An underflow should occur as the result of transcendental	
; emulation in the 060FPLSP. Create an underflow by using "fmul"	
; and two very small numbers of appropriate sign so the operating	
; system can log the event.						
;									
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.globl		t_unfl
t_unfl:
	tst.b		SRC_EX.w(a0)
	bpl.b		unf_pos

	.globl		t_unfl2
t_unfl2:
	ori.l		#unfinx_mask+neg_mask,USER_FPSR(a6) ; set N/UNFL/INEX2/AUNFL/AINEX

	fmove.l		USER_FPCR(a6),fpcr
	fmovem.x	mns_tiny(pc),fp0
	fmul.x		pls_tiny(pc),fp0

	fmove.l		fpsr,d0
	rol.l		#0x8,d0
	move.b		d0,FPSR_CC(a6)
	rts
unf_pos:
	ori.w		#unfinx_mask,FPSR_EXCEPT(a6) ; set UNFL/INEX2/AUNFL/AINEX

	fmove.l		USER_FPCR(a6),fpcr
	fmovem.x	pls_tiny(pc),fp0
	fmul.x		fp0,fp0

	fmove.l		fpsr,d0
	rol.l		#0x8,d0
	move.b		d0,FPSR_CC(a6)
	rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; XDEF ****************************************************************	
;	t_ovfl(): Handle 060FPLSP overflow exception during emulation.	
;		  (monadic)						
;	t_ovfl2(): Handle 060FPLSP overflow exception during		
;	           emulation. result always positive. (dyadic)		
;	t_ovfl_sc(): Handle 060FPLSP overflow exception during		
;	             emulation for "fscale".				
;									
;	This routine is used by the 060FPLSP package.			
;									
; XREF ****************************************************************	
;	None.								
;									
; INPUT ***************************************************************	
;	a0 = pointer to extended precision source operand		
;									
; OUTPUT **************************************************************	
;	fp0 = default underflow result					
;									
; ALGORITHM ***********************************************************	
;	An overflow should occur as the result of transcendental	
; emulation in the 060FPLSP. Create an overflow by using "fmul"		
; and two very lareg numbers of appropriate sign so the operating	
; system can log the event.						
;	For t_ovfl_sc() we take special care not to lose the INEX2 bit.	
;									
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.globl		t_ovfl_sc
t_ovfl_sc:
	ori.l		#ovfl_inx_mask,USER_FPSR(a6) ; set OVFL/AOVFL/AINEX

	move.b		d0,d1			; fetch rnd prec,mode
	andi.b		#0xc0,d1		; extract prec
	beq.w		ovfl_work

; dst op is a DENORM. we have to normalize the mantissa to see if the
; result would be inexact for the given precision. make a copy of the
; dst so we don't screw up the version passed to us.
	move.w		LOCAL_EX.w(a0),FP_SCR0_EX(a6)
	move.l		LOCAL_HI(a0),FP_SCR0_HI(a6)
	move.l		LOCAL_LO(a0),FP_SCR0_LO(a6)
	lea		FP_SCR0(a6),a0	; pass ptr to FP_SCR0
	movem.l		d0-d1/a0,-(sp)	; save d0-d1/a0
	bsr.l		norm			; normalize mantissa
	movem.l		(sp)+,d0-d1/a0		; restore d0-d1/a0

	cmpi.b		#0x40,d1		; is precision sgl?
	bne.b		ovfl_sc_dbl		; no; dbl
ovfl_sc_sgl:
	tst.l		LOCAL_LO(a0)		; is lo lw of sgl set?
	bne.b		ovfl_sc_inx		; yes
	tst.b		3+LOCAL_HI(a0)		; is lo byte of hi lw set?
	bne.b		ovfl_sc_inx		; yes
	bra.w		ovfl_work		; don't set INEX2
ovfl_sc_dbl:
	move.l		LOCAL_LO(a0),d1	; are any of lo 11 bits of
	andi.l		#0x7ff,d1		; dbl mantissa set?
	beq.w		ovfl_work		; no; don't set INEX2
ovfl_sc_inx:
	ori.l		#inex2_mask,USER_FPSR(a6) ; set INEX2
	bra.b		ovfl_work		; continue

	.globl		t_ovfl
t_ovfl:
	ori.w		#ovfinx_mask,FPSR_EXCEPT(a6) ; set OVFL/INEX2/AOVFL/AINEX
ovfl_work:
	tst.b		SRC_EX.w(a0)
	bpl.b		ovfl_p
ovfl_m:
	fmove.l		USER_FPCR(a6),fpcr
	fmovem.x	mns_huge(pc),fp0
	fmul.x		pls_huge(pc),fp0

	fmove.l		fpsr,d0
	rol.l		#0x8,d0
	ori.b		#0x0,d0 ; ZZZ was neg_mask
	move.b		d0,FPSR_CC(a6)
	rts
ovfl_p:
	fmove.l		USER_FPCR(a6),fpcr
	fmovem.x	pls_huge(pc),fp0
	fmul.x		pls_huge(pc),fp0

	fmove.l		fpsr,d0
	rol.l		#0x8,d0
	move.b		d0,FPSR_CC(a6)
	rts

	.globl		t_ovfl2
t_ovfl2:
	ori.w		#ovfinx_mask,FPSR_EXCEPT(a6) ; set OVFL/INEX2/AOVFL/AINEX
	fmove.l		USER_FPCR(a6),fpcr
	fmovem.x	pls_huge(pc),fp0
	fmul.x		pls_huge(pc),fp0

	fmove.l		fpsr,d0
	rol.l		#0x8,d0
	move.b		d0,FPSR_CC(a6)
	rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; XDEF ****************************************************************	
;	t_catch(): Handle 060FPLSP OVFL,UNFL,or INEX2 exception during	
;		   emulation.						
;	t_catch2(): Handle 060FPLSP OVFL,UNFL,or INEX2 exception during	
;		    emulation.						
;									
;	These routines are used by the 060FPLSP package.		
;									
; XREF ****************************************************************	
;	None.								
;									
; INPUT ***************************************************************	
;	fp0 = default underflow or overflow result			
;									
; OUTPUT **************************************************************	
;	fp0 = default result						
;									
; ALGORITHM ***********************************************************	
;	If an overflow or underflow occurred during the last		
; instruction of transcendental 060FPLSP emulation, then it has already	
; occurred and has been logged. Now we need to see if an inexact	
; exception should occur.						
;									
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.globl		t_catch2
t_catch2:
	fmove.l		fpsr,d0
	or.l		d0,USER_FPSR(a6)
	bra.b		inx2_work

	.globl		t_catch
t_catch:
	fmove.l		fpsr,d0
	or.l		d0,USER_FPSR(a6)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; XDEF ****************************************************************	
;	t_inx2(): Handle inexact 060FPLSP exception during emulation.	
;	t_pinx2(): Handle inexact 060FPLSP exception for "+" results.	
;	t_minx2(): Handle inexact 060FPLSP exception for "-" results.	
;									
; XREF ****************************************************************	
;	None.								
;									
; INPUT ***************************************************************	
;	fp0 = default result						
;									
; OUTPUT **************************************************************	
;	fp0 = default result						
;									
; ALGORITHM ***********************************************************	
;	The last instruction of transcendental emulation for the	
; 060FPLSP should be inexact. So, if inexact is enabled, then we create	
; the event here by adding a large and very small number together	
; so that the operating system can log the event.			
;	Must check, too, if the result was zero, in which case we just	
; set the FPSR bits and return.						
;									
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.globl		t_inx2
t_inx2:
	fblt.w		t_minx2
	fbeq.w		inx2_zero

	.globl		t_pinx2
t_pinx2:
	ori.w		#inx2a_mask,FPSR_EXCEPT(a6) ; set INEX2/AINEX
	bra.b		inx2_work

	.globl		t_minx2
t_minx2:
	ori.l		#inx2a_mask+neg_mask,USER_FPSR(a6)

inx2_work:
	btst		#inex2_bit,FPCR_ENABLE(a6) ; is inexact enabled?
	bne.b		inx2_work_ena		; yes
	rts
inx2_work_ena:
	fmove.l		USER_FPCR(a6),fpcr	; insert user's exceptions
	fmove.s		#0x3f800000,fp1	; load +1
	fadd.x		pls_tiny(pc),fp1	; cause exception
	rts

inx2_zero:
	move.b		#z_bmask,FPSR_CC(a6)
	ori.w		#inx2a_mask,2+USER_FPSR(a6) ; set INEX/AINEX
	rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; XDEF ****************************************************************	
;	t_extdnrm(): Handle DENORM inputs in 060FPLSP.			
;	t_resdnrm(): Handle DENORM inputs in 060FPLSP for "fscale".	
;									
;	This routine is used by the 060FPLSP package.			
;									
; XREF ****************************************************************	
;	None.								
;									
; INPUT ***************************************************************	
;	a0 = pointer to extended precision input operand		
;									
; OUTPUT **************************************************************	
;	fp0 = default result						
;									
; ALGORITHM ***********************************************************	
;	For all functions that have a denormalized input and that	
; f(x)=x, this is the entry point.					
;	DENORM value is moved using "fmove" which triggers an exception	
; if enabled so the operating system can log the event.			
;									
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.globl		t_extdnrm
t_extdnrm:
	fmove.l		USER_FPCR(a6),fpcr
	fmove.x		SRC_EX.w(a0),fp0
	fmove.l		fpsr,d0
	ori.l		#unfinx_mask,d0
	or.l		d0,USER_FPSR(a6)
	rts

	.globl		t_resdnrm
t_resdnrm:
	fmove.l		USER_FPCR(a6),fpcr
	fmove.x		SRC_EX.w(a0),fp0
	fmove.l		fpsr,d0
	or.l		d0,USER_FPSR(a6)
	rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; sto_cos:
;	This is used by fsincos library emulation. The correct
; values are already in fp0 and fp1 so we do nothing here.
;
	.globl		sto_cos
sto_cos:
	rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
;	dst_qnan --- force result when destination is a NaN
;
	.globl		dst_qnan
dst_qnan:
	fmove.x		DST.w(a1),fp0
	tst.b		DST_EX.w(a1)
	bmi.b		dst_qnan_m
dst_qnan_p:
	move.b		#nan_bmask,FPSR_CC(a6)
	rts
dst_qnan_m:
	move.b		#nan_bmask+neg_bmask,FPSR_CC(a6)
	rts

;
;	src_qnan --- force result when source is a NaN
;
	.globl		src_qnan
src_qnan:
	fmove.x		SRC.w(a0),fp0
	tst.b		SRC_EX.w(a0)
	bmi.b		src_qnan_m
src_qnan_p:
	move.b		#nan_bmask,FPSR_CC(a6)
	rts
src_qnan_m:
	move.b		#nan_bmask+neg_bmask,FPSR_CC(a6)
	rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
;	Native instruction support
;
;	Some systems may need entry points even for 68060 native
;	instructions.  These routines are provided for
;	convenience.
;
	.globl		_fadds_
_fadds_:
	fmove.l		fpcr,-(sp)		; save fpcr
	fmove.l		#0x00000000,fpcr	; clear fpcr for load
	fmove.s		0x8(sp),fp0		; load sgl dst
	fmove.l		(sp)+,fpcr		; restore fpcr
	fadd.s		0x8(sp),fp0		; fadd w/ sgl src
	rts

	.globl		_faddd_
_faddd_:
	fmove.l		fpcr,-(sp)		; save fpcr
	fmove.l		#0x00000000,fpcr	; clear fpcr for load
	fmove.d		0x8(sp),fp0		; load dbl dst
	fmove.l		(sp)+,fpcr		; restore fpcr
	fadd.d		0xc(sp),fp0		; fadd w/ dbl src
	rts

	.globl		_faddx_
_faddx_:
	fmovem.x	0x4(sp),fp0		; load ext dst
	fadd.x		0x10(sp),fp0		; fadd w/ ext src
	rts

	.globl		_fsubs_
_fsubs_:
	fmove.l		fpcr,-(sp)		; save fpcr
	fmove.l		#0x00000000,fpcr	; clear fpcr for load
	fmove.s		0x8(sp),fp0		; load sgl dst
	fmove.l		(sp)+,fpcr		; restore fpcr
	fsub.s		0x8(sp),fp0		; fsub w/ sgl src
	rts

	.globl		_fsubd_
_fsubd_:
	fmove.l		fpcr,-(sp)		; save fpcr
	fmove.l		#0x00000000,fpcr	; clear fpcr for load
	fmove.d		0x8(sp),fp0		; load dbl dst
	fmove.l		(sp)+,fpcr		; restore fpcr
	fsub.d		0xc(sp),fp0		; fsub w/ dbl src
	rts

	.globl		_fsubx_
_fsubx_:
	fmovem.x	0x4(sp),fp0		; load ext dst
	fsub.x		0x10(sp),fp0		; fsub w/ ext src
	rts

	.globl		_fmuls_
_fmuls_:
	fmove.l		fpcr,-(sp)		; save fpcr
	fmove.l		#0x00000000,fpcr	; clear fpcr for load
	fmove.s		0x8(sp),fp0		; load sgl dst
	fmove.l		(sp)+,fpcr		; restore fpcr
	fmul.s		0x8(sp),fp0		; fmul w/ sgl src
	rts

	.globl		_fmuld_
_fmuld_:
	fmove.l		fpcr,-(sp)		; save fpcr
	fmove.l		#0x00000000,fpcr	; clear fpcr for load
	fmove.d		0x8(sp),fp0		; load dbl dst
	fmove.l		(sp)+,fpcr		; restore fpcr
	fmul.d		0xc(sp),fp0		; fmul w/ dbl src
	rts

	.globl		_fmulx_
_fmulx_:
	fmovem.x	0x4(sp),fp0		; load ext dst
	fmul.x		0x10(sp),fp0		; fmul w/ ext src
	rts

	.globl		_fdivs_
_fdivs_:
	fmove.l		fpcr,-(sp)		; save fpcr
	fmove.l		#0x00000000,fpcr	; clear fpcr for load
	fmove.s		0x8(sp),fp0		; load sgl dst
	fmove.l		(sp)+,fpcr		; restore fpcr
	fdiv.s		0x8(sp),fp0		; fdiv w/ sgl src
	rts

	.globl		_fdivd_
_fdivd_:
	fmove.l		fpcr,-(sp)		; save fpcr
	fmove.l		#0x00000000,fpcr	; clear fpcr for load
	fmove.d		0x8(sp),fp0		; load dbl dst
	fmove.l		(sp)+,fpcr		; restore fpcr
	fdiv.d		0xc(sp),fp0		; fdiv w/ dbl src
	rts

	.globl		_fdivx_
_fdivx_:
	fmovem.x	0x4(sp),fp0		; load ext dst
	fdiv.x		0x10(sp),fp0		; fdiv w/ ext src
	rts

	.globl		_fabss_
_fabss_:
	fabs.s		0x4(sp),fp0		; fabs w/ sgl src
	rts

	.globl		_fabsd_
_fabsd_:
	fabs.d		0x4(sp),fp0		; fabs w/ dbl src
	rts

	.globl		_fabsx_
_fabsx_:
	fabs.x		0x4(sp),fp0		; fabs w/ ext src
	rts

	.globl		_fnegs_
_fnegs_:
	fneg.s		0x4(sp),fp0		; fneg w/ sgl src
	rts

	.globl		_fnegd_
_fnegd_:
	fneg.d		0x4(sp),fp0		; fneg w/ dbl src
	rts

	.globl		_fnegx_
_fnegx_:
	fneg.x		0x4(sp),fp0		; fneg w/ ext src
	rts

	.globl		_fsqrts_
_fsqrts_:
	fsqrt.s		0x4(sp),fp0		; fsqrt w/ sgl src
	rts

	.globl		_fsqrtd_
_fsqrtd_:
	fsqrt.d		0x4(sp),fp0		; fsqrt w/ dbl src
	rts

	.globl		_fsqrtx_
_fsqrtx_:
	fsqrt.x		0x4(sp),fp0		; fsqrt w/ ext src
	rts

	.globl		_fints_
_fints_:
	fint.s		0x4(sp),fp0		; fint w/ sgl src
	rts

	.globl		_fintd_
_fintd_:
	fint.d		0x4(sp),fp0		; fint w/ dbl src
	rts

	.globl		_fintx_
_fintx_:
	fint.x		0x4(sp),fp0		; fint w/ ext src
	rts

	.globl		_fintrzs_
_fintrzs_:
	fintrz.s	0x4(sp),fp0		; fintrz w/ sgl src
	rts

	.globl		_fintrzd_
_fintrzd_:
	fintrz.d	0x4(sp),fp0		; fintrx w/ dbl src
	rts

	.globl		_fintrzx_
_fintrzx_:
	fintrz.x	0x4(sp),fp0		; fintrz w/ ext src
	rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; src_zero(): Return signed zero according to sign of src operand.	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		src_zero
src_zero:
	tst.b		SRC_EX.w(a0)		; get sign of src operand
	bmi.b		ld_mzero		; if neg, load neg zero

;
; ld_pzero(): return a positive zero.
;
	.globl		ld_pzero
ld_pzero:
	fmove.s		#0x00000000,fp0	; load +0
	move.b		#z_bmask,FPSR_CC(a6)	; set 'Z' ccode bit
	rts

; ld_mzero(): return a negative zero.
	.globl		ld_mzero
ld_mzero:
	fmove.s		#0x80000000,fp0	; load -0
	move.b		#neg_bmask+z_bmask,FPSR_CC(a6) ; set 'N','Z' ccode bits
	rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; dst_zero(): Return signed zero according to sign of dst operand.	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		dst_zero
dst_zero:
	tst.b		DST_EX.w(a1)		; get sign of dst operand
	bmi.b		ld_mzero		; if neg, load neg zero
	bra.b		ld_pzero		; load positive zero

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; src_inf(): Return signed inf according to sign of src operand.	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		src_inf
src_inf:
	tst.b		SRC_EX.w(a0)		; get sign of src operand
	bmi.b		ld_minf			; if negative branch

;
; ld_pinf(): return a positive infinity.
;
	.globl		ld_pinf
ld_pinf:
	fmove.s		#0x7f800000,fp0	; load +INF
	move.b		#inf_bmask,FPSR_CC(a6)	; set 'INF' ccode bit
	rts

;
; ld_minf():return a negative infinity.
;
	.globl		ld_minf
ld_minf:
	fmove.s		#0xff800000,fp0	; load -INF
	move.b		#neg_bmask+inf_bmask,FPSR_CC(a6) ; set 'N','I' ccode bits
	rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; dst_inf(): Return signed inf according to sign of dst operand.	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		dst_inf
dst_inf:
	tst.b		DST_EX.w(a1)		; get sign of dst operand
	bmi.b		ld_minf			; if negative branch
	bra.b		ld_pinf

	.globl		szr_inf
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; szr_inf(): Return +ZERO for a negative src operand or		
;	            +INF for a positive src operand.		
;	     Routine used for fetox, ftwotox, and ftentox.	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
szr_inf:
	tst.b		SRC_EX.w(a0)		; check sign of source
	bmi.b		ld_pzero
	bra.b		ld_pinf

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; sopr_inf(): Return +INF for a positive src operand or			
;	      jump to operand error routine for a negative src operand.	
;	      Routine used for flogn, flognp1, flog10, and flog2.	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		sopr_inf
sopr_inf:
	tst.b		SRC_EX.w(a0)		; check sign of source
	bmi.w		t_operr
	bra.b		ld_pinf

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; setoxm1i(): Return minus one for a negative src operand or	
;	      positive infinity for a positive src operand.	
;	      Routine used for fetoxm1.				
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		setoxm1i
setoxm1i:
	tst.b		SRC_EX.w(a0)		; check sign of source
	bmi.b		ld_mone
	bra.b		ld_pinf

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; src_one(): Return signed one according to sign of src operand.	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		src_one
src_one:
	tst.b		SRC_EX.w(a0)		; check sign of source
	bmi.b		ld_mone

;
; ld_pone(): return positive one.
;
	.globl		ld_pone
ld_pone:
	fmove.s		#0x3f800000,fp0	; load +1
	clr.b		FPSR_CC(a6)
	rts

;
; ld_mone(): return negative one.
;
	.globl		ld_mone
ld_mone:
	fmove.s		#0xbf800000,fp0	; load -1
	move.b		#neg_bmask,FPSR_CC(a6)	; set 'N' ccode bit
	rts

ppiby2:	.dc.l		0x3fff0000, 0xc90fdaa2, 0x2168c235
mpiby2:	.dc.l		0xbfff0000, 0xc90fdaa2, 0x2168c235

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; spi_2(): Return signed PI/2 according to sign of src operand.	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		spi_2
spi_2:
	tst.b		SRC_EX.w(a0)		; check sign of source
	bmi.b		ld_mpi2

;
; ld_ppi2(): return positive PI/2.
;
	.globl		ld_ppi2
ld_ppi2:
	fmove.l		d0,fpcr
	fmove.x		ppiby2(pc),fp0	; load +pi/2
	bra.w		t_pinx2			; set INEX2

;
; ld_mpi2(): return negative PI/2.
;
	.globl		ld_mpi2
ld_mpi2:
	fmove.l		d0,fpcr
	fmove.x		mpiby2(pc),fp0	; load -pi/2
	bra.w		t_minx2			; set INEX2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; The following routines give support for fsincos. 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; ssincosz(): When the src operand is ZERO, store a one in the
;	      cosine register and return a ZERO in fp0 w/ the same sign
;	      as the src operand.
;
	.globl		ssincosz
ssincosz:
	fmove.s		#0x3f800000,fp1
	tst.b		SRC_EX.w(a0)		; test sign
	bpl.b		sincoszp
	fmove.s		#0x80000000,fp0	; return sin result in fp0
	move.b		#z_bmask+neg_bmask,FPSR_CC(a6)
	rts
sincoszp:
	fmove.s		#0x00000000,fp0	; return sin result in fp0
	move.b		#z_bmask,FPSR_CC(a6)
	rts

;
; ssincosi(): When the src operand is INF, store a QNAN in the cosine
;	      register and jump to the operand error routine for negative
;	      src operands.
;
	.globl		ssincosi
ssincosi:
	fmove.x		qnan(pc),fp1		; load NAN
	bra.w		t_operr

;
; ssincosqnan(): When the src operand is a QNAN, store the QNAN in the cosine
;		 register and branch to the src QNAN routine.
;
	.globl		ssincosqnan
ssincosqnan:
	fmove.x		LOCAL_EX.w(a0),fp1
	bra.w		src_qnan

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.globl		smod_sdnrm
	.globl		smod_snorm
smod_sdnrm:
smod_snorm:
	move.b		DTAG(a6),d1
	beq.l		smod
	cmpi.b		#ZERO,d1
	beq.w		smod_zro
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		smod
	bra.l		dst_qnan

	.globl		smod_szero
smod_szero:
	move.b		DTAG(a6),d1
	beq.l		t_operr
	cmpi.b		#ZERO,d1
	beq.l		t_operr
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		t_operr
	bra.l		dst_qnan

	.globl		smod_sinf
smod_sinf:
	move.b		DTAG(a6),d1
	beq.l		smod_fpn
	cmpi.b		#ZERO,d1
	beq.l		smod_zro
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		smod_fpn
	bra.l		dst_qnan

smod_zro:
srem_zro:
	move.b		SRC_EX.w(a0),d1		; get src sign
	move.b		DST_EX.w(a1),d0		; get dst sign
	eor.b		d0,d1			; get qbyte sign
	andi.b		#0x80,d1
	move.b		d1,FPSR_QBYTE(a6)
	tst.b		d0
	bpl.w		ld_pzero
	bra.w		ld_mzero

smod_fpn:
srem_fpn:
	clr.b		FPSR_QBYTE(a6)
	move.l		d0,-(sp)
	move.b		SRC_EX.w(a0),d1		; get src sign
	move.b		DST_EX.w(a1),d0		; get dst sign
	eor.b		d0,d1			; get qbyte sign
	andi.b		#0x80,d1
	move.b		d1,FPSR_QBYTE(a6)
	cmpi.b		#DENORM,DTAG(a6)
	bne.b		smod_nrm
	lea			DST.w(a1),a0
	move.l		(sp)+,d0
	bra.l		t_resdnrm
smod_nrm:
	fmove.l		(sp)+,fpcr
	fmove.x		DST.w(a1),fp0
	tst.b		DST_EX.w(a1)
	bmi.b		smod_nrm_neg
	rts

smod_nrm_neg:
	move.b		#neg_bmask,FPSR_CC(a6)	; set 'N' code
	rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.globl		srem_snorm
	.globl		srem_sdnrm
srem_sdnrm:
srem_snorm:
	move.b		DTAG(a6),d1
	beq.l		srem
	cmpi.b		#ZERO,d1
	beq.w		srem_zro
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		srem
	bra.l		dst_qnan

	.globl		srem_szero
srem_szero:
	move.b		DTAG(a6),d1
	beq.l		t_operr
	cmpi.b		#ZERO,d1
	beq.l		t_operr
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		t_operr
	bra.l		dst_qnan

	.globl		srem_sinf
srem_sinf:
	move.b		DTAG(a6),d1
	beq.w		srem_fpn
	cmpi.b		#ZERO,d1
	beq.w		srem_zro
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		srem_fpn
	bra.l		dst_qnan

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.globl		sscale_snorm
	.globl		sscale_sdnrm
sscale_snorm:
sscale_sdnrm:
	move.b		DTAG(a6),d1
	beq.l		sscale
	cmpi.b		#ZERO,d1
	beq.l		dst_zero
	cmpi.b		#INF,d1
	beq.l		dst_inf
	cmpi.b		#DENORM,d1
	beq.l		sscale
	bra.l		dst_qnan

	.globl		sscale_szero
sscale_szero:
	move.b		DTAG(a6),d1
	beq.l		sscale
	cmpi.b		#ZERO,d1
	beq.l		dst_zero
	cmpi.b		#INF,d1
	beq.l		dst_inf
	cmpi.b		#DENORM,d1
	beq.l		sscale
	bra.l		dst_qnan

	.globl		sscale_sinf
sscale_sinf:
	move.b		DTAG(a6),d1
	beq.l		t_operr
	cmpi.b		#QNAN,d1
	beq.l		dst_qnan
	bra.l		t_operr

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.globl		sop_sqnan
sop_sqnan:
	move.b		DTAG(a6),d1
	cmpi.b		#QNAN,d1
	beq.l		dst_qnan
	bra.l		src_qnan
