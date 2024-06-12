/*
 * Filename:     main.c
 * Project:      GlueSTiK
 * 
 * Note:         Please send suggestions, patches or bug reports to
 *               the MiNT mailing list <freemint-discuss@lists.sourceforge.net>
 * 
 * Copying:      Copyright 1999 Frank Naumann <fnaumann@freemint.de>
 * 
 * Portions copyright 1997, 1998, 1999 Scott Bigham <dsb@cs.duke.edu>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# include <stdio.h>
# include <netdb.h>
# include <string.h>

# ifdef __PUREC__
# include <tos.h>
#define C_MagX 0x4D616758L     /* MagX */
#define C_MiNT 0x4D694E54L     /* Mint/MultiTOS */
#define C_STiK 0x5354694BL     /* ST Internet Kit */
# else
# include <mint/osbind.h>
# include <mint/mintbind.h>
# include <mint/basepage.h>
# include <mint/cookie.h>
#endif
#include <mint/ssystem.h>

# include "gs.h"

# include "gs_conf.h"
# include "gs_func.h"
# include "gs_mem.h"
# include "gs_stik.h"
# include "version.h"


/*
 * We need the MiNT definitions here, not any library definitions
 */
#undef SIGHUP
#define SIGHUP 1
#undef SIGINT
#define SIGINT 2
#undef SIGQUIT
#define SIGQUIT 3
#undef SIGABRT
#define SIGABRT 6
#undef SIGALRM
#define SIGALRM 14
#undef SIGTERM
#define SIGTERM 15
#undef SIGTSTP
#define SIGTSTP 18
#undef SIGUSR1
#define SIGUSR1 29
#undef SIGUSR2
#define SIGUSR2 30


# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR)
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p GlueSTiK\277 STiK emulator for MiNTnet/MagiCNet version " \
	MSG_VERSION " \033q\r\n"

/*
 * If you change anything here,
 * also take a look at the TPL trampoline in gs_stik.c
 */
# define MSG_GREET	\
	"Redirect Daemon\r\n" \
	"\275 1996-98 Scott Bigham.\r\n" \
	"\275 2000-2010 Frank Naumann.\r\n" \
	"\275 Mar 22 2002 Vassilis Papathanassiou.\r\n" \
	"\275 2021 Thorsten Otto.\r\n"

# define MSG_ALPHA	\
	"\033p WARNING: This is an unstable version - alpha! \033q\7\r\n"

# define MSG_MINT_OR_MAGIC	\
	"\7This program requires FreeMiNT or MagicNet!\r\n"

# define MSG_ALREADY	\
	"\7There is an active STiK Cookie!\r\n"

# define MSG_FAILURE	\
	"\7Sorry, driver NOT installed - initialization failed!\r\n\r\n"

static int opt_force_install = 0;
short magix;


static void
cleanup (void)
{
	cleanup_config();
	cleanup_mem();
	cleanup_stik_if();
}

/* ------------------
   | Get cookie |
   ------------------ */
static long *get_jar(void)
{
	return (long *)Setexc(0x5a0 / 4, (void (*)(void))-1);
}


static int get_cookie(long id, long *value)
{
	long ret;

	ret = Ssystem (S_GETCOOKIE, id, (long)value);
	if (ret == 0)
		return 1;
	if (ret == -32)
	{
		/* have to do it the hard way */
		long *jar;

		jar = get_jar();
		if (jar != NULL)
		{
			while (jar[0] != 0)
			{
				if (jar[0] == id)
				{
					if (value)
						*value = *++jar;
					return 1;
				}
				jar += 2;
			}
		}
	}
	return 0;
}


/* ------------------
   | Remove cookie |
   ------------------ */
static void
uninstall_cookie (void)
{
	if (Ssystem(S_DELCOOKIE, C_STiK, 0L) == -32)
	{
		/* have to do it the hard way */
		long *jar;

		jar = get_jar();
		while (jar[0] != 0)
		{
			if (jar[0] == C_STiK)
			{
				while (jar[0] != 0)
				{
					jar[0] = jar[2];
					jar[1] = jar[3];
					jar += 2;
				}
				return;
			}
			jar += 2;
		}
	}
}

/* ------------------
   | Install cookie |
   ------------------ */
static int
install_cookie (void)
{
	long dummy;
	long ret;
	long *jar;
	int size;

	if (opt_force_install) {
		uninstall_cookie();
	}
	if (get_cookie(C_STiK, &dummy))
	{
		(void) Cconws (MSG_ALREADY);
		return 1;
	}
	
	ret = Ssystem (S_SETCOOKIE, C_STiK, (long) &stik_driver);
	if (ret == 0)
		return 0;
	if (ret != -32)
		return 1;

	/* have to do it the hard way */
	jar = get_jar();
	if (jar == NULL)
		return 1;
	size = 1;
	while (jar[0] != 0)
	{
		jar += 2;
		size++;
	}
	if ((long)size < jar[1])
	{
		jar[2] = 0;
		jar[3] = jar[1];
		jar[0] = C_STiK;
		jar[1] = (long) &stik_driver;
		return 0;
	}

	/* Note: no attempt made here to enlarge the cookiejar */
	return 1;
}


static void __CDECL
nothing (long sig)
{
	(void)sig;
}

static void __CDECL
end (long sig)
{
	(void)sig;
	Psigreturn ();
	
	cleanup ();
	uninstall_cookie ();
	
	exit (0);
}


static void 
parse_cmdline(void)
{
	BASEPAGE *bp = _base;
	unsigned char c;
	
	c = bp->p_cmdlin[0];
	if (c == 0 || c >= 0x7f)
		return;

  	if (!strncmp(bp->p_cmdlin + 1, "--force", 7) || !strncmp(bp->p_cmdlin + 1, "-f", 2))
		opt_force_install = 1; 
}

int
main (void)
{
	long r;
	long value;
	
	parse_cmdline();

	(void) Cconws (MSG_BOOT);
	(void) Cconws (MSG_GREET);
# ifdef ALPHA
	(void) Cconws (MSG_ALPHA);
# endif
	(void) Cconws ("\r\n");

	if (get_cookie(C_MiNT, &value))
	{
		magix = 0;
	} else if (get_cookie(C_MagX, &value))
	{
		magix = 1;
	} else
	{
		(void) Cconws(MSG_MINT_OR_MAGIC);
		return 1;
	}

	if (!magix)
	{
		/* for MiNT, daemonize */
		r = Pfork();
		if (r < 0)
		{
			/* fork failed */
			return 1;
		}
		if (r > 0)
		{
			/* parent can exit */
			return 0;
		}
	}

	if (init_stik_if() == 0 ||
		load_config_file() == 0 ||
		init_mem() == 0 ||
		install_cookie() != 0)
	{
		(void) Cconws (MSG_FAILURE);
		
		cleanup ();
		return 1;
	}
	
	if (magix)
	{
		/* for MagiC, run as TSR */
		Ptermres(-1, 0);
		/* not reached */
		return 1;
	}

	/*
	 * daemon process for MiNT.
	 * All we do here is to wait for gethostbyname requests
	 */
	Psignal (SIGALRM, nothing);
	Psignal (SIGTERM, end);
	Psignal (SIGQUIT, end);
	Psignal (SIGHUP,  nothing);
	Psignal (SIGTSTP, nothing);
	Psignal (SIGINT,  nothing);
	Psignal (SIGABRT, nothing);
	Psignal (SIGUSR1, nothing);
	Psignal (SIGUSR2, nothing);
	
	for (;;)
	{
		PMSG pmsg;
		
		r = Pmsg (0, GS_GETHOSTBYNAME, &pmsg);
		if (r)
		{
			/* printf ("Pmsg wait fail!\n"); */
			break;
		}
		
		switch ((int)pmsg.msg1)
		{
			case 1:
				pmsg.msg2 = (long) gethostbyname ((char *) pmsg.msg2);
				pmsg.msg1 = h_errno;
				break;
			case 2:
				pmsg.msg2 = (long) malloc (pmsg.msg2);
				break;
			case 3:
				free ((void *) pmsg.msg2);
				break;
			case 4:
				/* pmsg.msg2 = realloc ((void *) pmsg.msg2); */
				break;
			case 5:
				
				break;
		}
		
		r = Pmsg (1, 0xffff0000L | pmsg.pid, &pmsg);
		if (r)
		{
			/* printf ("Pmsg back fail!\n"); */
		}
	}
	
	cleanup ();
	uninstall_cookie ();
	
	return 0;
}
