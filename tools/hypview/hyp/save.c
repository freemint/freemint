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

#if 0
static short
SaveAsAscii(DOCUMENT *doc, short handle)
{
	LOADED_NODE *node;
	char temp[LINE_BUF];
	long ret,len,line;
	HYP_DOCUMENT *hyp;

	Hyp = hyp = doc->data;

	node = (LOADED_NODE *)hyp->entry;
	if(!node)									/*	Keine Seite geladen	*/
	{
		Debug("Error: Can't save, no page loaded");
		return(TRUE);
	}

	for(line=0;line<node->lines;line++)
	{
		HypGetTextLine(hyp, line,temp);

		strcat(temp,"\r\n");

		len = strlen(temp);
		ret = Fwrite(handle, len, temp);
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
	
	return(FALSE);
}
#endif
