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

# include <errno.h>
# include <fcntl.h>
# include <signal.h>
# include <string.h>
# include <mintbind.h>
# include <gemma/gemma.h>

# include "gemkfatfs.h"
# include "util.h"


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
		ret = (long)msg[5];
		if (ret != 0)
		{
			if ((char)ret != 127)
			{
				bell();
				if ((char)ret == 0)
					windial_alert(1, SIGNALTERM);
				else
				{
					if (ret < 0)
						windial_error(ret, ERRMKFATFS);
					else
						windial_error(-1L, ERRMKFATFS);
				}
			}
		} else
			windial_alert(1, MKFATFSOK);
	}
	return 0;
}

static long
newkey(WINDIAL *wd, short vec, short ks, short kc)
{
	if ((kc & 0x007f) == 0x0011)
		windial_longjmp(wd, vec);
	return 0;
}

char file1[1024];
char file2[1024];
short secperc;
const short option[] = 	{ SETVOLUMELABEL, BADBLOCKFILENAME, SETVOLUMEID,
			  BOOTMESSAGE, SETROOTENTRIES, SETSPC,
			  NOPARTITIONIDCHK, CHECKFAT };
const short argums[] =	{ 1, 2, 1, 2, 1, 3, 0, 0 };
const char *cmd_sw[] =	{ "-n ", "-l ", "-i 0x", "-m ", "-r ", "-s ", "-a", "-c" };
long argfdd[] =		{ VOLUMELABEL, 0, VOLUMEID, 0,
			  ROOTENTRIES, SPC, 0, 0 };

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
						if ((long)tmp)
						{
							strcat(cmd, cmd_sw[x]);
							strcat(cmd, tmp);
						}
					}
					break;
				case	3:
					pop = (OBJECT *)rsrc_xgaddr(R_TREE, POPUP);
					tmp = pop[secperc + SPC2].ob_spec.free_string;
					while(*tmp == 0x20)
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
	*cmd++ = '\x20';
	if (drive < 26)
		*cmd++ = (char)(drive + 0x61);
	else
		*cmd++ = (char)(drive + 0x17);
	*cmd++ = ':';
	*cmd = '\0';

	r = windial_alert(1, WARNING);
	if (r == 2)
		return 0;

	r = proc_exec(100, 0x5L, "mkfatfs", command, 0);

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

	dmap = Dsetdrv(Dgetdrv()) >> 2;
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
	if (windial_setjmp(wd, 1, newkey) == 1)
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
					if (chpid)
					{
						if (Pkill(chpid, SIGNULL) >= 0)
						{
							r = windial_alert(1, MKFATFSKILL);
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
						windial_error(r, NOMKFATFS);
					break;
			case	MYHOMEPAGE:
					open_url(ob[m].ob_spec.tedinfo->te_ptext);
					break;
			case	BADBLOCKFILENAME:
					f = file_select((char *)FSELTIT1, (char *)FSELMASK, 0);
					if ((long)f && (strlen(f) < 512))
					{
						strcpy(file1, f);
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
						strcpy(file2, f);
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
		return r;

	/* this initializes the entire WINDIAL structure */
	wd = (WINDIAL *)windial_create(0, WINDOW, ICON, VOLUMELABEL, WINTITLE);

	do_window(wd);		/* do all the stuff */

	windial_close(wd);
	windial_delete(wd);

	return appl_close();
}
