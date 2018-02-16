/*
 * HypView - (c)      - 2006 Philipp Donze
 *               2006 -      Philipp Donze & Odd Skancke
 *
 * A replacement hypertext viewer
 *
 * This file is part of HypView.
 *
 * HypView is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HypView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HypView; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef __GNUC__
	#include <mintbind.h>
#else
	#include <tos.h>
#endif
#include <gem.h>
#include "diallib.h"
#include "defs.h"

/* Ozk:
 * This belongs in gemlib .. ask Arnaud to add this later...
 */
#ifndef _AESrshdr
#define _AESrshdr *(RSHDR **)&aes_global[7]
#endif

extern short _app;

#if DEBUG_LOG == YES
short debug_handle = 0, old_stderr;
#endif

short	ap_id;
short	aes_handle,pwchar,phchar,pwbox,phbox;
short	has_agi=0,has_wlffp=0, has_iconify=0;
#if USE_GLOBAL_VDI==YES
short	vdi_handle;
short	workin[11];
short	workout[57];
short	ext_workout[57];
#if SAVE_COLORS==YES
RGB1000 save_palette[256];
#endif
#endif
#if USE_LONGFILENAMES==YES
short	old_domain;
#endif

#if LANGUAGE==GERMAN
char	*err_loading_rsc="[1][Fehler beim Laden von |'"RESOURCE_FILE"'.][Abbruch]";
#elif LANGUAGE==FRENCH
char	*err_loading_rsc="[1][Impossible de trouver '"RESOURCE_FILE"'.][Annuler]";
#else
char	*err_loading_rsc="[1][Error while loading |'"RESOURCE_FILE"'.][Cancel]";
#endif
char	*resource_file=RESOURCE_FILE;
RSHDR	*rsh;
OBJECT	**tree_addr;
short	tree_count;
char	**string_addr;
#if USE_MENU==YES
OBJECT	*menu_tree;
#endif

KEYTAB *key_table;

short DoAesInit(void);
short DoInitSystem(void);
void DoExitSystem(void);

short DoAesInit(void)
{
#if DEBUG_LOG == YES
	long ret;
	ret = Fopen(PROGRAM_NAME ".log",O_WRONLY|O_CREAT|O_APPEND);
	
	if(ret >= 0)
	{
		debug_handle = (short)ret;
		old_stderr = (short)Fdup(STDERR_FILENO);
		ret = Fforce(STDERR_FILENO, debug_handle);
		if(ret < 0L)
			puts("Error in Fforce()");
		else
			Debug(PROGRAM_NAME" startet");
	}
	else
		puts("Unable to create LOG file");
#endif

	ap_id = appl_init();
	if(ap_id < 0)
		return(TRUE);

	if(rsrc_load(resource_file) == 0)
	{
		form_alert(1, err_loading_rsc);
		appl_exit();
		return(TRUE);
	}
	
	rsh = _AESrshdr;
	
	tree_addr = (OBJECT **)(((char *)rsh) + rsh->rsh_trindex);
	tree_count = rsh->rsh_ntree;
	string_addr =(char **)((char *)rsh + rsh->rsh_frstr);

	has_agi = (_AESversion == 0x399) || (_AESversion >= 0x400) || (appl_find("?AGI") >= 0);

#if USE_ITEM == YES
	if(has_agi)
	{
		/*
			Bit 0:           wdlg_xx()-Funktionen sind vorhanden (1)
			Bit 1:           lbox_xx()-Funktionen sind vorhanden (1)
			Bit 2:           fnts_xx()-Funktionen sind vorhanden (1)
			Bit 3:           fslx_xx()-Funktionen sind vorhanden (1)
			Bit 4:           pdlg_xx()-Funktionen sind vorhanden (1)
		*/
		appl_getinfo(7,&has_wlffp,NULL,NULL,NULL);
		
		appl_getinfo(11,&has_iconify,NULL,NULL,NULL);
		has_iconify &= 0x80;
	}

	/*	ueberpruefe ob WDLG Aufrufe vorhanden sind	*/
#if USE_DIALOG==YES
/* [GS] 0.35.2a alt:
	if(!(has_wlffp & 0x01))
	{
		form_alert(1,tree_addr[DIAL_LIBRARY][DI_WDIALOG_ERROR].ob_spec.free_string);
		return(TRUE);
	}
*/
#endif

	iconified_tree = tree_addr[DIAL_LIBRARY];
	iconified_name = tree_addr[DIAL_LIBRARY][DI_ICONIFY_NAME].ob_spec.free_string;
/* Zentriere Iconify-Icon
	if(has_iconify)
	{
	GRECT icon={0,0,72,72};
	short w,h,dummy;
		if(wind_get(0,WF_ICONIFY,&dummy,&w,&h,&dummy))
		{
			icon.g_w=w;
			icon.g_h=h;
		}
		wind_calc(WC_WORK,NAME,&icon,&icon);
		tree_addr[DIAL_LIBRARY][DI_ICON].ob_x=(icon.g_w-tree_addr[DIAL_LIBRARY][DI_ICON].ob_width)>>1;
		tree_addr[DIAL_LIBRARY][DI_ICON].ob_y=(icon.g_h-tree_addr[DIAL_LIBRARY][DI_ICON].ob_height)>>1;
	}
*/
#endif

	aes_handle=graf_handle(&pwchar,&phchar,&pwbox,&phbox);

#if USE_MENU==YES
	menu_tree=tree_addr[MENU];
#endif

	return(FALSE);
}

short DoInitSystem(void)
{
#if USE_GLOBAL_VDI==YES
	{
	short i;
		for(i=0;i<10;workin[i++]=1);
		workin[10]=2;
		vdi_handle=aes_handle;
		v_opnvwk(workin,&vdi_handle,workout);
		if(!vdi_handle)
		{
			form_alert(1,tree_addr[DIAL_LIBRARY][DI_VDI_WKS_ERROR].ob_spec.free_string);
			rsrc_free();
			appl_exit();
			return(TRUE);
		}
		vq_extnd(vdi_handle,1,ext_workout);

#if SAVE_COLORS==YES
		for(i=0;i<256;i++)
			vq_color(vdi_handle,i,1,(short *)&save_palette[i]);
#endif
	}
#endif
#if USE_LONGFILENAMES==YES
	old_domain = (short)Pdomain(1);
#endif
#if USE_LONGEDITFIELDS==YES
	DoInitLongEdit();
#endif
#if USE_BUBBLEGEM==YES
	DoInitBubble();
#endif
#if USE_AV_PROTOCOL>=2
	if(_app)											/*	Ist kein Accessory?	*/
		DoAV_PROTOKOLL(AV_P_QUOTING|AV_P_VA_START|AV_P_AV_STARTED);
#endif

	key_table=Keytbl(NIL,NIL,NIL);			/*	Key-Table ermitteln	*/

	return(FALSE);
}

void DoExitSystem(void)
{
#if USE_AV_PROTOCOL>=2
	if(_app)											/*	Ist kein Accessory?	*/
		DoAV_EXIT();
#endif
#if USE_BUBBLEGEM==YES
	DoExitBubble();
#endif
#if USE_LONGEDITFIELDS==YES
	DoExitLongEdit();
#endif
#if USE_LONGFILENAMES==YES
	(void)Pdomain(old_domain);
#endif
#if USE_GLOBAL_VDI==YES
	if(vdi_handle)
	{
#if SAVE_COLORS==YES
	short i;
		for(i=0;i<256;i++)
			vs_color(vdi_handle,i,(short *)&save_palette[i]);
#endif
		v_clsvwk(vdi_handle);
	}
#endif

	rsrc_free();
	appl_exit();

#if DEBUG_LOG == YES
	Fclose(debug_handle);
	Fforce(STDERR_FILENO,old_stderr);
#endif
}
