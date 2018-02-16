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

#include <stdio.h>
#include <string.h>
#include <gemx.h>
#include "diallib.h"
#include "defs.h"
#include "hyp.h"

static DIALOG *Hypfind_Dialog;
static short HypfindID;
static char filename[DL_PATHMAX];

static short __CDECL
HypfindHandle ( struct HNDL_OBJ_args args)
{
	char ZStr[1024];
	
	switch ( args.obj )
	{
		case HNDL_CLSD:
			return 0;
		case HYPFIND_TEXT:
		  return 0;
		case HYPFIND_ABORT:
			tree_addr[HYPFIND][HYPFIND_ABORT].ob_state &= ~OS_SELECTED;
			return 0;
		case HYPFIND_REF:
			tree_addr[HYPFIND][HYPFIND_REF].ob_state &= ~OS_SELECTED;
			strcpy ( ZStr, tree_addr[HYPFIND][HYPFIND_STRING].ob_spec.tedinfo->te_ptext );
			search_allref ( ZStr, 0 );
 			return 0;
		case HYPFIND_ALL_PAGE:
			tree_addr[HYPFIND][HYPFIND_ALL_PAGE].ob_state &= ~OS_SELECTED;
			strcpy ( ZStr," -p '" );
			strcat ( ZStr, tree_addr[HYPFIND][HYPFIND_STRING].ob_spec.tedinfo->te_ptext );
			strcat ( ZStr,"'" );
			if ( (tree_addr[HYPFIND][HYPFIND_ALL_HYP].ob_state & OS_SELECTED ) == 0 )
			{
				strcat ( ZStr, " ");
				strcat ( ZStr, filename);
			}
			ZStr[0] = (char)strlen( ZStr + 1 );
			HypfindID = shel_write( SHW_EXEC, 0, SHW_PARALLEL, hypfind_path, ZStr);
			if ( HypfindID == 0 )
				HypfindID = -1;
			return 0;
		case HNDL_MESG:
			SpecialMessageEvents(args.dialog , args.events);
		break;
	}

	return 1;
}

void Hypfind( DOCUMENT *doc )
{
	static short first = 0;
    OBJECT *tree = tree_addr[HYPFIND];
   
    if ( !first )
    {
    	strcpy ( tree[HYPFIND_STRING].ob_spec.tedinfo->te_ptext, "" );
    	first = 1;
    	HypfindID = -1;
    }

	if ( HypfindID != -1 )
		return;

	strcpy ( filename, doc->path );
	strcat ( filename, doc->filename );
/*
	if(has_wlffp & 1) {
		Hypfind_Dialog = OpenDialog( HypfindHandle, tree, "Pattern suchen...", -1, -1);
	}
	else
*/
	{
		char ZStr[1024];
		short obj;
		GRECT big, little;

		little.g_x = little.g_y = little.g_w = little.g_h = 0;
		form_center_grect ( tree, &big );
		form_dial ( FMD_START, little.g_x, little.g_y, little.g_w, little.g_h, big.g_x, big.g_y, big.g_w, big.g_h );
		objc_draw_grect ( tree, ROOT, MAX_DEPTH, &big );
		obj = form_do ( tree, HYPFIND_STRING );
		form_dial ( FMD_FINISH, little.g_x, little.g_y, little.g_w, little.g_h, big.g_x, big.g_y, big.g_w, big.g_h );

		switch ( obj )
		{
			case HYPFIND_ABORT:
				tree_addr[HYPFIND][HYPFIND_ABORT].ob_state &= ~OS_SELECTED;
			break;
			case HYPFIND_REF:
				tree_addr[HYPFIND][HYPFIND_REF].ob_state &= ~OS_SELECTED;
				strcpy ( ZStr, tree_addr[HYPFIND][HYPFIND_STRING].ob_spec.tedinfo->te_ptext );
				search_allref ( ZStr, 0 );
	 		break;
			case HYPFIND_ALL_PAGE:
				tree_addr[HYPFIND][HYPFIND_ALL_PAGE].ob_state &= ~OS_SELECTED;
				strcpy ( ZStr," -p '" );
				strcat ( ZStr, tree_addr[HYPFIND][HYPFIND_STRING].ob_spec.tedinfo->te_ptext );
				strcat ( ZStr,"'" );
				if ( (tree_addr[HYPFIND][HYPFIND_ALL_HYP].ob_state & OS_SELECTED ) == 0 )
				{
					strcat ( ZStr, " ");
					strcat ( ZStr, filename);
				}
				ZStr[0] = (char)strlen( ZStr + 1 );
				HypfindID = shel_write( SHW_EXEC, 0, SHW_PARALLEL, hypfind_path, ZStr);
				if ( HypfindID == 0 )
					HypfindID = -1;
			break;
		}
	}
	
}

void HypfindFinsih ( short AppID, short ret )
{
	if ( AppID == HypfindID )
	{
		char ZStr[1024];
		
		HypfindID = -1;
		strcpy ( ZStr, path_list );
		strcat ( ZStr, "hypfind.hyp" );
		OpenFileNW (ZStr, NULL, 0 );
	}
}
