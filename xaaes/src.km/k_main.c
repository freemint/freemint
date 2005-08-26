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

#include RSCHNAME

#include "k_main.h"
#include "xa_global.h"

#include "app_man.h"
#include "adiload.h"
#include "c_window.h"
#include "desktop.h"
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

#include "xa_appl.h"
#include "xa_evnt.h"
#include "xa_form.h"
#include "xa_rsrc.h"
#include "xa_shel.h"

#include "mint/dcntl.h"
#include "mint/fcntl.h"
#include "mint/filedesc.h"
#include "mint/ioctl.h"
#include "mint/signal.h"
#include "mint/ssystem.h"
#include "cookie.h"


#include "c_mouse.h"

/*
 * Kernel Message Handler
 * 
 * This is responsible for accepting requests via the XaAES.cmd pipe and
 * sending (most) replies via the clients' reply pipes.
 * 	
 * We also get keyboard & mouse input data here.
 */

void
ceExecfunc(enum locks lock, struct c_event *ce, bool cancel)
{
	if (!cancel)
	{
		void (*f)(enum locks, struct xa_client *);
		if ((f = ce->ptr1))
			(*f)(lock, ce->client);
	}
}
void
cancel_cevents(struct xa_client *client)
{
	struct c_event *ce;

	while ((ce = client->cevnt_head))
	{
		struct c_event *nxt;

		DIAG((D_kern, client, "Cancel evnt %lx (next %lx) for %s",
			ce, ce->next, client->name));
		
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

	DIAG((D_evnt, client, "cancel_CE: find function %lx in client events for %s", f, client->name));

	while (ce)
	{
		bool removed = false;

		if (ce->funct == f)
		{
			DIAGS((" --- Found func==%lx, calling callback=%lx", f, callback));
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
				DIAGS(("---------- freeing CE %lx with function %lx", ce, f));
				kfree(ce);
				ce = nce;
				removed = true;
			}
#if GENERATE_DIAGS
			else
				DIAGS((" --- dont delete client event"));
#endif
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
	void (*func)(enum locks, struct c_event *, bool cancel),
	void *ptr1, void *ptr2,
	int d0, int d1, const RECT *r,
	const struct moose_data *md)
{
	struct c_event *c;

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
				c, client->cevnt_count, client->cevnt_head, client->cevnt_tail,
				client->name));
		}
		else
		{
			DIAGS(("kmalloc(%i) failed, out of memory?", sizeof(*c)));
		}
		Unblock(client, 1, 0);
	}
}

short
dispatch_cevent(struct xa_client *client)
{
	struct c_event *ce;
	short ret = 0;

	ce = client->cevnt_head;
	if (ce)
	{
		struct c_event *nxt;

		DIAG((D_kern, client, "Dispatch evnt %lx (head %lx, tail %lx, count %d) for %s",
			ce, client->cevnt_head, client->cevnt_tail, client->cevnt_count, client->name));
		
		
// 		(*ce->funct)(0, ce, false);

		if (!(nxt = ce->next))
			client->cevnt_tail = nxt;

		client->cevnt_head = nxt;
		client->cevnt_count--;
	
		(*ce->funct)(0, ce, false);
		kfree(ce);

// 		ret = client->cevnt_count + 1;
		ret = (volatile short)client->cevnt_count;
	}
	return ret;
}

void
do_block(struct xa_client *client)
{
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
// 		client->sleeplock = (long)client;
		sleep(IO_Q, client->sleeplock); //(long)client);
	}
	client->blocktype = XABT_NONE;
	client->sleeplock = 0;
}

void
Block(struct xa_client *client, int which)
{
	(*client->block)(client, which);
}

void
cBlock(struct xa_client *client, int which)
{

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
			cancel_evnt_multi(client, 1);
			cancel_mutimeout(client);
		}
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
		DIAG((D_kern, client, "[%d]Blocked %s", which, c_owner(client)));
		
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
				cancel_evnt_multi(client, 1);
 				cancel_mutimeout(client);
			}
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
		cancel_evnt_multi(client, 1);
		cancel_mutimeout(client);
	}
}

static void
iBlock(struct xa_client *client, int which)
{
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
			cancel_evnt_multi(client, 1);
			cancel_mutimeout(client);
		}
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
		DIAG((D_kern, client, "[%d]Blocked %s", which, c_owner(client)));
		
		if (client->tp_term)
			return;
		
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
				cancel_evnt_multi(client, 1);
 				cancel_mutimeout(client);
			}
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
		cancel_evnt_multi(client, 1);
		cancel_mutimeout(client);
	}
}
void
Unblock(struct xa_client *client, unsigned long value, int which)
{
	/* the following served as a excellent safeguard on the
	 * internal consistency of the event handling mechanisms.
	 */
 	if (client == C.Aes)
 		wake(IO_Q, client->sleeplock);
 	else
	{
		if (value == XA_OK)
			cancel_evnt_multi(client,1);

		if (client->blocktype == XABT_SELECT)
			wakeselect(client->p);
		else if (client->blocktype == XABT_SLEEP)
			wake(IO_Q, client->sleeplock); //(long)client);
	}

	DIAG((D_kern,client,"[%d]Unblocked %s 0x%lx", which, c_owner(client), value));
}

static vdi_vec *svmotv = NULL;
static vdi_vec *svbutv = NULL;
static vdi_vec *svwhlv = NULL;

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

	G.adi_mouse = adi_name2adi("moose_w");
	if (!G.adi_mouse)
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
					display("init_moose: Your moose.adi is outdated, please update!");
					return false;
				}
			}
			else
			{
				display("init_moose: Could not obtain moose.adi version, please update!");
				return false;
			}
			
			aerr = adi_ioctl(G.adi_mouse, MOOSE_READVECS, (long)&vecs);
			if (aerr == 0 && vecs.motv)
			{
				vex_motv(C.P_handle, vecs.motv, (void **)(&svmotv));
				vex_butv(C.P_handle, vecs.butv, (void **)(&svbutv));

				if (vecs.whlv)
				{
					vex_wheelv(C.P_handle, vecs.whlv, (void **)(&svwhlv));
					DIAGS(("Wheel support present"));
					//display("Wheel support present");
				}
				else
				{
					DIAGS(("No wheel support"));
					//display("No wheel support present");
				}

				if (adi_ioctl(G.adi_mouse, MOOSE_DCLICK, (long)cfg.double_click_time))
					display("Moose set dclick time failed");

				if (adi_ioctl(G.adi_mouse, MOOSE_PKT_TIMEGAP, (long)cfg.mouse_packet_timegap))
					display("Moose set mouse-packets time-gap failed");

				DIAGS(("Using moose adi"));
				ret = true;
			}
			else
				display("init_moose: MOOSE_READVECS failed (%lx)", aerr);
		}
		else
		{
			display("init_moose: opening moose adi failed (%lx)", aerr);	
			G.adi_mouse = NULL;
		}
	}
	else
	{
		display("Could not find moose.adi, please install in %s!", C.Aes->home_path);
	}

	return ret;
}


/* The active widgets are intimately connected to the mouse.
 * There is only 1 mouse, so there is only need for 1 global structure.
 */
XA_PENDING_WIDGET widget_active = { NULL }; /* Pending active widget (if any) */

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
	char *buf;
	enum locks lock;
};

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

		/* if an app left the mouse off */
		forcem();

		/* Bring up an alert */
		do_form_alert(data->lock, C.Aes, 1, data->buf);

		kfree(data);
	}
}

static unsigned short alert_masks[] =
{
	0x0001, 0x0002, 0x0004, 0x0008,
	0x0010, 0x0020, 0x0040, 0x0080
};

static void
alert_input(enum locks lock)
{
	/* System alert? Alert and add it to the log */
	long n;
	unsigned short amask;

	n = f_instat(C.alert_pipe);
	if (n > 0)
	{
		struct display_alert_data *data;
		OBJECT *form, *icon;
		char c;

		data = kmalloc(sizeof(*data));
		if (!data)
		{
			DIAGS(("kmalloc(%i) failed, out of memory?", sizeof(*data)));
			return;
		}

		data->lock = lock;
		data->buf = kmalloc(n+4);
		if (!data->buf)
		{
			kfree(data);

			DIAGS(("kmalloc(%i) failed, out of memory?", n+4));
			return;
		}

		f_read(C.alert_pipe, n, data->buf);

		/* Pretty up the log entry with a nice
		 * icon to match the one in the alert
		 */
		form = ResourceTree(C.Aes_rsc, SYS_ERROR);
		c = data->buf[1];
		switch(c)
		{
		case '1':
			icon = form + SALERT_IC1;
			amask = alert_masks[1];
			break;
		case '2':
			icon = form + SALERT_IC2;
			amask = alert_masks[2];
			break;
		case '3':
			icon = form + SALERT_IC3;
			amask = alert_masks[3];
			break;
		case '4':
			icon = form + SALERT_IC4;
			amask = alert_masks[4];
			break;
		default:
			icon = NULL;
			amask = alert_masks[0];
			break;
		}

		/* Add the log entry */
		{
			struct scroll_info *list = object_get_slist(form + SYSALERT_LIST);
			struct seget_entrybyarg p = { 0 };
			struct scroll_content sc = { 0 };
			
			sc.icon = icon;
			sc.text = data->buf;
			sc.n_strings = 1;
			p.arg.typ.txt = "Alerts";
			list->get(list, NULL, SEGET_ENTRYBYTEXT, &p);
			list->add(list, p.e, NULL, &sc, p.e ? SEADD_CHILD: 0, SETYP_MAL, true);
		}

		 /* Now you can always lookup the error in the log. */
		DIAGS(("ALERT PIPE: '%s' %s", data->buf, update_locked() ? "pending" : "displayed"));

		if ((cfg.alert_winds & amask))
			display_alert(NULL, (long)data);
		else
			kfree(data);
	}
	else
	{
		DIAGS(("No bytes in alert_pipe"));
	}
}

static void k_exit(void);
static void restore_sigs(void);
static void setup_common(void);

/*
 * signal handlers
 */
static void
ignore(void)
{
	DIAGS(("AESSYS: ignored signal"));
	KERNEL_DEBUG("AESSYS: ignored signal");
}
#if !GENERATE_DIAGS
static void
fatal(void)
{
	KERNEL_DEBUG("AESSYS: fatal error, try to cleaning up");
	k_exit();
}
#endif
static void
sigterm(void)
{
	DIAGS(("AESSYS: sigterm received, dispatch_shutdown(0)"));
	KERNEL_DEBUG("AESSYS: sigterm received, dispatch_shutdown(0)");
	dispatch_shutdown(0);
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

static void
aesthread_block(struct xa_client *client, int which)
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
		aesthread_block(client, 0);
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
	
	p_domain(1);
	setup_common();

	client = init_client(0);

	if (client)
	{
		AESPB *pb;
		short *d;

		if ((pb = kmalloc(sizeof(*pb) + ((12 + 32 + 32 + 32 + 32 + 32) * 2))))
		{
			(long)d = (long)pb + sizeof(*pb);
			pb->control = d;
			d += 12;
			pb->global = d;
			d += 32;
			pb->intin = (const short *)d;
			d += 32;
			pb->intout = d;
			d += 32;
			pb->addrin = (const long *)d;
			d += 32;
			pb->addrout = (long *)d;
		
			C.Hlp = client;
			C.Hlp_pb = client->waiting_pb = pb;
			client->waiting_for = 0;
			client->block = iBlock;
			init_helpthread(NOLOCKING, client);
// 			open_taskmanager(0, client);
			for (;;)
			{
				(*client->block)(client, 0);
				if (client->tp_term)
					break;
			}
		}
		exit_client(0, client, 0, false, false);
		C.Hlp = NULL;
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
CE_at_restoresigs(enum locks lock, struct c_event *ce, bool cancel)
{
	if (!cancel)
	{
		restore_sigs();
	}
}

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

#if 0
#define C_MAGX 0x4d616758	/* MagX */
static MAGX_COOKIE *c_magx = NULL;

static MAGX_AESVARS magx_aesvars =
{
	0x87654321,
	NULL,
	NULL,
	(((long)'M'<<24)|((long)'A'<<16)|('G'<<8)|'X'),
	(25<<9)|(1<<5)|1,		/* 1 jan, 2005	*/
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	0x0600,
	0x0003
};

static MAGX_DOSVARS magx_dosvars =
{
	NULL,
	0,
	0,
	0L,
	0L,
	0L,
	NULL,
	0L,
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	0L,
	0L,
	0L
};
	
static MAGX_COOKIE magx_cookie =
{
	0L,
	&magx_dosvars,
	&magx_aesvars,
	NULL,
	NULL,
	0L
};
#endif

static TIMEOUT *sdt = NULL;
static short shutdown_started = 0;
static short shutdown_step;

static void
sshutdown_timeout(struct proc *p, long arg)
{
	sdt = NULL;

	if (C.update_lock || S.clients_exiting)
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
			 *
			 */
			shutdown_step = 0;

			FOREACH_CLIENT(client)
			{
				if (isin_namelist(cfg.kwq, client->proc_name, 8, NULL, NULL))
				{
					client->status |= CS_SIGKILLED;
					ikill(client->p->pid, SIGKILL);
				}
			}
			
			quit_all_apps(NOLOCKING, NULL, (C.shutdown & RESOLUTION_CHANGE) ? AP_RESCHG : AP_TERM);
			C.shutdown |= SHUTTING_DOWN;
			set_shutdown_timeout(1000);
		}
		else
		{
			struct xa_client *flag = NULL;

			if (shutdown_step == 0)
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
						if (isin_namelist(cfg.kwq, client->proc_name, 8, NULL,NULL))
						{
							client->status |= CS_SIGKILLED;
							ikill(client->p->pid, SIGKILL);
						}
						else
						{
							flag = client;
							Unblock(client, 1, 1);
						}
					}
				}
				if (flag)
				{
					shutdown_step = 1;
					set_shutdown_timeout(10000);
				}
			}
			else
			{
				/*
				 * Step 3: Here we brutally kill any clients whose name is listed in
				 *         the kill-without-question list, and ask the user what to
				 *         do with other clients that havent termianted yet.
				 */
				FOREACH_CLIENT(client)
				{
					if (!(client->status & (CS_EXITING|CS_SIGKILLED)) && client != C.Aes && client != C.Hlp)
					{
						if (isin_namelist(cfg.kwq, client->proc_name, 8, NULL, NULL))
							ikill(client->p->pid, SIGKILL);
						else
						{
							flag = client;
							break;
						}
					}
				}
				if (flag)
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
	if (sdt)
	{
		cancelroottimeout(sdt);
		sdt = NULL;
	}
	if (!(C.shutdown & EXIT_MAINLOOP) && shutdown_started)
		sdt = addroottimeout(delta, sshutdown_timeout, 1);
	return sdt;
}

void
kick_shutdn_if_last_client(void)
{
	if (shutdown_started)
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
		if (!f)
			set_shutdown_timeout(0);
	}
}

void
dispatch_shutdown(int flags)
{
	if (!shutdown_started)
	{
		C.shutdown = QUIT_NOW | flags;
		shutdown_started = 1;
		set_shutdown_timeout(0);
	}

// 	C.shutdown = QUIT_NOW | flags;
// 	wakeselect(C.Aes->p);
}

void
k_main(void *dummy)
{
	int wait = 1;

	shutdown_started = 0;
	sdt = NULL;

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
		DIAGS(("Install 'nAES' cookie.."));
		if ( ((long)c_naes = m_xalloc(sizeof(*c_naes), (4<<4)|(1<<3)|3) ))
		{
			memcpy(c_naes, &naes_cookie, sizeof(*c_naes));
			if (s_system(S_SETCOOKIE, C_nAES, (long)c_naes) != 0)
			{
				m_free(c_naes);
				c_naes = NULL;
				DIAGS(("Installing 'nAES' cookie failed!"));
			}
#if GENERATE_DIAGS
			else
			{
				DIAGS(("Installed 'nAES' cookie in readable memory at %lx", (long)c_naes));
			}
#endif
		}
#if GENERATE_DIAGS
		else
		{
			DIAGS(("Could not get memory for 'nAES' cookie! (Mxalloc() fail)"));
		}
#endif
	}
#if 0
	/* Ozk:
	 * This was extremely not successful! :-) MagiC's fantastic "think I'll do this shel_write() mode like this insteadof
	 * like MultiTOS's way" is the biggest problem.
	 */
	{
		DIAGS(("Install 'magx' cookie.."));
		if ( ((long)c_magx = m_xalloc(sizeof(*c_magx) + sizeof(MAGX_DOSVARS) + sizeof(MAGX_AESVARS) + 16, (4<<4)|(1<<3)|3) ))
		{
			MAGX_COOKIE *mc;
			MAGX_DOSVARS *dv;
			MAGX_AESVARS *av;
			int *p;

			mc = c_magx;
			dv = (MAGX_DOSVARS *)((long)mc + sizeof(MAGX_COOKIE));
			av = (MAGX_AESVARS *)((long)dv + sizeof(MAGX_DOSVARS));
			p  = (int *)((long)av + sizeof(MAGX_AESVARS));
			
			memcpy(mc, &magx_cookie, sizeof(*mc));
			mc->dosvars = dv;
			mc->aesvars = av;
			memcpy(dv, &magx_dosvars, sizeof(*dv));
			memcpy(av, &magx_aesvars, sizeof(*av));
			
			p[0] = naes_cookie.time;
			p[1] = naes_cookie.date;
			dv->dos_time = (int *)&p[0];
			dv->dos_date = (int *)&p[1];

			if (s_system(S_SETCOOKIE, C_MAGX, (long)c_magx) != 0)
			{
				m_free(c_magx);
				c_magx = NULL;
				DIAGS(("Installing 'magx' cookie failed"));
			}
		}
	}
#endif
	{
		long vdo;

		C.reschange = NULL;
		if (!(s_system(S_GETCOOKIE, COOKIE__VDO, (unsigned long)(&vdo))))
		{
			switch (((vdo & 0xffff0000) >> 16))
			{
				case 0 ... 2:
					C.reschange = open_reschange;
					break;
				case 3 ... 4:
					C.reschange = open_falcon_reschange;
					break;
				default:;
			}
		}
	}
	
	{
		unsigned long mc;

		mvdi_api.dispatch = NULL;

		if (!(s_system(S_GETCOOKIE, COOKIE__VDI, (unsigned long)&mc)))
		{
			mvdi_api = *(struct cookie_mvdi *)mc;
			C.reschange = open_milan_reschange;
// 			display("found _VDI: Dispatch %lx(%lx)", mvdi_api.dispatch, mc->dispatch);
		}
		else
		{
			if (!nova_data)
			{
				if (!(s_system(S_GETCOOKIE, COOKIE_NOVA, (unsigned long)&mc)))
					nova_data = kmalloc(sizeof(struct nova_data));
				
				if (nova_data)
				{
					nova_data->xcb = (struct xcb *)mc;
					nova_data->valid = false;
				}
			}
			if (nova_data)
				C.reschange = open_nova_reschange;
		}
	}
	
	/*
	 * register trap#2 handler
	 */

	if (register_trap2(XA_handler, 0, 0, 0))
	{
		DIAGS(("register_trap2 failed!"));
		goto leave;
	}

	/*
	 * Initialization AES/VDI
	 */
	if (!(next_res & 0x80000000))
		next_res = cfg.videomode;

	if (k_init(next_res) != 0)
	{
		DIAGS(("k_init failed!"));
		goto leave;
	}

	/* 
	 * Initialization I/O
	 */

	/* Open the MiNT Salert() pipe to be polite about system errors */
	C.alert_pipe = f_open(alert_pipe_name, O_CREAT|O_RDWR);
	if (C.alert_pipe < 0)
	{
		display("XaAES ERROR: Can't open '%s' :: %ld",
			alert_pipe_name, C.alert_pipe);

		goto leave;
	}
	DIAGS(("Open '%s' to %ld", alert_pipe_name, C.alert_pipe));

	/* Open the u:/dev/console device to get keyboard input */
	C.KBD_dev = f_open(KBD_dev_name, O_DENYRW|O_RDONLY);
	if (C.KBD_dev < 0)
	{
		display("XaAES ERROR: Can't open '%s' :: %ld",
			KBD_dev_name, C.KBD_dev);

		goto leave;
	}
	DIAGS(("Open '%s' to %ld", KBD_dev_name, C.KBD_dev));

	/* forcing to be a pty master
	 * - pty master ignore job control
	 * - pty master always read in raw mode
	 *
	 * XXX it's just very ugly todo this so
	 */
	//get_curproc()->p_fd->ofiles[C.KBD_dev]->flags |= O_HEAD;

	/* next try
	 * switching tty device into RAW mode
	 */
	{
		struct sgttyb sg;
		long r;

		r = f_cntl(C.KBD_dev, (long)&KBD_dev_sg, TIOCGETP);
		KERNEL_DEBUG("fcntl(TIOCGETP) -> %li", r);
		assert(r == 0);

		sg = KBD_dev_sg;
		sg.sg_flags &= TF_FLAGS;
		sg.sg_flags |= T_RAW;
		KERNEL_DEBUG("sg.sg_flags 0x%x", sg.sg_flags);

		r = f_cntl(C.KBD_dev, (long)&sg, TIOCSETN);
		KERNEL_DEBUG("fcntl(TIOCSETN) -> %li", r);
		assert(r == 0);
	}

	/* initialize mouse */
	if (!init_moose())
	{
		display("XaAES ERROR: init_moose failed");
		goto leave;
	}

	DIAGS(("Handles: KBD %ld, ALERT %ld", C.KBD_dev, C.alert_pipe));

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
			display("XaAES ERROR: start AES thread failed");
			goto leave;
		}
	}
	/*
	 *
	 */
	{
		long tpc;
		tpc = kthread_create(C.Aes->p, helpthread_entry, NULL, NULL, "%s", "aeshlp");
		if (tpc < 0)
		{
			C.Hlp = NULL;
			display("XaAES ERROR: start AES thread failed");
			goto leave;
		}
	}
	/*
	 * Wait for aeshlp to start
	 */
	while (!C.Hlp)
		yield();

	if (cfg.opentaskman)
		post_cevent(C.Hlp, ceExecfunc, open_taskmanager,NULL, 0,0, NULL,NULL);
	/*
	 * Load Accessories
	 */
#if 0
	DIAGS(("loading accs"));
	load_accs();
	DIAGS(("loading accs done!"));
#endif

	/*
	 * startup shell and autorun
	 */

	DIAGS(("loading shell and autorun"));
	{
		enum locks lock = winlist|envstr|pending;
		Path parms;
		int i;

		if (cfg.cnf_shell)
		{
			parms[0] = '\0';
			if (cfg.cnf_shell_arg)
				parms[0] = sprintf(parms+1, sizeof(parms)-1, "%s", cfg.cnf_shell_arg);

			C.DSKpid = launch(lock, 0, 0, 0, cfg.cnf_shell, parms, C.Aes);
			if (C.DSKpid > 0)
				strcpy(C.desk, cfg.cnf_shell);
		}

		for (i = sizeof(cfg.cnf_run)/sizeof(cfg.cnf_run[0]) - 1; i >= 0; i--)
		{
			if (cfg.cnf_run[i])
			{
				DIAGS(("autorun[%d]: cmd=(%lx) '%s'", i, cfg.cnf_run[i], cfg.cnf_run[i] ? cfg.cnf_run[i] : "no cmd"));
				DIAGS(("   args[%d]:    =(%lx) '%s'", i, cfg.cnf_run_arg[i], cfg.cnf_run_arg[i] ? cfg.cnf_run_arg[i] : "no arg"));
				parms[0] = '\0';
				if (cfg.cnf_run_arg[i])
					parms[0] = sprintf(parms+1, sizeof(parms)-1, "%s", cfg.cnf_run_arg[i]);

				launch(lock, 0, 0, 0, cfg.cnf_run[i], parms, C.Aes);
			}
		}
#if 0
		if (cfg.cnf_shell)
		{
			parms[0] = '\0';
			if (cfg.cnf_shell_arg)
				parms[0] = sprintf(parms+1, sizeof(parms)-1, "%s", cfg.cnf_shell_arg);

			C.DSKpid = launch(lock, 0, 0, 0, cfg.cnf_shell, parms, C.Aes);
			if (C.DSKpid > 0)
				strcpy(C.desk, cfg.cnf_shell);
		}
#endif
	}
	DIAGS(("loading shell and autorun done!"));
	DIAGS(("loading accs"));
	load_accs();
	DIAGS(("loading accs done!"));

	C.Aes->waiting_for |= XAWAIT_MENU;

	/*
	 * Main kernel loop
	 */

	do {
		/* The root of all locking under AES pid. */
		/* how about this? It means that these
	         * semaphores are not needed and are effectively skipped.
		 */
		enum locks lock = winlist|envstr|pending;
		unsigned long input_channels;
		long fs_rtn;

		input_channels = 1UL << C.KBD_dev;	/* We are waiting on all these channels */
		input_channels |= 1UL << C.alert_pipe;	/* Monitor the system alert pipe */

		/* The pivoting point of XaAES!
		 * Wait via Fselect() for keyboard and alerts.
		 */
		fs_rtn = f_select(aessys_timeout, (long *) &input_channels, 0L, 0L);	

		DIAG((D_kern, NULL,">>Fselect -> %d, channels: 0x%08lx, update_lock %d, mouse_lock %d",
			fs_rtn,
			input_channels,
			update_locked() ? update_locked()->pid : 0,
			mouse_locked() ? mouse_locked()->pid : 0));

// 		if (C.shutdown & QUIT_NOW)
// 			break;

// 		if (C.shutdown & (QUIT_NOW|RESOLUTION_CHANGE))
// 		{
// 			to_shutdown = addroottimeout(0L, shutdown_timeout, 1);
// 		}

		if (fs_rtn > 0)
		{
			if (input_channels & (1UL << C.KBD_dev))
				keyboard_input(lock);

			if (input_channels & (1UL << C.alert_pipe))
				alert_input(lock);
		}

		if (aessys_timeout)
		{
			/* some regular thing todo */

			/* XXX
			 * hardcoded the only needed place;
			 * replace it by some register function
			 * with callback interface
			 */
			do_widget_repeat();
		}

		/* execute delayed delete_window */
		if (S.deleted_windows.first)
			do_delayed_delete_window(lock);
	}
	while (!(C.shutdown & EXIT_MAINLOOP)); //(QUIT_NOW|RESOLUTION_CHANGE)));

	DIAGS(("**** Leave kernel for shutdown"));
	wait = 0;
	if (sdt)
		cancelroottimeout(sdt);
	sdt = NULL;
leave:
	if (wait)
	{
		display("press any key to continue ...");
		_c_conin();
	}
	k_exit();
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
	p_signal(SIGTRAP,  (long) fatal);
	p_signal(SIGABRT,  (long) fatal);
	p_signal(SIGFPE,   (long) fatal);
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
	p_signal(SIGILL,   SIG_DFL);
	p_signal(SIGTRAP,  SIG_DFL);
	p_signal(SIGTRAP,  SIG_DFL);
	p_signal(SIGABRT,  SIG_DFL);
	p_signal(SIGFPE,   SIG_DFL);
	p_signal(SIGBUS,   SIG_DFL);
	p_signal(SIGSEGV,  SIG_DFL);

}

static void
k_exit(void)
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
// 	display("after yield");

	if (svmotv)
	{
		void *m, *b, *h;

		vex_motv(C.P_handle, svmotv, &m);
		vex_butv(C.P_handle, svbutv, &b);

		if (svwhlv)
			vex_wheelv(C.P_handle, svwhlv, &h);
	}

// 	display("k_shutdown..");
	k_shutdown();
// 	display("done");

	if (c_naes)
	{
		s_system(S_DELCOOKIE, C_nAES, 0L);
		m_free(c_naes);
		c_naes = NULL;
	}
#if 0
	if (c_magx)
	{
		s_system(S_DELCOOKIE, C_MAGX, 0L);
		m_free(c_magx);
		c_magx = NULL;
	}
#endif
// 	display("unreg trap2");
	/*
	 * deinstall trap #2 handler
	 */
	register_trap2(XA_handler, 1, 0, 0);
	DIAGS(("unregistered trap handler"));
// 	display("done");

	/*
	 * close input devices
	 */
	if (G.adi_mouse)
	{
		adi_close(G.adi_mouse);
// 		adi_unregister(G.adi_mouse);
// 		G.adi_mouse = NULL;
	}

	if (C.KBD_dev > 0)
	{
		long r;

		r = f_cntl(C.KBD_dev, (long)&KBD_dev_sg, TIOCSETN);
		KERNEL_DEBUG("fcntl(TIOCSETN) -> %li", r);

		f_close(C.KBD_dev);
	}

	if (C.alert_pipe > 0)
		f_close(C.alert_pipe);

	DIAGS(("closed all input devices"));
	
	/* wakeup loader */
	wake(WAIT_Q, (long)&loader_pid);
		
	DIAGS(("-> kthread_exit"));

	/* XXX todo -> module_exit */
// 	display("kthread_exit...");
	kthread_exit(0);
}
