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

char version[16];
char vversion[32];
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
