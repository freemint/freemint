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

#include <gem.h>
#include "diallib.h"

#if USE_BUBBLEGEM == YES
#include "bubble/bgh.h"

char *bub_fname=BUBBLEGEM_FILE;

void *Help;

void DoInitBubble(void);
void DoExitBubble(void);
void Bubble(short mx,short my);


void DoInitBubble(void)
{
	Help=BGH_load(bub_fname);
}

void DoExitBubble(void)
{
	BGH_free(Help);
}

void
Bubble(short mx,short my)
{
	CHAIN_DATA *ptr;
	short gruppe,index,typ=-1;

	ptr=find_ptr_by_whandle(wind_find(mx,my));

	if(!ptr)
		return;

	if(ptr->type==WIN_DIALOG)
	{
	OBJECT *tree=((DIALOG_DATA *)ptr)->obj;
	short i;
		if(ptr->status & WIS_ICONIFY)
		{
		GRECT box;
			tree=tree_addr[DIAL_LIBRARY];
			wind_get_grect(ptr->whandle, WF_WORKXYWH,&box);
			tree_addr[DIAL_LIBRARY][0].ob_x=box.g_x;
			tree_addr[DIAL_LIBRARY][0].ob_y=box.g_y;
		}

		index=objc_find(tree,ROOT,MAX_DEPTH,mx,my);
		if(index==-1)
			return;

		for(i=0;i<tree_count;i++)
		{
			if(tree_addr[i]==tree)
				gruppe=i;
		}
		typ=BGH_DIAL;
	}
	else if(ptr->type==WIN_WINDOW)
	{
	short data[4];
		data[0]=mx;
		data[1]=my;
		data[2]=-1;
		((WINDOW_DATA *)ptr)->proc((WINDOW_DATA *)ptr,WIND_BUBBLE,data);
		if(data[2]!=-1)
		{
			gruppe=data[2];
			index=data[3];
			typ=BGH_USER;
		}
	}

	if(typ!=-1)
		BGH_action(Help,mx,my,typ,gruppe,index);
}

#endif
