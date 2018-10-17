/*
 * COPS (c) 1995 - 2003 Sven & Wilfried Behne
 *                 2004 F.Naumann & O.Skancke
 *
 * A XCONTROL compatible CPX manager.
 *
 * This file is part of COPS.
 *
 * COPS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * COPS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "cops_rsc.h"
#include "callback.h"
#include "cpx_bind.h"
#include "cops.h"

#if defined(__GNUC__) && defined(__MINTLIB__)
#  ifdef _BITS_SETJMP_H
     /* using new definition with array of struct */
#    define JUMPBUF_RET_PC(jb) (jb)[0].__jmpbuf[0].ret_pc
#    define JUMPBUF_RET_SP(jb) (jb)[0].__jmpbuf[0].regs[11]
#  else
     /* using old definition with array of longs */
#    define JUMPBUF_RET_PC(jb) (jb)[0]
#    define JUMPBUF_RET_SP(jb) (jb)[12]
#  endif
#endif

#if defined(__PUREC__) || defined(__AHCC__)
#  define JUMPBUF_RET_PC(jb) ((long *)(jb))[5]
#  define JUMPBUF_RET_SP(jb) ((long *)(jb))[11]
#endif


#define DEBUG_CALLBACK(cpx) DEBUG(("%s(%s)\n", __FUNCTION__, cpx->file_name))


void
cpx_userdef(void (*userdef)(void))
{
	DEBUG(("cpx_userdef(%p)\n", userdef));

	if (userdef)
	{
		__asm__ __volatile__(
				"\tjsr (%0)\n"
			:
			: "a"(userdef)
			: "d2", "a2", "cc" AND_MEMORY
			);
	}
}


CPXINFO	*
cpx_init(CPX_DESC *cpx_desc, struct xcpb *xcpb)
{
	register CPXINFO *_cdecl (*init)(struct xcpb *, long, long);
	register CPXINFO *ret;

	DEBUG_CALLBACK(cpx_desc);

	init = cpx_desc->start_of_cpx;
	assert(init);

	/*
	 * COPS extensions:
	 * - first magic value 'COPS' (0x434F5053)
	 * - then hex value 0x10001 (whatever it means, taken from
	 *   original assembler bindings)
	 */
	ret = (*init)(xcpb, 0x434F5053L, 0x10001L);

	DEBUG(("cpx_init -> %p\n", ret));
	return ret;
}

short
cpx_call(CPX_DESC *cpx_desc, GRECT *work)
{
	register short ret;

	DEBUG_CALLBACK(cpx_desc);
	DEBUG(("calling out cpx_call at %p\n", cpx_desc->info->cpx_call));

	assert(cpx_desc->info->cpx_call);
	ret = (*cpx_desc->info->cpx_call)(work, cpx_desc->dialog);

	DEBUG(("cpx_call -> %d\n", ret));
	return ret;
}

void
cpx_draw(CPX_DESC *cpx_desc, GRECT *clip)
{
	DEBUG_CALLBACK(cpx_desc);

	if (cpx_desc->info->cpx_draw)
		(*cpx_desc->info->cpx_draw)(clip);
}

void
cpx_wmove(CPX_DESC *cpx_desc, GRECT *work)
{
	DEBUG_CALLBACK(cpx_desc);

	if (cpx_desc->info->cpx_wmove)
		(*cpx_desc->info->cpx_wmove)(work);
}

short
cpx_timer(CPX_DESC *cpx_desc)
{
	short ret;

	DEBUG_CALLBACK(cpx_desc);

	if (cpx_desc->info->cpx_timer)
		(*cpx_desc->info->cpx_timer)(&ret);
	else
		ret = 0;

	return ret;	
}

short
cpx_key(CPX_DESC *cpx_desc, short kstate, short key)
{
	short ret;

	DEBUG_CALLBACK(cpx_desc);

	if (cpx_desc->info->cpx_key)
	{
		struct cpx_key_args args;
		args.kstate = kstate;
		args.key = key;
		args.quit = &ret;
		(*cpx_desc->info->cpx_key)(args);
	}
	else
		ret = 0;

	return ret;
}

short
cpx_button(CPX_DESC *cpx_desc, MRETS *mrets, short nclicks)
{
	short ret;

	DEBUG_CALLBACK(cpx_desc);

	if (cpx_desc->info->cpx_button)
	{
		struct cpx_button_args args;
		args.mrets = mrets;
		args.nclicks = nclicks;
		args.quit = &ret;
		(*cpx_desc->info->cpx_button)(args);
	}
	else
		ret = 0;

	return ret;
}

short
cpx_m1(CPX_DESC *cpx_desc, MRETS *mrets)
{
	short ret;

	DEBUG_CALLBACK(cpx_desc);

	if (cpx_desc->info->cpx_m1)
		(*cpx_desc->info->cpx_m1)(mrets, &ret);
	else
		ret = 0;

	return ret;
}

short
cpx_m2(CPX_DESC *cpx_desc, MRETS *mrets)
{
	short ret;

	DEBUG_CALLBACK(cpx_desc);

	if (cpx_desc->info->cpx_m2)
		(*cpx_desc->info->cpx_m2)(mrets, &ret);
	else
		ret = 0;

	return ret;
}

short
cpx_hook(CPX_DESC *cpx_desc, _WORD event, _WORD *msg, MRETS *mrets, _WORD *key, _WORD *nclicks)
{
	register short ret;

	DEBUG_CALLBACK(cpx_desc);

	if (cpx_desc->info->cpx_hook)
	{
		struct cpx_hook_args args;
		args.event = event;
		args.msg = msg;
		args.mrets = mrets;
		args.key = key;
		args.nclicks = nclicks;
		ret = (*cpx_desc->info->cpx_hook)(args);
	}
	else
		ret = 0;

	return ret;
}

void
cpx_close(CPX_DESC *cpx_desc, _WORD flag)
{
	DEBUG_CALLBACK(cpx_desc);

	if (cpx_desc->info->cpx_close)
	{
		struct cpx_close_args args;
		args.flag = flag;
		(*cpx_desc->info->cpx_close)(args);
	}
}


#if 0
_a_call_main:
	movem.l	d0-d7/a0-a7,alpha_context	| Kontext des Hauptprogramms
	move.l	#a_call_return,-(sp)		| Rueckkehrfunktion ins Hauptprogramm
	move.l	sp,kernel_stack
	bsr	_cpx_main_loop			| Eventschleife

a_call_return:
	movem.l	alpha_context(pc),d0-d7/a0-a7
	rts
#endif

static jmp_buf alpha_context;
static long kernel_stack;

void
a_call_main(void)
{
	jmp_buf jb;

	DEBUG(("a_call_main: enter\n"));

	if (setjmp(alpha_context))
	{
		DEBUG(("a_call_main: leave\n"));
		return;
	}

	/* remember current stack */
	kernel_stack = JUMPBUF_RET_SP(alpha_context);
	DEBUG(("a_call_main: kernel_stack 0x%lx\n", kernel_stack));

	JUMPBUF_RET_PC(jb) = (long)cpx_main_loop;
	JUMPBUF_RET_SP(jb) = kernel_stack;

	longjmp(jb, 1);

	/* never reached */
	assert(0);
}
void
a_call_return(void)
{
	DEBUG(("a_call_return\n"));
	longjmp(alpha_context, 1);

	/* never reached */
	assert(0);
}

#if 0
|short cdecl Xform_do(OBJECT *tree, short edit_obj, short *msg)
|form_do() fuer ein CPX ausfuehren
|Eingaben:
|4(sp).l OBJECT	*tree
|8(sp).w short edit_obj
|10(sp).l short *msg
|
_Xform_do:
	movea.l	(sp),a0				| Ruecksprungadresse
	bsr	_get_cpx_desc			| CPX_DESC suchen
	move.l	a0,d0
	beq.s	Xform_do_err			| kein passendes CPX gefunden

	movem.l	d0-d7/a1-a7,CPXD_context(a0)

	movea.l	4(sp),a1			| tree
	move.w	8(sp),d0			| edit_obj
	move.l	10(sp),d1			| msg

	movea.l	kernel_stack(pc),sp
	move.l	d1,-(sp)			| msg
	bsr	_cpx_form_do			| CPX_DESC *cpx_form_do(CPX_DESC *cpx_desc,
						|		        OBJECT *tree, short edit_obj, short *msg)
	addq.l	#4,sp				| Stack korrigieren
	move.l	a0,d0
	beq	a_call_return			| Hauptfenster wurde geschlossen
	bra	_switch_context			| dieser Fall darf nicht auftreten!

Xform_do_err:
	move.l	10(sp),a0			| msg
	move.w	#41,(a0)			| 41 - AC_CLOSE; close CPX
	moveq	#-1,d0
	rts
#endif

static CPX_DESC *call_cpx_form_do_cpx = NULL;
static struct Xform_do_args call_cpx_form_do_args;

static void
call_cpx_form_do(void)
{
	DEBUG(("call_cpx_form_do\n"));

	cpx_form_do(call_cpx_form_do_cpx,
		    call_cpx_form_do_args.tree,
		    call_cpx_form_do_args.edit_obj,
		    call_cpx_form_do_args.msg);

	/* never reached */
	assert(0);
}
short _cdecl
Xform_do(const long *sp)
{
	register CPX_DESC *cpx;

	cpx = find_cpx_by_addr(sp);
	DEBUG_CALLBACK(cpx);

	if (cpx)
	{
		jmp_buf jb;

		/*
		 * save arguments
		 */
		call_cpx_form_do_cpx = cpx;
		call_cpx_form_do_args = *(const struct Xform_do_args *)(++sp);

		/* save for later return */
		if (setjmp(cpx->jb))
		{
			DEBUG(("Xform_do: return %i\n", cpx->button));
			return cpx->button;
		}

		/*
		 * switch stack and call out form_do
		 */

		JUMPBUF_RET_PC(jb) = (long)call_cpx_form_do;
		JUMPBUF_RET_SP(jb) = kernel_stack;

		longjmp(jb, 1);

		/* never reached */
		assert(0);
	}

	*(((const struct Xform_do_args *)(++sp))->msg) = AC_CLOSE;
	return -1;
}

#if 0
|void new_context(CPX_DESC *cpx_desc)|
|Neuen Kontext anlegen, CPX oeffnen, cpx_init() und cpx_call() aufrufen
|Eingaben:
|a0.l CPX_DESC *cpx_desc
|Ausgaben:
|-
_new_context:
	movem.l	d0-d1/a0-a1,-(sp)
	move.l	#16384,d0
	bsr	_malloc				| 16384 Bytes fuer temporaeren Stack anfordern
	move.l	a0,d2
	movem.l	(sp)+,d0-d1/a0-a1

	move.l	d2,CPXD_sp_memory(a0)		| Speicherbereich fuer Stack merken
	beq.s	new_context_err

	add.l	#16384,d2
	move.l	d2,sp				| neuer Stack fuer Eventloop

	movem.l	d0-d7/a0-a7,-(sp)		| CPX oeffnen, ggf. Eventloop aufrufen
	bsr	_open_cpx_context		| void open_cpx_context(CPX_DESC *cpx_desc)
	movem.l	(sp)+,d0-d7/a0-a7

	movea.l	kernel_stack(pc),sp

	movea.l	CPXD_sp_memory(a0),a0		| Speicherbereich fuer den Stack freigeben
	bsr	_free				| Stack freigeben

	bra	_cpx_main_loop			| Hauptschleife aufrufen

new_context_err:
	moveq	#0,d0
	rts
#endif
static CPX_DESC *call_open_cpx_context_desc = NULL;

static void
new_context_done(void)
{
	jmp_buf jb;

	DEBUG(("new_context_done\n"));

	free(call_open_cpx_context_desc->stack);
	call_open_cpx_context_desc->stack = NULL;

	JUMPBUF_RET_PC(jb) = (long)cpx_main_loop;
	JUMPBUF_RET_SP(jb) = kernel_stack;

	longjmp(jb, 1);
}
static void
call_open_cpx_context(void)
{
	jmp_buf jb;

	DEBUG(("call_open_cpx_context(%s)\n", call_open_cpx_context_desc->file_name));
	open_cpx_context(call_open_cpx_context_desc);

	DEBUG(("call_open_cpx_context(%s) -> go back\n", call_open_cpx_context_desc->file_name));

	JUMPBUF_RET_PC(jb) = (long)new_context_done;
	JUMPBUF_RET_SP(jb) = kernel_stack;

	longjmp(jb, 1);
}
short
new_context(CPX_DESC *cpx_desc)
{
	DEBUG(("new_context(%s)\n", cpx_desc->file_name));

	cpx_desc->stack = malloc(16384);
	if (cpx_desc->stack)
	{
		jmp_buf jb;

		DEBUG(("new_context: stack %p\n", cpx_desc->stack));
		DEBUG(("new_context -> call_open_cpx_context\n"));

		call_open_cpx_context_desc = cpx_desc;

		JUMPBUF_RET_PC(jb) = (long)call_open_cpx_context;
		JUMPBUF_RET_SP(jb) = (long)cpx_desc->stack + 16380;

		longjmp(jb, 1);

		/* never reached */
		assert(0);
	}

	return 0;
}

#if 0
|void switch_context(CPX_DESC *cpx_desc)
|Kontext wechseln, CPX anspringen
|Eingaben:
|a0.l CPX_DESC *cpx_desc
|
_switch_context:
	movem.l	CPXD_context(a0),d0-d7/a1-a7
	move.w	CPXD_button(a0),d0		| Objektnummer oder Message
	rts
#endif
void
switch_context(CPX_DESC *cpx_desc)
{
	DEBUG(("switch_context(%s)\n", cpx_desc->file_name));

	/* return to interrupted Xform_do */
	longjmp(cpx_desc->jb, 1);
}
