/*
 * $Id$
 * 
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

#include "cpx_bind.h"


void
cpx_userdef(void (*cpx_userdef)(void))
{
	if (cpx_userdef)
		(*cpx_userdef)();
}


CPXINFO	*
cpx_init(CPX_DESC *cpx_desc, XCPB *xcpb)
{
	CPXINFO *_cdecl (*init)(XCPB *, long, long);

	init = cpx_desc->start_of_cpx;

	/*
	 * COPS extensions:
	 * - first magic value 'COPS' (0x434F5053)
	 * - then hex value 0x10001 (whatever it means, taken from
	 *   original assembler bindings)
	 */
	return (*init)(xcpb, 0x434F5053, 0x10001);
}

short
cpx_call(CPX_DESC *cpx_desc, GRECT *work)
{
	return (*cpx_desc->info->cpx_call)(work, cpx_desc->dialog);
}

void
cpx_draw(CPX_DESC *cpx_desc, GRECT *clip)
{
	if (cpx_desc->info->cpx_draw)
		(*cpx_desc->info->cpx_draw)(clip);
}

void
cpx_wmove(CPX_DESC *cpx_desc, GRECT *work)
{
	if (cpx_desc->info->cpx_wmove)
		(*cpx_desc->info->cpx_wmove)(work);
}

short
cpx_timer(CPX_DESC *cpx_desc)
{
	short ret = 0;

	if (cpx_desc->info->cpx_timer)
		(*cpx_desc->info->cpx_timer)(&ret);

	return ret;	
}

short
cpx_key(CPX_DESC *cpx_desc, short kstate, short key)
{
	short ret = 0;

	if (cpx_desc->info->cpx_key)
	{
		struct cpx_key_args args = { kstate, key, &ret };
		(*cpx_desc->info->cpx_key)(args);
	}

	return ret;
}

short
cpx_button(CPX_DESC *cpx_desc, MRETS *mrets, short nclicks)
{
	short ret = 0;

	if (cpx_desc->info->cpx_button)
	{
		struct cpx_button_args args = { mrets, nclicks, &ret };
		(*cpx_desc->info->cpx_button)(args);
	}

	return ret;
}

short
cpx_m1(CPX_DESC *cpx_desc, MRETS *mrets)
{
	short ret = 0;

	if (cpx_desc->info->cpx_m1)
		(*cpx_desc->info->cpx_m1)(mrets, &ret);

	return ret;
}

short
cpx_m2(CPX_DESC *cpx_desc, MRETS *mrets)
{
	short ret = 0;

	if (cpx_desc->info->cpx_m2)
		(*cpx_desc->info->cpx_m2)(mrets, &ret);

	return ret;
}

short
cpx_hook(CPX_DESC *cpx_desc, short event, short *msg, MRETS *mrets, short *key, short *nclicks)
{
	short ret = 0;
	
	if (cpx_desc->info->cpx_hook)
	{
		struct cpx_hook_args args = { event, msg, mrets, key, nclicks };
		(*cpx_desc->info->cpx_hook)(args);
	}
	
	return ret;
}

void
cpx_close(CPX_DESC *cpx_desc, short flag)
{
	if (cpx_desc->info->cpx_close)
	{
		struct cpx_close_args args = { flag };
		(*cpx_desc->info->cpx_close)(args);
	}
}
