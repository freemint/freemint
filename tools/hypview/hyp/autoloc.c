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

#include <string.h>
#include "../diallib.h"
#include "../defs.h"
#include "../hyp.h"

extern HYP_DOCUMENT *Hyp;

LINEPTR *
HypGetYLine(LOADED_NODE *node, long y)
{
	int i;
	LINEPTR *line_ptr = node->line_ptr;
	long y1 = 0, y2;

	for (i = 0; i < node->lines; i++)
	{
		y1 += line_ptr->y;
		y2 = y1 + line_ptr->h;
		if (y >= y1 && y < y2)
			break;
		y1 = y2;
		line_ptr++;
	}
	
	if (i == node->lines)
		return NULL;
	return line_ptr;
}

/* Return the real Y value form a line                                         */
/* You can not only make "line * win->y_raster", because the picture have also */
/* lines. The first line have in his "y" value a offest, otherwise the are     */
/* zero.                                                                       */

short
HypGetLineY ( LOADED_NODE *node, long line )
{
	long i, sy;
	LINEPTR *line_ptr = node->line_ptr;

	i = sy = 0;
	while ( i < line)
	{
		sy  +=line_ptr->y + line_ptr->h;
		line_ptr++;
		i++;
	}
	return sy;
}

/* Return the real Textline                                                    */
/* It is the same Problme like above. If you get a Line you can't use this     */
/* direct to get for example the "line_ptr[line].txt".                         */
/* You must be convert the value "line" befor you can use them.                */

long
HypGetRealTextLine ( LOADED_NODE *node, short y )
{
	long i, sy;
	LINEPTR *line_ptr = node->line_ptr;

	i = sy = 0;
	while ( y > sy )
	{
		sy  +=line_ptr->y + line_ptr->h;
		line_ptr++;
		i++;
	}
  return i;
}

void
HypGetTextLine(HYP_DOCUMENT *hyp, long line, char *dst )
{
	char *src;

	
	src = ((LOADED_NODE *)hyp->entry)->line_ptr[line].txt;


  if(!src)
	{
		*dst = 0;
		return;
	}

	while(*src)
	{
		if (*src == 27)					/*	ESC-Sequenz ??	*/
		{
			*dst = 0;						/*	Pufferende schreiben	*/
			src++;
			switch (*src)
			{
				case 27:				/*	ESC	*/
					*dst++ = 27;
					break;
				case LINK:
				case 37:
				case ALINK:
				case 39:
				{
					unsigned short i;			/*	Index auf die Zielseite	*/

					if (*src & 1)			/*	Zeilennummer ueberspringen	*/
						src += 2;

					i = DEC_255(&src[1]);
					src += 3;


					/*	Verknuepfungstext ermitteln und ausgeben	*/
					if(*src == 32)		/*	Kein Text angegeben	*/
					{
						strcpy(dst, &hyp->indextable[i]->name);
						src++;
					}
					else
					{
						unsigned char num = (*(unsigned char *)src)-32+1;
						strncat(dst, src + 1, num - 1);
						src += num;
					}
					dst += strlen(dst);

					break;
				}
				default:
				{
					src = skip_esc(src - 1);
				}
			}
		}
		else
			*dst++ = *src++;
	}
	*dst = 0;				/*	Pufferende schreiben	*/
}


long
HypAutolocator(DOCUMENT *doc, long line)
{
	char *search = doc->autolocator;
	char *src;
	short y;
	WINDOW_DATA *win = Win;
	LOADED_NODE *node;
	char temp[LINE_BUF];
	long len = strlen(search);
	HYP_DOCUMENT *hyp;
	
	Hyp = hyp = doc->data;

	node = (LOADED_NODE *)hyp->entry;

	if (!node)								/*	no node loaded	*/
		return -1;

	y =  line * win->y_raster;
  line = HypGetRealTextLine ( node, y );

	if (doc->autolocator_dir > 0) 
	{
		while (line < node->lines)
		{
			HypGetTextLine(hyp, line, temp );
			src = temp;
			while (*src)
			{
				if (strnicmp(src++, search, len) == 0)
				{
          y = HypGetLineY ( node, line );
					line = y / win->y_raster;          /* Wirkliche Linie im Textfenster ermitteln */
					return line;
				}
			}
			line++;
		}
	}
	else
	{
		while (line > 0)
		{
			HypGetTextLine(hyp, line, temp );
			src = temp;
			while (*src)
			{
				if (strnicmp(src++, search, len) == 0)
				{
          y = HypGetLineY ( node, line );
					line = y / win->y_raster;          /* Wirkliche Linie im Textfenster ermitteln */
					return line;
				}
			}
			line--;
		}
	}		
	return -1;
}
