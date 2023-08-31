#ifdef __mcoldfire__

#include "mint/asmdefs.h"

	.data

	.globl	SYM(patch_memset_purec)
	.globl  SYM(call_stack_func)

SYM(patch_memset_purec):

	lea -28(sp),sp
	movem.l d0-d2/a0-a2/a5,(sp)
	move.l 32(sp),a5                                    // BASEPAGE
	move.l 0xC(a5),d1                                   // p_tlen
	lsr.l #1,d1
	lea 256(a5),a0                                      // text
	lea memset_purec(pc),a1
.loop_search_memset:
		move.l (a0),d0
		cmp.l (a1),d0
		bne.s .next_search_memset
		addq.l #4,a1
		lea 4(a0),a2
		moveq #(12*4)-1,d2
.loop_search_memset_2:
			move.l (a2)+,d0
			cmp.l (a1)+,d0
			bne.s .bad_cmp_memset
		subq.l #1,d2
		bgt.s .loop_search_memset_2
		// patch for incompatible instructions
		lea 0x14(a0),a0                        // move.b D0,-(SP) move.w (SP)+,D2
		move.l #0x1400E18A,(a0)                // => move.b D0,D2 lsl.l #8,D2 
		bra.s .end_pexec
.bad_cmp_memset:
		lea memset_purec(pc),a1	
.next_search_memset:
		addq.l #2,a0
	subq.l #1,d1
	bgt.s .loop_search_memset
.end_pexec:
	movem.l (sp),d0-d2/a0-a2/a5
	lea 28(sp),sp
	rts

|-------------------------------------------------------------------------------------
| call_stack_func(AESCALL func, AESPB* pb, my_aes_CLIENT* c, void* stack);
|

SYM(call_stack_func):
	move.l 16(sp),a0	| new stack area is now pointed by A0
	move.l sp, -(a0)
	move.l 12(sp), -(a0)
	move.l 8(sp), -(a0)
	move.l #ret_stack_func, -(a0)
	move.l 4(sp),a1
	move.l a0,sp
	jmp (a1)

ret_stack_func:
	move.l 8(sp),a1
	move.l a1,sp
	rts

	
memset_purec:
	dc.l 0x2F08D1C1,0x24080802,0x00006708,0x53816500
	dc.l 0x00AC1100,0x1F00341F,0x14003002,0x48423400
	dc.l 0x2001E088,0xE4886778,0x48E75F3E,0x22022602
	dc.l 0x28022A02,0x2C022E02,0x22422442,0x26422842
	dc.l 0x2A422C42,0x48E07F7E,0x48E07F7E,0x48E07F7E
	dc.l 0x48E07F7E,0x48E07F7E,0x48E07F7E,0x48E07F7E
	dc.l 0x48E07F7E,0x48E07F7E,0x48E07F7E,0x48E07F7E
	dc.l 0x48E07F7E,0x48E07F7E,0x48E07F7E,0x48E07F7E
	dc.l 0x48E07F7E,0x48E07F7E,0x48E07F7E,0x48E07F7E
	dc.l 0x48E07F60,0x538066AC,0x4CDF7CFA,0xC27C03FF
	dc.l 0x3001E448,0x67085340,0x210251C8,0xFFFCC27C
	dc.l 0x00036708,0x53411102,0x51C9FFFC,0x205F4E75
#endif
