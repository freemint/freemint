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
#include <signal.h>

#include "xa_types.h"
#include "xa_global.h" 

#include "util.h"
#include "xa_clnt.h"
#include "xa_shel.h"


XA_CLIENT *
xa_fork_exec(short x_mode, XSHELW *xsh, char *fname, char *tail)
{
	XSHELW x_shell = *xsh;
	XA_CLIENT *new;
	Path defdir,shelldir;				/* HR 060901; xshell defdir */
	int defdrive = -1;
	int child = 0;
	long oldmask;

#if GENERATE_DIAGS
	{
		char ppp[128];

		*ppp = 0;

		if (x_mode & SW_PSETLIMIT)
			/* Limit child's memory? */
			strcat(ppp,"SW_PSETLIMIT,");
		if (x_mode & SW_UID)
			/* XaAES extension - set user id */
			strcat(ppp, "SW_UID,");
		if (x_mode & SW_GID)
			/* XaAES extension - set group id */
			strcat(ppp, "SW_GID,");
		if (x_mode & SW_PRENICE)
			/* Was the child spawned as 'nice'? */
			strcat(ppp, "SW_PRENICE,");
		if (x_mode & SW_ENVIRON)
			strcat(ppp, "SW_ENVIRON");
		if (x_mode & SW_DEFDIR)
			/* Was the a default dir passed? */
			strcat(ppp, "SW_DEFDIR");

		DIAG((D_shel, NULL, "Extended shell_write bits: %s\n", ppp));

		if (x_mode & SW_DEFDIR)
			/* Was the a default dir passed? */
			DIAG((D_shel, NULL, "def dir: '%s'\n", x_shell.defdir));
	}

	if ((x_mode & SW_ENVIRON))
		display_env(&x_shell.env, 1);

#endif

	/* Make it local (for memory protection) */
	if (x_mode & SW_DEFDIR)
		strcpy(shelldir, x_shell.defdir);
	else
		*shelldir = 0;

#if MOUSE_KLUDGE
	/* Kludge to get round a bug in MiNT (or moose?) */
	if (C.MOUSE_dev)
		Fclose(C.MOUSE_dev);
#endif

	DIAG((D_shel, NULL, "Pexec(200) '%s', tail: %ld(%lx) %d '%s'\n",
		fname, tail, tail, *tail, tail + 1));
	DIAGS(("Normal Pexec\n"));
#if GENERATE_DIAGS
	display_env(C.strings, 1);
#endif

	/* block SIGCHLD until we setup our data structures */
	oldmask = Psigblock(1UL << SIGCHLD);

	/* Fork off a new process */
	child = Pvfork();
	if (!child)
	{
		/* In child here */
		long rep;

		/* restore old sigmask */
		Psigsetmask(oldmask);

#if GENERATE_DIAGS
      		/*  debug_file is not a handle of the child ? */
		if (D.debug_file > 0)
			/* Redirect console output */
			Fforce(1, D.debug_file);
#endif

#if GENERATE_DIAGS
		{
			Path havedir;
			int drv;

			DIAG((D_shel, NULL, "*****   in Fork for '%s' *****\n", fname));
			drv = Dgetdrv();
			Dgetpath(havedir, 0);
			DIAG((D_shel, NULL, "havedir(0) '%s'\n", havedir));
			Dgetpath(havedir, drv + 1);
			DIAG((D_shel, NULL, "havedir(%d) '%s'\n", drv, havedir));
			DIAG((D_shel, NULL, "Parent = %d\n", Pgetppid()));
		}
#endif

		if (x_mode)
		{
			/* Limit child's memory? */
			if (x_mode & SW_PSETLIMIT)
				Psetlimit(2, x_shell.psetlimit);

			/* XaAES extension - set user id */
			if (x_mode & SW_UID)
				(void) Psetuid(x_shell.uid);

			/* XaAES extension - set group id */
			if (x_mode & SW_GID)
				(void) Psetgid(x_shell.gid);

			/* Was the child spawned as 'nice'? */
			if ((x_mode & SW_PRENICE) && x_shell.prenice)
				(void) Pnice(x_shell.prenice);

			if ((x_mode & SW_DEFDIR) && *shelldir)
			{
				/* no name, set */
				defdrive = drive_and_path(shelldir, defdir, NULL, false, true);

				DIAG((D_shel, NULL, "x_mode drive: %d\n", defdrive));
				DIAG((D_shel, NULL, "       path: '%s'\n", defdir));
			}
		}

		/* It took me a few hours to realize that C.strings must be dereferenced,
		 * to get to the original type of string (which I call a 'superstring'
		 */

		/* Run the new client */
		rep = Pexec(200, fname, tail,
			    (x_mode & SW_ENVIRON) ? x_shell.env : *C.strings);

		DIAG((D_shel, NULL, "Pexec replied %ld(%lx)\n", rep, rep));

		Pterm(rep);
		/* If we Pterm, we've failed to execute */
		/* HR: this approach passes the Pexec reply to the SIGCHILD code in signal.c */
	}

	new = NewClient(child);
	if (new)
	{
		/* Was the a default dir passed? */
		if ((x_mode & SW_DEFDIR) && *shelldir)
		{
			new->xdrive = defdrive;
			strcpy(new->xpath, defdir);
		}
	}

	/* restore old sigmask */
	Psigsetmask(oldmask);

	return new;
}
