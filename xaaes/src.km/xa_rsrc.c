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

#include "objects.h"
#include "trnfm.h"
#include "xa_rsrc.h"
#include "xa_shel.h"

#include "mint/fcntl.h"


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
static short *
transform_icon_bitmap(struct xa_client *client, CICONBLK *icon, short *map, long len, int planes, short vdih)
{
	MFDB src, dst;
	short *new_data = map;
	short *tmp = NULL;
	long icon_len = len * planes;
	long new_len = len * screen.planes;

	DIAG((D_s, client, "icon_len %ld, new_len %ld", icon_len, new_len));

	if (planes < screen.planes)
	{
		DIAG((D_x, client, "alloc of %ld bytes", new_len));

		if (client == C.Aes)
			new_data = kmalloc(new_len);
		else
			new_data = umalloc(new_len);

		if (!new_data)
			return map;

		memcpy(new_data, map, icon_len);
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

	DIAG((D_x, client, "alloc of %ld bytes", new_len));
	tmp = kmalloc(new_len);
	if (tmp)
	{
		memcpy(tmp, new_data, new_len);
		src.fd_addr = tmp;
		transform_gem_bitmap_data(vdih, src, dst, planes, screen.planes);
		kfree(tmp);
	}

	return new_data;
}

/*
 * FixColourIconData: Convert a colour icon from device independent to device specific
 */
static void
FixColourIconData(struct xa_client *client, CICONBLK *icon, long base, short vdih)
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
			c->col_data = transform_icon_bitmap(client, icon, c->col_data, len, c->num_planes, vdih);
		if (c->sel_data)
			c->sel_data = transform_icon_bitmap(client, icon, c->sel_data, len, c->num_planes, vdih);

		/* set the new data plane count */
		c->num_planes = screen.planes;
	}
	else
	{
		DIAG((D_rsrc,client,"[1]No matching icon"));
	}
}

static void
list_resource(struct xa_client *client, void *resource)
{
	struct xa_rscs *new;

	DIAG((D_x, client, "XA_alloc 2 %ld", sizeof(*new)));

	if (client == C.Aes)
		new = kmalloc(sizeof(*new));
	else
		new = umalloc(sizeof(*new));

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
		new->id = 2;
		new->handle = client->rsct;
		new->rsc = resource;
		new->globl = client->globl_ptr;
	}
}

/*
 * LoadResources: Load a GEM resource file
 * fname = name of file to load
 * Return = base pointer of resources or NULL on failure
 */
void *
LoadResources(struct xa_client *client, char *fname, RSHDR *rshdr, short designWidth, short designHeight)
{
#define resWidth (screen.c_max_w)
#define resHeight (screen.c_max_h)

	RSHDR hdr;
	CICONBLK **cibh = NULL;
	OBJECT *obj, **trees;
	unsigned long osize = 0, size = 0;
	char *base = NULL;
	int i, j, type, numCibs = 0;
	short vdih = C.vh;

	IFDIAG(OBJECT *tree;)

	if (fname)
	{
		struct file *f;
		long err;

		DIAG((D_rsrc, client, "LoadResources(%s)", fname));

		f = kernel_open(fname, O_RDONLY, &err);
		if (!f)
		{
			DIAG((D_rsrc, client, "LoadResources(): file not found"));
			return NULL;
		}

		kernel_read(f, &hdr, sizeof(hdr));
		size = (unsigned long) hdr.rsh_rssize;
		osize = (size + 1UL) & 0xfffffffeUL;

		/* Extended format, get new 32-bit length */
		if (hdr.rsh_vrsn & 4)
		{
			kernel_lseek(f, osize, 0);
			kernel_read(f, &size, 4);
			DIAG((D_rsrc, client, "extended format osize=%ld: read extended size=%ld", osize, size));
		}

		kernel_lseek(f, 0, 0);

		DIAG((D_x, client, "XA_alloc 1 %ld", size));

		if (client == C.Aes)
			base = kmalloc(size);
		else
			base = umalloc(size);

		if (!base)
		{
			DIAG((D_rsrc, client, "Can't allocate space for resource file"));
			kernel_close(f);
			return NULL;
		}

		/* Reread everything */
		kernel_read(f, base, size);
		kernel_close(f);
	}
	else
	{
		DIAG((D_rsrc, client, "LoadResources %ld(%lx)", rshdr, rshdr));
		if (rshdr)
		{
			hdr = *rshdr;
			(RSHDR *)base = rshdr;
			size = (unsigned long)hdr.rsh_rssize;
			osize = (size + 1UL) & 0xfffffffeUL;
		}
	}

	client->rsct++;

	/* Put the resource in a list. */
	list_resource(client, base);

	/* fixup all free string pointers */
	{
		char **fs = (char **)(base + hdr.rsh_frstr);
		for (i = 0; i < hdr.rsh_nstring; i++, fs++)
		{
//			IFDIAG(char *d = *fs;)
			*fs += (long)base;
//			DIAG((D_s,client,"fs[%d]>%ld='%s'",i,d,*fs));
		}
		
		DIAG((D_rsrc, client, "fixed up %d free_string pointers", hdr.rsh_nstring));
	}

	/* fixup all free image pointers */
	{
		char **fs = (char **)(base + hdr.rsh_frimg);
		for (i = 0; i < hdr.rsh_nimages; i++, fs++)
		{
//			IFDIAG(char *d = *fs;)
			*fs += (long)base;
//			DIAG((D_s, client, "imgs[%d]>%ld=%lx",i, d, *fs));
		}
		
		DIAG((D_rsrc,client,"fixed up %d free_image pointers", hdr.rsh_nimages));
	}

	/* fixup all tedinfo field pointers */
	{
		TEDINFO *ti = (TEDINFO *)(base + hdr.rsh_tedinfo);
		for (i = 0; i < hdr.rsh_nted; i++, ti++)
		{
			(long)ti->te_ptext  += (long)base;
			(long)ti->te_ptmplt += (long)base;
			(long)ti->te_pvalid += (long)base;
		}
	
		DIAG((D_rsrc, client, "fixed up %d tedinfo's", hdr.rsh_nted));
	}

	/* fixup all iconblk field pointers */
	{
		ICONBLK *ib = (ICONBLK *)(base + hdr.rsh_iconblk);
		for (i = 0; i < hdr.rsh_nib; i++, ib++)
		{
			(long)ib->ib_pmask += (long)base;
			(long)ib->ib_pdata += (long)base;
			(long)ib->ib_ptext += (long)base;
		}
		
		DIAG((D_rsrc, client, "fixed up %d iconblk's", hdr.rsh_nib));
	}

	/* fixup all bitblk data pointers */
	{
		BITBLK *bb = (BITBLK *)(base + hdr.rsh_bitblk);
		for (i = 0; i < hdr.rsh_nbb; i++, bb++)
		{
			(long)bb->bi_pdata += (long)base;
		}

		DIAG((D_rsrc, client, "fixed up %d bitblk's", hdr.rsh_nbb));
	}

	if (hdr.rsh_vrsn & 4)
	{
		/* It's an enhanced RSC file */

		unsigned long *earray, *addr;

		DIAG((D_rsrc, client, "Enhanced resource file"));

		/* this the row of pointers to extensions */
		/* terminated by a 0L */
		earray = (unsigned long *)(osize + (long)base);
		earray++;						/* ozk: skip filesize */
		cibh = (CICONBLK **)(*earray + (long)base);
		if (*earray && *earray != -1L) /* Get colour icons */
		{
			CICONBLK *cib, **cp = cibh;

			while (*cp++ != (CICONBLK *) -1L)
				numCibs++;

			cib = (CICONBLK *)cp;

			/* Fix up all the CICONBLK's */
			for (i = 0; i < numCibs; i++)
			{
				CICON *cicn;
				ICONBLK *ib = &cib->monoblk;
				short *pdata, numRez;
				long isize;

				cibh[i] = cib;
				isize = calc_back((RECT*)&ib->ib_xicon, 1);
				addr = (unsigned long*)((long)cib + sizeof(ICONBLK));
				numRez = addr[0];
				pdata = (short *)&addr[1];
				/* mono data & mask */
				ib->ib_pdata = pdata;
				(long)pdata += isize;
				ib->ib_pmask = pdata;
				(long)pdata += isize;
				/* HR: the texts are placed the same way as for all other objects
				 * when longer than 12 bytes.
				 */
				if (ib->ib_ptext)
				{
					short l = ib->ib_wtext/6;
					/* fix some resources */
					ib->ib_ptext += (long)base;
					if (strlen(ib->ib_ptext) > l)
						*(ib->ib_ptext + l) = 0;
				}
				else
					/* The following word is no of planes which cannot
					 * be larger than 32, so there is 1 zero byte there */
					ib->ib_ptext = (char *)pdata; 

				/* (unused) place for name */
				(long)pdata += 12;

				cicn = (CICON *)pdata;
				/* There can be color icons with NO color icons,
				 * only the mono icon block. */
				cib->mainlist = NULL;
				/* Get CICON's at different rez's */
				for (j = 0; j < numRez; j++)
				{
					int planes = cicn->num_planes;
					long psize = isize * planes;
					(long)pdata += sizeof(CICON);
					cicn->col_data = pdata;
					(long)pdata += psize;
					cicn->col_mask = pdata;
					(long)pdata +=  isize;
					if (cicn->sel_data)
					{
						/* It's got a selected form */
						cicn->sel_data = pdata;
						(long)pdata += psize;
						cicn->sel_mask = pdata;
						(long)pdata +=  isize;
					}
					else
						/* No selected version */
						cicn->sel_data = cicn->sel_mask = NULL;

					if (cib->mainlist == NULL)
						cib->mainlist = cicn;

					if ((long)cicn->next_res == 1)
						cicn->next_res = (CICON *)pdata;
					else
						cicn->next_res = NULL;

					cicn = (CICON *)pdata;
				}
				cib = (CICONBLK *)cicn;
			}
			DIAG((D_rsrc, client, "fixed up %d color icons", numCibs));
		}

		if (*earray)
			earray++;

		if (*earray && *earray != -1L)
		{
			short *rsrc_colour_lut = (short *)(*earray + (long)base);

			int rc, col;
			short work_in[15] = { 1,1,1,1,1, 1,1,1,1,1, 2,0,0,0,0 };
			short work_out[58];
			vdih = C.P_handle;
			v_opnvwk(work_in, &vdih, work_out);
			DIAG((D_rsrc, client, "Color palette present"));

			/*
			* ozk:	under NO goddamned circumstances do we EVER touch the first 16 VDI pens!!!!
			*	These 16 pens are global, and should be setup upon boot, not for every
			*	resource file loaded!
			*/
#if 0
			/* set the palette up to the vdi workstation */
			for (rc = 0;rc < 16; rc++)
			{
				static short tos_colours[] = { 0, 255, 1, 2, 4, 6, 3, 5, 7, 8, 9, 10, 12, 14, 11, 13};
				vs_color(vdih, rc, &rsrc_colour_lut[tos_colours[rc]*4]);
			}
#endif
			/*
			 * ozk:	And here we make sure that the resource file actually contains a valid palette.
			 *	After some research I found out that although there is a palette, it is by no
			 *	means certain it is filled with valid RGB data. Most of the time, only the first
			 *	16 colors are set. So we just check if the 16 - 255 colours conatin anything.
			*/
			col = 0;
			for (rc = 16; rc < 256; rc++)
			{
				if (rsrc_colour_lut[rc*4] | rsrc_colour_lut[(rc*4)+1] | rsrc_colour_lut[(rc*4)+2])
					col = 1; break;
			}
			/* 1:1 mapping for other colors */
			if (col)
			{
				for (rc = 16;rc < 255; rc++)
					vs_color(vdih, rc, &rsrc_colour_lut[rc*4]);
				/* the last one mapped to 15 seee tos_colours */
				vs_color(vdih, 255, &rsrc_colour_lut[15*4]);
			}
		}
#if 0
		/* ozk: Enable this when (if) more extensions are added ... */
		if (*earray)
			earray++;
#endif
	}

	/* As you see, the objects are NOT in a pointer array!!! */
	obj = (OBJECT *)(base + hdr.rsh_object);

	IFDIAG (tree = obj;)

	/* fixup all objects' ob_spec pointers */
	for (i = 0; i < hdr.rsh_nobs; i++, obj++)
	{
		type = obj->ob_type & 255;

#if 0
		DIAG((D_s, client, "obj[%d]>%ld=%lx, %s;\t%d,%d,%d",
			i,
			(long)obj-(long)base,
			obj,
			object_type(tree,i),
			obj->ob_next,obj->ob_head,obj->ob_tail));
#endif

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
		case G_ICON:
			obj->ob_spec.free_string += (long)base;
			break;
		case G_CICON:
			FixColourIconData(client, cibh[obj->ob_spec.index], (long)base, vdih);
			obj->ob_spec.ciconblk = cibh[obj->ob_spec.index];
			break;
		case G_PROGDEF:
			obj->ob_spec.userblk = NULL;
			break;
		case G_IBOX:
		case G_BOX:
		case G_BOXCHAR:
			break;
		default:
			DIAG((D_rsrc, client, "Unknown object type %d", type));
			break;
		}
	}

	/*
	 * Close the virtual workstation that handled the RSC
	 * color palette (if present).
	 */
	if (vdih != C.vh)
		v_clsvwk(vdih);

	DIAG((D_rsrc, client, "fixed up %d objects ob_spec", hdr.rsh_nobs));

	/* HR!!!!! the pointer array wasnt fixed up!!!!! which made it unusable via global[5] */

	(unsigned long)trees = (unsigned long)(base + hdr.rsh_trindex);
	for (i = 0; i < hdr.rsh_ntree; i++)
	{
		(unsigned long)trees[i] += (unsigned long)base;
		DIAG((D_s,client,"tree[%d]>%ld = %lx",i,(long)trees[i]-(long)base,trees[i]));

		obj = trees[i];
		if ((obj[3].ob_type & 255) != G_TITLE)
		{
			/* Not a menu tree */
			do {
				/* Fix all object coordinates */
				obj->ob_x = (((obj->ob_x & 255) * designWidth + (obj->ob_x >> 8)) * resWidth) / designWidth;
				obj->ob_y = (((obj->ob_y & 255) * designHeight + (obj->ob_y >> 8)) * resHeight) / designHeight;
				obj->ob_width = (((obj->ob_width & 255) * designWidth + (obj->ob_width >> 8)) * resWidth) / designWidth;
				obj->ob_height = (((obj->ob_height & 255) * designHeight + (obj->ob_height >> 8)) * resHeight) / designHeight;
			}
			while (!(obj++->ob_flags & OF_LASTOB));
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
	
	return (void *)base;
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
		DIAGS(("Resources %ld(%lx) in global[5,6]", o, o));
		DIAGS(("      and %ld(%lx) in global[7,8]", h, h));
	}
}

/*
 * FreeResources: Dispose of a set of loaded resources
 */
void
FreeResources(struct xa_client *client, AESPB *pb)
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
			bool have = rsc && (rsc == cur->rsc);
			struct xa_rscs *nx = cur->next;

			DIAG((D_rsrc, client, "Free: test cur %lx", (long)cur));
			DIAG((D_rsrc, client, "Free: test cur handle %d", cur->handle));

			if (have || (!rsc && cur->handle == client->rsct))
			{
				/* free the entry for the freed rsc. */
				RSHDR *hdr = cur->rsc;
				char *base = cur->rsc;
				OBJECT **trees, *obj;
				int i;

				/* Free the memory allocated for scroll list objects. */
				(unsigned long)trees = (unsigned long)(base + hdr->rsh_trindex);
				for (i = 0; i < hdr->rsh_ntree; i++)
				{
					int f = 0;
					obj = trees[i];
					do {
						if ((obj[f].ob_type & 255) == G_SLIST)
						{
							if (client == C.Aes)
								kfree((SCROLL_INFO*)obj[f].ob_spec.index);
							else
								ufree((SCROLL_INFO*)obj[f].ob_spec.index);
						}
					}
					while (!(obj[f++].ob_flags & OF_LASTOB));
				}

				/* unhook the entry from the chain */
				if (cur->prior)
					cur->prior->next = cur->next;
				if (cur->next)
					cur->next->prior = cur->prior;
				if (!cur->prior)
					client->resources = cur->next;

				if (client == C.Aes)
				{
					DIAG((D_rsrc, client, "Free: cur %lx for AESSYS", cur));
					kfree(cur);
				}
				else
				{
					DIAG((D_rsrc, client, "Free: cur %lx", cur));
					ufree(cur);
				}
			}
			else if (cur->handle == client->rsct - 1)
			{
				DIAG((D_rsrc,client,"Free: Rsrc_setglobal %lx", cur->rsc));

				/* Yeah, there is a resource left! */
				client->rsrc = cur->rsc;
				Rsrc_setglobal(cur->rsc, client->globl_ptr);
			}

			if (have)
				break;

			cur = nx;
		}

		client->rsct--;
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
ResourceTree(RSHDR *hdr, int num)
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
	
		rsc = LoadResources(client, path, NULL, DU_RSX_CONV, DU_RSY_CONV);
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
			Rsrc_setglobal(rsc, client->globl_ptr);

			DIAG((D_rsrc,client,"pb %lx, gl %lx, gl->rsc %lx, gl->ptree %lx",
				pb, pb->global, ((struct aes_global *)pb->global)->rshdr, ((struct aes_global *)pb->global)->ptree));

			if (pb->global)
				/* Fill it out in the global of the rsrc_load. */
				Rsrc_setglobal(rsc, (struct aes_global *)pb->global);

			pb->intout[0] = 1;
			return XAC_DONE;
		}
	}

	DIAGS(("ERROR: rsrc_load '%s' failed", pb->addrin[0] ? (char*)pb->addrin[0] : "~~"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_rsrc_free(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(0,1,0)

	if (client->rsrc)
	{
		FreeResources(client, pb);
		pb->intout[0] = 1;
	}
	else
		pb->intout[0] = 0;

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

//	DIAG((D_rsrc, client, "rsrc_obfix for %s: tree %lx + %d",
//		c_owner(client), ob, item));

	if (ob)
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
	CONTROL(0,1,1)
	
	DIAG((D_rsrc, client, "rsrc_rcfix for %s on %ld(%lx)",
		c_owner(client), pb->addrin[0], pb->addrin[0]));

	client->rsrc = LoadResources(client, NULL, (RSHDR*)pb->addrin[0], DU_RSX_CONV, DU_RSY_CONV);
	if (client->rsrc)
	{
#if GENERATE_DIAGS
		if (client->globl_ptr != (struct aes_global *)pb->global)
		{
			DIAGS(("WARNING: rsrc_rcfix global %ld(%lx) is different from appl_init's global %ld(%lx)",
				pb->global, pb->global, client->globl_ptr, client->globl_ptr));
		}
#endif
		Rsrc_setglobal(client->rsrc, client->globl_ptr);

		pb->intout[0] = 1;
		return XAC_DONE;
	}

	pb->intout[0] = 0;
	return XAC_DONE;
}
