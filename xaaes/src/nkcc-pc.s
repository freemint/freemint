****************************************************************************
*
*	Reduzierte NKCC fÅr die CF-Lib/PureC
*	Version fÅr PureC
*
****************************************************************************

****************************************************************************
*						  ASSEMBLER CONTROL SECTION 					   *
****************************************************************************

VERSION 		=	$0294			; NKCC's version #

NKFf_FUNC		=	$8000
NKFf_RESVD		=	$4000
NKFf_NUM		=	$2000
NKFf_CAPS		=	$1000
NKFf_ALT		=	$0800
NKFf_CTRL		=	$0400
NKFf_SHIFT		=	$0300			; both shift keys

NKFb_FUNC		=	15				; function
NKFb_RESVD		=	14				; reserved, ignore it!
NKFb_NUM		=	13				; numeric pad
NKFb_CAPS		=	12				; CapsLock
NKFb_ALT		=	11				; Alternate
NKFb_CTRL		=	10				; Control
NKFb_LSH		=	9				; left Shift key
NKFb_RSH		=	8				; right Shift key

NK_UP			=	$01 			; cursor up
NK_DOWN 		=	$02 			; cursor down
NK_RIGHT		=	$03 			; cursor right
NK_LEFT 		=	$04 			; cursor left
NK_M_PGUP		=	$05 			; Mac: page up
NK_M_PGDOWN 	=	$06 			; Mac: page down
NK_M_END		=	$07 			; Mac: end
NK_INS			=	$0b 			; Insert
NK_CLRHOME		=	$0c 			; Clr/Home
NK_HELP 		=	$0e 			; Help
NK_UNDO 		=	$0f 			; Undo
NK_M_F11		=	$1a 			; Mac: function key #11
NK_M_F12		=	$1c 			; Mac: function key #12
NK_M_F14		=	$1d 			; Mac: function key #14
NK_ENTER		=	$0a 			; Enter
NK_DEL			=	$1f 			; Delete

****************************************************************************
*								   EXPORT								   *
****************************************************************************

			   .globl	nkc_init			 ; init NKCC
			   .globl	nkc_tos2n			 ; TOS key code converter
			   .globl	nkc_n2tos			 ; NKC to TOS key code converter
               .globl   nkc_toupper          ; convert character to upper case
               .globl   nkc_tolower          ; convert character to lower case

****************************************************************************
*							 LOCAL TEXT SECTION 						   *
****************************************************************************

			   .text

****************************************************************************
*
*  nk_findscan: find scan code
*
****************************************************************************

nk_findscan:   btst.l	#NKFb_NUM,d0		 ; on numeric keypad?
			   beq.s	.search 			 ; no ->

			   move 	#$4a,d1 			 ; yes: try all numeric keypad
			   cmp.b	(a0,d1),d0			 ;	scan codes first
			   beq.s	.found				 ; it matches ->

			   move 	#$4e,d1
			   cmp.b	(a0,d1),d0
			   beq.s	.found

			   move 	#$63,d1 			 ; block starts at $63

.numsearch:    cmp.b	(a0,d1),d0			 ; match?
			   beq.s	.found				 ; yes ->

			   addq 	#1,d1				 ; next scan code
			   cmp		#$73,d1 			 ; block end at $72
			   blo.s	.numsearch			 ; continue search ->

.search:	   move 	#1,d1				 ; start with first valid scan code

.mainsearch:   cmp.b	(a0,d1),d0			 ; match?
			   beq.s	.found				 ; yes ->

			   addq.b	#1,d1				 ; next scan code
			   cmp.b	#$78,d1 			 ; $78 = last valid scan code
			   blo.s	.mainsearch 		 ; continue search ->

			   moveq.l	#0,d1				 ; not found
			   rts

.found: 	   tst		d1					 ; found; set CCR
			   rts


****************************************************************************
*							 GLOBAL TEXT SECTION						   *
****************************************************************************

			   .text

****************************************************************************
*
*  nkc_init: initialize NKCC
*
****************************************************************************

nkc_init:	   

*------------- fetch addresses of TOS' key scan code translation tables

			   moveq.l	#-1,d0				 ; the function is also used to
			   move.l	d0,-(sp)			 ; change the addresses; values
			   move.l	d0,-(sp)			 ; of -1 as new addresses tell
			   move.l	d0,-(sp)			 ; XBIOS not to change them
			   move 	#$10,-(sp)			 ; Keytbl
			   trap 	#14 				 ; XBIOS
			   lea		$e(sp),sp			 ; clean stack

			   move.l	d0,a0				 ; ^key table structure
			   move.l	(a0)+,pkey_unshift	 ; get ^unshifted table
			   move.l	(a0)+,pkey_shift	 ; get ^shifted table
			   move.l	(a0),pkey_caps		 ; get ^CapsLock table

*------------- exit

.exit:		   move 	#VERSION,d0 		 ; load version #
			   rts							 ; bye

****************************************************************************
*
*  nkc_tconv: TOS key code converter
*
****************************************************************************

nkc_tos2n:	   movem.l	d3/d4,-(sp) 		 ; save registers

*------------- separate TOS key code

			   move.l	d0,d1				 ; TOS key code
			   swap 	d1					 ; .W = scan code and flags
			   move 	d1,d2				 ; copy
			   move 	#$ff,d3 			 ; and-mask
			   and		d3,d0				 ; .B = ASCII code
			   and		d3,d1				 ; .B = scan code
			   beq		.tos306 			 ; scancode=zero (key code created
											 ;	by ASCII input of TOS 3.06)? ->
			   and		#$1f00,d2			 ; .W = key flags (in high byte)

*------------- decide which translation table to use

			   move 	d2,d3				 ; key flags
			   and		#NKFf_SHIFT,d3		 ; isolate bits for shift keys
			   beq.s	.ktab1				 ; shift key pressed? no->

			   move.l	pkey_shift,a0		 ; yes: use shift table
			   bra.s	.ktab3				 ; ->

.ktab1: 	   btst.l	#NKFb_CAPS,d2		 ; CapsLock?
			   beq.s	.ktab2				 ; no->

			   move.l	pkey_caps,a0		 ; yes: use CapsLock table
			   bra.s	.ktab3				 ; ->

.ktab2: 	   move.l	pkey_unshift,a0 	 ; use unshifted table

*------------- check if scan code is out of range
*
* Illegal scancodes can be used to produce 'macro key codes'. Their format is:
*
* - the scancode must be $84 or larger (should be $ff to work properly with old
*	versions of Mag!x)
* - the ASCII code must be in the range $20...$ff (values below are set to $20
*	by NKCC)
* - Alternate and Control are not used for the normalized key code. However,
*	if at least one of them is non-zero, then the numeric keypad flag will be
*	set in the resulting key code.
*

.ktab3: 	   cmp.b	#$84,d1 			 ; illegal scan code?
			   blo.s	.ktab4				 ; no ->

			   move 	d2,d1				 ; flags
			   and		#NKFf_ALT|NKFf_CTRL,d1	; Alternate or Control?
			   beq.s	.special			 ; no ->

			   or		#NKFf_NUM,d0		 ; yes: set numeric keypad flag
			   and		#NKFf_CAPS|NKFf_SHIFT,d2   ; mask off both flags

.special:	   or		d2,d0				 ; combine with ASCII code
			   or		#NKFf_FUNC|NKFf_RESVD,d0   ; set function and resvd
			   cmp.b	#$20,d0 			 ; ASCII code in range?
			   bhs		.exit				 ; yes ->

			   move.b	#$20,d0 			 ; no: use minimum
			   bra		.exit				 ; ->

*------------- check if Alternate + number: they have simulated scan codes

.ktab4: 	   cmp.b	#$78,d1 			 ; scan code of Alt + number?
			   blo.s	.scan1				 ; no->

			   sub.b	#$76,d1 			 ; yes: calculate REAL scan code
			   move.b	(a0,d1),d0			 ; fetch ASCII code
			   or		#NKFf_ALT,d2		 ; set Alternate flag
			   bra		.cat_codes			 ; -> add flag byte and exit

*------------- check if exception scan code from cursor keypad

.scan1: 	   lea		xscantab,a1 		 ; ^exception scan code table

.search_scan:  move 	(a1)+,d3			 ; NKC and scan code
			   bmi.s	.tabend 			 ; <0? end of table reached ->

			   cmp.b	d1,d3				 ; scan code found?
			   bne.s	.search_scan		 ; no: continue search ->

			   lsr		#8,d3				 ; .B = NKC
			   moveq.l	#0,d0				 ; mark: key code found
			   bra.s	.scan2				 ; ->

.tabend:	   moveq.l	#0,d3				 ; no NKC found yet

*------------- check if rubbish ASCII code and erase it, if so

.scan2: 	   move.b	(a0,d1),d4			 ; ASCII code from translation table
			   cmp.b	#32,d0				 ; ASCII returned by TOS < 32?
			   bhs.s	.scan3				 ; no -> can't be rubbish

			   cmp.b	d4,d0				 ; yes: compare with table entry
			   beq.s	.scan3				 ; equal: that's ok ->

			   moveq.l	#0,d0				 ; not equal: rubbish! clear it

*------------- check if ASCII code could only be produced via Alternate key
*			   combination

.scan3: 	   tst.b	d0					 ; ASCII code valid?
			   beq.s	.scan4				 ; no ->

			   cmp.b	d4,d0				 ; compare with table entry
			   beq.s	.scan4				 ; equal: normal key ->

			   and		#!NKFf_ALT,d2		 ; no: clear Alternate flag

*------------- check if ASCII code found yet, and set it, if not

.scan4: 	   tst.b	d0					 ; found?
			   bne.s	.scan5				 ; yes ->

			   move.b	d3,d0				 ; no: use code from exception table
			   bne.s	.scan5				 ; now valid? yes ->

			   move.b	d4,d0				 ; no: use code from transl. table

*------------- check special case: delete key

.scan5: 	   cmp.b	#127,d0 			 ; ASCII code of Delete?
			   bne.s	.scan6				 ; no ->

			   move.b	#NK_DEL,d0			 ; yes: set according NKC

*------------- check if key is on numeric keypad (via scan code)

.scan6: 	   cmp.b	#$4a,d1 			 ; numeric pad scan code range?
			   beq.s	.numeric			 ; yes ->

			   cmp.b	#$4e,d1
			   beq.s	.numeric			 ; yes ->

			   cmp.b	#$63,d1
			   blo.s	.scan7				 ; no ->

			   cmp.b	#$72,d1
			   bhi.s	.scan7				 ; no ->

.numeric:	   or		#NKFf_NUM,d2		 ; yes: set numeric bit

*------------- check if "function key" and set bit accordingly

.scan7: 	   cmp.b	#32,d0				 ; ASCII code less than 32?
			   bhs.s	.scan8				 ; no ->

			   or		#NKFf_FUNC,d2		 ; yes: set function bit

*------------- check special case: Return or Enter key

			   cmp.b	#13,d0				 ; Return or Enter key?
			   bne.s	.scan8				 ; no ->

			   btst.l	#NKFb_NUM,d2		 ; yes: from the numeric pad?
			   beq.s	.scan8				 ; no -> it's Return, keep code

			   moveq.l	#NK_ENTER,d0		 ; yes: it's Enter; new code

*------------- check if function key (F1-F10) via scan code

.scan8: 	   cmp.b	#$54,d1 			 ; shift + function key?
			   blo.s	.scan9				 ; no ->

			   cmp.b	#$5d,d1
			   bhi.s	.scan9				 ; no ->

			   sub.b	#$54-$3b,d1 		 ; yes: scan code for unshifted key
			   move 	d2,d3				 ; shift flags
			   and		#NKFf_SHIFT,d3		 ; any shift key flag set?
			   bne.s	.scan9				 ; yes ->
			   or		#NKFf_SHIFT,d2		 ; no: set both flags

.scan9: 	   cmp.b	#$3b,d1 			 ; (unshifted) function key?
			   blo.s	.cat_codes			 ; no ->

			   cmp.b	#$44,d1
			   bhi.s	.cat_codes			 ; no ->

			   move.b	d1,d0				 ; yes: calc NKC
			   sub.b	#$2b,d0

*------------- final flag handling; mix key code (low byte) and flag byte

.cat_codes:    move.l	pkey_shift,a0		 ; ^shifted table
			   move.b	(a0,d1),d3			 ; get shifted ASCII code
			   or		d2,d0				 ; mix flags with key code
			   bmi.s	.scan10 			 ; result is "function key"? ->

			   and		#NKFf_CTRL+NKFf_ALT,d2	; Control or Alternate pressed?
			   bne.s	.scan11 			 ; yes ->

.scan10:	   move.l	pkey_unshift,a0 	 ; ^unshifted table
			   cmp.b	(a0,d1),d3			 ; shifted ASCII = unshifted ASCII?
			   beq.s	.scan12 			 ; yes ->

			   bra.s	.exit				 ; no ->

.scan11:	   or		#NKFf_FUNC,d0		 ; Alt/Ctrl + char: set function bit
			   move.l	pkey_caps,a0		 ; ^CapsLock table
			   cmp.b	(a0,d1),d3			 ; shifted ASCII = CapsLocked ASCII?
			   bne.s	.exit				 ; no ->

			   move.b	d3,d0				 ; yes: use shifted ASCII code

.scan12:	   or		#NKFf_RESVD,d0		 ; yes: nkc_cmp() has to check

*------------- restore registers and exit

.exit:		   tst		d0					 ; set CCR
			   movem.l	(sp)+,d3/d4 		 ; restore registers
			   rts							 ; bye

*------------- special handling for key codes created by TOS' 3.06 ASCII input

.tos306:	   and		#NKFf_CAPS,d2		 ; isolate CapsLock flag
			   or		d2,d0				 ; merge with ASCII code
			   movem.l	(sp)+,d3/d4 		 ; restore registers
			   rts							 ; bye

****************************************************************************
*
*  nkc_n2tos: convert normalized key codes back to TOS format
*
****************************************************************************

nkc_n2tos:	   move 	d0,d1				 ; normalized key code
			   and		#NKFf_FUNC|NKFf_ALT|NKFf_CTRL,d1 ; isolate flags
			   cmp		#NKFf_FUNC,d1		 ; only function flag set?
			   bne.s	.ktab0				 ; no ->

			   cmp.b	#$20,d0 			 ; ASCII code >= $20?
			   blo.s	.ktab0				 ; no ->

*------------- macro key

			   move 	d0,d1				 ; keep normalized key code
			   and.l	#NKFf_CAPS|NKFf_SHIFT,d0   ; isolate usable flags
			   btst.l	#NKFb_NUM,d1		 ; numeric keypad flag set?
			   beq.s	.mackey 			 ; no ->

			   or		#NKFf_ALT|NKFf_CTRL,d0	; yes: set Alternate + Control

.mackey:	   or.b 	#$ff,d0 			 ; scan code always $ff
			   swap 	d0					 ; flags and scan code in upper word
			   move.b	d1,d0				 ; ASCII code
			   bra		.exit				 ; ->

*------------- select system key table to use

.ktab0: 	   move 	d0,d1				 ; normalized key code
			   and		#NKFf_SHIFT,d1		 ; isolate bits for shift keys
			   beq.s	.ktab1				 ; shift key pressed? no->

			   lea		n_to_scan_s,a1		 ; ^default translation table
			   move.l	pkey_shift,a0		 ; yes: use shift table
			   bra.s	.ktab3				 ; ->

.ktab1: 	   lea		n_to_scan_u,a1		 ; ^unshifted translation table
			   btst.l	#NKFb_CAPS,d0		 ; CapsLock?
			   beq.s	.ktab2				 ; no->

			   move.l	pkey_caps,a0		 ; yes: use CapsLock table
			   bra.s	.ktab3				 ; ->

.ktab2: 	   move.l	pkey_unshift,a0 	 ; use unshifted table

*------------- handling for ASCII codes >= 32

.ktab3: 	   cmp.b	#32,d0				 ; ASCII code < 32?
			   blo.s	.lowascii			 ; yes ->

			   bsr		nk_findscan 		 ; find scan code
			   bne.s	.found				 ; found ->

			   btst.l	#NKFb_FUNC,d0		 ; function flag set?
			   beq.s	.notfound			 ; no ->

			   move.l	a0,d1				 ; save a0
			   lea		tolower,a0			 ; ^upper->lower case table
			   moveq.l	#0,d2				 ; clear for word operation
			   move.b	d0,d2				 ; ASCII code
			   move.b	(a0,d2),d0			 ; get lowercased ASCII code
			   move.l	d1,a0				 ; restore a0
			   bsr		nk_findscan 		 ; try to find scan code again
			   bne.s	.found				 ; found ->

*------------- unknown source: treat key code as it was entered using the
*			   TOS 3.06 direct ASCII input

.notfound:	   moveq.l	#0,d1				 ; not found: clear for word op.
			   move.b	d0,d1				 ; unchanged ASCII code
			   and		#$1f00,d0			 ; keep shift flags only
			   swap 	d0					 ; -> high word (scan code = 0)
			   move 	d1,d0				 ; low word: ASCII code
			   bra		.exit				 ; ->

*------------- handling for ASCII codes < 32

.lowascii:	   btst.l	#NKFb_FUNC,d0		 ; function key?
			   bne.s	.func				 ; yes ->

			   and		#$10ff,d0			 ; clear all flags except CapsLock
			   bra.s	.notfound			 ; ->

.func:		   moveq.l	#0,d1				 ; clear for word operation
			   move.b	d0,d1				 ; ASCII code (0...$1f)
			   move 	d1,d2				 ; copy
			   move.b	(a1,d1),d1			 ; get scan code
			   bne.s	.getascii			 ; valid? ->

			   moveq	#0,d0				 ; invalid key code!! return 0
			   bra		.exit				 ; ->

.getascii:	   lea		n_to_scan_u,a1		 ; ^unshifted translation table
			   move.b	(a1,d2),d2			 ; get scan code from unshifted tab.
			   move.b	(a0,d2),d0			 ; get ASCII from system's table

* register contents:
*
* d0.b		   ASCII code
* d1.b		   scan code
* d0.hb 	   NKCC flags
*

.found: 	   move 	d0,d2				 ; flags and ASCII code
			   and		#$1f00,d0			 ; isolate shift flags
			   move.b	d1,d0				 ; merge with scan code
			   swap 	d0					 ; -> high byte
			   clr		d0					 ; erase low word
			   move.b	d2,d0				 ; restore ASCII code

*------------- handling for Control key flag

			   btst.l	#NKFb_CTRL,d2		 ; control key flag set?
			   beq.s	.alternate			 ; no ->

			   cmp.b	#$4b,d1 			 ; scan code = "cursor left"?
			   bne.s	.scanchk2			 ; no ->

			   add.l	#$280000,d0 		 ; change scan code to $73
			   clr.b	d0					 ; erase ASCII code
			   bra.s	.exit				 ; ->

.scanchk2:	   cmp.b	#$4d,d1 			 ; scan code = "cursor right"?
			   bne.s	.scanchk3			 ; no ->

			   add.l	#$270000,d0 		 ; change scan code to $74
			   clr.b	d0					 ; erase ASCII code
			   bra.s	.exit				 ; ->

.scanchk3:	   cmp.b	#$47,d1 			 ; scan code = "ClrHome"?
			   bne.s	.ascchk 			 ; no ->

			   add.l	#$300000,d0 		 ; change scan code to $77
; keep ASCII code in this case! What a mess...
			   bra.s	.exit				 ; ->

.ascchk:	   lea		asc_trans,a0		 ; ^ASCII translation table

.ascloop:	   move 	(a0)+,d1			 ; get next entry
			   beq.s	.noctrlasc			 ; end of table ->

			   cmp.b	d0,d1				 ; ASCII code found?
			   bne.s	.ascloop			 ; no -> continue search

			   lsr		#8,d1				 ; yes: get translated code
			   move.b	d1,d0				 ; use it
			   bra.s	.exit				 ; ->

.noctrlasc:    and.b	#$1f,d0 			 ; mask off upper 3 bits
			   bra.s	.exit				 ; ->

*------------- handling for Alternate key flag

.alternate:    btst.l	#NKFb_ALT,d2		 ; alternate key flag set?
			   beq.s	.exit				 ; no ->

			   cmp.b	#2,d1				 ; top row on main keyboard?
			   blo.s	.alphachk			 ; no ->

			   cmp.b	#$d,d1
			   bhi.s	.alphachk			 ; no ->

			   add.l	#$760000,d0 		 ; yes: change scan code
			   clr.b	d0					 ; and erase ASCII code
			   bra.s	.exit				 ; ->

.alphachk:	   cmp.b	#'A',d0 			 ; alpha-characters?
			   blo.s	.exit				 ; no ->

			   cmp.b	#'z',d0
			   bhi.s	.exit				 ; no ->

			   cmp.b	#'Z',d0
			   bls.s	.ascii0 			 ; yes ->

			   cmp.b	#'a',d0
			   blo.s	.exit				 ; no ->

.ascii0:	   clr.b	d0					 ; alpha-character: clear ASCII code

.exit:		   tst.l	d0					 ; set CCR
			   rts							 ; bye

****************************************************************************
*
*  nkc_toupper: convert character to upper case
*
****************************************************************************

nkc_toupper:   lea      toupper,a0           ; ^upper case translation table
               and      #$ff,d0              ; high byte = 0 for word operation
               move.b   (a0,d0),d0           ; convert
               rts                           ; bye


****************************************************************************
*
*  nkc_tolower: convert character to lower case
*
****************************************************************************

nkc_tolower:   lea      tolower,a0           ; ^lower case translation table
               and      #$ff,d0              ; high byte = 0 for word operation
               move.b   (a0,d0),d0           ; convert
               rts                           ; bye


****************************************************************************
*							 LOCAL DATA SECTION 						   *
****************************************************************************

			   .data

*  exception scan code table for cursor block keys
*
*  first entry.B:  NKCC key code
*  second entry.B: scan code returned by TOS
*
*  the table is terminated with both entries -1

xscantab:	   .dc.b	NK_UP		,  $48	 ; cursor up
			   .dc.b	NK_DOWN 	,  $50	 ; cursor down
			   .dc.b	NK_LEFT 	,  $4b	 ; cursor left
			   .dc.b	NK_LEFT 	,  $73	 ; Control cursor left
			   .dc.b	NK_RIGHT	,  $4d	 ; cursor right
			   .dc.b	NK_RIGHT	,  $74	 ; Control cursor right
			   .dc.b	NK_M_PGUP	,  $49	 ; Mac: page up
			   .dc.b	NK_M_PGDOWN ,  $51	 ; Mac: page down
			   .dc.b	NK_M_END	,  $4f	 ; Mac: end
			   .dc.b	NK_INS		,  $52	 ; Insert
			   .dc.b	NK_CLRHOME	,  $47	 ; ClrHome
			   .dc.b	NK_CLRHOME	,  $77	 ; Control ClrHome
			   .dc.b	NK_HELP 	,  $62	 ; Help
			   .dc.b	NK_UNDO 	,  $61	 ; Undo
			   .dc.b	NK_M_F11	,  $45	 ; Mac: F11
			   .dc.b	NK_M_F12	,  $46	 ; Mac: F12
			   .dc.b	NK_M_F14	,  $37	 ; Mac: F14
			   .dc.w	-1


*  lower case to upper case conversion table
*  (array of 256 unsigned bytes)

toupper:
               .dc.b    $00,$01,$02,$03,$04,$05,$06,$07
               .dc.b    $08,$09,$0a,$0b,$0c,$0d,$0e,$0f
               .dc.b    $10,$11,$12,$13,$14,$15,$16,$17
               .dc.b    $18,$19,$1a,$1b,$1c,$1d,$1e,$1f
               .dc.b    " !",$22,"#$%&",$27,"()*+,-./0123456789:;<=>?"
               .dc.b    "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[",$5c,"]^_"
               .dc.b    "`ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}~"
               .dc.b    "ÄöêÉé∂èÄàâäãåçéèêííìôïñóòôöõúùûü"
               .dc.b    "†°¢£••¶ß®©™´¨≠ÆØ∑∏≤≤µµ∂∑∏π∫ªºΩæø"
               .dc.b    "¿¡¬√ƒ≈∆«»… ÀÃÕŒœ–—“”‘’÷◊ÿŸ⁄€‹›ﬁﬂ"
               .dc.b    "‡·‚„‰ÂÊÁËÈÍÎÏÌÓÔÒÚÛÙıˆ˜¯˘˙˚¸˝˛ˇ"


*  upper case to lower case conversion table
*  (array of 256 unsigned bytes)

tolower:
               .dc.b    $00,$01,$02,$03,$04,$05,$06,$07
               .dc.b    $08,$09,$0a,$0b,$0c,$0d,$0e,$0f
               .dc.b    $10,$11,$12,$13,$14,$15,$16,$17
               .dc.b    $18,$19,$1a,$1b,$1c,$1d,$1e,$1f
               .dc.b    " !",$22,"#$%&",$27,"()*+,-./0123456789:;<=>?"
               .dc.b    "@abcdefghijklmnopqrstuvwxyz[",$5c,"]^_"
               .dc.b    "`abcdefghijklmnopqrstuvwxyz{|}~"
               .dc.b    "áÅÇÉÑÖÜáàâäãåçÑÜÇëëìîïñóòîÅõúùûü"
               .dc.b    "†°¢£§§¶ß®©™´¨≠ÆØ∞±≥≥¥¥Ö∞±π∫ªºΩæø"
               .dc.b    "¿¡¬√ƒ≈∆«»… ÀÃÕŒœ–—“”‘’÷◊ÿŸ⁄€‹›ﬁﬂ"
               .dc.b    "‡·‚„‰ÂÊÁËÈÍÎÏÌÓÔÒÚÛÙıˆ˜¯˘˙˚¸˝˛ˇ"


*  ASCII code translation table for Control key
*
*  first entry.B:  modified ASCII code returned by TOS
*  second entry.B: original ASCII code as stored in key table
*
*  The table is terminated with both entries 0

asc_trans:	   .dc.b	0,'2'				 ; Control '2' becomes ASCII 0
			   .dc.b	$1e,'6' 			 ; Control '6' becomes ASCII $1e
			   .dc.b	$1f,'-' 			 ; Control '-' becomes ASCII $1f
			   .dc.b	$a,$d				 ; Control Return/Enter: $d -> $a
			   .dc.w	0					 ; terminator


*  normalized key code -> scan code translation table
*  for unshifted key codes
*  indexed by function code (NK_...)

n_to_scan_u:   .dc.b	$00 				 ; invalid key code
			   .dc.b	$48 				 ; cursor up
			   .dc.b	$50 				 ; cursor down
			   .dc.b	$4d 				 ; cursor right
			   .dc.b	$4b 				 ; cursor left
			   .dc.b	$49 				 ; Mac: page up
			   .dc.b	$51 				 ; Mac: page down
			   .dc.b	$4f 				 ; Mac: end
			   .dc.b	$0e 				 ; Backspace
			   .dc.b	$0f 				 ; Tab
			   .dc.b	$72 				 ; Enter
			   .dc.b	$52 				 ; Insert
			   .dc.b	$47 				 ; ClrHome
			   .dc.b	$1c 				 ; Return
			   .dc.b	$62 				 ; Help
			   .dc.b	$61 				 ; Undo
			   .dc.b	$3b 				 ; function key #1
			   .dc.b	$3c 				 ; function key #2
			   .dc.b	$3d 				 ; function key #3
			   .dc.b	$3e 				 ; function key #4
			   .dc.b	$3f 				 ; function key #5
			   .dc.b	$40 				 ; function key #6
			   .dc.b	$41 				 ; function key #7
			   .dc.b	$42 				 ; function key #8
			   .dc.b	$43 				 ; function key #9
			   .dc.b	$44 				 ; function key #10
			   .dc.b	$45 				 ; Mac: F11
			   .dc.b	$01 				 ; Esc
			   .dc.b	$46 				 ; Mac: F12
			   .dc.b	$37 				 ; Mac: F14
			   .dc.b	$00 				 ; reserved!
			   .dc.b	$53 				 ; Delete

*  normalized key code -> scan code translation table
*  for shifted key codes
*  indexed by function code (NK_...)

n_to_scan_s:   .dc.b	$00 				 ; invalid key code
			   .dc.b	$48 				 ; cursor up
			   .dc.b	$50 				 ; cursor down
			   .dc.b	$4d 				 ; cursor right
			   .dc.b	$4b 				 ; cursor left
			   .dc.b	$49 				 ; Mac: page up
			   .dc.b	$51 				 ; Mac: page down
			   .dc.b	$4f 				 ; Mac: end
			   .dc.b	$0e 				 ; Backspace
			   .dc.b	$0f 				 ; Tab
			   .dc.b	$72 				 ; Enter
			   .dc.b	$52 				 ; Insert
			   .dc.b	$47 				 ; ClrHome
			   .dc.b	$1c 				 ; Return
			   .dc.b	$62 				 ; Help
			   .dc.b	$61 				 ; Undo
			   .dc.b	$54 				 ; function key #1
			   .dc.b	$55 				 ; function key #2
			   .dc.b	$56 				 ; function key #3
			   .dc.b	$57 				 ; function key #4
			   .dc.b	$58 				 ; function key #5
			   .dc.b	$59 				 ; function key #6
			   .dc.b	$5a 				 ; function key #7
			   .dc.b	$5b 				 ; function key #8
			   .dc.b	$5c 				 ; function key #9
			   .dc.b	$5d 				 ; function key #10
			   .dc.b	$45 				 ; Mac: F11
			   .dc.b	$01 				 ; Esc
			   .dc.b	$46 				 ; Mac: F12
			   .dc.b	$37 				 ; Mac: F14
			   .dc.b	$00 				 ; reserved!
			   .dc.b	$53 				 ; Delete

****************************************************************************
*							  LOCAL BSS SECTION 						   *
****************************************************************************

			   .bss

pkey_unshift:  .ds.l	1					 ; ^unshifted key table
pkey_shift:    .ds.l	1					 ; ^shifted key table
pkey_caps:	   .ds.l	1					 ; ^CapsLock table

			   .even

* End Of File
