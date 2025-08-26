/*
 * Copyright 2000 Konrad M. Kokoszkiewicz <draco@atari.org>
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * GEM front end for mkfatfs (version of the GEM shell for mke2fs)
 * 
 * Known bugs:
 * 
 * - the command line is limited to 126 bytes (does not use ARGV)
 * 
 */

# include <stdio.h>
# include <errno.h>
# include <fcntl.h>
# include <signal.h>
# include <string.h>
# include <mintbind.h>
# include <slb/gemma.h>
# include <limits.h>

# include "gemkfatfs.h"
# include "util.h"


#define DriveToLetter(d) ((d) < 26 ? 'A' + (d) : (d) - 26 + '1')


/* `flag' tells thou whether to redraw the object being updated */

static void
disable(WINDIAL *wd, short obj, short flag)
{
	OBJECT *ob = (OBJECT *)wd->wb_treeptr;

	ob[obj].ob_flags &= ~(OF_SELECTABLE|OF_EXIT|OF_RBUTTON|OF_TOUCHEXIT);
	if (flag)
		objc_xchange(wd, obj, ob[obj].ob_state | OS_DISABLED, 1);
	else
		ob[obj].ob_state |= OS_DISABLED;
}

static void
enable(WINDIAL *wd, short obj, short flag)
{
	OBJECT *ob = (OBJECT *)wd->wb_treeptr;

	ob[obj].ob_flags |= (OF_SELECTABLE|OF_EXIT);
	if (flag)
		objc_xchange(wd, obj, ob[obj].ob_state & ~OS_DISABLED, 1);
	else
		ob[obj].ob_state &= ~OS_DISABLED;
}

static void
deselect(WINDIAL *wd, short obj, short force)
{
	OBJECT *ob = (OBJECT *)wd->wb_treeptr;

	if (force || (!(ob[obj].ob_state & OS_WHITEBAK) && !(ob[obj].ob_flags & OF_RBUTTON)))
		objc_xchange(wd, obj, ob[obj].ob_state & ~OS_SELECTED, 1);
}

/* Handlers called by the library */

static long
newmsg(WINDIAL *wd, short vec, short *msg)
{
	long ret;

	if (msg[0] == AP_TERM)
		windial_longjmp(wd, vec);	/* never returns */
	if (msg[0] == CH_EXIT)
	{
		ret = msg[4];
		if (ret != 0)
		{
			/* process was signaled, or exited with exitcode != 0 */
			if ((char)ret != 127)
			{
				/* process was signaled */
				bell();
				if ((char)ret == 0)
				{
					/* process was exited by signal */
					windial_alert(1, (const char *)SIGNALTERM);
				} else
				{
					/* process was exited with exitcode != 0 */
					if (ret < 0)
						windial_error(ret, (const char *)ERRMKFATFS);
					else
						windial_error(-1L, (const char *)ERRMKFATFS);
				}
			} else
			{
				/* process was stopped */
			}
		} else
		{
			/* process exited */
			windial_alert(1, (const char *)MKFATFSOK);
		}
	}
	return 0;
}

static short secperc;
static const short option[] = 	{ SETVOLUMELABEL, BADBLOCKFILENAME, SETVOLUMEID,
			  BOOTMESSAGE, SETROOTENTRIES, SETSPC,
			  NOPARTITIONIDCHK, CHECKFAT };
static const short argums[] =	{ 1, 2, 1, 2, 1, 3, 0, 0 };
static const char *const cmd_sw[] =	{ "-n ", "-l ", "-i 0x", "-m ", "-r ", "-s ", "-a", "-c" };
static long argfdd[] =		{ VOLUMELABEL, 0, VOLUMEID, 0, ROOTENTRIES, SPC, 0, 0 };

static void sig_child(void)
{
	long p;
	short msg[8];
	GEM_ARRAY *me;
	
	p = Pwait3(1, NULL);
	Psignal(SIGCHLD, sig_child);
	me = gem_control();
	msg[0] = CH_EXIT;
	msg[1] = me->global[2];
	msg[2] = 0;
	msg[3] = (short)(p>>16);
	msg[4] = (short)p;
	msg[5] = 0;
	msg[6] = 0;
	msg[7] = 0;
	appl_write(me->global[2], 16, msg);
}


static long
exec_mkfatfs(WINDIAL *wd, short drive)
{
	OBJECT *ob = wd->wb_treeptr, *pop;
	long r;
	short x;
	char command[4096], *cmd, *tmp;

	cmd = command;

	strcpy(cmd, "-q ");

	for (x = 0; x < 8; x++)
	{
		if (ob[option[x]].ob_state & OS_SELECTED)
		{
			switch(argums[x])
			{
				case	0:
					strcat(cmd, cmd_sw[x]);
					break;
				case	1:
					tmp = ob[argfdd[x]].ob_spec.tedinfo->te_ptext;
					if (strlen(tmp))
					{
						strcat(cmd, cmd_sw[x]);
						quote_and_cat(cmd, tmp);
					}
					break;
				case	2:
					if (argfdd[x])
					{
						tmp = (char *)argfdd[x];
						if (tmp)
						{
							strcat(cmd, cmd_sw[x]);
							strcat(cmd, tmp);
						}
					}
					break;
				case	3:
					pop = (OBJECT *)rsrc_xgaddr(R_TREE, POPUP);
					tmp = pop[secperc + SPC2].ob_spec.free_string;
					while(*tmp == ' ')
						tmp++;
					strcat(cmd, cmd_sw[x]);
					strcat(cmd, tmp);
			}
			strcat(cmd, " ");
		}
	}	

	/* Simple brutal methods are usually best */

	strcat(cmd, "-F ");
	tmp = "12";
	if (ob[FAT16].ob_state & OS_SELECTED)
		tmp = "16";
	else if (ob[FAT32].ob_state & OS_SELECTED)
		tmp = "32";
	strcat(cmd, tmp);
	strcat(cmd, " -f ");
	tmp = "1";
	if (ob[TWOFATS].ob_state & OS_SELECTED)
		tmp = "2";
	if (ob[THREEFATS].ob_state & OS_SELECTED)
		tmp = "3";
	if (ob[FOURFATS].ob_state & OS_SELECTED)
		tmp = "4";
	strcat(cmd, tmp);

	cmd = command + strlen(command);
	*cmd++ = ' ';
	*cmd++ = DriveToLetter(drive);
	*cmd++ = ':';
	*cmd = '\0';

	r = windial_alert(1, WARNING);
	if (r == 2)
		return 0;

	Psignal(SIGCHLD, sig_child);
	r = proc_exec(100, 0x1, "mkfatfs", command, NULL);

	return r;
}
		
static void
do_window(WINDIAL *wd)
{
	OBJECT *ob = wd->wb_treeptr;
	long r, dmap, chpid = 0;
	short m, drive = 0;
	MENU menu;
	char *f;

	disable(wd, MAKEFS, 0);

	/* Disable non-working options */
	disable(wd, BOOTMESSAGE, 0);
	disable(wd, NOPARTITIONIDCHK, 0);
	
	ob[FAT16].ob_state |= OS_SELECTED;
	ob[TWOFATS].ob_state |= OS_SELECTED;

	menu.mn_tree = (OBJECT *)rsrc_xgaddr(R_TREE, POPUP);
	menu.mn_menu = POP_PARENT;
	menu.mn_item = SPC2;
	menu.mn_scroll = SCROLL_NO;

	ob[SPC].ob_spec = menu.mn_tree[SPC2].ob_spec;

	drive = Dgetdrv();
	dmap = Dsetdrv(drive) >> 2;
	r = DISK_C;
	while(r < DISK_C+30)
	{
		if (!(dmap & 1))
			disable(wd, r, 0);
		dmap >>= 1;
		r++;
	}

	/* Make the window visible */
	windial_open(wd);

	/* These calls below must be done in the same stack context
	 * as windial_formdo().
	 */
	if (windial_setjmp(wd, 0, newmsg) == 1)
		return;

	for(;;)
	{
		m = windial_formdo(wd);
		if (m != -1)
		{
			m &= 0x7fff;
			deselect(wd, m, 0);	/* does not deselect radiobuttons */
		} else
			m = EXIT;

		switch(m)
		{
			case	EXIT:
					deselect(wd, m, 1);
					if (chpid)
					{
						if (Pkill(chpid, SIGNULL) >= 0)
						{
							r = windial_alert(1, (const char *)MKFATFSKILL);
							if (r == 1)
							{
								Psignal(SIGCHLD, SIG_DFL);
								(void)Pkill(chpid, SIGTERM);
								return;
							} else
								break;
						}
					} 
					return;
			case	MAKEFS:
					chpid = r = exec_mkfatfs(wd, drive);
					if (r < 0)
						windial_error(r, (const char *)NOMKFATFS);
					deselect(wd, m, 1);
					break;
			case	MYHOMEPAGE:
					deselect(wd, m, 1);
					open_url(ob[m].ob_spec.tedinfo->te_ptext);
					break;
			case	BADBLOCKFILENAME:
					f = file_select((char *)FSELTIT1, (char *)FSELMASK, 0);
					if ((long)f && (strlen(f) < 512))
					{
						argfdd[1] = (long)f;
						if (ob[CHECKFAT].ob_state & OS_SELECTED)
							objc_xchange(wd, CHECKFAT, ob[CHECKFAT].ob_state & ~OS_SELECTED, 1);
					}
					else
					{
						objc_xchange(wd, option[1], ob[option[1]].ob_state & ~OS_SELECTED, 1);
						argfdd[1] = 0;
					}
					break;
			case	BOOTMESSAGE:
					f = file_select((char *)FSELTIT2, (char *)FSELMASK, 0);
					if ((long)f && (strlen(f) < 512))
					{
						argfdd[3] = (long)f;
					}
					else
					{
						objc_xchange(wd, option[3], ob[option[3]].ob_state & ~OS_SELECTED, 1);
						argfdd[3] = 0;
					}
					break;
			case	SPC:
					if (!menu_xpop(wd, m, &menu))
						break;
					secperc = menu.mn_item - SPC2;
					break;	
			case	CHECKFAT:
					if (ob[BADBLOCKFILENAME].ob_state & OS_SELECTED)
					{
						objc_xchange(wd, BADBLOCKFILENAME, ob[BADBLOCKFILENAME].ob_state & ~OS_SELECTED, 1);
						argfdd[1] = 0;
					}
					break;
			case	SETVOLUMELABEL:
			case	SETVOLUMEID:
			case	SETROOTENTRIES:
			case	SETSPC:
			case	NOPARTITIONIDCHK:
					break;
			case	FAT12:
			case	FAT16:
					if (ob[SETROOTENTRIES].ob_state & OS_DISABLED)
					{
						enable(wd, SETROOTENTRIES, 1);
						enable(wd, ROOTENTRIES, 1);
					}
					break;
			case	FAT32:
					if ((ob[SETROOTENTRIES].ob_state & OS_DISABLED) == 0)
					{
						deselect(wd, SETROOTENTRIES, 1);
						if (wd->wb_startob == ROOTENTRIES)
						{
							objc_edit(wd->wb_treeptr, ROOTENTRIES, 0, 0, ED_END);
							wd->wb_startob = VOLUMEID;
							objc_edit(wd->wb_treeptr, VOLUMEID, 0, (short)strlen(ob[VOLUMEID].ob_spec.tedinfo->te_ptext), ED_INIT);
						}
						disable(wd, SETROOTENTRIES, 1);
						disable(wd, ROOTENTRIES, 1);
					}
					break;
			default:
					m -= DISK_C;
					if (m < 0 || m >= 30)
						break;
					drive = m + 2;
					enable(wd, MAKEFS, 1);
					break;
		}
	}
}

int
main(void)
{
	WINDIAL *wd;
	long r;

	/* Load the resource file and set the desk menu name
	 * to the object PNAME (alternatively an address could
	 * be given, but this is more elegant)
	 */
	r = appl_open("gemkfatfs.rsc", 0, (char *)PNAME);
	if (r < 0)
	{
		(void) Salert("gemkfatfs: appl_open() failed");
		return r;
	}

	/* this initializes the entire WINDIAL structure */
	wd = (WINDIAL *)windial_create(0, WINDOW, ICON, VOLUMELABEL, (char *)WINTITLE);

	do_window(wd);		/* do all the stuff */

	windial_close(wd);
	windial_delete(wd);

	return appl_close();
}
