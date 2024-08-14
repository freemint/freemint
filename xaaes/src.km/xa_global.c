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

struct nova_data *nova_data = NULL;

unsigned short next_res = 0;
unsigned short next_dev = 0;

char const aes_version[] = AES_ASCII_VERSION;
char const aes_version_verbose[] = AES_ASCII_VERSION_VERBOSE;
char arch_target[] = ASCII_ARCH_TARGET;
char long_name[] = LONG_NAME;
char aes_id[] = AES_ID;
char info_string[256];
char **xa_strings;

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
