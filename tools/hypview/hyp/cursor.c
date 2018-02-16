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
extern WINDOW_DATA *Win;

void HypGetCursorPosition(DOCUMENT *doc, short x, short y, TEXT_POS *pos)
{
	WINDOW_DATA *win = Win;
	LOADED_NODE *node;
	LINEPTR *line_ptr;
	short line = 0;
	long i = 0;
	long sy;
	short curr_txt_effect = 0;
	short x_pos = font_cw;
	short width = 0;
	short ext[8];
	char *src, *dst;
	char temp[LINE_BUF];
	HYP_DOCUMENT *hyp;

	if(doc->type != F_HYP)
	{
		Debug("Illegal call for this document type");
		return;
	}

	Hyp = hyp = doc->data;
	node = (LOADED_NODE *)hyp->entry;

	sy = -(win->y_pos * font_ch);
	line_ptr = node->line_ptr;

	while (line < node->lines)
	{
		long y2;
		y2 = sy + line_ptr->y + line_ptr->h;
		if ((sy + line_ptr->y) >= y || (y2 > y))
		{
			sy += line_ptr->y;
			break;
		}
		sy = y2;
		line_ptr++;
		line++;
	}

	if (line >= node->lines)
		return;
	
	src = line_ptr->txt;
	
	if (x < font_cw || !src)
	{
		pos->line = line;
		pos->y = sy + (win->y_pos * font_ch);
		pos->offset = 0;
		pos->x = 0;
		return;
	}

	/*	Standard Text-Effekt	*/
	vst_effects(vdi_handle, 0);
	dst = temp;

	while (*src)
	{
		if(*src == 27)					/*	ESC-Sequenz ??	*/
		{
			*dst = 0;
			/*	ungeschriebene Daten?	*/
			if (*temp)
			{
				/*	noch fehlende Daten ausgeben	*/
				vqt_extent(vdi_handle, temp, ext);
				width = (ext[2] + ext[4]) >> 1;
				if (x_pos + width >= x)
					goto go_back;

				x_pos += width;
			}
			dst = temp;
			src++;

			switch(*src)
			{
				case 27:				/*	ESC	*/
					*dst++ = *src++;
					i++;
					break;
				case LINK:
				case 37:
				case ALINK:
				case 39:
				{
					unsigned short idx;		/*	Index auf die Zielseite	*/
					
					if(*src & 1)			/*	Zeilennummer ueberspringen	*/
						src += 2;

					idx = DEC_255(&src[1]);
					src += 3;


					/*	Verknuepfungstext mit entsprechendem Texteffekt ausgeben	*/
					vst_color(vdi_handle, link_col);
					vst_effects(vdi_handle, link_effect|curr_txt_effect);

					/*	Verknuepfungstext ermitteln und ausgeben	*/
					if (*src == 32)		/*	Kein Text angegeben	*/
					{
						strcpy(temp, &hyp->indextable[idx]->name);
						src++;
					}
					else
					{
						short num = (*(unsigned char *)src) - 32;
						strncpy(temp, src + 1, num);
						temp[num] = 0;
						src += num + 1;
					}

					i += strlen(temp);
					vqt_extent(vdi_handle, temp, ext);
					width = (ext[2] + ext[4]) >> 1;
					if(x_pos + width >= x)
						goto go_back;

					x_pos += width;

					vst_color(vdi_handle, text_col);
					vst_effects(vdi_handle, curr_txt_effect);
					break;
				}
				default:
				{
					if (*(unsigned char *)src >= 100)		/*	Text-Attribute	*/
					{
						curr_txt_effect = *((unsigned char *)src) - 100;
						vst_effects(vdi_handle, curr_txt_effect);
						src++;
					}
					else
						src = skip_esc(src-1);
					src = src;
				}
			}
		}
		else
		{
			*dst++ = *src++;
			i++;
		}
	}
	*dst = 0;

	if (*temp)		/*	Noch ungeschriebene Daten?	*/
	{
		vqt_extent(vdi_handle, temp,ext);
		width = (ext[2] + ext[4]) >> 1;
		if(x_pos + width >= x)
			goto go_back;
		x_pos += width;
		*temp = 0;	
	}
	width = 0;
	
go_back:
	if (*temp && x_pos + width > x)
	{
		dst = temp;
		while (*dst)
			dst++;
		dst--;
	
		while (dst >= temp)
		{
			i--;
			*dst = 0;
			vqt_extent(vdi_handle, temp,ext);
/*				Debug("%d %d %d %d %d %d <%s>",ext[0],ext[1],ext[2],ext[3],ext[4],ext[5],temp);
*/				if (x_pos + ext[2] < x)
			{
				if (x - (x_pos + ext[2]) > (x_pos + width) - x)
				{
					i++;
					break;
				}
				width = (ext[2] + ext[4]) >> 1;
				break;
			}
			width =(ext[2] + ext[4]) >> 1;
			dst--;
		}

		if (dst >= temp)
			pos->x = x_pos + width;
		else
			pos->x = x_pos;
	}
	else
		pos->x = x_pos + width;
	
	if (i == 0)
		pos->x = 0;
	pos->offset = i;
	pos->line = line;
	pos->y = sy + (win->y_pos * font_ch);
}
