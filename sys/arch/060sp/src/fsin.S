/*
 *  ssin():     computes the sine of a normalized input			
 *  ssind():    computes the sine of a denormalized input			
 *  scos():     computes the cosine of a normalized input			
 *  scosd():    computes the cosine of a denormalized input		
 *  ssincos():  computes the sine and cosine of a normalized input	
 *  ssincosd(): computes the sine and cosine of a denormalized input	
 * 									
 *  INPUT *************************************************************** 
 * 	a0 = pointer to extended precision input			
 * 	d0 = round precision,mode					
 * 									
 *  OUTPUT ************************************************************** 
 * 	fp0 = sin(X) or cos(X)						
 * 									
 *     For ssincos(X):							
 * 	fp0 = sin(X)							
 * 	fp1 = cos(X)							
 * 									
 *  ACCURACY and MONOTONICITY ******************************************* 
 * 	The returned result is within 1 ulp in 64 significant bit, i.e.	
 * 	within 0.5001 ulp to 53 bits if the result is subsequently	
 * 	rounded to double precision. The result is provably monotonic	
 * 	in double precision.						
 * 									
 *  ALGORITHM ***********************************************************	
 * 									
 * 	SIN and COS:							
 * 	1. If SIN is invoked, set AdjN := 0 *  otherwise, set AdjN := 1.	
 * 									
 * 	2. If |X| >= 15Pi or |X| < 2**(-40), go to 7.			
 * 									
 * 	3. Decompose X as X = N(Pi/2) + r where |r| <= Pi/4. Let	
 * 		k = N mod 4, so in particular, k = 0,1,2,or 3.		
 * 		Overwrite k by k := k + AdjN.				
 * 									
 * 	4. If k is even, go to 6.					
 * 									
 * 	5. (k is odd) Set j := (k-1)/2, sgn := (-1)**j.			
 * 		Return sgn*cos(r) where cos(r) is approximated by an	
 * 		even polynomial in r, 1 + r*r*(B1+s*(B2+ ... + s*B8)),	
 * 		s = r*r.						
 * 		Exit.							
 * 									
 * 	6. (k is even) Set j := k/2, sgn := (-1)**j. Return sgn*sin(r)	
 * 		where sin(r) is approximated by an odd polynomial in r	
 * 		r + r*s*(A1+s*(A2+ ... + s*A7)),	s = r*r.	
 * 		Exit.							
 * 									
 * 	7. If |X| > 1, go to 9.						
 * 									
 * 	8. (|X|<2**(-40)) If SIN is invoked, return X * 			
 * 		otherwise return 1.					
 * 									
 * 	9. Overwrite X by X := X rem 2Pi. Now that |X| <= Pi,		
 * 		go back to 3.						
 * 									
 * 	SINCOS:								
 * 	1. If |X| >= 15Pi or |X| < 2**(-40), go to 6.			
 * 									
 * 	2. Decompose X as X = N(Pi/2) + r where |r| <= Pi/4. Let	
 * 		k = N mod 4, so in particular, k = 0,1,2,or 3.		
 * 									
 * 	3. If k is even, go to 5.					
 * 									
 * 	4. (k is odd) Set j1 := (k-1)/2, j2 := j1 (EOR) (k mod 2), ie.	
 * 		j1 exclusive or with the l.s.b. of k.			
 * 		sgn1 := (-1)**j1, sgn2 := (-1)**j2.			
 * 		SIN(X) = sgn1 * cos(r) and COS(X) = sgn2*sin(r) where	
 * 		sin(r) and cos(r) are computed as odd and even		
 * 		polynomials in r, respectively. Exit			
 * 									
 * 	5. (k is even) Set j1 := k/2, sgn1 := (-1)**j1.			
 * 		SIN(X) = sgn1 * sin(r) and COS(X) = sgn1*cos(r) where	
 * 		sin(r) and cos(r) are computed as odd and even		
 * 		polynomials in r, respectively. Exit			
 * 									
 * 	6. If |X| > 1, go to 8.						
 * 									
 * 	7. (|X|<2**(-40)) SIN(X) = X and COS(X) = 1. Exit.		
 * 									
 * 	8. Overwrite X by X := X rem 2Pi. Now that |X| <= Pi,		
 * 		go back to 2.						
 * 									
 */

	.include "hdr.fpu"

	.xref sto_cos
	.xref t_extdnrm
	.xref t_inx2
	.xref t_catch
	.xref t_pinx2
	.xref TWOBYPI
	.xref PITBL

	.text

SINA7:	.dc.l		0xBD6AAA77,0xCCC994F5
SINA6:	.dc.l		0x3DE61209,0x7AAE8DA1
SINA5:	.dc.l		0xBE5AE645,0x2A118AE4
SINA4:	.dc.l		0x3EC71DE3,0xA5341531
SINA3:	.dc.l		0xBF2A01A0,0x1A018B59,0x00000000,0x00000000
SINA2:	.dc.l		0x3FF80000,0x88888888,0x888859AF,0x00000000
SINA1:	.dc.l		0xBFFC0000,0xAAAAAAAA,0xAAAAAA99,0x00000000

COSB8:	.dc.l		0x3D2AC4D0,0xD6011EE3
COSB7:	.dc.l		0xBDA9396F,0x9F45AC19
COSB6:	.dc.l		0x3E21EED9,0x0612C972
COSB5:	.dc.l		0xBE927E4F,0xB79D9FCF
COSB4:	.dc.l		0x3EFA01A0,0x1A01D423,0x00000000,0x00000000
COSB3:	.dc.l		0xBFF50000,0xB60B60B6,0x0B61D438,0x00000000
COSB2:	.dc.l		0x3FFA0000,0xAAAAAAAA,0xAAAAAB5E
COSB1:	.dc.l		0xBF000000

	INARG = FP_SCR0

	X = FP_SCR0
/* 	XDCARE = X+2 */
	XFRAC = X+4

	RPRIME = FP_SCR0
	SPRIME = FP_SCR1

	POSNEG1 = L_SCR1
	TWOTO63 = L_SCR1

	ENDFLAG = L_SCR2
	INT = L_SCR2

	ADJN = L_SCR3

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		ssin
ssin:
	move.l		#0,ADJN(a6)		/*  yes; SET ADJN TO 0 */
	bra.b		SINBGN

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		scos
scos:
	move.l		#1,ADJN(a6)		/*  yes; SET ADJN TO 1 */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
SINBGN:
/* --SAVE FPCR, FP1. CHECK IF |X| IS TOO SMALL OR LARGE */

	fmove.x		(a0),fp0		/*  LOAD INPUT */
	fmove.x		fp0,X(a6)		/*  save input at X */

/*  "COMPACTIFY" X */
	move.l		(a0),d1		/*  put exp in hi word */
	move.w		4(a0),d1		/*  fetch hi(man) */
	andi.l		#0x7FFFFFFF,d1		/*  strip sign */

	cmpi.l		#0x3FD78000,d1		/*  is |X| >= 2**(-40)? */
	bge.b		SOK1			/*  no */
	bra.w		SINSM			/*  yes; input is very small */

SOK1:
	cmpi.l		#0x4004BC7E,d1		/*  is |X| < 15 PI? */
	blt.b		SINMAIN			/*  no */
	bra.w		SREDUCEX		/*  yes; input is very large */

/* --THIS IS THE USUAL CASE, |X| <= 15 PI. */
/* --THE ARGUMENT REDUCTION IS DONE BY TABLE LOOK UP. */
SINMAIN:
	fmove.x		fp0,fp1
	fmul.d		TWOBYPI(pc),fp1	/*  X*2/PI */

	lea		(PITBL+0x200).l(pc),a1	/*  TABLE OF N*PI/2, N = -32,...,32 */

	fmove.l		fp1,INT(a6)		/*  CONVERT TO INTEGER */

	move.l		INT(a6),d1		/*  make a copy of N */
	asl.l		#4,d1			/*  N *= 16 */
	add.l		d1,a1			/*  tbl_addr = a1 + (N*16) */

/*  A1 IS THE ADDRESS OF N*PIBY2 */
/*  ...WHICH IS IN TWO PIECES Y1 # Y2 */
	fsub.x		(a1)+,fp0		/*  X-Y1 */
	fsub.s		(a1),fp0		/*  fp0 = R = (X-Y1)-Y2 */

SINCONT:
/* --continuation from REDUCEX */

/* --GET N+ADJN AND SEE IF SIN(R) OR COS(R) IS NEEDED */
	move.l		INT(a6),d1
	add.l		ADJN(a6),d1		/*  SEE IF D0 IS ODD OR EVEN */
	ror.l		#1,d1			/*  D0 WAS ODD IFF D0 IS NEGATIVE */
	cmpi.l		#0,d1
	blt.w		COSPOLY

/* --LET J BE THE LEAST SIG. BIT OF D0, LET SGN := (-1)**J. */
/* --THEN WE RETURN	SGN*SIN(R). SGN*SIN(R) IS COMPUTED BY */
/* --R' + R'*S*(A1 + S(A2 + S(A3 + S(A4 + ... + SA7)))), WHERE */
/* --R' = SGN*R, S=R*R. THIS CAN BE REWRITTEN AS */
/* --R' + R'*S*( [A1+T(A3+T(A5+TA7))] + [S(A2+T(A4+TA6))]) */
/* --WHERE T=S*S. */
/* --NOTE THAT A3 THROUGH A7 ARE STORED IN DOUBLE PRECISION */
/* --WHILE A1 AND A2 ARE IN DOUBLE-EXTENDED FORMAT. */
SINPOLY:
	fmovem.x	fp2-fp3,-(sp)		/*  save fp2/fp3 */

	fmove.x		fp0,X(a6)		/*  X IS R */
	fmul.x		fp0,fp0		/*  FP0 IS S */

	fmove.d		SINA7(pc),fp3
	fmove.d		SINA6(pc),fp2

	fmove.x		fp0,fp1
	fmul.x		fp1,fp1		/*  FP1 IS T */

	ror.l		#1,d1
	andi.l		#0x80000000,d1
/*  ...LEAST SIG. BIT OF D0 IN SIGN POSITION */
	eor.l		d1,X(a6)		/*  X IS NOW R'= SGN*R */

	fmul.x		fp1,fp3		/*  TA7 */
	fmul.x		fp1,fp2		/*  TA6 */

	fadd.d		SINA5(pc),fp3		/*  A5+TA7 */
	fadd.d		SINA4(pc),fp2		/*  A4+TA6 */

	fmul.x		fp1,fp3		/*  T(A5+TA7) */
	fmul.x		fp1,fp2		/*  T(A4+TA6) */

	fadd.d		SINA3(pc),fp3		/*  A3+T(A5+TA7) */
	fadd.x		SINA2(pc),fp2		/*  A2+T(A4+TA6) */

	fmul.x		fp3,fp1		/*  T(A3+T(A5+TA7)) */

	fmul.x		fp0,fp2		/*  S(A2+T(A4+TA6)) */
	fadd.x		SINA1(pc),fp1		/*  A1+T(A3+T(A5+TA7)) */
	fmul.x		X(a6),fp0		/*  R'*S */

	fadd.x		fp2,fp1		/*  [A1+T(A3+T(A5+TA7))]+[S(A2+T(A4+TA6))] */

	fmul.x		fp1,fp0		/*  SIN(R')-R' */

	fmovem.x		(sp)+,fp2-fp3		/*  restore fp2/fp3 */

	fmove.l		d0,fpcr		/*  restore users round mode,prec */
	fadd.x		X(a6),fp0		/*  last inst - possible exception set */
	bra.l		t_inx2

/* --LET J BE THE LEAST SIG. BIT OF D0, LET SGN := (-1)**J. */
/* --THEN WE RETURN	SGN*COS(R). SGN*COS(R) IS COMPUTED BY */
/* --SGN + S'*(B1 + S(B2 + S(B3 + S(B4 + ... + SB8)))), WHERE */
/* --S=R*R AND S'=SGN*S. THIS CAN BE REWRITTEN AS */
/* --SGN + S'*([B1+T(B3+T(B5+TB7))] + [S(B2+T(B4+T(B6+TB8)))]) */
/* --WHERE T=S*S. */
/* --NOTE THAT B4 THROUGH B8 ARE STORED IN DOUBLE PRECISION */
/* --WHILE B2 AND B3 ARE IN DOUBLE-EXTENDED FORMAT, B1 IS -1/2 */
/* --AND IS THEREFORE STORED AS SINGLE PRECISION. */
COSPOLY:
	fmovem.x		fp2-fp3,-(sp)		/*  save fp2/fp3 */

	fmul.x		fp0,fp0		/*  FP0 IS S */

	fmove.d		COSB8(pc),fp2
	fmove.d		COSB7(pc),fp3

	fmove.x		fp0,fp1
	fmul.x		fp1,fp1		/*  FP1 IS T */

	fmove.x		fp0,X(a6)		/*  X IS S */
	ror.l		#1,d1
	andi.l		#0x80000000,d1
/*  ...LEAST SIG. BIT OF D0 IN SIGN POSITION */

	fmul.x		fp1,fp2		/*  TB8 */

	eor.l		d1,X(a6)		/*  X IS NOW S'= SGN*S */
	andi.l		#0x80000000,d1

	fmul.x		fp1,fp3		/*  TB7 */

	ori.l		#0x3F800000,d1		/*  D0 IS SGN IN SINGLE */
	move.l		d1,POSNEG1(a6)

	fadd.d		COSB6(pc),fp2		/*  B6+TB8 */
	fadd.d		COSB5(pc),fp3		/*  B5+TB7 */

	fmul.x		fp1,fp2		/*  T(B6+TB8) */
	fmul.x		fp1,fp3		/*  T(B5+TB7) */

	fadd.d		COSB4(pc),fp2		/*  B4+T(B6+TB8) */
	fadd.x		COSB3(pc),fp3		/*  B3+T(B5+TB7) */

	fmul.x		fp1,fp2		/*  T(B4+T(B6+TB8)) */
	fmul.x		fp3,fp1		/*  T(B3+T(B5+TB7)) */

	fadd.x		COSB2(pc),fp2		/*  B2+T(B4+T(B6+TB8)) */
	fadd.s		COSB1(pc),fp1		/*  B1+T(B3+T(B5+TB7)) */

	fmul.x		fp2,fp0		/*  S(B2+T(B4+T(B6+TB8))) */

	fadd.x		fp1,fp0

	fmul.x		X(a6),fp0

	fmovem.x		(sp)+,fp2-fp3		/*  restore fp2/fp3 */

	fmove.l		d0,fpcr		/*  restore users round mode,prec */
	fadd.s		POSNEG1(a6),fp0	/*  last inst - possible exception set */
	bra.l		t_inx2

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*  SINe: Big OR Small? */
/* --IF |X| > 15PI, WE USE THE GENERAL ARGUMENT REDUCTION. */
/* --IF |X| < 2**(-40), RETURN X OR 1. */
SINBORS:
	cmpi.l		#0x3FFF8000,d1
	bgt.l		SREDUCEX

SINSM:
	move.l		ADJN(a6),d1
	cmpi.l		#0,d1
	bgt.b		COSTINY

/*  here, the operation may underflow iff the precision is sgl or dbl. */
/*  extended denorms are handled through another entry point. */
SINTINY:
/* 	move.w		#0x0000,XDCARE(a6)	; JUST IN CASE */

	fmove.l		d0,fpcr		/*  restore users round mode,prec */
	move.b		#FMOV_OP,d1		/*  last inst is MOVE */
	fmove.x		X(a6),fp0		/*  last inst - possible exception set */
	bra.l		t_catch

COSTINY:
	fmove.s		#0x3F800000,fp0	/*  fp0 = 1.0 */
	fmove.l		d0,fpcr		/*  restore users round mode,prec */
	fadd.s		#0x80800000,fp0	/*  last inst - possible exception set */
	bra.l		t_pinx2

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		ssind
/* --SIN(X) = X FOR DENORMALIZED X */
ssind:
	bra.l		t_extdnrm

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		scosd
/* --COS(X) = 1 FOR DENORMALIZED X */
scosd:
	fmove.s		#0x3F800000,fp0	/*  fp0 = 1.0 */
	bra.l		t_pinx2

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		ssincos
ssincos:
/* --SET ADJN TO 4 */
	move.l		#4,ADJN(a6)

	fmove.x		(a0),fp0		/*  LOAD INPUT */
	fmove.x		fp0,X(a6)

	move.l		(a0),d1
	move.w		4(a0),d1
	andi.l		#0x7FFFFFFF,d1		/*  COMPACTIFY X */

	cmpi.l		#0x3FD78000,d1		/*  |X| >= 2**(-40)? */
	bge.b		SCOK1
	bra.w		SCSM

SCOK1:
	cmpi.l		#0x4004BC7E,d1		/*  |X| < 15 PI? */
	blt.b		SCMAIN
	bra.w		SREDUCEX


/* --THIS IS THE USUAL CASE, |X| <= 15 PI. */
/* --THE ARGUMENT REDUCTION IS DONE BY TABLE LOOK UP. */
SCMAIN:
	fmove.x		fp0,fp1

	fmul.d		TWOBYPI(pc),fp1	/*  X*2/PI */

	lea		(PITBL+0x200).l(pc),a1	/*  TABLE OF N*PI/2, N = -32,...,32 */

	fmove.l		fp1,INT(a6)		/*  CONVERT TO INTEGER */

	move.l		INT(a6),d1
	asl.l		#4,d1
	add.l		d1,a1			/*  ADDRESS OF N*PIBY2, IN Y1, Y2 */

	fsub.x		(a1)+,fp0		/*  X-Y1 */
	fsub.s		(a1),fp0		/*  FP0 IS R = (X-Y1)-Y2 */

SCCONT:
/* --continuation point from REDUCEX */

	move.l		INT(a6),d1
	ror.l		#1,d1
	cmpi.l		#0,d1			/*  D0 < 0 IFF N IS ODD */
	bge.w		NEVEN

SNODD:
/* --REGISTERS SAVED SO FAR: D0, A0, FP2. */
	fmovem.x		fp2,-(sp)		/*  save fp2 */

	fmove.x		fp0,RPRIME(a6)
	fmul.x		fp0,fp0		/*  FP0 IS S = R*R */
	fmove.d		SINA7(pc),fp1		/*  A7 */
	fmove.d		COSB8(pc),fp2		/*  B8 */
	fmul.x		fp0,fp1		/*  SA7 */
	fmul.x		fp0,fp2		/*  SB8 */

	move.l		d2,-(sp)
	move.l		d1,d2
	ror.l		#1,d2
	andi.l		#0x80000000,d2
	eor.l		d1,d2
	andi.l		#0x80000000,d2

	fadd.d		SINA6(pc),fp1		/*  A6+SA7 */
	fadd.d		COSB7(pc),fp2		/*  B7+SB8 */

	fmul.x		fp0,fp1		/*  S(A6+SA7) */
	eor.l		d2,RPRIME(a6)
	move.l		(sp)+,d2
	fmul.x		fp0,fp2		/*  S(B7+SB8) */
	ror.l		#1,d1
	andi.l		#0x80000000,d1
	move.l		#0x3F800000,POSNEG1(a6)
	eor.l		d1,POSNEG1(a6)

	fadd.d		SINA5(pc),fp1		/*  A5+S(A6+SA7) */
	fadd.d		COSB6(pc),fp2		/*  B6+S(B7+SB8) */

	fmul.x		fp0,fp1		/*  S(A5+S(A6+SA7)) */
	fmul.x		fp0,fp2		/*  S(B6+S(B7+SB8)) */
	fmove.x		fp0,SPRIME(a6)

	fadd.d		SINA4(pc),fp1		/*  A4+S(A5+S(A6+SA7)) */
	eor.l		d1,SPRIME(a6)
	fadd.d		COSB5(pc),fp2		/*  B5+S(B6+S(B7+SB8)) */

	fmul.x		fp0,fp1		/*  S(A4+...) */
	fmul.x		fp0,fp2		/*  S(B5+...) */

	fadd.d		SINA3(pc),fp1		/*  A3+S(A4+...) */
	fadd.d		COSB4(pc),fp2		/*  B4+S(B5+...) */

	fmul.x		fp0,fp1		/*  S(A3+...) */
	fmul.x		fp0,fp2		/*  S(B4+...) */

	fadd.x		SINA2(pc),fp1		/*  A2+S(A3+...) */
	fadd.x		COSB3(pc),fp2		/*  B3+S(B4+...) */

	fmul.x		fp0,fp1		/*  S(A2+...) */
	fmul.x		fp0,fp2		/*  S(B3+...) */

	fadd.x		SINA1(pc),fp1		/*  A1+S(A2+...) */
	fadd.x		COSB2(pc),fp2		/*  B2+S(B3+...) */

	fmul.x		fp0,fp1		/*  S(A1+...) */
	fmul.x		fp2,fp0		/*  S(B2+...) */

	fmul.x		RPRIME(a6),fp1	/*  R'S(A1+...) */
	fadd.s		COSB1(pc),fp0		/*  B1+S(B2...) */
	fmul.x		SPRIME(a6),fp0	/*  S'(B1+S(B2+...)) */

	fmovem.x		(sp)+,fp2		/*  restore fp2 */

	fmove.l		d0,fpcr
	fadd.x		RPRIME(a6),fp1	/*  COS(X) */
	bsr.l		sto_cos			/*  store cosine result */
	fadd.s		POSNEG1(a6),fp0	/*  SIN(X) */
	bra.l		t_inx2

NEVEN:
/* --REGISTERS SAVED SO FAR: FP2. */
	fmovem.x		fp2,-(sp)		/*  save fp2 */

	fmove.x		fp0,RPRIME(a6)
	fmul.x		fp0,fp0		/*  FP0 IS S = R*R */

	fmove.d		COSB8(pc),fp1		/*  B8 */
	fmove.d		SINA7(pc),fp2		/*  A7 */

	fmul.x		fp0,fp1		/*  SB8 */
	fmove.x		fp0,SPRIME(a6)
	fmul.x		fp0,fp2		/*  SA7 */

	ror.l		#1,d1
	andi.l		#0x80000000,d1

	fadd.d		COSB7(pc),fp1		/*  B7+SB8 */
	fadd.d		SINA6(pc),fp2		/*  A6+SA7 */

	eor.l		d1,RPRIME(a6)
	eor.l		d1,SPRIME(a6)

	fmul.x		fp0,fp1		/*  S(B7+SB8) */

	ori.l		#0x3F800000,d1
	move.l		d1,POSNEG1(a6)

	fmul.x		fp0,fp2		/*  S(A6+SA7) */

	fadd.d		COSB6(pc),fp1		/*  B6+S(B7+SB8) */
	fadd.d		SINA5(pc),fp2		/*  A5+S(A6+SA7) */

	fmul.x		fp0,fp1		/*  S(B6+S(B7+SB8)) */
	fmul.x		fp0,fp2		/*  S(A5+S(A6+SA7)) */

	fadd.d		COSB5(pc),fp1		/*  B5+S(B6+S(B7+SB8)) */
	fadd.d		SINA4(pc),fp2		/*  A4+S(A5+S(A6+SA7)) */

	fmul.x		fp0,fp1		/*  S(B5+...) */
	fmul.x		fp0,fp2		/*  S(A4+...) */

	fadd.d		COSB4(pc),fp1		/*  B4+S(B5+...) */
	fadd.d		SINA3(pc),fp2		/*  A3+S(A4+...) */

	fmul.x		fp0,fp1		/*  S(B4+...) */
	fmul.x		fp0,fp2		/*  S(A3+...) */

	fadd.x		COSB3(pc),fp1		/*  B3+S(B4+...) */
	fadd.x		SINA2(pc),fp2		/*  A2+S(A3+...) */

	fmul.x		fp0,fp1		/*  S(B3+...) */
	fmul.x		fp0,fp2		/*  S(A2+...) */

	fadd.x		COSB2(pc),fp1		/*  B2+S(B3+...) */
	fadd.x		SINA1(pc),fp2		/*  A1+S(A2+...) */

	fmul.x		fp0,fp1		/*  S(B2+...) */
	fmul.x		fp2,fp0		/*  s(a1+...) */


	fadd.s		COSB1(pc),fp1		/*  B1+S(B2...) */
	fmul.x		RPRIME(a6),fp0	/*  R'S(A1+...) */
	fmul.x		SPRIME(a6),fp1	/*  S'(B1+S(B2+...)) */

	fmovem.x		(sp)+,fp2		/*  restore fp2 */

	fmove.l		d0,fpcr
	fadd.s		POSNEG1(a6),fp1	/*  COS(X) */
	bsr.l		sto_cos			/*  store cosine result */
	fadd.x		RPRIME(a6),fp0	/*  SIN(X) */
	bra.l		t_inx2

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

SCBORS:
	cmpi.l		#0x3FFF8000,d1
	bgt.w		SREDUCEX

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

SCSM:
/* 	move.w		#0x0000,XDCARE(a6) */
	fmove.s		#0x3F800000,fp1

	fmove.l		d0,fpcr
	fsub.s		#0x00800000,fp1
	bsr.l		sto_cos			/*  store cosine result */
	fmove.l		fpcr,d0		/*  d0 must have fpcr,too */
	move.b		#FMOV_OP,d1		/*  last inst is MOVE */
	fmove.x		X(a6),fp0
	bra.l		t_catch

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		ssincosd
/* --SIN AND COS OF X FOR DENORMALIZED X */
ssincosd:
	move.l		d0,-(sp)		/*  save d0 */
	fmove.s		#0x3F800000,fp1
	bsr.l		sto_cos			/*  store cosine result */
	move.l		(sp)+,d0		/*  restore d0 */
	bra.l		t_extdnrm

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/* --WHEN REDUCEX IS USED, THE CODE WILL INEVITABLY BE SLOW. */
/* --THIS REDUCTION METHOD, HOWEVER, IS MUCH FASTER THAN USING */
/* --THE REMAINDER INSTRUCTION WHICH IS NOW IN SOFTWARE. */
SREDUCEX:
	fmovem.x		fp2-fp5,-(sp)		/*  save {fp2-fp5} */
	move.l		d2,-(sp)		/*  save d2 */
	fmove.s		#0x00000000,fp1	/*  fp1 = 0 */

/* --If compact form of abs(arg) in d0=$7ffeffff, argument is so large that */
/* --there is a danger of unwanted overflow in first LOOP iteration.  In this */
/* --case, reduce argument by one remainder step to make subsequent reduction */
/* --safe. */
	cmpi.l		#0x7ffeffff,d1		/*  is arg dangerously large? */
	bne.b		SLOOP			/*  no */

/*  yes; create 2**16383*PI/2 */
	move.w		#0x7ffe,FP_SCR0_EX(a6)
	move.l		#0xc90fdaa2,FP_SCR0_HI(a6)
	clr.l		FP_SCR0_LO(a6)

/*  create low half of 2**16383*PI/2 at FP_SCR1 */
	move.w		#0x7fdc,FP_SCR1_EX(a6)
	move.l		#0x85a308d3,FP_SCR1_HI(a6)
	clr.l		FP_SCR1_LO(a6)

	ftst.x		fp0			/*  test sign of argument */
	fblt		sred_neg

	or.b		#0x80,FP_SCR0_EX(a6)	/*  positive arg */
	or.b		#0x80,FP_SCR1_EX(a6)
sred_neg:
	fadd.x		FP_SCR0(a6),fp0	/*  high part of reduction is exact */
	fmove.x		fp0,fp1		/*  save high result in fp1 */
	fadd.x		FP_SCR1(a6),fp0	/*  low part of reduction */
	fsub.x		fp0,fp1		/*  determine low component of result */
	fadd.x		FP_SCR1(a6),fp1	/*  fp0/fp1 are reduced argument. */

/* --ON ENTRY, FP0 IS X, ON RETURN, FP0 IS X REM PI/2, |X| <= PI/4. */
/* --integer quotient will be stored in N */
/* --Intermeditate remainder is 66-bit long; (R,r) in (FP0,FP1) */
SLOOP:
	fmove.x		fp0,INARG(a6)		/*  +-2**K * F, 1 <= F < 2 */
	move.w		INARG(a6),d1
	move.l		d1,a1			/*  save a copy of D0 */
	andi.l		#0x00007FFF,d1
	subi.l		#0x00003FFF,d1		/*  d0 = K */
	cmpi.l		#28,d1
	ble.b		SLASTLOOP
SCONTLOOP:
	subi.l		#27,d1			/*  d0 = L := K-27 */
	move.b		#0,ENDFLAG(a6)
	bra.b		SWORK
SLASTLOOP:
	clr.l		d1			/*  d0 = L := 0 */
	move.b		#1,ENDFLAG(a6)

SWORK:
/* --FIND THE REMAINDER OF (R,r) W.R.T.	2**L * (PI/2). L IS SO CHOSEN */
/* --THAT	INT( X * (2/PI) / 2**(L) ) < 2**29. */

/* --CREATE 2**(-L) * (2/PI), SIGN(INARG)*2**(63), */
/* --2**L * (PIby2_1), 2**L * (PIby2_2) */

	move.l		#0x00003FFE,d2		/*  BIASED EXP OF 2/PI */
	sub.l		d1,d2			/*  BIASED EXP OF 2**(-L)*(2/PI) */

	move.l		#0xA2F9836E,FP_SCR0_HI(a6)
	move.l		#0x4E44152A,FP_SCR0_LO(a6)
	move.w		d2,FP_SCR0_EX(a6)	/*  FP_SCR0 = 2**(-L)*(2/PI) */

	fmove.x		fp0,fp2
	fmul.x		FP_SCR0(a6),fp2	/*  fp2 = X * 2**(-L)*(2/PI) */

/* --WE MUST NOW FIND INT(FP2). SINCE WE NEED THIS VALUE IN */
/* --FLOATING POINT FORMAT, THE TWO FMOVE'S	FMOVE.L FP <--> N */
/* --WILL BE TOO INEFFICIENT. THE WAY AROUND IT IS THAT */
/* --(SIGN(INARG)*2**63	+	FP2) - SIGN(INARG)*2**63 WILL GIVE */
/* --US THE DESIRED VALUE IN FLOATING POINT. */
	move.l		a1,d2
	swap		d2
	andi.l		#0x80000000,d2
	ori.l		#0x5F000000,d2		/*  d2 = SIGN(INARG)*2**63 IN SGL */
	move.l		d2,TWOTO63(a6)
	fadd.s		TWOTO63(a6),fp2	/*  THE FRACTIONAL PART OF FP1 IS ROUNDED */
	fsub.s		TWOTO63(a6),fp2	/*  fp2 = N */
/* 	fint.x		fp2 */

/* --CREATING 2**(L)*Piby2_1 and 2**(L)*Piby2_2 */
	move.l		d1,d2			/*  d2 = L */

	addi.l		#0x00003FFF,d2		/*  BIASED EXP OF 2**L * (PI/2) */
	move.w		d2,FP_SCR0_EX(a6)
	move.l		#0xC90FDAA2,FP_SCR0_HI(a6)
	clr.l		FP_SCR0_LO(a6)		/*  FP_SCR0 = 2**(L) * Piby2_1 */

	addi.l		#0x00003FDD,d1
	move.w		d1,FP_SCR1_EX(a6)
	move.l		#0x85A308D3,FP_SCR1_HI(a6)
	clr.l		FP_SCR1_LO(a6)		/*  FP_SCR1 = 2**(L) * Piby2_2 */

	move.b		ENDFLAG(a6),d1

/* --We are now ready to perform (R+r) - N*P1 - N*P2, P1 = 2**(L) * Piby2_1 and */
/* --P2 = 2**(L) * Piby2_2 */
	fmove.x		fp2,fp4		/*  fp4 = N */
	fmul.x		FP_SCR0(a6),fp4	/*  fp4 = W = N*P1 */
	fmove.x		fp2,fp5		/*  fp5 = N */
	fmul.x		FP_SCR1(a6),fp5	/*  fp5 = w = N*P2 */
	fmove.x		fp4,fp3		/*  fp3 = W = N*P1 */

/* --we want P+p = W+w  but  |p| <= half ulp of P */
/* --Then, we need to compute  A := R-P   and  a := r-p */
	fadd.x		fp5,fp3		/*  fp3 = P */
	fsub.x		fp3,fp4		/*  fp4 = W-P */

	fsub.x		fp3,fp0		/*  fp0 = A := R - P */
	fadd.x		fp5,fp4		/*  fp4 = p = (W-P)+w */

	fmove.x		fp0,fp3		/*  fp3 = A */
	fsub.x		fp4,fp1		/*  fp1 = a := r - p */

/* --Now we need to normalize (A,a) to  "new (R,r)" where R+r = A+a but */
/* --|r| <= half ulp of R. */
	fadd.x		fp1,fp0		/*  fp0 = R := A+a */
/* --No need to calculate r if this is the last loop */
	cmpi.b		#0,d1
	bgt.w		SRESTORE

/* --Need to calculate r */
	fsub.x		fp0,fp3		/*  fp3 = A-R */
	fadd.x		fp3,fp1		/*  fp1 = r := (A-R)+a */
	bra.w		SLOOP

SRESTORE:
	fmove.l		fp2,INT(a6)
	move.l		(sp)+,d2		/*  restore d2 */
	fmovem.x		(sp)+,fp2-fp5		/*  restore {fp2-fp5} */

	move.l		ADJN(a6),d1
	cmpi.l		#4,d1

	blt.w		SINCONT
	bra.w		SCCONT

