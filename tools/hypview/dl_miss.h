/*
 * HypView - (c)      - 2006 Philipp Donze
 *               2006 -      Philipp Donze & Odd Skancke
 *
 * A replacement hypertext viewer
 *
 * This file is part of HypView.
 *
 * HypView is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HypView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HypView; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _dl_miss_h
#define _dl_miss_h
/*
		definiere fehlende Typen
*/

#ifdef __GNUC__
	#include <mint/ostruct.h>
	#define KEYTAB _KEYTAB
#endif

#ifndef __GEMLIB_DEFS
	typedef short GRECT[4];
	typedef char *OBSPEC;
	typedef void *OBJECT;
	typedef void *RSHDR;
#endif
#ifndef _MT_GEMLIB_X_H_
	typedef void *EVNT;
	typedef void *DIALOG;
#ifdef __GNUC__
	typedef short (*HNDL_OBJ)();
#else
	typedef short (cdecl *HNDL_OBJ)();
#endif
	typedef void *FNT_DIALOG;
#endif

#ifndef __VDI__
	#if USE_GLOBAL_VDI == YES
		#if SAVE_COLORS == YES
			typedef void *RGB1000;
		#endif
	#endif
#endif

#if !defined(__TOS) && !defined(_MINT_OSTRUCT_H)
	typedef void *KEYTAB;
#endif

#ifndef NULL
	#define	NULL	((void *)0)
#endif

#ifndef TRUE
	#define	TRUE	1
#endif

#ifndef FALSE
	#define	FALSE	0
#endif

#undef NIL
#define	NIL	((void *)-1)

#ifndef min
	#define	min(a, b)	((a) < (b) ? (a) : (b))
#endif
#ifndef max
	#define	max(a, b)	((a) > (b) ? (a) : (b))
#endif
#ifndef abs
	#define	abs(a)		((a) >= 0   ? (a) : -(a))
#endif


/*	Maus-Position/Status und Tastatur-Status (evnt_button, evnt_multi)	*/
typedef struct
{
	short	x;
	short	y;
	short	bstate;
	short	kstate;
} EVNTDATA;

typedef struct _xted
{
	char	*xte_ptmplt;
	char	*xte_pvalid;
	short	xte_vislen;
	short	xte_scroll;
} XTED;

typedef struct
{
	short ap_version;
	short ap_count;
	short ap_id;
	long ap_private;
	long ap_ptree;
	long ap_pmem;
	short ap_lmem;
	short ap_nplanes;
	long ap_res;
	short ap_bvdisk;
	short ap_bvhard;
} GlobalArray;

#endif       /* _dl_miss_h */
