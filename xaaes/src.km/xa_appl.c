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
#include "xa_appl.h"
#include "xa_global.h"

#if WITH_BBL_HELP
#include "xa_bubble.h"
#endif

#include "xaaes.h"

#include "app_man.h"
#include "c_window.h"
#include "desktop.h"
#include "k_main.h"
#include "k_mouse.h"
#include "k_keybd.h"
#include "messages.h"
#include "menuwidg.h"
#include "draw_obj.h"
#include "sys_proc.h"
#include "taskman.h"
#include "util.h"
#include "widgets.h"
#include "xa_graf.h"
#include "xa_types.h"
#include "xa_evnt.h"
#include "xa_rsrc.h"
#include "xa_pdlg.h"
#include "xa_user_things.h"
#include "version.h"
#include "mint/fcntl.h"
#include "mint/stat.h"
#include "mint/signal.h"


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
			DIAGS(("Found '%s'", op->name));
			client->options = op->options;
			break;
		}

		op = op->next;
	}
}

static void
proc_is_now_client(struct xa_client *client)
{
	/* Ozk:
	 * Since processes not yet called appl_init() can call wind_update()
	 * we need to check and set stuff here... if only programmers
	 * could follow Atari's documentation!!!!
	 */
	if (C.update_lock && C.update_lock == client->p)
		client->fmd.lock |= SCREEN_UPD;
	if (C.mouse_lock && C.mouse_lock == client->p)
		client->fmd.lock |= MOUSE_UPD;

}

void
client_nicename(struct xa_client *client, const char *n, bool autonice)
{
	int l;
	char *d = client->name;

	l = strlen(n);

	if (autonice)
	{
		*d++ = ' ';
		*d++ = ' ';
	}
	if (l >= NICE_NAME)
		l = NICE_NAME - 1;
	strncpy(d, n, l);
	d[l] = '\0';
}

void
init_client_mdbuff(struct xa_client *client)
{
	client->md_head = client->mdb;
	client->md_tail = client->mdb;
	client->md_end = client->mdb + CLIENT_MD_BUFFERS;
	client->md_head->clicks = -1;
}

/*
 * Ozk: New scheme; We keep the shel_info extension throughout the processes
 * lifetime. Reason for this is that applications may call appl_init()/appl_exit()
 * more than once. And that means we need the shel_info on the second init() too.
 * So, shel_info extension is not freed until we are absolutely sure the process
 * is about to terminate...
 */
struct xa_client *
init_client(int lock, bool sysclient)
{
	struct xa_client *client;
	struct proc *p = get_curproc();
	long f;
	extern long loader_pgrp;

	/* if attach_extension succeed it returns a pointer
	 * to kmalloc'ed and *clean* memory area of the given size
	 */
	client = attach_extension(NULL, XAAES_MAGIC, PEXT_NOSHARE, sizeof(*client), &xaaes_cb_vector);
	if (!client) {
		ALERT((xa_strings(AL_ATTACH)/*"attach_extension for %u failed, out of memory?"*/, p->pid));
		return NULL;
	}

	init_client_mdbuff(client);
	client->block = cBlock;

	/*
	 * Stuff the new client inherits from AESSYS
	 */
	client->vdi_settings = C.Aes->vdi_settings;

	client->xmwt = C.Aes->xmwt;
	client->wtheme_handle = C.Aes->wtheme_handle;

	client->objcr_module = C.Aes->objcr_module;

	if (sysclient) {
		client->ut = kmalloc(xa_user_things.len);
		client->mnu_clientlistname = kmalloc(strlen(xa_strings(MNU_CLIENTS))+1);
	} else {
		client->ut = umalloc(xa_user_things.len);
		client->mnu_clientlistname = umalloc(strlen(xa_strings(MNU_CLIENTS))+1);
	}

	if (!client->ut || !client->mnu_clientlistname) {
		ALERT((xa_strings(AL_MEM)/*"umalloc for %u failed, out of memory?"*/, p->pid));

		if (client->ut)
			ufree(client->ut);

		if (client->mnu_clientlistname)
			ufree(client->mnu_clientlistname);

		detach_extension(NULL, XAAES_MAGIC);
		return NULL;
	}

	/*
	 * add to client list
	 * add to app list
	 */
	CLIENT_LIST_INSERT_END(client);
	APP_LIST_INSERT_END(client);

	/* remember process descriptor */
	client->p = p;

	/* initialize trampoline
	 */
	bcopy(&xa_user_things, client->ut, xa_user_things.len);
	/* relocate relative addresses */
	client->ut->progdef_p  += (long)client->ut;
	client->ut->userblk_pp += (long)client->ut;
	client->ut->ret_p      += (long)client->ut;
	client->ut->parmblk_p  += (long)client->ut;
	/* make sure data cache is flushed */
	cpushi(client->ut, xa_user_things.len);

	client->cmd_tail = "\0";

	/* Ozk: About the fix_menu() thing; This is just as bad as it
	 * originally was, the client should have an attachment with
	 * umalloced space for such things as this. Did it like this
	 * temporarily...
	 * When changing this, also change it in k_init.c for the AESSYS
	 */
	strcpy(client->mnu_clientlistname, xa_strings(MNU_CLIENTS));

	strncpy(client->proc_name, client->p->name, 8);
	f = strlen(client->proc_name);
	while (f < 8) {
		client->proc_name[f] = ' ';
		f++;
	}
	client->proc_name[8] = '\0';
	strnupr(client->proc_name, 8);
	DIAGS(("proc_name for %d: '%s'", client->p->pid, client->proc_name));

	client->options = default_options;

	/* Individual option settings. */
	get_app_options(client);

	if (client->options.inhibit_hide)
		client->swm_newmsg |= NM_INHIBIT_HIDE;

	/* awaiting menu_register */
	sprintf(client->name, sizeof(client->name), "  %s", client->proc_name);

	DIAGS(("init_client: checking shel info (pid %i) home=%s, fname=%s", client->p->pid, client->home_path, p->fname));
	{
		struct shel_info *info;

		info = lookup_extension(client->p, XAAES_MAGIC_SH);
		if (info) {
			DIAGS(("appl_init: shel_write started"));
			DIAGS(("appl_init: type %i", info->type));
			DIAGS(("appl_init: cmd_name '%s'", info->cmd_name));
			DIAGS(("appl_init: home_path '%s'", info->home_path));

			client->type = info->type;
			/*
			 * The 'real' parent ID, which is the client that called
			 * shel_write() to start us..
			 */
			client->rppid = info->rppid;
			client->cmd_tail = info->cmd_tail;
			strcpy(client->cmd_name, info->cmd_name);
			strcpy(client->home_path, info->home_path);
		}
	}

	/* use loader-pgrp for keyboard-access for clients */
	p_setpgrp(client->p->pid, loader_pgrp);

	/* Get the client's home directory (where it was started)
	 * - we use this later to load resource files, etc
	 */
	if (!*client->home_path || (*client->home_path == 'u' && *(client->home_path+1) == ':' && *(client->home_path+2) == 0))
	{
		char hp[PATH_MAX], tmp[PATH_MAX];
		fcookie fc;
		long r;
		bool done = false;

		strcpy(tmp, p->fname);
		hp[0] = '\0';

		while (!done && path2cookie(p, tmp, NULL, &fc) == 0) {
			XATTR x;

			r = xfs_getxattr(fc.fs, &fc, &x);
			if (!r &&  S_ISLNK(x.mode)) {
				r = xfs_readlink(fc.fs, &fc, tmp, PATH_MAX);
				if (r != E_OK) {
					tmp[0] = '\0';
					done = true;
				}
			} else
				done = true;

			strcpy(hp, tmp);
			release_cookie(&fc);
		}
		fix_path(hp);
		strip_fname(hp, client->home_path, NULL);

	}
	proc_is_now_client(client);

	init_client_objcrend(client);

	if( !client->options.standard_font_point )
		client->options.standard_font_point = cfg.standard_font_point;
	set_standard_point(client);

	/* Reset the AES messages pending list for our new application */
	client->msg = NULL;
	/* Initially, client isn't waiting on any event types */
	cancel_evnt_multi(client);
	/* Initial settings for the clients mouse cursor */
	client->mouse = ARROW;		/* Default client mouse cursor is an arrow */
	client->mouse_form = NULL;
	client->save_mouse = client->mouse;
	client->save_mouse_form = client->mouse_form;
#ifndef ST_ONLY

	if( C.is_init_icn_pal == -1 )
		C.is_init_icn_pal = 0;
#endif

	return client;
}

#include "mint/pathconf.h"

/*
 * make X:\Y from u:/x/y
 *
 * return TOS-compliant path
 */
static char  *strip_uni_drive( char *in )
{
	char *p = in, *p1 = in;
	long c;
	bool cconv;

	c = d_pathconf(in, DP_CASE);
	cconv = c == DP_CASECONV;

	if( (*in == 'u' || *in == 'U') && in[1] == ':' && (in[2] == '/' || in[2] == '\\')
		&& isalpha( in[3] ) && (in[4] == '/' || in[4] == '\\') )
	{
		p1 += 4;
		p += 2;
		in[0] = toupper(in[3]);
	}

	for(; *p1; p++, p1++ )
		if( *p1 == '/' )
			*p = '\\';
		else
			*p = cconv ? toupper(*p1) : *p1;
	if( *(p-1) == '\\' )
		p--;
	*p = 0;

	return in;

}


#define F_FORCE_MINT	0x40000

struct file *xconout_dev = 0;
/*
 * Application initialise - appl_init()
 */
unsigned long
XA_appl_init(int lock, struct xa_client *client, AESPB *pb)
{
	struct aes_global *globl = (struct aes_global *)pb->global;
	struct proc *p = get_curproc();
	struct proc *pp = pid2proc( p->ppid );

	CONTROL(0,1,0);

	DIAG((D_appl, client, "appl_init for %d", p->pid));

	/* some slb do appl_init !?! */
	if( (pp->p_flag & P_FLAG_SLO) || (p->p_flag & P_FLAG_SLB) )
	{
		globl->id =	pb->intout[0] = pp->pid;
		return XAC_DONE;
	}

	if (client) {
		if (client->forced_init_client) {
			client->globl_ptr = globl;
			client->forced_init_client = false;
		}
	} else {
		if ((client = init_client(lock, false))) {

			if( p->domain == 0 )
			{
				strip_uni_drive( client->home_path );
#if 0
				if( isalpha( *client->home_path ) && client->home_path[1] == ':' )
				{
					d_setdrv(drive_from_letter(*client->home_path));
					d_setpath(client->home_path+2);
				}
#endif
			}
			add_to_tasklist(client);
			/* Preserve the pointer to the globl array
			 * so we can fill in the resource address later
			 */
			client->globl_ptr = globl;
		}
	}

	if( (p->modeflags & M_SINGLE_TASK) )
	{
		if( C.SingleTaskPid <= 0 )
		{
			struct shel_info *info = lookup_extension(p, XAAES_MAGIC_SH);
			struct proc *k = pid2proc(0);	/* MiNT */

			if( !info )
			{
				BLOG((0,"can execute singletask-app %s only by shel_write (killing it).", p->name));
				ikill(p->pid, SIGTERM);
				return XAC_DONE;
			}
			BLOG((0,"%s(%d): enter single-mode.", p->name, p->pid));
			if( k )
			{
				struct xa_client *c;

				k->modeflags |= M_SINGLE_TASK;
				C.SingleTaskPid = p->pid;

				/*
				 * stop other AES-clients
				 * dont stop if F_DONT_STOP is set and F_DONT_STOP of singletask-app is not set
				 */
				FOREACH_CLIENT(c)
				{
					if( c->p->pid && c != C.Aes && c != C.Hlp && c != client
						&& ( (p->modeflags & M_DONT_STOP) || !(c->p->modeflags & M_DONT_STOP)) )
					{
						BLOG((0,"stopping %s(%d)", c->name, c->p->pid));
						ikill(c->p->pid, SIGSTOP);
					}
				}
			}
			/* else: shutdown! */
		}
		else if( C.SingleTaskPid == p->ppid )
		{
			BLOG((0,"appl_init: SingleTask launched child" ));
			C.SingleTaskPid = p->pid;
		}
		else if( C.SingleTaskPid != p->pid )
		{
			BLOG((0,"appl_init: Cannot launch in singletask-mode, single-task=%d p=%d", C.SingleTaskPid, p->pid ));
			ikill(p->pid, SIGTERM);
			return XAC_DONE;
		}
		_s_ync();
	}

	if (client) {
		/* fill out global */
		globl->version = 0x0410;		/* Emulate AES 4.1 */
		globl->count = -1;			/* unlimited applications XXX -> mint process limit */
		globl->id = p->pid;			/* appid -> pid */
		globl->pprivate = NULL;
		globl->ptree = NULL;			/* Atari: pointer to pointerarray of trees in rsc. */
		globl->rshdr = NULL;			/* Pointer to resource header. */
		globl->lmem = 0;
		globl->nplanes = screen.planes;
		globl->res1 = 0;
		globl->res2 = 0;
		globl->c_max_h = screen.c_max_h;	/* AES 4.0 extensions */
		globl->bvhard = 4;

		pb->intout[0] = p->pid;
	} else
		pb->intout[0] = -1;

	return XAC_DONE;
}

static void
send_ch_exit(struct xa_client *client, short pid, int code)
{
	if (client) {
		send_app_message(NOLOCKS, NULL, client, AMQ_NORM, 0,
				 CH_EXIT, 0,0, pid,
				 code, 0,0,0);
	}
}

static void
CE_pwaitpid(int lock, struct c_event *ce, short cancel)
{
	long r;

	while ((r = p_waitpid(-1, 1, NULL)) > 0) {

		DIAGS(("sigchld -> %li (pid %li)", r, ((r & 0xffff0000L) >> 16)));
	}

	/* if single-mode-task exited: continue clients, reset flag */
	if( ce->d0 && ce->d0 == C.SingleTaskPid )
	{
		struct proc *k = pid2proc(0);
		struct xa_client *c;

		while( !ikill( C.SingleTaskPid, 0 ) )
		{
		 	yield();
		}
		FOREACH_CLIENT(c)
		{
			if( c->p->pid && c != C.Aes && c != C.Hlp && !(c->status & (CS_EXITING | CS_SIGKILLED)) )
			{
				BLOG((0,"continue %s(%d)", c->name, c->p->pid));
				ikill(c->p->pid, SIGCONT);
			 	yield();	/* mouse ? */
			}
		}
		BLOG((0,"%s: leaving single-mode (k=%lx).", get_curproc()->name, (unsigned long)k));
		k->modeflags &= ~M_SINGLE_TASK;
		C.SingleTaskPid = -1;

		/* menubar may be corrupted */
		C.Aes->nxt_menu = C.Aes->std_menu;	/* kludge to run swap_menu() */
		app_in_front( lock, C.Aes, 0, true, true);
		C.Aes->nxt_menu = 0;
	}

	if (ce->d0)
	{
		send_ch_exit(ce->client, ce->d0, ce->d1);
	}
}
/*
 * Clean up client-indipendant stuff, since XaAES must handle
 * some things even when the process is not yet (or ever) a
 * client.
 */
int
exit_proc(int lock, struct proc *p, int code)
{
	struct shel_info *info;
	struct xa_client *clnt = lookup_extension(p, XAAES_MAGIC);
	int ret = 0;

	/* Unlock mouse & screen */
	if (update_locked() == p)
		free_update_lock();
	if (C.update_lock == p) {
		C.update_lock = NULL;
		C.updatelock_count = 0;
	}
	if (mouse_locked() == p)
		free_mouse_lock();
	if (C.mouse_lock == p) {
		C.mouse_lock = NULL;
		C.mouselock_count = 0;
	}


	/* NOT FINISHED WITH THESE COMMENTS, THEY ARE INCORRECT NOW */
	/* If both xaaes and shel_info extensions are attached, we rely on
	 * the facts that either;
	 *
	 * a -	if both xaaes and shel_info extension are attached, exit_proc()
	 *	is called from exit_client(), and does not necessarily mean that
	 *	the process terminates (may be an appl_exit())
	 *
	 * b -	Only shel_info extension present - the application either crashed
	 *	before doing appl_init(), or it terminates without ever being an
	 *	AES client, or terminates normally after doing an appl_exit().
	 *	In this case we can be sure we are terminating, because exit_proc()
	 *	is only called from exit_client() or on_exit() module callback.
	 *	exit_client() will remove both extensions if it is called on real
	 *	process termination.
	 *
	 * This makes us able to find out our 'real' AES parent and send it a CH_EXIT
	 * AES message.
	 */
	if ((info = lookup_extension(p, XAAES_MAGIC_SH)) && (!clnt || clnt->pexit) ) {
		struct xa_client *client = pid2client(info->rppid);

		/* Ozk:
		 * If process was started via shel_write, we need to perform
		 * pwaitpid on behalf of the parent client.
		 *
		 * We should not sent CH_EXIT for processes that an AES client
		 * started using other means than shel_write(), even if we can.
		 * This is because the AES should not/need not bother, and that
		 * it is defined that shel_write() should be used when AES needs
		 * to bother.
		 */

		/*
		 * in single-task-mode wake client that called single-task-app
		 * to execute CE_pwaitpid()
		 */
		if (client)
		{
			if( C.SingleTaskPid > 0 )
				ikill( client->p->pid, SIGCONT );

			if( client->p->pid == p->ppid && (client->p->modeflags & M_SINGLE_TASK) )
			{
				BLOG((0,"child launched by single-task has exited."));
				C.SingleTaskPid = client->p->pid;
			}
			if (info->shel_write) {
				post_cevent(client, CE_pwaitpid, NULL,NULL, p->pid,code, NULL,NULL);
				ret = 1;
			}
		}

		if (client)
		{
			DIAGS(("Sent CH_EXIT (premature client exit) to (pid %d)%s for (pid %d)%s",
				client->p->pid, client->name, p->pid, p->name));
		} else {
			DIAGS(("No real parent client"));
		}
	}

	return ret;
}

/*
 * Ozk: check if other clients reference us, and remove or fix it
 */
static void
remove_client_crossrefs(struct xa_client *client)
{
	struct xa_client *cl;

	FOREACH_CLIENT(cl) {
		if (cl->nextclient == client) {
			cl->nextclient = NEXT_CLIENT(client);
			break;
		}
	}
}

/*
 * close and free all client resources
 *
 * Called on appl_exit() or on process termination (if app crashed
 * or exit without appl_exit). With other words, this is the final
 * routine *before* the process becomes invalid and should do any
 * cleanup related to this client.
 */
void
exit_client(int lock, struct xa_client *client, int code, bool pexit, bool detach)
{
	struct xa_client *top_owner;
	long redraws;
	bool was_infront = false;

	DIAG((D_appl, NULL, "exit_client: %s", c_owner(client)));

	/* switch menu off for single-task (todo: fix it) */
	if( C.SingleTaskPid > 0 )
		toggle_menu( lock, 0 );

	S.clients_exiting++;

	client->pexit = pexit;

	/*
	 * Clear if client was exclusively waiting for mouse input
	 */
	client->status |= CS_EXITING;

	if (client != C.Hlp) {
		cancel_winctxt_popup(lock, NULL, client);

		remove_from_tasklist(client);
		if (C.csr_client == client)
			post_cevent(C.Hlp, CE_abort_csr, client, NULL, 0,0, NULL,NULL);
		else
			cancel_csr(client);
	}

	if (S.wait_mouse == client) {
		S.wm_count = 0;
		S.wait_mouse = NULL;
	}

	cancel_mutimeout(client);
	/*
	 * Figure out which client to make active
	 */
	top_owner = C.Aes;

	if (is_infront(client)) {
		was_infront = true;
		if (cfg.next_active == 1) {
			top_owner = APP_LIST_START;
			if (top_owner == client)
				top_owner = previous_client(lock, 1);
		} else if (cfg.next_active == 0) {
			struct xa_window *tw = get_topwind(lock, client, window_list, true, XAWS_OPEN|XAWS_HIDDEN, XAWS_OPEN);
			if (!tw)
				tw = get_topwind(lock, client, window_list, true, XAWS_OPEN, XAWS_OPEN);
			if (tw)
				top_owner = tw->owner;
		}
	}

	swap_menu(lock, client, NULL, SWAPM_REMOVE);

	exit_proc(lock, client->p, code);

	/*
	 * It is no longer interested in button released packet
	 */
	if (C.button_waiter == client)
		C.button_waiter = NULL;

	/*
	 * Check and remove if client set its own mouse-form
	 */
	if (C.mouse_owner == client || C.realmouse_owner == client)
	{
		xa_graf_mouse(ARROW, NULL, NULL, false);
	}

	remove_widget_active(client);

	/*
	 * Go through and check that all windows belonging to this
	 * client are closed
	 */
	remove_all_windows(lock, client);

	/*
	 * Cancel, and count, all redraw messages that this
	 * client may have received
	 */
	redraws = cancel_app_aesmsgs(client);
	/*
	 * Cancel all client events waiting for this client, then
	 * set CS_BLOCK_CE to stop any new client events from being
	 * queued..
	 */

	cancel_cevents(client);
	client->status |= CS_BLOCK_CE;
	/*
	 * Clear any queued key events
 	*/
	cancel_keyqueue(client);
	client->rsrc = NULL;

	while (client->resources)
	{
		FreeResources(client, NULL, NULL);
	}

#if WITH_BBL_HELP
	if( cfg.xa_bubble )
	{
		if( !C.shutdown && xa_bubble( 0, bbl_get_status, 0, 0 ) <= bs_inactive && !strcmp( "  BubbleGEM ", client->name ) )
			post_cevent(C.Aes, XA_bubble_event, NULL, NULL, BBL_EVNT_ENABLE, 0, NULL, NULL);
	}
	xa_bubble( 0, bbl_close_bubble1, 0, 0 );
#endif
	/* Free name *only if* it is malloced: */
	if ( client->tail_is_heap) {
		kfree(client->cmd_tail);
		client->cmd_tail = NULL;
		client->tail_is_heap = false;
	}

	/* Ozk:
	 * sending CH_EXIT is done by proc_exit()
	 */

	/*
	 * remove any references
	 */

	if (client->desktop)
		set_desktop(client, true);
	if (was_infront && top_owner != client)
	{
		app_in_front(lock, top_owner, true, true, false);
	}
	/*
	 * remove from client list
	 * remove from app list
	 */
	CLIENT_LIST_REMOVE(client);
	APP_LIST_REMOVE(client);

	if (redraws) {
		C.redraws -= redraws - 1;
		kick_mousemove_timeout();
	}

	/*
	 * free remaining resources
	 */

	free_xa_data_list(&client->xa_data);
	/*
	 * Free wt list last
	 */
	free_attachments(client);
	free_wtlist(client);


	/* free the quart screen buffer */
	if (client->half_screen_buffer)
		ufree(client->half_screen_buffer);

	if (client == C.Aes || client == C.Hlp) {
		if (client->ut)
			kfree(client->ut);
		if (client->mnu_clientlistname)
			kfree(client->mnu_clientlistname);
	} else {
		if (client->ut)
			ufree(client->ut);
		if (client->mnu_clientlistname)
			ufree(client->mnu_clientlistname);
	}

	client->cmd_tail = "\0";

#if 0
	/* Ozk:
	 * If we are called to really terminate the process, we detach
	 * AES extensions here and now. If not, we leave detaching to
	 * our caller(s)...
	 */
	if (client->pexit)
	{
		struct proc *p = client->p;


		DIAG((D_appl, NULL, "Process terminating - detaching extensions..."));
		/* Ozk:
		 * We trust the kernel to detach the client extension.
		 * If that changes, detach client here (means extensions must be detachable
		 * by its callbacks).
		 */
	}
#endif


	/* zero out; just to be sure */
	/* bzero(client, sizeof(*client)); */

	remove_client_crossrefs(client);

	exit_client_widget_theme(client);

	delete_wc_cache(&client->wcc);

	if (client == C.Hlp && !client->tp_term) {
		kfree(C.Hlp_pb);
		C.Hlp_pb = NULL;
		C.Hlp = NULL;
		DIAGS(("attempt to restart XaSYS"));
		wakeselect(C.Aes->p);
	}

	if (C.DSKpid == client->p->pid)
		C.DSKpid = -1;

	exit_client_objcrend( client );

	if (detach)
		detach_extension(NULL, XAAES_MAGIC);

	S.clients_exiting--;

	kick_shutdn_if_last_client();

	DIAG((D_appl, NULL, "client exit done"));
}

/*
 * Application Exit
 */
unsigned long
XA_appl_exit(int lock, struct xa_client *client, AESPB *pb)
{
	struct aes_global *globl = (struct aes_global *)pb->global;
	CONTROL(0,1,0)

	DIAG((D_appl, client, "appl_exit for %d", client->p->pid));

	/* Which process are we? It'll be a client pid */
	pb->intout[0] = client->p->pid;
	client->p->modeflags |= M_XA_CLIENT_EXIT;

	/* XXX ??? */
	/*
	 * Ozk: Weirdest thing; imgc4cd and ic4plus calls appl_exit() directly
	 * after it calls appl_init() and ignoring the appl_exit() make'em work!
	 *
	 * Update: Ozk:
	 *  After changing to keeping shel_info until we are sure process
	 *  terminates, appl_init()/appl_exit() works as expected.
	 *
	 */
	if (	strnicmp(client->proc_name, "wdialog", 7) == 0 ) {
		DIAG((D_appl, client, "appl_exit ignored for %s", client->name));
		return XAC_DONE;
	}

	/* we assume correct termination */
	exit_client(lock, client, 0, false, true);

	if (globl) {
		globl->version = 0;
		globl->count = 0;
	}

	/* and decouple from process context */
	/* detach_extension(NULL, XAAES_MAGIC); */

	return XAC_DONE;

}

/*
 * Free timeslice.
 */
unsigned long
XA_appl_yield(int lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(0,1,0)

	yield();

	pb->intout[0] = 1; /* OK */

	return XAC_DONE;
}

/*
 * AES 4.0 compatible appl_search
 */
unsigned long
XA_appl_search(int lock, struct xa_client *client, AESPB *pb)
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

	if (cpid < 0) {
		cpid = -cpid;
		if (cpid < 1 || cpid > MAXPID)
			cpid = 1;
		next = pid2client(cpid);
		lang = true;
	} else if (cpid == APP_DESK) {
		/* N.AES: short name of desktop program */
		next = pid2client(C.DSKpid);
	} else {
		/* XaAES extension. */
		spec = (cpid & APP_TASKINFO) != 0;
		cpid &= ~APP_TASKINFO;

		if (spec)
			lang = true;

		Sema_Up(LOCK_CLIENTS);

		if (cpid == APP_FIRST) {
			/* simply the first */
			next = CLIENT_LIST_START;
			client->nextclient = NEXT_CLIENT(next);
		} else if (cpid == APP_NEXT) {
			next = client->nextclient;
			if (next)
				client->nextclient = NEXT_CLIENT(next);
		}
		Sema_Dn(LOCK_CLIENTS);
	}

	if (!next) {
		/* No more clients or no active clients */
		o[0] = 0;
	} else {
		cpid = next->p->pid;

		/* replies according to the compendium */
		if (cpid == C.AESpid)
			o[1] = APP_SYSTEM;
		else if (cpid == C.DSKpid)
			o[1] = APP_APPLICATION | APP_SHELL;
		else
			o[1] = next->type;

		/* XaAES extensions. */
		if (spec) {
			if (any_hidden(lock, next, NULL))
				o[1] |= APP_HIDDEN;
			if (focus_owner() == next)
				o[1] |= APP_FOCUS;

			DIAG((D_appl, client, "   --   o[1] --> 0x%x", o[1]));
		}

		o[2] = cpid;

		if (lang) {
			strcpy(fname, next->name);
			DIAGS((" -- nicename '%s'", fname));
		} else {
			strncpy(fname, next->proc_name, 8);
			fname[8] = '\0';
			DIAGS((" -- procname '%s'", fname));
		}
	}
	return XAC_DONE;
}

static void
handle_XaAES_msgs(int lock, union msg_buf *msg)
{
	union msg_buf m = *msg;
	struct xa_client *dest_clnt;
	int mt = m.s.msg;

	DIAGS(("Message to AES %d", mt));
	m.s.msg = 0;

	switch (mt) {
		case XA_M_DESK: {
			DIAGS(("Desk %d, '%s'", m.s.m3, m.s.p2 ? (const char *)m.s.p2 : "~~~"));
			if (m.s.p2 && m.s.m3) {
				strcpy(C.desk, m.s.p2);
				C.DSKpid = m.s.m3;
				m.s.msg = XA_M_OK;
			}
			break;
		}
		default:
			break;
	}
#if WITH_BBL_HELP
	if( cfg.xa_bubble && xa_bubble( lock, bbl_get_status, 0, 0 ) >= bs_closed )
	{
		switch (mt) {
			case BUBBLEGEM_SHOW:
			case BUBBLEGEM_HIDE:
				xa_bubble( lock, bbl_process_event, msg, 0 );
			return;
			/*
			case BUBBLEGEM_ACK:
			case BUBBLEGEM_REQUEST:
			case BUBBLEGEM_ASKFONT:
			case BUBBLEGEM_FONT:
			*/
			default:
			{
				BLOG((0,"handle_XaAES_msgs: unhandled BUBBLE: %x, from %d, win=%d(%s)", mt, m.m[1], m.m[3], get_curproc()->name));
				return;
			}
			break;
		}
	}
#endif
	dest_clnt = pid2client(m.m[1]);
	if (dest_clnt)
		send_a_message(lock, dest_clnt, AMQ_NORM, QMF_CHKDUP, &m);
}

/*
 * XaAES's current appl_write() only works for standard 16 byte messages
 */
unsigned long
XA_appl_write(int lock, struct xa_client *client, AESPB *pb)
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
		handle_XaAES_msgs(lock, m);
	else {
		short i = 50;
		struct xa_client *dest_clnt = NULL;

		/*
		 * Ozk: Problem; 1. aMail starts asock to do network stuff.
		 *               2. aMail sends asock an AES message.
		 *               3. Since asock didnt get the chance to do appl_init() yet,
		 *                  this message ends up in the great big void.
		 *     Solution; If pid2client returns NULL, we yield() and try again upto 50
		 *               times before giving up. Anyone else got better ideas?
		 */
		dest_clnt = pid2client(dest_id);

		while (!dest_clnt && i) {
			yield();
			dest_clnt = pid2client(dest_id);
			i--;
		}

		if (dest_clnt) {
			short amq = AMQ_NORM;
			short qmf = QMF_NORM;
			/* experimental */
			if( (dest_clnt->status & CS_EXITING) )
			{
				BLOG((0,"%s:XA_appl_write:dest_clnt:%s is exiting", client->name, dest_clnt->name));
				pb->intout[0] = rep;
				return XAC_DONE;
			}

			if (m->m[0] == WM_REDRAW) {
				struct xa_window *wind = get_wind_by_handle(lock, m->m[3]);

				if (wind) {
					generate_redraws(lock, wind, (GRECT *)(m->m + 4), RDRW_WA);
					m = NULL;
				} else {
					amq = AMQ_REDRAW;
					qmf = QMF_CHKDUP;
				}
			} else {
				if (client != dest_clnt) {
					/* message conversion */
					switch (m->m[0]) {
						case WM_SIZED:
						case WM_MOVED:
						case WM_ICONIFY ... WM_ALLICONIFY:
						case WM_REPOSED: {
							struct xa_window *wind = get_wind_by_handle(lock, m->m[3]);

							if (wind) {
								short t = (client->options.wind_opts & XAWO_WCOWORK) ? 1 : 0;

								if (dest_clnt->options.wind_opts & XAWO_WCOWORK)
									t |= 2;

								if (t == 1)		/* sender in WCOWORK, receiver in normal */
									*(GRECT *)(m->m + 4) = w2f(&wind->delta, (const GRECT *)(m->m + 4), true);
								else if (t == 2)	/* sender in normal, receiver in WCOWORK */
									*(GRECT *)(m->m + 4) = f2w(&wind->delta, (const GRECT *)(m->m + 4), true);
							}
							break;
						}
						default:;
					}
				}
			}
			if (m)
			{
				send_a_message(lock, dest_clnt, amq, qmf, m);
			}

			if (dest_clnt != client)
				yield();
		}
	}
	pb->intout[0] = rep;
	return XAC_DONE;
}

/*
 * Data table for appl_getinfo
 */
short info_tab[][4] =
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
		1,		/* phys. handle */
		256,		/* no of colours */
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
	/* 5 PC_GEM */
	{
		1,		/* objc_xfind */
		0,
		1,		/* menu_click */
		0		/* shel_rdef/wdef */
	},
	/* 6 extended inquiry */
	{
		0,		/* -1 not a valid app id for appl_read */
		0,		/* -1 not a valid length parameter to shel_get */
		1,		/* -1 is a valid mode parameter to menu_bar */
		1		/* MENU_INSTL is not a valid mode parameter to menu_bar */
	},
	/* 7 MagiC specific */
	{
		/* bit: 0 wdlg_xx(), 1 lbox_xx(), 2 fnts_xx(), 3 fslx_xx(), 4 pdlg_xx() */
		(WDIALOG_WDLG ? 0x01 : 0) |
		(WDIALOG_LBOX ? 0x02 : 0) |
		(WDIALOG_FNTS ? 0x04 : 0) |
		(WDIALOG_FSLX ? 0x08 : 0) |
		(WDIALOG_PDLG ? 0x10 : 0),
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
		0,		/* shel_write (0) makes previous shel_write calls invalid (false) */
		1,		/* shel_write (1) launches program immediately (true) */
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
#define AGI_WF_TOP		0x0001
#define AGI_WF_NEWDESK		0x0002
#define AGI_WF_COLOR		0x0004
#define AGI_WF_DCOLOR		0x0008
#define AGI_WF_OWNER		0x0010
#define AGI_WF_BEVENT		0x0020
#define AGI_WF_BOTTOM		0x0040
#define AGI_WF_ICONIFY		0x0080
#define AGI_WF_UNICONIFY	0x0100
#define AGI_WF_WHEEL		0x0200	/* wind_set(handle, WF_WHEEL, mode, whl_mode, 0,0) available */
#define AGI_WF_FIRSTAREAXYWH	0x0400	/* wind_get(handle, WF_FIRSTAREAXYWH, clip_x, clip_y, clip_w, clip_h) available */
#define AGI_WF_OPTS		0x0800	/* wind_set(handle, WF_OPTS, wopt0, wopt1, wopt2) available */
#define AGI_WF_MENU		0x1000	/* wind_set(handle, WF_MENU) exists */
#define AGI_WF_WORKXYWH		0x2000	/* wind_get/set(handle, WF_WORKXYWH, .. ) available (and bugfixed) */
#define AGI_WF_CALC		0x4000	/* wind_get(handle, WF_CALCx2x, ... ) and WO0_WCOWORK available - theme changes supported*/

#define AGI_WF_WIDGETS		0x0001

#define AGI_ICONIFIER		0x0001
#define AGI_BACKDROP		0x0002
#define AGI_BDSHIFTCLICK	0x0004
#define AGI_HOTCLOSER		0x0008

#define AGI_WINDUPD_CAS		0x0001

	{
		0
	|	AGI_WF_TOP
	|	AGI_WF_NEWDESK
#if 0
	|	AGI_WF_COLOR
	|	AGI_WF_DCOLOR
#endif
	|	AGI_WF_OWNER
	|	AGI_WF_BEVENT
	|	AGI_WF_BOTTOM
	|	AGI_WF_ICONIFY
	|	AGI_WF_UNICONIFY
	|	AGI_WF_WHEEL		/* 01763, see above */
	|	AGI_WF_FIRSTAREAXYWH
	|	AGI_WF_OPTS
	|	AGI_WF_MENU
	|	AGI_WF_WORKXYWH
	|	AGI_WF_CALC,
		0,
		0
	|	AGI_ICONIFIER
#if 0
	|	AGI_BACKDROP
#endif
#if 0
	|	AGI_HOTCLOSER		/* 5,	window behaviours iconifier & click for bottoming */
#endif
	|	AGI_BDSHIFTCLICK,
		0
	|	AGI_WINDUPD_CAS		/*1	wind_update(): check and set available (mode + 0x100) */
	},

#define AGI_WM_NEWTOP		0x0001
#define AGI_WM_UNTOPPED		0x0002
#define AGI_WM_ONTOP		0x0004
#define AGI_AP_TERM			0x0008
#define AGI_CHRES			0x0010
#define AGI_CH_EXIT			0x0020
#define AGI_WM_BOTTOMED		0x0040
#define AGI_WM_ICONIFY		0x0080
#define AGI_WM_UNICONIFY	0x0100
#define AGI_WM_ALLICONIFY	0x0200
#define AGI_WM_REPOSED		0x0400

	/*12 messages
	 * WM_UNTOPPED + WM_ONTOP + AP_TERM + CH_EXIT (HR) + WM_BOTTOMED +
	 * WM_ICONIFY + WM_UNICONIFY
	 */
	{
		0
#if 0
	|	AGI_WM_NEWTOP
#endif
	|	AGI_WM_UNTOPPED
	|	AGI_WM_ONTOP
	|	AGI_AP_TERM
#if 0
	|	AGI_CHRES
#endif
	|	AGI_CH_EXIT
	|	AGI_WM_BOTTOMED
	|	AGI_WM_ICONIFY
	|	AGI_WM_UNICONIFY
	|	AGI_WM_ALLICONIFY	/*0756,	see above */ /* XXX is this correct? 0756 is octal */
	|	AGI_WM_REPOSED
		,
		0
		,
		1
		,		/* WM_ICONIFY gives coordinates */
		0
	},

#define AGI_G_SWBUTTON	0x0001
#define AGI_G_POPUP	0x0002
#define AGI_OS_WHITEBAK	0x0004
#define AGI_G_SHORTCUT	0x0008
	/*13 objects */
	{
		1,		/* 3D objects */
		1,		/* objc_sysvar */
		0,		/* GDOS fonts */
		0		/* Extended objects */
	|	AGI_G_SWBUTTON
	|	AGI_G_POPUP
	|	AGI_OS_WHITEBAK
	|	AGI_G_SHORTCUT
	},
	/*14 form support (form_xdo, form_xdial) */
	{
		0,		/* MagiC flydials */
		0,		/* MagiC keyboard tables */
		1,		/* return last cursor position */
		0
	},
	/*15 <-- 64 */
	{
		0,		/* shel_write and AP_AESTERM */
		1,		/* shel_write and SHW_SHUTDOWN/SHW_RESCHANGE */
		3,		/* appl_search with long names and additive mode APP_TASKINFO available. */
		1		/* form_error and all GEMDOS errorcodes */
	},
	/*16 <-- 65 */
	{
		1,		/* appl_control exists. */
		APC_INFO,	/* highest opcode for appl_control */
		0,		/* shel_help exists. */
		0		/* wind_draw exists. */
	},
	/*17 <-- 22360 Winx */
	{
		0x0008,			/* WF_SHADE is supported */
		(0x0002|0x0004),	/* WM_SHADED/WM_UNSHADED supported */
		0,
		0
	},

	/* 18 <-- 96 AES_VERSION */
	/* AES version information */
	{
		XAAES_MAJ_VERSION,	/* Major */
		XAAES_MIN_VERSION,	/* Minor */
		DEV_STATUS,	/* Status */
		ARCH_TARGET	/* Target */
	},
	/* 19 <-- 97 AES_WOPTS */
	{
		(XAWO_SUPPORTED >> 16),
		(XAWO_SUPPORTED & 0xffff),
		0,
		0
	},
	/* 20 <-- 98 AES_WFORM */
	{
		0
	|	AGI_AOPTS	/* appl_options() present */
	|	AGI_WFORM
#if WITH_OBJC_DATA
	|	AGI_OBJCDATA
#endif
		,
		0,
		0,
		0
	},
	/* 21 <-- 99 AES_AOPTS */
	{
		(XAAO_SUPPORTED >> 16),
		(XAAO_SUPPORTED & 0xffff),
		0,
		0
	},
};

#define XA_VERSION_INFO	18
#define LAST_IN_INFOTAB AES_AOPTS

#ifndef AGI_WINX
#define AGI_WINX WF_WINX
#endif

/*
 * appl_getinfo() handler
 *
 * Ozk: appl_getinfo() may be called by processes not yet called
 * appl_init(). So, it must not depend on client being valid!
 */
unsigned long
XA_appl_getinfo(int lock, struct xa_client *client, AESPB *pb)
{
	unsigned short gi_type = pb->intin[0];
	int i, n_intout = 5;

	CONTROL(1,5,0)

	/* Extremely curious to who's asking what. */
	if (client)
	{
		DIAG((D_appl, client, "appl_getinfo %d for %s", gi_type, c_owner(client)));
	} else
	{
		DIAG((D_appl, client, "appl_getinfo %d for non AES process (pid %ld)", gi_type, p_getpid()));
	}
	if (gi_type > 14) {
		switch (gi_type) {
			case 64:
			case 65: {
				gi_type -= (64 - 15);
				break;
			}
			case AGI_WINX: {
				gi_type = 17;
				break;
			}
			case AES_VERSION: {
				if (pb->control[N_ADDRIN] == 4) {
					char *d;

					if ((d = (char *)pb->addrin[0])) {
						for (i = 0; i < 8; i++)
							*d++ = aes_id[i];
					}
					if ((d = (char *)pb->addrin[1]))
						strcpy(d, long_name);
					if ((d = (char *)pb->addrin[2]))
						strcpy(d, info_string);
				}
				n_intout = pb->control[N_INTOUT];
				gi_type = XA_VERSION_INFO;
				break;
			}
			case AES_WOPTS ... LAST_IN_INFOTAB: {
				gi_type = (gi_type - AES_VERSION) + XA_VERSION_INFO;
				break;
			}
			default: {
				gi_type = -1;
				break;
			}
		}
	}

	info_tab[0][0] = screen.standard_font_height;
	if (gi_type != -1) {
		if (n_intout > 5) n_intout = 5;

		for (i = 1; i < n_intout; i++)
			pb->intout[i] = info_tab[gi_type][i-1];

#if WDIALOG_PDLG
		if (gi_type == 7 && !pdlg_available())
			pb->intout[1] &= ~0x10;
#endif
		gi_type = 1;
	} else
		gi_type = 0;

	pb->intout[0] = gi_type;

	return XAC_DONE;
}

unsigned long
XA_appl_options(int lock, struct xa_client *client, AESPB *pb)
{
	short ret = 1, mode = pb->intin[0];

	CONTROL(5,5,0)

	switch (mode)
	{
		case 0:
		{
			unsigned long ao = ((unsigned long)(unsigned short)pb->intin[1] << 16) | (unsigned long)(unsigned short)pb->intin[2];

			client->options.app_opts &= ~ao;
			goto ret_aopts;
		}
		case 1:
		{
			unsigned long ao = ((unsigned long)(unsigned short)pb->intin[1] << 16) | (unsigned long)(unsigned short)pb->intin[2];

			client->options.app_opts |= ao;
ret_aopts:
			ao = client->options.app_opts;
			pb->intout[1] = (unsigned long)client->options.app_opts >> 16;
			pb->intout[2] = (unsigned long)client->options.app_opts & 0xffff;
			pb->intout[3] = pb->intout[4] = 0;
			break;
		}
		default:
		{
			ret = 0;
			break;
		}
	}
	pb->intout[0] = ret;
	return XAC_DONE;
}

/*
 * appl_find()
 *
 * Ozk: appl_find() may be called by processes not yet called
 * appl_init(). So, it must not depend on client being valid!
 */
unsigned long
XA_appl_find(int lock, struct xa_client *client, AESPB *pb)
{
	const char *name = (const char *)pb->addrin[0];

	CONTROL(0,1,1)

	if (client)
	{
		DIAG((D_appl, client, "appl_find for %s", c_owner(client)));
	} else
	{
		DIAG((D_appl, NULL, "appl_find for non AES process %s (pid %ld)", c_owner(client), p_getpid()));
	}

	/* default to error */
	pb->intout[0] = -1;

	if (name == NULL)
	{
		/* pid of current process */
		if (client)
		{
			pb->intout[0] = client->p->pid;
			DIAG((D_appl, client, "   Mode NULL"));
		}
		else
			pb->intout[0] = p_getpid();
	}
	else
	{
		short lo = (short)((unsigned long)name & 0xffff);
		short hi = (short)((unsigned long)name >> 16);

		switch (hi)
		{
		/* convert mint id -> aes id */
		case -1:
		/* convert aes id -> mint id */
		case -2:
		{
			struct xa_client *cl;

			DIAG((D_appl, client, "   Mode 0xfff%c, convert %i", hi == -1 ? 'f' : 'e', lo));

			Sema_Up(LOCK_CLIENTS);

			FOREACH_CLIENT(cl)
			{
				if (cl->p->pid == lo)
				{
					pb->intout[0] = lo;
					break;
				}
			}

			Sema_Dn(LOCK_CLIENTS);

			break;
		}
		/* pid of topped application */
		case -3:
		{
			struct xa_client *cl;

			DIAG((D_appl, client, "   Mode 0xfffd"));

			cl = focus_owner();
			if (cl)
				pb->intout[0] = cl->p->pid;
			else
				pb->intout[0] = -1;

			break;
		}
		/* search in client list */
		default:
		{
			if (!strcmp(name, "?AGI") || /* XXX hack for eb_model, looking for "AGI?" instead of "?AGI" */!strcmp(name, "AGI?"))
			{
				/* Tell application we understand appl_getinfo()
				 * (Invented by Martin Osieka for his AES extension WINX;
				 * used by MagiC 4, too.)
				 */

				pb->intout[0] = 0; /* OK */
				DIAG((D_appl, client, "   Mode ?AGI"));
			}
			else
			{
				struct xa_client *cl;

				DIAG((D_appl, client, "   Mode search for '%s'", name));
#if WITH_BBL_HELP
				if( cfg.xa_bubble && xa_bubble( lock, bbl_get_status, 0, 0 ) >= bs_closed && !strnicmp( "BUBBLE  ", name, 8 ) )
				{
					pb->intout[0] = C.AESpid;
					break;
				}
#endif

				Sema_Up(LOCK_CLIENTS);

				FOREACH_CLIENT(cl)
				{
					if (strnicmp(cl->proc_name, name, 8) == 0)
					{
						pb->intout[0] = cl->p->pid;
						break;
					}
				}

				Sema_Dn(LOCK_CLIENTS);
			}
			break;
		}
		}
	}

	DIAG((D_appl, client, "   --> %d", pb->intout[0]));
	return XAC_DONE;
}

/*
 * Extended XaAES calls
 */
unsigned long
XA_appl_control(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_client *cl = NULL;
	short pid = pb->intin[0];
	short f = pb->intin[1], ret;

	CONTROL(2,1,1)

	DIAG((D_appl, client, "appl_control for %s", c_owner(client)));

	if (pid == -1) {
		if (!(cl = find_focus(true, NULL, NULL, NULL)))
			cl = get_app_infront();
	} else
		cl = pid2client(pid);

	DIAG((D_appl, client, "  --    on %s, func %d, 0x%lx",
		c_owner(cl), f, pb->addrin[0]));

	/*
	 * note: When extending this switch be sure to update
                *       the appl_getinfo(65) mode corresponding structure
	 *  tip: grep APC_ *
	 */

	ret = 1;
	switch (f)
	{
		case APC_SYSTEM:
		{
			if (cl)
			{
				cl->type = APP_SYSTEM;
			}
			else
				ret = 0;
			break;
		}
		case APC_HIDE:
		{
			if (cl && !(cl->swm_newmsg & NM_INHIBIT_HIDE))
			{
				hide_app(lock, cl);
			}
			else
				ret = 0;
			break;
		}
		case APC_SHOW:
		{
			if (pid == -1)
				unhide_all(lock, cl);
			else
				unhide_app(lock, cl);
			break;
		}
		case APC_TOP:
		{
			if (cl)
			{
				if (any_hidden(lock, cl, NULL))
					unhide_app(lock, cl);
				else
					app_in_front(lock, cl, true, true, true);

				if (cl->type & APP_ACCESSORY)
					send_app_message(lock, NULL, cl, AMQ_NORM, QMF_CHKDUP,
							 AC_OPEN, 0, 0, 0,
							 cl->p->pid, 0, 0, 0);
			}
			else
				ret = 0;
			break;
		}
		case APC_HIDENOT:
		{
			if (cl)
			{
				if (any_hidden(lock, cl, NULL))
					unhide_app(lock, cl);
				hide_other(lock, cl);
			}
			else
				ret = 0;
			break;
		}
		case APC_INFO:
		{
			if (cl)
			{
				short *ii = (short*)pb->addrin[0];

				if (ii)
				{
					*ii = 0;
					if (any_hidden(lock, cl, NULL))
						*ii |= APCI_HIDDEN;
					if (cl->std_menu || cl->nxt_menu)
						*ii |= APCI_HASMBAR;
					if (cl->desktop)
						*ii |= APCI_HASDESK;
				}
			}
			else
				ret = 0;
			break;
		}
		case APC_MENU:
		{
			OBJECT **m = (OBJECT **)pb->addrin[0];

			if (cl && m)
				*m = cl->nxt_menu ? cl->nxt_menu->tree : (cl->std_menu ? cl->std_menu->tree : NULL);
			else if (m)
				*m = NULL;
			break;
		}
		default:
		{
			ret = 0;
		}
	}
	pb->intout[0] = ret;
	return XAC_DONE;
}

#if 0
unsigned long
XA_appl_trecord(int lock, struct xa_client *client, AESPB *pb)
{
	EVNTREC *ap_tbuffer = (void *)pb->addrin[0];
	short ap_trcount = pb->intin[0];

	CONTROL(1,1,1)

	DIAG((D_appl, client, "appl_trecord for %s", c_owner(client)));

	/* error */
	pb->intout[0] = 0;

	return XAC_DONE;
}

unsigned long
XA_appl_tplay(int lock, struct xa_client *client, AESPB *pb)
{
	EVNTREC *ap_tpmem = (void *)pb->addrin[0];
	short ap_tpnum = pb->intin[0];
	short ap_tpscale = pb->intin[1];

	CONTROL(2,1,1)

	DIAG((D_appl, client, "appl_tplay for %s", c_owner(client)));

	/* error */
	pb->intout[0] = 0;

	return XAC_DONE;
}
#endif
