/*
 * $Id$
 * 
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann
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

#include "xa_appl.h"
#include "xa_global.h"

#include "app_man.h"
#include "c_window.h"
#include "desktop.h"
#include "k_main.h"
#include "menuwidg.h"
#include "sys_proc.h"
#include "taskman.h"
#include "util.h"
#include "widgets.h"
#include "xalloc.h"

#include "xa_evnt.h"

#include "mint/fcntl.h"


bool
is_client(struct xa_client *client)
{
	return client && client != C.Aes;
}

static void
pname_to_string(char *to, unsigned long tolen, const char *name)
{
	int i = 8;

	strncpy(to, name, tolen);
	to[tolen-1] = '\0';

	while (i)
	{
		if (to[i] == ' ' || to[i] == 0)
			to[i] = '\0';
		else
			break;
		i--;
	}
}

static void
get_app_options(struct xa_client *client)
{
	struct opt_list *op = S.app_options;
	char proc_name[10];

	pname_to_string(proc_name, sizeof(proc_name), client->proc_name);

	DIAGS(("get_app_options '%s'", client->proc_name));

	while (op)
	{
		if (   stricmp(proc_name,        op->name) == 0
		    || stricmp(client->name + 2, op->name) == 0)
		{
			client->options = op->options;
			DIAGS(("Found '%s'", op->name));
			break;
		}

		op = op->next;
	}
}

static void
new_client(enum locks lock, struct xa_client *client)
{
	long f;

	DIAG((D_appl, NULL, "XA_new_client for %d", client->p->pid));

	strncpy(client->proc_name, client->p->name, 8);

	f = strlen(client->proc_name);
	while (f < 8)
	{
		client->proc_name[f] = ' ';
		f++;
	}

	client->proc_name[8] = '\0';
	strnupr(client->proc_name, 8);

	/* Individual option settings. */
	get_app_options(client);
	DIAGS(("proc_name for %d: '%s'", client->p->pid, client->proc_name));

	/* awaiting menu_register */
	sprintf(client->name, sizeof(client->name), "  %s", client->proc_name);
	update_tasklist(lock);

	DIAG((D_appl, NULL, "Init client '%s' pid=%d",
		client->proc_name, client->p->pid));

	DIAGS(("new_client: checking shel info (pid %i)", client->p->pid));
	{
		struct shel_info *info;

		info = lookup_extension(client->p, XAAES_MAGIC_SH);
		if (info)
		{
			DIAGS(("new_client: shel_write started"));
			DIAGS(("new_client: type %i", info->type));
			DIAGS(("new_client: cmd_name '%s'", info->cmd_name));
			DIAGS(("new_client: home_path '%s'", info->home_path));

			client->type = info->type;

			client->cmd_tail = info->cmd_tail;
			client->tail_is_heap = info->tail_is_heap;

			strcpy(client->cmd_name, info->cmd_name);
			strcpy(client->home_path, info->home_path);

			detach_extension(client->p, XAAES_MAGIC_SH);
		}
	}

	client->init = true;
}

/*
 * Application initialise - appl_init()
 * Remember that this executes under the CLIENT pid, not the kernal.
 * (Hence the semaphore locking on the routine)
 */

/* HR 230301:  new_client_contrl
 *             client_exit_contrl
 *             new_client_packet
 *             new_client_pb;
 * moved these areas to the client structure   !!!!!!!
 * Solved many problems with GEM-init of Ulrich Kayser.
 * Solved problems with proper shutdown. (Many apps issue appl_exit concurrently.
 * Sigh of relief. :-)
 */

unsigned long
XA_appl_init(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct aes_global *globl = (struct aes_global *)pb->global;
	struct proc *p = get_curproc();

	CONTROL(0,1,0);

	if (client)
	{
		ALERT(("Double appl_init for %s, ignored!\n", client->proc_name));
		goto clean_out;
	}

	client = attach_extension(NULL, XAAES_MAGIC, sizeof(*client), &xaaes_cb_vector);
	if (!client)
	{
		ALERT(("attach_extension for %u failed, out of memory?", p->pid));
		return XAC_DONE;
	}

	DIAG((D_appl, client, "appl_init for %d", p->pid));

	/* add to client list */
	{
		struct xa_client **end = &(S.client_list);
		struct xa_client *prior = NULL;

		while (*end)
		{
			prior = *end;
			end = &((*end)->next);
		}

		*end = client;
		client->prior = prior;
	}

	client->p = p;
	client->globl_ptr = globl;		/* Preserve the pointer to the globl array */
						/* so we can fill in the resource address later */
	client->cmd_tail = "\0";
	client->wt.edit_obj = -1;
	client->options = default_options;

	new_client(lock, client);

	/* Get the client's home directory (where it was started)
	 * - we use this later to load resource files, etc
	 */
	if (!*client->home_path)
	{
		int drv = d_getdrv();
		client->home_path[0] = (char)drv + 'A';
		client->home_path[1] = ':';
		d_getcwd(client->home_path + 2, drv + 1, sizeof(client->home_path) - 3);

		DIAG((D_appl, client, "[1]Client %d home path = '%s'",
			client->p->pid, client->home_path));
	}
	else /* already filled out by launch() */
	{
		DIAG((D_appl, client, "[2]Client %d home path = '%s'",
			client->p->pid, client->home_path));
	}

clean_out:
	/* Reset the AES messages pending list for our new application */
	client->msg = NULL;
	/* Initially, client isn't waiting on any event types */
	cancel_evnt_multi(client, 0);
	/* Initial settings for the clients mouse cursor */
	client->mouse = ARROW;		/* Default client mouse cursor is an arrow */
	client->mouse_form = NULL;
	client->save_mouse = client->mouse;
	client->save_mouse_form = client->mouse_form;

	pb->intout[0] = -1;

	/* fill out global */
	globl->version = 0x0410;		/* Emulate AES 4.1 */
	globl->count = -1;			/* unlimited applications XXX -> mint process limit */
	globl->id = p->pid;			/* appid -> pid */
	globl->pprivate = NULL;
	globl->ptree = NULL;			/* Atari: pointer to pointerarray of trees in rsc. */
	globl->rshdr = NULL;			/* Pointer to resource header. */
	globl->nplanes = screen.planes;
	globl->res1 = 0;
	globl->res2 = 0;
	globl->c_max_h = screen.c_max_h;	/* AES 4.0 extensions */
	globl->bvhard = 4;

	pb->intout[0] = p->pid;

	return XAC_DONE;
}

void
release_client(enum locks lock, struct xa_client *client)
{
	Sema_Up(clients);

	if (!client->killed)
		remove_attachments(lock|clients, client, client->std_menu.tree);

	if (client == S.client_list)
		S.client_list = client->next;

	if (client->prior)
		client->prior->next = client->next;
	if (client->next)
		client->next->prior = client->prior;

#if HALF_SCREEN
	/* If no client using the quart screen buffer is left, free it! */
	if (client->half_screen_buffer)
	{
		proc_free(client->half_screen_buffer);
		client->half_screen_buffer = NULL;
	}
#endif

	/* zero out */
	bzero(client, sizeof(*client));

	client->cmd_tail = "\0";
	client->wt.edit_obj = -1;

	Sema_Dn(clients);
}

static void
remove_wind_refs(struct xa_window *wl, struct xa_client *client)
{
	while (wl)
	{
		if (wl->owner == client)
		{			
			remove_widget(0, wl, XAW_TOOLBAR);
			remove_widget(0, wl, XAW_MENU   );
			wl->redraw = NULL;
		}
		wl = wl->next;
	}
}

/* remove all references to a clients memory. */
void
remove_refs(struct xa_client *client, bool secure)
{
	XA_TREE *menu_bar = get_menu();

	DIAGS(("remove_refs for %s mtree %lx %s",
		c_owner(client), client->std_menu.tree, secure ? "secure" : ""));

	root_window->owner = C.Aes;

	if (client->std_menu.tree)
		if (client->std_menu.tree == menu_bar->tree)
			*menu_bar = C.Aes->std_menu;

	client->std_menu.tree = NULL;

	if (secure)
	{
		/* Not called from within signal handler. */
		remove_wind_refs(window_list, client);
		remove_wind_refs(S.closed_windows.first, client);
	}

	if (client->desktop.tree)
	{
		if (get_desktop()->tree == client->desktop.tree)
		{
			set_desktop(&C.Aes->desktop);
			client->desktop.tree = NULL;
		}
	}

	client->killed = true;
	client->secured = secure;
}

/*
 * Close down the client reply pipe in response to an struct xa_client_EXIT message
 * HR: No!! it doesnt!
 * - Also does a tidy-up and deletes all the clients windows (in case some untidy programs
 *   fail to close them for themselves).
 * - Also disposes of any pending messages.
 *
 */
void
exit_client(enum locks lock, struct xa_client *client, int code)
{
	struct xa_client *top_owner;

	DIAG((D_appl,NULL,"XA_client_exit: %s %s", c_owner(client), client->killed ? "killed" : ""));

	/* Because of the window list, these cannot be done in the signal handler. */
	if (!client->secured)
	{
		remove_wind_refs(window_list, client);
		remove_wind_refs(S.closed_windows.first, client);
	}

	/* Go through and check that all windows belonging to this client are closed */
	remove_windows(lock, client);

	top_owner = window_list->owner;

	/* Dispose of any pending messages for the client */
	while (client->msg)
	{
		struct xa_aesmsg_list *nm = client->msg->next;
		free(client->msg);
		client->msg = nm;
	}

	if (client->attach)
	{
		/* if menu attachments */
#if GENERATE_DIAGS
		if (!client->killed)
		{
			XA_MENU_ATTACHMENT *at = client->attach;
			while (at->to_tree)
			{
				DIAGS(("tree left in attachments %lx", at->to_tree));
				at++;
			}
		}
#endif
		free(client->attach);
		client->attach = NULL;
	}

	app_in_front(lock, top_owner);

	/* If the client forgot to free its resources, we do it for him. */
	DIAG((D_appl, NULL, "Freeing client xmalloc base"));

	/* free all blocks allocated on behalf of the client. */
	/* XXX XA_free_all(&client->base, -1, -1); */

	client->rsrc = NULL;
	client->resources = NULL;

	/* Free name *only if* it is malloced: */
	/* HR dont compare with a pointer!!!! */
	if (client->tail_is_heap)		/* HR what if another constant was used? */
	{
		free(client->cmd_tail);
		client->cmd_tail = NULL;
		client->tail_is_heap = false;
	}

	/* Unlock mouse & screen */
	if (update_locked() == client)
		free_update_lock();
	if (mouse_locked() == client)
		free_mouse_lock();

	client->init = false;
	DIAG((D_appl, NULL, "client exit done"));

	if (client->p->ppid != C.AESpid)
	{
		/* Send a CH_EXIT message if the client
		 * is not an AES child of the XaAES system
		 */
		struct xa_client *parent = pid2client(client->p->ppid);

		/* is the parent a active AES client? */
		if (parent)
		{
			DIAGS(("sending CH_EXIT to %d for %s", client->p->ppid, c_owner(client)));

			send_app_message(NOLOCKS, NULL, parent,
					 CH_EXIT, 0, 0, client->p->pid,
					 code,    0, 0, 0);
		}
	}

	/* remove any references */
	remove_refs(client, false);
}

/*
 * Application Exit
 * This also executes under the CLIENT pid.
 * This closes the clients end of the reply pipe, and sends a message to the kernal
 * to tell it to close its end as well - the client tidy-up is done at the server
 * end when the struct xa_client_EXIT op-code is recieved, not here.
 */
unsigned long
XA_appl_exit(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(0,1,0)

	DIAG((D_appl,client,"appl_exit for %d", client->p->pid));

	/* Which process are we? It'll be a client pid */
	pb->intout[0] = client->p->pid;

	/* XXX ??? */
	if (strnicmp(client->proc_name, "wdialog", 7) == 0)
		return XAC_DONE;

	/* we assume correct termination */
	exit_client(lock, client, 0);

	/* and decouple from process context */
	detach_extension(NULL, XAAES_MAGIC);

	return XAC_DONE;
}

/*
 * Free timeslice.
 * Runs (of course) under the clients pid.
 */
unsigned long
XA_appl_yield(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(0,1,0)

	yield();

	pb->intout[0] = 1;	/* OK */
	return XAC_DONE;

}

/*
 * AES 4.0 compatible appl_search
 */
unsigned long
XA_appl_search(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_client *next = NULL;
	bool lang = false;
	bool spec = false;
	char *fname = (char *)pb->addrin[0];
	short *o = pb->intout;
	short cpid = pb->intin[0];

	CONTROL(1,3,1)

	o[0] = 1;
	o[1] = 0;

	DIAG((D_appl, client, "appl_search for %s cpid=0x%x", c_owner(client), cpid));

	if (cpid < 0)
	{
		cpid = -cpid;
		if (cpid < 1 || cpid > MAXPID)
			cpid = 1;

		next = pid2client(cpid);
		lang = true;
	}
	else if (cpid == APP_DESK)
	{
		/* N.AES: short name of desktop program */
		next = pid2client(C.DSKpid);
	}
	else
	{
		/* XaAES extension. */
		spec = (cpid & APP_TASKINFO) != 0;
		cpid &= ~APP_TASKINFO;

		if (spec)
			lang = true;

		Sema_Up(clients);

		if (cpid == APP_FIRST)
		{
			/* simply the first */
			next = S.client_list;
		}
		else if (cpid == APP_NEXT)
		{
			next = client->temp;
			if (next)
				next = next->next;
		}

		Sema_Dn(clients);
	}

	if (!next)
	{
		/* No more clients or no active clients */
		o[0] = 0;
	}
	else
	{
		cpid = next->p->pid;

		/* replies according to the compendium */		
		if (cpid == C.AESpid)
			o[1] = APP_SYSTEM;
		else if (cpid == C.DSKpid)
			o[1] = APP_APPLICATION | APP_SHELL;
		else if (next->type == APP_ACCESSORY)
			o[1] = APP_ACCESSORY;
		else
			o[1] = APP_APPLICATION;

		/* XaAES extensions. */
		if (spec)
		{
			if (any_hidden(lock, next))
				o[1] |= APP_HIDDEN;
			if (focus_owner() == next)
				o[1] |= APP_FOCUS;

			DIAG((D_appl, client, "   --   o[1] --> 0x%x", o[1]));
		}

		o[2] = cpid;

		if (lang)
			strcpy(fname, next->name);
		else
		{
			strncpy(fname, next->proc_name, 8);
			fname[8] = '\0';
		}

		client->temp = next;
	}

	return XAC_DONE;
}

static void
handle_XaAES_msgs(enum locks lock, union msg_buf *msg)
{
	union msg_buf m = *msg;
	int mt = m.s.msg;

	DIAGS(("Message to AES %d", mt));
	m.s.msg = 0;

	switch (mt)
	{
		case XA_M_EXEC:
		{
#if 0
			char *txt = m.s.p2;
			if (txt)
			{
				DIAGS(("Exec scl '%s'", txt));
				SCL(lock, 1, 0, 0, txt);
				m.s.msg = XA_M_OK;
			}
#endif
		}
		break;

		case XA_M_GETSYM:
		{
#if 0
			char *txt = m.s.p2;
			if (txt)
			{
				SYMBOL *t, *f = m.s.p1;
				DIAGS(("Get sym '%s'", txt));
				
				if (f)
				{
					t = find_sym(txt, 0);
					if (t)
					{
						DIAGS((" --> '%s'", t->s));
						*f = *t;
						m.s.msg = XA_M_OK;
					}
				}
			}
#endif
		}
		break;

		case XA_M_DESK:
		{
			DIAGS(("Desk %d, '%s'", m.s.m3, m.s.p2 ? m.s.p2 : "~~~"));
			if (m.s.p2 && m.s.m3)
			{
				strcpy(C.desk, m.s.p2);
				C.DSKpid = m.s.m3;
				m.s.msg = XA_M_OK;
			}
		}
		break;
	}

	{
		struct xa_client *dest_clnt;

		dest_clnt = pid2client(m.m[1]);
		if (dest_clnt)
			send_a_message(lock, dest_clnt, &m);
	}
}

/*
 * XaAES's current appl_write() only works for standard 16 byte messages
 */
unsigned long
XA_appl_write(enum locks lock, struct xa_client *client, AESPB *pb)
{
	int dest_id = pb->intin[0];
	int len = pb->intin[1];
	int rep = 1;
	union msg_buf *m = (union msg_buf *)pb->addrin[0];

	CONTROL(2,1,1)

	DIAGS(("appl_write: %d --> %d, len=%d msg = (%s) "
		"%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x\n",
		client->p->pid, dest_id, len,
		pmsg(m->m[0]),
		m->m[0], m->m[1], m->m[2], m->m[3], m->m[4], m->m[5], m->m[6], m->m[7]));

	if (len < 0)
		rep = 0;

	if (dest_id == C.AESpid)
	{
		handle_XaAES_msgs(lock, m);
	}
	else
	{
		struct xa_client *dest_clnt;

		dest_clnt = pid2client(dest_id);
		if (dest_clnt)
			send_a_message(lock, dest_clnt, m);
	}

	pb->intout[0] = rep;
	return XAC_DONE;
}

/*
 * Data table for appl_getinfo
 */
static short info_tab[17][4] =
{
	/* 0 large font */
	{
		0,
		0,
		0,
		0
	},
	/* 1 small font */
	{
		0,
		0,
		0,
		0
	},
	/* 2 colours */
	{
		1,		/* Getrez() */
		16,		/* no of colours */
		1,		/* colour icons */
		1		/* extended rsrc file format */
	},
	/* 3 language (english) */
	{
		0,
		0,
		0,
		0
	},
	/* 4 processes */
	{
		1,		/* preemptive */
		1,		/* convert mint id <--> aes id */
		1,		/* appl_search */
		1		/* rsrc_rcfix */
	},
	/* 5 PC_GEM (none!) */
	{
		0,		/* objc_xfind */
		0,
		0,		/* menu_click */
		0		/* shel_rdef/wdef */
	},
	/* 6 extended inquiry */
	{
		0,		/* -1 not a valid app id for appl_read */
		0,		/* -1 not a valid length parameter to shel_get */
		1,		/* -1 is a valid mode parameter to menu_bar */
		0		/* MENU_INSTL is not a valid mode parameter to menu_bar */
	},
	/* 7 MagiC specific */
	{
#if WDIAL
		3,		/* bit: 0 WDIALOG, 1 SCRLBOX, 2 FONTSEL, 3 FSELX, 4 PDLB  */
#else
		0,
#endif
		0,
		0,
		0
	},
	/* 8 mouse support */
	{
		1,		/* modes 258 - 260 applicable */
		1,		/* mouse form maintained per application */
		1,		/* mouse wheels support */
		0
	},
	/* 9 menu support */
	{
		1,		/* sub menus */
		1,		/* popup menus */
		1,		/* scrollable menus */
		1		/* MN_SELECTED provides object tree information */
	},
	/*10 AES shell support */
	{
		0x3f07,		/* supported extended bits + highest mode */
		0,		/* 0 launch mode */
		0,		/* 1 launch mode */
		1		/* ARGV style via wiscr to shel_write supported */
	},
	/*11 window functions
	 * 
	 * WF_COLOR and WF_DCOLOR are not completely supported. Especially not changing them.
	 * So the bits are off, although wind_get() will supply default values.
	 * 
	 * These values are'nt bits, so I dont think this is correct
	 * WF_TOP + WF_NEWDESK + WF_OWNER + WF_BEVENT + WF_BOTTOM + WF_ICONIFY + WF_UNICONIFY
	 * bit 9 WF_WHEEL */
	{
		01763,		/* see above */
		0,
		5,		/* window behaviours iconifier & click for bottoming */
		1		/* wind_update(): check and set available (mode + 0x100) */
	},
	/*12 messages
	 * WM_UNTOPPED + WM_ONTOP + AP_TERM + CH_EXIT (HR) + WM_BOTTOMED +
	 * WM_ICONIFY + WM_UNICONIFY
	 */
	{
		0756,		/* see above */ /* XXX is this correct? 0756 is octal */
		0,
		1,		/* WM_ICONIFY gives coordinates */
		0
	},
	/*13 objects */
	{
		1,		/* 3D objects */
		1,		/* objc_sysvar */
		0,		/* GDOS fonts */
		014		/* extended objects (0x8 G_SHORTCUT, 0x4 WHITEBAK objects)
				                     0x2 G_POPUP,    0x1 G_SWBUTTON */
	},
	/*14 form support (form_xdo, form_xdial) */
	{
		0,		/* MagiC flydials */
		0,		/* MagiC keyboard tables */
		0,		/* return last cursor position */
		0
	},
	/*15 <-- 64 */
	{
		0,		/* shel_write and AP_AESTERM */
		0,		/* shel_write and SHW_SHUTDOWN/SHW_RESCHANGE */
		3,		/* appl_search with long names and additive mode APP_TASKINFO available. */
		0		/* form_error and all GEMDOS errorcodes */
	},
	/*16 <-- 65 */
	{
		1,		/* appl_control exists. */
		APC_INFO,	/* highest opcode for appl_control */
		0,		/* shel_help exists. */
		0		/* wind_draw exists. */
	}
};										

/*
 * appl_getinfo() handler
 */
unsigned long
XA_appl_getinfo(enum locks lock, struct xa_client *client, AESPB *pb)
{
	unsigned short gi_type = pb->intin[0];

	CONTROL(1,5,0)

	/* Extremely curious to who's asking what. */
	DIAG((D_appl, client, "appl_getinfo %d for %s", gi_type, c_owner(client)));

	if (gi_type > 14)
	{
		/* N.AES extensions */
		if (gi_type == 64 || gi_type == 65)
		{
			gi_type -= 64 - 15;
		}
		else
		{
			/* "error" - unimplemented info type */

			pb->intout[0] = 0;
			return XAC_DONE;
		}
	}

	info_tab[0][0] = screen.standard_font_height;
 	info_tab[0][1] = screen.standard_font_id;
	info_tab[1][0] = screen.small_font_height;
	info_tab[1][1] = screen.small_font_id;

	info_tab[2][0] = xbios_getrez() + 2;

	pb->intout[0] = 1;
	pb->intout[1] = info_tab[gi_type][0];
	pb->intout[2] = info_tab[gi_type][1];
	pb->intout[3] = info_tab[gi_type][2];
	pb->intout[4] = info_tab[gi_type][3];

	return XAC_DONE;
}

/*
 * appl_find()
 *
 * HR: the -1 -2 support wasnt good at all
 */
unsigned long
XA_appl_find(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct nn { short m, id;};
	const struct nn *ex = (const struct nn *)pb->addrin;
	char *name = (char *)pb->addrin[0];
	short f = ex->id;

	CONTROL(0,1,1)

	DIAG((D_appl, client, "appl_find for %s", c_owner(client)));

	/* mint id <--> aes_id */
	if (ex->m == -1 || ex->m == -2)
	{
		pb->intout[0] = f;

		DIAG((D_appl, client, "   Mode 0xfff? --> %d", pb->intout[0]));
	}
	else if (ex->m == 0)
	{
		/* Return the pid of current process */
		pb->intout[0] = client->p->pid;

		DIAG((D_appl, client, "   Mode 0x0000 --> %d", pb->intout[0]));
	}
	else

	/* Tell application we understand appl_getinfo()
	 * (Invented by Martin Osieka for his AES extension WINX;
	 * used by MagiC 4, too.)
	 */
	if (strcmp(name, "?AGI") == 0)
	{
		/* OK */
		pb->intout[0] = 0;
		DIAG((D_appl, client, "   ?AGI"));
	}
	else
	{
		struct xa_client *cl;

		DIAG((D_appl, client, "   '%s'", pb->addrin[0]));
		pb->intout[0] = -1;

		Sema_Up(clients);

		/* use proper loop */
		cl = S.client_list;
		while (cl)
		{
			if (strnicmp(cl->proc_name, name, 8) == 0)
			{
				pb->intout[0] = cl->p->pid;
				DIAG((D_appl, client, "   --> %d", pb->intout[0]));
				break;
			}
			cl = cl->next;
		}

		Sema_Dn(clients);
	}

	return XAC_DONE;
}

/*
 * Extended XaAES calls
 */

unsigned long
XA_appl_control(enum locks lock, struct xa_client *client, AESPB *pb)
{
	int pid = pb->intin[0];
	short f = pb->intin[1];
	struct xa_client *cl = NULL;

	CONTROL(2,1,1)

	DIAG((D_appl, client, "appl_control for %s", c_owner(client)));

	if (pid == -1)
		cl = focus_owner();
	else
		cl = pid2client(pid);

	if (cl == NULL)
	{
		pb->intout[0] = 0;
	}
	else
	{
		pb->intout[0] = 1;

		DIAG((D_appl, client, "  --    on %s, func %d, 0x%lx",
			c_owner(cl), f, pb->addrin[0]));
	
		/**
		 * note: When extending this switch be sure to update
                 *       the appl_getinfo(65) mode corresponding structure
		 *  tip: grep APC_ * 
		 **/
		switch (f)
		{
			case APC_HIDE:
				hide_app(lock, cl);
			break;
			case APC_SHOW:
				unhide_app(lock, cl);
			break;
			case APC_TOP:
				app_in_front(lock, cl);
				if (cl->type == APP_ACCESSORY)
					send_app_message(lock, NULL, cl,
							 AC_OPEN, 0, 0, 0,
							 cl->p->pid, 0, 0, 0);
			break;
			case APC_HIDENOT:
				hide_other(lock, cl);
			break;
			case APC_INFO:
			{
				short *ii = (short*)pb->addrin[0];
				pb->intout[0] = 0;

				if (ii)
				{
					*ii = 0;
					if (any_hidden(lock, cl))
						*ii |= 1 /* APCI_HIDDEN */;
					if (cl->std_menu.tree)
						*ii |= 2 /* APCI_HASMBAR */;
					if (cl->desktop.tree)
						*ii |= 4 /* APCI_HASDESK */;

					pb->intout[0] = 1;
				}
			}
			break;
#if 0
			case APC_MENU:
			{
				OBJECT **mn = pb->addrin[0];
				pb->intout[0] = 0;

				if (mn)
				{
					*mn = cl->std_menu.tree;
					pb->intout[0] = 1;
				}
			}
			break;
#endif
			default:
				pb->intout[0] = 0;
		}

	}

	return XAC_DONE;
}
