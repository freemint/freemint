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
#include "xa_types.h"
#include "mt_gem.h"
#include "xa_global.h"
#include "xa_strings.h"
#include "matchpat.h"	//strmchr

#include "draw_obj.h"
#include "trnfm.h"
#include "obtree.h"
#include "taskman.h"
#include "xa_appl.h"
#include "xa_rsrc.h"
#include "xa_shel.h"
#include "util.h"

#include "mint/fcntl.h"
#include "mint/stat.h"
#include "mint/signal.h"

/*
 * RESOURCE FILE HANDLER
 *
 * Simulate the standard GEM AES resource access functions.
 *
 * I've added these to Steve's routines to act as a bit of an interface
 * to his routines. Each application has its own rsrc as part
 * of the xa_client structure. As Steve's resource loader is happy to
 * allow multiple resource files, I should add some extra calls to support it.
 */


/*
 * Fixup OBJECT coordinates; the lower byte contains a (character-based)
 * coordinate, the upper byte an additional (pixel) offset.
 */
static void
fixupobj(short *val, unsigned long fact, short design)
{
	unsigned short h, l;
	unsigned long temp;

	h = (*val & 0xff) * design;
	l = (*val >> 8) & 0xff;
	if (l > 128) l -= 256;
	h += l;
	temp = fact * h;
	if (temp & 0x8000)
		temp += 0x00010000L;
	*val = (temp >> 16);
}

void _cdecl
obfix(OBJECT *tree, short object, short designWidth, short designHeight)
{
	unsigned long x_fact, y_fact;
	OBJECT *ob = tree + object;

	x_fact = (long)screen.c_max_w << 16;
	x_fact /= designWidth;
	y_fact = (long)screen.c_max_h << 16;
	y_fact /= designHeight;

	fixupobj(&ob->ob_x, x_fact, designWidth);
	fixupobj(&ob->ob_y, y_fact, designHeight);

	/*
	 * Special case handling: any OBJECT 80 characters wide is supposed
	 * to be a menu bar, which always covers the entire screen width...
	 */
#if 0
	if (!cfg.menu_layout && ob->ob_width == 80 && ob->ob_type == G_BOX && ob->ob_x == 0)
	{
		ob->ob_width = screen.r.w;
	}
	else
#endif
	{
		fixupobj(&ob->ob_width, x_fact, designWidth);
	}

	fixupobj(&ob->ob_height, y_fact, designHeight);
}

/*
 * Code in this module is based on the resource loader from
 * Steve Sowerby's AGiLE library. Thanks to Steve for allowing
 * me to use his code.
 */

/* new function to make the code orthogonal.
 */
extern short disp;

#if 0
static short *
transform_icon_bitmap(struct xa_client *client, struct xa_rscs *rscs, CICONBLK *icon, short *map, long len, int planes, short vdih)
{
	MFDB src, dst;
	short *new_data = map;
	short *tmp = NULL;
	long icon_len = len * planes;
	long new_len = len * screen.planes;
	struct remember_alloc *ra;

	DIAG((D_s, client, "icon_len %ld, new_len %ld", icon_len, new_len));

//  if (planes < screen.planes)
	{
		DIAG((D_x, client, "alloc of %ld bytes", new_len));

		/*
		 * Ozk: we needs to remember mallocs done when new icon data
		 * is created.
		 */
		if ((ra = kmalloc(sizeof(*ra))))
		{
			if (client == C.Aes || client == C.Hlp)
				new_data = kmalloc(new_len);
			else
				new_data = umalloc(new_len);

			if (!new_data)
			{
				kfree(ra);
				return map;
			}
			/*
			 * setup and attach the new remember_alloc
			 */
			ra->addr = new_data;
			ra->next = rscs->ra;
			rscs->ra = ra;

//      memcpy(new_data, map, icon_len);
		}
		else
			return map;
	}

	src.fd_addr = map;
	src.fd_w = icon->monoblk.ib_wicon; /* Transform MFDB's */
	src.fd_h = icon->monoblk.ib_hicon;
	src.fd_wdwidth = (src.fd_w + 15) >> 4; // / 16; /* round up */
	src.fd_stand = 1;
	src.fd_r1 = src.fd_r2 = src.fd_r3 = 0;
	src.fd_nplanes = planes;

	dst = src;

	dst.fd_addr = new_data;
	dst.fd_stand = 0;
	dst.fd_nplanes = screen.planes;

	if (!transform_bitmap(vdih, &src, &dst, cfg.remap_cicons ? rscs->palette : NULL, screen.palette))
		return map;
	else
		return new_data;
}
#else

extern int init_pal_cref;
#define RECALC_REMAP_LUT 127
static short *
transform_icon_bitmap(struct xa_client *client, struct xa_rscs *rscs, CICONBLK *icon, short *map, long len, int planes, short vdih)
{
	MFDB src, dst;
	short *new_data = map;
	short *tmp = NULL;
	long icon_len = len * planes;
	long new_len = len * screen.planes;
	struct remember_alloc *ra;

	DIAG((D_s, client, "icon_len %ld, new_len %ld", icon_len, new_len));

	if (planes < screen.planes)
	{
		DIAG((D_x, client, "alloc of %ld bytes", new_len));

		/*
		 * Ozk: we needs to remember mallocs done when new icon data
		 * is created.
		 */
		if ((ra = kmalloc(sizeof(*ra))))
		{
			if (client == C.Aes || client == C.Hlp)
				new_data = kmalloc(new_len);
			else
				new_data = umalloc(new_len);

			if (!new_data)
			{
				kfree(ra);
				return map;
			}
			/*
			 * setup and attach the new remember_alloc
			 */
			ra->addr = new_data;
			ra->next = rscs->ra;
			rscs->ra = ra;

			memcpy(new_data, map, icon_len);
		}
		else
			return map;
	}

	tmp = kmalloc(new_len);
	if (tmp)
	{

#ifndef ST_ONLY
		static struct rgb_1000 *src_pal;
#endif
		src.fd_addr = tmp;
		src.fd_w	= icon->monoblk.ib_wicon; /* Transform MFDB's */
		src.fd_h	= icon->monoblk.ib_hicon;
		src.fd_wdwidth	= (src.fd_w + 15) >> 4; // / 16; /* round up */
		src.fd_stand	= 1;
		src.fd_r1 = src.fd_r2 = src.fd_r3 = 0;
		src.fd_nplanes	= screen.planes;

		dst = src;

		dst.fd_addr = new_data;
		dst.fd_stand	= 0;
		dst.fd_nplanes	= screen.planes;

		memcpy(tmp, new_data, new_len);
#ifndef ST_ONLY
		if( cfg.remap_cicons )
		{
			static char *icn_pal_name = 0;
			int not_to_sys_pal = cfg.palette[0] && client->options.icn_pal_name && strcmp( cfg.palette, client->options.icn_pal_name );
			if( !rscs->palette && not_to_sys_pal )
			{
				if( C.is_init_icn_pal == 1 && strcmp( icn_pal_name, client->options.icn_pal_name) )
					C.is_init_icn_pal = 0;
				if( !C.is_init_icn_pal )
				{
					if( !C.icn_pal )
						C.icn_pal = kmalloc( 256 * sizeof(*C.icn_pal) );	// free!
					if( C.icn_pal && !rw_syspalette( READ, C.icn_pal, C.Aes->home_path, client->options.icn_pal_name ) )
					{
						if( icn_pal_name )
							kfree(icn_pal_name);
						icn_pal_name = xa_strdup( client->options.icn_pal_name );
						C.is_init_icn_pal = RECALC_REMAP_LUT;
					}
					else
						C.is_init_icn_pal = -1;
				}
			}
			if( rscs->palette )
			{
				if( src_pal != rscs->palette )
				{
					C.is_init_icn_pal = RECALC_REMAP_LUT;
					src_pal = rscs->palette;
				}
			}
			else if( C.is_init_icn_pal >= 1 )
				src_pal = C.icn_pal;
			else
				src_pal = 0;
		}
		else
			src_pal = 0;

		transform_gem_bitmap(vdih, src, dst, planes, src_pal, screen.palette);
#else
		transform_gem_bitmap(vdih, src, dst, planes, 0, screen.palette);
#endif
		kfree(tmp);
	}
	return new_data;
}
#endif
#if 0	// debugging!
void dump_hex( void *data, long len, int bpw, int doit );
void dump_hex( void *data, long len, int bpw, int doit )
{
	union{
		const uchar *p1;
		const short *p2;
		const long *p4;
	}rv;
	int llen = 16;
	char  s[llen*(bpw*2+1)];
	int col = 1, sl = 0;
	long l, off, vl;
	short vs;
	if( len <= 0 || !doit )
		return;

	for( l = off = 0, rv.p1 = data; l < len; rv.p1 += bpw, l += bpw )
	{
		switch( bpw )
		{
			default:
			case 1:
				vs = *rv.p1;
				sl += sprintf( (char*)(s + sl), sizeof( s ) - sl, "%02x ", vs & 0xff );
			break;
			case 2:
				vs = *rv.p2;
				sl += sprintf( (char*)(s + sl), sizeof( s ) - sl, "%04x ", vs );
			break;
			case 3:
				vl = *rv.p4 & 0x00ffffff;
				sl += sprintf( (char*)(s + sl), sizeof( s ) - sl, "%06lx ", vl );
			break;
			case 4:
				vl = *rv.p4;
				sl += sprintf( (char*)(s + sl), sizeof( s ) - sl, "%08lx ", vl );
			break;
		}
		if( ++col > llen )
		{
			s[sl] = 0;
			BLOG(( 0, "%04ld: %s", off, s));
			off = l+bpw;
			col = 1;
			sl = 0;
		}
	}
	if( sl )
	{
		s[sl-1] = 0;
		BLOG(( 0, "%04ld: %s", off, s));
	}

}
#endif

/*
 * FixColourIconData: Convert a colour icon from device independent to device specific
 */
static void
FixColourIconData(struct xa_client *client, CICONBLK *icon, struct xa_rscs *rscs, short vdih)
{
	CICON *best_cicon = NULL;
	CICON *c;
	long len = calc_back((RECT *) &icon->monoblk.ib_xicon, 1);

	DIAG((D_s, client, "color icon: '%s' %d*%d %ld tx.w=%d",
		icon->monoblk.ib_ptext,
		icon->monoblk.ib_wicon, icon->monoblk.ib_hicon, len, icon->monoblk.ib_wtext));

	/* Use the same mechanism from d_g_cicon() for reducing the
	 * number of transformations done.
	 */
	c = icon->mainlist;
	while (c)
	{
		DIAG((D_rsrc,client,"[1]probe cicon 0x%lx", c));

		if (c->num_planes <= screen.planes
				&& (!best_cicon || (best_cicon && c->num_planes > best_cicon->num_planes)))
		{
			best_cicon = c;
		}
		c = c->next_res;
	}

	if (best_cicon)
	{
		/* DIAG((D_rsrc,client,"[1]best_cicon planes: %d", best_cicon->num_planes)); */
		c = best_cicon;
		if (c->col_data)
		{
			c->col_data = transform_icon_bitmap(client, rscs, icon, c->col_data, len, c->num_planes, vdih);
		}
		if (c->sel_data)
			c->sel_data = transform_icon_bitmap(client, rscs, icon, c->sel_data, len, c->num_planes, vdih);

		/* set the new data plane count */
		c->num_planes = screen.planes;
	}
	else
	{
		DIAG((D_rsrc,client,"[1]No matching icon"));
	}
}

#define LF_OFFS 3
#define LF_SEPCH	'_'
#define LF_COMMCH '#'
#define LF_SEPSTR "_"
#define LF_COMMSTR	"#"

static short rsc_trans_rw( struct xa_client *client, XA_FILE *rfp, char **buf, long l );
static XA_FILE *rsc_lang_file( int md, XA_FILE *fp, char *buf, long l );

static void
fix_chrarray(struct xa_client *client, void *b, char **p, unsigned long n, char **extra, XA_FILE *rfp)
{
	if( client->options.rsc_lang == WRITE && n && rfp)
	{
		rsc_lang_file( WRITE, rfp, "# - Strings -", 13 );
	}
	while (n)
	{
		DIAG((D_rsrc, NULL, " -- %lx, value %lx", p, *p));
		*p += (unsigned long)b;
		if( rfp )
		{
			if( !strmchr(*p, "\r\n") )
				rsc_trans_rw( client, rfp, p, 0 );
		}

		DIAG((D_rsrc, NULL, " -- to %lx", *p));

		p++;
		n--;
	}
}

/* fixup all tedinfo field pointers */
static void
fix_tedarray(struct xa_client *client, void *b, TEDINFO *ti, unsigned long n, char **extra)
{
	if ((client->options.app_opts & XAAO_OBJC_EDIT))
	{
		XTEDINFO *ei;

		ei = *(XTEDINFO **)extra;

		while (n)
		{
			bzero(ei, sizeof(*ei));

			ei->p_ti =	ti;
			ti->te_ptext	+= (long)b;
			ti->te_ptmplt += (long)b;
			ti->te_pvalid += (long)b;
			ei->ti = *ti;
			ti->te_ptext	= (char *)-1L;
			ti->te_ptmplt = (char *)ei;

			ti++;
			ei++;
			n--;
			DIAG((D_rsrc, NULL, "fix_tedarray: ti=%lx, ptext='%s'", ti, ei->ti.te_ptext));
			DIAG((D_rsrc, NULL, "ptext=%lx, ptmpl=%lx, pvalid=%lx",
				ti->te_ptext, ti->te_ptmplt, ti->te_pvalid));
		}
		*extra = (char *)ei;
	}
	else
	{
		while (n)
		{
			ti->te_ptext	+= (long)b;
			ti->te_ptmplt += (long)b;
			ti->te_pvalid += (long)b;

			DIAG((D_rsrc, NULL, "fix_tedarray: ti=%lx, ptext='%s'", ti, ti->te_ptext));
			DIAG((D_rsrc, NULL, "ptext=%lx, ptmpl=%lx, pvalid=%lx",
				ti->te_ptext, ti->te_ptmplt, ti->te_pvalid));

			ti++;
			n--;
		}
	}
	DIAG((D_rsrc, NULL, "fixed up %ld tedinfo's", n));
}

/* fixup all iconblk field pointers */
static void
fix_icnarray(struct xa_client *client, void *b, ICONBLK *ib, unsigned long n, char **extra)
{
	while (n)
	{
		ib->ib_pmask = (void *)((char *)ib->ib_pmask + (long)b);
		ib->ib_pdata = (void *)((char *)ib->ib_pdata + (long)b);
		ib->ib_ptext += (long)b;

		DIAG((D_rsrc, NULL, "fix_icnarray: ib=%lx, ib->ib_pmask=%lx, ib->ib_pdata=%lx, ib->ib_ptext=%lx",
			ib, ib->ib_pmask, ib->ib_pdata, ib->ib_ptext));

		ib++;
		n--;
	}

	DIAG((D_rsrc, NULL, "fixed up %ld iconblk's", n));
}

/* fixup all bitblk data pointers */
static void
fix_bblarray(struct xa_client *client, void *b, BITBLK *bb, unsigned long n, char **extra)
{
	while (n)
	{
		bb->bi_pdata = (void *)((char *)bb->bi_pdata + (long)b);

		DIAG((D_rsrc, NULL, "fix_bblarray: bb=%lx, pdata=%lx",
			bb, bb->bi_pdata));
		bb++;
		n--;
	}
	DIAG((D_rsrc, NULL, "fixed up %ld bitblk's", n));
}

static short
fix_cicons(struct xa_client *client, void *base, CICONBLK **cibh, char **extra)
{
	unsigned long *addr;
	long isize;
	short *pdata, numRez, maxplanes = 1;
	int i, j, numCibs = 0;
	CICONBLK *cib, **cp = cibh;

	DIAG((D_rsrc, NULL, "Enhanced resource file"));

	/*
	 * Find out how many CICONBLK's are present
	 */
	while (*cp++ != (CICONBLK *) -1L)
		numCibs++;

	DIAG((D_rsrc, NULL, "fix_cicons: got %d cicons at %lx(first=%lx)", numCibs, cibh, cibh[0]));

	/*
	 * Pointer to the first CICONBLK (went past the -1L above)
	 */
	cib = (CICONBLK *)cp;

	/* Fix up all the CICONBLK's */
	for (i = 0; i < numCibs; i++)
	{
		CICON *cicn, *prev_cicn;
		ICONBLK *ib = &cib->monoblk;

		cibh[i] = cib;						/* Put absolute address of this ciconblk into array */
		isize = calc_back((RECT*)&ib->ib_xicon, 1);

		addr = (unsigned long *)((char *)cib + sizeof(ICONBLK));
		numRez = addr[0];
		pdata = (short *)&addr[1];
		/* mono data & mask */
		ib->ib_pdata = pdata;
		pdata = (short *)((char *)pdata + isize);
		ib->ib_pmask = pdata;
		pdata = (short *)((char *)pdata + isize);
		/* HR: the texts are placed the same way as for all other objects
		 * when longer than 12 bytes.
		 */
		DIAG((D_rsrc, NULL, "cibh[%d]=%lx, isize=%ld, addr=%lx, numRez=%d",
			i, cibh[i], isize, addr, numRez));
		DIAG((D_rsrc, NULL, "ib = %lx, ib_pdata=%lx, ib_pmask=%lx",
			ib, ib->ib_pdata, ib->ib_pmask));

		if (ib->ib_wtext && ib->ib_ptext)
		{
			short l = ib->ib_wtext/6;
			/* fix some resources */
			ib->ib_ptext += (long)base;
			DIAG((D_rsrc, NULL, "cicon: ib->ptext = %lx", ib->ib_ptext));
			if (strlen(ib->ib_ptext) > l)
				*(ib->ib_ptext + l) = 0;
		}
		else
			/* The following word is no of planes which cannot
			 * be larger than 32, so there is 1 zero byte there */
			ib->ib_ptext = (char *)pdata;

		/* (unused) place for name, 12 bytes */
		pdata = (short *)((char *)pdata + 12);

		cicn = (CICON *)pdata;
		prev_cicn = cicn;
		/* There can be color icons with NO color icons,
		 * only the mono icon block. */
		cib->mainlist = NULL;
		/* Get CICON's at different rez's */
		for (j = 0; j < numRez; j++)
		{
			short planes = cicn->num_planes;
			unsigned long psize = isize * planes;

			if (planes > maxplanes)
				maxplanes = planes;

			pdata = (short *)((char *)pdata + sizeof(CICON));
			cicn->col_data = pdata;
			pdata = (short *)((char *)pdata + psize);
			cicn->col_mask = pdata;
			pdata = (short *)((char *)pdata + isize);
			if (cicn->sel_data)
			{
				/* It's got a selected form */
				cicn->sel_data = pdata;
				pdata = (short *)((char *)pdata + psize);
				cicn->sel_mask = pdata;
				pdata = (short *)((char *)pdata + isize);
			}
			else
				/* No selected version */
				cicn->sel_data = cicn->sel_mask = NULL;

			if (cib->mainlist == NULL)
				cib->mainlist = cicn;

			cicn->next_res = (CICON *)pdata;
			prev_cicn = cicn;
			cicn = (CICON *)pdata;
		}
		if (prev_cicn)
			prev_cicn->next_res = NULL;

		cib = (CICONBLK *)cicn;
	}
	DIAG((D_rsrc, NULL, "fixed up %d color icons", numCibs));
	return maxplanes;
}

static int make_rscl_name( char *in, char *out )
{
	char *p;
	strncpy( out, in, PATH_MAX );
	p = strrchr( out, '.' );
	if( !p || stricmp( ++p, "rsc" ) )
		return 1;
	strcpy( p, "rsl" );
	return 0;
}


static int rsl_errors = 0;	// maximum alerts for invalid rsl-file
static int reported_skipped = 0;
static long rsl_lno = 1;

enum{
	NOT_FOUND=0,
	OFOUND=-1,
	TFOUND=1
};

/*
 * md=REPLACE buf is char**
 *            l=-1: don't translate
 */
static XA_FILE *rsc_lang_file( int md, XA_FILE *fp, char *buf, long l )
{
	switch( md )
	{
		case OREAD:
			rsl_errors = 0;
			rsl_lno = 1;
			reported_skipped = 0;
			BLOG((0,"open lang-file '%s'", buf ));
		return xa_fopen( buf, O_RDONLY );
		case OWRITE:
		return xa_fopen( buf, O_WRONLY|O_CREAT|O_TRUNC);
		case READ:
		return 0;
		case WRITE:
			if (fp)
			{
				xa_writeline( buf, l, fp );
			}
		return 0;
		case CLOSE:
			xa_fclose(fp);
		return 0;
		case REPLACE:
		{
			char lbuf[256], *p = 0, *in = *((char**)buf);
			bool strip = true;
			int len = strlen(in), found;
			short blen = 0, clen, cmplen, *chgl = 0;

			if( l > 0 )
				chgl = (short*)l;
			// BLOG((0,"trans: looking '%s'", in ));
			if( chgl && *chgl == 2 )
			{
				strip = false;
			}

			if( !len )
				return 0;
			/* strip trailing blanks */
			for( clen = len - 1; clen && in[clen] == ' '; clen-- );
			/*if( clen == 0 && *in == ' ' )	// blank line
				return (XA_FILE*)(long)len;
			*/
			clen++;

			for( found = NOT_FOUND, lbuf[0] = 0; found == NOT_FOUND && lbuf[0] != -1; )
			{
				for( lbuf[0] = 0; lbuf[0] != LF_SEPCH ; )
				{
					if( !xa_readline( lbuf, sizeof(lbuf)-1, fp ) ){
						lbuf[0] = -1;
						break;
					}
					rsl_lno++;
					if( lbuf[0] == LF_COMMCH )
						continue;

					if( lbuf[0] == LF_SEPCH )
						break;

					cmplen = clen;

					p = lbuf + LF_OFFS;
					blen = strlen( p );

					/* input-length */
					if( lbuf[blen+LF_OFFS-1] == '@' && lbuf[blen+LF_OFFS-2] == ' ' )
					{
						blen--;
						cmplen = len;
					}
					lbuf[blen+LF_OFFS] = 0;

					if( found == 0 )
					{
						if( l == -1 )
						{
							found = OFOUND;
							break;
						}
						if( !strnicmp( lbuf, "nn", 2 ) )
						{
							if( (*p == '-' && *in == '-') || !strncmp( p, in, cmplen ) )
							{
								found = OFOUND;
								if( lbuf[LF_OFFS-1] == ';')	// dont translate this
									break;
								if( lbuf[LF_OFFS-1] == '+' )	// realloc
									strip = false;
							}
							else if( reported_skipped == 0 )
							{
								BLOG((0,"%ld:'%s' skipped (expected:'%s')", rsl_lno-1, p, in ));
								reported_skipped = 1;
							}
						}
					}
					else if( !strncmp( lbuf, cfg.lang, 2 ) )
					{
						/* copy shorter length */
						if( blen > len && strip == true )
							blen = len;
						found = TFOUND;
						break;	// translation found
					}
				}
			}

			if( found == TFOUND && p && lbuf[0] != -1 && lbuf[0] != LF_SEPCH )	// found
			{
				if( blen > len && strip == false )
				{
					in = xa_strdup( p );
					*((char**)buf) = in;
				}
				else
				{
					if( blen > 0 )
					{
						in[blen] = 0;
						memcpy( in, p, blen );
					}
					else
						blen = len;
				}
			}
			else
				blen = clen;

			while( lbuf[0] != LF_SEPCH )
			{
				if( !xa_readline( lbuf, sizeof(lbuf)-1, fp ) )
				{
					break;
				}
				rsl_lno++;
			}
			if( chgl && *chgl != 2 )
			{
				*chgl = (strip == false);
			}
			// BLOGif(!found,(0,"rsc_lang_file:'%s' not found/%d", in, len ));
			switch( found )
			{
			case TFOUND:
				return (XA_FILE*)(long)blen;
			case OFOUND:
				return (XA_FILE*)(long)-blen;
			default:
				return 0;
			}

		}
		default:
		return 0;
	}
}

static short translate_string( struct xa_client *client, XA_FILE *rfp, char **p, long i )
{
	short blen, j;

	if( !p )
		return 0;

	for( blen = j = 0; j < 2 && !blen; j++ )
	{
		if( (blen = (short)(long)rsc_lang_file( REPLACE, rfp, (char*)p, i)))
			break;	//found
		if( 1 || rsl_errors++ < RSL_MAX_ERRORS )
		{
			 BLOG((0,"translate:'%s':rewind", *p ));
			xa_rewind( rfp );
			rsl_lno = 1;
			reported_skipped = -1;
		}
		else
			break;	// give up
	}
	if( !blen )
	{
		//ALERT(("translate: item '%s' not found", *p ));
		BLOG((0, "%s:translate: item '%s': not found", client->name, *p ));
	}
	return blen;
}

static short rsc_trans_rw( struct xa_client *client, XA_FILE *rfp, char **txt, long l )
{
	short blen = 0;
	char buf[256];

	if( !rfp || !txt || !*txt || !**txt )
		return 0;

	if( client->options.rsc_lang == WRITE )
	{
		int len = sprintf( buf, sizeof(buf), "nn:%s", *txt);

		if( *(buf + len - 1) == ' ' )
		{
			*(buf + len) = '@';
			len++;
			*(buf + len) = 0;
		}
		blen = len;
		rsc_lang_file( WRITE, rfp, buf, len );
		rsc_lang_file( WRITE, rfp, LF_SEPSTR, 1 );

	}
	else if( client->options.rsc_lang == READ )
	{
		blen = translate_string( client, rfp, txt, l );
	}

	return blen;
}


#define MAX_TITLES 32

struct rsc_scan
{
	short n_titles, title, lastbox, mwidth, keep_w, change_lo[MAX_TITLES], shift[MAX_TITLES];
	char *titles[MAX_TITLES];
};

static void
fix_objects(struct xa_client *client,
			struct xa_rscs *rscs,
			CICONBLK **cibh,
			short vdih,
			void *b,
			OBJECT *obj,
			unsigned long n,
			XA_FILE *rfp
)
{
	unsigned long i;
	short type;
	struct rsc_scan scan;

	memset( &scan, 0, sizeof(scan) );

	DIAG((D_rsrc, client, "fix_objects: b=%lx, cibh=%lx, obj=%lx, num=%ld",
		b, cibh, obj, n));

	if( client->options.rsc_lang == WRITE && n && rfp)
	{
		rsc_lang_file( WRITE, rfp, "# - Objects -", 13 );
	}
	/* fixup all objects' ob_spec pointers */
	for (i = 0; i < n; i++, obj++)
	{
		type = obj->ob_type & 255;

		/* What kind of object is it? */
		switch (type)
		{
			case G_IMAGE:
			case G_BOXTEXT:
			case G_FBOXTEXT:
			case G_TEXT:
			case G_FTEXT:

			case G_BUTTON:
			case G_STRING:
			case G_SHORTCUT:
			case G_TITLE:
			{
				obj->ob_spec.free_string += (long)b;
				if( client->options.rsc_lang && !(type == G_IMAGE) )
				{
					char **p = 0;
					short blen, chgl;

					if( object_has_tedinfo(obj) )
					{
						TEDINFO *ted = object_get_tedinfo(obj, NULL);
						if( ted )
						{
							p = &ted->te_ptext;
						}
					}
					else
						p = &obj->ob_spec.free_string;

					if( !p || !*p || !**p || strmchr(*p, "\r\n") )
						break;

					if( client->options.rsc_lang == WRITE )
					{
						if( obj->ob_type == G_TITLE )
						{
							if( scan.n_titles == 0 )
								rsc_lang_file( WRITE, rfp, "### Menubar ###", 15 );

							scan.titles[scan.n_titles++] = *p;
							scan.title = 0;
						}
						else if( obj->ob_type == G_STRING && scan.title > 0 && scan.titles[scan.title-1] )
						{
							char buf[256];
							int l = sprintf( buf, sizeof(buf)-1, "### - \"%s\"-menu - ###", scan.titles[scan.title-1] );
							rsc_lang_file( WRITE, rfp, buf, l );
							scan.titles[scan.title-1] = 0;
						}

					}

					blen = rsc_trans_rw( client, rfp, p, obj->ob_type == G_TITLE ? (long)&chgl : (scan.title > 0 ? (long)&scan.change_lo[scan.title-1] : 0) );

					if( blen )
					{
						if( client->options.rsc_lang == READ )
						{
							if( blen > 0 )	// orig and trans found
							{
								if( obj->ob_type == G_BUTTON && (obj->ob_flags & OF_SELECTABLE) )
								{
									obj->ob_state &= (0x80ff);	// make own shortcuts
								}
							}
							else
								blen = -blen;		// only orig found

							if( obj->ob_type == G_STRING )	//&& ( scan.title > 0 && scan.change_lo[scan.title-1]) )
							{
								if( **p != '-' )
								{
									if( *(*p + blen - 1) != ' ' )
										blen++;
									if( obj->ob_x > 0 )
										scan.keep_w = 1;	// more than one obj in one line -> don't adjust width
									if( blen + obj->ob_x > scan.mwidth )
									{
										scan.mwidth = blen + obj->ob_x;
									}
								}

							}
							else if( obj->ob_type == G_TITLE && scan.n_titles >= 0 )
							{
								short s;

								if( *(*p + blen - 1) != ' ' )
									blen++;
								s = blen - obj->ob_width;
								scan.title = 0;
								if( scan.n_titles > 0 )
									obj->ob_x += scan.shift[scan.n_titles-1];

								if( s && chgl == true )
								{
									obj->ob_width = blen;
									scan.shift[scan.n_titles] = s;
								}
								scan.change_lo[scan.n_titles] = chgl * 2;

								if( scan.n_titles > 0 && scan.n_titles < MAX_TITLES )
								{
									scan.shift[scan.n_titles] += scan.shift[scan.n_titles-1];
								}
								scan.n_titles++;
							}
						}	// /if( client->options.rsc_lang == READ )
					}	// /if( blen )
				}	// /if( client->options.rsc_lang && !(type == G_IMAGE) )
				break;
			}
			case G_ICON:
			{
				ICONBLK *ib;
				obj->ob_spec.free_string += (long)b;
				ib = obj->ob_spec.iconblk;

				if( client != C.Aes && screen.c_max_h < 16 )
					ib->ib_hicon /= (160 / (screen.c_max_h * 10));
				break;
			}
			case G_CICON:
			{
				/*
				 * Ozk: Added xa_rsc * parameter to FixColourIconData(),
				 * since its needed to remember memory allocs.
				 */
				if (cibh)
				{
			#if GENERATE_DIAGS
					long idx = obj->ob_spec.index;
			#endif
					FixColourIconData(client, cibh[obj->ob_spec.index], rscs, vdih);
					obj->ob_spec.ciconblk = cibh[obj->ob_spec.index];
					DIAG((D_rsrc, client, "ciconobj: idx=%ld, ciconblk=%lx (%lx)",
						idx, cibh[idx], obj->ob_spec.ciconblk));
				}
			#if GENERATE_DIAGS
				else
					DIAG((D_rsrc, client, "No cicons but G_CICON object???"));
			#endif
				break;
			}
			case G_PROGDEF:
			{
				obj->ob_spec.userblk = NULL;
				break;
			}
			case G_BOX:
				if( client->options.rsc_lang == WRITE )
				{
					if( scan.title >= 0 )
					{
						if( scan.title < MAX_TITLES )
							scan.title++;
					}
					else
					{
						if( obj->ob_next == -1 )
						{
							rsc_lang_file( WRITE, rfp, "### - TREE - ###", 16 );
						}
					}
				}
				else if( client->options.rsc_lang == READ && scan.title >= 0 )
				{
					if( scan.mwidth && scan.lastbox > 0 )	// last entry in menu-box
					{
						OBJECT *o = (obj - i + scan.lastbox);
						o->ob_width = scan.mwidth;
						if( scan.title > 1 )
							o->ob_x += scan.shift[scan.title-2];

						if( scan.keep_w == 0 )
							for( o++; o < obj && o->ob_type == G_STRING; o++ )
							{
								o->ob_width = scan.mwidth;
							}

						scan.mwidth = scan.keep_w = 0;
					}
					scan.title++;
				}
				scan.lastbox = i;
				if( scan.n_titles > 0 && scan.title > scan.n_titles )
				{
					scan.title = -1;
					scan.n_titles = -1;
				}

			case G_IBOX:
				if( i > 0 && (obj-1)->ob_type == G_BOX && i + 1 < n && (obj+1)->ob_type == G_TITLE )
				{
					memset( &scan, 0, sizeof(scan) );
				}
			case G_BOXCHAR:
			{
				break;
			}
			default:
			{
				DIAG((D_rsrc, client, "Unknown object type %d", type));
				break;
			}
		}
	}
	DIAG((D_rsrc, client, "fixed up %ld objects ob_spec (end=%lx)", n, obj));
}

#define resWidth (screen.c_max_w)
#define resHeight (screen.c_max_h)

static void
fix_trees(struct xa_client *client, void *b, OBJECT **trees, unsigned long n, short designWidth, short designHeight)
{
	unsigned long i;
	unsigned long x_fact, y_fact;
	int j, k;
	OBJECT *obj;

	x_fact = (long)screen.c_max_w << 16;
	x_fact /= designWidth;
	y_fact = (long)screen.c_max_h << 16;
	y_fact /= designHeight;

	DIAG((D_rsrc, NULL, "fix_trees: trees=%lx (%lx)", trees, trees[0]));
	for (i = 0; i < n; i++, trees++)
	{
		if (*trees != (OBJECT *)-1)
		{
			*trees = (OBJECT *)(*(char **)trees + (long)b);

			DIAGS((" -- tree[%ld]>%ld = %lx", i, (unsigned long)*trees-(unsigned long)b, *trees));

			obj = *trees;
			k = 0;

			if ((obj[3].ob_type & 255) != G_TITLE)
			{
				/* Not a menu tree */
				do {
					/* Fix all object coordinates */
					unsigned short *c;
					unsigned short ch, cl;
					unsigned long temp;
					DIAGS((" -- obj %d, type %x (n=%d, h=%d, t=%d)",
						k, obj->ob_type, obj->ob_next, obj->ob_head, obj->ob_tail));

					c = (unsigned short *)&obj->ob_x;

					switch (obj->ob_type & 0xff)
					{
						case G_TEXT:
						case G_BOXTEXT:
						case G_FTEXT:
						case G_FBOXTEXT:
						{
							TEDINFO *ted;

							ted = (TEDINFO *)obj->ob_spec.index;
							if (ted->te_ptext == (char *)-1L)
							{
								((XTEDINFO *)ted->te_ptmplt)->o = aesobj(*trees, k);
//								set_aesobj(&((XTEDINFO *)ted->te_ptmplt)->o, *trees, k); //->index = k;
							}
							//else
								//bootlog((0,"%d:%s", obj->ob_type, ted->te_ptmplt));
							break;
						}
						default:;
					}
					/*
					 * Coordinates: arranged as number of chars in low byte,
					 * with a pixel correction in the upper byte.
					 * Pixel correction is a signed byte, giving a negative
					 * pixel addition to the character coordinate in the lower
					 * byte
					 */
					for (j = 0; j < 4; j++)
					{
						cl = (*c & 0xff);
						ch = (*c >> 8) & 0x00ff;
						if (ch > 128) ch -= 256;
						if (j & 1)
						{
							cl *= designHeight;
							cl += ch;
							temp = y_fact * cl;
						}
						else
						{
							cl *= designWidth;
							cl += ch;
							temp = x_fact * cl;
						}
						if (temp & 0x8000)
							temp += 0x00010000L;
						*c++ = (temp >> 16);
						/*if( j == 3 && screen.c_max_h < 16 && (obj->ob_type == G_CICON || obj->ob_type == G_ICON || obj->ob_type == G_IBOX) )
						{
							*(c-1) *= (16 / screen.c_max_h);
						}
						*/
					}
					k++;
				}
				while (!(obj++->ob_flags & OF_LASTOB));

				if ( (client->options.alt_shortcuts & ALTSC_DIALOG) && !((*trees)->ob_state & OS_WHITEBAK) )
				{
					ob_fix_shortcuts(*trees, false, 0);
					//sc_exit();
				}
			}
			else
			{
				/* Standard AES menu */
				j = 0;
				do {
					obfix(obj, j, designWidth, designHeight);
				}
				while (!(obj[j++].ob_flags & OF_LASTOB));
			}
		}
	}
	DIAGS((" -- fix_trees: done"));
}


static struct xa_rscs *
list_resource(struct xa_client *client, void *resource, short flags)
{
	struct xa_rscs *new;

	if (client == C.Aes || client == C.Hlp)
		new = kmalloc(sizeof(*new));
	else
		new = umalloc(sizeof(*new));

	DIAG((D_x, client, "XA_alloc 2 %ld, at %lx", sizeof(*new), new));

	if (new)
	{
		DIAG((D_rsrc, client, "list_resource %ld(%lx) for %s rsc:%ld(%lx) rscs %lx",
			new, new, c_owner(client), resource, resource, client->resources));

		/* hook it to the chain (double linked list) */
		new->next = client->resources;
		new->prior = NULL;
		if (new->next)
			new->next->prior = new;

		client->resources = new;

		/* set defaults up */
		new->palette = NULL;

		new->id = 2;
		new->handle = client->rsct;
		new->flags = flags;
		new->rsc = resource;
		new->ra = NULL;
		new->globl = client->globl_ptr;
	}
	return new;
}


/*
 * LoadResources: Load a GEM resource file
 * fname = name of file to load
 * Return = base pointer of resources or NULL on failure
 */
RSHDR * _cdecl
LoadResources(struct xa_client *client, char *fname, RSHDR *rshdr, short designWidth, short designHeight, bool set_pal)
{

	RSHDR *hdr = NULL;
	CICONBLK **cibh = NULL;
	unsigned long osize = 0, size = 0, extra = 0;
	unsigned long sz;
	char *base = NULL;	//, *end = NULL;
	char *extra_ptr = NULL;
	struct xa_rscs *rscs = NULL;
	short vdih;
	XA_FILE *rfp = 0;

	if (!client)
		client = C.Aes;

	vdih = client->vdi_settings->handle;

	// BLOG((0,"%s:LoadResources,fname=%s,designWidth=%d, designHeight=%d", client->name, fname?fname:"", designWidth, designHeight));
	if (fname)
	{
		struct file *f;
		long err, fsize;
		XATTR x;

		DIAG((D_rsrc, client, "LoadResources(%s)", fname));

		f = kernel_open(fname, O_RDONLY, &err, &x);

		if (!f)
		{
			DIAG((D_rsrc, client, "LoadResources(): file not found"));
			return NULL;
		}

		fsize = x.size;

		if ((client->options.app_opts & XAAO_OBJC_EDIT))
		{
			hdr = kmalloc(sizeof(*hdr));
			if (!hdr)
			{
				DIAG((D_rsrc, client, "LoadResources(): kmalloc failed, out of memory?"));
				kernel_close(f);
				return NULL;
			}
			size = kernel_read(f, hdr, sizeof(*hdr));
			if (size != sizeof(*hdr))
			{
				DIAG((D_rsrc, client, "LoadResource(): Error reading file '%s'", fname));
				kernel_close(f);
				return NULL;
			}
			if (hdr->rsh_nted)
			{
				extra = sizeof(XTEDINFO) * hdr->rsh_nted;
			}
			kfree(hdr);
			kernel_lseek(f, 0, 0);
		}

		if (client == C.Aes || client == C.Hlp)
			base = kmalloc(fsize + extra + sizeof(RSXHDR));
		else
			base = umalloc(fsize + extra + sizeof(RSXHDR));

		if (extra)
		{
			extra_ptr = base + fsize + sizeof(RSXHDR);
		}

		sz = size = kernel_read(f, base, fsize);
		kernel_close(f);
		if (size != fsize)
		{
			DIAG((D_rsrc, client, "LoadResource(): Error loading file (size mismatch)"));
			BLOG((0, "LoadResource(): Error loading file (size mismatch)"));
			if (client == C.Aes || client == C.Hlp)
				kfree(base);
			else
				ufree(base);
			return NULL;
		}

		hdr = (RSHDR *)base;
		client->rsrc = hdr;
		size = (unsigned long)hdr->rsh_rssize;
		osize = (size + 1UL) & 0xfffffffeUL;

		if (hdr->rsh_vrsn & 4)
		{
			size = *(unsigned long *)(base + osize);
		}
		if (size != sz )
		{
			char s[256];
			if( client == C.Aes )
			{
				BLOG((1, "XAAES:Error loading file '%s':(size mismatch)", fname));
			}
			else
			{
				DIAG((D_rsrc, client, "LoadResource(): Error loading file (size mismatch)"));
				sprintf( s, 255, xa_strings[RS_RSCSZ], fname, sz, size );

				if ( !client->options.ignore_rsc_size && (xaaes_do_form_alert( 0, client, 2, s ) == 1) )
					client->options.ignore_rsc_size = 1;
			}
			if( client->options.ignore_rsc_size )
			{
				if( size > sz )
					size = sz;
			}
			else
			{
				if (client == C.Aes || client == C.Hlp)
					kfree(base);
				else
					ufree(base);
				return NULL;
			}
		}
		//end = base + size;
		/*
		 * Ozk: Added 'flags' to xa_rscs structure, so we know
		 * if the resource associated with it was allocated by
		 * XaAES or not. Added this parameter to list_resource().
		 * list_resource() also return the address of the new
		 * xa_rscs attached to the client for further references.
		 */
		client->rsct++;
		if (!(rscs = list_resource(client, base, RSRC_ALLOC)))
		{
			client->rsct--;

			if (client == C.Aes || client == C.Hlp)
				kfree(base);
			else
				ufree(base);
		}
	}
	else
	{
		DIAG((D_rsrc, client, "LoadResources %ld(%lx)", rshdr, rshdr));
		if (rshdr)
		{
			hdr = rshdr;
			base = (char *)rshdr;
			size = (unsigned long)hdr->rsh_rssize;
			osize = (size + 1UL) & 0xfffffffeUL;
			if (hdr->rsh_vrsn & 4)
				size = *(unsigned long *)(base + osize);

			/*	no chance to check for correct rsc-size if loaded from memory!
			if( size > sz )
			{
				BLOG((1,"LoadResources:%s: wrong size (%ld,%ld)!", fname, sz, size ));
				return NULL;
			}
			*/
			client->rsct++;
			rscs = list_resource(client, base, 0);
			//end = base + size;
		}
	}


	if (!rscs)
	{
		return NULL;
	}

	if( client->options.rsc_lang && (client != C.Aes || info_tab[3][0]) )
	{
		char rscl_name[PATH_MAX];
		if( !make_rscl_name( fname, rscl_name ) )
		{
			if( client->options.rsc_lang == READ )
			{
				if( !( rfp = rsc_lang_file( OREAD, 0, rscl_name, 0)) )
					client->options.rsc_lang = WRITE;
			}
			if( client->options.rsc_lang == WRITE )
			{
				if( !(rfp = rsc_lang_file( OWRITE, 0, rscl_name, 0)) ){
					BLOG((0,"LoadResources: could not open to write:%s.", rscl_name));
					client->options.rsc_lang = 0;
				}
				else
				{
					char buf[256];
					int l = sprintf( buf, sizeof(buf)-1, "#%s: Language-File for %s\n# %s", rscl_name, client->name, C.Aes->name );
					rsc_lang_file( WRITE, rfp, buf, l );
					BLOG((0,"%s: Language-File for %s created", rscl_name, client->name ));
				}
			}
		}
		else{
			client->options.rsc_lang = 0;
		}
	}

	fix_chrarray(client, base, (char **)	(base + hdr->rsh_frstr),	 hdr->rsh_nstring, &extra_ptr, rfp );
	fix_chrarray(client, base, (char **)	(base + hdr->rsh_frimg),	 hdr->rsh_nimages, &extra_ptr, 0);
	fix_tedarray(client, base, (TEDINFO *)	(base + hdr->rsh_tedinfo), hdr->rsh_nted, &extra_ptr);
	fix_icnarray(client, base, (ICONBLK *)	(base + hdr->rsh_iconblk), hdr->rsh_nib, &extra_ptr);
	fix_bblarray(client, base, (BITBLK *) (base + hdr->rsh_bitblk),  hdr->rsh_nbb, &extra_ptr);

	if (hdr->rsh_vrsn & 4)
	{
		/* It's an enhanced RSC file */
		short maxplanes = 0;
		unsigned long *earray; //, *addr;

		DIAG((D_rsrc, client, "Enhanced resource file"));

		/* this the row of pointers to extensions */
		/* terminated by a 0L */
		earray = (unsigned long *)(base + osize);
		earray++; 					/* ozk: skip filesize */
		if (*earray && *earray != -1L) /* Get colour icons */
		{
			cibh = (CICONBLK **)(base + *earray);
			maxplanes = fix_cicons(client, base, cibh, &extra_ptr);
		}

		if (*earray)
			earray++;

		/* Ozk: I need help here! Need a foolproof way of checking if the palette
		 *	present in the resource file is a valid one. The criterias I've
		 *	implemented here is;
		 *	1. The resource must have 256 color cicons - else the palette is
		 *		 not considered at all.
		 *	2. The first 16 palette entries are checked and only valid if
		 *		 there are 16 unique pen indexes.
		 *	3. If check 2 passes, check if there are valid RGB data in the
		 *		 remaining palette entries. This simply means the check fails
		 *		 if there is only 0 for r, g and b levels .. not a very reliable
		 *		 test.
		 */
		if (maxplanes >= 8 && *earray && *earray != -1L)
		{
			struct xa_rsc_rgb *p;
			int i;
			unsigned short mask;
			bool pal = false;

			DIAG((D_rsrc, client, "Color palette present"));
			p = (struct xa_rsc_rgb *)(base + *earray);

			/* Ozk: First we check if the first 16 pens have valid data
			 *	Valid data means two palette entries can not index
			 *	the same pen index
			 */
			mask = 0;
			for (i = 0; i < 16; i++)
			{
				short pen = p[i].pen;
				if (pen >= 0 && pen < 256)
				{
					if (pen == 255)
						pen = 1;
					if ((mask & (1 << pen)))
						break;
					else
						mask |= 1 << pen;
				}
				else
					break;
			}
			/*
			 * then we check if the remaining palette have valid RGB data...
			 * I dont know if this check is good enough, but...
			 */
			for (i = 16; i < 256; i++)
			{
				if ((p[i].red || p[i].green || p[i].blue))
				{
					pal = true;
					break;
				}
			}

			if (pal && mask == 0xffff)
			{
				DIAG((D_rsrc, client, "%s got palette", fname ? fname : "noname"));
				rscs->palette = (void *)(base + *earray);

				fix_rsc_palette((struct xa_rsc_rgb *)rscs->palette);
				if (set_pal && cfg.set_rscpalette)
				{
					set_syspalette(C.P_handle, rscs->palette);
					get_syspalette(C.P_handle, screen.palette);
				}
			}
		}
#if 0
		/* ozk: Enable this when (if) more extensions are added ... */
		if (*earray)
			earray++;
#endif
	}

	/*
	 * fix_objects MUST run before fix_trees!!!
	 */
	fix_objects(client, rscs, cibh, vdih, base, (OBJECT *)(base + hdr->rsh_object), hdr->rsh_nobs, rfp );

	if( rfp ){
		if( client == C.Aes && trans_strings[0] && client->options.rsc_lang )
		{
			int i;
			char **t;

			if( client->options.rsc_lang == WRITE )
			{
				rsc_lang_file( WRITE, rfp, "# - Internal Strings -", 22 );
			}
			for(i = 0; trans_strings[i]; i++)
			{
				//char *t_1 = 0;
				for( t = trans_strings[i]; *t; t++)
				{
					if( **t )
					{
						char **t_2;
						short ptr_seen = 0;

						/* strings might be merged
						 * gcc4 puts all strings in the same order in the out-file as they are in the source
						 * gcc2 (and maybe also others) uses another order, so a search for the current string is done
						 */
						for( t_2 = trans_strings[i]; t_2 < t; t_2++)
							if( *t_2 == *t )
							{
								ptr_seen = -1;
								break;
							}

						rsc_trans_rw( client, rfp, t, ptr_seen );
						//t_1 = *t;
					}
				}
			}
			trans_strings[0] = 0;	// no further translation possible
		}
		rsc_lang_file( CLOSE, rfp, 0, 0 );
	}
	if( client == C.Aes )
		client->options.rsc_lang = 0;	// dont translate other rsc-files
	fix_trees(client, base, (OBJECT **)(base + hdr->rsh_trindex), hdr->rsh_ntree, designWidth, designHeight);

	return (RSHDR *)base;
}

static void
Rsrc_setglobal(RSHDR *h, struct aes_global *gl)
{
	if (gl)
	{
		OBJECT **o;
		o = (OBJECT **)((char *)h + h->rsh_trindex);

		/* Fill in the application's global array with a pointer to the resource */
		gl->ptree = o;
		gl->rshdr = h;
		gl->lmem = h->rsh_rssize;

		DIAGS(("Resources %ld(%lx) in global[5,6]", o, o));
		DIAGS(("			and %ld(%lx) in global[7,8]", h, h));
	}
}

#if 0
void
dump_ra_list(struct xa_rscs *rscs)
{
	struct remember_alloc *ra;

	while (rscs)
	{
		ra = rscs->ra;
		display("\r\ndump ra-list for rscs=%lx, first ra=%lx", rscs, ra);
		while (ra)
		{
			display("  ra=%lx, ra->next=%lx, ra->addr=%lx", ra, ra->next, ra->addr);
			ra = ra->next;
		}
		rscs = rscs->next;
	}
}
#endif

/*
 * FreeResources: Dispose of a set of loaded resources
 */
void
FreeResources(struct xa_client *client, AESPB *pb, struct xa_rscs *rsrc)
{
	struct xa_rscs *cur = client->resources;
#if GENERATE_DIAGS
	RSHDR *rsc = NULL;

	if (pb && pb->global)
		rsc = ((struct aes_global *)pb->global)->rshdr;

	DIAG((D_rsrc,client,"FreeResources: %lx for %d, ct=%d, pb->global->rsc=%lx",
		cur, client->p->pid, client->rsct, rsc));
#endif
	if (cur && client->rsct)
	{
		/* Put older rsrc back, and the pointer in global */
		while (cur)
		{
			//bool have = rsc && (rsc == cur->rsc);
			struct xa_rscs *nx = cur->next;

			if (!rsrc || rsrc == cur)
			{
				DIAG((D_rsrc, client, "Free: test cur %lx", (long)cur));
				DIAG((D_rsrc, client, "Free: test cur handle %d", cur->handle));

				//if (have || (!rsc && cur->handle == client->rsct))
				{
					/* free the entry for the freed rsc. */
					struct xa_rscs *nxt_active = NULL;
					RSHDR *hdr = cur->rsc;
					char *base = cur->rsc;
					OBJECT **trees;
					short i;

					/* Free the memory allocated for scroll list objects. */
					trees = (OBJECT **)(base + hdr->rsh_trindex);
					for (i = 0; i < hdr->rsh_ntree; i++)
					{
						free_obtree_resources(client, trees[i]);
					}

					/* unhook the entry from the chain */
					if (cur->prior)
					{
						cur->prior->next = cur->next;
						nxt_active = cur->prior;
					}
					else
						client->resources = cur->next;

					if (cur->next)
					{
						if (!nxt_active)
							nxt_active = cur->next;
						cur->next->prior = cur->prior;
					}

					if (client->rsrc == hdr)
					{
						RSHDR *nxt_hdr = NULL;
						OBJECT **o;

						if (nxt_active)
							nxt_hdr = nxt_active->rsc;

						client->rsrc = nxt_hdr;
						o = nxt_hdr ? (OBJECT **)((char *)nxt_hdr + nxt_hdr->rsh_trindex) : (OBJECT **)NULL;
						client->trees = o;

						Rsrc_setglobal(client->rsrc, client->globl_ptr);
						if (pb && pb->global && (struct aes_global *)pb->global != client->globl_ptr)
							Rsrc_setglobal(client->rsrc, (struct aes_global *)pb->global);
					}
					/*
					 * Ozk: Here we fix and clean up a LOT of memory-leaks.
					 * Now we remember each allocation done related to loading
					 * a resource file, and we release the stuff here.
					 */
					if (client == C.Aes || client == C.Hlp)
					{
						struct remember_alloc *ra;

						while (cur->ra)
						{
							ra = cur->ra;
							cur->ra = ra->next;
							DIAG((D_rsrc, client, "kFreeRa: addr=%lx, ra=%lx", ra->addr, ra));
							kfree(ra->addr);
							kfree(ra);
						}

						if (cur->flags & RSRC_ALLOC)
						{
							DIAG((D_rsrc, client, "kFree: cur->rsc %lx", cur->rsc));
							kfree(cur->rsc);
						}
						DIAG((D_rsrc, client, "kFree: cur %lx", cur));

						kfree(cur);
					}
					else
					{
						struct remember_alloc *ra;

						while (cur->ra)
						{
							ra = cur->ra;
							cur->ra = ra->next;
							DIAG((D_rsrc, client, "uFreeRa: addr=%lx, ra=%lx", ra->addr, ra));
							ufree(ra->addr);
							kfree(ra);
						}
						if (cur->flags & RSRC_ALLOC)
						{
							DIAG((D_rsrc, client, "uFree: cur->rsc %lx", cur->rsc));
							ufree(cur->rsc);
						}
						DIAG((D_rsrc, client, "uFree: cur %lx", cur));

						ufree(cur);
					}
					nx = NULL;
					client->rsct--;
				}
			}
			cur = nx;
		}
	}
}


/*
 * The routines below are almost entirely rewritten such, that it is
 * quite easy to see the subtle differences.
 *
 * hdr = pointer to base of resources
 * num = index number of tree
 * Return = pointer to tree or object or stuff, or NULL on failure
 */

#define num_nok(t) (!hdr || num < 0 || num >= hdr->rsh_ ## t)
#define start(t) (unsigned long)index = (unsigned long)hdr + hdr->rsh_ ## t

void hide_object_tree( RSHDR *rsc, short tree, short item, int Unhide )
{
	OBJECT *obtree = ResourceTree( rsc, tree );
	if( Unhide )
		obtree[item].ob_flags &= ~OF_HIDETREE;
	else
		obtree[item].ob_flags |= OF_HIDETREE;
}
/*
 * Find the tree with a given index
 * fixing up the pointer array is now done in Loadresources, to make it usable via global[5]
 */
OBJECT * _cdecl
ResourceTree(RSHDR *hdr, long num)
{
	OBJECT **index;
	DIAGS(("ResourceTree: hdr = %lx, num = %ld", hdr, num));
	if (num_nok(ntree))
		return NULL;

	index = (OBJECT **)((char *)hdr + hdr->rsh_trindex);
	return index[num];
}

/* Find the object with a given index  */
static OBJECT *
ResourceObject(RSHDR *hdr, int num)
{
	OBJECT *index;

	if (num_nok(nobs))
		return NULL;

	index = (OBJECT *)((char *)hdr + hdr->rsh_object);//	start(object);
	return index + num;
}

/* Find the tedinfo with a given index */
static TEDINFO *
ResourceTedinfo(RSHDR *hdr, int num)
{
	TEDINFO *index;

	if (num_nok(nted))
		return NULL;

	index = (TEDINFO *)((char *)hdr + hdr->rsh_tedinfo); //start(tedinfo);

	index += num;

	if (index->te_ptext == (char *)-1L)
		index = &((XTEDINFO *)index->te_ptmplt)->ti;

	return index;
}

/* Find the iconblk with a given index */
static ICONBLK *
ResourceIconblk(RSHDR *hdr, int num)
{
	ICONBLK *index;

	if (num_nok(nib))
		return NULL;

	index = (ICONBLK *)((char *)hdr + hdr->rsh_iconblk); //start(iconblk);
	return index + num;
}

/* Find the bitblk with a given index */
static BITBLK *
ResourceBitblk(RSHDR *hdr, int num)
{
	BITBLK *index;

	if (num_nok(nbb))
		return NULL;

	index = (BITBLK *)((char *)hdr + hdr->rsh_bitblk); //start(bitblk);
	return index + num;
}

/* Find the string with a given index */

/* HR: I think free_strings are the target; The only difference
			is the return value.
		 Well at least TERADESK now works the same as with other AES's */

static char *
ResourceString(RSHDR *hdr, int num)
{
	char **index;

	if (num_nok(nstring))
		return NULL;

	if( hdr->rsh_vrsn == 3 ){
		index = (char **)((char *)hdr + ((RSXHDR*)hdr)->rsh_frstr);
	}
	else
	{
		index = (char **)((char *)hdr + hdr->rsh_frstr);
	}

//	DIAG((D_s, NULL, "Gaddr 5 %lx '%s'", index[num], index[num]));
	return index[num];
}

/* Find the image with a given index */
/* HR: images (like strings) have no fixed length, so they must be held
			 (like strings) in a pointer array, which should have been fixed up.
			 Where????? Well I couldnt find it, so I wrote it, see loader.
 */
static void *
ResourceImage(RSHDR *hdr, int num)
{
	void **index;

	if (num_nok(nimages))
		return NULL;

	index = (void **)((char *)hdr + hdr->rsh_frimg); //start(frimg);
	return index[num];
}

/* HR: At last the following 2 make sense */

/* Find the ref to the free strings array */
static char **
ResourceFrstr(RSHDR *hdr, int num)
{
	char **index;

	index = (char **)((char *)hdr + hdr->rsh_frstr); //start(frstr);

//	DIAG((D_s, NULL, "Gaddr 15 %lx '%s'", index, *index));
	return index + num;
}

/* Find the ref to free images array */
static void **
ResourceFrimg(RSHDR *hdr, int num)
{
	void **index;

	index = (void **)((char *)hdr + hdr->rsh_frimg); //start(frimg);

//	DIAG((D_s, NULL, "Gaddr 16 %lx", index));
	return index + num;
}

unsigned long
XA_rsrc_load(enum locks lock, struct xa_client *client, AESPB *pb)
{
	char *path;
	short ret = 0;

	CONTROL(0,1,1)

	/* XXX	- ozk: Because of the hack at line 526 in handler.c, we
	 * cannot allow applications with a NULL globl_ptr to use rsrc_load()
	 * until we come up with a decent solution...
	 */
	if (client->globl_ptr)
	{
/* If the client is overwriting its existing resource then better free it
	 (it shouldn't, but just in case) */
/* I don't think this is a good idea - much better to have a memory leak
	than a process continuing to access a freed memory region; GEM AES
	doesn't auto-free an existing RSC either, so this would be
	incompatible anyway... <mk>

	HR: suggestion: put them in a list, last in front. So you can free what is
			left in XA_client_exit.
			7/9/200 	done.  As well as the memory allocated for colour icon data.
*/
		path = shell_find(lock, client, (char*)pb->addrin[0]);
		if (path)
		{
			RSHDR *rsc;
			DIAG((D_rsrc, client, "rsrc_load('%s')", path));

			rsc = LoadResources(client, path, NULL, DU_RSX_CONV, DU_RSY_CONV, false);
//			rsc = LoadResources(client, path, NULL, screen.c_max_w, screen.c_max_h, false);
			if (rsc)
			{
				OBJECT **o;
//				(unsigned long)o = (unsigned long)rsc + rsc->rsh_trindex;
				o = (OBJECT **)((char *)rsc + rsc->rsh_trindex);
				client->trees = o;

#if GENERATE_DIAGS
				if (client->globl_ptr != (struct aes_global *)pb->global)
				{
					DIAGS(("WARNING: rsrc_load global is different from appl_init's global"));
				}
#endif
				if (client->globl_ptr)
					Rsrc_setglobal(rsc, client->globl_ptr);

				DIAG((D_rsrc,client,"pb %lx, gl %lx, gl->rsc %lx, gl->ptree %lx",
					pb, pb->global, ((struct aes_global *)pb->global)->rshdr, ((struct aes_global *)pb->global)->ptree));

				if (pb->global && (struct aes_global *)pb->global != client->globl_ptr)
					/* Fill it out in the global of the rsrc_load. */
					Rsrc_setglobal(rsc, (struct aes_global *)pb->global);

				ret = 1; //pb->intout[0] = 1;
//				return XAC_DONE;
			}
			kfree(path);
		}
	}
	else
	{
		/* inform user what's going on */
		ALERT((xa_strings[AL_NOGLOBPTR], "rsrc_load: ", client->name));
		//exit_client(lock, client, -1, true, true);
		raise(SIGKILL);
		yield();
		return 0;
	}

	DIAGS(("ERROR: rsrc_load '%s' %s", (pb->addrin[0]) ? (const char *)pb->addrin[0] : "~~", ret ? "suceeded":"failed"));

	pb->intout[0] = ret; //0;
	return XAC_DONE;
}
/*
 * Ozk: XA_rsrc_free() may be called by processes not yet called
 * appl_ini(), or already called appl_exit(). So, it must not
 * depend on 'client' being valid!
 */
unsigned long
XA_rsrc_free(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(0,1,0)

	if (client && client->globl_ptr)
	{
		if (client->rsrc)
		{
			FreeResources(client, pb, NULL);
			pb->intout[0] = 1;
		}
		else
			pb->intout[0] = 0;
	}
	else /* ozk: if no client, or global paramblock, we assume everythings OK */
		pb->intout[0] = 1;

	return XAC_DONE;
}

unsigned long
XA_rsrc_gaddr(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT **trees = NULL;
	RSHDR *rsc = NULL;
	void **addr = (void **)pb->addrout;
	short type = pb->intin[0], index = pb->intin[1];

	CONTROL(2,1,0)

	if (pb == NULL || client == NULL)
	{
		DIAGS(("rsrc_gaddr NULL %lx, %lx", pb, client));
	}

	DIAG((D_s,client,"rsrc_gaddr type %d, index %d pb %lx", type, index, pb));

	/* For multiple resource, first look at the supplied global ptr. */
	if (pb->global)
	{

		rsc = ((struct aes_global *)pb->global)->rshdr;
		trees = ((struct aes_global *)pb->global)->ptree;
		DIAG((D_rsrc,client,"  --  pb->gl  rsc %lx, ptree %lx", rsc, trees));
	}

	/* The below represents the last resource loaded */
	if (!rsc && !trees)
	{
		/* Nothing specified: take the last rsc read. */

		rsc = client->rsrc;
		trees = client->trees;
		DIAG((D_rsrc,client,"  --  client->gl  rsc %lx, ptree %lx", rsc, trees));
	}

	if (!rsc && trees)
	{
		DIAG((D_s,client,"Tree pointer only!"));

		if (addr && trees)
		{
			switch(type)
			{
				default:
					pb->intout[0] = 0;
					break;
				case R_TREE:
					*addr = trees[index];
					break;
			}
			DIAG((D_s,client,"	from pb->global --> %ld",*addr));
			pb->intout[0] = 1;
		}
		else
			pb->intout[0] = 0;

		return XAC_DONE;
	}

	if (!rsc && !client->rsrc)
	{
		pb->intout[0] = 0;
		if (addr)
			*addr = NULL;
		DIAG((D_rsrc,client,"NO resource loaded!!!"));
		return XAC_DONE;
	}

	client->rsrc = rsc;

	/* It is better to let the client bomb out in stead of precious us. */
	if (addr)
	{
		switch(type)
		{
		default:
			pb->intout[0] = 0;
			break;
		case R_TREE:
			if( rsc->rsh_vrsn == 3 && trees )
			{
				*addr = trees[index];
			}
			else
				*addr = ResourceTree(client->rsrc, index);
			break;
		case R_OBJECT:
			*addr = ResourceObject(client->rsrc, index);
			break;
		case R_TEDINFO:
			*addr = ResourceTedinfo(client->rsrc, index);
			break;
		case R_ICONBLK:
			*addr = ResourceIconblk(client->rsrc, index);
			break;
		case R_BITBLK:
			*addr = ResourceBitblk(client->rsrc, index);
			break;
/*!*/ 	case R_STRING:
			*addr = ResourceString(client->rsrc, index);
			break;
/*!*/ 	case R_IMAGEDATA:
			*addr = ResourceImage(client->rsrc, index);
			break;
		case R_OBSPEC:
			*addr = (void *)ResourceObject(client->rsrc, index)->ob_spec.index;
			break;
		case R_TEPTEXT:
			*addr = ResourceTedinfo(client->rsrc, index)->te_ptext;
			break;
		case R_TEPTMPLT:
			*addr = ResourceTedinfo(client->rsrc, index)->te_ptmplt;
			break;
		case R_TEPVALID:
			*addr = ResourceTedinfo(client->rsrc, index)->te_pvalid;
			break;
		case R_IBPMASK:
			*addr = ResourceIconblk(client->rsrc, index)->ib_pmask;
			break;
		case R_IBPDATA:
			*addr = ResourceIconblk(client->rsrc, index)->ib_pdata;
			break;
		case R_IBPTEXT:
			*addr = ResourceIconblk(client->rsrc, index)->ib_ptext;
			break;
		case R_BIPDATA:
			*addr = ResourceBitblk(client->rsrc, index)->bi_pdata;
			break;
/*!*/ 	case R_FRSTR:
			*addr = ResourceFrstr(client->rsrc, index);
			break;
/*!*/ 	case R_FRIMG:
			*addr = ResourceFrimg(client->rsrc, index);
			break;
		}
		DIAG((D_s,client,"	--> %lx",*addr));
		{
			char *rsc_end;
			if (rsc->rsh_vrsn == 3)
				rsc_end = (char*)rsc + ((RSXHDR*)rsc)->rsh_rssize;
			else
				rsc_end = (char*)rsc + rsc->rsh_rssize;

			if( (char*)*addr < (char*)rsc || (char*)*addr > rsc_end )
			{
				ALERT((xa_strings[AL_INVALIDP], *addr, rsc, rsc_end, type ));
				pb->intout[0] = 0;
				*addr = 0;
			}
			else
				pb->intout[0] = 1;
		}
	}
	else
		pb->intout[0] = 0;

	return XAC_DONE;
}

unsigned long
XA_rsrc_obfix(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *ob;
	int item = pb->intin[0];

	CONTROL(1,1,1)

	ob = (OBJECT*)pb->addrin[0];

	DIAG((D_rsrc, client, "rsrc_obfix for %s: tree %lx + %d",
		c_owner(client), ob, item));

	if (validate_obtree(client, ob, "XA_rsrc_fix:"))
	{
		obfix(ob, item, DU_RSX_CONV, DU_RSY_CONV);
		pb->intout[0] = 1;
	}
	else
		pb->intout[0] = 0;

	return XAC_DONE;
}

unsigned long
XA_rsrc_rcfix(enum locks lock, struct xa_client *client, AESPB *pb)
{
	RSHDR *rsc;
	CONTROL(0,1,1)

	DIAG((D_rsrc, client, "rsrc_rcfix for %s on %ld(%lx)",
		c_owner(client), pb->addrin[0], pb->addrin[0]));

	if (client->globl_ptr)
	{
		rsc = LoadResources(client, NULL, (RSHDR*)pb->addrin[0], DU_RSX_CONV, DU_RSY_CONV, false);
		if (rsc)
		{
			client->rsrc = rsc;
#if GENERATE_DIAGS
			if (client->globl_ptr != (struct aes_global *)pb->global)
			{
				DIAGS(("WARNING: rsrc_rcfix global %ld(%lx) is different from appl_init's global %ld(%lx)",
					pb->global, pb->global, client->globl_ptr, client->globl_ptr));
			}
#endif
			Rsrc_setglobal(client->rsrc, client->globl_ptr);
			if (pb->global && (struct aes_global *)pb->global != client->globl_ptr)
				Rsrc_setglobal(client->rsrc, (struct aes_global *)pb->global);

			pb->intout[0] = 1;
			return XAC_DONE;
		}
	}
	else
	{
		/* inform user what's going on */
		ALERT(( xa_strings[AL_NOGLOBPTR]/*"XaAES: rsrc_rcfix: %s, client with no globl_ptr, killing it"*/, client->name));
		//exit_client(lock, client, -1, true, true); //get_curproc());
		raise(SIGKILL);
		yield();
		return 0;
	}

	pb->intout[0] = 0;
	return XAC_DONE;
}
