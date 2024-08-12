/*
 * 	srem_mod.sa 3.1 12/10/90
 *
 *       The entry point sMOD computes the floating point MOD of the
 *       input values X and Y. The entry point sREM computes the floating
 *       point (IEEE) REM of the input values X and Y.
 *
 *       INPUT
 *       -----
 *       Double-extended value Y is pointed to by address in register
 *       A0. Double-extended value X is located in -12(A0). The values
 *       of X and Y are both nonzero and finite; although either or both
 *       of them can be denormalized. The special cases of zeros, NaNs,
 *       and infinities are handled elsewhere.
 *
 *       OUTPUT
 *       ------
 *       FREM(X,Y) or FMOD(X,Y), depending on entry point.
 *
 *        ALGORITHM
 *        ---------
 *
 *        Step 1.  Save and strip signs of X and Y: signX := sign(X),
 *                 signY := sign(Y), X := |X|, Y := |Y|,
 *                 signQ := signX EOR signY. Record whether MOD or REM
 *                 is requested.
 *
 *        Step 2.  Set L := expo(X)-expo(Y), k := 0, Q := 0.
 *                 If (L < 0) then
 *                    R := X, go to Step 4.
 *                 else
 *                    R := 2^(-L)X, j := L.
 *                 endif
 *
 *        Step 3.  Perform MOD(X,Y)
 *             3.1 If R = Y, go to Step 9.
 *             3.2 If R > Y, then { R := R - Y, Q := Q + 1}
 *             3.3 If j = 0, go to Step 4.
 *             3.4 k := k + 1, j := j - 1, Q := 2Q, R := 2R. Go to
 *                 Step 3.1.
 *
 *        Step 4.  At this point, R = X - QY = MOD(X,Y). Set
 *                 Last_Subtract := false (used in Step 7 below). If
 *                 MOD is requested, go to Step 6.
 *
 *        Step 5.  R = MOD(X,Y), but REM(X,Y) is requested.
 *             5.1 If R < Y/2, then R = MOD(X,Y) = REM(X,Y). Go to
 *                 Step 6.
 *             5.2 If R > Y/2, then { set Last_Subtract := true,
 *                 Q := Q + 1, Y := signY*Y }. Go to Step 6.
 *             5.3 This is the tricky case of R = Y/2. If Q is odd,
 *                 then { Q := Q + 1, signX := -signX }.
 *
 *        Step 6.  R := signX*R.
 *
 *        Step 7.  If Last_Subtract = true, R := R - Y.
 *
 *        Step 8.  Return signQ, last 7 bits of Q, and R as required.
 *
 *        Step 9.  At this point, R = 2^(-j)*X - Q Y = Y. Thus,
 *                 X = 2^(j)*(Q+1)Y. set Q := 2^(j)*(Q+1),
 *                 R := 0. Return signQ, last 7 bits of Q, and R.
 *
 *
 *
 * 		Copyright (C) Motorola, Inc. 1990
 * 			All Rights Reserved
 *
 *        For details on the license for this file, please see the
 *        file, README, in this same directory.
 */

#include "mint/asmdefs.h"
	.include "fpsp.h"

	Mod_Flag = L_SCR3
	SignY = FP_SCR3+4
	SignX = FP_SCR3+8
	SignQ = FP_SCR3+12
	Sc_Flag = FP_SCR4

	Y = FP_SCR1
	Y_Hi = Y+4
	Y_Lo = Y+8

	R = FP_SCR2
	R_Hi = R+4
	R_Lo = R+8


	.text

Scale:     .dc.l	0x00010000,0x80000000,0x00000000,0x00000000

	.xref	t_avoid_unsupp

	.globl	smod
smod:

   move.l	       #0,Mod_Flag(a6)
   bra.s		Mod_Rem

	.globl	srem
srem:

   move.l	       #1,Mod_Flag(a6)

Mod_Rem:
/*
 * ..Save sign of X and Y
 */
   movem.l	      d2-d7,-(a7)     /*  ...save data registers */
   move.w	       (a0),d3
   move.w	       d3,SignY(a6)
   andi.l	       #0x00007FFF,d3   /*  ...Y := |Y| */

   move.l	       4(a0),d4
   move.l	       8(a0),d5	/*  ...(D3,D4,D5) is |Y| */

   tst.l		d3
   bne.s		Y_Normal

   move.l	       #0x00003FFE,d3	/*  ...$3FFD + 1 */
   tst.l		d4
   bne.s		HiY_not0

HiY_0:
   move.l	       d5,d4
   clr.l		d5
   subi.l	       #32,d3
   clr.l		d6
   bfffo		d4{0:32},d6
   lsl.l		d6,d4
   sub.l		d6,d3	   /*  ...(D3,D4,D5) is normalized with bias $7FFD */
   bra.s		Chk_X

HiY_not0:
   clr.l		d6
   bfffo		d4{0:32},d6
   sub.l		d6,d3
   lsl.l		d6,d4
   move.l	       d5,d7	   /*  ...a copy of D5 */
   lsl.l		d6,d5
   neg.l		d6
   addi.l	       #32,d6
   lsr.l		d6,d7
   or.l		 d7,d4	   /*  ...(D3,D4,D5) normalized with bias $7FFD */
   bra.s		Chk_X

Y_Normal:
   addi.l	       #0x00003FFE,d3   /*  ...(D3,D4,D5) normalized with bias $7FFD */

Chk_X:
   move.w	       -12(a0),d0
   move.w	       d0,SignX(a6)
   move.w	       SignY(a6),d1
   eor.l		d0,d1
   andi.l	       #0x00008000,d1
   move.w	       d1,SignQ(a6)	/*  ...sign(Q) obtained */
   andi.l	       #0x00007FFF,d0
   move.l	       -8(a0),d1
   move.l	       -4(a0),d2       /*  ...(D0,D1,D2) is |X| */
   tst.l		d0
   bne.s		X_Normal
   move.l	       #0x00003FFE,d0
   tst.l		d1
   bne.s		HiX_not0_srem

HiX_0_srem:
   move.l	       d2,d1
   clr.l		d2
   subi.l	       #32,d0
   clr.l		d6
   bfffo		d1{0:32},d6
   lsl.l		d6,d1
   sub.l		d6,d0	   /*  ...(D0,D1,D2) is normalized with bias $7FFD */
   bra.s		Init

HiX_not0_srem:
   clr.l		d6
   bfffo		d1{0:32},d6
   sub.l		d6,d0
   lsl.l		d6,d1
   move.l	       d2,d7	   /*  ...a copy of D2 */
   lsl.l		d6,d2
   neg.l		d6
   addi.l	       #32,d6
   lsr.l		d6,d7
   or.l		 d7,d1	   /*  ...(D0,D1,D2) normalized with bias $7FFD */
   bra.s		Init

X_Normal:
   addi.l	       #0x00003FFE,d0   /*  ...(D0,D1,D2) normalized with bias $7FFD */

Init:
   move.l	       d3,L_SCR1(a6)   /*  ...save biased expo(Y) */
   move.l		d0,L_SCR2(a6)	/* save d0 */
   sub.l		d3,d0	   /*  ...L := expo(X)-expo(Y) */
/*   move.l	       D0,L	    ...D0 is j */
   clr.l		d6	      /*  ...D6 := carry <- 0 */
   clr.l		d3	      /*  ...D3 is Q */
   suba.l	      a1,a1	   /*  ...A1 is k; j+k=L, Q=0 */

/*
 * ..(Carry,D1,D2) is R
 */
   tst.l		d0
   bge.s		Mod_Loop

/*
 * ..expo(X) < expo(Y). Thus X = mod(X,Y)
 */
   move.l		L_SCR2(a6),d0	/* restore d0 */
   bra.s		Get_Mod

/*
 * ..At this point  R = 2^(-L)X; Q = 0; k = 0; and  k+j = L
 */

Mod_Loop:
   tst.l		d6	      /*  ...test carry bit */
   bgt.s		R_GT_Y

/*
 * ..At this point carry = 0, R = (D1,D2), Y = (D4,D5)
 */
   cmp.l		d4,d1	   /*  ...compare hi(R) and hi(Y) */
   bne.s		R_NE_Y
   cmp.l		d5,d2	   /*  ...compare lo(R) and lo(Y) */
   bne.s		R_NE_Y

/*
 * ..At this point, R = Y
 */
   bra		Rem_is_0

R_NE_Y:
/*
 * ..use the borrow of the previous compare
 */
   bcs.s		R_LT_Y	  /*  ...borrow is set iff R < Y */

R_GT_Y:
/*
 * ..If Carry is set, then Y < (Carry,D1,D2) < 2Y. Otherwise, Carry = 0
 * ..and Y < (D1,D2) < 2Y. Either way, perform R - Y
 */
   sub.l		d5,d2	   /*  ...lo(R) - lo(Y) */
   subx.l	       d4,d1	   /*  ...hi(R) - hi(Y) */
   clr.l		d6	      /*  ...clear carry */
   addq.l	       #1,d3	   /*  ...Q := Q + 1 */

R_LT_Y:
/*
 * ..At this point, Carry=0, R < Y. R = 2^(k-L)X - QY; k+j = L; j >= 0.
 */
   tst.l		d0	      /*  ...see if j = 0. */
   beq.s		PostLoop

   add.l		d3,d3	   /*  ...Q := 2Q */
   add.l		d2,d2	   /*  ...lo(R) = 2lo(R) */
   roxl.l	      #1,d1	   /*  ...hi(R) = 2hi(R) + carry */
   scs		  d6	      /*  ...set Carry if 2(R) overflows */
   addq.l	       #1,a1	   /*  ...k := k+1 */
   subq.l	       #1,d0	   /*  ...j := j - 1 */
/*
 * ..At this point, R=(Carry,D1,D2) = 2^(k-L)X - QY, j+k=L, j >= 0, R < 2Y.
 */
   bra.s		Mod_Loop

PostLoop:
/*
 * ..k = L, j = 0, Carry = 0, R = (D1,D2) = X - QY, R < Y.
 *
 * ..normalize R.
 */
   move.l	       L_SCR1(a6),d0	   /*  ...new biased expo of R */
   tst.l		d1
   bne.s		HiR_not0

HiR_0:
   move.l	       d2,d1
   clr.l		d2
   subi.l	       #32,d0
   clr.l		d6
   bfffo		d1{0:32},d6
   lsl.l		d6,d1
   sub.l		d6,d0	   /*  ...(D0,D1,D2) is normalized with bias $7FFD */
   bra.s		Get_Mod

HiR_not0:
   clr.l		d6
   bfffo		d1{0:32},d6
   bmi.s		Get_Mod	 /*  ...already normalized */
   sub.l	       d6,d0
   lsl.l		d6,d1
   move.l	       d2,d7	   /*  ...a copy of D2 */
   lsl.l		d6,d2
   neg.l		d6
   addi.l	       #32,d6
   lsr.l		d6,d7
   or.l		 d7,d1	   /*  ...(D0,D1,D2) normalized */


Get_Mod:
   cmpi.l		#0x000041FE,d0
   bge.s		No_Scale
Do_Scale:
   move.w		d0,R(a6)
   clr.w		R+2(a6)
   move.l		d1,R_Hi(a6)
   move.l		d2,R_Lo(a6)
   move.l		L_SCR1(a6),d6
   move.w		d6,Y(a6)
   clr.w		Y+2(a6)
   move.l		d4,Y_Hi(a6)
   move.l		d5,Y_Lo(a6)
   fmove.x		R(a6),fp0		/*  ...no exception */
   move.l		#1,Sc_Flag(a6)
   bra.s		ModOrRem
No_Scale:
   move.l		d1,R_Hi(a6)
   move.l		d2,R_Lo(a6)
   subi.l		#0x3FFE,d0
   move.w		d0,R(a6)
   clr.w		R+2(a6)
   move.l		L_SCR1(a6),d6
   subi.l		#0x3FFE,d6
   move.l		d6,L_SCR1(a6)
   fmove.x		R(a6),fp0
   move.w		d6,Y(a6)
   move.l		d4,Y_Hi(a6)
   move.l		d5,Y_Lo(a6)
   move.l		#0,Sc_Flag(a6)



ModOrRem:
   move.l	       Mod_Flag(a6),d6
   beq.s		Fix_Sign

   move.l	       L_SCR1(a6),d6	   /*  ...new biased expo(Y) */
   subq.l	       #1,d6	   /*  ...biased expo(Y/2) */
   cmp.l		d6,d0
   blt.s		Fix_Sign
   bgt.s		Last_Sub

   cmp.l		d4,d1
   bne.s		Not_EQ
   cmp.l		d5,d2
   bne.s		Not_EQ
   bra.s		Tie_Case

Not_EQ:
   bcs.s		Fix_Sign

Last_Sub:
   fsub.x		Y(a6),fp0		/*  ...no exceptions */
   addq.l	       #1,d3	   /*  ...Q := Q + 1 */

Fix_Sign:
/*
 * ..Get sign of X
 */
   move.w	       SignX(a6),d6
   bge.s		Get_Q
   fneg.x		fp0

/*
 * ..Get Q
 */
Get_Q:
   clr.l		d6
   move.w	       SignQ(a6),d6	/*  ...D6 is sign(Q) */
   move.l	       #8,d7
   lsr.l		d7,d6
   andi.l	       #0x0000007F,d3   /*  ...7 bits of Q */
   or.l		 d6,d3	   /*  ...sign and bits of Q */
   swap		 d3
   fmove.l	      fpsr,d6
   andi.l	       #0xFF00FFFF,d6
   or.l		 d3,d6
   fmove.l	      d6,fpsr	 /*  ...put Q in fpsr */

Restore:
   movem.l	      (a7)+,d2-d7
   fmove.l	      USER_FPCR(a6),fpcr
   move.l	       Sc_Flag(a6),d0
   beq.s		Finish
   fmul.x	Scale(pc),fp0	/*  ...may cause underflow */
   bra			t_avoid_unsupp	/* check for denorm as a result of the scaling */

Finish:
	fmove.x		fp0,fp0		/* capture exceptions & round */
	rts

Rem_is_0:
/*
 * ..R = 2^(-j)X - Q Y = Y, thus R = 0 and quotient = 2^j (Q+1)
 */
   addq.l	       #1,d3
   cmpi.l	      #8,d0	   /*  ...D0 is j */
   bge.s		Q_Big

   lsl.l	       d0,d3
   bra.s	       Set_R_0

Q_Big:
   clr.l		d3

Set_R_0:
   fmove.s		#0x00000000,fp0
   move.l		#0,Sc_Flag(a6)
   bra.s	      Fix_Sign

Tie_Case:
/*
 * ..Check parity of Q
 */
   move.l	       d3,d6
   andi.l	       #0x00000001,d6
   tst.l		d6
   beq.s	      Fix_Sign	/*  ...Q is even */

/*
 * ..Q is odd, Q := Q + 1, signX := -signX
 */
   addq.l	       #1,d3
   move.w	       SignX(a6),d6
   eori.l	       #0x00008000,d6
   move.w	       d6,SignX(a6)
   bra		Fix_Sign
