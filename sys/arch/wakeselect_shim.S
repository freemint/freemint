/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution.
 *
 * wakeselect_shim(): the kerinfo->wakeselect entry point.
 * Runs wakeselect() on a private kernel stack with interrupts masked.
 *
 * kerinfo->wakeselect is called by third-party interrupt handlers, some
 * of which run on very small private stacks.  The critical case is AES
 * 4.1 (GEM.SYS): its mouse/button interrupt code runs on a static
 * 92-byte stack ("gstack" in the AES's INTERRUP.S) and calls this slot
 * via forkq() -> mt_wakeup().  The chain wakeselect->rm_q/add_q reaches
 * the very edge of those 92 bytes, and any extra stack depth from
 * changed kernel codegen overwrites the AES tick-engine variables
 * (TICKS, CMP_TICK, NUM_TICK, tiksav) located directly below that
 * buffer.  A corrupted CMP_TICK stops all AES timer event delivery,
 * which leaves the AES screen manager permanently blocked in
 * evnt_multi()/evnt_timer() -- issue #384.  (This is also why the bug
 * appeared with the gcc 2.8.1 -> 2.95.2 switch in 1.15.7b: add_q grew
 * by exactly one saved register.)  b_click() only takes this deep path
 * for button transitions that fall outside the AES double-click
 * window, which is why only fast, irregular clicking triggered it; see
 * the commit message for the full trigger analysis.
 *
 * Switching to a kernel-owned stack makes the excursion below the
 * caller's stack pointer tiny and constant, independent of compiler
 * and kernel changes.  IPL 7 serializes use of the single static
 * stack; callers are necessarily in supervisor mode (wakeselect has
 * always executed privileged instructions on their behalf).
 */

#include "mint/asmdefs.h"

#define WAKESELECT_SHIM_STACKSIZE	4096	// bytes

	.text

	.globl	SYM(wakeselect_shim)
	.extern	SYM(wakeselect)

	FUNC(wakeselect_shim)
SYM(wakeselect_shim):
	move.w	sr,d1				// caller IPL
#ifdef __mcoldfire__
	move.l	d1,d0
	ori.l	#0x0700,d0			// no ori-to-SR on ColdFire
	move.w	d0,sr
#else
	ori.w	#0x0700,sr			// mask: single stack below
#endif
	move.l	#ws_stack+WAKESELECT_SHIM_STACKSIZE,d0	// top of the private stack
	andi.l	#-4,d0				// longword-align
	move.l	4(sp),a0			// arg: PROC* (from caller stack)
	move.l	sp,ws_savesp
	move.l	d0,sp				// switch to the private stack
	move.w	d1,-(sp)
	move.l	a0,-(sp)
	jsr	SYM(wakeselect)
	addq.l	#4,sp
	move.w	(sp)+,d1
	move.l	ws_savesp,sp
	move.w	d1,sr
	rts
	END(wakeselect_shim)

	.bss
	.even
ws_savesp:
	ds.l	1
ws_stack:
	ds.b	WAKESELECT_SHIM_STACKSIZE
