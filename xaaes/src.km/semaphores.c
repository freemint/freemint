/*
 * $Id$
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann
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

		while (s->client)
			sleep(WAIT_Q, (long)s);

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
	/* check for correct usage */
	assert(s->client && s->client == client);

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
	DIAG((D_sema, NULL, "[%d]lock_screen for %s, state: %d for %d",
		which, c_owner(client), update_lock.counter,
		update_lock.client ? update_lock.client->p->pid : -1));

	if (try)
	{
		if (ressource_semaphore_try(&update_lock, client))
			return true;

		if (ret)
			*ret = 0;

		return false;
	}

	ressource_semaphore_lock(&update_lock, client);
	return true;
}

bool
unlock_screen(struct xa_client *client, int which)
{
	bool r = false;

	DIAG((D_sema, NULL, "[%d]unlock_screen for %s, state: %d for %d",
		which, c_owner(client), update_lock.counter,
		update_lock.client ? update_lock.client->p->pid : -1));

	if (update_lock.client == client)
		r = ressource_semaphore_rel(&update_lock, client);
	else
		ALERT(("unlock_screen from %d without lock_screen!", client->p->pid));

	return r;
}

bool
lock_mouse(struct xa_client *client, bool try, short *ret, int which)
{
	DIAG((D_sema, NULL, "[%d]lock_mouse for %s, state: %d for %d",
		which, c_owner(client), mouse_lock.counter,
		mouse_lock.client ? mouse_lock.client->p->pid : -1));

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

	DIAG((D_sema, NULL, "[%d]unlock_mouse for %s, state: %d for %d",
		which, c_owner(client), mouse_lock.counter,
		mouse_lock.client ? mouse_lock.client->p->pid : -1));

	if (update_lock.client == client)
		r = ressource_semaphore_rel(&mouse_lock, client);
	else
		ALERT(("unlock_mouse from %d without lock_screen!", client->p->pid));

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
