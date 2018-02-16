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
#ifdef __GNUC__
	#include <osbind.h>
#else
	#include <tos.h>
#endif
#include <gem.h>
#include "../diallib.h"
#include "../defs.h"
#include "ascii.h"


extern WINDOW_DATA *Win;

void BinaryDisplayPage(DOCUMENT *doc)
{
	FMT_ASCII *ascii = doc->data;
	short x,y;
	char line_buffer[1024];
	char *src = &ascii->start;
	char *end = src + ascii->length;
	WINDOW_DATA *win;

	win = Win;

	wind_get_grect(win->whandle, WF_WORKXYWH, &win->work);

	x = win->work.g_x - (short)(win->x_pos) * font_cw;
	y = win->work.g_y + win->y_offset;

	src += win->y_pos * binary_columns;
	end = min(end, src + ((unsigned long)win->work.g_h / font_ch) * binary_columns);

	vswr_mode(vdi_handle, MD_TRANS);
	vst_color(vdi_handle, G_BLACK);
	vst_effects(vdi_handle, 0);

	while (src < end)
	{
		short i;
		char *dst;
		dst = line_buffer;
		i = (short)min(binary_columns,end - src + 1);
		while (i--)
		{
			if (*src)
				*dst++=*src++;
			else
			{
				*dst++=' ';
				src++;
			}
		}
		*dst = 0;
		v_gtext(vdi_handle,x,y,line_buffer);
		y += font_ch;
	}
}


long
BinaryAutolocator(DOCUMENT *doc, long line)
{
	FMT_ASCII *ascii = doc->data;
	char *search = doc->autolocator;
	char *src, *end;
	long len = strlen(search);

	if (!ascii)								/*	Keine Datei geladen	*/
		return -1;
	
	end = &ascii->start + ascii->length - len;
	src = &ascii->start + line * binary_columns;
	if (doc->autolocator_dir > 0) 
	{
		while (src <= end)
		{
			if (strnicmp(src, search, len) == 0) {
				return (src - 1 - &ascii->start) / binary_columns;
			}
			src++;
		}
	}
	else
	{
		while (src >= &ascii->start)
		{
			if (strnicmp(src, search, len) == 0) {
				return (src - 1 - &ascii->start) / binary_columns;
			}
			src--;
		}
	}
	return -1;
}

void
BinaryGetCursorPosition(DOCUMENT *doc, short x, short y, TEXT_POS *pos)
{
	WINDOW_DATA *win = Win;
	FMT_ASCII *ascii = doc->data;
	long line = y / font_ch + win->y_pos;
	long i = 0;
	char temp[LINE_BUF];
	char *src;
	char *dst = temp;
	short ext[8];
	short width = 0;
	
	/*	Falls die Zeile nicht mehr im Dokument enthalten ist...	*/
	if(line < 0)
	{
		pos->line = 0;
		pos->y = 0;
		pos->offset = 0;
		pos->x = 0;
		return;
	}
	if(line >= doc->lines)
	{
		pos->line = doc->lines;
		pos->y = doc->lines * font_ch;
		pos->offset = 0;
		pos->x = 0;
		return;
	}

	x += (short)(font_cw * win->x_pos);

	if (doc->type != F_BINARY)
	{
		Debug("Illegal call for this document type");
		return;
	}

	src = &ascii->start + line * binary_columns;
	vst_effects(vdi_handle,0);

	while (i < binary_columns)
	{
		if (*src)
			*dst++ = *src++;
		else
		{
			*dst++ = ' ';
			src++;
		}
		*dst = 0;
		i++;

		vqt_extent(vdi_handle,temp,ext);

		if (ext[2] >= x)
		{
			if(ext[2] - x > x - width)
			{
				i--;
				break;
			}
			width = (ext[2] + ext[4]) >> 1;
			break;
		}
		width = (ext[2] + ext[4]) >> 1;
	}

	pos->line = line;
	pos->y = line * font_ch;
	pos->offset = i;
	pos->x = width;
}


short BinaryBlockOperations(DOCUMENT *doc, short op, BLOCK *block, void *param)
{
	FMT_ASCII *ascii = doc->data;

	if(!block->valid)
	{
		Debug("Operation on invalid block");
		return(FALSE);
	}

	switch(op)
	{
		case BLK_CHKOP:			/*	Welche Operationen werden unterstuetzt?	*/
		{
			switch (*(short *)param)
			{
				case BLK_ASCIISAVE:
				{
					return(TRUE);	/*	Unterstuetzt	*/
				}
				case BLK_PRINT:
				default:
				{
					return(FALSE);	/*	Nicht unterstuetzt	*/
				}
			}
		}
		case BLK_ASCIISAVE:
		{
			long ret, len;
			char *src;
			short *handle=(short *)param;
		
			src = &ascii->start + block->start.line * binary_columns + block->start.offset;
			len = (block->end.line - block->start.line) * binary_columns +
						block->end.offset - block->start.offset;
			ret=Fwrite(*handle,len,src);
			if(ret<0)
			{
				Debug("Error %ld while writing file. Abort.",ret);
				return(TRUE);
			}
			else if(ret!=len)
			{
				Debug("Error while writing file. Abort.");
				return(TRUE);
			}
			break;
		}
		case BLK_PRINT:
			break;
	}
	return(FALSE);
}
