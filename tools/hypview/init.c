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

#include <string.h>
#ifdef __GNUC__
	#include <unistd.h>
	#include <osbind.h>
	#include <fcntl.h>
	#include <mint/sysvars.h>
#else
	#include <tos.h>
	#include <sysvars.h>
#endif
#include <gem.h>
#include "diallib.h"
#include "defs.h"


char bootDrive=-1;

static long
getBootDrive(void)
{
	bootDrive = *_bootdev + 'A';
	
	return bootDrive;
}


void Init(void)
{
	short dummy,i;

	/*	GEMDOS-Initialisierung:	*/
	/***************************/

	/*	Falls die Fehler nicht auf STDERR_FILENO ausgegeben werden sollen	*/
	if(*debug_file)
	{
		dummy = (short)Fopen(debug_file,O_WRONLY|O_CREAT|O_APPEND);
		Fforce(STDERR_FILENO,dummy);
	}




	/*	VDI-Initialisierung:	*/
	/************************/

	/*	Standard-Werte fr die VDI-Text-Ausgaben	*/
	vswr_mode(vdi_handle,MD_TRANS);

	if(vq_gdos())								/*	GDOS installiert?	*/
		vst_load_fonts(vdi_handle,0);		/*	Zeichensaetze laden	*/

	vst_font(vdi_handle,font_id);			/*	Font einstellen	*/
	vst_point(vdi_handle,font_pt,&font_w,&font_h,&font_cw,&font_ch);
	sel_font_id = font_id;

/*	Debug("font_w: %d font_h: %d font_cw: %d font_ch: %d",font_w,font_h,font_cw,font_ch);
*/	if(ProportionalFont(&font_w))
		font_cw = font_w;


	/*	Text-Ausgabe-Ausrichtung konfigurieren	*/
	vst_alignment(vdi_handle,TA_LEFT,TA_TOP,&dummy,&dummy);

	/*	Standard-Werte fuer die VDI-Fuell-Funktionen	*/
	vsf_color(vdi_handle,G_WHITE);
	vsf_interior(vdi_handle,FIS_SOLID);
	vsf_perimeter(vdi_handle,0);
	
	/*	Standard-Wert fuer User-spezifisches Linienattribut	*/
	vsl_udsty(vdi_handle,0xAAAA);			/*	Gepunktetes Linienmuster	*/


	/*	AES-Initialisierung:	*/
	/************************/

	/*	Toolbar/Skin laden	*/
	if(*skin_path)
	{
		if (rsrc_load(skin_path))
		{
			RSHDR	*skin_rsh;
			RSHDR	**phdr = (RSHDR **)&aes_global[7];
			OBJECT **pobj;
			skin_rsh = *phdr;
			pobj = (OBJECT **)(((char *)skin_rsh)+skin_rsh->rsh_trindex);
			tree_addr[TOOLBAR] = *pobj;
		}
	#if 0
		if (mt_rsrc_load(skin_path, &skin_global))
		{
			RSHDR	*skin_rsh;

			skin_rsh = (RSHDR	*)skin_global.ap_pmem;
	
			tree_addr[TOOLBAR]=*(OBJECT **)(((char *)skin_rsh)+skin_rsh->rsh_trindex);
		}
	#endif
		else
		{
			Debug("Could not load skin '%s'",skin_path);
			*skin_path = 0;
		}
	}

	/*	Entferne jeglichen Icon-Text	*/	
	for(i = TO_BACK; i < TO_ID; i++)
	{
		if(tree_addr[TOOLBAR][i].ob_type == G_CICON)
		{
			tree_addr[TOOLBAR][i].ob_spec.iconblk->ib_wtext=0;
			tree_addr[TOOLBAR][i].ob_spec.iconblk->ib_htext=0;
		}
	}


	/*	Toolbar ID unsichtbar machen	*/
	tree_addr[TOOLBAR][TO_ID].ob_y=-tree_addr[TOOLBAR][TO_ID].ob_height;
	/*	Grsse des Hintergrundobjekts anpassen	*/
	tree_addr[TOOLBAR][TO_BACKGRND].ob_height=max(tree_addr[TOOLBAR][TO_SEARCHBOX].ob_height,tree_addr[TOOLBAR][TO_BUTTONBOX].ob_height);
	/*	Toolbar Boxen an endgltige Position verschieben	*/
	tree_addr[TOOLBAR][TO_SEARCHBOX].ob_y=(tree_addr[TOOLBAR][TO_BACKGRND].ob_height-tree_addr[TOOLBAR][TO_SEARCHBOX].ob_height)>>1;
	tree_addr[TOOLBAR][TO_BUTTONBOX].ob_y=(tree_addr[TOOLBAR][TO_BACKGRND].ob_height-tree_addr[TOOLBAR][TO_BUTTONBOX].ob_height)>>1;
	


	/*	diverse Initialisierungen:	*/
	/******************************/


	/*	Pfadliste an Startlaufwerk anpassen	*/
	{
	char *cptr=strchr(path_list,':');
		Supexec(getBootDrive);
		while(cptr)
		{
			cptr--;
			if(*cptr=='*')				/*	Pfad anpassen?	*/
				*cptr=bootDrive;

			cptr=strchr(cptr+2,':');
		}
	}

	/*	Lade Marken	*/
	MarkerInit();
}

void Exit(void)
{
	MarkerSaveToDisk();

	if(*skin_path)
		rsrc_free();
	
	if(vq_gdos())
		vst_unload_fonts(vdi_handle,0);
}

