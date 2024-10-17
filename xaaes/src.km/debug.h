/*
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

#ifndef _debug_h
#define _debug_h

#include "global.h"
#ifndef PROFILING
#define PROFILING	0
#endif

#define WAIT_SEE					{int _i;for( _i = 0; _i < 10000; _i++ )nap( 60000 );}
#define WAIT_SEEi(i)			{int _i;for( _i = 0; _i < i; _i++ )nap( 60000 );}


void _cdecl display(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void _cdecl ndisplay(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

enum prof_cmd{ P_Init_All = 0, P_Init = 1, P_Start = 2, P_Stop = 3, P_Print_All = 4,
	P_Drop_Name = 5};

int prof_acc( const char *name, enum prof_cmd cmd, int rv );
void profile( const char *t, ...) __attribute__((format(printf, 1, 2)));


#define STRING(x)	# x
/* makes f->_Pf */
#define PRV(x)	_P ## x
/* c,x -> "c:x"*/
#define PRSTR(c,f)	# c":" #f

#if PROFILING

#define PRINIT	prof_acc(0,P_Init_All,0)
#define PRPRINT	prof_acc(0,P_Print_All,0)
#define PRCLOSE	profile(0)

/* define symbol */
#define PRDEF(c,f)	static char* PRV(f) = PRSTR(c,f)
/* record usage of function */

/* start && (f(args) ? stop->1:stop->0) */
#define PROFREC(f,args) (prof_acc(_P ## f,P_Start,1) && ((f args) ? prof_acc(_P ## f,P_Stop,1):prof_acc(_P ## f,P_Stop,0)))

/* start; f(args); stop */
#define PROFRECv(f,args) prof_acc(_P ## f,P_Start,0) ; (f args); prof_acc(_P ## f,P_Stop,0)

/* start && (a f(args) ? stop->1:stop->0) */
#define PROFRECa(a,f,args) (prof_acc(_P ## f,P_Start,1) && ((a f args) ? prof_acc(_P ## f,P_Stop,1):prof_acc(_P ## f,P_Stop,0)))

/* start; a f(args); stop */
#define PROFRECs(a,f,args) prof_acc(_P ## f,P_Start,0); a f args; prof_acc(_P ## f,P_Stop,0)

/* start; (a f)(args); stop */
#define PROFRECp(a,f,args) prof_acc(_P ## f,P_Start,0); (a f) args; prof_acc(_P ## f,P_Stop,0)

/* start; a stop(f(args)) */
#define PROFRECr(a,f,args) prof_acc(_P ## f,P_Start,1); a prof_acc(_P ## f,P_Stop,f args)

#define PROFILE(msg) profile msg

#else

#define PRINIT
#define PRPRINT
#define PRCLOSE

#define PRDEF(c,f)

#define PROFILE(x)

#define PROFREC(f,args)	f args
#define PROFRECv(f,args)	f args
#define PROFRECa(a,f,args)	(a f args)
#define PROFRECs(a,f,args)	a f args
#define PROFRECp(a,f,args)	(a f) args
#define PROFRECr(a,f,args)	a f args

#endif

#if GENERATE_DIAGS

#define BOOTLOG 1

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
	D_fnts,
	D_fslx,
	D_pdlg,
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

void show_bits(unsigned short, const char *prf, const char *const t[], char *x);
const char *w_owner(struct xa_window *w);
const char *c_owner(struct xa_client *c);
const char *t_owner(struct widget_tree *t);

#define DIAGS(x) if (D.debug_level) diags x
#define DIAGA(x) if (D.debug_level) diaga x
#define DIAG(x) diag x
#define IFDIAG(x) x

/* debugging catagories & data */
extern struct debugger D;


void diags(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void diaga(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void diag(enum debug_item item, struct xa_client *client, const char *t, ...) __attribute__((format(printf, 3, 4)));

#if DEBUG_CONTROL
#define CONTROL(nintin,nintout,naddrin) {short *co = pb->control; \
			 if (co[N_INTIN] != nintin || co[N_INTOUT] != nintout || co[N_ADDRIN] != naddrin) \
				diags(D_cl,nintin,co[1],nintout,co[N_INTOUT],naddrin,co[N_ADDRIN]); }
#define CONTROL2(nintin,nintout,naddrin, nintin2,nintout2,naddrin2) {short *co = pb->control; \
			 if ((co[N_INTIN] != nintin || co[N_INTOUT] != nintout || co[N_ADDRIN] != naddrin) && \
			     (co[N_INTIN] != nintin2 || co[N_INTOUT] != nintout2 || co[N_ADDRIN] != naddrin2))\
				diags(D_cl,nintin,co[N_INTIN],nintout,co[N_INTOUT],naddrin,co[N_ADDRIN]); }
#define CONTROL3(nintin,nintout,naddrin, nintin2,nintout2,naddrin2, nintin3,nintout3,naddrin3) {short *co = pb->control; \
			 if ((co[N_INTIN] != nintin || co[N_INTOUT] != nintout || co[N_ADDRIN] != naddrin) && \
			     (co[N_INTIN] != nintin2 || co[N_INTOUT] != nintout2 || co[N_ADDRIN] != naddrin2) && \
			     (co[N_INTIN] != nintin3 || co[N_INTOUT] != nintout3 || co[N_ADDRIN] != naddrin3))\
				diags(D_cl,nintin,co[N_INTIN],nintout,co[N_INTOUT],naddrin,co[N_ADDRIN]); }
#endif

#else /* GENERATE_DIAGS */

#define MAX_NAMED_DIAG 0
#define DIAGS(x)
#define DIAGA(x)
#define DIAG(x)
#define IFDIAG(x)

#endif /* GENERATE_DIAGS */

#ifndef CONTROL
#define CONTROL(a,b,c)
#define CONTROL2(a,b,c,d,e,f)
#define CONTROL3(a,b,c,d,e,f,g,h,i)
#endif

void _cdecl bootlog(short disp, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
#ifdef BOOTLOG
#define BLOG(x)	bootlog x
#define BLOGif(c,x)	if(c)bootlog x
#define KDBG(x) KERNEL_DEBUG x
#else
#define BLOG(x)
#define BLOGif(c, x)
#define KDBG(x)
#endif

#endif /* _debug_h */
