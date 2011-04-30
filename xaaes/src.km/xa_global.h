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

#ifndef _xa_global_h
#define _xa_global_h

#include "global.h"
#include "xa_types.h"
#include "mvdi.h"
#include "xcb.h"

# define SHUT_POWER	0
# define SHUT_BOOT	1
# define SHUT_COLD	2
# define SHUT_HALT	3

extern struct nova_data *nova_data;
extern struct cookie_mvdi mvdi_api;

extern unsigned long next_res;

extern char version[32];
extern char vversion[128];

extern char arch_target[];
extern char long_name[];
extern char aes_id[];

extern char info_string[256];


/* Area's shared between server and client, subject to locking. */
extern struct shared S;


/* CLIENT list operations */

#define CLIENT_LIST_INIT() \
	LIST_INIT(&(S.client_list))

#define CLIENT_LIST_START \
	LIST_START(&(S.client_list))

#define NEXT_CLIENT(client) \
	LIST_NEXT(client,client_entry)

#define PREV_CLIENT(client) \
	LIST_PREV(client,client_entry)

#define CLIENT_LIST_INSERT_START(client) \
	LIST_INSERT_START(&(S.client_list), client, client_entry);

#define CLIENT_LIST_INSERT_END(client) \
	LIST_INSERT_END(&(S.client_list), client, client_entry, xa_client);

#define CLIENT_LIST_REMOVE(client) \
	LIST_REMOVE(&(S.client_list), client, client_entry)

#define FOREACH_CLIENT(client) \
	LIST_FOREACH(&(S.client_list), client, client_entry)

static inline size_t
client_list_size(void)
{
	struct xa_client *cl;
	size_t length = 0;

	FOREACH_CLIENT(cl)
		length++;

	return length;
}


/* APP list operations */

#define APP_LIST_INIT() \
	LIST_INIT(&(S.app_list))

#define APP_LIST_START \
	LIST_START(&(S.app_list))

#define NEXT_APP(client) \
	LIST_NEXT(client,app_entry)

#define PREV_APP(client) \
	LIST_PREV(client,app_entry)

#define APP_LIST_INSERT_START(client) \
	LIST_INSERT_START(&(S.app_list), client, app_entry);

#define APP_LIST_INSERT_END(client) \
	LIST_INSERT_END(&(S.app_list), client, app_entry, xa_client);

#define APP_LIST_REMOVE(client) \
	LIST_REMOVE(&(S.app_list), client, app_entry)

#define FOREACH_APP(client) \
	LIST_FOREACH(&(S.app_list), client, app_entry)

/* task administration block list */
#define TAB_LIST_INIT() \
	LIST_INIT(&(S.menu_base))

#define TAB_LIST_START \
	LIST_START(&(S.menu_base))

#define NEXT_TAB(tab) \
	LIST_NEXT(tab,tab_entry)

#define PREV_TAB(tab) \
	LIST_PREV(tab,tab_entry)

#define TAB_LIST_INSERT_START(tab) \
	LIST_INSERT_START(&(S.menu_base), tab, tab_entry);

#define TAB_LIST_INSERT_END(tab) \
	LIST_INSERT_END(&(S.menu_base), tab, tab_entry, task_administration_block);

#define TAB_LIST_REMOVE(tab) \
	LIST_REMOVE(&(S.menu_base), tab, tab_entry)

#define FOREACH_TAB(tab) \
	LIST_FOREACH(&(S.menu_base), tab, tab_entry)

static inline size_t
tab_list_size(void)
{
	struct task_administration_block *tab;
	size_t length = 0;

	FOREACH_TAB(tab)
		length++;

	return length;
}

/* All areas that are common. */
extern struct common C;

extern struct aesys_global G;


/* Global config data */
extern struct config cfg;

/* The screen descriptor */
extern struct xa_screen screen;
extern struct xa_objc_render objc_rend;
extern struct xa_vdi_settings global_vdi_settings;
extern struct xa_vdi_api *xa_vdiapi;

#define MONO (screen.colours < 16)

//extern struct xa_widget_theme default_widget_theme;
extern struct options default_options;
extern struct options local_options;

extern XA_PENDING_WIDGET widget_active;

extern struct moose_data mainmd;

extern short border_mouse[];

extern const char mnu_clientlistname[];

// extern XA_TREE nil_tree;

/* mscall.c */
void hidem(void);
void showm(void);
void forcem(void);
int tellm(void);

struct xa_client *pid2client(short pid);
struct xa_client *proc2client(struct proc *p);

void *	lookup_xa_data		(struct xa_data_hdr **l,    void *_data);
void *	lookup_xa_data_byname	(struct xa_data_hdr **list, char *name);
void *	lookup_xa_data_byid	(struct xa_data_hdr **list, long id);
void *	lookup_xa_data_byidname	(struct xa_data_hdr **list, long id, char *name);
void	add_xa_data		(struct xa_data_hdr **list, void *_data, long id, char *name, void _cdecl(*destruct)(void *d));
void	remove_xa_data		(struct xa_data_hdr **list, void *_data);
void	delete_xa_data		(struct xa_data_hdr **list, void *_data);
void	ref_xa_data		(struct xa_data_hdr **list, void *_data, short count);
long	deref_xa_data		(struct xa_data_hdr **list, void *_data, short flags);
void	free_xa_data_list	(struct xa_data_hdr **list);

/* Global VDI calls */
XVDIPB *	create_vdipb(void);
// void		do_vdi_trap (XVDIPB * vpb);
// void		VDI(XVDIPB *vpb, short c0, short c1, short c3, short c5, short c6);
void		get_vdistr(char *d, short *s, short len);
void		xvst_font(XVDIPB *vpb, short handle, short id);
XFNT_INFO *	xvqt_xfntinfo(XVDIPB *vpb, short handle, short flags, short id, short index);
short		xvst_point(XVDIPB *vpb, short handle, short point);

static inline void
do_vdi_trap (XVDIPB * vpb)
{
	__asm__ volatile
	(
		"movea.l	%0,a0\n\t" 		\
		"move.l		a0,d1\n\t"		\
		"move.w		#115,d0\n\t"		\
		"trap		#2\n\t"			\
		:
		: "a"(vpb)
		: "a0", "d0", "d1", "memory"
	);
}

static inline void
VDI(XVDIPB *vpb, short c0, short c1, short c3, short c5, short c6)
{
	vpb->control[V_OPCODE   ] = c0;
	vpb->control[V_N_PTSIN  ] = c1;
	vpb->control[V_N_INTIN  ] = c3;
	vpb->control[V_SUBOPCODE] = c5;
	vpb->control[V_HANDLE   ] = c6;

	do_vdi_trap(vpb);
}

#endif /* _xa_global_h */
