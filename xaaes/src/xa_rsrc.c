/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
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

#include <mint/osbind.h>
#include <fcntl.h>

#include "xa_types.h"
#include "xa_global.h"

#include "objects.h"
#include "trnfm.h"
#include "xalloc.h"
#include "xa_rsrc.h"
#include "xa_shel.h"


/*
 * RESOURCE FILE HANDLER
 * 
 * Simulate the standard GEM AES resource access functions.
 * 	
 * I've added these to Steve's routines to act as a bit of an interface
 * to his routines. Each application has its own rsrc as part
 * of the XA_CLIENT structure. As Steve's resource loader is happy to
 * allow multiple resource files, I should add some extra calls to support it.
 */


/*
 * Fixup OBJECT coordinates; the lower byte contains a (character-based)
 * coordinate, the upper byte an additional (pixel) offset.
 */

static inline short
fixup(short val, short scale)
{
	return (val & 0xff)*scale + ((val >> 8) & 0xff);
}

void
obfix(OBJECT *tree, int object)
{
	OBJECT *o = tree + object;

	o->r.x = fixup(o->r.x, screen.c_max_w) ;
	o->r.y = fixup(o->r.y, screen.c_max_h) ;
	/*
	 * Special case handling: any OBJECT 80 characters wide is supposed
	 * to be a menu bar, which always covers the entire screen width...
	 */
	o->r.w = (o->r.w == 80) ? screen.r.w
				: fixup(o->r.w, screen.c_max_w) ;
	o->r.h = fixup(o->r.h, screen.c_max_h) ;
}

/*
 * Code in this module is based on the resource loader from
 * Steve Sowerby's AGiLE library. Thanks to Steve for allowing
 * me to use his code.
 */

static short resWidth, resHeight;

/* HR 021101: new function to make the code orthogonal.
*/
static short *
transform_icon_bitmap(XA_CLIENT *client, CICONBLK *icon, short *map, long len, int planes)
{
	MFDB src, dst;
	short *new_data = map;
	short *tmp = NULL;
	long icon_len = len * planes;
	long new_len = len * screen.planes;

	DIAG((D_s, client, "icon_len %ld, new_len %ld\n", icon_len, new_len));

	if (planes < screen.planes)
	{
		DIAG((D_x, client, "XA_calloc 3 %ld\n", new_len));

		new_data = XA_calloc(&client->base, 1, new_len, 3, client->rsct);
		if (new_data == NULL)
			return map;

		memcpy(new_data, map, icon_len);
	}

	src.fd_w = icon->monoblk.ic.w;			/* Transform MFDB's */
	src.fd_h = icon->monoblk.ic.h;
	src.fd_wdwidth = (src.fd_w + 15) / 16;		/* round up */
	src.fd_stand = 1;
	src.fd_r1 = src.fd_r2 = src.fd_r3 = 0;
	src.fd_nplanes = screen.planes;	
	dst = src;

	dst.fd_addr = new_data;
	dst.fd_stand = 0;

	DIAG((D_x, client, "XA_calloc %d*%ld\n", 1, new_len));
	tmp = XA_calloc(NULL, 1, new_len, 0, 0);
	if (tmp)
	{
		memcpy(tmp, new_data, new_len);
		src.fd_addr = tmp;
		transform_gem_bitmap_data(src, dst, planes, screen.planes);
		/* vr_trnfm( C.vh, &src, &dst ); */
		free(tmp);
	}

	return new_data;
}

/*
 * FixColourIconData : Convert a colour icon from device independent to device specific
 */
static void
FixColourIconData(XA_CLIENT *client, CICONBLK *icon, long base)
{
	CICON *c, *best_cicon = NULL;
	long len = calc_back(&icon->monoblk.ic, 1);

	DIAG((D_s, client, "color icon: '%s' %d*%d %ld tx.w=%d\n",
		icon->monoblk.ib_ptext,
		icon->monoblk.ic.w, icon->monoblk.ic.h, len, icon->monoblk.tx.w));

	/* HR 021101: Use the same mechanism from d_g_cicon() for reducing the
	 *            number of transformations done.
	 */
	c = icon->mainlist;
	while(c)
	{
		/* DIAG((D_rsrc,client,"[1]probe cicon planes %d\n", c->num_planes)); */
	
		if (    c->num_planes <= screen.planes
		    && (!best_cicon || (best_cicon && c->num_planes > best_cicon->num_planes)))
		{
			best_cicon = c;
		}
		c = c->next_res;	
	}

	if (best_cicon)
	{
		/* DIAG((D_rsrc,client,"[1]best_cicon planes: %d\n", best_cicon->num_planes)); */

		c = best_cicon;
		if (c->col_data)
			c->col_data = transform_icon_bitmap(client, icon, c->col_data, len, c->num_planes);
		if (c->sel_data)
			c->sel_data = transform_icon_bitmap(client, icon, c->sel_data, len, c->num_planes);
		/* set the new data plane count */
		c->num_planes = screen.planes;
	}
	else
	{
		/* DIAG((D_rsrc,client,"[1]No matching icon\n")); */
	}
}

static void
list_resource(XA_CLIENT *client, void *resource)
{
	XA_RSCS *new;

	DIAG((D_x, client, "XA_alloc 2 %ld\n", sizeof(XA_RSCS)));

	new = XA_alloc(&client->base, sizeof(XA_RSCS), 2, 0);
	if (new)
	{
		DIAG((D_rsrc, client, "list_resource %ld(%lx) for %s rsc:%ld(%lx)\n",
			new, new, c_owner(client), resource, resource));

		if (client->resources)
			client->resources->prior = new;

		new->next = client->resources;
		client->resources = new;			
		new->prior = NULL;
		new->id = 2;
		new->handle = client->rsct;
		new->rsc = resource;
		new->globl = client->globl_ptr;
	}
}

/*
 * LoadResources : Load a GEM resource file
 * fname = name of file to load
 * Return = base pointer of resources or NULL on failure
 */
void *
LoadResources(XA_CLIENT *client, char *fname, RSHDR *rshdr, short designWidth, short designHeight)
{
	long handle;
	RSHDR hdr;
	OBJECT *obj, **trees;

	IFDIAG (OBJECT *tree;)

	CICONBLK **cibh;
	unsigned long osize, size;
	char *base;
	int i, j, type, numCibs = 0;

	resWidth = screen.c_max_w;
	resHeight = screen.c_max_h;

	if (fname)
	{
		DIAG((D_rsrc, client, "LoadResources(%s)\n", fname));
		
		handle = Fopen(fname, O_RDONLY);
		if (handle < 0L)
		{
			DIAG((D_rsrc, client, "LoadResources(): file not found\n"));
			return NULL;
		}

		Fread(handle, sizeof(RSHDR), &hdr);
		size = (unsigned long)hdr.rsh_rssize;
		osize = (size + 1UL) & 0xfffffffeUL;
		if (hdr.rsh_vrsn & 4)	/* Extended format, get new 32-bit length */
		{
			Fseek(osize,handle, 0);
			Fread(handle, 4L, &size);
			DIAG((D_rsrc, client, "extended format osize=%ld: read extended size=%ld\n", osize, size));
		}
	
		Fseek(0L, handle, 0);
	
		DIAG((D_x, client, "XA_alloc 1 %ld\n", size));
		base = XA_alloc(&client->base, size, 1, client->rsct + 1); /* and put in a list. */
		if (!base)
		{
			DIAG((D_rsrc, client, "Can't allocate space for resource file\n"));
			Fclose(handle);
			return NULL;
		}

		/* Reread everything */
		Fread(handle, size, base);
		Fclose(handle);
	}
	else
	{
		DIAG((D_rsrc, client, "LoadResources %ld(%lx)\n", rshdr, rshdr));
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

	{	/* fixup all free string pointers */
		char **fs = (char **)(base + hdr.rsh_frstr);
		for (i = 0; i < hdr.rsh_nstring; i++, fs++)
		{
			IFDIAG (char *d = *fs;)
			*fs += (long)base;
			DIAG((D_s,client,"fs[%d]>%ld='%s'\n",i,d,*fs));
		}
		
		DIAG((D_rsrc, client, "fixed up %d free_string pointers\n",hdr.rsh_nstring));
	}
	{	/* HR: fixup all free image pointers */
		char **fs = (char **)(base + hdr.rsh_frimg);
		for (i = 0; i < hdr.rsh_nimages; i++, fs++)
		{
			IFDIAG (char *d = *fs;)
			*fs += (long)base;
			DIAG((D_s, client, "imgs[%d]>%ld=%lx\n",i,d,*fs));
		}
		
		DIAG((D_rsrc,client,"fixed up %d free_image pointers\n",hdr.rsh_nimages));
	}
	{	/* fixup all tedinfo field pointers */
		TEDINFO *ti = (TEDINFO *)(base + hdr.rsh_tedinfo);
		for (i = 0; i < hdr.rsh_nted; i++, ti++)
		{
			(long)ti->te_ptext  += (long)base;
			(long)ti->te_ptmplt += (long)base;
			(long)ti->te_pvalid += (long)base;
		}
	
		DIAG((D_rsrc, client, "fixed up %d tedinfo's\n",hdr.rsh_nted));
	}
	{	/* fixup all iconblk field pointers */
		ICONBLK *ib = (ICONBLK *)(base + hdr.rsh_iconblk);
		for (i = 0; i < hdr.rsh_nib; i++, ib++)
		{
			(long)ib->ib_pmask += (long)base;
			(long)ib->ib_pdata += (long)base;
			(long)ib->ib_ptext += (long)base;
		}
		
		DIAG((D_rsrc, client, "fixed up %d iconblk's\n", hdr.rsh_nib));
	}
	{	/* fixup all bitblk data pointers */
		BITBLK *bb = (BITBLK *)(base + hdr.rsh_bitblk);
		for (i = 0; i < hdr.rsh_nbb; i++, bb++)
		{
			(long)bb->bi_pdata += (long)base;
		}

		DIAG((D_rsrc, client, "fixed up %d bitblk's\n", hdr.rsh_nbb));
	}

	if (hdr.rsh_vrsn & 4)
	{
		/* It's an enhanced RSC file */

		unsigned long *earray, *addr;

		DIAG((D_rsrc, client, "Enhanced resource file\n"));

		/* this the row of pointers to extensions */
		/* terminated by a 0L */
		earray = (unsigned long *)(osize + (long)base);
		cibh = (CICONBLK **)(earray[1] + (long)base);
		if ((long)cibh > 0L)	/* Get colour icons */
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

				cibh[i] = cib;
				size = calc_back(&ib->ic, 1);
				addr = (unsigned long*)((long)cib + sizeof(ICONBLK));
				numRez = addr[0];
				pdata = (short *)&addr[1];
				/* mono data & mask */
				ib->ib_pdata = pdata;
				(long)pdata += size;
				ib->ib_pmask = pdata;
				(long)pdata += size;
				/* HR: the texts are placed the same way as for all other objects
				 * when longer than 12 bytes.
				 */
				if (ib->ib_ptext)
				{
					short l = ib->tx.w/6;
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
					long psize = size * planes;
					(long)pdata += sizeof(CICON);
					cicn->col_data = pdata;
					(long)pdata += psize;
					cicn->col_mask = pdata;
					(long)pdata +=  size;
					if (cicn->sel_data)
					{
						/* It's got a selected form */
						cicn->sel_data = pdata;
						(long)pdata += psize;
						cicn->sel_mask = pdata;
						(long)pdata +=  size;
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
			DIAG((D_rsrc, client, "fixed up %d color icons\n", numCibs));
		}
	}

	/* As you see, the objects are NOT in a pointer array!!! */
	obj = (OBJECT *)(base + hdr.rsh_object);

	IFDIAG (tree = obj;)

	/* fixup all objects' ob_spec pointers */
	for (i = 0; i < hdr.rsh_nobs; i++, obj++)
	{
		type = obj->ob_type & 255;

		DIAG((D_s, client, "obj[%d]>%ld=%lx, %s;\t%d,%d,%d\n",
			i,
			(long)obj-(long)base,
			obj,
			object_type(tree,i),
			obj->ob_next,obj->ob_head,obj->ob_tail));

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
			obj->ob_spec.string += (long)base;
			break;
		case G_CICON:
			FixColourIconData(client, cibh[obj->ob_spec.lspec], (long)base);
			obj->ob_spec.ciconblk = cibh[obj->ob_spec.lspec];
			break;
		case G_PROGDEF:
			obj->ob_spec.appblk = NULL;
			break;
		case G_IBOX:
		case G_BOX:
		case G_BOXCHAR:
			break;
		default:
			DIAG((D_rsrc, client, "Unknown object type %d\n", type));
			break;
		}
	}

	DIAG((D_rsrc, client, "fixed up %d objects ob_spec\n", hdr.rsh_nobs));

	/* HR!!!!! the pointer array wasnt fixed up!!!!! which made it unusable via global[5] */

	(unsigned long)trees = (unsigned long)(base + hdr.rsh_trindex);
	for (i = 0; i < hdr.rsh_ntree; i++)
	{
		(unsigned long)trees[i] += (unsigned long)base;
		DIAG((D_s,client,"tree[%d]>%ld = %lx\n",i,(long)trees[i]-(long)base,trees[i]));

		obj = trees[i];
		if ((obj[3].ob_type & 255) != G_TITLE)
		{
			/* Not a menu tree */
			do {
				/* Fix all object coordinates */
				obj->r.x = (((obj->r.x & 255) * designWidth + (obj->r.x >> 8)) * resWidth) / designWidth;
				obj->r.y = (((obj->r.y & 255) * designHeight + (obj->r.y >> 8)) * resHeight) / designHeight;
				obj->r.w = (((obj->r.w & 255) * designWidth + (obj->r.w >> 8)) * resWidth) / designWidth;
				obj->r.h = (((obj->r.h & 255) * designHeight + (obj->r.h >> 8)) * resHeight) / designHeight;
			}
			while (!(obj++->ob_flags & LASTOB));
		}
		else
		{
			/* Standard AES menu */

			j = 0;
			do {
				obfix(obj, j);
			}
			while (!(obj[j++].ob_flags & LASTOB));
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
		DIAGS(("Resources %ld(%lx) in global[5,6]\n", o, o));
		DIAGS(("      and %ld(%lx) in global[7,8]\n", h, h));
	}
}

/*
 * FreeResources : Dispose of a set of loaded resources
 * HR: improvements regarding multiple resources.
 */
void
FreeResources(XA_CLIENT *client, AESPB *pb)
{
	XA_RSCS *cur;
	RSHDR *rsc = NULL;

	if (pb)
		if (pb->globl)
			rsc = pb->globl->rshdr;

	cur = client->resources;
	DIAG((D_rsrc,client,"FreeResources: %ld for %d, ct=%d, pb->globl->rsc=%lx\n",
		cur, client->pid, client->rsct, rsc));

	if (cur && client->rsct)
	{
		/* Put older rsrc back, and the pointer in global */
		while (cur)
		{
			bool have = rsc && rsc == cur->rsc;
			XA_RSCS *nx = cur->next;
			if (   have
			    || (!rsc && cur->handle == client->rsct))	/* free the entry for the freed rsc. */
			{
				int i;
				RSHDR *hdr = cur->rsc;
				char *base = cur->rsc;
				OBJECT **trees,*obj;

				/* Free the memory allocated for scroll list objects. */

				(unsigned long)trees = (unsigned long)(base + hdr->rsh_trindex);
				for (i = 0; i < hdr->rsh_ntree; i++)
				{
					int f = 0;
					obj = trees[i];
					do
					{	if ((obj[f].ob_type & 255) == G_SLIST)
							XA_free(&client->base, obj[f].ob_spec.listbox);
					} while ( ! (obj[f++].ob_flags & LASTOB));
				}
		
				XA_free_all(&client->base, -1, client->rsct);

				if (cur->prior)
					cur->prior->next = cur->next;
				if (cur->next)
					cur->next->prior = cur->prior;
				DIAG((D_rsrc,client,"Free cur %lx\n", (long)cur - 16));
				XA_free(&client->base, cur);
			}
			else if (cur->handle == client->rsct - 1)
			{
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
 *	HR: The routines below are almost entirely rewritten such, that it is
 *      quite easy to see the subtle differences.
 *
 * ResourceTree : Find the tree with a given index
 * hdr = pointer to base of resources
 * num = index number of tree
 * Return = pointer to tree or object or stuff, or NULL on failure
 */

#define num_nok(t) (!hdr || num < 0 || num >= hdr->rsh_ ## t)
#define start(t) (unsigned long)index = (unsigned long)hdr + hdr->rsh_ ## t

/* HR: fixing up the pointer array is now done in Loadresources, to make it usable via global[5] */
OBJECT *
ResourceTree(RSHDR *hdr, int num)
{
	OBJECT **index;
	
	if num_nok(ntree)
		return NULL;
	start(trindex);
	return index[num];
}

/* Find the object with a given index  */
static OBJECT *
ResourceObject(RSHDR *hdr, int num)
{
	OBJECT *index;

	if num_nok(nobs)
		return NULL;
	start(object);
	return index + num;
}

/* Find the tedinfo with a given index */
static TEDINFO *
ResourceTedinfo(RSHDR *hdr, int num)
{
	TEDINFO *index;

	if num_nok(nted)
		return NULL;
	start(tedinfo);
	return index + num;
}

/* Colour icons are too new */

/* Find the iconblk with a given index */
static ICONBLK *
ResourceIconblk(RSHDR *hdr, int num)
{
	ICONBLK *index;

	if num_nok(nib)
		return NULL;
	start(iconblk);
	return index + num;
}

/* Find the bitblk with a given index */
static BITBLK *
ResourceBitblk(RSHDR *hdr, int num)
{
	BITBLK *index;

	if num_nok(nbb)
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

	if num_nok(nstring)
		return NULL;
	start(frstr);

	DIAG((D_s, NULL, "Gaddr 5 %lx '%s'\n", index[num], index[num]));
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

	if num_nok(nimages)
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

	DIAG((D_s, NULL, "Gaddr 15 %lx '%s'\n", index, *index));
	return index + num;
}

/* Find the ref to free images array */
static void **
ResourceFrimg(RSHDR *hdr, int num)
{
	void **index;

	start(frimg);

	DIAG((D_s, NULL, "Gaddr 16 %lx\n", index));
	return index + num;
}

unsigned long
XA_rsrc_load(LOCK lock, XA_CLIENT *client, AESPB *pb)
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
	path = shell_find(lock, client, pb->addrin[0]);
	
	if (path)
	{
		RSHDR *rsc;
		DIAG((D_rsrc, client, "rsrc_load('%s')\n", path));
	
		rsc = LoadResources(client, path, NULL, DU_RSX_CONV, DU_RSY_CONV);
		if (rsc)
		{
			OBJECT **o;
			(unsigned long)o = (unsigned long)rsc + rsc->rsh_trindex;
			client->rsrc = rsc;
			client->trees = o;

#if GENERATE_DIAGS
			if (client->globl_ptr != pb->globl)
			{
				DIAGS(("WARNING: rsrc_load global is different from appl_init's global\n"));
			}
#endif
			Rsrc_setglobal(rsc, client->globl_ptr);

			DIAG((D_rsrc,client,"pb %lx, gl %lx, gl->rsc %lx, gl->ptree %lx\n",
				pb, pb->globl, pb->globl->rshdr, pb->globl->ptree));

			if (pb->globl)
				/* Fill it out in the globl of the rsrc_load. */
				Rsrc_setglobal(rsc, pb->globl);

			pb->intout[0] = 1;
			return XAC_DONE;
		}
	}

	DIAGS(("ERROR: rsrc_load '%s' failed\n", pb->addrin[0] ? pb->addrin[0] : "~~"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_rsrc_free(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	CONTROL(0,1,0)

	if (client->rsrc)
	{
		FreeResources(client, pb);
		pb->intout[0] = 1;
	}
	else
	{
		pb->intout[0] = 0;
	}

	return XAC_DONE;
}

OBJECT **trees = NULL;
RSHDR *rsc = NULL;

unsigned long
XA_rsrc_gaddr(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	OBJECT **trees = NULL;
	RSHDR *rsc = NULL;
	void **addr = (void **)pb->addrout;
	short type = pb->intin[0], index = pb->intin[1];

	CONTROL(2,1,0)

	if (pb == NULL || client == NULL)
	{
		DIAGS(("rsrc_gaddr NULL %lx, %lx\n", pb, client));
	}

	DIAG((D_s,client,"rsrc_gaddr type %d, index %d pb %lx\n", type, index, pb));

	/* For multiple resource, first look at the supplied global ptr. */
	if (pb->globl)
	{
		rsc = pb->globl->rshdr;
		trees = pb->globl->ptree;
		DIAG((D_rsrc,client,"  --  pb->gl  rsc %lx, ptree %lx\n", rsc, trees));
	}
	
	/* The below represents the last resource loaded */
	if (!rsc && !trees)
	{
		/* Nothing specified: take the last rsc read. */

		rsc = client->rsrc; /* global is a structure */
		trees = client->trees;
		DIAG((D_rsrc,client,"  --  client->gl  rsc %lx, ptree %lx\n", rsc, trees));
	}

	if (!rsc && trees)
	{
		DIAG((D_s,client,"Tree pointer only!\n"));

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
			DIAG((D_s,client,"  from pb->globl --> %ld\n",*addr));		
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
		DIAG((D_rsrc,client,"NO resource loaded!!!\n"));
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
			*addr = (void *)ResourceObject(client->rsrc, index)->ob_spec.lspec;
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
		DIAG((D_s,client,"  --> %lx\n",*addr));		
		pb->intout[0] = 1;
	}
	else
		pb->intout[0] = 0;

	return XAC_DONE;
}

unsigned long
XA_rsrc_obfix(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	OBJECT *ob;
	int item = pb->intin[0];

	CONTROL(1,1,1)

	ob = pb->addrin[0];

	DIAG((D_rsrc, client, "rsrc_obfix for %s: tree %lx + %d\n",
		c_owner(client), ob, item));

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
XA_rsrc_rcfix(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	CONTROL(0,1,1)
	
	DIAG((D_rsrc, client, "rsrc_rcfix for %s on %ld(%lx)\n",
		c_owner(client), pb->addrin[0], pb->addrin[0]));

	client->rsrc = LoadResources(client, NULL, pb->addrin[0], DU_RSX_CONV, DU_RSY_CONV);
	if (client->rsrc)
	{
#if GENERATE_DIAGS
		if (client->globl_ptr != pb->globl)
		{
			DIAGS(("WARNING: rsrc_rcfix global %ld(%lx) is different from appl_init's global %ld(%lx)\n",
				pb->globl, pb->globl, client->globl_ptr, client->globl_ptr));
			/* client->globl_ptr = pb->globl; */
		}
#endif
		Rsrc_setglobal(client->rsrc, client->globl_ptr);

		pb->intout[0] = 1;
		return XAC_DONE;
	}

	pb->intout[0] = 0;
	return XAC_DONE;
}
