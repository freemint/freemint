/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

/* MiNT debugging output routines */

# include <stdarg.h>
# include "debug.h"
# include "global.h"

# include "arch/mprot.h"
# include "arch/startup.h"	/* _base */
# include "libkern/libkern.h"
# include "mint/signal.h"

# include "biosfs.h"
# include "block_IO.h"
# include "dos.h"
# include "fatfs.h"
# include "filesys.h"
# include "gmon.h"
# include "init.h"
# include "memory.h"
# include "k_fds.h"
# include "kmemory.h"
# include "proc.h"
# include "signal.h"

# include <osbind.h>


static void VDEBUGOUT (int, const char *, va_list);


int debug_level = 1;	/* how much debugging info should we print? */
# if 1
int out_device = 2;	/* BIOS device to write errors to */
# else
int out_device = 6;
# endif

/*
 * out_next[i] is the out_device value to use when the current
 * device is i and the user hits F3.
 * Cycle is CON -> PRN -> AUX -> MIDI -> 6 -> 7 -> 8 -> 9 -> CON
 * (Note: BIOS devices 6-8 exist on Mega STe and TT, 9 on TT.)
 *
 * out_device and this table are exported to bios.c and used here in HALT().
 */

/*		    0  1  2  3  4  5  6  7  8  9 */
char out_next[] = { 1, 3, 0, 6, 0, 0, 7, 8, 9, 2 };

/*
 * debug log modes:
 *
 * 0: no logging.
 * 1: log all messages, dump the log any time something happens at
 *    a level that gets shown.  Thus, if you're at debug_level 2,
 *    everything is logged, and if something at levels 1 or 2 happens,
 *    the log is dumped.
 *
 * LB_LINE_LEN is 20 greater than SPRINTF_MAX because up to 20 bytes
 * are prepended to the buffer string passed to ksprintf.
 */

# ifdef DEBUG_INFO
# define LBSIZE		50			/* number of lines */
# else
# define LBSIZE		15
# endif
# define LB_LINE_LEN	(SPRINTF_MAX + 20)	/* width of a line */

static char logbuf [LBSIZE][LB_LINE_LEN];
static ushort logtime [LBSIZE];	/* low 16 bits of 200 Hz: timestamp of msg */

int debug_logging;
int logptr;


/*
 * The inner loop does this: at each newline, the keyboard is polled. If
 * you've hit a key, then it's checked: if it's ctl-alt, do_func_key is
 * called to do what it says, and that's that.  If not, then you pause the
 * output.  If you now hit a ctl-alt key, it gets done and you're still
 * paused.  Only hitting a non-ctl-alt key will get you out of the pause. 
 * (And only a non-ctl-alt key got you into it, too!)
 *
 * When out_device isn't the screen, number keys give you the same effects
 * as function keys.  The only way to get into this code, however, is to
 * have something produce debug output in the first place!  This is
 * awkward: Hit a key on out_device, then hit ctl-alt-F5 on the console so
 * bios.c will call DUMPPROC, which will call ALERT, which will call this.
 * It'll see the key you hit on out_device and drop you into this loop.
 * CTL-ALT keys make BIOS call do_func_key even when out_device isn't the
 * console.
 */

void
debug_ws (const char *s)
{
	long key;
	int scan;
	int stopped;
	
	while (*s)
	{
		(void) Bconout (out_device, *s);
		
		while (*s == '\n' && out_device != 0 && Bconstat (out_device))
		{
			stopped = 0;
			while (1)
			{
				if (out_device == 2)
				{
					/* got a key; if ctl-alt then do it */
					if ((Kbshift (-1) & 0x0c) == 0x0c)
					{
						key = Bconin (out_device);
						scan = (int) (((key >> 16) & 0xff));
						do_func_key (scan);
						
						goto ptoggle;
					}
					else
					{
						goto cont;
					}
				}
				else
				{
					key = Bconin (out_device);
					if (key < '0' || key > '9')
					{
ptoggle:
						/* not a func key */
						if (stopped) break;
						else stopped = 1;
					}
					else
					{
						/* digit key from debug device == Fn */
						if (key == '0') scan = 0x44;
						else scan = (int) (key - '0' + 0x3a);
						do_func_key (scan);
					}
				}
			}
		}
cont:
		s++;
	}
}

/*
 * _ALERT(s) returns 1 for success and 0 for failure.
 * It attempts to write the string to "the alert pipe," u:\pipe\alert.
 * If the write fails because the pipe is full, we "succeed" anyway.
 *
 * This is called in vdebugout and also in mprot0x0.c for memory violations.
 * It's also used by the Salert() system call in dos.c.
 */

int
_ALERT (char *s)
{
	FILEPTR *fp;
	long ret;
	
	/* temporarily reduce the debug level, so errors finding
	 * u:\pipe\alert don't get reported
	 */
	int olddebug = debug_level;
	int oldlogging = debug_logging;
	
	debug_level = 0;
	debug_logging = 0;
	
	ret = fp_alloc (rootproc, &fp);
	if (!ret)
		ret = do_open (&fp, "u:\\pipe\\alert", (O_WRONLY | O_NDELAY), 0, NULL);
	
	debug_level = olddebug;
	debug_logging = oldlogging;
	
	if (!ret)
	{		
		char *alert;
		
		/* format the string into an alert box
		 */
		if (*s == '[')
		{
			/* already an alert */
			alert = s;
		}
		else
		{
			char alertbuf [SPRINTF_MAX + 10];
			char *ptr;
			char *lastspace;
			int counter;
			
			alert = alertbuf;
			ksprintf (alertbuf, sizeof (alertbuf), "[1][%s", s);
			
			/* make sure no lines exceed 30 characters;
			 * also, filter out any reserved characters
			 * like '[' or ']'
			 */
			ptr = alertbuf + 4;
			counter = 0;
			lastspace = 0;
			
			while (*ptr)
			{
				if (*ptr == ' ')
				{
					lastspace = ptr;
				}
				else if (*ptr == '[')
				{
					*ptr = '(';
				}
				else if (*ptr == ']')
				{
					*ptr = ')';
				}
				else if (*ptr == '|')
				{
					*ptr = ':';
				}
				
				if (counter++ >= 29)
				{
					if (lastspace)
					{
						*lastspace = '|';
						counter = (int) (ptr - lastspace);
						lastspace = 0;
					}
					else
					{
						*ptr = '|';
						counter = 0;
					}
				}
				
				ptr++;
			}
			
			strcpy (ptr, "][  OK  ]");
		}
		
		(*fp->dev->write)(fp, alert, strlen (alert) + 1);
		do_close (rootproc, fp);
		
		return 1;
	}
	
	return 0;
}

static void
VDEBUGOUT (int level, const char *s, va_list args)
{
	char *lp;
	char *lptemp;
	long len;
	
	logtime [logptr] = (ushort)(*(long *) 0x4baL);
	lptemp = lp = logbuf [logptr];
	len = LB_LINE_LEN;
	
	if (++logptr == LBSIZE)
		logptr = 0;
	
	if (curproc)
	{
		ksprintf (lp, len, "pid %3d (%s): ", curproc->pid, curproc->name);
		lptemp += strlen (lp);
		len -= strlen (lp);
	}
	
	vsprintf (lptemp, len, s, args);
	
	/* for alerts, try the alert pipe unconditionally */
	if (level == ALERT_LEVEL && _ALERT (lp))
		return;
	
	if (debug_level >= level)
	{
		debug_ws (lp);
		debug_ws ("\r\n");
	}
}

void _cdecl
Tracelow (const char *s, ...)
{
	if (debug_logging || (debug_level >= LOW_LEVEL))
	{
		va_list args;
		
		va_start (args, s);
		VDEBUGOUT (LOW_LEVEL, s, args);
		va_end (args);
	}
}

void _cdecl
Trace (const char *s, ...)
{
	if (debug_logging || (debug_level >= TRACE_LEVEL))
	{
		va_list args;
		
		va_start (args, s);
		VDEBUGOUT (TRACE_LEVEL, s, args);
		va_end (args);
	}
}

void _cdecl
Debug (const char *s, ...)
{
	if (debug_logging || (debug_level >= DEBUG_LEVEL))
	{
		va_list args;
		
		va_start (args, s);
		VDEBUGOUT (DEBUG_LEVEL, s, args);
		va_end (args);
	}
	
	if (debug_logging && (debug_level >= DEBUG_LEVEL))
		DUMPLOG ();
}

void _cdecl
ALERT (const char *s, ...)
{
	if (debug_logging || debug_level >= ALERT_LEVEL)
	{
		va_list args;
		
		va_start (args, s);
		VDEBUGOUT (ALERT_LEVEL, s, args);
		va_end (args);
	}
	
	if (debug_logging && (debug_level >= ALERT_LEVEL))
		DUMPLOG ();
}

void _cdecl
FORCE (const char *s, ...)
{
	va_list args;
	
	va_start (args, s);
	VDEBUGOUT (FORCE_LEVEL, s, args);
	va_end (args);
	
	/* don't dump log here - hardly ever what you mean to do. */
}

void
DUMPLOG (void)
{
	char *end;
	char *start;
	ushort *timeptr;
	char timebuf[6];
	
	/* logbuf [logptr] is the oldest string here */
	
	end = start = logbuf [logptr];
	timeptr = &logtime [logptr];
	
	do
	{
		if (*start)
		{
			ksprintf (timebuf, sizeof (timebuf), "%04x ", *timeptr);
			debug_ws (timebuf);
			debug_ws (start);
			debug_ws ("\r\n");
			*start = '\0';
		}
		
		start += LB_LINE_LEN;
		timeptr++;
		
		if (start == logbuf[LBSIZE])
		{	
			start = logbuf[0];
			timeptr = &logtime[0];
		}
        }
        while (start != end);
	
        logptr = 0;
}

/* wait for a key to be pressed */
void
PAUSE (void)
{
	debug_ws ("Hit a key\r\n");
	(void) Bconin (2);
}
  
EXITING _cdecl
FATAL (const char *s, ...)
{
	va_list args;
	
	va_start (args, s);
	VDEBUGOUT (-1, s, args);
	va_end (args);
	
	if (debug_logging)
	{
		DUMPLOG ();
	}
	
	HALT ();
}


/* uk: a normal halt() function without an error message. This function
 *     may only be called if all processes are shut down and the file
 *     systems are synced.
 */

static const char *haltmsg [MAXLANG] =
{
	"System halted.\r\n",
	"System angehalten.\r\n",	/* German */
	"Arrˆt du systŠme.\r\n",	/* French */
	"System halted.\r\n",		/* UK */
	"System halted.\r\n",		/* Spanish */
	"System halted.\r\n"		/* Italian */
};

EXITING
halt (void)
{
	long r;
	long key;
	int scan;
	
	DEBUG (("halt() called, system halting...\r\n"));
	debug_ws (haltmsg[gl_lang]);
	
	sys_q[READY_Q] = 0;	/* prevent context switches */
	restr_intr();		/* restore interrupts to normal */
	
	for(;;)
	{
		/* get a key; if ctl-alt then do it, else halt */
		key = Bconin (out_device);
		
		if ((key & 0x0c000000L) == 0x0c000000L)
		{
			scan = (int) ((key >> 16) & 0xff);
			do_func_key (scan);
		}
		else
		{
			break;
		}
	}
	
	for(;;)
	{
		debug_ws (haltmsg[gl_lang]);
		r = Bconin (2);
		if ((r & 0x0ff) == 'x')
		{
		}
	}
}


static const char *rebootmsg[MAXLANG] =
{
	"FATAL ERROR. You must reboot the system.\r\n",
	"FATALER FEHLER. Das System muž neu gestartet werden.\r\n",	/* German */
	"ERREUR FATALE. Vous devez red‚marrer le systŠme.\r\n",		/* French */
	"FATAL ERROR. You must reboot the system.\r\n",			/* UK */
	"FATAL ERROR. You must reboot the system.\r\n",			/* Spanish */
	"FATAL ERROR. You must reboot the system.\r\n"			/* Italian */
};

EXITING 
HALT (void)
{
	long r;
	long key;
	int scan;
	
	DEBUG (("Fatal MiNT error: adjust debug level and hit a key...\r\n"));
	debug_ws (rebootmsg[gl_lang]);
	
	sys_q[READY_Q] = 0;	/* prevent context switches */
	restr_intr ();		/* restore interrupts to normal */
	
	for (;;)
	{
		/* get a key; if ctl-alt then do it, else halt */
		key = Bconin (2);
		
		if ((key & 0x0c000000L) == 0x0c000000L)
		{
			scan = (int) ((key >> 16) & 0xff);
			do_func_key (scan);
		}
		else
			break;
	}
	
	for (;;)
	{
		debug_ws (rebootmsg[gl_lang]);
		r = Bconin (2);
		
		if (((r & 0x0ff) == 'x') || ((r & 0xff) == 's'))
		{
			close_filesys ();
			
			/* if the user pressed 's', try to sync before halting the system */
			if ((r & 0xff) == 's')
			{
				debug_ws ("Syncing...");
				s_ync ();
				debug_ws ("done.\r\n");
			}
		}
	}
}


/* some key definitions */
# define CTRLALT	0xc
# define DEL		0x53	/* scan code of delete key */
# define UNDO		0x61	/* scan code of undo key */

void
do_func_key (int scan)
{
	switch (scan)
	{
		case DEL:
		{
			s_hutdown(1);
			break;
		}
		case UNDO:
		{
			killgroup (con_tty.pgrp, SIGQUIT, 1);
			break;
		}
# ifdef DEBUG_INFO
		/* F1: increase debugging level */
		case 0x3b:
		{
			debug_level++;
			break;
		}
		
		/* F2: reduce debugging level */
		case 0x3c:
		{
			if (debug_level > 0)
				--debug_level;
			break;
		}
		
		/* F3: cycle out_device */
		case 0x3d:
		{
			out_device = out_next [out_device];
			break;
		}
		
		/* F4: set out_device to console */
		case 0x3e:
		{
			out_device = 2;
			break;
		}
		
		/* F5: dump memory */
		case 0x3f:
		{
			DUMP_ALL_MEM ();
			break;
		}
		
		/* shift+F5: dump kernel allocated memory */
		case 0x58:
		{
			km_config (KM_STAT_DUMP, 0);
			
			{
				static long mode = DISABLE;
				
				if (mode == ENABLE)
					mode = DISABLE;
				else
					mode = ENABLE;
				
				fatfs_config (0, FATFS_DEBUG, mode);
				fatfs_config (0, FATFS_DEBUG_T, mode);
				
				bio.config (0, BIO_DEBUG_T, mode);
			}
			
			break;
		}
		
		/* F6: dump processes */
		case 0x40:
		{
			DUMPPROC ();
			_s_ync ();
			break;
		}
		
# ifdef PROFILING
		/* shift+F6: control profiling */
		case 0x59:
		{
			write_profiling ();
			break;
		}
		
		/* shift+F7: toogle profiling */
		case 0x5A:
		{
			toogle_profiling ();
			break;
		}
# endif
		
		/* F7: toggle debug_logging */
		case 0x41:
		{
			debug_logging ^= 1;
			break;
		}
# else
		/* F6: always print MiNT basepage */
		case 0x40:
		{
			FORCE ("MiNT base %lx (%lx)", rootproc->base, _base);
			break;
		}
# endif
		/* F8: dump log */
		case 0x42:
		{
			DUMPLOG ();
			break;
		}
# ifdef DEBUG_INFO
		/* F9: dump the global memory table */
		case 0x43:
		{
			QUICKDUMP ();
			break;
		}
		
		/* shift-F9: dump the mmu tree */
		case 0x5c:
		{
			BIG_MEM_DUMP (1, curproc);
			break;
		}
		
		/* F10: do an annotated dump of memory */
		case 0x44:
		{
			BIG_MEM_DUMP (0, 0);
			break;
		}
# endif
	}
}
