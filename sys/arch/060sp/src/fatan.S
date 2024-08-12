/*
 *  satan():  computes the arctangent of a normalized number		
 *  satand(): computes the arctangent of a denormalized number		
 * 									
 *  INPUT	*************************************************************** 
 * 	a0 = pointer to extended precision input			
 * 	d0 = round precision,mode					
 * 									
 *  OUTPUT ************************************************************** 
 * 	fp0 = arctan(X)							
 * 									
 *  ACCURACY and MONOTONICITY ******************************************* 
 * 	The returned result is within 2 ulps in	64 significant bit,	
 * 	i.e. within 0.5001 ulp to 53 bits if the result is subsequently	
 * 	rounded to double precision. The result is provably monotonic	
 * 	in double precision.						
 * 									
 *  ALGORITHM *********************************************************** 
 * 	Step 1. If |X| >= 16 or |X| < 1/16, go to Step 5.		
 * 									
 * 	Step 2. Let X = sgn * 2**k * 1.xxxxxxxx...x.			
 * 		Note that k = -4, -3,..., or 3.				
 * 		Define F = sgn * 2**k * 1.xxxx1, i.e. the first 5	
 * 		significant bits of X with a bit-1 attached at the 6-th	
 * 		bit position. Define u to be u = (X-F) / (1 + X*F).	
 * 									
 * 	Step 3. Approximate arctan(u) by a polynomial poly.		
 * 									
 * 	Step 4. Return arctan(F) + poly, arctan(F) is fetched from a	
 * 		table of values calculated beforehand. Exit.		
 * 									
 * 	Step 5. If |X| >= 16, go to Step 7.				
 * 									
 * 	Step 6. Approximate arctan(X) by an odd polynomial in X. Exit.	
 * 									
 * 	Step 7. Define X' = -1/X. Approximate arctan(X') by an odd	
 * 		polynomial in X'.					
 * 		Arctan(X) = sign(X)*Pi/2 + arctan(X'). Exit.		
 * 									
 */

	.include "hdr.fpu"

	.xref t_extdnrm
	.xref t_inx2
	.xref t_catch
	.xref t_pinx2
	.xref t_minx2
	.xref TWOBYPI

	.text

ATANA3:	.dc.l		0xBFF6687E,0x314987D8
ATANA2:	.dc.l		0x4002AC69,0x34A26DB3
ATANA1:	.dc.l		0xBFC2476F,0x4E1DA28E

ATANB6:	.dc.l		0x3FB34444,0x7F876989
ATANB5:	.dc.l		0xBFB744EE,0x7FAF45DB
ATANB4:	.dc.l		0x3FBC71C6,0x46940220
ATANB3:	.dc.l		0xBFC24924,0x921872F9
ATANB2:	.dc.l		0x3FC99999,0x99998FA9
ATANB1:	.dc.l		0xBFD55555,0x55555555

ATANC5:	.dc.l		0xBFB70BF3,0x98539E6A
ATANC4:	.dc.l		0x3FBC7187,0x962D1D7D
ATANC3:	.dc.l		0xBFC24924,0x827107B8
ATANC2:	.dc.l		0x3FC99999,0x9996263E
ATANC1:	.dc.l		0xBFD55555,0x55555536

PPIBY2:	.dc.l		0x3FFF0000,0xC90FDAA2,0x2168C235,0x00000000
NPIBY2:	.dc.l		0xBFFF0000,0xC90FDAA2,0x2168C235,0x00000000

PTINY:	.dc.l		0x00010000,0x80000000,0x00000000,0x00000000
NTINY:	.dc.l		0x80010000,0x80000000,0x00000000,0x00000000

ATANTBL:
	.dc.l		0x3FFB0000,0x83D152C5,0x060B7A51,0x00000000
	.dc.l		0x3FFB0000,0x8BC85445,0x65498B8B,0x00000000
	.dc.l		0x3FFB0000,0x93BE4060,0x17626B0D,0x00000000
	.dc.l		0x3FFB0000,0x9BB3078D,0x35AEC202,0x00000000
	.dc.l		0x3FFB0000,0xA3A69A52,0x5DDCE7DE,0x00000000
	.dc.l		0x3FFB0000,0xAB98E943,0x62765619,0x00000000
	.dc.l		0x3FFB0000,0xB389E502,0xF9C59862,0x00000000
	.dc.l		0x3FFB0000,0xBB797E43,0x6B09E6FB,0x00000000
	.dc.l		0x3FFB0000,0xC367A5C7,0x39E5F446,0x00000000
	.dc.l		0x3FFB0000,0xCB544C61,0xCFF7D5C6,0x00000000
	.dc.l		0x3FFB0000,0xD33F62F8,0x2488533E,0x00000000
	.dc.l		0x3FFB0000,0xDB28DA81,0x62404C77,0x00000000
	.dc.l		0x3FFB0000,0xE310A407,0x8AD34F18,0x00000000
	.dc.l		0x3FFB0000,0xEAF6B0A8,0x188EE1EB,0x00000000
	.dc.l		0x3FFB0000,0xF2DAF194,0x9DBE79D5,0x00000000
	.dc.l		0x3FFB0000,0xFABD5813,0x61D47E3E,0x00000000
	.dc.l		0x3FFC0000,0x8346AC21,0x0959ECC4,0x00000000
	.dc.l		0x3FFC0000,0x8B232A08,0x304282D8,0x00000000
	.dc.l		0x3FFC0000,0x92FB70B8,0xD29AE2F9,0x00000000
	.dc.l		0x3FFC0000,0x9ACF476F,0x5CCD1CB4,0x00000000
	.dc.l		0x3FFC0000,0xA29E7630,0x4954F23F,0x00000000
	.dc.l		0x3FFC0000,0xAA68C5D0,0x8AB85230,0x00000000
	.dc.l		0x3FFC0000,0xB22DFFFD,0x9D539F83,0x00000000
	.dc.l		0x3FFC0000,0xB9EDEF45,0x3E900EA5,0x00000000
	.dc.l		0x3FFC0000,0xC1A85F1C,0xC75E3EA5,0x00000000
	.dc.l		0x3FFC0000,0xC95D1BE8,0x28138DE6,0x00000000
	.dc.l		0x3FFC0000,0xD10BF300,0x840D2DE4,0x00000000
	.dc.l		0x3FFC0000,0xD8B4B2BA,0x6BC05E7A,0x00000000
	.dc.l		0x3FFC0000,0xE0572A6B,0xB42335F6,0x00000000
	.dc.l		0x3FFC0000,0xE7F32A70,0xEA9CAA8F,0x00000000
	.dc.l		0x3FFC0000,0xEF888432,0x64ECEFAA,0x00000000
	.dc.l		0x3FFC0000,0xF7170A28,0xECC06666,0x00000000
	.dc.l		0x3FFD0000,0x812FD288,0x332DAD32,0x00000000
	.dc.l		0x3FFD0000,0x88A8D1B1,0x218E4D64,0x00000000
	.dc.l		0x3FFD0000,0x9012AB3F,0x23E4AEE8,0x00000000
	.dc.l		0x3FFD0000,0x976CC3D4,0x11E7F1B9,0x00000000
	.dc.l		0x3FFD0000,0x9EB68949,0x3889A227,0x00000000
	.dc.l		0x3FFD0000,0xA5EF72C3,0x4487361B,0x00000000
	.dc.l		0x3FFD0000,0xAD1700BA,0xF07A7227,0x00000000
	.dc.l		0x3FFD0000,0xB42CBCFA,0xFD37EFB7,0x00000000
	.dc.l		0x3FFD0000,0xBB303A94,0x0BA80F89,0x00000000
	.dc.l		0x3FFD0000,0xC22115C6,0xFCAEBBAF,0x00000000
	.dc.l		0x3FFD0000,0xC8FEF3E6,0x86331221,0x00000000
	.dc.l		0x3FFD0000,0xCFC98330,0xB4000C70,0x00000000
	.dc.l		0x3FFD0000,0xD6807AA1,0x102C5BF9,0x00000000
	.dc.l		0x3FFD0000,0xDD2399BC,0x31252AA3,0x00000000
	.dc.l		0x3FFD0000,0xE3B2A855,0x6B8FC517,0x00000000
	.dc.l		0x3FFD0000,0xEA2D764F,0x64315989,0x00000000
	.dc.l		0x3FFD0000,0xF3BF5BF8,0xBAD1A21D,0x00000000
	.dc.l		0x3FFE0000,0x801CE39E,0x0D205C9A,0x00000000
	.dc.l		0x3FFE0000,0x8630A2DA,0xDA1ED066,0x00000000
	.dc.l		0x3FFE0000,0x8C1AD445,0xF3E09B8C,0x00000000
	.dc.l		0x3FFE0000,0x91DB8F16,0x64F350E2,0x00000000
	.dc.l		0x3FFE0000,0x97731420,0x365E538C,0x00000000
	.dc.l		0x3FFE0000,0x9CE1C8E6,0xA0B8CDBA,0x00000000
	.dc.l		0x3FFE0000,0xA22832DB,0xCADAAE09,0x00000000
	.dc.l		0x3FFE0000,0xA746F2DD,0xB7602294,0x00000000
	.dc.l		0x3FFE0000,0xAC3EC0FB,0x997DD6A2,0x00000000
	.dc.l		0x3FFE0000,0xB110688A,0xEBDC6F6A,0x00000000
	.dc.l		0x3FFE0000,0xB5BCC490,0x59ECC4B0,0x00000000
	.dc.l		0x3FFE0000,0xBA44BC7D,0xD470782F,0x00000000
	.dc.l		0x3FFE0000,0xBEA94144,0xFD049AAC,0x00000000
	.dc.l		0x3FFE0000,0xC2EB4ABB,0x661628B6,0x00000000
	.dc.l		0x3FFE0000,0xC70BD54C,0xE602EE14,0x00000000
	.dc.l		0x3FFE0000,0xCD000549,0xADEC7159,0x00000000
	.dc.l		0x3FFE0000,0xD48457D2,0xD8EA4EA3,0x00000000
	.dc.l		0x3FFE0000,0xDB948DA7,0x12DECE3B,0x00000000
	.dc.l		0x3FFE0000,0xE23855F9,0x69E8096A,0x00000000
	.dc.l		0x3FFE0000,0xE8771129,0xC4353259,0x00000000
	.dc.l		0x3FFE0000,0xEE57C16E,0x0D379C0D,0x00000000
	.dc.l		0x3FFE0000,0xF3E10211,0xA87C3779,0x00000000
	.dc.l		0x3FFE0000,0xF919039D,0x758B8D41,0x00000000
	.dc.l		0x3FFE0000,0xFE058B8F,0x64935FB3,0x00000000
	.dc.l		0x3FFF0000,0x8155FB49,0x7B685D04,0x00000000
	.dc.l		0x3FFF0000,0x83889E35,0x49D108E1,0x00000000
	.dc.l		0x3FFF0000,0x859CFA76,0x511D724B,0x00000000
	.dc.l		0x3FFF0000,0x87952ECF,0xFF8131E7,0x00000000
	.dc.l		0x3FFF0000,0x89732FD1,0x9557641B,0x00000000
	.dc.l		0x3FFF0000,0x8B38CAD1,0x01932A35,0x00000000
	.dc.l		0x3FFF0000,0x8CE7A8D8,0x301EE6B5,0x00000000
	.dc.l		0x3FFF0000,0x8F46A39E,0x2EAE5281,0x00000000
	.dc.l		0x3FFF0000,0x922DA7D7,0x91888487,0x00000000
	.dc.l		0x3FFF0000,0x94D19FCB,0xDEDF5241,0x00000000
	.dc.l		0x3FFF0000,0x973AB944,0x19D2A08B,0x00000000
	.dc.l		0x3FFF0000,0x996FF00E,0x08E10B96,0x00000000
	.dc.l		0x3FFF0000,0x9B773F95,0x12321DA7,0x00000000
	.dc.l		0x3FFF0000,0x9D55CC32,0x0F935624,0x00000000
	.dc.l		0x3FFF0000,0x9F100575,0x006CC571,0x00000000
	.dc.l		0x3FFF0000,0xA0A9C290,0xD97CC06C,0x00000000
	.dc.l		0x3FFF0000,0xA22659EB,0xEBC0630A,0x00000000
	.dc.l		0x3FFF0000,0xA388B4AF,0xF6EF0EC9,0x00000000
	.dc.l		0x3FFF0000,0xA4D35F10,0x61D292C4,0x00000000
	.dc.l		0x3FFF0000,0xA60895DC,0xFBE3187E,0x00000000
	.dc.l		0x3FFF0000,0xA72A51DC,0x7367BEAC,0x00000000
	.dc.l		0x3FFF0000,0xA83A5153,0x0956168F,0x00000000
	.dc.l		0x3FFF0000,0xA93A2007,0x7539546E,0x00000000
	.dc.l		0x3FFF0000,0xAA9E7245,0x023B2605,0x00000000
	.dc.l		0x3FFF0000,0xAC4C84BA,0x6FE4D58F,0x00000000
	.dc.l		0x3FFF0000,0xADCE4A4A,0x606B9712,0x00000000
	.dc.l		0x3FFF0000,0xAF2A2DCD,0x8D263C9C,0x00000000
	.dc.l		0x3FFF0000,0xB0656F81,0xF22265C7,0x00000000
	.dc.l		0x3FFF0000,0xB1846515,0x0F71496A,0x00000000
	.dc.l		0x3FFF0000,0xB28AAA15,0x6F9ADA35,0x00000000
	.dc.l		0x3FFF0000,0xB37B44FF,0x3766B895,0x00000000
	.dc.l		0x3FFF0000,0xB458C3DC,0xE9630433,0x00000000
	.dc.l		0x3FFF0000,0xB525529D,0x562246BD,0x00000000
	.dc.l		0x3FFF0000,0xB5E2CCA9,0x5F9D88CC,0x00000000
	.dc.l		0x3FFF0000,0xB692CADA,0x7ACA1ADA,0x00000000
	.dc.l		0x3FFF0000,0xB736AEA7,0xA6925838,0x00000000
	.dc.l		0x3FFF0000,0xB7CFAB28,0x7E9F7B36,0x00000000
	.dc.l		0x3FFF0000,0xB85ECC66,0xCB219835,0x00000000
	.dc.l		0x3FFF0000,0xB8E4FD5A,0x20A593DA,0x00000000
	.dc.l		0x3FFF0000,0xB99F41F6,0x4AFF9BB5,0x00000000
	.dc.l		0x3FFF0000,0xBA7F1E17,0x842BBE7B,0x00000000
	.dc.l		0x3FFF0000,0xBB471285,0x7637E17D,0x00000000
	.dc.l		0x3FFF0000,0xBBFABE8A,0x4788DF6F,0x00000000
	.dc.l		0x3FFF0000,0xBC9D0FAD,0x2B689D79,0x00000000
	.dc.l		0x3FFF0000,0xBD306A39,0x471ECD86,0x00000000
	.dc.l		0x3FFF0000,0xBDB6C731,0x856AF18A,0x00000000
	.dc.l		0x3FFF0000,0xBE31CAC5,0x02E80D70,0x00000000
	.dc.l		0x3FFF0000,0xBEA2D55C,0xE33194E2,0x00000000
	.dc.l		0x3FFF0000,0xBF0B10B7,0xC03128F0,0x00000000
	.dc.l		0x3FFF0000,0xBF6B7A18,0xDACB778D,0x00000000
	.dc.l		0x3FFF0000,0xBFC4EA46,0x63FA18F6,0x00000000
	.dc.l		0x3FFF0000,0xC0181BDE,0x8B89A454,0x00000000
	.dc.l		0x3FFF0000,0xC065B066,0xCFBF6439,0x00000000
	.dc.l		0x3FFF0000,0xC0AE345F,0x56340AE6,0x00000000
	.dc.l		0x3FFF0000,0xC0F22291,0x9CB9E6A7,0x00000000

	X = FP_SCR0
	XDCARE = X+2
	XFRAC = X+4
	XFRACLO = X+8

	ATANF = FP_SCR1
	ATANFHI = ATANF+4
	ATANFLO = ATANF+8

	.globl		satan
/* --ENTRY POINT FOR ATAN(X), HERE X IS FINITE, NON-ZERO, AND NOT NAN'S */
satan:
	fmove.x		(a0),fp0		/*  LOAD INPUT */

	move.l		(a0),d1
	move.w		4(a0),d1
	fmove.x		fp0,X(a6)
	andi.l		#0x7FFFFFFF,d1

	cmpi.l		#0x3FFB8000,d1		/*  |X| >= 1/16? */
	bge.b		ATANOK1
	bra.w		ATANSM

ATANOK1:
	cmpi.l		#0x4002FFFF,d1		/*  |X| < 16 ? */
	ble.b		ATANMAIN
	bra.w		ATANBIG

/* --THE MOST LIKELY CASE, |X| IN [1/16, 16). WE USE TABLE TECHNIQUE */
/* --THE IDEA IS ATAN(X) = ATAN(F) + ATAN( [X-F] / [1+XF] ). */
/* --SO IF F IS CHOSEN TO BE CLOSE TO X AND ATAN(F) IS STORED IN */
/* --A TABLE, ALL WE NEED IS TO APPROXIMATE ATAN(U) WHERE */
/* --U = (X-F)/(1+XF) IS SMALL (REMEMBER F IS CLOSE TO X). IT IS */
/* --TRUE THAT A DIVIDE IS NOW NEEDED, BUT THE APPROXIMATION FOR */
/* --ATAN(U) IS A VERY SHORT POLYNOMIAL AND THE INDEXING TO */
/* --FETCH F AND SAVING OF REGISTERS CAN BE ALL HIDED UNDER THE */
/* --DIVIDE. IN THE END THIS METHOD IS MUCH FASTER THAN A TRADITIONAL */
/* --ONE. NOTE ALSO THAT THE TRADITIONAL SCHEME THAT APPROXIMATE */
/* --ATAN(X) DIRECTLY WILL NEED TO USE A RATIONAL APPROXIMATION */
/* --(DIVISION NEEDED) ANYWAY BECAUSE A POLYNOMIAL APPROXIMATION */
/* --WILL INVOLVE A VERY LONG POLYNOMIAL. */

/* --NOW WE SEE X AS +-2^K * 1.BBBBBBB....B <- 1. + 63 BITS */
/* --WE CHOSE F TO BE +-2^K * 1.BBBB1 */
/* --THAT IS IT MATCHES THE EXPONENT AND FIRST 5 BITS OF X, THE */
/* --SIXTH BITS IS SET TO BE 1. SINCE K = -4, -3, ..., 3, THERE */
/* --ARE ONLY 8 TIMES 16 = 2^7 = 128 |F|'S. SINCE ATAN(-|F|) IS */
/* -- -ATAN(|F|), WE NEED TO STORE ONLY ATAN(|F|). */

ATANMAIN:

	andi.l		#0xF8000000,XFRAC(a6)	/*  FIRST 5 BITS */
	ori.l		#0x04000000,XFRAC(a6)	/*  SET 6-TH BIT TO 1 */
	move.l		#0x00000000,XFRACLO(a6) /*  LOCATION OF X IS NOW F */

	fmove.x		fp0,fp1		/*  FP1 IS X */
	fmul.x		X(a6),fp1		/*  FP1 IS X*F, NOTE THAT X*F > 0 */
	fsub.x		X(a6),fp0		/*  FP0 IS X-F */
	fadd.s		#0x3F800000,fp1	/*  FP1 IS 1 + X*F */
	fdiv.x		fp1,fp0		/*  FP0 IS U = (X-F)/(1+X*F) */

/* --WHILE THE DIVISION IS TAKING ITS TIME, WE FETCH ATAN(|F|) */
/* --CREATE ATAN(F) AND STORE IT IN ATANF, AND */
/* --SAVE REGISTERS FP2. */

	move.l		d2,-(sp)		/*  SAVE d2 TEMPORARILY */
	move.l		d1,d2			/*  THE EXP AND 16 BITS OF X */
	andi.l		#0x00007800,d1		/*  4 VARYING BITS OF F'S FRACTION */
	andi.l		#0x7FFF0000,d2		/*  EXPONENT OF F */
	subi.l		#0x3FFB0000,d2		/*  K+4 */
	asr.l		#1,d2
	add.l		d2,d1			/*  THE 7 BITS IDENTIFYING F */
	asr.l		#7,d1			/*  INDEX INTO TBL OF ATAN(|F|) */
	lea		ATANTBL(pc),a1
	add.l		d1,a1			/*  ADDRESS OF ATAN(|F|) */
	move.l		(a1)+,ATANF(a6)
	move.l		(a1)+,ATANFHI(a6)
	move.l		(a1)+,ATANFLO(a6)	/*  ATANF IS NOW ATAN(|F|) */
	move.l		X(a6),d1		/*  LOAD SIGN AND EXPO. AGAIN */
	andi.l		#0x80000000,d1		/*  SIGN(F) */
	or.l		d1,ATANF(a6)		/*  ATANF IS NOW SIGN(F)*ATAN(|F|) */
	move.l		(sp)+,d2		/*  RESTORE d2 */

/* --THAT'S ALL I HAVE TO DO FOR NOW, */
/* --BUT ALAS, THE DIVIDE IS STILL CRANKING! */

/* --U IN FP0, WE ARE NOW READY TO COMPUTE ATAN(U) AS */
/* --U + A1*U*V*(A2 + V*(A3 + V)), V = U*U */
/* --THE POLYNOMIAL MAY LOOK STRANGE, BUT IS NEVERTHELESS CORRECT. */
/* --THE NATURAL FORM IS U + U*V*(A1 + V*(A2 + V*A3)) */
/* --WHAT WE HAVE HERE IS MERELY	A1 = A3, A2 = A1/A3, A3 = A2/A3. */
/* --THE REASON FOR THIS REARRANGEMENT IS TO MAKE THE INDEPENDENT */
/* --PARTS A1*U*V AND (A2 + ... STUFF) MORE LOAD-BALANCED */

	fmovem.x		fp2,-(sp)		/*  save fp2 */

	fmove.x		fp0,fp1
	fmul.x		fp1,fp1
	fmove.d		ATANA3(pc),fp2
	fadd.x		fp1,fp2		/*  A3+V */
	fmul.x		fp1,fp2		/*  V*(A3+V) */
	fmul.x		fp0,fp1		/*  U*V */
	fadd.d		ATANA2(pc),fp2	/*  A2+V*(A3+V) */
	fmul.d		ATANA1(pc),fp1	/*  A1*U*V */
	fmul.x		fp2,fp1		/*  A1*U*V*(A2+V*(A3+V)) */
	fadd.x		fp1,fp0		/*  ATAN(U), FP1 RELEASED */

	fmovem.x		(sp)+,fp2		/*  restore fp2 */

	fmove.l		d0,fpcr		/*  restore users rnd mode,prec */
	fadd.x		ATANF(a6),fp0		/*  ATAN(X) */
	bra.l		t_inx2

ATANBORS:
/* --|X| IS IN d0 IN COMPACT FORM. FP1, d0 SAVED. */
/* --FP0 IS X AND |X| <= 1/16 OR |X| >= 16. */
	cmpi.l		#0x3FFF8000,d1
	bgt.w		ATANBIG			/*  I.E. |X| >= 16 */

ATANSM:
/* --|X| <= 1/16 */
/* --IF |X| < 2^(-40), RETURN X AS ANSWER. OTHERWISE, APPROXIMATE */
/* --ATAN(X) BY X + X*Y*(B1+Y*(B2+Y*(B3+Y*(B4+Y*(B5+Y*B6))))) */
/* --WHICH IS X + X*Y*( [B1+Z*(B3+Z*B5)] + [Y*(B2+Z*(B4+Z*B6)] ) */
/* --WHERE Y = X*X, AND Z = Y*Y. */

	cmpi.l		#0x3FD78000,d1
	blt.w		ATANTINY

/* --COMPUTE POLYNOMIAL */
	fmovem.x		fp2-fp3,-(sp)		/*  save fp2/fp3 */

	fmul.x		fp0,fp0		/*  FPO IS Y = X*X */

	fmove.x		fp0,fp1
	fmul.x		fp1,fp1		/*  FP1 IS Z = Y*Y */

	fmove.d		ATANB6(pc),fp2
	fmove.d		ATANB5(pc),fp3

	fmul.x		fp1,fp2		/*  Z*B6 */
	fmul.x		fp1,fp3		/*  Z*B5 */

	fadd.d		ATANB4(pc),fp2	/*  B4+Z*B6 */
	fadd.d		ATANB3(pc),fp3	/*  B3+Z*B5 */

	fmul.x		fp1,fp2		/*  Z*(B4+Z*B6) */
	fmul.x		fp3,fp1		/*  Z*(B3+Z*B5) */

	fadd.d		ATANB2(pc),fp2	/*  B2+Z*(B4+Z*B6) */
	fadd.d		ATANB1(pc),fp1	/*  B1+Z*(B3+Z*B5) */

	fmul.x		fp0,fp2		/*  Y*(B2+Z*(B4+Z*B6)) */
	fmul.x		X(a6),fp0		/*  X*Y */

	fadd.x		fp2,fp1		/*  [B1+Z*(B3+Z*B5)]+[Y*(B2+Z*(B4+Z*B6))] */

	fmul.x		fp1,fp0		/*  X*Y*([B1+Z*(B3+Z*B5)]+[Y*(B2+Z*(B4+Z*B6))]) */

	fmovem.x		(sp)+,fp2-fp3		/*  restore fp2/fp3 */

	fmove.l		d0,fpcr		/*  restore users rnd mode,prec */
	fadd.x		X(a6),fp0
	bra.l		t_inx2

ATANTINY:
/* --|X| < 2^(-40), ATAN(X) = X */

	fmove.l		d0,fpcr		/*  restore users rnd mode,prec */
	move.b		#FMOV_OP,d1		/*  last inst is MOVE */
	fmove.x		X(a6),fp0		/*  last inst - possible exception set */

	bra.l		t_catch

ATANBIG:
/* --IF |X| > 2^(100), RETURN	SIGN(X)*(PI/2 - TINY). OTHERWISE, */
/* --RETURN SIGN(X)*PI/2 + ATAN(-1/X). */
	cmpi.l		#0x40638000,d1
	bgt.w		ATANHUGE

/* --APPROXIMATE ATAN(-1/X) BY */
/* --X'+X'*Y*(C1+Y*(C2+Y*(C3+Y*(C4+Y*C5)))), X' = -1/X, Y = X'*X' */
/* --THIS CAN BE RE-WRITTEN AS */
/* --X'+X'*Y*( [C1+Z*(C3+Z*C5)] + [Y*(C2+Z*C4)] ), Z = Y*Y. */

	fmovem.x		fp2-fp3,-(sp)		/*  save fp2/fp3 */

	fmove.s		#0xBF800000,fp1	/*  LOAD -1 */
	fdiv.x		fp0,fp1		/*  FP1 IS -1/X */

/* --DIVIDE IS STILL CRANKING */

	fmove.x		fp1,fp0		/*  FP0 IS X' */
	fmul.x		fp0,fp0		/*  FP0 IS Y = X'*X' */
	fmove.x		fp1,X(a6)		/*  X IS REALLY X' */

	fmove.x		fp0,fp1
	fmul.x		fp1,fp1		/*  FP1 IS Z = Y*Y */

	fmove.d		ATANC5(pc),fp3
	fmove.d		ATANC4(pc),fp2

	fmul.x		fp1,fp3		/*  Z*C5 */
	fmul.x		fp1,fp2		/*  Z*B4 */

	fadd.d		ATANC3(pc),fp3	/*  C3+Z*C5 */
	fadd.d		ATANC2(pc),fp2	/*  C2+Z*C4 */

	fmul.x		fp3,fp1		/*  Z*(C3+Z*C5), FP3 RELEASED */
	fmul.x		fp0,fp2		/*  Y*(C2+Z*C4) */

	fadd.d		ATANC1(pc),fp1	/*  C1+Z*(C3+Z*C5) */
	fmul.x		X(a6),fp0		/*  X'*Y */

	fadd.x		fp2,fp1		/*  [Y*(C2+Z*C4)]+[C1+Z*(C3+Z*C5)] */

	fmul.x		fp1,fp0		/*  X'*Y*([B1+Z*(B3+Z*B5)] */
/* 					...	+[Y*(B2+Z*(B4+Z*B6))]) */
	fadd.x		X(a6),fp0

	fmovem.x		(sp)+,fp2-fp3		/*  restore fp2/fp3 */

	fmove.l		d0,fpcr		/*  restore users rnd mode,prec */
	tst.b		(a0)
	bpl.b		pos_big

neg_big:
	fadd.x		NPIBY2(pc),fp0
	bra.l		t_minx2

pos_big:
	fadd.x		PPIBY2(pc),fp0
	bra.l		t_pinx2

ATANHUGE:
/* --RETURN SIGN(X)*(PIBY2 - TINY) = SIGN(X)*PIBY2 - SIGN(X)*TINY */
	tst.b		(a0)
	bpl.b		pos_huge

neg_huge:
	fmove.x		NPIBY2(pc),fp0
	fmove.l		d0,fpcr
	fadd.x		PTINY(pc),fp0
	bra.l		t_minx2

pos_huge:
	fmove.x		PPIBY2(pc),fp0
	fmove.l		d0,fpcr
	fadd.x		NTINY(pc),fp0
	bra.l		t_pinx2

	.globl		satand
/* --ENTRY POINT FOR ATAN(X) FOR DENORMALIZED ARGUMENT */
satand:
	bra.l		t_extdnrm

