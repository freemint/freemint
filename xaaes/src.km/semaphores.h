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

#ifndef _xaaes_semaphores_h
#define _xaaes_semaphores_h

#include "global.h"

struct xa_client;

struct proc *update_locked(void);
struct proc *mouse_locked(void);
struct proc *menustruct_locked(void);

struct xa_client *get_update_locker(void);
struct xa_client *get_mouse_locker(void);
struct xa_client *get_menustruct_locker(void);

void free_update_lock(void);
void free_mouse_lock(void);
void free_menustruct_lock(void);

bool   lock_screen(struct proc *proc, bool try);
bool unlock_screen(struct proc *proc);
bool   lock_mouse(struct proc *proc, bool try);
bool unlock_mouse(struct proc *proc);
bool   lock_menustruct(struct proc *proc, bool try);
bool unlock_menustruct(struct proc *proc);

/*-----------------------------------------------------------------
 * Lock control
 *-----------------------------------------------------------------*/

#define NOLOCKS         0x000
#define NOLOCKING       -1
#define LOCK_APPL       0x001
#define LOCK_WINLIST    0x008
#define LOCK_DESK       0x010
#define LOCK_CLIENTS    0x020
#define LOCK_FSEL       0x040
#define LOCK_LCK_UPDATE 0x080
#define LOCK_MOUSE      0x100
#define LOCK_ENVSTR     0x200
#define LOCK_PENDING    0x400

#if 1

#define Sema_Up(id)
#define Sema_Dn(id)

#else

/*----------------------------------------------------------------- */
/* Define the semaphores used in various places... */

#define APPL_INIT_SEMA	0x58410001 /* 'XA:I' Semaphore id for appl_init() routine access */
#define WIN_LIST_SEMA	0x58410004 /* 'XA:W' Semaphore for order modify / entry delete access to the window list */
#define ROOT_SEMA	0x58410008 /* 'XA:R' Semaphore for access to the root window */
#define CLIENTS_SEMA	0x58410010 /* 'XA:C' Semaphore for access to the clients structure */
#define FSELECT_SEMA	0x58410020 /* 'XA:F' Semaphore for access to the file selector */
#define ENV_SEMA	0x58410040 /* 'XA:E' Semaphore for access to the environment strings */

#define UPDATE_LOCK	0x58510080 /* 'XA:U' Semaphore id for BEG_UPDATE */
#define MOUSE_LOCK	0x58410100 /* 'XA:M' Semaphore id for BEG_MCTRL */

#define PENDING_SEMA	0x58410200 /* 'XA:P' Semaphore id to guard pending button&keybd events */


#define LOCK_APPL_SEMA	APPL_INIT_SEMA
#define LOCK_WINLIST_SEMA	WIN_LIST_SEMA
#define LOCK_DESK_SEMA	ROOT_SEMA
#define LOCK_CLIENTS_SEMA	CLIENTS_SEMA
#define LOCK_FSEL_SEMA	FSELECT_SEMA
#define LOCK_ENVSTR_SEMA	ENV_SEMA

#define LOCK_LCK_UPDATE_SEMA	UPDATE_LOCK
#define LOCK_MOUSE_SEMA	MOUSE_LOCK

#define LOCK_PENDING_SEMA	PENDING_SEMA


#if GENERATE_DIAGS && DEBUG_SEMA

/* The semaphores are crucial, so have ample debugging features. */
#define Sema_Up(id) if (!(lock&id))\
			 { long r; DIAGS((D_su,(short)id)); \
			     r = p_semaphore(2,id ## _SEMA,-1); \
			     if (r < 0) DIAGS((D_sr,r));}
#define Sema_Dn(id) if (!(lock&id)){long r = p_semaphore(3,id ## _SEMA,0); DIAGS((D_sd,(short)id,r));}
#else

#define Sema_Up(id) if (!(lock & id)) p_semaphore(2, id ## _SEMA, -1)
#define Sema_Dn(id) if (!(lock & id)) p_semaphore(3, id ## _SEMA,  0)

#endif

#define unlocked(a) (!(lock & a))

int create_semaphores(struct file *);
void destroy_semaphores(struct file *);

#endif

#endif /* _xaaes_semaphores_h */
