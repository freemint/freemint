/*
 * Copyright (C) 1998, 1999 Christian Felsch
 * All rights reserved.
 * 
 * Modified for FreeMiNT CVS by Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
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
 */

#include <mintbind.h>
#include <cflib.h>
#include <sys/statfs.h>
#include <mint/dcntl.h>
#include <errno.h>
#include <mint/sysvars.h>

#include "fsetter.h"
#include "xhdi.h"

#if !defined(__CFLIB__)
#error at least cflib 0.20.0 is required to compile FSetter
#endif


#define UNLIMITED	0x7fffffffL
extern short _app;

#define PROGRAM "FSetter"
#define VERSION	"0.80"

#define FALSE 0
#define TRUE 1
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
typedef int bool;
#endif

static WDIALOG	*maindial;

static OBJECT	*driveinfo;
static OBJECT	*special;
static OBJECT	*pzip;
static OBJECT	*strings;

static char	**alerts;
static bool	gl_done = FALSE;
static int	desktop = -2;

static int	active_drv = -1;
static char	drive_list[32][8];
static char	fs_names[][7] =
{
	"tos", "?", "minix", "minix2", "ramfs", "ext2",
	"fat12", "fat16", "fat32",
	"vfat12", "vfat16", "vfat32"
};

#define rsc_string(a) (char *) get_obspec(strings, a)

#define DriveToLetter(d) ((d) < 26 ? 'A' + (d) : (d) - 26 + '1')
#define DriveFromLetter(d) \
	(((d) >= 'A' && (d) <= 'Z') ? ((d) - 'A') : \
	 ((d) >= 'a' && (d) <= 'z') ? ((d) - 'a') : \
	 ((d) >= '1' && (d) <= '6') ? ((d) - '1' + 26) : \
	 -1)

/* -------------------------------------------------------------------------- */

/*
 * Laufwerks-Daten ermitteln
 */
static long
get_nflops (void)
{
	return get_sysvar (_nflops);
}

static bool
get_drives (void)
{
	long drive_bits;
	short num_flops;
	int i, j;
	char lw[] = " A:";
	
	i = Dgetdrv ();
	drive_bits = Dsetdrv (i);
	num_flops = get_nflops ();
	if (num_flops == 1)			/* nur eine Floppy */
		drive_bits &= ~2;		/*  -> B ausblenden */
	
	j = 0;
	for (i = 0; i < 32; i++)
	{
		set_state (maindial->tree, LW_A + i, OS_DISABLED, TRUE);
		strcpy (drive_list[i], "-");
		
		if (drive_bits & (1L<<i))
		{
			lw[1] = DriveToLetter(i);
			
			set_state (maindial->tree, LW_A + i, OS_DISABLED, FALSE);
			strcpy (drive_list[i], lw);
			j++;
		}
	}
	
	if (j == 0)
		do_walert (1, 0, alerts[NO_DRIVE], PROGRAM);
	
	return (j > 0);
}

/*
 * Fenster-Update beim Desktop auslîsen
 */
static void
send_shwdraw (char lw)
{
	if (desktop == -2)
	{
		short i, d;
		char name[9], *p;
		
		if (appl_xgetinfo (4, &d, &d, &i, &d) && (i == 1))
		{
			if (appl_search (2, name, &d, &i))
				desktop = i;
		}
		
		if (desktop == -2)
		{
			shel_envrn (&p, "AVSERVER=");
			if (p == NULL)
				p = getenv ("AVSERVER");
			
			if (p != NULL)
			{
				strncpy (name, p, 8);
				name[8] = '\0';
				
				for (i = strlen (name); i < 8; i++)
					strcat (name, " ");
				
				i = appl_find (name);
				if (i >= 0)
					desktop = i;
				else
					desktop = -1;
			}
		}
	}
	
	if (desktop >= 0)
	{
		short msg[8] = { 72, 0, 0, 0, 0, 0, 0, 0 };
		
		msg[1] = gl_apid;
		msg[3] = DriveFromLetter(lw);
		
		appl_write (desktop, sizeof (msg), msg);
	}
}

/*
 * Laufwerks-Info-Dialog anzeigen.
 */
static void
run_info (void)
{
	if (active_drv != -1)
	{
		char titel[25];
		char lw[] = "X:\\", name[32] = "";
		long l;
		ulong i;
		struct statfs st;
		struct fs_info info;
		
		graf_mouse (BUSY_BEE, 0);
		
		lw[0] = drive_list[active_drv][1];
		
		if (statfs (lw, &st) == 0)
		{
			/* xfs driver type
			 */
			l = Dcntl (FS_INFO, lw, (long) &info);
			if (l == 0)
				strcpy (name, info.type_asc);
			else
			{
				if (st.f_type == 0 || gl_magx)	/* Dcntl(0x104) ist unter MagiC kein MFS_INFO!! */
				{
					l = Dcntl (MX_KER_XFSNAME, lw, (long) name);
					if (l < 0)									
						strcpy (name, fs_names[0]);
				}
				else
					strcpy (name, fs_names[st.f_type]);
			}
			name[10] = '\0';
			set_string (driveinfo, I_SYS, name);
			
			
			/* xfs driver name
			 */
			if (l == 0)
			{
				strcpy (name, info.name);
				strrev (name);
				i = strlen (name);
				while (i < 14)
					name[i++] = ' ';
				name[14] = '\0';
				strrev (name);
			}
			else
				memset (name, '-', 14);
			name[14] = '\0';
			set_string (driveinfo, I_DRIVER, name);
			
			
			/* xfs driver version
			 */
			if (l == 0)
			{
				sprintf (name, "%2li.%02li", info.version >> 16, info.version & 0xff);
				strrev (name);
				i = strlen (name);
				while (i < 10)
					name[i++] = ' ';
				name[10] = '\0';
				strrev (name);
			}
			else
				memset (name, '-', 10);
			name[10] = '\0';
			set_string (driveinfo, I_DRIVER_VERSION, name);
			
			
			/* xfs features
			 */
			l = Dpathconf (lw, 6);
			switch (l)
			{
				case 0:
					set_string (driveinfo, I_CASE, rsc_string (S_JA));
					break;
				case 1:
					set_string (driveinfo, I_CASE, rsc_string (S_NEIN));
					break;
				case 2:
					set_string (driveinfo, I_CASE, rsc_string (S_HALB));
					break;
				default:
					set_string (driveinfo, I_CASE, rsc_string (S_NEIN));
			}
			
			l = Dpathconf (lw, 3);
			if (l == -32)
				l = 12;
			if (l == UNLIMITED)
				set_string (driveinfo, I_NLEN, rsc_string (S_BEL));
			else if (l < 0)
				set_string (driveinfo, I_NLEN, rsc_string (S_UNBK));
			else
				set_long (driveinfo, I_NLEN, l);
			
			l = Dpathconf(lw, 2);
			if (l == -32)
				l = 128;
			if (l == UNLIMITED)
				set_string (driveinfo, I_PLEN, rsc_string (S_BEL));
			else if (l < 0)
				set_string (driveinfo, I_PLEN, rsc_string (S_UNBK));
			else
				set_long (driveinfo, I_PLEN, l);
			
			
			/* filesystem informations
			 */
			i = ((long long) st.f_blocks * st.f_bsize) / 1024UL;
			set_ulong (driveinfo, I_GESAMT, i);
			i = ((long long) (st.f_blocks - st.f_bfree) * st.f_bsize) / 1024UL;
			set_ulong (driveinfo, I_BELEGT, i);
			i = ((long long) st.f_bfree * st.f_bsize) / 1024UL;
			set_ulong (driveinfo, I_FREI, i);
			
			if (st.f_files != -1)
			{
				set_ulong (driveinfo, I_INODES_ALL, st.f_files);
				set_ulong (driveinfo, I_INODES_USED, st.f_files - st.f_ffree);
				set_ulong (driveinfo, I_INODES_FREE, st.f_ffree);
			}
			else
			{
				set_ulong (driveinfo, I_INODES_ALL, 0);
				set_ulong (driveinfo, I_INODES_USED, 0);
				set_ulong (driveinfo, I_INODES_FREE, 0);
			}
			
			
			/* run it
			 */
			sprintf (titel, rsc_string (S_TITEL), lw);
			set_string (driveinfo, I_TITEL, titel);
			simple_mdial (driveinfo, 0);
		}
		else
		{
			do_walert (1, 0, alerts[NO_INFO], PROGRAM);
		}
	}
}

static void
update_pzip (WDIALOG *wd)
{
	static char *name = "IOMEGA ZIP100";
	char tmp[32];
	long r;
	
	r = XHInqDriver (active_drv, tmp, NULL, NULL, NULL, NULL);
	if (r == 0)
		r = strncmp (tmp, name, strlen (name));
	
	set_flag  (wd->tree, LW_PZIP, OF_HIDETREE, (r != 0));
	set_state (wd->tree, LW_PZIP, OS_DISABLED, FALSE);
	
	redraw_wdobj (wd, LW_PZIP);
}

static void
update_special (WDIALOG *wd)
{
	struct control_FAT32 cf32;
	char lw[] = "X:\\";
	long l;
	int ret;
	
	lw[0] = drive_list[active_drv][1];
	
	l = Dcntl (V_CNTR_MODE, lw, -1);
	ret = (l < 0);
	
	tree_state (special, NAME_BOX, OS_DISABLED, (l < 0));
	set_state (special, NAME_GEMDOS, OS_SELECTED, (l == 0));
	set_state (special, NAME_ISO, OS_SELECTED, (l == 1));
	set_state (special, NAME_MSDOS, OS_SELECTED, (l == 2));
	
	cf32.mode = 0;
	l = Dcntl (V_CNTR_FAT32, lw, (long) &cf32);
	if (ret) ret = (l < 0);
	
	tree_state (special, FAT32_BOX, OS_DISABLED, (l < 0));
	if (l == 0)
	{
		set_state (special, FAT32_KOPIE, OS_SELECTED, (cf32.mirr == 0));
		set_long (special, FAT32_ANZAHL, cf32.fats);
		set_long (special, FAT32_AKTIV, cf32.mirr);
	}
	
	set_state (wd->tree, LW_SPECIAL, OS_DISABLED, ret);
	redraw_wdobj (wd, LW_SPECIAL);
}

static void
update_xhdibuttons (WDIALOG *wd)
{
	char tmp[8] = "Lock";
	ushort major, minor;
	int lock = 0;
	int eject = 0;
	long r;
	
	r = XHInqDev (active_drv, &major, &minor, NULL, NULL);
	if (r == 0)
	{
		ulong flags;
		
		r = XHInqTarget (major, minor, NULL, &flags, NULL);
		if (r == 0)
		{
			lock = (flags & XH_TARGET_LOCKABLE);
			eject = (flags & XH_TARGET_EJECTABLE);
			
			if (flags & XH_TARGET_LOCKED)
				strcpy (tmp, "Unlock");
		}
	}
	
	set_state (wd->tree, LW_LOCK, OS_DISABLED, (lock == 0));
	set_string (wd->tree, LW_LOCK, tmp);
	
	set_state (wd->tree, LW_EJECT, OS_DISABLED, (eject == 0));
	
	redraw_wdobj (wd, LW_LOCK);
	redraw_wdobj (wd, LW_EJECT);
}

static void
run_pzip (void)
{
	long l;
	
	l = simple_mdial (pzip, 0);
	
	if (l == PZIP_REMOVE)
	{
	}
	else if (l == PZIP_SET)
	{
	}
}

static void
run_special (void)
{
	long l;
	
	l = simple_mdial (special, 0);
	
	if (l == SPECIAL_SET)
	{
		struct control_FAT32 cf32;
		char lw[] = "X:\\";
		
		lw[0] = drive_list[active_drv][1];
		
		if (get_state (special, NAME_GEMDOS, OS_SELECTED))
			Dcntl (V_CNTR_MODE, lw, 0);
		else if (get_state (special, NAME_ISO, OS_SELECTED))
			Dcntl (V_CNTR_MODE, lw, 1);
		else if (get_state (special, NAME_MSDOS, OS_SELECTED))
			Dcntl (V_CNTR_MODE, lw, 2);
		
		cf32.mode = 1;
		if (get_state (special, FAT32_KOPIE, OS_SELECTED))
			cf32.mirr = 0;
		else
			cf32.mirr = get_long (special, FAT32_AKTIV);
		
		Dcntl (V_CNTR_FAT32, lw, (long) &cf32);
		
		cf32.mode = 0;
		if (Dcntl (V_CNTR_FAT32, lw, (long) &cf32) == 0)
		{
			if (get_state (special, FAT32_KOPIE, OS_SELECTED))
				cf32.mirr = 0;
			else
				cf32.mirr = get_long (special, FAT32_AKTIV);
			
			cf32.mode = 1;
			Dcntl (V_CNTR_FAT32, lw, (long) &cf32);
		}
	}
}

static void
run_lockbutton (void)
{
	ushort major, minor;
	long r;
	
	r = XHInqDev (active_drv, &major, &minor, NULL, NULL);
	if (r == 0)
	{
		ulong flags;
		
		r = XHInqTarget (major, minor, NULL, &flags, NULL);
		if (r == 0)
			r = XHLock (major, minor, !(flags & XH_TARGET_LOCKED), 0);
	}
	
	// if (!r) do_walert (1, 0, "Test", PROGRAM);
}

static void
run_ejectbutton (void)
{
	ushort major, minor;
	long r;
	
	r = XHInqDev (active_drv, &major, &minor, NULL, NULL);
	if (r == 0)
	{
		r = Dlock (1, active_drv);
		if (r == 0)
		{
			r = XHEject (major, minor, 1, 0);
			
			(void) Dlock (0, active_drv);
		}
	}
	
	// if (r) 
}

static void
show_data (WDIALOG *wd)
{
	if (active_drv != -1)
	{
		char lw[] = "X:\\";
		long l;
		
		lw[0] = drive_list[active_drv][1];
		
		l = Dcntl (VFAT_CNFLN, lw, -1);
		set_state (wd->tree, A_VFAT, OS_DISABLED, (l < 0));
		set_state (wd->tree, A_VFAT, OS_SELECTED, (l == 1L));
		
		l = Dcntl (V_CNTR_SLNK, lw, -1);
		set_state (wd->tree, A_SLNK, OS_DISABLED, (l < 0));
		set_state (wd->tree, A_SLNK, OS_SELECTED, (l == 1L));
		
		l = Dcntl (V_CNTR_WB, lw, -1);
		set_state (wd->tree, A_CACHE, OS_DISABLED, (l < 0));
		set_state (wd->tree, A_CACHE, OS_SELECTED, (l == 1L));
		
		l = Dcntl (V_CNTR_WP, lw, -1);
		set_state (wd->tree, A_WP, OS_DISABLED, (l < 0));
		set_state (wd->tree, A_WP, OS_SELECTED, (l == 1L));
		
		redraw_wdobj (wd, A_BOX);
		
		/* buttons */
		
		set_state (wd->tree, LW_INFO, OS_DISABLED, FALSE);
		redraw_wdobj (wd, LW_INFO);
		
		update_pzip (wd);
		update_special (wd);
		update_xhdibuttons (wd);
		
		set_state (wd->tree, SETZEN, OS_DISABLED, FALSE);
		redraw_wdobj (wd, SETZEN);
		
		set_state (wd->tree, SETENDE, OS_DISABLED, FALSE);
		redraw_wdobj (wd, SETENDE);
		
	}
}

/*
 * Laufwerks-Daten Ñndern
 */
static void
set_data (WDIALOG *wd)
{
	if (active_drv != -1)
	{
		char lw[] = "X:\\";
		
		lw[0] = drive_list[active_drv][1];
		
		if (get_state (wd->tree, A_VFAT, OS_SELECTED))
			Dcntl (VFAT_CNFLN, lw, 1);
		else
			Dcntl (VFAT_CNFLN, lw, 0);
		
		if (get_state (wd->tree, A_SLNK, OS_SELECTED))
			Dcntl (V_CNTR_SLNK, lw, 1);
		else
			Dcntl (V_CNTR_SLNK, lw, 0);
		
		if (get_state (wd->tree, A_CACHE, OS_SELECTED))
			Dcntl (V_CNTR_WB, lw, 1);
		else
			Dcntl (V_CNTR_WB, lw, 0);
		
		if (get_state (wd->tree, A_WP, OS_SELECTED))
			Dcntl (V_CNTR_WP, lw, 1);
		else
			Dcntl (V_CNTR_WP, lw, 0);
		
		send_shwdraw (lw[0]);
	}
}

/*
 * Callbacks des Haupt-Dialogs
 */
static void
main_open (WDIALOG *wd)
{
	if (active_drv == -1)			/* nur beim ersten îffnen */
	{
		char str[256];
		
		sprintf (str, rsc_string (S_VERSION), VERSION);
		set_string (wd->tree, MB_VERS, str);
		
		tree_state (wd->tree, A_BOX, OS_DISABLED, TRUE);
		
		if (_app)
		{
			/* aktives Verzeichnis holen und LW selektieren */
			get_path (str, 0);
			active_drv = DriveFromLetter(str[0]);
			set_state (wd->tree, active_drv + LW_A, OS_SELECTED, TRUE);
			show_data (wd);
		}
		else
		{
			set_flag (wd->tree, LW_PZIP, OF_HIDETREE, TRUE);
			set_state (wd->tree, LW_INFO, OS_DISABLED, TRUE);
			set_state (wd->tree, LW_SPECIAL, OS_DISABLED, TRUE);
			set_state (wd->tree, LW_LOCK, OS_DISABLED, TRUE);
			set_state (wd->tree, LW_EJECT, OS_DISABLED, TRUE);
			set_state (wd->tree, SETZEN, OS_DISABLED, TRUE);
			set_state (wd->tree, SETENDE, OS_DISABLED, TRUE);
		}
	}
}

static int
main_exit (WDIALOG *wd, short obj)
{
	bool close = FALSE;
	
	switch (obj)
	{
		case LW_INFO :
			run_info ();
			set_state (wd->tree, obj, OS_SELECTED, FALSE);
			redraw_wdobj (wd, obj);
			update_xhdibuttons (wd);
			break;
		
		case LW_PZIP:
			run_pzip ();
			set_state (wd->tree, obj, OS_SELECTED, FALSE);
			update_pzip (wd);
			break;
		
		case LW_SPECIAL:
			run_special ();
			set_state (wd->tree, obj, OS_SELECTED, FALSE);
			update_special (wd);
			update_xhdibuttons (wd);
			break;
		
		case LW_LOCK:
			run_lockbutton ();
			set_state (wd->tree, obj, OS_SELECTED, FALSE);
			update_xhdibuttons (wd);
			break;
		
		case LW_EJECT:
			run_ejectbutton ();
			set_state (wd->tree, obj, OS_SELECTED, FALSE);
			update_xhdibuttons (wd);
			break;
		
		case SETZEN:
			set_data (wd);
			set_state (wd->tree, obj, OS_SELECTED, FALSE);
			show_data (wd);
			break;
		
		case SETENDE :
			set_data (wd);
			/* weiter wie bei Ende */
		
		case WD_CLOSER:
		case ENDE :
			close = TRUE;
			if (_app)
				gl_done = TRUE;
			break;
		
		default:
			if (obj >= LW_A && obj <= LW_6)
			{
				int new = obj - LW_A;
				
				if (new != active_drv)
				{
					active_drv = new;
					show_data (wd);
				}
			}
	}
	
	return close;
}

/*
 * Event-Verarbeitung 
 */
static void
handle_msg (short *msgbuff)
{
	if (!message_wdial (msgbuff))
	{
		switch (msgbuff[0]) 
		{
			case AC_OPEN:
				open_wdial (maindial, -1, -1);
				if (get_drives ())
				{
					if (active_drv != -1)
						set_state (maindial->tree, LW_A + active_drv, OS_SELECTED, TRUE);
					show_data (maindial);
				}
				break;
				
			case AP_TERM:
				// if (_app)
					/* wir sind kein ACC! */
					gl_done = TRUE;
				break;
		}
	}
}

static void
main_loop (void)
{
	short evset, event, msx, msy, mbutton, kstate, mbreturn, kreturn;
	short msgbuff[8];

	do {
		if (maindial->mode == WD_OPEN)
			evset = (MU_MESAG | MU_BUTTON | MU_KEYBD);
		else
			evset = MU_MESAG;
		
		event = evnt_multi (evset, 1, 1, 1,
					0, 0, 0, 0, 0,	0, 0, 0, 0, 0,
					msgbuff, 0,
					&msx, &msy, &mbutton, &kstate,
					&kreturn, &mbreturn);
		
		if (event & MU_MESAG) 
			handle_msg (msgbuff);
		
		if (event & MU_KEYBD) 
		{
			if (!key_wdial (kreturn, kstate))
			{
				if (kstate == K_CTRL && kreturn == 0x1011)		/* ^Q */
				{
					if (_app)
						gl_done = TRUE;
					else
						close_wdial (maindial);
				}
			}
		}
		
		if (event & MU_BUTTON) 
			click_wdial (mbreturn, msx, msy, kstate, mbutton);
	}
	while (!gl_done);
}

int
main (void)
{
	OBJECT *t, *t2;
	
	// debug_init ("fsetter", TCon, NULL);
	
	init_XHDI ();
	init_app ("fsetter.rsc");
	
	rsrc_gaddr (R_TREE, STRINGS, &strings);
	
	if (gl_naes || !_app)
		menu_register (gl_apid, rsc_string (S_MENU));
	
	
	rsrc_gaddr (R_TREE, WINICON, &t2);
	rsrc_gaddr (R_FRSTR, NO_DRIVE, &alerts); /* _erster_ Alert */
	rsrc_gaddr (R_TREE, INFOBOX, &driveinfo); fix_dial (driveinfo);
	rsrc_gaddr (R_TREE, SPECIALBOX, &special); fix_dial (special);
	rsrc_gaddr (R_TREE, BOX_PZIP, &pzip); fix_dial (pzip);
	
	rsrc_gaddr (R_TREE, MAINBOX, &t); fix_dial (t);
	maindial = create_wdial (t, t2, 0, main_open, main_exit);
	
	
	/* FÅr Info-Dialog und Alerts */
	set_mdial_wincb (handle_msg);
	
	if (_app)
	{
		open_wdial (maindial, -1, -1);
		get_drives ();
	}
	
	main_loop ();
	
	delete_wdial (maindial);
	exit_app (0);
	
	return 0;
}
