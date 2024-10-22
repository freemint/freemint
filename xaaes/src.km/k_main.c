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
#define PROFILING	0


#define MX_STRAM
#include "version.h"
#include "xa_defs.h"
#include "k_main.h"
#undef MX_STRAM	/* sigh ...*/

/* add time to alerts in syslog */
#if ALERTTIME
#ifdef trap_14_w
#undef trap_14_w	/* "redefined" warning */
#endif
#include <mintbind.h>	/* Tgettimeofday */
#define MAXALERTLEN	196

#endif

#include "xa_global.h"

#include "xaaes.h"

#include "about.h"
#include "app_man.h"
#include "adiload.h"
#include "c_window.h"
#include "cnf_xaaes.h"
#include "xacookie.h"
#include "desktop.h"
#include "debug.h"
#include "handler.h"
#include "init.h"
#include "k_init.h"
#include "k_keybd.h"
#include "k_mouse.h"
#include "k_shutdown.h"
#include "nkcc.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"
#include "obtree.h"
#include "render_obj.h"
#include "taskman.h"
#include "trnfm.h"

#include "xa_appl.h"
#include "xa_evnt.h"
#include "xa_form.h"
#include "xa_graf.h"	//graf_mouse
#include "xa_rsrc.h"
#include "xa_shel.h"
#if WITH_BBL_HELP
#include "xa_bubble.h"
#endif

#include "mint/dcntl.h"
#include "mint/fcntl.h"
#include "mint/filedesc.h"
#include "mint/ioctl.h"
#include "mint/signal.h"
#include "mint/ssystem.h"
#include "cookie.h"

#if CHECK_STACK
short check_stack_alignment( long e );
extern short stack_align;
#endif

#include "c_mouse.h"
void set_tty_mode( short md );

/* ask before shutting down (does not work yet) */
#define ALERT_SHUTDOWN 0
#define AESSYS_TIMEOUT	3*2000	/* s/1000 */

void
ceExecfunc(int lock, struct c_event *ce, short cancel)
{
	if (!cancel)
	{
		if (ce->d1)
		{
			void _cdecl (*f)(int, struct xa_client *, short);
			if ((f = ce->ptr1))
				(*f)(lock, ce->client, ce->d0);
		}
		else
		{
			void (*f)(int, struct xa_client *, short);
			if ((f = ce->ptr1))
				(*f)(lock, ce->client, ce->d0);
		}
	}
}

void
cancel_cevents(struct xa_client *client)
{
	struct c_event *ce;

	while ((ce = client->cevnt_head))
	{
		struct c_event *nxt;

		DIAG((D_kern, client, "Cancel evnt %lx (next %lx) for %s", (unsigned long)ce, (unsigned long)ce->next, client->name));

		(*ce->funct)(0, ce, true);

		if (!(nxt = ce->next))
			client->cevnt_tail = nxt;
		client->cevnt_head = nxt;
		client->cevnt_count--;
		kfree(ce);
	}

	DIAG((D_kern, client, " --- cancelled events for %s! (count %d)", client->name, client->cevnt_count));

	if (C.ce_open_menu == client)
		C.ce_open_menu = NULL;
	if (C.ce_menu_click == client)
		C.ce_menu_click = NULL;
	if (C.ce_menu_move == client)
		C.ce_menu_move = NULL;
}
#if INCLUDE_UNUSED
bool
CE_exists(struct xa_client *client, void *f)
{
	struct c_event *ce = client->cevnt_head;

	while (ce)
	{
		if (ce->funct == f)
			return true;
		ce = ce->next;
	}
	return false;
}
#endif
/*
 * cancel_CE() - search for a client event with function callback == f.
 * If found, call the 'callback' with pointer to this client event along
 * with 'arg'. If the 'callback' return true, the found client event is
 * deleted.
 */
void
cancel_CE(struct xa_client *client,
	  void *f,
	  bool(*callback)(struct c_event *ce, long arg),
	  long arg)
{
	struct c_event *ce = client->cevnt_head, *p = NULL;

	DIAG((D_evnt, client, "cancel_CE: find function %lx in client events for %s", (unsigned long)f, client->name));

	while (ce)
	{
		bool removed = false;

		if (ce->funct == f)
		{
			DIAGS((" --- Found func==%lx, calling callback=%lx", (unsigned long)f, (unsigned long)callback));
			if (callback(ce, arg))
			{
				struct c_event *nce;
				DIAGS((" --- delete client event!"));

				if (p)
					p->next = ce->next;
				else
					client->cevnt_head = ce->next;

				if (!(nce = ce->next))
					client->cevnt_tail = p;

				client->cevnt_count--;
				DIAGS(("---------- freeing CE %lx with function %lx", (unsigned long)ce, (unsigned long)f));
				kfree(ce);
				ce = nce;
				removed = true;
			} else
			{
				DIAGS((" --- dont delete client event"));
			}
		}
		if (!removed)
		{
			p = ce;
			ce = ce->next;
		}
	}
}

void
post_cevent(struct xa_client *client,
	void (*func)(int, struct c_event *, short cancel),
	void *ptr1, void *ptr2,
	int d0, int d1, const GRECT *r,
	const struct moose_data *md)
{
	struct c_event *c;

	//if( !client || !client->p->pid )
		//return;
	if (!(client->status & CS_BLOCK_CE))
	{
		c = kmalloc(sizeof(*c));
		if (c)
		{
			c->next		= NULL;
			c->funct	= func;
			c->client	= client;
			c->ptr1		= ptr1;
			c->ptr2		= ptr2;
			c->d0		= d0;
			c->d1		= d1;

			if (r)
				c->r = *r;
			if (md)
				c->md = *md;

			if (!client->cevnt_head)
			{
				client->cevnt_head = c;
				client->cevnt_tail = c;
			}
			else
			{
				client->cevnt_tail->next = c;
				client->cevnt_tail = c;
			}
			client->cevnt_count++;

			DIAG((D_mouse, client, "added cevnt %lx(%d) (head %lx, tail %lx) for %s",
				(unsigned long)c, client->cevnt_count, (unsigned long)client->cevnt_head, (unsigned long)client->cevnt_tail,
				client->name));

		}
		else
		{
			DIAGS(("kmalloc(%ld) failed, out of memory?", (long)sizeof(*c)));
		}
		Unblock(client, 1);
	}
}

short
dispatch_selcevent(struct xa_client *client, void *f, bool cancel)
{
	struct c_event *ce = client->cevnt_head, *p = NULL;

	DIAG((D_evnt, client, "dispatch_selcevent: function %lx in client events for %s", (unsigned long)f, client->name));

	while (ce)
	{
		if (ce->funct == f)
		{
			struct c_event *nce;
			DIAGS((" --- Found func==%lx, dispatching...", (unsigned long)f));
			DIAGS((" --- delete client event!"));

			if (p)
				p->next = ce->next;
			else
				client->cevnt_head = ce->next;

			if (!(nce = ce->next))
				client->cevnt_tail = p;

			client->cevnt_count--;

			(*ce->funct)(0, ce, cancel);

			DIAGS(("---------- freeing CE %lx with function %lx", (unsigned long)ce, (unsigned long)f));
			kfree(ce);
			ce = nce;
			return (volatile short)client->cevnt_count;
		}
		p = ce;
		ce = ce->next;
	}
	return 0;
}

static short
dispatch_cevent(struct xa_client *client)
{
	struct c_event *ce;
	short ret = 0;

	ce = client->cevnt_head;
	if (ce)
	{
		struct c_event *nxt;

		DIAG((D_kern, client, "Dispatch evnt %lx (head %lx, tail %lx, count %d) for %s",
			(unsigned long)ce, (unsigned long)client->cevnt_head, (unsigned long)client->cevnt_tail, client->cevnt_count, client->name));


		if (!(nxt = ce->next))
			client->cevnt_tail = nxt;

		client->cevnt_head = nxt;
		client->cevnt_count--;

		(*ce->funct)(0, ce, false);
		kfree(ce);

//		ret = client->cevnt_count + 1;
		ret = (volatile short)client->cevnt_count;
	}
	return ret;
}

void
do_block(struct xa_client *client)
{
	/*if( client->status & 0x00010000L )
	{
		return;
	}*/
#if 0
	if ((client->i_xevmask.ev_0 & XMU_FSELECT))
	{
		client->fselect.inuse = true;
		client->blocktype = XABT_SELECT;
		client->sleepqueue = SELECT_Q;
		client->fselect.ret = f_select(0L, (long *)&client->fselect.rfds, (long *)&client->fselect.wfds, (long *)&client->fselect.xfds);
	}
	else
#endif
	{
		client->blocktype = XABT_SLEEP;
		client->sleepqueue = IO_Q;
		client->sleeplock = client == C.Aes ? (long)client->tp : (long)client;
		sleep(IO_Q, client->sleeplock);
	}
	client->blocktype = XABT_NONE;
	client->sleeplock = 0;
}

#if ALT_CTRL_APP_OPS
void
Block(struct xa_client *client)
{
	(*client->block)(client);
}
#endif
void
cBlock(struct xa_client *client)
{
	while (!client->usr_evnt && (client->irdrw_msg || client->cevnt_count))
	{
		if (client->irdrw_msg)
		{
			exec_iredraw_queue(0, client);
		}

		dispatch_cevent(client);
	}

	if (client->usr_evnt)
	{
		if (client->usr_evnt & 1)
		{
			cancel_evnt_multi(client);
			cancel_mutimeout(client);
		}
		else
			client->usr_evnt = 0;
		return;
	}
	/*
	 * Now check if there are any events to deliver to the
	 * application...
	 */
	if (check_queued_events(client))
	{
		cancel_mutimeout(client);
		return;
	}

	/*
	 * Getting here if no more client events are in the queue
	 * Looping around doing client events until a user event
	 * happens.. that is, we've got something to pass back to
	 * the application.
	 */
	while (!client->usr_evnt)
	{
		DIAG((D_kern, client, "Blocked %s", c_owner(client)));

		do_block(client);

		/*
		 * Ozk: This is gonna be the new style of delivering events;
		 * If the receiver of an event is not the same as the sender of
		 * of the event, the event is queued (only for AES messges for now)
		 * Then the sender will wake up the receiver, which will call
		 * check_queued_events() and handle the event.
		*/
		while (!client->usr_evnt && (client->irdrw_msg || client->cevnt_count))
		{
			if (client->irdrw_msg)
				exec_iredraw_queue(0, client);

			dispatch_cevent(client);
		}

		if (client->usr_evnt)
		{
			if (client->usr_evnt & 1)
			{
				cancel_evnt_multi(client);
				cancel_mutimeout(client);
			}
			else
				client->usr_evnt = 0;
			return;
		}

		if (check_queued_events(client))
		{
			cancel_mutimeout(client);
			return;
		}
	}
	if (client->usr_evnt & 1)
	{
		cancel_evnt_multi(client);
		cancel_mutimeout(client);
	}
	else
		client->usr_evnt = 0;
}

static void
iBlock(struct xa_client *client)
{
	XAESPB *a = C.Hlp_pb;

	client->usr_evnt = 0;

	while (!client->usr_evnt && (client->irdrw_msg || client->cevnt_count))
	{
		if (client->irdrw_msg)
			exec_iredraw_queue(0, client);

		dispatch_cevent(client);
	}

	if (client->usr_evnt)
	{
		if (client->usr_evnt & 1)
		{
			cancel_evnt_multi(client);
			cancel_mutimeout(client);
		}
		else
			client->usr_evnt = 0;
		return;
	}

	/*
	 * Now check if there are any events to deliver to the
	 * application...
	 */
	if (check_queued_events(client))
	{
		if (a->intout[0] & MU_MESAG)
		{
			CHlp_aesmsg(client);
		}
		a->addrin[0] = (long)a->msg;
		client->usr_evnt = 0;
		client->waiting_for = XAWAIT_MULTI|MU_MESAG;
		client->waiting_pb = C.Hlp_pb;
	}

	/*
	 * Getting here if no more client events are in the queue
	 * Looping around doing client events until a user event
	 * happens.. that is, we've got something to pass back to
	 * the application.
	 */
	while (!client->usr_evnt)
	{
		DIAG((D_kern, client, "Blocked %s", c_owner(client)));

		if (client->tp_term)
		{
			return;
		}

		do_block(client);

		/*
		 * Ozk: This is gonna be the new style of delivering events;
		 * If the receiver of an event is not the same as the sender of
		 * of the event, the event is queued (only for AES messges for now)
		 * Then the sender will wake up the receiver, which will call
		 * check_queued_events() and handle the event.
		*/
		while (!client->usr_evnt && (client->irdrw_msg || client->cevnt_count))
		{
			if (client->irdrw_msg)
				exec_iredraw_queue(0, client);

			dispatch_cevent(client);
		}

		if (client->usr_evnt)
		{
			if (client->usr_evnt & 1)
			{
				cancel_evnt_multi(client);
				cancel_mutimeout(client);
			}
			else
				client->usr_evnt = 0;
			return;
		}

		if (check_queued_events(client))
		{
			if (a->intout[0] & MU_MESAG)
			{
				CHlp_aesmsg(client);
			}
			a->addrin[0] = (long)a->msg;
			client->usr_evnt = 0;
			client->waiting_for = XAWAIT_MULTI|MU_MESAG;
			client->waiting_pb = C.Hlp_pb;
		}
	}
	if (client->usr_evnt & 1)
	{
		cancel_evnt_multi(client);
		cancel_mutimeout(client);
	}
	else
		client->usr_evnt = 0;
}

void
Unblock(struct xa_client *client, unsigned long value)
{
	/* the following served as a excellent safeguard on the
	 * internal consistency of the event handling mechanisms.
	 */
	if (client == C.Aes)
		wake(IO_Q, client->sleeplock);
	else
	{
		if (value == XA_OK)
			cancel_evnt_multi(client);

		if (client->blocktype == XABT_SELECT)
			wakeselect(client->p);
		else if (client->blocktype == XABT_SLEEP)
			wake(IO_Q, client->sleeplock); //(long)client);
	}

	DIAG((D_kern, client,"Unblocked %s 0x%lx", client->proc_name, value));
}

static void *svmotv = NULL;
static void *svbutv = NULL;
static void *svwhlv = NULL;

/* shortened NVDI structur */
struct _nvdi_struct
{
	short version;
	unsigned long date;
	short conf;
};


static short disable_nvdi_errors(void)
{
	unsigned long cookie;
	struct _nvdi_struct	*nvdi_struct;
	short ret;
	
	if (get_toscookie(COOKIE_NVDI, &cookie) == 0 && cookie != 0)
	{
		nvdi_struct = (struct _nvdi_struct *)cookie;
		ret = nvdi_struct->conf;
		nvdi_struct->conf |= 0x0002;  /* turn error compatibility on */
		nvdi_struct->conf &= ~0x0040; /* turn alerts off */
		return ret;
	}
	return 0;
}


static void enable_nvdi_errors(short conf)
{
	unsigned long cookie;
	struct _nvdi_struct	*nvdi_struct;
	
	if (get_toscookie(COOKIE_NVDI, &cookie) == 0 && cookie != 0)
	{
		nvdi_struct = (struct _nvdi_struct *)cookie;
		nvdi_struct->conf = conf;
	}
}



/*
 * initialise the mouse device
 */
#define MIN_MOOSE_VER_MAJOR 0
#define MIN_MOOSE_VER_MINOR 10

static bool
init_moose(void)
{
	bool ret = false;

	C.button_waiter = NULL;
	C.redraws = 0;
	C.move_block = 0;
	C.rect_lock = 0;

	//G.adi_mouse = adi_name2adi("moose_w");
	//if (!G.adi_mouse)
		G.adi_mouse = adi_name2adi("moose");

	if (G.adi_mouse)
	{
		long aerr;

		aerr = adi_open(G.adi_mouse);
		if (!aerr)
		{
			long moose_version;
			struct moose_vecsbuf vecs;

			aerr = adi_ioctl(G.adi_mouse, FS_INFO, (long)&moose_version);
			if (!aerr)
			{
				if (moose_version < (((long)MIN_MOOSE_VER_MAJOR << 16) | MIN_MOOSE_VER_MINOR))
				{
					display(/*0000000b*/"init_moose: Your moose.adi is outdated, please update!");
					return false;
				}
			}
			else
			{
				display(/*0000000c*/"init_moose: Could not obtain moose.adi version, please update!");
				return false;
			}

			aerr = adi_ioctl(G.adi_mouse, MOOSE_READVECS, (long)&vecs);
			if (aerr == 0 && vecs.motv)
			{
				vex_motv(C.P_handle, vecs.motv, &svmotv);
				vex_butv(C.P_handle, vecs.butv, &svbutv);

				if (vecs.whlv)
				{
					short conf;
					
					/*
					 * NVDI does not have that call, and may report an error
					 */
					conf = disable_nvdi_errors();
					vex_wheelv(C.P_handle, vecs.whlv, &svwhlv);
					enable_nvdi_errors(conf);
					BLOG((false, "Wheel support present"));
				}
				else
				{
					BLOG((false, "No wheel support"));
				}

				if (adi_ioctl(G.adi_mouse, MOOSE_DCLICK, (long)cfg.double_click_time))
					display(/*0000000d*/"Moose set dclick time failed");

				if (adi_ioctl(G.adi_mouse, MOOSE_PKT_TIMEGAP, (long)cfg.mouse_packet_timegap))
					display(/*0000000e*/"Moose set mouse-packets time-gap failed");

				BLOG((false, "Using moose adi"));
				ret = true;
			}
			else
				display(/*0000000f*/"init_moose: MOOSE_READVECS failed (%lx)", aerr);
		}
		else
		{
			display(/*00000010*/"init_moose: opening moose adi failed (%lx)", aerr);
			G.adi_mouse = NULL;
		}
	}
	else
	{
		display(/*00000011*/"Could not find moose.adi, please install in %s!", C.Aes->home_path);
	}

	return ret;
}


/*
 * Ozk: multi_intout() may be called by processes not yet called
 * appl_init(). So, it must not depend on client being valid!
 */
void
multi_intout(struct xa_client *client, short *o, int evnt)
{
	check_mouse(client, &o[3], &o[1], &o[2]);
	o[0] = evnt;
	vq_key_s(C.P_handle, &o[4]);
	if (evnt)
	{
		o[5] = o[6] = 0;
	}
}
struct display_alert_data
{
	int lock;
	short len;
	char buf[0];
};

static void display_alert(struct proc *p, long arg);

static unsigned short alert_masks[] =
{
	0x0001, 0x0002, 0x0004, 0x0008,
	0x0010, 0x0020, 0x0040, 0x0080
};

/*
 * replace all occurences of in by out in s
 * replace string of rep1 by one rep1
 */
static char *strrpl( char *s, char in, char out, char rep1 )
{
	char *ret = s, *t = s;
	bool out_rep1 = out == rep1;
	for( ; *s; s++ )
	{
		if( rep1 && *s == rep1 )
		{
			*t++ = rep1;
			for( s++; *s == rep1 || (out_rep1 && *s == in); s++ );
		}
		if( *s == in )
			*t++ = out;
		else
			*t++ = *s;
	}
	*t = 0;

	return ret;
}

static void
CE_fa(int lock, struct c_event *ce, short cancel)
{
	if (!cancel)
	{
		struct display_alert_data *data = ce->ptr1;

		/* make sure the evil process really goes away */
		long pid = -1;
		char *ps = strstr( data->buf, "(PID ");
		if( !ps )
			ps = strstr( data->buf, "pid ");

		yield();
		if( ps )
			pid = atol( ps + 4 );

		if( pid >= 0 && strstr( data->buf, "KILLED:" ) )
		{
			short s;
			long r;
			if( pid == C.Aes->p->pid || pid == C.Hlp->p->pid || pid == C.Aes->tp->pid )
				return;
			for( s = 0; !(r=ikill(pid, 0)) && s < 666; s++ )
			{
				ikill(pid, SIGKILL);
				nap(20);
			}
			nap(2000);
		}
		/***********************************************/

		if (C.update_lock)
		{
			struct timeout *t;
			t = addroottimeout(100, display_alert, 0);
			if (t)
				t->arg = (long)data;
		}
		else if( data->len )
		{
			struct xa_client *client = ce->client;
			struct helpthread_data *htd = lookup_xa_data_byname(&client->xa_data, HTDNAME);
			struct xa_window *wind = NULL;
			int c;
#if ALERTTIME
			char b[MAXALERTLEN];	// to be output by form_alert
#endif
			unsigned short amask;
			struct widget_tree *wt;
			OBJECT *form=0, *icon=0;		/* gcc4 isnt that clever? */

			if (!htd || !htd->w_sysalrt)
			{
				open_systemalerts(0, client, false);
			}

			if (htd)
				wind = htd->w_sysalrt;

			c = data->buf[1] - '0';

			if (wind)
			{
				wt = get_widget(wind, XAW_TOOLBAR)->stuff.wt;
				form = wt->tree;
			}
#if SALERT_IC4 != SALERT_IC3+1 || SALERT_IC3 != SALERT_IC2+1 ||SALERT_IC2 != SALERT_IC1+1
#error "false xaaes.h: SALERT_IC? not consecutive"
#endif
				if( c >= 1 && c <= 4 )
				{
					if( wind )
						icon = form + SALERT_IC1 + c - 1;	/* numbers for SALERT_IC? are consecutive */
					amask = alert_masks[c];
				}
				else
				{
					//icon = NULL;
					amask = alert_masks[0];
				}
			if (wind)
			{
				strncpy( b, data->buf, sizeof(b)-1 );
				b[sizeof(b)-1] = 0;

				/* Add the log entry */
				{
					struct scroll_info *list = object_get_slist(form + SYSALERT_LIST);
					struct sesetget_params p = { 0 };
					struct scroll_content sc = {{ 0 }};

#if ALERTTIME
					union udostim
					{
						long l;
						/* from libkern/unix2xbios.c */
						struct dostim
						{
							unsigned year: 7;
							unsigned month: 4;
							unsigned day: 5;
							unsigned hour: 5;
							unsigned minute: 6;
							unsigned sec2: 5;
						}t;
					};
					struct timeval tv;
					struct timezone tz;
					union udostim dtim;
					extern struct xa_wtxt_inf norm_txt;	/* from taskman.c */
					long len;

					Tgettimeofday( &tv, &tz );
					dtim.l = unix2xbios( tv.tv_sec );
					sprintf( data->buf, data->len, "%02d:%02d:%02d: %s", dtim.t.hour, dtim.t.minute, dtim.t.sec2, b + 4 );
					strrpl( data->buf, '|', ' ', ' ' );
					len = strlen(data->buf);
					if( len > 7 )
						data->buf[len-7] = 0;	/* strip off [ OK ] */
					BLOG((0, data->buf));
#endif
					sc.t.text = data->buf;
					sc.icon = icon;
					sc.t.strings = 1;
					sc.fnt = &norm_txt;
					p.idx = -1;
					p.arg.txt = xa_strings(RS_ALERTS);
					list->get(list, NULL, SEGET_ENTRYBYTEXT, &p);
					if( p.e )
					{
						list->add(list, p.e, NULL, &sc, SEADD_CHILD, 0, true);
						if( !(p.e->xstate & OS_OPENED) )
							list->cur = p.e;	// don't set cur to leaf if not opened (->add_scroll_entry?)
					}
				}
			}

			if ( C.shutdown == 0 && (cfg.alert_winds & amask))
			{

#if ALERTTIME
				/* really bad hack: apps often crash when accessing /host/clipbrd under aranym
				 * and this should prevent from debugging XaAES
				 * (see: http://sourceforge.net/tracker/?func=detail&aid=3038473&group_id=41106&atid=429796)
				 */
				char *p;
				if( (p = strstr( b, "AF000000" )) )
				{
					memcpy( p, "CLIPBRD ", 8 );
				}
				/* if an app left the mouse off */
				forcem();
				do_form_alert(data->lock, client, 1, b, "XaAES");
#else
				/* if an app left the mouse off */
				forcem();
				do_form_alert(data->lock, client, 1, data->buf, "XaAES");
#endif
			}

			kfree(data);
		}
	}
	else
	{
		kfree(ce->ptr1);
	}
}

static void
display_alert(struct proc *p, long arg)
{
	if (C.update_lock)
	{
		/* we need to delay */
		struct timeout *t;

		t = addroottimeout(100, display_alert, 0);
		if (t)
			t->arg = arg;
	}
	else
	{
		struct display_alert_data *data = (struct display_alert_data *)arg;

		/* Bring up an alert */
		post_cevent(C.Hlp, CE_fa, data,NULL, 0,0, NULL,NULL);
	}
}

typedef void XA_CONF (void *p);

void show_bubble( void *str );
static XA_CONF *xa_config[] = {load_grd, show_bubble, load_config};
#if ALERTTIME
#define ALERTPL	10
#else
#define ALERTPL	0
#endif
#define MAX_ALERTLEN 512
static void
alert_input(int lock)
{
	/* System alert? Alert and add it to the log */
	long n;

	n = f_instat(C.alert_pipe);

	if( n > MAX_ALERTLEN)
		n = MAX_ALERTLEN;
	if (n > 0)
	{
		struct display_alert_data *data;

		data = kmalloc(sizeof(*data) + n + ALERTPL + 4 );

		if (!data)
		{
			DIAGS(("kmalloc(%ld) failed, out of memory?", (long)sizeof(*data)));
			return;
		}
		data->lock = lock;
		data->len = n + ALERTPL;
		f_read(C.alert_pipe, n, data->buf);
		data->buf[n] = '\0';
		if( ferr )
			return;

		if( data->buf[0] == '#' && data->buf[1] == '$' )
		{
			if( data->buf[2] == '#' && data->buf[3] >= '0' && data->buf[3] - '0' < sizeof(xa_config) / sizeof(void*) )
				xa_config[(int)data->buf[3]-'0']( (void*)(data->buf + 4) );
			kfree(data);
			return;
		}
		if (C.Hlp && n < MAX_ALERTLEN && *data->buf == '[')
		{
			post_cevent(C.Hlp, CE_fa, data, NULL, 0, 0, NULL,NULL);
		}
		else
			kfree(data);
	}
	else
	{
		DIAGS(("No bytes in alert_pipe"));
	}
}

static void k_exit(int);
static void restore_sigs(void);
static void setup_common(void);
int ferr = 0;	/* global to indicate serious problems */
static void print_context(int sig)
{
#ifdef BOOTLOG
	struct proc *p = get_curproc();

	BLOG((true,"exception %d for %s pc:%lx addr:%lx", sig, p->name, p->exception_pc, p->exception_addr));
#endif
}

/*
 * signal handlers
 */
static void
ignore(int sig)
{
	DIAGS(("AESSYS: ignored signal"));
	print_context(sig);
	BLOG((0, "'%s': received signal: %d(ignored)", get_curproc()->name, sig));
	KERNEL_DEBUG("AESSYS: ignored signal %d", sig);
	if( sig == SIGINT )
		recover();
}

#if !GENERATE_DIAGS
static void
fatal(int sig)
{
	struct proc *p = get_curproc();
	print_context(sig);
	KERNEL_DEBUG("'%s': fatal error, trying to clean up", p->name );
	ferr = sig;
	S.clients_exiting = 0;
	C.shutdown |= EXIT_MAINLOOP | KILLEM_ALL;
	dispatch_shutdown( RESTART_XAAES);
}
#endif

extern char XAAESNAME[];

static void
sigterm(void)
{
#ifdef BOOTLOG
	struct proc *p = get_curproc();
	BLOG((true, "%s(%d:AES:%d): sigterm received", p->name, p->pid, C.AESpid ));
#endif
#if 1
	BLOG((false, "(ignored)" ));
	return;
#else
	if( p->pid != C.AESpid ){
		BLOG((false, "(ignored)" ));
		return;

	}
	BLOG((false, "dispatch_shutdown(0)" ));
	/*KERNEL_DEBUG("AESSYS: sigterm received, dispatch_shutdown(0)");*/
	dispatch_shutdown(0);
#endif
}

static void
sigchld(void)
{
	long r;

	while ((r = p_waitpid(-1, 1, NULL)) > 0)
	{
		DIAGS(("sigchld -> %li (pid %li)", r, ((r & 0xffff0000L) >> 16)));
	}
}

static const char alert_pipe_name[] = "u:\\pipe\\alert";
static const char KBD_dev_name[] = "u:\\dev\\console";
static struct sgttyb KBD_dev_sg;

int aessys_timeout = 0;

static const char aesthread_name[] = "aesthred";
static const char aeshlp_name[] = "XaSYS";

static void
aesthread_block(struct xa_client *client)
{
	while ((client->irdrw_msg || client->cevnt_count))
	{
		if (client->irdrw_msg)
			exec_iredraw_queue(0, client);

		dispatch_cevent(client);
	}

	if (client->status & (CS_LAGGING | CS_MISS_RDRW))
	{
		client->status &= ~(CS_LAGGING|CS_MISS_RDRW);
	}

	if (!client->tp_term)
		do_block(client);
}

/*
 * AES thread
 */
static void
aesthread_entry(void *c)
{
	struct xa_client *client = c;

	p_domain(1);
	setup_common();

	for (;;)
	{
		aesthread_block(client);
		if (client->tp_term)
			break;
	}
	client->tp = NULL;
	client->tp_term = 2;
	kthread_exit(0);
}

static void
helpthread_entry(void *c)
{
	struct xa_client *client;

#if CHECK_STACK
	long stk = (long)get_sp();
	stack_align |= (check_stack_alignment(stk) << 8);
#endif
	p_domain(1);
	setup_common();

	client = init_client(0, true);

	if (client)
	{
		struct helpthread_data *htd;
		XAESPB *pb;
		short *d;
		long pbsize = sizeof(*pb) + 4 + ((12 + 32 + 32 + 32 + 32 + 32 + 32) * 2);

		if ((pb = kmalloc(pbsize)))
		{
			volatile short *t = &client->tp_term;
//			union msg_buf *msgb;

			bzero(pb, pbsize);

			d = (short *)((long)pb + sizeof(*pb));
			d += 2;
			pb->control = d;
			d += 12;
			pb->global = d;
			d += 32;
			pb->intin = (short *)d;
			d += 32;
			pb->intout = d;
			d += 32;
			pb->addrin = (long *)d;
			d += 32;
			pb->addrout = (long *)d;

			d += 32;
			pb->msg = (union msg_buf *)d;

			client_nicename(client, aeshlp_name, true);
			C.Hlp = client;
			client->type = APP_AESTHREAD;
			C.Hlp_pb = pb;
			client->waiting_pb = (AESPB *)pb;
			client->waiting_for = MU_MESAG;
			client->block = iBlock;
			client->options.app_opts |= XAAO_OBJC_EDIT;
			client->status |= CS_NO_SCRNLOCK;
			init_helpthread(NOLOCKING, client);
			while (!*t)
			{
				pb->addrin[0] = (long)pb->msg;
				client->waiting_pb = (AESPB *)pb;
				client->waiting_for = MU_MESAG|XAWAIT_MULTI;
//				BLOG((true, "enter block %lx", client->waiting_pb->addrin[0]));
				(*client->block)(client);
				/*if (*t)
				{
					break;
				}*/
			}
		}

		htd = lookup_xa_data_byname(&client->xa_data, HTDNAME);
		if (htd)
			delete_xa_data(&client->xa_data, htd);

		while (dispatch_cevent(client))
			;

		exit_client(0, client, 0, false, false);

		detach_extension(NULL, XAAES_MAGIC);
	}
	if (C.Hlp_pb)
	{
		kfree(C.Hlp_pb);
		C.Hlp_pb = NULL;
	}
	C.Hlp = NULL;
	kthread_exit(0);
}

static void
CE_at_restoresigs(int lock, struct c_event *ce, short cancel)
{
	if (!cancel)
	{
		restore_sigs();
	}
}

#define C_nAES 0x6E414553L     /* N.AES, the AES for MiNT */
static N_AESINFO *c_naes = NULL;
static N_AESINFO naes_cookie =
{
	0x0300,				/* version 3.0.0 */
	(25<<9)|(1<<5)|1,		/* 1 jan, 2005	*/
	(1<<11)|(2<<5)|3,		/* 01:02:03	*/
	0x8000,				/* Bit 15 set indicates that this really is XaAES */
	0L,
	0L,
};


#define SD_TIMEOUT	1000	// s/100

static void
sshutdown_timeout(struct proc *p, long arg)
{
	struct helpthread_data *htd;

	C.sdt = NULL;
#if ALERT_SHUTDOWN
	if ( xaaes_do_form_alert( 0, 1, xa_strings(ASK_SHUTDOWN_ALERT), XAAESNAME ) != 2 )
		return;
#endif

	htd = get_helpthread_data(C.Aes);

	if (/*C.update_lock ||*/ S.clients_exiting)
	{
		/* we need to delay */
		struct timeout *t;

		t = set_shutdown_timeout(100);
		if (t)
			t->arg = arg;
	}
	else
	{
		struct xa_client *client;

		if (!(C.shutdown & SHUTTING_DOWN))
		{
			/*
			 * Step one: send all running apps AP_TERM or AC_CLOSE
			 *           and brutally kill clients listed in the
			 *           kill-without-question.
			 */
			C.shutdown_step = 0;
			C.shutdown |= SHUTTING_DOWN;

			FOREACH_CLIENT(client)
			{
				if (!(client->status & CS_EXITING) && isin_namelist(cfg.kwq, client->proc_name, 8, NULL, NULL))
				{
					client->status |= CS_SIGKILLED;
					ikill(client->p->pid, SIGKILL);
				}
			}

			quit_all_apps(NOLOCKING, (struct xa_client*)-1, (C.shutdown & RESOLUTION_CHANGE) ? AP_RESCHG : AP_TERM);
			set_shutdown_timeout(SD_TIMEOUT);
		}
		else
		{
			struct xa_client *flag = NULL;

			if (C.shutdown_step == 0)
			{
				/*
				 * Step two: Brutally kill clients whose name is found in the
				 *           kill-without-question list. Unblock other clients
				 *           incase they were blocked somewhere.
				 *           Then give apps 10 seconds to shutdown.
				 */
				FOREACH_CLIENT(client)
				{
					if (client != C.Aes && client != C.Hlp)
					{
						if (!(client->status & (CS_EXITING|CS_SIGKILLED)) && isin_namelist(cfg.kwq, client->proc_name, 8, NULL,NULL))
						{
							client->status |= CS_SIGKILLED;
							ikill(client->p->pid, SIGKILL);
						}
						else
						{
							flag = client;
							Unblock(client, 1);
						}
					}
				}
				if (flag)
				{
					C.shutdown_step = 1;
					set_shutdown_timeout(SD_TIMEOUT); //(4000);
				}
			}
			else
			{
				/*
				 * Step 3: Here we brutally kill any clients whose name is listed in
				 *         the kill-without-question list, and ask the user what to
				 *         do with other clients that havent termianted yet.
				 */
#if 1
				if (htd && htd->w_taskman)
				{
					force_window_top( 0, htd->w_taskman);
				}
#endif
				FOREACH_CLIENT(client)
				{
					if (!(client->status & (CS_EXITING|CS_SIGKILLED)) && client != C.Aes && client != C.Hlp)
					{
						if ((C.shutdown & KILLEM_ALL) || isin_namelist(cfg.kwq, client->proc_name, 8, NULL, NULL))
							ikill(client->p->pid, SIGKILL);
						else
						{
							if (!(flag = get_update_locker()))
								flag = client;
							break;
						}
					}
				}
				if (flag && !ikill(flag->p->pid, 0) )
				{
					post_cevent(C.Hlp, CE_open_csr, flag, NULL, 0,0, NULL,NULL);
				}
			}
			if (!flag)
			{
				C.shutdown |= EXIT_MAINLOOP;
				wakeselect(C.Aes->p);
			}
		}
	}
}

struct timeout *
set_shutdown_timeout(long delta)
{
	if (C.sdt)
	{
		cancelroottimeout(C.sdt);
		C.sdt = NULL;
	}
	if ((C.shutdown & (EXIT_MAINLOOP|SHUTDOWN_STARTED)) == SHUTDOWN_STARTED)
		C.sdt = addroottimeout(delta, sshutdown_timeout, 1);
	return C.sdt;
}

/*
 * Called when a client is terminated to kicks shutdown if the client
 * was the last one running
 */
void
kick_shutdn_if_last_client(void)
{
	if ((C.shutdown & (SHUTDOWN_STARTED|EXIT_MAINLOOP)) == SHUTDOWN_STARTED)
	{
		struct xa_client *c, *f = NULL;
		FOREACH_CLIENT(c)
		{
			if (c == C.Aes || c == C.Hlp)
				continue;
			else
			{
				f = c;
				break;
			}
		}
		if (!f) /* If last client, complete shutdown now! */
			set_shutdown_timeout(0);
		else /* Else reset the timeout giving the next app new chances */
			set_shutdown_timeout(SD_TIMEOUT);
	}
}

static int in_ce_dispatch_shutdown = 0;
void _cdecl
ce_dispatch_shutdown(int lock, struct xa_client *client, short b)
{
	short r = 0, def = 1;
	char *s;
	if( in_ce_dispatch_shutdown )	// no clue why this happens ..
	{
		return;
	}
	in_ce_dispatch_shutdown = 1;
	switch( b & (RESTART_XAAES | HALT_SYSTEM | RESTART_AFTER_BOMB) )
	{
	case RESTART_XAAES:
		s = xa_strings(ASK_RESTART_ALERT);
	break;
	case RESTART_AFTER_BOMB:
		s = xa_strings(HAD_A_PROBLEM);
		def = 2;
	break;
	case HALT_SYSTEM:
		s = xa_strings(ASK_SHUTDOWN_ALERT);
	break;
	default:
		s = xa_strings(ASK_QUIT_ALERT);
	}
	if( !(b & RESOLUTION_CHANGE) )
		next_res = 0;

	r = xaaes_do_form_alert( lock, C.Hlp, def, s);
	if( r == 2 )
	{
		if( b & RESTART_AFTER_BOMB )
		{
			b |= HALT_SYSTEM;
		}
		S.clients_exiting = 0;
		dispatch_shutdown(b);
	}
	in_ce_dispatch_shutdown = 0;
}

/*
 * Initiate shutdown...
 */
void _cdecl
dispatch_shutdown(short flags)
{
	if ( !(C.shutdown & SHUTDOWN_STARTED))
	{
		C.shutdown = SHUTDOWN_STARTED | flags;
		set_shutdown_timeout(0);
	}
}
static void
CE_start_apps(int lock, struct c_event *ce, short cancel)
{
	if (!cancel)
	{
		Path parms;
		int i;
		lock = LOCK_WINLIST|LOCK_ENVSTR|LOCK_PENDING;

		/*
		 * Load Accessories
		 */
		BLOG((false, "loading accs ---------------------------"));
		load_accs();

		/*
		 * startup shell and autorun
		 */
		BLOG((false, "loading shell and autorun ---------------"));

		for (i = sizeof(cfg.cnf_run)/sizeof(cfg.cnf_run[0]) - 1; i >= 0; i--)
		{
			if (cfg.cnf_run[i])
			{
				int pid;
				BLOG((false, "autorun[%d]: cmd=(%lx) '%s'", i, (unsigned long)cfg.cnf_run[i], cfg.cnf_run[i] ? cfg.cnf_run[i] : "no cmd"));
				BLOG((false, "   args[%d]:    =(%lx) '%s'", i, (unsigned long)cfg.cnf_run_arg[i], cfg.cnf_run_arg[i] ? cfg.cnf_run_arg[i] : "no arg"));
				parms[0] = '\0';
				if (cfg.cnf_run_arg[i])
					parms[0] = sprintf(parms+1, sizeof(parms)-2, "%s", cfg.cnf_run_arg[i]);

				pid = launch(lock, 0, 0, 0, cfg.cnf_run[i], parms, C.Aes);
				if( pid > 0 )
				{
					struct proc *p = pid2proc(pid);
					if( !strcmp( cfg.focus, p->name) )
						C.boot_focus = p;
				}
			}
		}
		if (cfg.cnf_shell)
		{
			parms[0] = '\0';
			if (cfg.cnf_shell_arg)
				parms[0] = sprintf(parms+1, sizeof(parms)-1, "%s", cfg.cnf_shell_arg);

			BLOG((false, "Launch shell '%s' with args '%s'", cfg.cnf_shell, parms[0] ? parms : "No args"));
			C.DSKpid = launch(lock, 0, 0, 0, cfg.cnf_shell, parms, C.Aes);
			if (C.DSKpid > 0)
			{
				strcpy(C.desk, cfg.cnf_shell);
				if( !strcmp( cfg.focus, "shell" ) )
					C.boot_focus = pid2proc(C.DSKpid);
			}
		}
	}
}

	/* forcing to be a pty master
	 * - pty master ignore job control
	 * - pty master always read in raw mode
	 *
	 * XXX it's just very ugly todo this so
	 */
//	get_curproc()->p_fd->ofiles[C.KBD_dev]->flags |= O_HEAD;

	/* next try
	 * switching tty device into RAW mode
	 */
void set_tty_mode( short md )
{
#if 0
	return;
#else
	struct sgttyb sg;
	long r;
	r = f_cntl(C.KBD_dev, (long)&KBD_dev_sg, TIOCGETP);
	//KERNEL_DEBUG("fcntl(TIOCGETP) -> %li", r);
	assert(r == 0);

	sg = KBD_dev_sg;
	sg.sg_flags &= TF_FLAGS;

	sg.sg_flags |= T_CRMOD; /* Avoid stairs effect on Console */

	if( md == RAW )
		sg.sg_flags |= T_RAW;
	else
		sg.sg_flags &= ~T_RAW;

	//KERNEL_DEBUG("sg.sg_flags 0x%x", sg.sg_flags);

	r = f_cntl(C.KBD_dev, (long)&sg, TIOCSETN);
	//KERNEL_DEBUG("fcntl(TIOCSETN) -> %li", r);
	assert(r == 0);
	get_curproc()->p_fd->ofiles[C.KBD_dev]->flags |= O_HEAD;
#endif
}

void show_bubble( void *str )
{
#if WITH_BBL_HELP
	union msg_buf msg;
	short br, xr, yr;
	check_mouse( C.Aes, &br, &xr, &yr );
	msg.m[0] = BUBBLEGEM_SHOW;
	msg.m[1] = C.AESpid;
	msg.m[2] = 0;
	msg.m[3] = xr;
	msg.m[4] = yr;
	msg.sb.p56 = str;
	msg.m[7] = 0;
	xa_bubble( 0, bbl_process_event, &msg, C.AESpid );
#endif
}

void load_grd( void *fn )
{
#if WITH_GRADIENTS
	load_gradients( 0, fn );
	update_windows_below(0, &screen.r, NULL, window_list, NULL);
	redraw_menu(0);
#endif
}

void load_bkg_img( void *fn )
{
#if WITH_BKG_IMG
	do_bkg_img(C.Aes, 3, fn);
	update_windows_below(0, &screen.r, NULL, window_list, NULL);
#endif
}
void load_palette( void *fn )
{
	if( !rw_syspalette( READ, screen.palette, 0, fn ) )
	{
		set_syspalette(C.Aes->vdi_settings->handle, screen.palette);
		update_windows_below(0, &screen.r, NULL, window_list, NULL);
	}
}

static void set_boot_focus(int lock)
{
	if( C.boot_focus )
	{
		struct xa_client *c = is_aes_client( C.boot_focus );
		C.boot_focus = 0;

		if( c )
		{
			set_active_client(lock, c);
		}
	}
}

/*
 * Main AESSYS thread...
 */
/*
 * our XaAES server kernel thread
 *
 * It have it's own context and can use all syscalls like a normal
 * process (except that it don't go through the syscall handler).
 *
 * It run in kernel mode and share the kernel memspace, cwd, files
 * and sigs with all other kernel threads (mainly the idle thread
 * alias rootproc).
 */
void
k_main(void *dummy)
{
	int wait = 1, pferr, p_exc = -1;
	unsigned long default_input_channels;
	struct tty *tty;
	struct file *fp = 0, *fo = 0 /* for gcc */;
	struct proc *p = get_curproc();
	uchar user_stack[100]; /* Stack to call the AES from supervisor mode */
#if CHECK_STACK
	long stk;
#endif

	/* The TOS AES saves the registers on USP inside trap #2 (56 bytes)
	 * This hack is necessary to call the AES from supervisor mode.
	 */
	set_usp(user_stack + sizeof user_stack);
#if CHECK_STACK
	stk = (long)get_sp();
	stack_align |= (check_stack_alignment(stk) << 4);
#endif
	/* test if already running */
	in_ce_dispatch_shutdown = 0;
	if( p_semaphore( SEMGET, XA_SEM, 0 ) != EBADARG )
	{
		display("XaAES already running!" );
		/* release */
		p_semaphore( SEMRELEASE, XA_SEM, 0 );
		goto leave;
	}
	/* create XaAES semaphore, gets released at exit */
	p_semaphore( SEMCREATE, XA_SEM, 0 );
	/* create _SCP semaphore and release it immediately */
	p_semaphore( SEMCREATE, XA_SCP, 0 );
	p_semaphore( SEMRELEASE, XA_SCP, 0 );

	/*
	 * setup kernel thread
	 */
	C.AESpid = p_getpid();

	/*
	 * Set MiNT domain, else keyboard stuff dont work correctly
	 */
	p_domain(1);

	setup_common();

	/* join process group of loader */
	p_setpgrp(0, loader_pgrp);

	c_naes = NULL;

	if (cfg.naes_cookie)
	{
		install_cookie( (void**)&c_naes, (void*)&naes_cookie, sizeof(*c_naes), C_nAES, true );
	}
	C.reschange = NULL;
	{
	short x;
	if( mt_appl_init(my_global_aes) != -1 )
	{
		C.P_handle = mt_graf_handle( &x, &x, &x, &x, my_global_aes );
	}
	else mt_appl_exit(my_global_aes);
	}
	BLOG((0,"apid=%d,P_handle=%d", my_global_aes[2], C.P_handle));

	/*
	 * register trap#2 handler
	 */

	if (register_trap2(XA_handler, 0, 0, 0))
	{
		display(/*00000012*/"ERROR: register_trap2 failed!");
		goto leave;
	}

	C.Aes->options.app_opts |= XAAO_OBJC_EDIT;
	/*
	 * Initialization AES/VDI
	 */

	if (!next_res)
			next_res = cfg.videomode;

	if (!next_dev)
			next_dev = cfg.device;

	pferr = ferr;

	if ((ferr = k_init(next_dev, next_res)) != 0)
	{
		display(/*00000013*/"ERROR: k_init failed!");
		goto leave;
	}
	{
	char *bl;
	long r;
	if( (r=s_system(S_GETBOOTLOG, (long)&bl, 0 )) >= 0 )
	{
		//fp = kernel_open("c:/mint/boot.log", O_RDWR|O_CREAT, NULL,NULL);
		long err = 0;
		fp = kernel_open(bl, O_RDWR|O_CREAT, &err,NULL);
		if (fp)
		{
			kernel_lseek(fp, 0, SEEK_END);
			fo = p->p_fd->ofiles[1];
			p->p_fd->ofiles[1] = fp;	// f->links++;
		}
	}
	}
	read_inf();

	/*
	 * Initialization I/O
	 */

	/* Open the MiNT Salert() pipe to be polite about system errors */
	C.alert_pipe = f_open(alert_pipe_name, O_CREAT|O_RDWR);
	if (C.alert_pipe < 0)
	{
		display(/*00000014*/"ERROR: Can't open alert pipe '%s' :: %ld",
			alert_pipe_name, C.alert_pipe);

		ferr = 1;
		goto leave;
	}

	/* Open the u:/dev/console device to get keyboard input */
	C.KBD_dev = f_open(KBD_dev_name, O_DENYRW|O_RDONLY);
	if (C.KBD_dev < 0)
	{
		display(/*00000015*/"ERROR: Can't open keyboard device '%s' :: %ld",
			KBD_dev_name, C.KBD_dev);

		ferr = 2;
		goto leave;
	}

	BLOG((0,"alert:%ld, KBD:%ld,pferr=%d", C.alert_pipe, C.KBD_dev, pferr ));

	/* initialize mouse */
	if (!init_moose())
	{
		display(/*00000016*/"ERROR: init_moose failed");
		ferr = 3;
		goto leave;
	}

	/*
	 * Start AES thread
	 */
	{
		long tpc;

		C.Aes->block = aesthread_block;
		tpc = kthread_create(C.Aes->p, aesthread_entry, C.Aes, &C.Aes->tp, "%s", aesthread_name);
		if (tpc < 0)
		{
			C.Aes->tp = NULL;
			display(/*00000017*/"ERROR: start AES thread failed");
			ferr = 4;
			goto leave;
		}
	}
	/*
	 * Start AES help thread - will be dealing with AES windows.
	 */
	{
		long tpc;
		tpc = kthread_create(C.Aes->p, helpthread_entry, NULL, NULL, "%s", aeshlp_name);
		if (tpc < 0)
		{
			C.Hlp = NULL;
			display(/*00000018*/"ERROR: start AES helper thread failed");
			ferr = 5;
			goto leave;
		}
	}
	/*
	 * Wait for aeshlp to start
	 */
	while (!C.Hlp)
		yield();

	xam_load(true);

	if (cfg.opentaskman)
		post_cevent(C.Hlp, ceExecfunc, open_taskmanager,NULL, 1,0, NULL,NULL);

	add_to_tasklist(C.Aes);
	add_to_tasklist(C.Hlp);
#if CHECK_STACK
	if( stack_align & 0x111 )
	{
		ALERT(( "WARNING:your stack is odd!" ));
	}
#endif
	C.DSKpid = -1;

	/* initialize mouse-position */
	{
		short b, x, y;
		vq_mouse( C.Aes->vdi_settings->handle, &b, &x, &y );
		adi_move( 0, x, y );
		xa_graf_mouse(-1, NULL, C.Aes, true);
	}

	/*
	 * console-output:
	 * if RAW Ctrl-S is not eaten by the kernel, but \n is not translated to \r\n
	 * if COOKED Ctrl-S may confuse XaAES but \n is translated to \r\n
	 *
	 * set RAW for maximum TOS-compatibility - toswin-clients have to use their own settings
	 *
	 */
	set_tty_mode( RAW/*COOKED*/ );

	C.Aes->waiting_for |= XAWAIT_MENU;

	default_input_channels = 1UL << C.KBD_dev;	/* We are waiting on all these channels */
	default_input_channels |= 1UL << C.alert_pipe;	/* Monitor the system alert pipe */
	tty = (struct tty *)C.Aes->p->p_fd->ofiles[C.KBD_dev]->devinfo;

	if( pferr )
	{
		post_cevent(C.Hlp, ceExecfunc, ce_dispatch_shutdown, NULL, RESTART_AFTER_BOMB, 1, NULL, NULL);
	}
	reset_about();
	post_cevent(C.Hlp, ceExecfunc, open_about,NULL, 0,0, NULL,NULL);
	if (cfg.allow_setexc != -1 )
	{
		/* don't want anyone to change system-vectors */
		long r;
		if( (r=s_system(S_SETEXC, -1, 0 )) >= 0 )
		{
			p_exc = r;
			if( (r=s_system(S_SETEXC, cfg.allow_setexc, 0 )) )
				BLOG((0,"could not set SETEXC to %d:%ld", cfg.allow_setexc, r));
		}
		else
			BLOG((0,"could not get SETEXC:%ld", r));
	}

	if( fp )
	{
		kernel_close(fp);
		p->p_fd->ofiles[1] = fo;
	}

	/* redirect stderr (mintlib (main.c) assumes this to be done when argv[0] != 0, see launch()) */
	(void)_f_force(2,1);

	if( !pferr )
		post_cevent(C.Hlp, CE_start_apps, NULL,NULL, 0,0, NULL,NULL);

	/*
	 * Main kernel loop
	 */
	do
	{
		/* The root of all locking under AES pid. */
		/* how about this? It means that these
	         * semaphores are not needed and are effectively skipped.
		 */
		int lock = LOCK_WINLIST|LOCK_ENVSTR|LOCK_PENDING;
		unsigned long input_channels;
		long fs_rtn;

		input_channels = default_input_channels;

		/* The pivoting point of XaAES!
		 * Wait via Fselect() for keyboard and alerts.
		 */
		PROFILE(("main:f_select:%ld", n++ ));
		if( aessys_timeout == 0 )
			aessys_timeout = AESSYS_TIMEOUT;
		fs_rtn = f_select(aessys_timeout, (long *) &input_channels, 0L, 0L);

		DIAG((D_kern, NULL,">>Fselect -> %ld, channels: 0x%08lx, update_lock %d(%s), mouse_lock %d(%s)",
			fs_rtn,
			input_channels,
			update_locked() ? update_locked()->pid : 0,
			update_locked() ? update_locked()->name : "",
			mouse_locked() ? mouse_locked()->pid : 0,
			mouse_locked() ? mouse_locked()->name : ""));

		if (fs_rtn > 0)
		{
			set_boot_focus(lock);
			C.boot_focus = 0;
			if (input_channels & (1UL << C.KBD_dev))
			{
				keyboard_input(lock);
			}

			if (input_channels & (1UL << C.alert_pipe))
			{
				alert_input(lock);
			}
		}
		else if( aessys_timeout > 1 )
		{
			tty->state &= ~TS_COOKED;	/* we are kernel ... */
			if( C.rdm )
			{
				redraw_menu(0);
				C.rdm = 0;
			}
		}
		while (aessys_timeout == 1)
		{
			/* some regular thing todo */

			/* XXX
			 * hardcoded the only needed place;
			 * replace it by some register function
			 * with callback interface
			 */
			do_widget_repeat();
			yield();
		}

		/* execute delayed delete_window */
		if (S.deleted_windows.first)
		{
			do_delayed_delete_window(lock);
		}
	}
	while (!(C.shutdown & EXIT_MAINLOOP));

	BLOG((false, "**** Leave kernel for shutdown"));
	wait = 0;
	if (C.sdt)
		cancelroottimeout(C.sdt);
	C.sdt = NULL;

leave:
	{
		int r;
		/* delete semaphores */
		r = p_semaphore( SEMDESTROY, XA_SCP, 0 );
		if( r )
			BLOG((0,"k_main:could not destroy _SCP semaphore:%d", r ));
		r = p_semaphore( SEMDESTROY, XA_SEM, 0 );
		if( r )
			BLOG((0,"k_main:could not destroy XaAES semaphore:%d", r ));
		/* reset Setexc */
		if( p_exc != -1 && (r=s_system(S_SETEXC, p_exc, 0 )) < 0 )
			BLOG((0,"k_main:could not reset setexc:%d", r ));
	}

	k_exit(wait);

	kthread_exit(0);
}

static void
setup_common(void)
{

	/* terminating signals */
	p_signal(SIGHUP,   (long) ignore);
	p_signal(SIGINT,   (long) ignore);
	p_signal(SIGQUIT,  (long) ignore);
	p_signal(SIGPIPE,  (long) ignore);
	p_signal(SIGALRM,  (long) ignore);
	p_signal(SIGSTOP,  (long) ignore);
	p_signal(SIGTSTP,  (long) ignore);
	p_signal(SIGTTIN,  (long) ignore);
	p_signal(SIGTTOU,  (long) ignore);
	p_signal(SIGXCPU,  (long) ignore);
	p_signal(SIGXFSZ,  (long) ignore);
	p_signal(SIGVTALRM,(long) ignore);
	p_signal(SIGPROF,  (long) ignore);
	p_signal(SIGUSR1,  (long) ignore);
	p_signal(SIGUSR2,  (long) ignore);

#if !GENERATE_DIAGS
	/* fatal signals */
	p_signal(SIGILL,   (long) fatal);
	p_signal(SIGTRAP,  (long) fatal);
	p_signal(SIGABRT,  (long) fatal);
	p_signal(SIGFPE,   (long) ignore);//fatal);
	p_signal(SIGBUS,   (long) fatal);
	p_signal(SIGSEGV,  (long) fatal);
	p_signal(SIGSYS,   (long) fatal);
#endif
	/* other stuff */
	p_signal(SIGTERM,  (long) sigterm);
	p_signal(SIGCHLD,  (long) sigchld);

	d_setdrv('u' - 'a');
	d_setpath("/");

}

static void
restore_sigs(void)
{
	/* don't reenter on fatal signals */
	p_signal(SIGILL,   SIG_IGN);
	p_signal(SIGTRAP,  SIG_IGN);
	p_signal(SIGTRAP,  SIG_IGN);
	p_signal(SIGABRT,  SIG_IGN);
	p_signal(SIGFPE,   SIG_IGN);
	p_signal(SIGBUS,   SIG_IGN);
	p_signal(SIGSEGV,  SIG_IGN);

}

static void
k_exit(int wait)
{
//	display("k_exit");
	C.shutdown |= QUIT_NOW;

	restore_sigs();
	if (C.Aes)
	{
		post_cevent(C.Aes, CE_at_restoresigs, NULL, NULL, 0,0, NULL, NULL);
		yield();
	}
	if (C.Hlp)
	{
		post_cevent(C.Hlp, CE_at_restoresigs, NULL, NULL, 0,0, NULL, NULL);
		yield();
	}

	if (svmotv)
	{
		void *m, *b, *h;

		vex_motv(C.P_handle, svmotv, &m);
		vex_butv(C.P_handle, svbutv, &b);

		if (svwhlv)
		{
			short conf;

			/*
			 * NVDI does not have that call, and may report an error
			 */
			conf = disable_nvdi_errors();
			vex_wheelv(C.P_handle, svwhlv, &h);
			enable_nvdi_errors(conf);
		}
	}

	k_shutdown();
	if (wait)
	{
		display("XaAES: see %s - press any key to continue ...", C.bootlog_path);
		_c_conin();
	}

	if (c_naes)
	{
		delete_cookie( (void**)&c_naes, C_nAES, 1 );
	}
#if WITH_BBL_HELP
	if( cfg.xa_bubble )
		xa_bubble( 0, bbl_disable_and_free, 0, 0 );
#endif
	/*
	 * deinstall trap #2 handler
	 */
	if(register_trap2(XA_handler, 1, 0, 0))
		BLOG((false, "unregister trap handler failed"));

	/*
	 * close input devices
	 */
	if (G.adi_mouse)
	{
		BLOG((false, "Closing adi_mouse"));
		adi_close(G.adi_mouse);
//		adi_unregister(G.adi_mouse);
//		G.adi_mouse = NULL;
	}


	if( my_global_aes[2] != -1 )
	{
		mt_appl_exit(my_global_aes);
	}


	/*
	 * close profile
	 */
	PRCLOSE;

	/*
	 * reset keyboard
	 */
	if( cfg.keyboards.c )
		switch_keyboard( "keyboard" );
	if (C.alert_pipe > 0)
	{
		f_close(C.alert_pipe);
	}
	BLOG((1,"wake loader: %lx;shutdown=%x", (long)&loader_pid, C.shutdown));
	wake(WAIT_Q, (long)&loader_pid);

	/* XXX todo -> module_exit */

	if (C.KBD_dev > 0)
	{
		f_cntl(C.KBD_dev, (long)&KBD_dev_sg, TIOCSETN);
		f_close(C.KBD_dev);
	}

	/* reset single-flags in case of previous fault */
	pid2proc(0)->modeflags &= ~(M_SINGLE_TASK|M_DONT_STOP);

	kthread_exit(0);
}
