/*
 * $Id$
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann
 *
 * A multitasking AES replacement for MiNT
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

#ifndef _debug_h
#define _debug_h

#include "global.h"


void fdisplay(struct file *f, const char *fmt, ...);
void display(const char *fmt, ...);

#if GENERATE_DIAGS

#define MAX_NAMED_DIAG 130

enum debug_item
{
	D_this,			/* unspecific, used for incidental debugging. */
	D_appl,			/* includes client pool */
	D_evnt,
	D_multi,		/* evnt_multi only */
	D_form,
	D_fsel,
	D_graf,
	D_menu,
	D_objc,
	D_rsrc,
	D_scrp,
	D_shel,
	D_wind,
	D_widg,
	D_mouse,
	D_button,
	D_keybd,
	D_sema,
	D_rect,			/* rectangle checking */
	D_pipe,			/* pipes & devices excl trap */
	D_trap,			/* plain trap #2,AES commands */
	D_kern,			/* Gemdos part of kernel */
	D_wdlg,			/* WDIALOG extensions */
	D_lbox,
	D_a,D_b,D_c,D_d,D_e,D_f,D_g,D_h,D_i,D_j,D_k,D_l,D_m,
	D_n,D_o,D_p,D_q,D_r,D_s,D_t,D_u,D_v,D_w,D_x,D_y,D_z,
	D_max
};

/* debugging catagories */
struct debugger
{
	enum debug_item point[D_max];
	struct file *debug_file;/* File handle to dump debug stuff to */
	long bug_line;
	long debug_lines;
	int debug_level;	/* HR 081102: global level */
	char debug_path[200];	/* Path to dump debug info to */
};

extern const char *D_su;
extern const char *D_sd;
extern const char *D_sr;
extern const char *D_cl;
extern const char *D_fl;
extern const char *D_flu;

struct xa_client;
struct xa_window;
struct widget_tree;

void show_bits(unsigned short, char *prf, char *t[], char *x);
char *w_owner(struct xa_window *w);
char *c_owner(struct xa_client *c);
char *t_owner(struct widget_tree *t);

#define File_Line display(D_fl,__FILE__,__LINE__)
#define DIAGS(x) if (D.debug_level) File_Line,display x
#define DIAG(x) DeBug x
#define IFDIAG(x) x

/* debugging catagories & data */
extern struct debugger D;

void DeBug(enum debug_item item, struct xa_client *client, char *t, ...);
void display_env(char **env, int which);

#if DEBUG_CONTROL
#define CONTROL(a,b,c) {short *co = pb->control; \
			 if (co[1] != a || co[2] != b || co[3] != c) \
				display(D_cl,a,co[1],b,co[2],c,co[3]); }
#else
#define CONTROL(a,b,c)
#endif

#else /* GENERATE_DIAGS */

#define MAX_NAMED_DIAG 0
#define DIAGS(x)
#define DIAG(x)
#define IFDIAG(x)
#define CONTROL(a,b,c)

#endif /* GENERATE_DIAGS */

#endif /* _debug_h */
