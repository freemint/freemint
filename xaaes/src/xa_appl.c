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

#include <mint/mintbind.h>

#include "xa_types.h"
#include "xa_global.h"

#include "app_man.h"
#include "bootup.h"
#include "config.h"
#include "ipff.h"


/*
 * AES 4.0 compatible appl_search
 */
unsigned long
XA_appl_search(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	XA_CLIENT *next = NULL;
	bool lang = false;
	bool spec = false;
	char *fname = (char *)pb->addrin[0];
	short *o = pb->intout, cpid = pb->intin[0];

	CONTROL(1,3,1)

	o[0] = 1;
	o[1] = 0;

	/* DIAG((D_appl, client, "appl_search for %s cpid=0x%x\n", c_owner(client), cpid)); */

	if (cpid < 0)
	{
		cpid = -cpid;
		if (cpid < 1 || cpid > MAX_PID)
			cpid = 1;
		next = Pid2Client(cpid);
		lang = true;
	}
	else if (cpid == APP_DESK) /* N.aes  short name of desktop program */
	{
		next = Pid2Client(C.DSKpid);
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

		/* skip inactive clients */
		while (next && !next->client_end)
			next = next->next;

		Sema_Dn(clients);
	}

	if (!next || (next && !next->client_end))
	{
		/* No more clients or no active clients */
		o[0] = 0;
	}
	else
	{
		cpid = next->pid;

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
			DIAG((D_appl, client, "   --   o[1] --> 0x%x\n", o[1]));
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
handle_XaAES_msgs(LOCK lock, union msg_buf *msg)
{
	union msg_buf m = *msg;
	int mt = m.s.msg;

	DIAGS(("Message to AES %d\n", mt));
	m.s.msg = 0;

	switch (mt)
	{
		case XA_M_EXEC:
		{
			char *txt = m.s.p2;
			if (txt)
			{
				DIAGS(("Exec scl '%s'\n", txt));
				SCL(lock, 1, 0, 0, txt);
				m.s.msg = XA_M_OK;
			}
		}
		break;

		case XA_M_GETSYM:
		{
			char *txt = m.s.p2;
			if (txt)
			{
				SYMBOL *t, *f = m.s.p1;
				DIAGS(("Get sym '%s'\n", txt));
				
				if (f)
				{
					t = find_sym(txt, 0);
					if (t)
					{
						DIAGS((" --> '%s'\n", t->s));
						*f = *t;
						m.s.msg = XA_M_OK;
					}
				}
			}
		}
		break;

		case XA_M_DESK:
		{
			DIAGS(("Desk %d, '%s'\n", m.s.m3, m.s.p2 ? m.s.p2 : "~~~"));
			if (m.s.p2 && m.s.m3)
			{
				strcpy(C.desk, m.s.p2);
				C.DSKpid = m.s.m3;
				m.s.msg = XA_M_OK;
			}
		}
		break;
	}

	send_a_message(lock, m.m[1], &m);
}

/*
 * XaAES's current appl_write() only works for standard 16 byte messages
 */
unsigned long
XA_appl_write(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	int dest_id = pb->intin[0], len = pb->intin[1], rep = 1;
	union msg_buf *m = pb->addrin[0];

	CONTROL(2,1,1)

	DIAGS(("appl_write: %d --> %d, len=%d msg = (%s) "
		"%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x\n",
		client->pid, dest_id, len,
		pmsg(m->m[0]),
		m->m[0], m->m[1], m->m[2], m->m[3], m->m[4], m->m[5], m->m[6], m->m[7]));

	if (len < 0)
		rep = 0;

	if (dest_id == C.AESpid)
		handle_XaAES_msgs(lock, m);
	else
		send_a_message(lock, dest_id, m);

	pb->intout[0] = rep;

	return XAC_DONE;
}

/*
 * Data table for appl_getinfo
 */
static short info_tab[17][4] =
{
	{0, 0, 0, 0},		/* 0 large font */
	{0, 0, 0, 0},		/* 1 small font */
	{			/* 2 colours */
		1,		/* Getrez() */
		16,		/* no of colours */
		1,		/* colour icons */
		1		/* extended rsrc file format */
	},
	{0, 0, 0, 0},		/* 3 language (english) */
	{			/* 4 processes */
		1,		/* preemptive */
		1,		/* convert mint id <--> aes id */
		1,		/* appl_search */
		1		/* rsrc_rcfix */
	},
	{			/* 5 PC_GEM (none!) */
		0,		/* objc_xfind */
		0,
		0,		/* menu_click */
		0		/* shel_rdef/wdef */
	},
	{			/* 6 extended inquiry */
		0,		/* -1 not a valid app id for appl_read */
		0,		/* -1 not a valid length parameter to shel_get */
		1,		/* -1 is a valid mode parameter to menu_bar */
		0		/* MENU_INSTL is not a valid mode parameter to menu_bar */
	},
	{			/* 7 MagiC specific */
#if WDIAL
		3,		/* bit: 0 WDIALOG, 1 SCRLBOX, 2 FONTSEL, 3 FSELX, 4 PDLB  */
#else
		0,
#endif
		0,
		0,
		0
	},
	{			/* 8 mouse support */
		1,		/* modes 258 - 260 applicable */
		1,		/* mouse form maintained per application */
		1,		/* mouse wheels support */
		0
	},
	{			/* 9 menu support */
		1,		/* sub menus */
		1,		/* popup menus */
		1,		/* scrollable menus */
		1		/* MN_SELECTED provides object tree information */
	},
	{			/*10 AES shell support */
		0x3f07,		/* supported extended bits + highest mode */
		0,		/* 0 launch mode */
		0,		/* 1 launch mode */
		1		/* ARGV style via wiscr to shel_write supported */
	},
	/* WF_COLOR and WF_DCOLOR are not completely supported. Especially not changing them.
	 * So the bits are off, although wind_get() will supply default values.
	 */
	/* These values are'nt bits, so I dont think this is correct
	 * WF_TOP + WF_NEWDESK + WF_OWNER + WF_BEVENT + WF_BOTTOM + WF_ICONIFY + WF_UNICONIFY */
	/* bit 9 WF_WHEEL */
	{			/*11 window functions ?*/
		01763,		/* see above */
		0,
		5,		/* window behaviours iconifier & click for bottoming */
		1		/* wind_update(): check and set available (mode + 0x100) */
	},
	/* WM_UNTOPPED + WM_ONTOP + AP_TERM + CH_EXIT (HR) + WM_BOTTOMED + WM_ICONIFY + WM_UNICONIFY */
	{			/*12 messages */
		0756,		/* see above */
		0,
		1,		/* WM_ICONIFY gives coordinates */
		0
	},
	{			/*13 objects */
		1,		/* 3D objects */
		1,		/* objc_sysvar */
		0,		/* GDOS fonts */
		014		/* extended objects (0x8 G_SHORTCUT, 0x4 WHITEBAK objects)
				                     0x2 G_POPUP,    0x1 G_SWBUTTON */
	},
	{			/*14 form support (form_xdo, form_xdial) */
		0,		/* MagiC flydials */
		0,		/*   "   keyboard tables */
		0,		/* return last cursor position */
		0
	},
	{			/*15 <-- 64 */
		0,		/* shel_write and AP_AESTERM */
		0,		/* shel_write and SHW_SHUTDOWN/SHW_RESCHANGE */
		3,		/* appl_search with long names */
		                /*   and additive mode APP_TASKINFO available. */
		0		/* form_error and all GEMDOS errorcodes */
	},
	{			/*16 <-- 65 */
		1,		/* appl_control exists. */
		APC_HIGH,	/* highest opcode for appl_control */
		0,		/* shel_help exists. */
		0		/* wind_draw exists. */
	}
};										


/*
 * appl_getinfo() handler
 */
unsigned long
XA_appl_getinfo(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	ushort gi_type = pb->intin[0];

	CONTROL(1,5,0)

	/* Extremely curious to who's asking what. */
	DIAG((D_appl, client, "appl_getinfo %d for %s\n", gi_type, c_owner(client)));

	if (gi_type > 14)
	{
		/* N.Aes extensions */
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

	info_tab[2][0] = Getrez();

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
XA_appl_find(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	struct nn { short m, id;};
	struct nn *ex = (struct nn *)pb->addrin;
	char *name = (char *)pb->addrin[0], *t;
	short f = ex->id, n;

	CONTROL(0,1,1)

	DIAG((D_appl, client, "appl_find for %s\n", c_owner(client)));

	/* mint id <--> aes_id */
	if (ex->m == -1 || ex->m == -2)
	{
		pb->intout[0] = f;

		DIAG((D_appl, client, "   Mode 0xfff? --> %d\n", pb->intout[0]));
	}
	else if (ex->m == 0)
	{
		/* Return the pid of current process */
		pb->intout[0] = client->pid;

		DIAG((D_appl, client, "   Mode 0x0000 --> %d\n", pb->intout[0]));
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
		DIAG((D_appl, client, "   ?AGI\n"));
	}
	else
	{
		XA_CLIENT *cl;
		DIAG((D_appl, client, "   '%s'\n", pb->addrin[0]));
		pb->intout[0] = -1;

		Sema_Up(clients);

		/* use proper loop */
		cl = S.client_list;
		while (cl)
		{
			/* AES is OK */
			if (cl->client_end || cl->pid == C.AESpid)
			{
				t = cl->proc_name;

				for (n = 0; (n < 8) && (toupper(name[n]) == toupper(t[n])); n++)
					;

				if (n == 8)
				{
					pb->intout[0] = cl->pid;
					DIAG((D_appl, client, "   --> %d\n", pb->intout[0]));
					break;
				}
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
XA_appl_control(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	XA_CLIENT *cl;
	int pid = pb->intin[0];
	enum apc_code f = pb->intin[1];

	CONTROL(2,1,1)

	DIAG((D_appl, client, "appl_control for %s\n", c_owner(client)));

	if (pid == -1)
		pid = focus_owner()->pid;

	cl = Pid2Client(pid);
	if (cl == NULL)
	{
		pb->intout[0] = 0;
	}
	else
	{
		pb->intout[0] = 1;

		DIAG((D_appl, client, "  --    on %s, func %d, 0x%lx\n",
			c_owner(cl), f, pb->addrin[0]));
	
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
							 cl->pid, 0, 0, 0);
			break;
			case APC_HIDENOT:
				hide_other(lock, cl);
			break;
			case APC_INFO:
			{
				short *ii = pb->addrin[0];
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

/*
 * Get the file handle of the client reply pipe
 */
unsigned long
XA_appl_pipe(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	CONTROL(0,1,0)

	pb->intout[0] = client ? client->client_end : 0;
	return XAC_DONE;
}
