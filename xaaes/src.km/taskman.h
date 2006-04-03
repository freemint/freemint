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

#ifndef _taskman_h
#define _taskman_h

#include "global.h"
#include "xa_types.h"

struct helpthread_data * get_helpthread_data(struct xa_client *client);

void add_to_tasklist(struct xa_client *client);
void remove_from_tasklist(struct xa_client *client);
void update_tasklist_entry(struct xa_client *client);

void quit_all_apps(enum locks lock, struct xa_client *except, short reason);
void quit_all_clients(enum locks lock, struct cfg_name_list *except_nl, struct xa_client *except_cl, short reason);

bool isin_namelist(struct cfg_name_list *list, char *name, short nlen, struct cfg_name_list **last, struct cfg_name_list **prev);
void addto_namelist(struct cfg_name_list **list, char *name);
void removefrom_namelist(struct cfg_name_list **list, char *name, short nlen);
void free_namelist(struct cfg_name_list **list);


void open_taskmanager(enum locks lock, struct xa_client *client, bool open);
void open_systemalerts(enum locks lock, struct xa_client *client, bool open);
void open_reschange(enum locks lock, struct xa_client *client, bool open);
void open_milan_reschange(enum locks lock, struct xa_client *client, bool open);
void open_nova_reschange(enum locks lock, struct xa_client *client, bool open);
void open_falcon_reschange(enum locks lock, struct xa_client *client, bool open);
void CE_open_csr(enum locks lock, struct c_event *ce, bool cancel);
void CE_abort_csr(enum locks lock, struct c_event *ce, bool cancel);
void cancel_csr(struct xa_client *running);

void open_imgload(enum locks lock);

void do_system_menu(enum locks lock, int clicked_title, int menu_item);

#endif /* _taskman_h */
