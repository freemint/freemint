/*
 * 	setox.sa 3.1 12/10/90
 * 
 * 	The entry point setox computes the exponential of a value.
 * 	setoxd does the same except the input value is a denormalized
 * 	number.	setoxm1 computes exp(X)-1, and setoxm1d computes
 * 	exp(X)-1 for denormalized X.
 * 
 * 	INPUT
 * 	-----
 * 	Double-extended value in memory location pointed to by address
 * 	register a0.
 * 
 * 	OUTPUT
 * 	------
 * 	exp(X) or exp(X)-1 returned in floating-point register fp0.
 * 
 * 	ACCURACY and MONOTONICITY
 * 	-------------------------
 * 	The returned result is within 0.85 ulps in 64 significant bit, i.e.
 * 	within 0.5001 ulp to 53 bits if the result is subsequently rounded
 * 	to double precision. The result is provably monotonic in double
 * 	precision.
 * 
 * 	SPEED
 * 	-----
 * 	Two timings are measured, both in the copy-back mode. The
 * 	first one is measured when the function is invoked the first time
 * 	(so the instructions and data are not in cache), and the
 * 	second one is measured when the function is reinvoked at the same
 * 	input argument.
 * 
 * 	The program setox takes approximately 210/190 cycles for input
 * 	argument X whose magnitude is less than 16380 log2, which
 * 	is the usual situation.	For the less common arguments,
 * 	depending on their values, the program may run faster or slower --
 * 	but no worse than 10% slower even in the extreme cases.
 * 
 * 	The program setoxm1 takes approximately ??? / ??? cycles for input
 * 	argument X, 0.25 <= |X| < 70log2. For |X| < 0.25, it takes
 * 	approximately ??? / ??? cycles. For the less common arguments,
 * 	depending on their values, the program may run faster or slower --
 * 	but no worse than 10% slower even in the extreme cases.
 * 
 * 	ALGORITHM and IMPLEMENTATION NOTES
 * 	----------------------------------
 * 
 * 	setoxd
 * 	------
 * 	Step 1.	Set ans := 1.0
 * 
 * 	Step 2.	Return	ans := ans + sign(X)*2^(-126). Exit.
 * 	Notes:	This will always generate one exception -- inexact.
 * 
 * 
 * 	setox
 * 	-----
 * 
 * 	Step 1.	Filter out extreme cases of input argument.
 * 		1.1	If |X| >= 2^(-65), go to Step 1.3.
 * 		1.2	Go to Step 7.
 * 		1.3	If |X| < 16380 log(2), go to Step 2.
 * 		1.4	Go to Step 8.
 * 	Notes:	The usual case should take the branches 1.1 -> 1.3 -> 2.
 * 		 To avoid the use of floating-point comparisons, a
 * 		 compact representation of |X| is used. This format is a
 * 		 32-bit integer, the upper (more significant) 16 bits are
 * 		 the sign and biased exponent field of |X|; the lower 16
 * 		 bits are the 16 most significant fraction (including the
 * 		 explicit bit) bits of |X|. Consequently, the comparisons
 * 		 in Steps 1.1 and 1.3 can be performed by integer comparison.
 * 		 Note also that the constant 16380 log(2) used in Step 1.3
 * 		 is also in the compact form. Thus taking the branch
 * 		 to Step 2 guarantees |X| < 16380 log(2). There is no harm
 * 		 to have a small number of cases where |X| is less than,
 * 		 but close to, 16380 log(2) and the branch to Step 9 is
 * 		 taken.
 * 
 * 	Step 2.	Calculate N = round-to-nearest-int( X * 64/log2 ).
 * 		2.1	Set AdjFlag := 0 (indicates the branch 1.3 -> 2 was taken)
 * 		2.2	N := round-to-nearest-integer( X * 64/log2 ).
 * 		2.3	Calculate	J = N mod 64; so J = 0,1,2,..., or 63.
 * 		2.4	Calculate	M = (N - J)/64; so N = 64M + J.
 * 		2.5	Calculate the address of the stored value of 2^(J/64).
 * 		2.6	Create the value Scale = 2^M.
 * 	Notes:	The calculation in 2.2 is really performed by
 * 
 * 			Z := X * constant
 * 			N := round-to-nearest-integer(Z)
 * 
 * 		 where
 * 
 * 			constant := single-precision( 64/log 2 ).
 * 
 * 		 Using a single-precision constant avoids memory access.
 * 		 Another effect of using a single-precision "constant" is
 * 		 that the calculated value Z is
 * 
 * 			Z = X*(64/log2)*(1+eps), |eps| <= 2^(-24).
 * 
 * 		 This error has to be considered later in Steps 3 and 4.
 * 
 * 	Step 3.	Calculate X - N*log2/64.
 * 		3.1	R := X + N*L1, where L1 := single-precision(-log2/64).
 * 		3.2	R := R + N*L2, L2 := extended-precision(-log2/64 - L1).
 * 	Notes:	a) The way L1 and L2 are chosen ensures L1+L2 approximate
 * 		 the value	-log2/64	to 88 bits of accuracy.
 * 		 b) N*L1 is exact because N is no longer than 22 bits and
 * 		 L1 is no longer than 24 bits.
 * 		 c) The calculation X+N*L1 is also exact due to cancellation.
 * 		 Thus, R is practically X+N(L1+L2) to full 64 bits.
 * 		 d) It is important to estimate how large can |R| be after
 * 		 Step 3.2.
 * 
 * 			N = rnd-to-int( X*64/log2 (1+eps) ), |eps|<=2^(-24)
 * 			X*64/log2 (1+eps)	=	N + f,	|f| <= 0.5
 * 			X*64/log2 - N	=	f - eps*X 64/log2
 * 			X - N*log2/64	=	f*log2/64 - eps*X
 * 
 * 
 * 		 Now |X| <= 16446 log2, thus
 * 
 * 			|X - N*log2/64| <= (0.5 + 16446/2^(18))*log2/64
 * 					<= 0.57 log2/64.
 * 		 This bound will be used in Step 4.
 * 
 * 	Step 4.	Approximate exp(R)-1 by a polynomial
 * 			p = R + R*R*(A1 + R*(A2 + R*(A3 + R*(A4 + R*A5))))
 * 	Notes:	a) In order to reduce memory access, the coefficients are
 * 		 made as "short" as possible: A1 (which is 1/2), A4 and A5
 * 		 are single precision; A2 and A3 are double precision.
 * 		 b) Even with the restrictions above,
 * 			|p - (exp(R)-1)| < 2^(-68.8) for all |R| <= 0.0062.
 * 		 Note that 0.0062 is slightly bigger than 0.57 log2/64.
 * 		 c) To fully utilize the pipeline, p is separated into
 * 		 two independent pieces of roughly equal complexities
 * 			p = [ R + R*S*(A2 + S*A4) ]	+
 * 				[ S*(A1 + S*(A3 + S*A5)) ]
 * 		 where S = R*R.
 * 
 * 	Step 5.	Compute 2^(J/64)*exp(R) = 2^(J/64)*(1+p) by
 * 				ans := T + ( T*p + t)
 * 		 where T and t are the stored values for 2^(J/64).
 * 	Notes:	2^(J/64) is stored as T and t where T+t approximates
 * 		 2^(J/64) to roughly 85 bits; T is in extended precision
 * 		 and t is in single precision. Note also that T is rounded
 * 		 to 62 bits so that the last two bits of T are zero. The
 * 		 reason for such a special form is that T-1, T-2, and T-8
 * 		 will all be exact --- a property that will give much
 * 		 more accurate computation of the function EXPM1.
 * 
 * 	Step 6.	Reconstruction of exp(X)
 * 			exp(X) = 2^M * 2^(J/64) * exp(R).
 * 		6.1	If AdjFlag = 0, go to 6.3
 * 		6.2	ans := ans * AdjScale
 * 		6.3	Restore the user FPCR
 * 		6.4	Return ans := ans * Scale. Exit.
 * 	Notes:	If AdjFlag = 0, we have X = Mlog2 + Jlog2/64 + R,
 * 		 |M| <= 16380, and Scale = 2^M. Moreover, exp(X) will
 * 		 neither overflow nor underflow. If AdjFlag = 1, that
 * 		 means that
 * 			X = (M1+M)log2 + Jlog2/64 + R, |M1+M| >= 16380.
 * 		 Hence, exp(X) may overflow or underflow or neither.
 * 		 When that is the case, AdjScale = 2^(M1) where M1 is
 * 		 approximately M. Thus 6.2 will never cause over/underflow.
 * 		 Possible exception in 6.4 is overflow or underflow.
 * 		 The inexact exception is not generated in 6.4. Although
 * 		 one can argue that the inexact flag should always be
 * 		 raised, to simulate that exception cost to much than the
 * 		 flag is worth in practical uses.
 * 
 * 	Step 7.	Return 1 + X.
 * 		7.1	ans := X
 * 		7.2	Restore user FPCR.
 * 		7.3	Return ans := 1 + ans. Exit
 * 	Notes:	For non-zero X, the inexact exception will always be
 * 		 raised by 7.3. That is the only exception raised by 7.3.
 * 		 Note also that we use the FMOVEM instruction to move X
 * 		 in Step 7.1 to avoid unnecessary trapping. (Although
 * 		 the FMOVEM may not seem relevant since X is normalized,
 * 		 the precaution will be useful in the library version of
 * 		 this code where the separate entry for denormalized inputs
 * 		 will be done away with.)
 * 
 * 	Step 8.	Handle exp(X) where |X| >= 16380log2.
 * 		8.1	If |X| > 16480 log2, go to Step 9.
 * 		(mimic 2.2 - 2.6)
 * 		8.2	N := round-to-integer( X * 64/log2 )
 * 		8.3	Calculate J = N mod 64, J = 0,1,...,63
 * 		8.4	K := (N-J)/64, M1 := truncate(K/2), M = K-M1, AdjFlag := 1.
 * 		8.5	Calculate the address of the stored value 2^(J/64).
 * 		8.6	Create the values Scale = 2^M, AdjScale = 2^M1.
 * 		8.7	Go to Step 3.
 * 	Notes:	Refer to notes for 2.2 - 2.6.
 * 
 * 	Step 9.	Handle exp(X), |X| > 16480 log2.
 * 		9.1	If X < 0, go to 9.3
 * 		9.2	ans := Huge, go to 9.4
 * 		9.3	ans := Tiny.
 * 		9.4	Restore user FPCR.
 * 		9.5	Return ans := ans * ans. Exit.
 * 	Notes:	Exp(X) will surely overflow or underflow, depending on
 * 		 X's sign. "Huge" and "Tiny" are respectively large/tiny
 * 		 extended-precision numbers whose square over/underflow
 * 		 with an inexact result. Thus, 9.5 always raises the
 * 		 inexact together with either overflow or underflow.
 * 
 * 
 * 	setoxm1d
 * 	--------
 * 
 * 	Step 1.	Set ans := 0
 * 
 * 	Step 2.	Return	ans := X + ans. Exit.
 * 	Notes:	This will return X with the appropriate rounding
 * 		 precision prescribed by the user FPCR.
 * 
 * 	setoxm1
 * 	-------
 * 
 * 	Step 1.	Check |X|
 * 		1.1	If |X| >= 1/4, go to Step 1.3.
 * 		1.2	Go to Step 7.
 * 		1.3	If |X| < 70 log(2), go to Step 2.
 * 		1.4	Go to Step 10.
 * 	Notes:	The usual case should take the branches 1.1 -> 1.3 -> 2.
 * 		 However, it is conceivable |X| can be small very often
 * 		 because EXPM1 is intended to evaluate exp(X)-1 accurately
 * 		 when |X| is small. For further details on the comparisons,
 * 		 see the notes on Step 1 of setox.
 * 
 * 	Step 2.	Calculate N = round-to-nearest-int( X * 64/log2 ).
 * 		2.1	N := round-to-nearest-integer( X * 64/log2 ).
 * 		2.2	Calculate	J = N mod 64; so J = 0,1,2,..., or 63.
 * 		2.3	Calculate	M = (N - J)/64; so N = 64M + J.
 * 		2.4	Calculate the address of the stored value of 2^(J/64).
 * 		2.5	Create the values Sc = 2^M and OnebySc := -2^(-M).
 * 	Notes:	See the notes on Step 2 of setox.
 * 
 * 	Step 3.	Calculate X - N*log2/64.
 * 		3.1	R := X + N*L1, where L1 := single-precision(-log2/64).
 * 		3.2	R := R + N*L2, L2 := extended-precision(-log2/64 - L1).
 * 	Notes:	Applying the analysis of Step 3 of setox in this case
 * 		 shows that |R| <= 0.0055 (note that |X| <= 70 log2 in
 * 		 this case).
 * 
 * 	Step 4.	Approximate exp(R)-1 by a polynomial
 * 			p = R+R*R*(A1+R*(A2+R*(A3+R*(A4+R*(A5+R*A6)))))
 * 	Notes:	a) In order to reduce memory access, the coefficients are
 * 		 made as "short" as possible: A1 (which is 1/2), A5 and A6
 * 		 are single precision; A2, A3 and A4 are double precision.
 * 		 b) Even with the restriction above,
 * 			|p - (exp(R)-1)| <	|R| * 2^(-72.7)
 * 		 for all |R| <= 0.0055.
 * 		 c) To fully utilize the pipeline, p is separated into
 * 		 two independent pieces of roughly equal complexity
 * 			p = [ R*S*(A2 + S*(A4 + S*A6)) ]	+
 * 				[ R + S*(A1 + S*(A3 + S*A5)) ]
 * 		 where S = R*R.
 * 
 * 	Step 5.	Compute 2^(J/64)*p by
 * 				p := T*p
 * 		 where T and t are the stored values for 2^(J/64).
 * 	Notes:	2^(J/64) is stored as T and t where T+t approximates
 * 		 2^(J/64) to roughly 85 bits; T is in extended precision
 * 		 and t is in single precision. Note also that T is rounded
 * 		 to 62 bits so that the last two bits of T are zero. The
 * 		 reason for such a special form is that T-1, T-2, and T-8
 * 		 will all be exact --- a property that will be exploited
 * 		 in Step 6 below. The total relative error in p is no
 * 		 bigger than 2^(-67.7) compared to the final result.
 * 
 * 	Step 6.	Reconstruction of exp(X)-1
 * 			exp(X)-1 = 2^M * ( 2^(J/64) + p - 2^(-M) ).
 * 		6.1	If M <= 63, go to Step 6.3.
 * 		6.2	ans := T + (p + (t + OnebySc)). Go to 6.6
 * 		6.3	If M >= -3, go to 6.5.
 * 		6.4	ans := (T + (p + t)) + OnebySc. Go to 6.6
 * 		6.5	ans := (T + OnebySc) + (p + t).
 * 		6.6	Restore user FPCR.
 * 		6.7	Return ans := Sc * ans. Exit.
 * 	Notes:	The various arrangements of the expressions give accurate
 * 		 evaluations.
 * 
 * 	Step 7.	exp(X)-1 for |X| < 1/4.
 * 		7.1	If |X| >= 2^(-65), go to Step 9.
 * 		7.2	Go to Step 8.
 * 
 * 	Step 8.	Calculate exp(X)-1, |X| < 2^(-65).
 * 		8.1	If |X| < 2^(-16312), goto 8.3
 * 		8.2	Restore FPCR; return ans := X - 2^(-16382). Exit.
 * 		8.3	X := X * 2^(140).
 * 		8.4	Restore FPCR; ans := ans - 2^(-16382).
 * 		 Return ans := ans*2^(140). Exit
 * 	Notes:	The idea is to return "X - tiny" under the user
 * 		 precision and rounding modes. To avoid unnecessary
 * 		 inefficiency, we stay away from denormalized numbers the
 * 		 best we can. For |X| >= 2^(-16312), the straightforward
 * 		 8.2 generates the inexact exception as the case warrants.
 * 
 * 	Step 9.	Calculate exp(X)-1, |X| < 1/4, by a polynomial
 * 			p = X + X*X*(B1 + X*(B2 + ... + X*B12))
 * 	Notes:	a) In order to reduce memory access, the coefficients are
 * 		 made as "short" as possible: B1 (which is 1/2), B9 to B12
 * 		 are single precision; B3 to B8 are double precision; and
 * 		 B2 is double extended.
 * 		 b) Even with the restriction above,
 * 			|p - (exp(X)-1)| < |X| 2^(-70.6)
 * 		 for all |X| <= 0.251.
 * 		 Note that 0.251 is slightly bigger than 1/4.
 * 		 c) To fully preserve accuracy, the polynomial is computed
 * 		 as	X + ( S*B1 +	Q ) where S = X*X and
 * 			Q	=	X*S*(B2 + X*(B3 + ... + X*B12))
 * 		 d) To fully utilize the pipeline, Q is separated into
 * 		 two independent pieces of roughly equal complexity
 * 			Q = [ X*S*(B2 + S*(B4 + ... + S*B12)) ] +
 * 				[ S*S*(B3 + S*(B5 + ... + S*B11)) ]
 * 
 * 	Step 10.	Calculate exp(X)-1 for |X| >= 70 log 2.
 * 		10.1 If X >= 70log2 , exp(X) - 1 = exp(X) for all practical
 * 		 purposes. Therefore, go to Step 1 of setox.
 * 		10.2 If X <= -70log2, exp(X) - 1 = -1 for all practical purposes.
 * 		 ans := -1
 * 		 Restore user FPCR
 * 		 Return ans := ans + 2^(-126). Exit.
 * 	Notes:	10.2 will always create an inexact and return -1 + tiny
 * 		 in the user rounding precision and mode.
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

	.text
L2:	.dc.l	0x3FDC0000,0x82E30865,0x4361C4C6,0x00000000

EXPA3_setox:	.dc.l	0x3FA55555,0x55554431
EXPA2_setox:	.dc.l	0x3FC55555,0x55554018

EM1A4:	.dc.l	0x3F811111,0x11174385
EM1A3:	.dc.l	0x3FA55555,0x55554F5A

EM1A2:	.dc.l	0x3FC55555,0x55555555,0x00000000,0x00000000

EM1B8:	.dc.l	0x3EC71DE3,0xA5774682
EM1B7:	.dc.l	0x3EFA01A0,0x19D7CB68

EM1B6:	.dc.l	0x3F2A01A0,0x1A019DF3
EM1B5:	.dc.l	0x3F56C16C,0x16C170E2

EM1B4:	.dc.l	0x3F811111,0x11111111
EM1B3:	.dc.l	0x3FA55555,0x55555555

EM1B2:	.dc.l	0x3FFC0000,0xAAAAAAAA,0xAAAAAAAB

TWO140:	.dc.l	0x48B00000,0x00000000
TWON140:	.dc.l	0x37300000,0x00000000

ONEHALF: dc.l 0x3F000000

EXPTBL:
	.dc.l	0x3FFF0000,0x80000000,0x00000000,0x00000000
	.dc.l	0x3FFF0000,0x8164D1F3,0xBC030774,0x9F841A9B
	.dc.l	0x3FFF0000,0x82CD8698,0xAC2BA1D8,0x9FC1D5B9
	.dc.l	0x3FFF0000,0x843A28C3,0xACDE4048,0xA0728369
	.dc.l	0x3FFF0000,0x85AAC367,0xCC487B14,0x1FC5C95C
	.dc.l	0x3FFF0000,0x871F6196,0x9E8D1010,0x1EE85C9F
	.dc.l	0x3FFF0000,0x88980E80,0x92DA8528,0x9FA20729
	.dc.l	0x3FFF0000,0x8A14D575,0x496EFD9C,0xA07BF9AF
	.dc.l	0x3FFF0000,0x8B95C1E3,0xEA8BD6E8,0xA0020DCF
	.dc.l	0x3FFF0000,0x8D1ADF5B,0x7E5BA9E4,0x205A63DA
	.dc.l	0x3FFF0000,0x8EA4398B,0x45CD53C0,0x1EB70051
	.dc.l	0x3FFF0000,0x9031DC43,0x1466B1DC,0x1F6EB029
	.dc.l	0x3FFF0000,0x91C3D373,0xAB11C338,0xA0781494
	.dc.l	0x3FFF0000,0x935A2B2F,0x13E6E92C,0x9EB319B0
	.dc.l	0x3FFF0000,0x94F4EFA8,0xFEF70960,0x2017457D
	.dc.l	0x3FFF0000,0x96942D37,0x20185A00,0x1F11D537
	.dc.l	0x3FFF0000,0x9837F051,0x8DB8A970,0x9FB952DD
	.dc.l	0x3FFF0000,0x99E04593,0x20B7FA64,0x1FE43087
	.dc.l	0x3FFF0000,0x9B8D39B9,0xD54E5538,0x1FA2A818
	.dc.l	0x3FFF0000,0x9D3ED9A7,0x2CFFB750,0x1FDE494D
	.dc.l	0x3FFF0000,0x9EF53260,0x91A111AC,0x20504890
	.dc.l	0x3FFF0000,0xA0B0510F,0xB9714FC4,0xA073691C
	.dc.l	0x3FFF0000,0xA2704303,0x0C496818,0x1F9B7A05
	.dc.l	0x3FFF0000,0xA43515AE,0x09E680A0,0xA0797126
	.dc.l	0x3FFF0000,0xA5FED6A9,0xB15138EC,0xA071A140
	.dc.l	0x3FFF0000,0xA7CD93B4,0xE9653568,0x204F62DA
	.dc.l	0x3FFF0000,0xA9A15AB4,0xEA7C0EF8,0x1F283C4A
	.dc.l	0x3FFF0000,0xAB7A39B5,0xA93ED338,0x9F9A7FDC
	.dc.l	0x3FFF0000,0xAD583EEA,0x42A14AC8,0xA05B3FAC
	.dc.l	0x3FFF0000,0xAF3B78AD,0x690A4374,0x1FDF2610
	.dc.l	0x3FFF0000,0xB123F581,0xD2AC2590,0x9F705F90
	.dc.l	0x3FFF0000,0xB311C412,0xA9112488,0x201F678A
	.dc.l	0x3FFF0000,0xB504F333,0xF9DE6484,0x1F32FB13
	.dc.l	0x3FFF0000,0xB6FD91E3,0x28D17790,0x20038B30
	.dc.l	0x3FFF0000,0xB8FBAF47,0x62FB9EE8,0x200DC3CC
	.dc.l	0x3FFF0000,0xBAFF5AB2,0x133E45FC,0x9F8B2AE6
	.dc.l	0x3FFF0000,0xBD08A39F,0x580C36C0,0xA02BBF70
	.dc.l	0x3FFF0000,0xBF1799B6,0x7A731084,0xA00BF518
	.dc.l	0x3FFF0000,0xC12C4CCA,0x66709458,0xA041DD41
	.dc.l	0x3FFF0000,0xC346CCDA,0x24976408,0x9FDF137B
	.dc.l	0x3FFF0000,0xC5672A11,0x5506DADC,0x201F1568
	.dc.l	0x3FFF0000,0xC78D74C8,0xABB9B15C,0x1FC13A2E
	.dc.l	0x3FFF0000,0xC9B9BD86,0x6E2F27A4,0xA03F8F03
	.dc.l	0x3FFF0000,0xCBEC14FE,0xF2727C5C,0x1FF4907D
	.dc.l	0x3FFF0000,0xCE248C15,0x1F8480E4,0x9E6E53E4
	.dc.l	0x3FFF0000,0xD06333DA,0xEF2B2594,0x1FD6D45C
	.dc.l	0x3FFF0000,0xD2A81D91,0xF12AE45C,0xA076EDB9
	.dc.l	0x3FFF0000,0xD4F35AAB,0xCFEDFA20,0x9FA6DE21
	.dc.l	0x3FFF0000,0xD744FCCA,0xD69D6AF4,0x1EE69A2F
	.dc.l	0x3FFF0000,0xD99D15C2,0x78AFD7B4,0x207F439F
	.dc.l	0x3FFF0000,0xDBFBB797,0xDAF23754,0x201EC207
	.dc.l	0x3FFF0000,0xDE60F482,0x5E0E9124,0x9E8BE175
	.dc.l	0x3FFF0000,0xE0CCDEEC,0x2A94E110,0x20032C4B
	.dc.l	0x3FFF0000,0xE33F8972,0xBE8A5A50,0x2004DFF5
	.dc.l	0x3FFF0000,0xE5B906E7,0x7C8348A8,0x1E72F47A
	.dc.l	0x3FFF0000,0xE8396A50,0x3C4BDC68,0x1F722F22
	.dc.l	0x3FFF0000,0xEAC0C6E7,0xDD243930,0xA017E945
	.dc.l	0x3FFF0000,0xED4F301E,0xD9942B84,0x1F401A5B
	.dc.l	0x3FFF0000,0xEFE4B99B,0xDCDAF5CC,0x9FB9A9E3
	.dc.l	0x3FFF0000,0xF281773C,0x59FFB138,0x20744C05
	.dc.l	0x3FFF0000,0xF5257D15,0x2486CC2C,0x1F773A19
	.dc.l	0x3FFF0000,0xF7D0DF73,0x0AD13BB8,0x1FFE90D5
	.dc.l	0x3FFF0000,0xFA83B2DB,0x722A033C,0xA041ED22
	.dc.l	0x3FFF0000,0xFD3E0C0C,0xF486C174,0x1F853F3A

	ADJFLAG = L_SCR2
	SCALE = FP_SCR1
	ADJSCALE = FP_SCR2
	SC = FP_SCR3
	ONEBYSC = FP_SCR4

	.xref	t_frcinx
	.xref	t_extdnrm
	.xref	t_unfl
	.xref	t_ovfl

	.text

	.globl	setoxd
setoxd:
/*
 * --entry point for EXP(X), X is denormalized
 */
	move.l		(a0),d0
	andi.l		#0x80000000,d0
	ori.l		#0x00800000,d0		/* ...sign(X)*2^(-126) */
	move.l		d0,-(sp)
	.IFNE GAS
	fmove.s		#0x3F800000,fp0
	.ELSE
	fmove.s		#1.0,fp0
	.ENDC
	fmove.l		d1,fpcr
	fadd.s		(sp)+,fp0
	bra		t_frcinx

	.globl	setox
setox:
/*
 * --entry point for EXP(X), here X is finite, non-zero, and not NaN's
 */
/*
 * --Step 1.
 */
	move.l		(a0),d0	 		/* ...load part of input X */
	andi.l		#0x7FFF0000,d0	/* ...biased expo. of X */
	cmpi.l		#0x3FBE0000,d0	/* ...2^(-65) */
	bge.s		EXPC1			/* ...normal case */
	bra		EXPSM_setox

EXPC1:
/*
 * --The case |X| >= 2^(-65)
 */
	move.w		4(a0),d0		/* ...expo. and partial sig. of |X| */
	cmpi.l		#0x400CB167,d0	/* ...16380 log2 trunc. 16 bits */
	blt.s		EXPMAIN			/* ...normal case */
	bra		EXPBIG_setox

EXPMAIN:
/*
 * --Step 2.
 * --This is the normal branch:	2^(-65) <= |X| < 16380 log2.
 */
	fmove.x		(a0),fp0			/* ...load input from (a0) */

	fmove.x		fp0,fp1
	.IFNE GAS
	fmul.s		#0x42B8AA3B,fp0		/* ...64/log2 * X */
	.ELSE
	fmul.s		#92.3324813842773438,fp0		/* ...64/log2 * X */
	.ENDC
	fmovem.x	fp2-fp2/fp3,-(a7)	/* ...save fp2 */
	move.l		#0,ADJFLAG(a6)
	fmove.l		fp0,d0				/* ...N = int( X * 64/log2 ) */
	lea		EXPTBL.w(pc),a1
	fmove.l		d0,fp0				/* ...convert to floating-format */

	move.l		d0,L_SCR1(a6)		/* ...save N temporarily */
	andi.l		#0x3F,d0			/* ...D0 is J = N mod 64 */
	lsl.l		#4,d0
	adda.l		d0,a1				/* ...address of 2^(J/64) */
	move.l		L_SCR1(a6),d0
	asr.l		#6,d0				/* ...D0 is M */
	addi.w		#0x3FFF,d0			/* ...biased expo. of 2^(M) */
	move.w		L2.w(pc),L_SCR1(a6)		/* ...prefetch L2, no need in CB */

EXPCONT1:
/*
 * --Step 3.
 * --fp1,fp2 saved on the stack. fp0 is N, fp1 is X,
 * --a0 points to 2^(J/64), D0 is biased expo. of 2^(M)
 */
	fmove.x		fp0,fp2
	.IFNE GAS
	fmul.s		#0xBC317218,fp0		/* ...N * L1, L1 = lead(-log2/64) */
	.ELSE
	fmul.s		#-0.0108304247260093689,fp0		/* ...N * L1, L1 = lead(-log2/64) */
	.ENDC
	fmul.x		L2.w(pc),fp2				/* ...N * L2, L1+L2 = -log2/64 */
	fadd.x		fp1,fp0				/* ...X + N*L1 */
	fadd.x		fp2,fp0				/* ...fp0 is R, reduced arg. */
/* 	MOVE.W		#0x3FA5,EXPA3_setox	...load EXPA3 in cache */

/*
 * --Step 4.
 * --WE NOW COMPUTE EXP(R)-1 BY A POLYNOMIAL
 * -- R + R*R*(A1 + R*(A2 + R*(A3 + R*(A4 + R*A5))))
 * --TO FULLY UTILIZE THE PIPELINE, WE COMPUTE S = R*R
 * --[R+R*S*(A2+S*A4)] + [S*(A1+S*(A3+S*A5))]
 */
	fmove.x		fp0,fp1
	fmul.x		fp1,fp1				/* ...fp1 IS S = R*R */

	.IFNE GAS
	fmove.s		#0x3AB60B70,fp2			/* ...fp2 IS A5 */
	.ELSE
	fmove.s		#0.00138889066874980927,fp2			/* ...fp2 IS A5 */
	.ENDC
/* 	MOVE.W		#0,2(a1)	...load 2^(J/64) in cache */

	fmul.x		fp1,fp2				/* ...fp2 IS S*A5 */
	fmove.x		fp1,fp3
	.IFNE GAS
	fmul.s		#0x3C088895,fp3		/* ...fp3 IS S*A4 */
	.ELSE
	fmul.s		#0.00833334494382143021,fp3		/* ...fp3 IS S*A4 */
	.ENDC

	fadd.d		EXPA3_setox.w(pc),fp2		/* ...fp2 IS A3+S*A5 */
	fadd.d		EXPA2_setox.w(pc),fp3		/* ...fp3 IS A2+S*A4 */

	fmul.x		fp1,fp2				/* ...fp2 IS S*(A3+S*A5) */
	move.w		d0,SCALE(a6)		/* ...SCALE is 2^(M) in extended */
	clr.w		SCALE+2(a6)
	move.l		#0x80000000,SCALE+4(a6)
	clr.l		SCALE+8(a6)

	fmul.x		fp1,fp3				/* ...fp3 IS S*(A2+S*A4) */

	fadd.s		ONEHALF.w(pc),fp2	/* ...fp2 IS A1+S*(A3+S*A5) */
	fmul.x		fp0,fp3				/* ...fp3 IS R*S*(A2+S*A4) */

	fmul.x		fp1,fp2				/* ...fp2 IS S*(A1+S*(A3+S*A5)) */
	fadd.x		fp3,fp0				/* ...fp0 IS R+R*S*(A2+S*A4), */
									/* ...fp3 released */

	fmove.x		(a1)+,fp1			/* ...fp1 is lead. pt. of 2^(J/64) */
	fadd.x		fp2,fp0				/* ...fp0 is EXP(R) - 1 */
									/* ...fp2 released */

/*
 * --Step 5
 * --final reconstruction process
 * --EXP(X) = 2^M * ( 2^(J/64) + 2^(J/64)*(EXP(R)-1) )
 */
	fmul.x		fp1,fp0				/* ...2^(J/64)*(Exp(R)-1) */
	fmovem.x	(a7)+,fp2-fp2/fp3	/* ...fp2 restored */
	fadd.s		(a1),fp0			/* ...accurate 2^(J/64) */

	fadd.x		fp1,fp0				/* ...2^(J/64) + 2^(J/64)*... */
	move.l		ADJFLAG(a6),d0

/*
 * --Step 6
 */
	tst.l		d0
	beq.s		NORMAL
ADJUST:
	fmul.x		ADJSCALE(a6),fp0
NORMAL:
	fmove.l		d1,fpcr				/* ...restore user FPCR */
	fmul.x		SCALE(a6),fp0		/* ...multiply 2^(M) */
	bra		t_frcinx

EXPSM_setox:
/*
 * --Step 7
 */
	fmovem.x	(a0),fp0-fp0		/* ...in case X is denormalized */
	fmove.l		d1,fpcr
	.IFNE GAS
	fadd.s		#0x3F800000,fp0		/* ...1+X in user mode */
	.ELSE
	fadd.s		#1.0,fp0			/* ...1+X in user mode */
	.ENDC
	bra		t_frcinx

EXPBIG_setox:
/*
 * --Step 8
 */
	cmpi.l		#0x400CB27C,d0		/* ...16480 log2 */
	bgt.s		EXP2BIG
/*
 * --Steps 8.2 -- 8.6
 */
	fmove.x		(a0),fp0			/* ...load input from (a0) */

	fmove.x		fp0,fp1
	.IFNE GAS
	fmul.s		#0x42B8AA3B,fp0		/* ...64/log2 * X */
	.ELSE
	fmul.s		#92.3324813842773438,fp0		/* ...64/log2 * X */
	.ENDC
	fmovem.x	 fp2-fp2/fp3,-(a7)	/* ...save fp2 */
	move.l		#1,ADJFLAG(a6)
	fmove.l		fp0,d0				/* ...N = int( X * 64/log2 ) */
	lea		EXPTBL.w(pc),a1
	fmove.l		d0,fp0				/* ...convert to floating-format */
	move.l		d0,L_SCR1(a6)		/* ...save N temporarily */
	andi.l		#0x3F,d0			/* ...D0 is J = N mod 64 */
	lsl.l		#4,d0
	adda.l		d0,a1				/* ...address of 2^(J/64) */
	move.l		L_SCR1(a6),d0
	asr.l		#6,d0				/* ...D0 is K */
	move.l		d0,L_SCR1(a6)		/* ...save K temporarily */
	asr.l		#1,d0				/* ...D0 is M1 */
	sub.l		d0,L_SCR1(a6)		/* ...a1 is M */
	addi.w		#0x3FFF,d0			/* ...biased expo. of 2^(M1) */
	move.w		d0,ADJSCALE(a6)		/* ...ADJSCALE := 2^(M1) */
	clr.w		ADJSCALE+2(a6)
	move.l		#0x80000000,ADJSCALE+4(a6)
	clr.l		ADJSCALE+8(a6)
	move.l		L_SCR1(a6),d0		/* ...D0 is M */
	addi.w		#0x3FFF,d0			/* ...biased expo. of 2^(M) */
	bra		EXPCONT1				/* ...go back to Step 3 */

EXP2BIG:
/*
 * --Step 9
 */
	fmove.l		d1,fpcr
	move.l		(a0),d0
	bclr.b		#sign_bit,(a0)		/* ...setox always returns positive */
	tst.l		d0
	blt		t_unfl
	bra		t_ovfl

	.globl	setoxm1d
setoxm1d:
/*
 * --entry point for EXPM1(X), here X is denormalized
 * --Step 0.
 */
	bra		t_extdnrm


	.globl	setoxm1
setoxm1:
/*
 * --entry point for EXPM1(X), here X is finite, non-zero, non-NaN
 */
/*
 * --Step 1.
 * --Step 1.1
 */
	move.l		(a0),d0	 /* ...load part of input X */
	andi.l		#0x7FFF0000,d0		/* ...biased expo. of X */
	cmpi.l		#0x3FFD0000,d0		/* ...1/4 */
	bge.s		EM1CON1	 			/* ...|X| >= 1/4 */
	bra		EM1SM

EM1CON1:
/*
 * --Step 1.3
 * --The case |X| >= 1/4
 */
	move.w		4(a0),d0			/* ...expo. and partial sig. of |X| */
	cmpi.l		#0x4004C215,d0		/* ...70log2 rounded up to 16 bits */
	ble.s		EM1MAIN	 			/* ...1/4 <= |X| <= 70log2 */
	bra		EM1BIG

EM1MAIN:
/*
 * --Step 2.
 * --This is the case:	1/4 <= |X| <= 70 log2.
 */
	fmove.x		(a0),fp0			/* ...load input from (a0) */

	fmove.x		fp0,fp1
	.IFNE GAS
	fmul.s		#0x42B8AA3B,fp0		/* ...64/log2 * X */
	.ELSE
	fmul.s		#92.3324813842773438,fp0		/* ...64/log2 * X */
	.ENDC
	fmovem.x	fp2-fp2/fp3,-(a7)	/* ...save fp2 */
/* 	move.w		#0x3F81,EM1A4		...prefetch in CB mode */
	fmove.l		fp0,d0				/* ...N = int( X * 64/log2 ) */
	lea		EXPTBL.w(pc),a1
	fmove.l		d0,fp0				/* ...convert to floating-format */

	move.l		d0,L_SCR1(a6)		/* ...save N temporarily */
	andi.l		#0x3F,d0			/* ...D0 is J = N mod 64 */
	lsl.l		#4,d0
	adda.l		d0,a1				/* ...address of 2^(J/64) */
	move.l		L_SCR1(a6),d0
	asr.l		#6,d0				/* ...D0 is M */
	move.l		d0,L_SCR1(a6)		/* ...save a copy of M */
/* 	MOVE.W		#0x3FDC,L2		...prefetch L2 in CB mode */

/*
 * --Step 3.
 * --fp1,fp2 saved on the stack. fp0 is N, fp1 is X,
 * --a0 points to 2^(J/64), D0 and a1 both contain M
 */
	fmove.x		fp0,fp2
	.IFNE GAS
	fmul.s		#0xBC317218,fp0		/* ...N * L1, L1 = lead(-log2/64) */
	.ELSE
	fmul.s		#-0.0108304247260093689,fp0		/* ...N * L1, L1 = lead(-log2/64) */
	.ENDC
	fmul.x		L2.w(pc),fp2				/* ...N * L2, L1+L2 = -log2/64 */
	fadd.x		fp1,fp0	 			/* ...X + N*L1 */
	fadd.x		fp2,fp0	 			/* ...fp0 is R, reduced arg. */
/* 	MOVE.W		#0x3FC5,EM1A2		...load EM1A2 in cache */
	addi.w		#0x3FFF,d0			/* ...D0 is biased expo. of 2^M */

/*
 * --Step 4.
 * --WE NOW COMPUTE EXP(R)-1 BY A POLYNOMIAL
 * -- R + R*R*(A1 + R*(A2 + R*(A3 + R*(A4 + R*(A5 + R*A6)))))
 * --TO FULLY UTILIZE THE PIPELINE, WE COMPUTE S = R*R
 * --[R*S*(A2+S*(A4+S*A6))] + [R+S*(A1+S*(A3+S*A5))]
 */
	fmove.x		fp0,fp1
	fmul.x		fp1,fp1				/* ...fp1 IS S = R*R */

	.IFNE GAS
	fmove.s		#0x3950097B,fp2		/* ...fp2 IS a6 */
	.ELSE
	fmove.s		#0.000198399575310759246,fp2		/* ...fp2 IS a6 */
	.ENDC
/* 	MOVE.W		#0,2(a1)	...load 2^(J/64) in cache */

	fmul.x		fp1,fp2				/* ...fp2 IS S*A6 */
	fmove.x		fp1,fp3
	.IFNE GAS
	fmul.s		#0x3AB60B6A,fp3		/* ...fp3 IS S*A5 */
	.ELSE
	fmul.s		#0.00138888997025787830,fp3		/* ...fp3 IS S*A5 */
	.ENDC

	fadd.d		EM1A4.w(pc),fp2			/* ...fp2 IS A4+S*A6 */
	fadd.d		EM1A3.w(pc),fp3			/* ...fp3 IS A3+S*A5 */
	move.w		d0,SC(a6)			/* ...SC is 2^(M) in extended */
	clr.w		SC+2(a6)
	move.l		#0x80000000,SC+4(a6)
	clr.l		SC+8(a6)

	fmul.x		fp1,fp2				/* ...fp2 IS S*(A4+S*A6) */
	move.l		L_SCR1(a6),d0		/* ...D0 is	M */
	neg.w		d0					/* ...D0 is -M */
	fmul.x		fp1,fp3				/* ...fp3 IS S*(A3+S*A5) */
	addi.w		#0x3FFF,d0			/* ...biased expo. of 2^(-M) */
	fadd.d		EM1A2.w(pc),fp2			/* ...fp2 IS A2+S*(A4+S*A6) */
	fadd.s		ONEHALF.w(pc),fp3	/* ...fp3 IS A1+S*(A3+S*A5) */

	fmul.x		fp1,fp2				/* ...fp2 IS S*(A2+S*(A4+S*A6)) */
	ori.w		#0x8000,d0			/* ...signed/expo. of -2^(-M) */
	move.w		d0,ONEBYSC(a6)		/* ...OnebySc is -2^(-M) */
	clr.w		ONEBYSC+2(a6)
	move.l		#0x80000000,ONEBYSC+4(a6)
	clr.l		ONEBYSC+8(a6)
	fmul.x		fp3,fp1				/* ...fp1 IS S*(A1+S*(A3+S*A5)) */
									/* ...fp3 released */

	fmul.x		fp0,fp2				/* ...fp2 IS R*S*(A2+S*(A4+S*A6)) */
	fadd.x		fp1,fp0				/* ...fp0 IS R+S*(A1+S*(A3+S*A5)) */
									/* ...fp1 released */

	fadd.x		fp2,fp0				/* ...fp0 IS EXP(R)-1 */
									/* ...fp2 released */
	fmovem.x	(a7)+,fp2-fp2/fp3	/* ...fp2 restored */

/*
 * --Step 5
 * --Compute 2^(J/64)*p
 */
	fmul.x		(a1),fp0			/* ...2^(J/64)*(Exp(R)-1) */

/*
 * --Step 6
 * --Step 6.1
 */
	move.l		L_SCR1(a6),d0		/* ...retrieve M */
	cmpi.l		#63,d0
	ble.s		MLE63
/*
 * --Step 6.2	M >= 64
 */
	fmove.s		12(a1),fp1			/* ...fp1 is t */
	fadd.x		ONEBYSC(a6),fp1		/* ...fp1 is t+OnebySc */
	fadd.x		fp1,fp0				/* ...p+(t+OnebySc), fp1 released */
	fadd.x		(a1),fp0			/* ...T+(p+(t+OnebySc)) */
	bra.s		EM1SCALE
MLE63:
/*
 * --Step 6.3	M <= 63
 */
	cmpi.l		#-3,d0
	bge.s		MGEN3
MLTN3:
/*
 * --Step 6.4	M <= -4
 */
	fadd.s		12(a1),fp0			/* ...p+t */
	fadd.x		(a1),fp0			/* ...T+(p+t) */
	fadd.x		ONEBYSC(a6),fp0		/* ...OnebySc + (T+(p+t)) */
	bra.s		EM1SCALE
MGEN3:
/*
 * --Step 6.5	-3 <= M <= 63
 */
	fmove.x		(a1)+,fp1			/* ...fp1 is T */
	fadd.s		(a1),fp0			/* ...fp0 is p+t */
	fadd.x		ONEBYSC(a6),fp1		/* ...fp1 is T+OnebySc */
	fadd.x		fp1,fp0				/* ...(T+OnebySc)+(p+t) */

EM1SCALE:
/*
 * --Step 6.6
 */
	fmove.l		d1,fpcr
	fmul.x		SC(a6),fp0

	bra		t_frcinx

EM1SM:
/*
 * --Step 7	|X| < 1/4.
 */
	cmpi.l		#0x3FBE0000,d0		/* ...2^(-65) */
	bge.s		EM1POLY

EM1TINY:
/*
 * --Step 8	|X| < 2^(-65)
 */
	cmpi.l		#0x00330000,d0		/* ...2^(-16312) */
	blt.s		EM12TINY
/*
 * --Step 8.2
 */
	move.l		#0x80010000,SC(a6)	/* ...SC is -2^(-16382) */
	move.l		#0x80000000,SC+4(a6)
	clr.l		SC+8(a6)
	fmove.x		(a0),fp0
	fmove.l		d1,fpcr
	fadd.x		SC(a6),fp0

	bra		t_frcinx

EM12TINY:
/*
 * --Step 8.3
 */
	fmove.x		(a0),fp0
	fmul.d		TWO140.w(pc),fp0
	move.l		#0x80010000,SC(a6)
	move.l		#0x80000000,SC+4(a6)
	clr.l		SC+8(a6)
	fadd.x		SC(a6),fp0
	fmove.l		d1,fpcr
	fmul.d		TWON140.w(pc),fp0

	bra		t_frcinx

EM1POLY:
/*
 * --Step 9	exp(X)-1 by a simple polynomial
 */
	fmove.x		(a0),fp0			/* ...fp0 is X */
	fmul.x		fp0,fp0				/* ...fp0 is S := X*X */
	fmovem.x	fp2-fp2/fp3,-(a7)	/* ...save fp2 */
	.IFNE GAS
	fmove.s		#0x2F30CAA8,fp1		/* ...fp1 is B12 */
	.ELSE
	fmove.s		#1.60791047143504784e-10,fp1		/* ...fp1 is B12 */
	.ENDC
	fmul.x		fp0,fp1				/* ...fp1 is S*B12 */
	.IFNE GAS
	fmove.s		#0x310F8290,fp2		/* ...fp2 is B11 */
	fadd.s		#0x32D73220,fp1		/* ...fp1 is B10+S*B12 */
	.ELSE
	fmove.s		#2.08834549653147405e-09,fp2		/* ...fp2 is B11 */
	fadd.s		#2.50520884037541691e-08,fp1		/* ...fp1 is B10+S*B12 */
	.ENDC

	fmul.x		fp0,fp2				/* ...fp2 is S*B11 */
	fmul.x		fp0,fp1				/* ...fp1 is S*(B10 + ... */

	.IFNE GAS
	fadd.s		#0x3493F281,fp2		/* ...fp2 is B9+S*... */
	.ELSE
	fadd.s		#2.75573285080099595e-07,fp2		/* ...fp2 is B9+S*... */
	.ENDC
	fadd.d		EM1B8.w(pc),fp1		/* ...fp1 is B8+S*... */

	fmul.x		fp0,fp2				/* ...fp2 is S*(B9+... */
	fmul.x		fp0,fp1				/* ...fp1 is S*(B8+... */

	fadd.d		EM1B7.w(pc),fp2		/* ...fp2 is B7+S*... */
	fadd.d		EM1B6.w(pc),fp1		/* ...fp1 is B6+S*... */

	fmul.x		fp0,fp2				/* ...fp2 is S*(B7+... */
	fmul.x		fp0,fp1				/* ...fp1 is S*(B6+... */

	fadd.d		EM1B5.w(pc),fp2		/* ...fp2 is B5+S*... */
	fadd.d		EM1B4.w(pc),fp1		/* ...fp1 is B4+S*... */

	fmul.x		fp0,fp2				/* ...fp2 is S*(B5+... */
	fmul.x		fp0,fp1				/* ...fp1 is S*(B4+... */

	fadd.d		EM1B3.w(pc),fp2		/* ...fp2 is B3+S*... */
	fadd.x		EM1B2.w(pc),fp1		/* ...fp1 is B2+S*... */

	fmul.x		fp0,fp2				/* ...fp2 is S*(B3+... */
	fmul.x		fp0,fp1				/* ...fp1 is S*(B2+... */

	fmul.x		fp0,fp2				/* ...fp2 is S*S*(B3+...) */
	fmul.x		(a0),fp1			/* ...fp1 is X*S*(B2... */

	fmul.s		ONEHALF.w(pc),fp0		/* ...fp0 is S*B1 */
	fadd.x		fp2,fp1				/* ...fp1 is Q */
									/* ...fp2 released */

	fmovem.x	(a7)+,fp2-fp2/fp3	/* ...fp2 restored */

	fadd.x		fp1,fp0				/* ...fp0 is S*B1+Q */
									/* ...fp1 released */

	fmove.l		d1,fpcr
	fadd.x		(a0),fp0

	bra		t_frcinx

EM1BIG:
/*
 * --Step 10	|X| > 70 log2
 */
	move.l		(a0),d0
	tst.l		d0
	bgt		EXPC1
/*
 * --Step 10.2
 */
	.IFNE GAS
	fmove.s		#0xBF800000,fp0		/* ...fp0 is -1 */
	fmove.l		d1,fpcr
	fadd.s		#0x00800000,fp0		/* ...-1 + 2^(-126) */
	.ELSE
	fmove.s		#-1.0,fp0			/* ...fp0 is -1 */
	fmove.l		d1,fpcr
	fadd.s		#1.17549435082228751e-38,fp0		/* ...-1 + 2^(-126) */
	.ENDC

	bra		t_frcinx
