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

#include "xa_global.h"
#include "version.h"
#include "vdi_parms.h"

struct nova_data *nova_data = NULL;
struct cookie_mvdi mvdi_api;

unsigned short next_res = 0;

char version[32];
char vversion[128];
char arch_target[] = ASCII_ARCH_TARGET;
char long_name[] = LONG_NAME;
char aes_id[] = AES_ID;
char info_string[256];

struct config cfg;
struct options default_options;
struct options local_options;
struct aesys_global G;
struct common C;
struct shared S;

struct xa_screen screen; /* The screen descriptor */
struct xa_objc_render objc_rend;

struct xa_vdi_settings global_vdi_settings;
struct xa_vdi_api *xa_vdiapi;

short border_mouse[CDV] =
{
	XACRS_SE_SIZER,
	XACRS_VERTSIZER,
	XACRS_NE_SIZER,
	XACRS_HORSIZER,
	XACRS_SE_SIZER,
	XACRS_VERTSIZER,
	XACRS_NE_SIZER,
	XACRS_HORSIZER
};

XA_PENDING_WIDGET widget_active;

// XA_TREE nil_tree;

struct xa_client *
pid2client(short pid)
{
	struct proc *p = pid2proc(pid);
	struct xa_client *client = p ? lookup_extension(p, XAAES_MAGIC) : NULL;

	if (!client && pid == C.Aes->p->pid)
		client = C.Aes;

	DIAGS(("pid2client(%i) -> p %lx client %s",
		pid, (unsigned long)p, client ? client->name : "NULL"));

	return client;
}

#if ALT_CTRL_APP_OPS
struct xa_client *
proc2client(struct proc *p)
{
	return lookup_extension(p, XAAES_MAGIC);
}
#endif

void *
lookup_xa_data(struct xa_data_hdr **list, void *_data)
{
	struct xa_data_hdr *data = _data;

	while (*list)
	{
		if (*list == data)
			break;

		list = &((*list)->next);
	}
	return *list;
}

void *
lookup_xa_data_byname(struct xa_data_hdr **list, char *name)
{
	while (*list)
	{
		if (!strcmp((*list)->name, name))
			break;

		list = &((*list)->next);
	}
	return *list;
}
void *
lookup_xa_data_byid(struct xa_data_hdr **list, long id)
{
	while (*list)
	{
		if ((*list)->id == id)
			break;

		list = &((*list)->next);
	}
	return *list;
}

void *
lookup_xa_data_byidname(struct xa_data_hdr **list, long id, char *name)
{
	while (*list)
	{
		if ((*list)->id == id && !strcmp((*list)->name, name) )
			break;

		list = &((*list)->next);
	}
	return *list;
}

void
add_xa_data(struct xa_data_hdr **list, void *_data, long id, char *name, void (*destruct)(void *d))
{
	int i;
	struct xa_data_hdr *data = _data;

	while (*list && (*list)->next)
		list = &((*list)->next);

	if (*list)
		(*list)->next = data;
	else
		*list = data;

	data->next = NULL;

	if (name)
	{
		for (i = 0; i < 15 && (data->name[i] = *name++); i++)
			;
	}
	else
	{
		for (i = 0; i < 15; i++)
			data->name[i] = '\0';
	}
	data->name[15] = '\0';
	data->id = id;
	data->destruct = destruct;
}

void
remove_xa_data(struct xa_data_hdr **list, void *_data)
{
	struct xa_data_hdr *data = _data;

	while (*list)
	{
		if (*list == data)
		{
			*list = (*list)->next;
			data->next = NULL;
			break;
		}
		list = &((*list)->next);
	}
}

void
delete_xa_data(struct xa_data_hdr **list, void *_data)
{
	struct xa_data_hdr *data = _data;

	remove_xa_data(list, data);

	if (data->destruct)
		(*data->destruct)(data);
}

void
ref_xa_data(struct xa_data_hdr **list, void *_data, short count)
{
	struct xa_data_hdr *data = _data;
	data->links += (long)count;
}

long
deref_xa_data(struct xa_data_hdr **list, void *_data, short flags)
{
	long ret;
	struct xa_data_hdr *data = _data;

	data->links--;
	if (!(ret = data->links) && (flags & 1))
	{
		delete_xa_data(list, data);
	}
	return ret;
}

void
free_xa_data_list(struct xa_data_hdr **list)
{
	struct xa_data_hdr *l = *list;
	while (l)
	{
		struct xa_data_hdr *n = l->next;
		l->next = NULL;
		if (l->destruct)
			(*l->destruct)(l);
		l = n;
	}
	*list = NULL;
}

/*
 * Setup a new VDI parameter block
 */
XVDIPB *
create_vdipb(void)
{
	XVDIPB *v;
	short *p, **e;
	int i;

	v = kmalloc(sizeof(XVDIPB) + ((12 + 500 + 500 + 500 + 500) << 1) );

	p = (short *)((long)v + sizeof(XVDIPB));
	v->control = p;
	p += 12;

	e = &v->intin;
	for (i = 0; i < 4; i++)
	{
		*e++ = p;
		p += 500;
	}
	return v;
}
#if 0
/*
 * callout the VDI
 */
void
do_vdi_trap (XVDIPB * vpb)
{
	__asm__ volatile
	(
		"movea.l	%0,a0\n\t" 	\
		"move.l		a0,d1\n\t"	\
		"move.w		#115,d0\n\t"	\
		"trap		#2\n\t"		\
		:
		: "a"(vpb)
		: "a0", "d0", "d1", "memory"
	);
}

void
VDI(XVDIPB *vpb, short c0, short c1, short c3, short c5, short c6)
{
	vpb->control[V_OPCODE   ] = c0;
	vpb->control[V_N_PTSIN  ] = c1;
	vpb->control[V_N_INTIN  ] = c3;
	vpb->control[V_SUBOPCODE] = c5;
	vpb->control[V_HANDLE   ] = c6;

	do_vdi_trap(vpb);
}
#endif
void
get_vdistr(char *d, short *s, short len)
{
	for (;len >= 0; len--)
	{
		if (!(*d++ = *s++))
			break;
	}
	*d = '\0';
}

/*
 * our private VDI calls
 */
void
xvst_font(XVDIPB *vpb, short handle, short id)
{
	vpb->intin[0] = id;
	VDI(vpb, 21, 0, 1, 0, handle);
}


XFNT_INFO *
xvqt_xfntinfo(XVDIPB *vpb, short handle, short flags, short id, short index)
{
	XFNT_INFO *x;

	if ((x = kmalloc(sizeof(*x))))
	{
		x->size = sizeof(*x);
		V_XFNT_INFO(vpb, handle, flags, id, index, x);
#if 0
		x->size = sizeof(*x);
		vpb->intin[0] = flags;
		vpb->intin[1] = id;
		vpb->intin[2] = index;
		*(XFNT_INFO **)&vpb->intin[3] = x;
		VDI(vpb, 229, 0, 5, 0, handle);
#endif
	}
	return x;
}

short
xvst_point(XVDIPB *vpb, short handle, short point)
{
	short ret;
	VST_POINT(vpb, handle, point, ret);
	return ret;
#if 0
	vpb->intin[0] = point;
	VDI(vpb, 107, 0, 1, 0, handle);
	return vpb->intout[0];
#endif
}

#if 0
/* ***************************************************** */
static short
xvq_devinfo(XVDIPB *vpb, short handle, short dev_id)
{
	vpb->intin[0] = dev_id;
	VDI(vpb, 248, 0, 1, 0, handle);
	return vpb->intout[0];
}

static void
dump_devstuff(XVDIPB *vpb, short handle)
{
	int i, j;
	char rn[200], fn[100];

	for (i = 0; i < 100; i++)
	{
		xvq_devinfo(vpb, handle, i);
		if (vpb->control[4])
		{
			for (j = 0; j < ((vpb->control[2] - 1) << 1) && j < 200; j++)
				rn[j] = *(char *)((long)vpb->ptsout + j + 2), rn[j+1] = '\0';
			for (j = 0; j < vpb->control[4] && j < 200 && vpb->intout[j]; j++)
				fn[j] = (char)vpb->intout[j], fn[j + 1] = '\0';

			DIAGS(("driver %d is %s, name %s, file %s", i, vpb->ptsout[0] ? "Open" : "Closed", rn, fn));
		} else
		{
			DIAGS(("No driver at ID %d", i));
		}
	}
}
/* ***************************************************** */
#endif

