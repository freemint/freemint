/*
 * Filename:     
 * Version:      
 * Author:       Frank Naumann
 * Started:      1999-05
 * Last Updated: 2001-04-17
 * Target O/S:   FreeMiNT 1.16
 * Description:  
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list <mint@fishpool.com>.
 * 
 * Copying:      Copyright 1999, 200, 2001 Frank Naumann <fnaumann@freemint.de>
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
 * 
 */

# include <signal.h>
# include <mint/mintbind.h>
# include <mint/ssystem.h>

# include "global.h"
# include "mgw.h"

/*
 * version
 */

# define VER_MAJOR	0
# define VER_MINOR	30
# define VER_STATUS	b


/*
 * messages
 */

# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS) 
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p Draconis to MiNT-Net gateway " MSG_VERSION " \033q\r\n" \
	"Redirect Daemon\r\n"

# define MSG_GREET	\
	"½ 1999 Jens Heitmann.\r\n" \
	"½ " MSG_BUILDDATE " Frank Naumann.\r\n\r\n"

# define MSG_MINT	\
	"\7This program requires FreeMiNT!\r\n"

# define MSG_ALREADY	\
	"\7There is an active Draconis TCP/IP Cookie!\r\n"

# define MSG_FAILURE	\
	"\7Sorry, driver NOT installed - initialization failed!\r\n\r\n"


/* ------------------
   | Install cookie |
   ------------------ */
static int
install_cookie (void)
{
	long dummy;
	
	if (Ssystem (-1, 0, 0) == -32)
	{
		Cconws (MSG_MINT);
		return 1;
	}
	
	if (Ssystem (S_GETCOOKIE, TCPCOOKIE, &dummy) == 0)
	{
		Cconws (MSG_ALREADY);
		return 1;
	}
	
	if (Ssystem (S_SETCOOKIE, TCPCOOKIE, tcp_cookie) != 0)
	{
		return 1;
	}
	
	return 0;
}

/* ------------------
   | Remove cookie |
   ------------------ */
static void
uninstall_cookie (void)
{
	long old_stk = Super (0L);
	long *jar;
	
	jar = *(long **) JAR;
	if (jar)
	{
		while (*jar)
		{
			if (*jar == TCPCOOKIE)
			{
				*jar++ = FREECOOKIE;
				*jar++ = 0L;
			}
			
			jar += 2;
		}
	}
	
	Super ((void *) old_stk);
}


static void
nothing (long sig)
{
	;
}

static void
end (long sig)
{
	Psigreturn ();
	uninstall_cookie ();
	exit (0);
}

int
main (void)
{
	(void) Dsetdrv ('U'-'A');
	
	if (fork () == 0)
	{
		Cconws (MSG_BOOT);
		Cconws (MSG_GREET);
		
		if (install_cookie ())
		{
			Cconws (MSG_FAILURE);
			exit (1);
		}
		
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
			PMSG pmsg_in;
			PMSG pmsg_out;
			long r;
			
			r = Pmsg (0, MGW_LIBSOCKETCALL, &pmsg_in);
			if (r)
			{
				/* printf ("Pmsg wait fail!\n"); */
				continue;
			}
			
			switch (pmsg_in.msg1)
			{
				case MGW_GETHOSTBYNAME:
				{
					char *s = (char *) pmsg_in.msg2;
					
					pmsg_out.msg1 = MGW_GETHOSTBYNAME;
					pmsg_out.msg2 = (long) gethostbyname (s);
					
					break;
				}
				case MGW_GETHOSTBYADDR:
				{
					long *buf = (long *) pmsg_in.msg2;
					
					pmsg_out.msg1 = MGW_GETHOSTBYADDR;
					pmsg_out.msg2 = (long) gethostbyaddr ((char *) buf[0], buf[1], buf[2]);
					
					break;
				}
				case MGW_GETHOSTNAME:
				{
					long *buf = (long *) pmsg_in.msg2;
					
					pmsg_out.msg1 = MGW_GETHOSTNAME;
					pmsg_out.msg2 = (long) gethostname ((char *) buf[0], buf[1]);
					
					break;
				}
				case MGW_GETSERVBYNAME:
				{
					long *buf = (long *) pmsg_in.msg2;
					
					pmsg_out.msg1 = MGW_GETSERVBYNAME;
					pmsg_out.msg2 = (long) getservbyname ((char *) buf[0], (char *) buf[1]);
					
					break;
				}
				case MGW_GETSERVBYPORT:
				{
					long *buf = (long *) pmsg_in.msg2;
					
					pmsg_out.msg1 = MGW_GETSERVBYPORT;
					pmsg_out.msg2 = (long) getservbyport (buf[0], (char *) buf[1]);
					
					break;
				}
				default:
					continue;
			}
			
			pmsg_out.pid = pmsg_in.pid;
			r = Pmsg (1, 0xffff0000L | pmsg_in.pid, &pmsg_out);
			if (r)
			{
				/* printf ("Pmsg back fail!\n"); */
				continue;
			}
		}
		
		uninstall_cookie ();
	}
	
	return 0;
}
