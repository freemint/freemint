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

#include "semaphores.h"
#include "xa_types.h"
#include "xa_global.h"
#include "debug.h"


struct ressource_semaphore
{
	struct xa_client *client;
	unsigned short counter;
	unsigned short sleepers;
};

static void
ressource_semaphore_lock(struct ressource_semaphore *s, struct xa_client *client)
{
	if (s->client && s->client != client)
	{
		s->sleepers++;

		client->sleeplock = (long)s;
		client->sleepqueue = WAIT_Q;
		while (s->client)
			sleep(WAIT_Q, (long)s);

		client->sleeplock = 0;

		s->sleepers--;
	}

	/* enter semaphore */
	s->client = client;

	/* increment */
	s->counter++;
}

static int
ressource_semaphore_rel(struct ressource_semaphore *s, struct xa_client *client)
{
	/*
	 * Ozk: It seems that applications sometimes call wind_update() with
	 * END_UPDATE and/or END_MOUSE one or two times more than necessary
	 * just to be sure locks are released. This means that AES should permit
	 * and report success with calls to unlock mouse and screen as long as
	 * they are not owned by another process. A free lock dont belong anywhere, so...
	*/
	/* check for correct usage */
	//assert(s->client && s->client == client);
	if (!s->client)
	{
		DIAG((D_sema, client, "Releasing unused lock %s", client->name));
		return 1;
	}
	if (s->client != client)
	{
		DIAG((D_sema, client, "%s try releasing lock owned by %s", client->name, s->client->name));
		return 0;
	}

	/* decrement semaphore */
	s->counter--;

	/* if completed wakup any waiter */
	if (!s->counter)
	{
		if (s->sleepers)
			wake(WAIT_Q, (long)s);

		s->client = NULL;

		/* completed, semaphore free */
		return 1;
	}

	/* not completed */
	return 0;
}

static void
ressource_semaphore_free(struct ressource_semaphore *s)
{
	s->client = NULL;
	s->counter = 0;

	if (s->sleepers)
		wake(WAIT_Q, (long)s);
}

static int
ressource_semaphore_try(struct ressource_semaphore *s, struct xa_client *client)
{
	if (s->client && s->client != client)
	{
		/* semaphore locked */
		return 0;
	}

	/* enter semaphore */
	s->client = client;

	/* increment */
	s->counter++;

	return 1;
}

static struct ressource_semaphore update_lock;	/* wind_update() locks */
static struct ressource_semaphore mouse_lock;

struct xa_client *
update_locked(void)
{
	return update_lock.client;
}

struct xa_client *
mouse_locked(void)
{
	return mouse_lock.client;
}

void
free_update_lock(void)
{
	ressource_semaphore_free(&update_lock);
}

void
free_mouse_lock(void)
{
	ressource_semaphore_free(&mouse_lock);
}

bool
lock_screen(struct xa_client *client, bool try, short *ret, int which)
{

	DIAG((D_sema, NULL, "[%d]lock_screen for (%d)%s state: %d for (%d)%s, try: %d",
		which, client->p->pid, c_owner(client), update_lock.counter,
		update_lock.client ? update_lock.client->p->pid : -1,
		update_lock.client ? update_lock.client->name : "", try));

	if (try)
	{
		if (ressource_semaphore_try(&mouse_lock, client))
		{
			if (ressource_semaphore_try(&update_lock, client))
				return true;
			else
				ressource_semaphore_free(&mouse_lock);
		}

		if (ret)
			*ret = 0;

		return false;
	}

	ressource_semaphore_lock(&mouse_lock, client);
	ressource_semaphore_lock(&update_lock, client);
	return true;
}

bool
unlock_screen(struct xa_client *client, int which)
{
	bool r = false;

	DIAG((D_sema, NULL, "[%d]unlock_screen for (%d)%s state: %d for (%d)%s, try: %d",
		which, client->p->pid, c_owner(client), update_lock.counter,
		update_lock.client ? update_lock.client->p->pid : -1,
		update_lock.client ? update_lock.client->name : "", try));

	if (update_lock.client == client)
	{
		r = ressource_semaphore_rel(&update_lock, client);
		r = ressource_semaphore_rel(&mouse_lock, client);
	}
	else
	{
		DIAG((D_sema, NULL, "unlock_screen from %d without lock_screen!", locker->p->pid));
	}

	return r;
}

bool
lock_mouse(struct xa_client *client, bool try, short *ret, int which)
{
	DIAG((D_sema, NULL, "[%d]lock_mouse for (%d)%s state: %d for (%d)%s, try: %d",
		which, client->p->pid, c_owner(client), update_lock.counter,
		mouse_lock.client ? mouse_lock.client->p->pid : -1,
		mouse_lock.client ? mouse_lock.client->name : "", try));

	if (try)
	{
		if (ressource_semaphore_try(&mouse_lock, client))
			return true;

		if (ret)
			*ret = 0;

		return false;
	}

	ressource_semaphore_lock(&mouse_lock, client);
	return true;
}

bool
unlock_mouse(struct xa_client *client, int which)
{
	bool r = false;

	DIAG((D_sema, NULL, "[%d]unlock_mouse for (%d)%s state: %d for (%d)%s, try: %d",
		which, client->p->pid, c_owner(client), update_lock.counter,
		mouse_lock.client ? mouse_lock.client->p->pid : -1,
		mouse_lock.client ? mouse_lock.client->name : "", try));



	if (!update_lock.client || update_lock.client == client)
		r = ressource_semaphore_rel(&mouse_lock, client);
	else
	{
		DIAG((D_sema, NULL, "unlock_mouse from %d without lock_screen!", locker->p->pid));
		r = ressource_semaphore_rel(&mouse_lock, client);
	}

	return r;
}

// old stuff
#if 0

static void
create_semaphore(struct file *log, long value, const char *name, long *error)
{
	long ret;

	ret = p_semaphore(0, value, 0);
	if (ret)
	{
		fdisplay(log, "Psemaphore(0, %s) -> %ld", name, ret);
		*error = ret;
	}
}

int
create_semaphores(struct file *log)
{
	long error = 0;

	DIAGS(("Creating Semaphores"));

	create_semaphore(log, APPL_INIT_SEMA,    "APPL_INIT_SEMA",    &error);
	create_semaphore(log, ROOT_SEMA,         "ROOT_SEMA",         &error);
	create_semaphore(log, CLIENTS_SEMA,      "CLIENTS_SEMA",      &error);
	create_semaphore(log, UPDATE_LOCK,       "UPDATE_LOCK",       &error);
	create_semaphore(log, MOUSE_LOCK,        "MOUSE_LOCK",        &error);
	create_semaphore(log, FSELECT_SEMA,      "FSELECT_SEMA",      &error);
	create_semaphore(log, ENV_SEMA,          "ENV_SEMA",          &error);
	create_semaphore(log, PENDING_SEMA,      "PENDING_SEMA",      &error);

	/* there was an error during semaphore creation */
	if (error)
	{
		fdisplay(log, "XaAES ERROR: can't create all semaphores");
		destroy_semaphores(log);
	}

	return error;
}

static void
destroy_semaphore(struct file *log, long value, const char *name)
{
	long ret;

	ret = p_semaphore(1, value, 0);
	/*
	 * E_OK -> deleted
	 * EACCES -> semaphore locked (or is free on older kernels)
	 * EBADARG -> no such semaphore
	 */
	if (ret == EACCES)
	{
		fdisplay(log, "deleting Psemaphore(%s) -> ", name);
		fdisplay(log, "locked or free; try to aquire");

		ret = p_semaphore(2, value, 0);
		if (ret == 0)
		{
			ret = p_semaphore(1, value, 0);
			if (ret != 0)
			{
				fdisplay(log, "deleting Psemaphore(%s) -> ", name);
				fdisplay(log, "aquired but not deleted (%ld)???", ret);
			}
			else
				goto ok;
		}
		else
		{
			fdisplay(log, "deleting Psemaphore(%s) -> ", name);
			fdisplay(log, "can't aquire, giving up (%ld)", ret);
		}
	}
	else if (ret)
	{
		if (ret == EBADARG)
		{
			fdisplay(log, "deleting Psemaphore(%s) -> ", name);
			fdisplay(log, "don't exist");
		}
		else
		{
			fdisplay(log, "deleting Psemaphore(%s) -> ", name);
			fdisplay(log, "unknown error %ld, ignored", ret);
		}
	}
	else
	{
ok:
		fdisplay(log, "deleting Psemaphore(%s) -> ", name);
		fdisplay(log, "OK");
	}
}

void
destroy_semaphores(struct file *log)
{
	DIAGS(("Deleting semaphores"));

	destroy_semaphore(log, APPL_INIT_SEMA, "APPL_INIT_SEMA");
	destroy_semaphore(log, ROOT_SEMA, "ROOT_SEMA");
	destroy_semaphore(log, CLIENTS_SEMA, "CLIENTS_SEMA");
	destroy_semaphore(log, UPDATE_LOCK, "UPDATE_LOCK");
	destroy_semaphore(log, MOUSE_LOCK, "MOUSE_LOCK");
	destroy_semaphore(log, FSELECT_SEMA, "FSELECT_SEMA");
	destroy_semaphore(log, ENV_SEMA, "ENV_SEMA");
	destroy_semaphore(log, PENDING_SEMA, "PENDING_SEMA");
}

#endif
