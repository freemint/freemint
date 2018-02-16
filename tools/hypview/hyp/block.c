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

#ifdef __GNUC__
	#include <osbind.h>
#else
	#include <tos.h>
#endif
#include <string.h>
#include "../diallib.h"
#include "../defs.h"
#include "../hyp.h"

extern HYP_DOCUMENT *Hyp;

short HypBlockOperations(DOCUMENT *doc, short op, BLOCK *block, void *param)
{
	LOADED_NODE *node;
	HYP_DOCUMENT *hyp;

	Hyp = hyp = doc->data;
	
	node = (LOADED_NODE *)hyp->entry;

	switch (op)
	{
		case BLK_CHKOP:			/*	Welche Operationen werden unterstuetzt?	*/
		{
			switch (*(short *)param)
			{
				case BLK_ASCIISAVE:
					return(TRUE);	/*	Unterstuetzt	*/

				case BLK_PRINT:
				default:
					return(FALSE);	/*	Nicht unterstuetzt	*/
			}
		}
		case BLK_ASCIISAVE:
		{
			long line;
			long ret, len;
			char line_buffer[LINE_BUF], *src;
			short *handle = (short *)param;
		
			if(!node)						/*	Keine Seite geladen	*/
			{
				Debug("Error: Can't save, no page loaded");
				return(TRUE);
			}
		
			line = block->start.line;

			while((line < doc->lines) && (line <= block->end.line))
			{
				HypGetTextLine(hyp, line, line_buffer);
		
				if(line + 1 < doc->lines)
					strcat(line_buffer,"\r\n");

				if(line == block->start.line)
					src = &line_buffer[block->start.offset];
				else
					src = line_buffer;				

				if(line == block->end.line)
					line_buffer[block->end.offset] = 0;
				
				line++;

				len = strlen(src);
				ret = Fwrite(*handle, len, src);
				if(ret < 0)
				{
					Debug("Error %ld while writing file. Abort.",ret);
					return(TRUE);
				}
				else if(ret != len)
				{
					Debug("Error while writing file. Abort.");
					return(TRUE);
				}
			}
			break;
		}
		case BLK_PRINT:
			break;
	}
	return(FALSE);
}
