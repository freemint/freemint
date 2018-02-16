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
#include "../diallib.h"
#include "../hyp.h"

long GetCacheSize(long num_elements)
{
	return(num_elements*sizeof(INDEX_ENTRY *));
}

void InitCache(HYP_DOCUMENT *hyp)
{
	long i;
	for(i=0;i<hyp->num_index;i++)
	{
		hyp->cache[i]=NULL;
	}
}

void ClearCache(HYP_DOCUMENT *hyp)
{
	long i;
	for(i=0;i<hyp->num_index;i++)
	{
		if(hyp->cache[i])
		{
			Mfree(hyp->cache[i]);
			hyp->cache[i]=NULL;
		}
	}
}

void TellCache(HYP_DOCUMENT *hyp, long node_num,void *node)
{
	hyp->cache[node_num]=node;
}

void *
AskCache(HYP_DOCUMENT *hyp, long node_num)
{
	return(hyp->cache[node_num]);
}




void RemoveNodes(HYP_DOCUMENT *hyp)
{
long i;
	for(i=0;i<hyp->num_index;i++)
	{
		if(hyp->cache[i] && hyp->indextable[i]->type<=POPUP)
		{
			Mfree(hyp->cache[i]);
			hyp->cache[i]=NULL;
		}
	}
}

