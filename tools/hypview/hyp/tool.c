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
#include "../diallib.h"
#include "../hyp.h"

short find_nr_by_title(HYP_DOCUMENT *hyp_doc,char *title)
{
short i=0;
	while(hyp_doc->indextable[i])
	{
		if(hyp_doc->indextable[i]->type<=POPUP)
		{
			if(!stricmp(&hyp_doc->indextable[i]->name,title))
				return(i);
		}
		i++;
	}
	return(-1);
}

char *skip_esc(char *pos)
{
	if(*pos != 27)		/*	Kein ESC-Code	*/
		return (pos);

	pos++;			/*	ESC ueberspringen	*/
	switch (*pos)	/*	Was fuer ein Code?	*/
	{
		case 27:		/*	ESC Zeichen	*/
			pos++;
			break;
		case PIC:	/*	Bild	*/
			pos += 8;
			break;
		case LINE:
		case BOX:
		case RBOX:
			pos += 7;
			break;
		case 37:
		case 39:
			pos += 2;
		case LINK:
		case ALINK:
			pos += (pos[3] - 32 + 1) + 3;
			break;
		case EXTERNAL_REFS:		/*	bis zu 12 Querverweis-Datenbloecke	*/
		case 40:						/*	weitere Datenbloecke	*/
		case 41:						/*	weitere Datenbloecke	*/
		case 42:						/*	weitere Datenbloecke	*/
		case 43:						/*	weitere Datenbloecke	*/
		case 44:						/*	weitere Datenbloecke	*/
		case 45:						/*	weitere Datenbloecke	*/
		case 46:						/*	weitere Datenbloecke	*/
		case 47:						/*	weitere Datenbloecke	*/
			pos += pos[1] - 1;			/*	Daten ueberspringen	*/
			break;
		case WINDOWTITLE:
			pos += strlen(pos) + 1;	/*	Daten ueberspringen	*/
			break;
		case OBJTABLE:				/*	Tabelle mit Objekten und Seiten	*/
			pos += 9;					/*	Daten ueberspringen	*/
			break;
		default:
		{
			if(*(unsigned char *)pos >= 100)
				pos++;
		}
	}
	return(pos);
}
