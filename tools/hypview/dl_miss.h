/*
 * $Id$
 * 
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

/*
		definiere fehlende Typen
*/
#ifndef __GNUC__
#ifndef __AES__
#include <types.h>

	typedef short GRECT[4];
	typedef short GlobalArray[12];
	typedef char *OBSPEC;
	typedef void *OBJECT;
	typedef void *RSHDR;
	typedef void *EVNT;
	typedef void *DIALOG;
	typedef void *EVNTDATA;
	typedef short	(cdecl *HNDL_OBJ)(DIALOG *dialog,EVNT *events,short obj,short clicks,void *data);
	typedef void *FNT_DIALOG;
	typedef void *XTED;
#endif

#ifndef __VDI__
	#if USE_GLOBAL_VDI == YES
		#if SAVE_COLORS == YES
			typedef void *RGB1000;
		#endif
	#endif
#endif

#ifndef __TOS
	typedef void *_KEYTAB;
#endif


#ifndef min
	#define	min(a, b)	((a) < (b) ? (a) : (b))
#endif
#ifndef max
	#define	max(a, b)	((a) > (b) ? (a) : (b))
#endif
#ifndef abs
#define	abs(a)		((a) >= 0   ? (a) : -(a))
#endif

#else	/* __GNUC__ */
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

#endif /* __GNUC__ */
