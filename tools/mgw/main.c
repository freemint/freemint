/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1999, 2000, 2001 Frank Naumann <fnaumann@freemint.de>
 * Copyright 1999 Jens Heitmann
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
 * 
 * Author: Frank Naumann, Jens Heitmann
 * Started: 1999-05
 * 
 * please send suggestions or bug reports to me or
 * the MiNT mailing list
 * 
 */

# include <signal.h>
# include <ctype.h>
# include <arpa/inet.h>
# include <mint/mintbind.h>
# include <mint/ssystem.h>

# include "global.h"
# include "mgw.h"

/*
 * version
 */

# define VER_MAJOR	0
# define VER_MINOR	33
# define VER_STATUS	b4


/*
 * messages
 */

# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS) 
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p Draconis to MiNT-Net gateway " MSG_VERSION " \033q\r\n" \
	"Redirect Daemon\r\n"

# define MSG_GREET	\
	"½ 2000-2010 Jens Heitmann.\r\n" \
	"½ 2000-2010 Frank Naumann.\r\n\r\n"

# define MSG_MINT	\
	"\7This program requires FreeMiNT!\r\n"

# define MSG_ALREADY	\
	"\7There is an active Draconis TCP/IP Cookie!\r\n"

# define MSG_FAILURE	\
	"\7Sorry, driver NOT installed - initialization failed!\r\n\r\n"

/* Prototypes */
void getfunc_unlock (void);
void disable_old_hostbyX(void);		/* located in mgw.c */
void enable_old_hostbyX(void);		/* located in mgw.c */
void disable_hostbyX(void);			/* located in mgw.c */
void enable_hostbyX(void);				/* located in mgw.c */

long compatibility = 0L;					/* 0 = None */

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
# ifndef S_DELCOOKIE
# define S_DELCOOKIE	26
# endif

	Ssystem(S_DELCOOKIE, TCPCOOKIE, 0L);
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

/* ----------------------------------------------
   | Parse cmdline and use it for this instance |
   ---------------------------------------------- */
static void 
parse_cmdline(int argc, char *argv[])
{
int i;

for (i = 1; i < argc; i++)
	{
	if (!strncmp(argv[i], "--dns", 5) && isdigit(argv[i][5]) && 
			argv[i][6] == '=')
		{
		extern ulong dns1, dns2;
		int no = argv[i][5] - '0';
		if (no == 1)
		  dns1 = inet_addr(argv[i] + 7);
		else
		  dns2 = inet_addr(argv[i] + 7);
		}
	else if (!strcmp(argv[i], "--disable-old-system-resolve") || !strcmp(argv[i], "-O"))
		{
		disable_old_hostbyX();
		}
	else if (!strcmp(argv[i], "--disable-system-resolve") || !strcmp(argv[i], "-R"))
		{
		disable_hostbyX();
		}
	else if (!strcmp(argv[i], "--enable-old-system-resolve") || !strcmp(argv[i], "-o"))
		{
		enable_old_hostbyX();
		}
	else if (!strcmp(argv[i], "--enable-system-resolve") || !strcmp(argv[i], "-r"))
		{
		enable_hostbyX();
		}
	else if (!strncmp(argv[i], "--compatibility=", 14) || 
					 !strncmp(argv[i], "-c=", 3) || 
					 !strcmp(argv[i], "--compatibility") ||
					 !strcmp(argv[i], "-c"))
		{
		char *ptr;
		
		if (argv[i][1] == '-')
			ptr = argv[i] + 13;
		else
		  ptr = argv[i] + 2;
		  
		if (*ptr == '=')
			{
			ptr++;
			compatibility = atol(ptr) << 16L;
			ptr = strchr(ptr, '.');
			if (ptr)
				compatibility |= atol(ptr + 1);
			}
		else
			compatibility = 0x00010007;
		}
	else if (!strcmp(argv[i], "--nocompatibility") ||
					 !strcmp(argv[i], "-n"))
		compatibility = 0;
	else
		printf("%s: Unknown option '%s'\n", argv[0], argv[i]);
	}
}

/* --------------------------------------------------
   | Send cmdline to the currently running instance |
   -------------------------------------------------- */
static void 
send_cmdline(int argc, char *argv[])
{
PMSG pmsg;
char **buf;
int i;

buf = (char **)malloc((argc + 1) * sizeof(char *));
for (i = 0; i < argc; i++)
	buf[i] = argv[i];
buf[argc] = 0L;

pmsg.msg1 = MGW_NEWCMDLINE;
pmsg.msg2 = (long) buf;
	
Pmsg (2, MGW_LIBSOCKETCALL, &pmsg);
}

int
main (int argc, char *argv[])
{
	long dummy;

	(void) Dsetdrv ('U'-'A');
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	  	{
		printf("Usage: %s (requires Kernel >=1.16)\n\n"
			" [--dns1=inet_addr]\n"
			"\tSet the ip address of the primary name server\n"
			" [--dns2=inet_addr]\n"
			"\tSet the ip address of the secondary name server\n"
			"\t(Both of the above options are only useful,\n"
			"\t if system-resolve is switched off. It may be\n"
			"\t set for dial-up connections in /etc/ppp/ip-up\n"
			" [--disable-old-system-resolve|-O]\n"
		        "\tDisable MiNT function calls for DRACONIS \n"
			"\tapplications <v1.8, bypass MiNT DNS calls.\n"
			" [--disable-system-resolve|-R]\n"
		        "\tDisable MiNT function calls for DRACONIS \n"
			"\tapplications >=v1.8, bypass MiNT DNS calls.\n"
			" [--enable-old-system-resolve|-o]\n"
		        "\tEnable MiNT function calls for DNS query,\n"
			"\tfor DRACONIS application <v1.8\n"
			" [--enable-system-resolve|-r]\n"
		        "\tEnable MiNT function calls for DNS query,\n"
			"\tfor DRACONIS application >=v1.8\n"
			" [-c|--compatibility[=version]]\n"
			"\tSets the compatibility mode. If no version\n"
		        "\tis specified, v1.7 is assumed\n"
		        " [--nocompatibility|-n]\n"
			"\tDisable compatibility mode\n"
			"\nFor changing parameters simply restart mgw.prg\n"
			"with the new parameters. The parameters are\n"
		        "automatically passed to the running application\n",
	  		argv[0]);
		exit(0);
	  	}

		/* If there are command line parameters and there is a
			running instance, pass these parameters to it */
	if (Ssystem (S_GETCOOKIE, TCPCOOKIE, &dummy) == 0 && argc > 1)
		{
		send_cmdline(argc, argv);
		exit(0);
		}

		/* Try installation */
	if (fork () == 0)
	{
		Cconws (MSG_BOOT);
		Cconws (MSG_GREET);
		
		if (install_cookie ())
		{
			Cconws (MSG_FAILURE);
			exit (1);
		}
		
		parse_cmdline(argc, argv);		
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
				case MGW_GETUNLOCK:
				{
				getfunc_unlock();
				continue;
				}
				
				/* Internal communication */
				case MGW_NEWCMDLINE:
				{
				char **buf = (char **) pmsg_in.msg2;
				int cnt;
				
				cnt = 0;
				while(buf[cnt]) cnt++;
				
				parse_cmdline(cnt, buf);		
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
