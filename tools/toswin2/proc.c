
#include <mintbind.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <macros.h>
#include <signal.h>
#include <string.h>
#include <support.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

#define _OLD_UTMP
#include <utmp.h>

#include "proc.h"
#include "console.h"
#include "environ.h"
#include "toswin2.h"
#include "share.h"

/* static long 	lasthz; */
static long 	fdmask = 0L;
static int	shell_cnt = 0;
static OBJECT	*argbox;

static int open_pty(char *name)
{
	int	fd = -1;
	char	line[] = "u:\\pipe\\ttypX";
	char	lnk[] = "u:\\dev\\ttypX";
	char	*c; 
	
	for (c = "0123456789abcdef"; *c; c++) 
	{
		line[12] = *c;
		if (!file_exists(line))
		{
			fd = (int)Fcreate(line, FA_SYSTEM|FA_HIDDEN);
			if (fd < 0) 
				return -ENOENT;
			else 
			{
				lnk[11] = *c;
				(void)Fsymlink(line, lnk);
				if (name)
					strcpy(name, line);
				
				/* set to non-delay mode, so Fread() won't block */
				Fcntl(fd, (long)O_NDELAY, F_SETFL);
				
				return fd;
			}
		}
	}
	return -ENOENT;
}

TEXTWIN *new_proc(char *progname, char *arg, char *env, char *progdir, 
						WINCFG *cfg, int talkID)
{
	TEXTWIN 			*t;
	int 				i, ourfd, kidfd;
	char 				termname[15];
	long 				oldblock;
	struct winsize tw;
	char				str[80], *p;

	if (cfg->title[0] == '\0')
		strcpy(str, progname);
	else
		strcpy(str, cfg->title);
	t = create_textwin(str, cfg);
	if (!t) 
	{
		alert(1, 0, NOTEXT);
		return NULL;
	}
	t->prog = strdup(progname);


	/* for testing under MagiC: open a window and do some output */
	if (gl_debug && gl_magx)
	{
		t->fd = 0;
		t->pgrp = 0;
		t->shell = 0;
		t->talkID = -1;

		write_text(t, "pDummy window for TW2 under MagiC\r\nq", -1);
		for (i = 0; i<50; i++)
		{
			sprintf(str, "Some output for testing, line %d...\r\n", i);
			write_text(t, str, -1);
		}
		refresh_textwin(t, FALSE);
		return t;
	}
	
	ourfd = open_pty(termname);
	if (ourfd < 0) 
	{
		alert(1, 0, NOPTY);
		destroy_textwin(t);
		return NULL;
	}
	t->fd = ourfd;
	p = strrchr(termname, '\\');
	strcpy(t->pty, p+1);

	tw.ws_xpixel = tw.ws_ypixel = 0;
	tw.ws_row = cfg->row;
	tw.ws_col = cfg->col;
	Fcntl(ourfd, &tw, TIOCSWINSZ);

	oldblock = Psigblock(1L << SIGCHLD);

	i = (int)Pfork();
	if (i < 0) 			/* Fehler */
	{
		(void)Fclose(ourfd);
		(void)Psigsetmask(oldblock);
		alert(1, 0, NOFORK);
		destroy_textwin(t);
		return NULL;
	}
	else if (i == 0) 	/* im Child */
	{
		(void)Psetpgrp(0, 0);
		kidfd = (int)Fopen(termname, 2);
		if (kidfd < 0) 
			_exit(998);

		if (!gl_magx)
			Fforce(-1, kidfd);
		Fforce(0, kidfd);
		Fforce(1, kidfd);
		Fforce(2, kidfd);
		Fclose(kidfd);

		chdir(progdir);
		i = (int)Pexec(200, progname, arg, env);

		sprintf(str, "\r\n  Pexec failed (err = %d)!\r\n", i);
		write_text(t, str, -1);
		_exit(i);
	}

	t->pgrp = i;
	fdmask |= (1L << ourfd);

	/* turn on the cursor in the window */
	(*t->output)(t, '\033');
	(*t->output)(t, 'e');

	(void)Psigsetmask(oldblock);

	t->talkID = talkID;
/*
debug("new_proc: pid= %d, fd= %d\n", i, ourfd);
*/
	return t;
}

void add_fd(int fd)
{
	fdmask |= (1L << fd);
}

void term_proc(TEXTWIN *t)
{
	if (t->fd > 0)
	{
		(void)Fclose(t->fd);
		if (t->fd == con_fd)
		{
			con_fd = 0;
			Fdelete(TWCONNAME);
		}
		t->fd = 0;
	}
	if (t->pgrp > 0)
	{
		(void)Pkill(-t->pgrp, SIGHUP);
		t->pgrp = 0;
	}
	if (t->shell != NO_SHELL)
	{
		if (t->shell == LOGIN_SHELL)
		{
			_write_utmp(t->pty, "", "", time(NULL));
			_write_wtmp(t->pty, "", "", time(NULL));
		}
		t->shell = 0;
		shell_cnt--;
	}

	handle_exit(t->talkID);
	t->talkID = -1;

	exit_code = 0;
}


TEXTWIN *new_shell(void)
{
	struct passwd	*pw;
	TEXTWIN			*t = NULL;
	char				*env, *p;
	char				arg[] = "";	/* 127 -> ARGV */
	char				shell[80];
	WINCFG			*cfg;
	
	shel_envrn(&p, "LOGNAME=");
	if (p != NULL)
		pw = getpwnam(p);
	else
		pw = getpwuid(geteuid());
  	if ((pw != NULL) && (pw->pw_shell[0] != '\0'))
	{
		graf_mouse(BEE, NULL);
		unx2dos(pw->pw_shell, shell);
		cfg = get_wincfg(shell);
		env = shell_env(cfg->col, cfg->row, cfg->vt_mode, pw->pw_dir, pw->pw_shell,
							 ((shell_cnt == 0)|| gl_allogin));
		t = new_proc(shell, arg, env, pw->pw_dir, cfg, -1);
		if (t)
		{
			if ((shell_cnt == 0) || gl_allogin)
			{
				char	hostname[20];
		
				t->shell = LOGIN_SHELL;
				gethostname(hostname, sizeof(hostname));
				_write_utmp(t->pty, pw->pw_name, hostname, time(NULL));
				_write_wtmp(t->pty, pw->pw_name, hostname, time(NULL));
			}
			else
				t->shell = NORM_SHELL;
			shell_cnt++;
			open_window(t->win, cfg->iconified);
		}
		free(env);
		graf_mouse(ARROW, NULL);
	}
	else
		alert(1, 0, NOPWD);
	return t;
}


TEXTWIN *start_prog(void)
{
	char		filename[256],	path[80] = "", tmp[60] = "";
	char		*env, arg[51] = " ";
	TEXTWIN	*t = NULL;
	WINCFG	*cfg;
	int		antw;
	
	if (select_file(path, tmp, "", rsc_string(STRPROGSEL), FSCB_NULL))
	{
		strcpy(filename, path);
		strcat(filename, tmp);
		if (Dpathconf(filename, 6) != 0)
			str_tolower(filename);
		else
			filename[0] = tolower(filename[0]);		/* sonst nur Laufwerk */

		cfg = get_wincfg(filename);
		make_shortpath(filename, tmp, 30);
		set_string(argbox, ARGPROG, tmp);
		set_string(argbox, ARGSTR, cfg->arg);
		antw = simple_dial(argbox, ARGSTR);
		if (antw == ARGOK)
		{
			get_string(argbox, ARGSTR, cfg->arg);
			strcat(arg, cfg->arg);
			arg[0] = strlen(arg);
			env = normal_env(cfg->col, cfg->row, cfg->vt_mode);
			t = new_proc(filename, arg, env, path, cfg, -1);
			if (t)
				open_window(t->win, cfg->iconified);
			free(env);
		}
	}
	return t;
}

static void rebuild_fdmask(long which)
{
	int 		i;
	WINDOW	*w, *wnext;
	TEXTWIN	*t;

	for (i = 0; i < 32; i++) 
	{
		if ((which & (1L << i)) && (Finstat(i) < 0)) 
		{
			/* window has died now */
			fdmask &= ~(1L << i);
			for (w = gl_winlist; w; w = wnext) 
			{
				wnext = w->next;
				t = w->extra;
				if (t && t->fd == i) 
				{
					if (t->cfg->autoclose)
						(*w->closed)(w);
					else 
					{
						write_text(t, "\r\n Process terminated.\r\n", -1);
						write_text(t, " Hit Return to close window.\r\n\r\n", -1);
						refresh_textwin(t, FALSE);
						t->pgrp = 0;
						term_proc(t);
					}
				}
			}
		}
	}
}


#define READBUFSIZ 4096 	/* 256 */
static char buf[READBUFSIZ];

void fd_input(void)
{
	long		readfds, r, checkdead;
	WINDOW 	*w;
	TEXTWIN 	*t;

	r = 0;
	checkdead = 0;

/*
	long 		updtime;
	long 		newhz;
	newhz = clock();
	updtime = (newhz - lasthz) >> 2;
	lasthz = newhz;
*/

	if (fdmask) 
	{
		readfds = fdmask;
		if ((r = Fselect(1, &readfds, 0L, 0L)) > 0) 
		{
			for (w = gl_winlist; w; w = w->next) 
			{
				if (w->flags & WISDIAL)
					continue;

				t = w->extra;
				if (!t || !t->fd) 
					continue;
				if (readfds & (1L << t->fd)) 
				{
					long int read_bytes =
						Fread(t->fd, (long)READBUFSIZ, buf);
					if (read_bytes > 0)
					{
						write_text(t, buf, read_bytes);
						if (t->fd == con_fd)
							handle_console(buf, read_bytes);
					}
					else
						checkdead |= (1L << t->fd);
				}
			}
		}
		if (checkdead)
			rebuild_fdmask(checkdead);
	}
	if (r == -EBADF) 
		rebuild_fdmask(fdmask);

	/* for all windows on screen, see if it's time to refresh them */
	for (w = gl_winlist; w; w = w->next) 
	{
		if (w->flags & WISDIAL)
			continue;

		t = w->extra;
		if (!t || !t->fd || !t->nbytes) 
			continue;
#if 0
		t->draw_time += updtime;
		if (t->draw_time < 3) /* max no. of 50Hz ticks before a refresh */
			continue;
#endif

		refresh_textwin(t, FALSE);
	}
}


/*
 * signal handler for dead children
 * all we do is collect the exit code
 * it's up to fd_input() and friends to figure out when no
 * more data exists for a window
 */

static void dead_kid(void)
{
	long	r;

	r = Pwait3(WNOHANG, (void *)0);
/*
printf("dead_kid: pid=%ld, exit=%ld\n", ((r>>16) & 0x0000ffff),(r & 0x0000ffff));
*/
	exit_code = (short) (r & 0x0000ffff);
}


/*
 * signal handler for SIGINT and SIGQUIT: these get passed along to the
 * process group in the top window
 */
static void send_sig(long sig)
{
	if (gl_topwin) 
	{
		TEXTWIN *t;

		t = gl_topwin->extra;
		if (t->pgrp)
			(void)Pkill(-t->pgrp, (short)sig);
	}
}

static void ignore()
{
}

void proc_init(void)
{
	(void)Psignal(SIGCHLD, (long)dead_kid);
	(void)Psignal(SIGINT, (long)send_sig);
	(void)Psignal(SIGQUIT, (long)send_sig);
	(void)Psignal(SIGHUP, (long)send_sig);
	(void)Psignal(SIGTSTP, (long)send_sig);
	(void)Psignal(SIGTTIN, (long)ignore);
	(void)Psignal(SIGTTOU, (long)ignore);
	
	rsrc_gaddr(R_TREE, ARG, &argbox);
	fix_dial(argbox);
}
