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
#include "xa_global.h"

#include "draw_obj.h"
#include "trnfm.h"
#include "obtree.h"
#include "xa_appl.h"
#include "xa_rsrc.h"
#include "xa_shel.h"

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

static inline short
fixup(short val, short scale)
{
	return (val & 0xff) * scale + ((val >> 8) & 0xff);
}

void
obfix(OBJECT *tree, int object)
{
	OBJECT *o = tree + object;

	o->ob_x = fixup(o->ob_x, screen.c_max_w);
	o->ob_y = fixup(o->ob_y, screen.c_max_h);
	/*
	 * Special case handling: any OBJECT 80 characters wide is supposed
	 * to be a menu bar, which always covers the entire screen width...
	 */
	o->ob_width = (o->ob_width == 80) ? screen.r.w : fixup(o->ob_width, screen.c_max_w);
	o->ob_height = fixup(o->ob_height, screen.c_max_h);
}

	
/*
 * Code in this module is based on the resource loader from
 * Steve Sowerby's AGiLE library. Thanks to Steve for allowing
 * me to use his code.
 */

/* new function to make the code orthogonal.
 */
extern short disp;

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
			if (client == C.Aes)
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

	src.fd_w = icon->monoblk.ib_wicon; /* Transform MFDB's */
	src.fd_h = icon->monoblk.ib_hicon;
	src.fd_wdwidth = (src.fd_w + 15) / 16; /* round up */
	src.fd_stand = 1;
	src.fd_r1 = src.fd_r2 = src.fd_r3 = 0;
	src.fd_nplanes = screen.planes;
	dst = src;

	dst.fd_addr = new_data;
	dst.fd_stand = 0;
	dst.fd_nplanes = screen.planes;

	DIAG((D_x, client, "alloc of %ld bytes", new_len));
	tmp = kmalloc(new_len);
	if (tmp)
	{
		memcpy(tmp, new_data, new_len);
		src.fd_addr = tmp;
		//transform_gem_bitmap_data(vdih, src, dst, planes, screen.planes);
		transform_gem_bitmap(vdih, src, dst, planes, cfg.remap_cicons ? rscs->palette : NULL, screen.palette);
		kfree(tmp);
	}
	return new_data;
}

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
		
		if (    c->num_planes <= screen.planes
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
			c->col_data = transform_icon_bitmap(client, rscs, icon, c->col_data, len, c->num_planes, vdih);
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

static void
fix_chrarray(void *b, char **p, unsigned long n)
{
	while (n)
	{
		DIAG((D_rsrc, NULL, " -- %lx, value %lx", p, *p));
		*p += (unsigned long)b;
		DIAG((D_rsrc, NULL, " -- to %lx", *p));
		
		p++;
		n--;
	}
}
/* fixup all tedinfo field pointers */
static void
fix_tedarray(void *b, TEDINFO *ti, unsigned long n)
{
	while (n)
	{
		(unsigned long)ti->te_ptext  += (unsigned long)b;
		(unsigned long)ti->te_ptmplt += (unsigned long)b;
		(unsigned long)ti->te_pvalid += (unsigned long)b;
		DIAG((D_rsrc, NULL, "fix_tedarray: ti=%lx, ptext='%s'", ti, ti->te_ptext));
		DIAG((D_rsrc, NULL, "ptext=%lx, ptmpl=%lx, pvalid=%lx",
			ti->te_ptext, ti->te_ptmplt, ti->te_pvalid));
		
		ti++;
		n--;
	}
	
	DIAG((D_rsrc, NULL, "fixed up %ld tedinfo's", n));
}

/* fixup all iconblk field pointers */
static void
fix_icnarray(void *b, ICONBLK *ib, unsigned long n)
{
	while (n)
	{
		(unsigned long)ib->ib_pmask += (unsigned long)b;
		(unsigned long)ib->ib_pdata += (unsigned long)b;
		(unsigned long)ib->ib_ptext += (unsigned long)b;

		DIAG((D_rsrc, NULL, "fix_icnarray: ib=%lx, ib->ib_pmask=%lx, ib->ib_pdata=%lx, ib->ib_ptext=%lx",
			ib, ib->ib_pmask, ib->ib_pdata, ib->ib_ptext));
		
		ib++;
		n--;
	}
	
	DIAG((D_rsrc, NULL, "fixed up %ld iconblk's", n));
}

/* fixup all bitblk data pointers */
static void
fix_bblarray(void *b, BITBLK *bb, unsigned long n)
{
	while (n)
	{
		(unsigned long)bb->bi_pdata += (unsigned long)b;

		DIAG((D_rsrc, NULL, "fix_bblarray: bb=%lx, pdata=%lx",
			bb, bb->bi_pdata));
		bb++;
		n--;
	}
	DIAG((D_rsrc, NULL, "fixed up %ld bitblk's", n));
}

static short
fix_cicons(void *base, CICONBLK **cibh)
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

		addr = (unsigned long *)((unsigned long)cib + sizeof(ICONBLK));
		numRez = addr[0];
		pdata = (short *)&addr[1];
		/* mono data & mask */
		ib->ib_pdata = pdata;
		(unsigned long)pdata += isize;
		ib->ib_pmask = pdata;
		(unsigned long)pdata += isize;
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
			ib->ib_ptext += (unsigned long)base;
			DIAG((D_rsrc, NULL, "cicon: ib->ptext = %lx", ib->ib_ptext));
			if (strlen(ib->ib_ptext) > l)
				*(ib->ib_ptext + l) = 0;
		}
		else
			/* The following word is no of planes which cannot
			 * be larger than 32, so there is 1 zero byte there */
			ib->ib_ptext = (char *)pdata;

		/* (unused) place for name */
		(unsigned long)pdata += 12;

		cicn = (CICON *)pdata;
		prev_cicn = NULL;
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
			
			(unsigned long)pdata += sizeof(CICON);
			cicn->col_data = pdata;
			(unsigned long)pdata += psize;
			cicn->col_mask = pdata;
			(unsigned long)pdata +=  isize;
			if (cicn->sel_data)
			{
				/* It's got a selected form */
				cicn->sel_data = pdata;
				(unsigned long)pdata += psize;
				cicn->sel_mask = pdata;
				(unsigned long)pdata +=  isize;
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

static void
fix_objects(struct xa_client *client,
	    struct xa_rscs *rscs,
	    CICONBLK **cibh,
	    short vdih,
	    void *b,
	    OBJECT *obj,
	    unsigned long n)
{
	unsigned long i;
	short type;

	DIAG((D_rsrc, client, "fix_objects: b=%lx, cibh=%lx, obj=%lx, num=%ld",
		b, cibh, obj, n));

	/* fixup all objects' ob_spec pointers */
	for (i = 0; i < n; i++, obj++)
	{
		type = obj->ob_type & 255;

		/* What kind of object is it? */
		switch (type)
		{
			case G_TEXT:
			case G_BOXTEXT:
			case G_IMAGE:
			case G_BUTTON:
			case G_STRING:
			case G_SHORTCUT:
			case G_FTEXT:
			case G_FBOXTEXT:
			case G_TITLE:
			{
				obj->ob_spec.free_string += (long)b;
				break;
			}
			case G_ICON:
			{
				obj->ob_spec.free_string += (long)b;
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
			case G_IBOX:
			case G_BOX:
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

static void
fix_shortcuts(struct xa_client *client, OBJECT *obtree)
{
	struct sc
	{
		short o;
		char c;
	};
	struct sc *sc, *scuts;
	short objs;
	long len;
	char nk;

	objs = ob_count_objs(obtree, 0);

	len = ((long)objs + 1) * sizeof(struct sc);

	sc = kmalloc(len);
	
	if (sc)
	{
		int i = 0;
		bzero(sc, len);
		
		do {
			OBJECT *ob = obtree + i;
			if ((ob->ob_state & OS_WHITEBAK))
			{
				short ty = ob->ob_type & 0xff;
			
				if (ty == G_BUTTON || ty == G_STRING)
				{
					int j = (ob->ob_state >> 8) & 0x7f;
					int nc = 0;
					if (j < 126)
					{
						char *s = object_get_spec(ob)->free_string;
						if (s)
						{
							int slen = strlen(s);
							
							if (j < slen)
							{
								scuts = sc;
								nk = toupper(*(s + j));
								while (scuts->c && nc < slen)
								{
									if (scuts->c == nk)
									{
										nk = toupper(*(s + nc));
										nc++;
										scuts = sc;
									}
									else
										scuts++;
								}
								if (!scuts->c)
								{
									scuts->c = nk;
									scuts->o = i;
								}
								if (nc)
								{
									if (nc < slen)
									{
										nc--;
										ob->ob_state = (ob->ob_state & 0x80ff) | ((nc & 0x7f) << 8);
									}
									else
									{
										ob->ob_state &= (~OS_WHITEBAK & 0x80ff);
									}
								}
							}
						}
					}
				}
			}
		} while ( !(obtree[i++].ob_flags & OF_LASTOB));

		kfree(sc);
	}	
}
	
static void
fix_trees(struct xa_client *client, void *b, OBJECT **trees, unsigned long n, short designWidth, short designHeight)
{
	unsigned long i;
	int j;
	OBJECT *obj;

	DIAG((D_rsrc, NULL, "trees=%lx (%lx)", trees, trees[0]));
	for (i = 0; i < n; i++, trees++)
	{
		if (*trees != (OBJECT *)-1)
		{
			(unsigned long)*trees += (unsigned long)b;
			
			DIAG((D_s, NULL,"tree[%ld]>%ld = %lx", i, (unsigned long)*trees-(unsigned long)b, *trees));
			
			obj = *trees;

			if ((obj[3].ob_type & 255) != G_TITLE)
			{
				/* Not a menu tree */
				do {
					/* Fix all object coordinates */
#if 1
					unsigned short *c;
					short ch, cl;
				
					c = (short *)&obj->ob_x;
				
					for (j = 0; j < 4; j++)
					{
						cl = *c & 0xff;
						ch = *c >> 8;
						if (j & 1)
							cl *= designHeight;
						else
						{
							if (cl == 80 && j == 2)
								cl = screen.r.w;
							else
								cl *= designWidth;
						}
						*c++ = cl + (ch > 128 ? ch - 256 : ch);
					}
#else			
					obj->ob_x = (((obj->ob_x & 255) * designWidth + (obj->ob_x >> 8)) * resWidth) / designWidth;
					obj->ob_y = (((obj->ob_y & 255) * designHeight + (obj->ob_y >> 8)) * resHeight) / designHeight;
					obj->ob_width = (((obj->ob_width & 255) * designWidth + (obj->ob_width >> 8)) * resWidth) / designWidth;
					obj->ob_height = (((obj->ob_height & 255) * designHeight + (obj->ob_height >> 8)) * resHeight) / designHeight;
#endif			
				}
				while (!(obj++->ob_flags & OF_LASTOB));
				fix_shortcuts(client, *trees);
			}
			else
			{
				/* Standard AES menu */	

				j = 0;
				do {
					obfix(obj, j);
				}
				while (!(obj[j++].ob_flags & OF_LASTOB));
			}
		}
	}
}


static struct xa_rscs *
list_resource(struct xa_client *client, void *resource, short flags)
{
	struct xa_rscs *new;

	if (client == C.Aes)
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
void *
LoadResources(struct xa_client *client, char *fname, RSHDR *rshdr, short designWidth, short designHeight, bool set_pal)
{
#define resWidth (screen.c_max_w)
#define resHeight (screen.c_max_h)

	RSHDR *hdr = NULL;
	CICONBLK **cibh = NULL;
	unsigned long osize = 0, size = 0;
	char *base = NULL, *end = NULL;
	struct xa_rscs *rscs = NULL; 
	short vdih = client->vdi_settings->handle; //C.vh;

	if (!client)
		client = C.Aes;

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
		
		if (client == C.Aes)
			base = kmalloc(fsize + sizeof(RSXHDR));
		else
			base = umalloc(fsize + sizeof(RSXHDR));

		size = kernel_read(f, base, fsize);
		kernel_close(f);
		if (size != fsize)
		{
			DIAG((D_rsrc, client, "LoadResource(): Error loading file (size mismatch)"));
			if (client == C.Aes)	kfree(base);
			else			ufree(base);
			return NULL;
		}

		hdr = (RSHDR *)base;
		size = (unsigned long)hdr->rsh_rssize;
		osize = (size + 1UL) & 0xfffffffeUL;

		if (hdr->rsh_vrsn & 4)
		{
			size = *(unsigned long *)(base + osize);
		}
		end = base + size;
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

			if (client == C.Aes)	kfree(base);
			else			ufree(base);
		}
	}
	else
	{
		DIAG((D_rsrc, client, "LoadResources %ld(%lx)", rshdr, rshdr));
		if (rshdr)
		{
			hdr = rshdr;
			(RSHDR *)base = rshdr;
			size = (unsigned long)hdr->rsh_rssize;
			osize = (size + 1UL) & 0xfffffffeUL;
			if (hdr->rsh_vrsn & 4)
				size = *(unsigned long *)(base + osize);
			client->rsct++;
			rscs = list_resource(client, base, 0);
			end = base + size;
		}
	}
	
	if (!rscs)
		return NULL;

	fix_chrarray(base, (char **)	(base + hdr->rsh_frstr),   hdr->rsh_nstring);
	fix_chrarray(base, (char **)	(base + hdr->rsh_frimg),   hdr->rsh_nimages);
	fix_tedarray(base, (TEDINFO *)	(base + hdr->rsh_tedinfo), hdr->rsh_nted);
	fix_icnarray(base, (ICONBLK *)	(base + hdr->rsh_iconblk), hdr->rsh_nib);
	fix_bblarray(base, (BITBLK *)	(base + hdr->rsh_bitblk),  hdr->rsh_nbb);
	
	if (hdr->rsh_vrsn & 4)
	{
		/* It's an enhanced RSC file */
		short maxplanes = 0;
		unsigned long *earray; //, *addr;

		DIAG((D_rsrc, client, "Enhanced resource file"));

		/* this the row of pointers to extensions */
		/* terminated by a 0L */
		earray = (unsigned long *)(osize + (long)base);
		earray++;						/* ozk: skip filesize */
		if (*earray && *earray != -1L) /* Get colour icons */
		{
			cibh = (CICONBLK **)(*earray + (long)base);
			maxplanes = fix_cicons(base, cibh);
		}

		if (*earray)
			earray++;

		/* Ozk: I need help here! Need a foolproof way of checking if the palette
		 *	present in the resource file is a valid one. The criterias I've
		 *	implemented here is;
		 *	1. The resource must have 256 color cicons - else the palette is
		 *	   not considered at all.
		 *	2. The first 16 palette entries are checked and only valid if
		 *	   there are 16 unique pen indexes.
		 *	3. If check 2 passes, check if there are valid RGB data in the
		 *	   remaining palette entries. This simply means the check fails
		 *	   if there is only 0 for r, g and b levels .. not a very reliable
		 *	   test.
		 */
		if (maxplanes >= 8 && *earray && *earray != -1L)
		{
			short work_in[15] = { 1,1,1,1,1, 1,1,1,1,1, 2,0,0,0,0 };
			short work_out[58];
			struct xa_rsc_rgb *p;
			int i;
			unsigned short mask;
			bool pal = false;

			vdih = C.P_handle;
			v_opnvwk(work_in, &vdih, work_out);

			DIAG((D_rsrc, client, "Color palette present"));
			(long)p = (long)(*earray + (long)base);

			/* Ozk:	First we check if the first 16 pens have valid data
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
				if ((p[i].red | p[i].green | p[i].blue))
				{
					pal = true;
					break;
				}
			}
			
			if (pal && mask == 0xffff)
			{
				DIAG((D_rsrc, client, "%s got palette", fname ? fname : "noname"));
				(long)rscs->palette = (long)(*earray + (long)base);
								
				fix_rsc_palette((struct xa_rsc_rgb *)rscs->palette);
				if (set_pal && cfg.set_rscpalette)
				{
					set_syspalette(client->vdi_settings->handle, rscs->palette);
					get_syspalette(client->vdi_settings->handle, screen.palette);
				}
			}
		}
#if 0
		/* ozk: Enable this when (if) more extensions are added ... */
		if (*earray)
			earray++;
#endif
	}
	fix_objects(client, rscs, cibh, vdih, base, (OBJECT *)(base + hdr->rsh_object), hdr->rsh_nobs);

	/*
	 * Close the virtual workstation that handled the RSC
	 * color palette (if present).
	 */
	if (vdih != client->vdi_settings->handle)
		v_clsvwk(vdih);

	fix_trees(client, base, (OBJECT **)(unsigned long)(base + hdr->rsh_trindex), hdr->rsh_ntree, designWidth, designHeight);
	
	return base;
}

static void
Rsrc_setglobal(RSHDR *h, struct aes_global *gl)
{
	if (gl)
	{
		OBJECT **o;
		(unsigned long)o = (unsigned long)h + h->rsh_trindex;

		/* Fill in the application's global array with a pointer to the resource */
		gl->ptree = o;
		gl->rshdr = h;
		gl->lmem = h->rsh_rssize;

		DIAGS(("Resources %ld(%lx) in global[5,6]", o, o));
		DIAGS(("      and %ld(%lx) in global[7,8]", h, h));
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
	struct xa_rscs *cur;
	RSHDR *rsc = NULL;

	if (pb && pb->global)
		rsc = ((struct aes_global *)pb->global)->rshdr;

	cur = client->resources;
	
	DIAG((D_rsrc,client,"FreeResources: %lx for %d, ct=%d, pb->global->rsc=%lx",
		cur, client->p->pid, client->rsct, rsc));

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
					(unsigned long)trees = (unsigned long)(base + hdr->rsh_trindex);
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
						(unsigned long)o = nxt_hdr ? (unsigned long)nxt_hdr + nxt_hdr->rsh_trindex : 0L;
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
					if (client == C.Aes)
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

/*
 * Find the tree with a given index
 * fixing up the pointer array is now done in Loadresources, to make it usable via global[5]
 */
OBJECT *
ResourceTree(RSHDR *hdr, long num)
{
	OBJECT **index;

	if (num_nok(ntree))
		return NULL;

	start(trindex);
	return index[num];
}

/* Find the object with a given index  */
static OBJECT *
ResourceObject(RSHDR *hdr, int num)
{
	OBJECT *index;

	if (num_nok(nobs))
		return NULL;

	start(object);
	return index + num;
}

/* Find the tedinfo with a given index */
static TEDINFO *
ResourceTedinfo(RSHDR *hdr, int num)
{
	TEDINFO *index;

	if (num_nok(nted))
		return NULL;

	start(tedinfo);
	return index + num;
}

/* Find the iconblk with a given index */
static ICONBLK *
ResourceIconblk(RSHDR *hdr, int num)
{
	ICONBLK *index;

	if (num_nok(nib))
		return NULL;

	start(iconblk);
	return index + num;
}

/* Find the bitblk with a given index */
static BITBLK *
ResourceBitblk(RSHDR *hdr, int num)
{
	BITBLK *index;

	if (num_nok(nbb))
		return NULL;

	start(bitblk);
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

	start(frstr);

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

	start(frimg);
	return index[num];
}

/* HR: At last the following 2 make sense */

/* Find the ref to the free strings array */
static char **
ResourceFrstr(RSHDR *hdr, int num)
{
	char **index;

	start(frstr);

//	DIAG((D_s, NULL, "Gaddr 15 %lx '%s'", index, *index));
	return index + num;
}

/* Find the ref to free images array */
static void **
ResourceFrimg(RSHDR *hdr, int num)
{
	void **index;

	start(frimg);

//	DIAG((D_s, NULL, "Gaddr 16 %lx", index));
	return index + num;
}

unsigned long
XA_rsrc_load(enum locks lock, struct xa_client *client, AESPB *pb)
{
	char *path;

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
	    7/9/200   done.  As well as the memory allocated for colour icon data.
*/
		path = shell_find(lock, client, (char*)pb->addrin[0]);
		if (path)
		{
			RSHDR *rsc;
			DIAG((D_rsrc, client, "rsrc_load('%s')", path));

			rsc = LoadResources(client, path, NULL, DU_RSX_CONV, DU_RSY_CONV, false);
			if (rsc)
			{
				OBJECT **o;
				(unsigned long)o = (unsigned long)rsc + rsc->rsh_trindex;
				client->rsrc = rsc;
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

				pb->intout[0] = 1;
				return XAC_DONE;
			}
		}
	}
	else
	{
		/* inform user what's going on */
		ALERT(("XaAES: rsrc_load: %s, client with no globl_ptr, killing it", client->name));
		//exit_client(lock, client, -1, true, true);
		raise(SIGKILL);
		yield();
		return 0;
	}


	DIAGS(("ERROR: rsrc_load '%s' failed", pb->addrin[0] ? (char*)pb->addrin[0] : "~~"));

	pb->intout[0] = 0;
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
			DIAG((D_s,client,"  from pb->global --> %ld",*addr));		
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
/*!*/		case R_STRING:
			*addr = ResourceString(client->rsrc, index);
			break;
/*!*/		case R_IMAGEDATA:
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
/*!*/		case R_FRSTR:
			*addr = ResourceFrstr(client->rsrc, index);
			break;
/*!*/		case R_FRIMG:
			*addr = ResourceFrimg(client->rsrc, index);
			break;
		}
		DIAG((D_s,client,"  --> %lx",*addr));		
		pb->intout[0] = 1;
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
		obfix(ob, item);
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
		ALERT(("XaAES: rsrc_rcfix: %s, client with no globl_ptr, killing it", client->name));
		//exit_client(lock, client, -1, true, true); //get_curproc());
		raise(SIGKILL);
		yield();
		return 0;
	}

	pb->intout[0] = 0;
	return XAC_DONE;
}
