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

#ifndef _taskman_h
#define _taskman_h

#include "global.h"
#include "xa_types.h"

struct xa_window * _cdecl create_dwind(int lock, XA_WIND_ATTR tp, char *title, struct xa_client *client, struct widget_tree *wt, FormExit(*f), WindowDisplay(*d));

struct helpthread_data * get_helpthread_data(struct xa_client *client);

extern struct xa_wtxt_inf norm_txt;
extern struct xa_wtxt_inf acc_txt;
extern struct xa_wtxt_inf prg_txt;
extern struct xa_wtxt_inf sys_txt;
extern struct xa_wtxt_inf sys_thrd;
extern struct xa_wtxt_inf desk_txt;

extern GRECT systemalerts_r;
extern GRECT taskman_r;

int xaaes_do_form_alert( int lock, struct xa_client *client, int def_butt, char al_text[] );

short	set_xa_fnt( int pt, struct xa_wtxt_inf *wp[], OBJECT *obtree, int objs[], SCROLL_INFO *list, short *wd, short *hd );

void add_window_to_tasklist(struct xa_window *wi, const char *title);
void add_to_tasklist(struct xa_client *client);
void remove_from_tasklist(struct xa_client *client);

#define NO_AES_CLIENT	0
#define AES_CLIENT	1

/* opcodes for handle_launcher */
#define HL_LOAD_GRAD  0
#define HL_LAUNCH     1
#define HL_LOAD_IMG   2
#define HL_LOAD_PAL   3
#define HL_LOAD_CFG   4


void update_tasklist_entry(int md, void *app, struct helpthread_data *htd, long pid, int redraw);

void ce_quit_all_clients(int lock, struct xa_client *client, bool b);
void quit_all_apps(int lock, struct xa_client *except, short reason);

bool isin_namelist(struct cfg_name_list *list, char *name, short nlen, struct cfg_name_list **last, struct cfg_name_list **prev);
void addto_namelist(struct cfg_name_list **list, char *name);
void removefrom_namelist(struct cfg_name_list **list, char *name, short nlen);
void free_namelist(struct cfg_name_list **list);

void send_terminate(int lock, struct xa_client *client, short reason);

void CHlp_aesmsg(struct xa_client *client);
void screen_dump(int lock, struct xa_client *client, short open);

void force_window_top( int lock, struct xa_window *wind );
void wakeup_client(struct xa_client *client);
void app_or_acc_in_front( int lock, struct xa_client *client );
struct xa_client *is_aes_client( struct proc *p );
void open_taskmanager(int lock, struct xa_client *client, short open);
void open_systemalerts(int lock, struct xa_client *client, short open);
void open_launcher(int lock, struct xa_client *client, int what);
void CE_open_csr(int lock, struct c_event *ce, short cancel);
void CE_abort_csr(int lock, struct c_event *ce, short cancel);
void cancel_csr(struct xa_client *running);

void open_imgload(int lock);

void do_system_menu(int lock, int clicked_title, int menu_item);

#endif /* _taskman_h */
