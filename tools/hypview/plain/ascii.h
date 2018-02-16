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


typedef struct
{
	long length;
	char **line_ptr;
	char start;
}FMT_ASCII;


/*
 *		Ascii.c
 */
void AsciiClose(DOCUMENT *doc);
void AsciiGotoNode(DOCUMENT *doc, char *chapter, long node);
long AsciiGetNode(DOCUMENT *doc);
void AsciiGetTextLine(char *src, char *end,char *line_buffer);
void AsciiDisplayPage(DOCUMENT *doc);
void AsciiClick(DOCUMENT *doc, EVNTDATA *event);
long AsciiAutolocator(DOCUMENT *doc,long line);
void AsciiGetCursorPosition(DOCUMENT *doc,short x, short y, TEXT_POS *pos);
short AsciiBlockOperations(DOCUMENT *doc, short op, BLOCK *block, void *param);

/*
 *		Binary.c
 */
void BinaryDisplayPage(DOCUMENT *doc);
long BinaryAutolocator(DOCUMENT *doc,long line);
void BinaryGetCursorPosition(DOCUMENT *doc, short x, short y, TEXT_POS *pos);
short BinaryBlockOperations(DOCUMENT *doc, short op, BLOCK *block, void *param);
