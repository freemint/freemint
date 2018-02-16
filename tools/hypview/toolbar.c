/*
 * HypView - (c) 2001 - 2006 Philipp Donze
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

#include <gemx.h>
#include "diallib.h"
#include "defs.h"

extern WINDOW_DATA *Win;

void
ToolbarUpdate(DOCUMENT *doc, OBJECT *toolbar, short redraw)
{
	WINDOW_DATA *win = Win;

	/*	Autolocator aktiv?	*/
	if ((doc->buttons & BITVAL(TO_SEARCHBOX)) && doc->autolocator)
	{
		toolbar[TO_BUTTONBOX].ob_flags |= OF_HIDETREE;
		toolbar[TO_SEARCHBOX].ob_flags &= ~OF_HIDETREE;
		toolbar[TO_SEARCH].ob_spec.tedinfo->te_ptext = doc->autolocator;
		return;
	}
	
	toolbar[TO_SEARCHBOX].ob_flags |= OF_HIDETREE;
	toolbar[TO_BUTTONBOX].ob_flags &= ~OF_HIDETREE;
	
	toolbar[TO_MEMORY].ob_state &= ~OS_DISABLED;
	toolbar[TO_INFO].ob_state &= ~OS_DISABLED;
	toolbar[TO_LOAD].ob_state &= ~OS_DISABLED;
	toolbar[TO_MENU].ob_state &= ~OS_DISABLED;
	toolbar[TO_MOREBACK].ob_state |= OS_DISABLED;

	if (CountWindowHistoryEntries(win))
	{
		toolbar[TO_BACK].ob_state &= ~OS_DISABLED;
		toolbar[TO_MOREBACK].ob_state &= ~OS_DISABLED;
	}
	else
	{
		toolbar[TO_BACK].ob_state |= OS_DISABLED;
		toolbar[TO_MOREBACK].ob_state |= OS_DISABLED;
	}

	/*	Gibt es ein Katalog?	*/
	if (*catalog_file)
		toolbar[TO_KATALOG].ob_state &= ~OS_DISABLED;
	else
		toolbar[TO_KATALOG].ob_state |= OS_DISABLED;

	/*	Folgende Buttons sind Typ-spezifisch	*/
	if (doc->buttons & BITVAL(TO_PREVIOUS))
		toolbar[TO_PREVIOUS].ob_state &= ~OS_DISABLED;
	else
		toolbar[TO_PREVIOUS].ob_state |= OS_DISABLED;

	if (doc->buttons & BITVAL(TO_HOME))
		toolbar[TO_HOME].ob_state &= ~OS_DISABLED;
	else
		toolbar[TO_HOME].ob_state |= OS_DISABLED;

	if (doc->buttons & BITVAL(TO_NEXT))
		toolbar[TO_NEXT].ob_state &= ~OS_DISABLED;
	else
		toolbar[TO_NEXT].ob_state |= OS_DISABLED;

	if (doc->buttons & BITVAL(TO_INDEX))
		toolbar[TO_INDEX].ob_state &= ~OS_DISABLED;
	else
		toolbar[TO_INDEX].ob_state |= OS_DISABLED;

	if (doc->buttons & BITVAL(TO_REFERENCES))
		toolbar[TO_REFERENCES].ob_state &= ~OS_DISABLED;
	else
		toolbar[TO_REFERENCES].ob_state |= OS_DISABLED;

	if (doc->buttons & BITVAL(TO_HELP))
		toolbar[TO_HELP].ob_state &= ~OS_DISABLED;
	else
		toolbar[TO_HELP].ob_state |= OS_DISABLED;

	if (doc->buttons & BITVAL(TO_SAVE))
		toolbar[TO_SAVE].ob_state &= ~OS_DISABLED;
	else
		toolbar[TO_SAVE].ob_state |= OS_DISABLED;

	if (!has_wlffp)
	{
		toolbar[TO_MOREBACK].ob_state |= OS_DISABLED;
		toolbar[TO_MEMORY].ob_state |= OS_DISABLED;
		toolbar[TO_REFERENCES].ob_state |= OS_DISABLED;
		toolbar[TO_MENU].ob_state |= OS_DISABLED;
	}

	if (redraw)
	{
		GRECT tbar;
		wind_get_grect(win->whandle, WF_WORKXYWH, &tbar);
		tbar.g_h = win->y_offset;
		wind_update(BEG_UPDATE);
		objc_draw_grect(win->toolbar, TO_BACKGRND, MAX_DEPTH, &tbar);
		wind_update(END_UPDATE);
	}	
}

/* Handle mouse click on toolbar */
void ToolbarClick(DOCUMENT *doc, short obj)
{
	short redraw_button = FALSE;
	WINDOW_DATA *win = Win;

	if (!obj)
		RemoveSearchBox(doc);
	else if (win->toolbar[obj].ob_state & OS_DISABLED)
		return;

	if (check_time)
		CheckFiledate(doc);		/* Check if file has changed */
	
	switch (obj)
	{
		case TO_LOAD:
		{
			SelectFileLoad();
			break;
		}
		case TO_SAVE:
		{
			SelectFileSave(doc);
			break;
		}
		case TO_INDEX:
		{
			GotoIndex(doc);
			break;
		}
		case TO_KATALOG:
		{
			OpenFileSW(catalog_file, NULL, 0);
			break;
		}
		case TO_REFERENCES:
		{
			extern void HypExtRefPopup(DOCUMENT *, short, short);
			short x, y;
			objc_offset(win->toolbar, TO_REFERENCES, &x, &y);
			y += win->toolbar[TO_REFERENCES].ob_height;
			HypExtRefPopup(doc, x, y);
			break;
		}
		case TO_HELP:
		{
			GotoHelp(doc);
			break;
		}
		case TO_MOREBACK:
		{
		short x,y;
			objc_offset(win->toolbar, TO_MOREBACK, &x, &y);
			y += win->toolbar[TO_MOREBACK].ob_height;
			MoreBackPopup(doc, x, y);
			break;
		}
		case TO_BACK:
		{
			GoBack(doc);
			break;
		}
		case TO_NEXT:
		case TO_PREVIOUS:
		case TO_HOME:
		{
			GoThisButton(doc, obj);
			break;
		}
		case TO_MEMORY:
		{
			short x, y;

			objc_offset(win->toolbar, TO_MEMORY, &x, &y);
			y+=win->toolbar[TO_MEMORY].ob_height;
			MarkerPopup ( doc, x, y);
			break;
		}
		case TO_INFO:
		{
			ProgrammInfos(doc);
			break;
		}
		case TO_MENU:
		{
			short x,y, num;
			OBJECT *tree;
			objc_offset(win->toolbar, TO_MENU, &x, &y);
			tree = tree_addr[CONTEXT];
			tree[0].ob_x = x;			
			y+=win->toolbar[TO_MENU].ob_height;
			tree[0].ob_y = y;

			num=form_popup(tree_addr[CONTEXT], 0, 0);
			BlockOperation(doc, num);
			break;
		}
		default:
			break;
	}
	win->toolbar[obj].ob_state &= ~OS_SELECTED;
	if (redraw_button)
		objc_draw_grect(win->toolbar, obj, 1, (GRECT *)&win->toolbar[0].ob_x);
}

void RemoveSearchBox(DOCUMENT *doc)
{
	WINDOW_DATA *win = Win; 

	/*	Is the autolocator/search box displayed?	*/
	if (doc->buttons & BITVAL(TO_SEARCHBOX))
	{
	GRECT tbar;

		doc->buttons &= ~BITVAL(TO_SEARCHBOX);	/* disable it */
		*doc->autolocator = 0;					/* clear autolocator string */

		ToolbarUpdate(doc, win->toolbar, TRUE);	/* update toolbar */

		/* redraw toolbar*/
		wind_get_grect(win->whandle, WF_WORKXYWH, &tbar);
		tbar.g_h = win->y_offset;
		wind_update(BEG_UPDATE);
		objc_draw_grect(win->toolbar, TO_BACKGRND, MAX_DEPTH, &tbar);
		wind_update(END_UPDATE);
	}
}
