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

#include "semaphores.h"
#include "xa_types.h"
#include "xa_global.h"
#include "debug.h"


struct ressource_semaphore
{
	struct proc *proc;
	unsigned short counter;
	unsigned short sleepers;
};

static void
ressource_semaphore_lock(struct ressource_semaphore *s, struct proc *proc)
{

	if (s->proc && s->proc != proc)
	{
		s->sleepers++;

		while (s->proc)
			sleep(WAIT_Q, (long)s);

		s->sleepers--;
	}

	/* enter semaphore */
	s->proc = proc;

	/* increment */
	s->counter++;
}

static int
ressource_semaphore_rel(struct ressource_semaphore *s, struct proc *proc)
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
	if (!s->proc)
	{
		DIAG((D_sema, NULL, "Releasing unused lock %s", proc->name));
		return 1;
	}
	if (s->proc != proc)
	{
		DIAG((D_sema, NULL, "%s try releasing lock owned by %s", proc->name, s->proc->name));
		return 0;
	}

	/* decrement semaphore */
	s->counter--;

	/* if completed wakup any waiter */
	if (!s->counter)
	{
		if (s->sleepers)
			wake(WAIT_Q, (long)s);

		s->proc = NULL;

		/* completed, semaphore free */
		return 1;
	}

	/* not completed */
	return 0;
}

static void
ressource_semaphore_free(struct ressource_semaphore *s)
{
	s->proc = NULL;
	s->counter = 0;

	if (s->sleepers)
		wake(WAIT_Q, (long)s);
}

static int
ressource_semaphore_try(struct ressource_semaphore *s, struct proc *proc)
{
	if (s->proc && s->proc != proc)
	{
		/* semaphore locked */
		return 0;
	}

	/* enter semaphore */
	s->proc = proc;

	/* increment */
	s->counter++;

	return 1;
}

static struct ressource_semaphore update_lock;	/* wind_update() locks */
static struct ressource_semaphore mouse_lock;
static struct ressource_semaphore ms_lock;
struct proc *
update_locked(void)
{
	return update_lock.proc;
}

struct proc *
mouse_locked(void)
{
	return mouse_lock.proc;
}
struct proc *
menustruct_locked(void)
{
	return ms_lock.proc;
}

static struct xa_client *
get_lock_client(struct ressource_semaphore *s)
{
	struct xa_client *ret = CLIENT_LIST_START;

	while (ret)
	{
		if (ret->p == s->proc)
			break;
		ret =  NEXT_CLIENT(ret);
	}
	return ret;
}
struct xa_client *
get_update_locker(void)
{
	return get_lock_client(&update_lock);
}
struct xa_client *
get_mouse_locker(void)
{
	return get_lock_client(&mouse_lock);
}
#if INCLUDE_UNUSED
struct xa_client *
get_menustruct_locker(void)
{
	return get_lock_client(&ms_lock);
}
#endif
void
free_update_lock(void)
{
	free_mouse_lock();
	ressource_semaphore_free(&update_lock);
}

void
free_mouse_lock(void)
{
	ressource_semaphore_free(&mouse_lock);
}

#if ALT_CTRL_APP_OPS
void
free_menustruct_lock(void)
{
	//free_update_lock();
	ressource_semaphore_free(&ms_lock);
}
#endif
bool
lock_screen(struct proc *proc, bool try)
{
	DIAG((D_sema, NULL, "lock_screen for (%d)%s state: %d for (%d)%s, try: %d",
		proc->pid, proc->name, update_lock.counter,
		update_lock.proc ? update_lock.proc->pid : -1,
		update_lock.proc ? update_lock.proc->name : "", try));

	if (try)
	{
		if (ressource_semaphore_try(&mouse_lock, proc))
		{
			if (ressource_semaphore_try(&update_lock, proc))
				return true;
			else
				ressource_semaphore_free(&mouse_lock);
		}

// 		if (ret)
// 			*ret = 0;

		return false;
	}

	ressource_semaphore_lock(&mouse_lock, proc);
	ressource_semaphore_lock(&update_lock, proc);
	return true;
}

bool
unlock_screen(struct proc *proc)
{
	bool r = false;

	DIAG((D_sema, NULL, "unlock_screen for (%d)%s state: %d for (%d)%s",
		proc->pid, proc->name, update_lock.counter,
		update_lock.proc ? update_lock.proc->pid : -1,
		update_lock.proc ? update_lock.proc->name : ""));

	if (update_lock.proc == proc)
	{
		r = ressource_semaphore_rel(&update_lock, proc);
	}
	if (mouse_lock.proc == proc)
	{
		r |= ressource_semaphore_rel(&mouse_lock, proc);
	}
	if( !r )
	{
		DIAG((D_sema, NULL, "unlock_screen from %d without lock_screen!", proc->pid));
	}

	return r;
}

bool
lock_mouse(struct proc *proc, bool try)
{
	DIAG((D_sema, NULL, "lock_mouse for (%d)%s state: %d for (%d)%s, try: %d",
		proc->pid, proc->name, mouse_lock.counter,
		mouse_lock.proc ? mouse_lock.proc->pid : -1,
		mouse_lock.proc ? mouse_lock.proc->name : "", try));

	if (try)
	{
		if (ressource_semaphore_try(&mouse_lock, proc))
			return true;

		return false;
	}

	ressource_semaphore_lock(&mouse_lock, proc);
	return true;
}

bool
unlock_mouse(struct proc *proc)
{
	bool r = false;

	DIAG((D_sema, NULL, "unlock_mouse for (%d)%s state: %d for (%d)%s",
		proc->pid, proc->name, mouse_lock.counter,
		mouse_lock.proc ? mouse_lock.proc->pid : -1,
		mouse_lock.proc ? mouse_lock.proc->name : ""));



	if (!update_lock.proc || update_lock.proc == proc)
		r = ressource_semaphore_rel(&mouse_lock, proc);
	else
	{
		DIAG((D_sema, NULL, "unlock_mouse from %d without lock_screen!", proc->pid));
		r = ressource_semaphore_rel(&mouse_lock, proc);
	}

	return r;
}

bool
lock_menustruct(struct proc *proc, bool try)
{
	DIAG((D_sema, NULL, "lock_menustruct for (%d)%s state: %d for (%d)%s, try: %d",
		proc->pid, proc->name, ms_lock.counter,
		ms_lock.proc ? ms_lock.proc->pid : -1,
		ms_lock.proc ? ms_lock.proc->name : "", try));

	if (try)
	{
		return ressource_semaphore_try(&ms_lock, proc);
	}

	//lock_screen(proc, false);
	ressource_semaphore_lock(&ms_lock, proc);
	return true;
}

bool
unlock_menustruct(struct proc *proc)
{
	bool r = false;

	DIAG((D_sema, NULL, "unlock_menustruct for (%d)%s state: %d for (%d)%s",
		proc->pid, proc->name, ms_lock.counter,
		ms_lock.proc ? ms_lock.proc->pid : -1,
		ms_lock.proc ? ms_lock.proc->name : ""));


	if (ms_lock.proc == proc)
	{
		r = ressource_semaphore_rel(&ms_lock, proc);
		//unlock_screen(proc);
	}
	else
	{
		DIAG((D_sema, NULL, "unlock_menustruct from (%d)%s without ownership!", proc->pid, proc->name));
	}

	return r;
}
