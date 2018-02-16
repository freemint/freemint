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
#include <gemx.h>
#include "diallib.h"
#include "defs.h"
#include "hyp.h"

static DIALOG *Prog_Dialog;


static short __CDECL
ProgHandle ( struct HNDL_OBJ_args args)
{
	if (args.obj == HNDL_CLSD)
		return 0;
	if (args.obj == PROG_OK) {
		tree_addr[PROGINFO][PROG_OK].ob_state &= ~OS_SELECTED;
		return 0;
	}
	else if(args.obj == HNDL_MESG)
		SpecialMessageEvents(args.dialog , args.events);

	return 1;
}

void ProgrammInfos( DOCUMENT *doc )
{
	GRECT big, little;
	HYP_DOCUMENT *hyp = doc->data;
	OBJECT *tree = tree_addr[PROGINFO];

	strcpy ( tree[PROG_NAME].ob_spec.free_string, PROGRAM_NAME " V" VERSION );
	strcpy ( tree[PROG_DATE].ob_spec.free_string, "from: " __DATE__ );
	
	strncpy ( tree[PROG_DATEI].ob_spec.free_string, doc->filename, 39 );
	tree[PROG_DATEI].ob_spec.free_string[39] = '\0';
	if ( doc->type == F_HYP )
	{
		strncpy ( tree[PROG_THEMA].ob_spec.free_string, hyp->database, 39 );
		tree[PROG_THEMA].ob_spec.free_string[39] = '\0';
		strncpy ( tree[PROG_AUTOR].ob_spec.free_string, hyp->author, 39 );
		tree[PROG_AUTOR].ob_spec.free_string[39] = '\0';
		if ( strncmp ( hyp->version, "$VER: ", 6) == 0 )
			strncpy ( tree[PROG_VERSION].ob_spec.free_string, &hyp->version[6], 39 );
		else
			strncpy ( tree[PROG_VERSION].ob_spec.free_string, hyp->version, 39 );
		tree[PROG_VERSION].ob_spec.free_string[39] = '\0';
  }
  else
  {
		tree[PROG_THEMA].ob_spec.free_string[0] = '\0';
		tree[PROG_AUTOR].ob_spec.free_string[0] = '\0';
		tree[PROG_VERSION].ob_spec.free_string[0] = '\0';
  }
  
	if(has_wlffp & 1) {
		Prog_Dialog = OpenDialog( ProgHandle, tree, "Programminfo...", -1, -1);
	}
	else
	{
		little.g_x = little.g_y = little.g_w = little.g_h = 0;
		form_center_grect ( tree, &big );
		form_dial ( FMD_START, little.g_x, little.g_y, little.g_w, little.g_h, big.g_x, big.g_y, big.g_w, big.g_h );
		objc_draw_grect ( tree, ROOT, MAX_DEPTH, &big );
		form_do ( tree, 0 );
		form_dial ( FMD_FINISH, little.g_x, little.g_y, little.g_w, little.g_h, big.g_x, big.g_y, big.g_w, big.g_h );
	}
}
