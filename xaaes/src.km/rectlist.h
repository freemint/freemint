/*
 * $Id$
 * 
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for FreeMiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _rectlist_h
#define _rectlist_h

#include "global.h"

#define RECT_SYS	0
#define RECT_OPT	1

struct build_rl_parms;
struct build_rl_parms
{
	int	(*getnxtrect)(struct build_rl_parms *p);
	RECT	*area;
	RECT	*next_r;
	
	void *ptr1;
};

bool was_visible(struct xa_window *w);
bool xa_rc_intersect(const RECT s, RECT *d);
bool xa_rect_clip(const RECT *s, RECT *d, RECT *r);

//struct xa_rect_list *generate_rect_list(enum locks lock, struct xa_window *w, short which);

struct xa_rect_list *build_rect_list(struct build_rl_parms *p);
struct xa_rect_list *make_rect_list(struct xa_window *w, bool swap, short which);
int get_rect(struct xa_window *wind, RECT *clip, bool first, RECT *ret);
void free_rect_list(struct xa_rect_list *first);

//void dispose_rect_list(struct xa_window *w);

struct xa_rect_list *rect_get_optimal_first(struct xa_window *w);
struct xa_rect_list *rect_get_user_first(struct xa_window *w);
struct xa_rect_list *rect_get_optimal_next(struct xa_window *w);
struct xa_rect_list *rect_get_user_next(struct xa_window *w);
struct xa_rect_list *rect_get_system_first(struct xa_window *w);
struct xa_rect_list *rect_get_system_next(struct xa_window *w);

#endif /* _rectlist_h */
