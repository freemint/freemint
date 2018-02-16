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

short AsciiLoad(DOCUMENT *doc, short handle)
{
	long ret,file_len;
	FMT_ASCII *ascii;

	/*	Dateilaenge bestimmen	*/
	file_len = Fseek(0, handle, 2);
	Fseek(0, handle, 0);
	
	/*	Speicher fuer die gesamte (!) Datei reservieren	*/
	ascii = (FMT_ASCII *)Malloc(sizeof(FMT_ASCII) + file_len);
	if (ascii)
	{
		ascii->length = file_len;
		ascii->line_ptr = NULL;

		/*	Datei in den Speicher lesen	*/
		ret = Fread(handle, file_len, &ascii->start);
		if(ret != file_len)
		{
			Mfree(ascii);
			FileError(doc->filename,"while reading (LoadAscii)");
		}
		else
		{
			unsigned char *start = (unsigned char *)&ascii->start;
			unsigned char *end = start + file_len;
			unsigned char *ptr = start;
			unsigned char val;
			long columns = 0;
			short type = F_ASCII;

			/*	Anzahl Linien und Kolonnen initialisieren	*/
			doc->lines = 1;
			doc->height = font_ch;
			doc->columns = 0;
			*end = 0;
			
			/*	In dieser Schleife wird bestimmt ob es sich wirklich
				um einen ASCII Text handelt oder ob es ein Null-Byte
				in den Daten hat.
				Ausserdem wird die Anzahl Linien und Kolonnen ermittelt.	*/
			while (ptr < end)
			{
				val = *ptr;

				if (columns >= ascii_break_len)
				{
					unsigned char *old_ptr = ptr;
					doc->lines++;				/*	Zeile zaehlen	*/

					/*	Suche den Wort-Anfang	*/
					while(columns)
					{
						
						ptr--;		/* was  *ptr--;  ... XXX ozk: looks like a bug? */
						columns--;
						if((*ptr == ' ') || (*ptr == '\t'))
						{
							break;
						}
					}

					if (columns)
					{
						*ptr++ = '\n';		/*	kuenstliches Zeilen-Ende	*/
						doc->columns = max(doc->columns, columns);
					}
					else
					{
						ptr = old_ptr;
						doc->columns = max(doc->columns, ascii_break_len);
					}
					columns = 0;
				}
				else if ((val == '\r') || (val == '\n'))	/*	CR oder LF?	*/
				{
					doc->lines++;				/*	Zeile zaehlen	*/
					doc->columns = max(doc->columns,columns);
					columns = 0;
					ptr++;						/*	Zeilenende ueberspringen	*/
					if ((val == '\r')&&(*ptr == '\n'))
						ptr++;
				}
				else if(val == '\t')				/*	Tab-Stopp?...	*/
				{
					columns += ascii_tab_size - columns % ascii_tab_size;
					ptr++;						/*	Tabulator ueberspringen	*/
				}
				else if (val)
				{
					ptr++;							/*	Normales Zeichen ueberspringen	*/
					columns++;
				}
				else
				{
					/*	... dann handelt es sich um eine binaer Datei	*/
					doc->lines = (file_len + binary_columns) / binary_columns;
					doc->columns = binary_columns;
					columns = 0;
					type = F_BINARY;
					break;
				}
			}

			doc->height = doc->lines * font_ch;
			doc->data = ascii;
			doc->start_line = 0;
			doc->buttons = 0;
			doc->type = type;
			doc->closeProc = AsciiClose;
			doc->gotoNodeProc = AsciiGotoNode;
			doc->getNodeProc = AsciiGetNode;
			doc->clickProc = AsciiClick;

			/*	Handelt es sich um eine ASCII Datei?	*/
			if (type == F_ASCII)
			{
				long line = 1;
				doc->columns = max(doc->columns, columns);
				ptr = (unsigned char *)&ascii->start;
				doc->lines++;

				/*	Zeilen-Tabelle allozieren	*/
				ascii->line_ptr = (char **)Malloc(sizeof(char *)*doc->lines);
				if (ascii->line_ptr)
					ascii->line_ptr[0] = (char *)ptr;

				/*	Diese Schleife konvertiert alle CR/LF in 0/1 und
					speichert zusaetzlich die Zeilenanfaenge	*/
				ptr = start;
				columns = 0;
				while (ptr < end)
				{
					val = *ptr;
	
					if (columns >= ascii_break_len)
					{
						columns = 0;
						if(ascii->line_ptr)
							ascii->line_ptr[line++] = (char *)ptr;
					}
					else if ((val == '\r') || (val == '\n'))	/*	CR oder LF?	*/
					{
						columns = 0;
						*ptr++ = 0;
						if ((val == '\r') && (*ptr == '\n'))
							*ptr++ = 1;
						if (ascii->line_ptr)
							ascii->line_ptr[line++] = (char *)ptr;
					}
					else if (val == '\t')				/*	Tab-Stopp?...	*/
					{
						columns += ascii_tab_size - columns % ascii_tab_size;
						ptr++;						/*	Tabulator ueberspringen	*/
					}
					else
					{
						ptr++;						/*	Normales Zeichen ueberspringen	*/
						columns++;
					}
				}
				ascii->line_ptr[line] = (char *)end;
				
				doc->displayProc = AsciiDisplayPage;
				doc->autolocProc = AsciiAutolocator;
				doc->getCursorProc = AsciiGetCursorPosition;
				doc->blockProc = AsciiBlockOperations;

				/*	ASCII Export supported	*/
				doc->buttons |= BITVAL(TO_SAVE);
			}
			else
			{
				doc->displayProc = BinaryDisplayPage;
				doc->autolocProc = BinaryAutolocator;
				doc->getCursorProc = BinaryGetCursorPosition;
				doc->blockProc = BinaryBlockOperations;
			}

			return(type);
		}
	}
	else
	{
		FileError(doc->filename,"not enough memory");
	}
	return(-1);
}

void AsciiClose(DOCUMENT *doc)
{
	FMT_ASCII *ascii = doc->data;
	if (ascii->line_ptr)
		Mfree(ascii->line_ptr);
	Mfree(ascii);
	doc->data = NULL;
}

void AsciiGotoNode(DOCUMENT *doc, char *chapter, long node)
{
#if DEBUG==ON
	Debug("AsciiGotoNode(Chapter: <%s> / <%ld>)",(chapter?chapter:"NULL"), node);
#endif
}

long AsciiGetNode(DOCUMENT *doc)
{
#if DEBUG==ON
	Debug("AsciiGetNode not implemented.");
#endif
	return(0);
}

void AsciiGetTextLine(char *src, char *end, char *line_buffer)
{
	char *dst = line_buffer;
	char val;

	while (src < end)
	{
		val = *src++;
		if ((val == '\t') && (ascii_tab_size))
		{
			val = ascii_tab_size - (dst - line_buffer) % ascii_tab_size;
			while (val--)
				*dst++ = ' ';
		}
		else if (val)
			*dst++ = val;
		else
			break;
	}
	*dst = 0;
}

void
AsciiDisplayPage(DOCUMENT *doc)
{
	WINDOW_DATA *win = Win;
	FMT_ASCII *ascii = doc->data;
	long line = win->y_pos;
	short x, y;
	unsigned end_y;
	char line_buffer[LINE_BUF];

	wind_get_grect(win->whandle,WF_WORKXYWH, &win->work);

	x = win->work.g_x - (short)(win->x_pos) * font_cw;
	y = win->work.g_y + win->y_offset;

	vswr_mode(vdi_handle,MD_TRANS);
	vst_color(vdi_handle,text_col);
	vst_effects(vdi_handle,0);

	end_y = y + min((unsigned short)(doc->lines - win->y_pos) * font_ch, win->work.g_h - win->y_offset);
	while(y < end_y)
	{
		AsciiGetTextLine(ascii->line_ptr[line], ascii->line_ptr[line + 1], line_buffer);
		line++;
		v_gtext(vdi_handle, x, y, line_buffer);

		y += font_ch;
	}
}

void
AsciiClick(DOCUMENT *doc, EVNTDATA *event)
{
	
}

long AsciiAutolocator(DOCUMENT *doc,long line)
{
	FMT_ASCII *ascii = doc->data;
	char *search = doc->autolocator;
	char *src;
	long len = strlen(search);

	if (!ascii)						/* no file load */
		return -1;
	
	if (doc->autolocator_dir > 0) 
	{
		while (line < doc->lines)
		{
			src = ascii->line_ptr[line];
			if (src)
			{
				while (*src)
				{
					if (strnicmp(src++, search, len) == 0)
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
			src = ascii->line_ptr[line];
			if (src)
			{
				while (*src)
				{
					if (strnicmp(src++, search, len) == 0)
						return line;
				}
			}
			line--;
		}
	}
	return -1;
}

void AsciiGetCursorPosition(DOCUMENT *doc, short x, short y, TEXT_POS *pos)
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
	if (line >= doc->lines)
	{
		pos->line = doc->lines;
		pos->y = doc->lines * font_ch;
		pos->offset = 0;
		pos->x = 0;
		return;
	}

	x += (short)(font_cw * win->x_pos);

	if (doc->type != F_ASCII)
	{
		Debug("Illegal call for this document type");
		return;
	}

	src = ascii->line_ptr[line];
	vst_effects(vdi_handle, 0);

	while (*src)
	{
		i++;
		if ((*src == '\t') && (ascii_tab_size))
		{
			/*	Tabulator expansion	*/
			*dst++=' ';
			if(i % ascii_tab_size == 0)
				src++;
		}
		else
			*dst++ = *src++;
		*dst = 0;

		vqt_extent(vdi_handle,temp,ext);

		if (ext[2] >= x)
		{
			if (ext[2] - x > x - width)
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


short AsciiBlockOperations(DOCUMENT *doc, short op, BLOCK *block, void *param)
{
	FMT_ASCII *ascii=doc->data;

	if(!block->valid)
	{
		Debug("Operation on invalid block");
		return(FALSE);
	}

	switch(op)
	{
		case BLK_CHKOP:			/*	Welche Operationen werden unterstuetzt?	*/
		{
			switch(*(short *)param)
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
			long line;
			long ret, len;
			char line_buffer[LINE_BUF], *src;
			short *handle = (short *)param;
		
		
			line = block->start.line;
			
			while ((line < doc->lines) && (line <= block->end.line))
			{
				AsciiGetTextLine(ascii->line_ptr[line], ascii->line_ptr[line + 1], line_buffer);

				if (line + 1 < doc->lines)
					strcat(line_buffer,"\r\n");

				if (line == block->start.line)
					src = &line_buffer[block->start.offset];
				else
					src = line_buffer;				

				if (line == block->end.line)
					line_buffer[block->end.offset] = 0;
				
				line++;

				len = strlen(src);
				ret = Fwrite(*handle, len, src);
				if (ret < 0)
				{
					Debug("Error %ld while writing file. Abort.",ret);
					return(TRUE);
				}
				else if (ret != len)
				{
					Debug("Error while writing file. Abort.");
					return(TRUE);
				}
			};
			break;
		}
		case BLK_PRINT:
			break;
	}
	return(FALSE);
}
