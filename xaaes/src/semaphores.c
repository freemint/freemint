
#include <mint/mintbind.h>

#define __KERNEL__
#include <errno.h>

#include "display.h"
#include "xa_defs.h"

#include "semaphores.h"


static void
create_semaphore(int loghandle, long value, const char *name, long *error)
{
	long ret;

	ret = Psemaphore(0, value, 0);
	if (ret)
	{
		fdisplay(loghandle, true, "Psemaphore(0, %s) -> %ld\n", name, ret);
		*error = ret;
	}
}

int
create_semaphores(int loghandle)
{
	long error = 0;

	DIAGS(("Creating Semaphores\n"));

	create_semaphore(loghandle, APPL_INIT_SEMA,    "APPL_INIT_SEMA",    &error);
//	create_semaphore(loghandle, TRAP_HANDLER_SEMA, "TRAP_HANDLER_SEMA", &error);
	create_semaphore(loghandle, ROOT_SEMA,         "ROOT_SEMA",         &error);
	create_semaphore(loghandle, CLIENTS_SEMA,      "CLIENTS_SEMA",      &error);
	create_semaphore(loghandle, UPDATE_LOCK,       "UPDATE_LOCK",       &error);
	create_semaphore(loghandle, MOUSE_LOCK,        "MOUSE_LOCK",        &error);
	create_semaphore(loghandle, FSELECT_SEMA,      "FSELECT_SEMA",      &error);
	create_semaphore(loghandle, ENV_SEMA,          "ENV_SEMA",          &error);
	create_semaphore(loghandle, PENDING_SEMA,      "PENDING_SEMA",      &error);

	/* there was an error during semaphore creation */
	if (error)
	{
		fdisplay(loghandle, true, "XaAES ERROR: can't create all semaphores\n");
		destroy_semaphores(loghandle);
	}

	return error;
}

static void
destroy_semaphore(int loghandle, long value, const char *name)
{
	long ret;

	ret = Psemaphore(1, value, 0);
	/*
	 * E_OK -> deleted
	 * EACCES -> semaphore locked (or is free on older kernels)
	 * EBADARG -> no such semaphore
	 */
	if (ret == EACCES)
	{
		fdisplay(loghandle, true, "deleting Psemaphore(%s) -> ", name);
		fdisplay(loghandle, true, "locked or free; try to aquire\n");

		ret = Psemaphore(2, value, 0);
		if (ret == 0)
		{
			ret = Psemaphore(1, value, 0);
			if (ret != 0)
			{
				fdisplay(loghandle, true, "deleting Psemaphore(%s) -> ", name);
				fdisplay(loghandle, true, "aquired but not deleted (%ld)???\n", ret);
			}
			else
				goto ok;
		}
		else
		{
			fdisplay(loghandle, true, "deleting Psemaphore(%s) -> ", name);
			fdisplay(loghandle, true, "can't aquire, giving up (%ld)\n", ret);
		}
	}
	else if (ret)
	{
		if (ret == EBADARG)
		{
			fdisplay(loghandle, true, "deleting Psemaphore(%s) -> ", name);
			fdisplay(loghandle, true, "don't exist\n");
		}
		else
		{
			fdisplay(loghandle, true, "deleting Psemaphore(%s) -> ", name);
			fdisplay(loghandle, true, "unknown error %ld, ignored\n", ret);
		}
	}
	else
	{
ok:
		fdisplay(loghandle, true, "deleting Psemaphore(%s) -> ", name);
		fdisplay(loghandle, true, "OK\n");
	}
}

void
destroy_semaphores(int loghandle)
{
	DIAGS(("Deleting semaphores\n"));

	destroy_semaphore(loghandle, APPL_INIT_SEMA, "APPL_INIT_SEMA");
//	destroy_semaphore(loghandle, TRAP_HANDLER_SEMA, "TRAP_HANDLER_SEMA");
	destroy_semaphore(loghandle, ROOT_SEMA, "ROOT_SEMA");
	destroy_semaphore(loghandle, CLIENTS_SEMA, "CLIENTS_SEMA");
	destroy_semaphore(loghandle, UPDATE_LOCK, "UPDATE_LOCK");
	destroy_semaphore(loghandle, MOUSE_LOCK, "MOUSE_LOCK");
	destroy_semaphore(loghandle, FSELECT_SEMA, "FSELECT_SEMA");
	destroy_semaphore(loghandle, ENV_SEMA, "ENV_SEMA");
	destroy_semaphore(loghandle, PENDING_SEMA, "PENDING_SEMA");
}
