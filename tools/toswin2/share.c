
#include <string.h>
#include <sys/ioctl.h>

#include <cflib.h>

#include "global.h"
#include "config.h"
#include "environ.h"
#include "proc.h"
#include "share.h"
#include "toswin2.h"
#include "window.h"


#undef DEBUG_SH

int env_bytes;

static void sendMsg(int id, int what)
{
	short	msg[8];

	msg[0] = what;
	msg[1] = gl_apid;
	msg[2] = 0;
	appl_write(id, (int)sizeof(msg), msg);
}

void handle_share(int id)
{
	int			fd;
	int			msg[8];
	SHAREDATA	*blk;
	TEXTWIN		*t;
	char			name[80], 
					*env;
	char			arg[125] = " ";
	WINCFG		*cfg;
	int			err, ev, d;
		
	sendMsg(id, TWOK);
	err = 0;
	do
	{
		ev = evnt_multi((MU_MESAG|MU_TIMER), 0, 0, 0, 
							 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
							 msg, 5000L, &d, &d, &d, &d, &d, &d); 
		if ((ev & MU_MESAG) && (msg[0] == TWWRITE))
			err = 1;
		if (ev & MU_TIMER)		/* Timeout, tw-call antwortet nicht */
			err = -1;
	}
	while (err == 0);

	if (err == -1)
	{
		alert(1, 0, NOTWCALL);
		return ;
	}
	
	sprintf(name, SHARENAME, id);
#ifdef DEBUG_SH
	debug("SHM-Name: %s\n", name);
#endif

	fd = (int)Fopen(name, 2);
	if (fd > 0)
	{
		blk = (SHAREDATA *)Fcntl(fd, 0L, SHMGETBLK);

#ifdef DEBUG_SH
		debug("Name : %s\n", blk->name);
		debug("Pfad : %s\n", blk->pfad);
		debug("Arg  : %s\n", blk->arg);
#endif
		cfg = get_wincfg(blk->name);
		env = share_env(cfg->col, cfg->row, cfg->vt_mode, blk->name, blk->arg, blk->env);
		if (blk->arg[0] != '\0')
		{
			strcat(arg, blk->arg);
			arg[0] = strlen(arg);
		}
		else
		{
			arg[0] = 127;
			arg[1] = '\0';
		}
		t = new_proc(blk->name, arg, env, blk->pfad, cfg, id);
		if (t)
			open_window(t->win, cfg->iconified);
		free(env);
		Fclose(fd);
		Mfree(blk);
		Fdelete(name);
		sendMsg(id, TWREAD);
	}
	else
	{
#ifdef DEBUG_SH
		debug("shared open failed (%d)!\n", fd);
#endif
		sendMsg(id, TWERR);
	}	
}


void handle_exit(int id)
{
	if (id != -1)
	{
		short	msg[8];

		msg[0] = CH_EXIT;
		msg[1] = gl_apid;
		msg[2] = 0;
		msg[3] = id;
		msg[4] = exit_code;
		appl_write(id, (int)sizeof(msg), msg);
	}
}
