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

#include "xa_global.h"
#include "version.h"

char version[32];
char vversion[128];
char arch_target[] = ASCII_ARCH_TARGET;
char long_name[] = LONG_NAME;
char aes_id[] = AES_ID;
char info_string[256];

struct config cfg;
struct options default_options;
struct options local_options;
struct common C;
struct shared S;

struct xa_screen screen; /* The screen descriptor */

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

const char mnu_clientlistname[] = "  Clients \3";

XA_TREE nil_tree;

struct xa_client *
pid2client(short pid)
{
	struct proc *p = pid2proc(pid);
	struct xa_client *client = p ? lookup_extension(p, XAAES_MAGIC) : NULL;

	if (!client && pid == C.Aes->p->pid)
		client = C.Aes;
	
	DIAGS(("pid2client(%i) -> p %lx client %s",
		pid, p, client ? client->name : "NULL"));

	return client;
}

struct xa_client *
proc2client(struct proc *p)
{
	return lookup_extension(p, XAAES_MAGIC);
}


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

void
add_xa_data(struct xa_data_hdr **list, void *_data, void (*destruct)(void *d))
{
	struct xa_data_hdr *data = _data;

	while (*list && (*list)->next)
		list = &((*list)->next);

	if (*list)
		(*list)->next = data;
	else
		*list = data;

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
			break;
		}
		list = &((*list)->next);
	}
}

void
delete_xa_data(struct xa_data_hdr **list, void *_data)
{
	struct xa_data_hdr *data = _data;
	//display("delete_xa_data: %lx (destruct=%lx)", data, (long)data->destruct);
	
	remove_xa_data(list, data);
	
	if (data->destruct)
		(data->destruct)(data);
}

void
free_xa_data_list(struct xa_data_hdr **list)
{
	struct xa_data_hdr *l = *list;
	//display("free_xa_data_list:");
	while (l)
	{
		struct xa_data_hdr *n = l->next;
		//display(" --- Calling xa_data_destruct: %lx (destruct=%lx)", l, (long)l->destruct);
		if (l->destruct)
			(*l->destruct)(l);
		l = n;
	}
}
