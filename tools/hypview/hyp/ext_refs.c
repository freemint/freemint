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
#include "../diallib.h"
#include "../defs.h"
#include "../hyp.h"


extern HYP_DOCUMENT *Hyp;

short HypCountExtRefs(LOADED_NODE *entry)
{
	char *pos = entry->start;
	short count = 0;
	
	while(*pos==27)
	{
		if(pos[1]==EXTERNAL_REFS)
			count++;
		pos=skip_esc(pos);
	}
	return(count);
}

void HypExtRefPopup(DOCUMENT *doc, short x, short y)
{
	OBJECT *tree=tree_addr[EMPTYPOPUP];
	char *pos;
	short i=EM_1,m;
	long len=0;
	HYP_DOCUMENT *hyp;
	
	Hyp = hyp = doc->data;

	pos=((LOADED_NODE *)hyp->entry)->start;
	while(*pos==27)
	{
		if(pos[1]==EXTERNAL_REFS)
		{
			tree[i].ob_flags &= ~OF_HIDETREE;
			tree[i++].ob_spec.free_string=&pos[5];
			len=max(strlen(&pos[5])+1,len);
		}
		pos=skip_esc(pos);
	}
	/* Hide unused entries */	
	m=i;
	while(i<=EM_12)
		tree[i++].ob_flags |= OF_HIDETREE;
	i=m;
	
	len=len*pwchar;
	tree[EM_BACK].ob_x = x;
	tree[EM_BACK].ob_y = y + 5;
	tree[EM_BACK].ob_width=(short)len;
	tree[EM_BACK].ob_height=(i-EM_1)*phchar;
	
	while(--i>=EM_1)
	{
		tree[i].ob_width=(short)len;
		tree[i].ob_height=phchar;
	}

	i=form_popup(tree_addr[EMPTYPOPUP], 0, 0);
	if(i>0)
	{
		pos=((LOADED_NODE *)hyp->entry)->start;
		while(*pos==27)
		{
			if(pos[1]==EXTERNAL_REFS)
			{
				i--;
				if(i==0)
				{
					INDEX_ENTRY *entry;
					entry = hyp->indextable[DEC_255(&pos[3])];

					if(entry->type == EXTERNAL_REF)
						HypOpenExtRef(&entry->name, FALSE);
					else
						Debug("Illegal External reference!");
				}
			}
			pos=skip_esc(pos);
		}
	}
}


/*
 *	Oeffnet einen Link <name> der als Externe Referenz angegeben wurde.
 *	D.h. er hat die Form: <Datei.hyp>/<Kapitelname>
 *	Falls es sich um die Form: <absoluter Pfad zur Datei.hyp> handelt,
 *	so wird kein Kapitel-Trennzeichen (='/') umgewandelt.
 */
void HypOpenExtRef(char *name, short new_window)
{
char *cptr;
char temp[DL_PATHMAX];

	strcpy(temp,name);
	
	/*
		Kein Doppelpunkt im Namen? => relativer Pfad
		Doppelpunkt an zweiter Position => absoluter Pfad
		sonst => kein Pfad, also auch kein Kapitelname suchen
	*/
	cptr=strchr(temp,':');
	if((cptr==NULL) || (cptr==&temp[1]))
	{
		/*	Kapitelseparator von '/' nach ' ' konvertieren	*/
		cptr=strchr(temp,'/');
		if(cptr)
			*cptr++=0;
	}

	if(new_window)
		OpenFileNW(temp,cptr,0);
	else
		OpenFileSW(temp,cptr,0);
}
