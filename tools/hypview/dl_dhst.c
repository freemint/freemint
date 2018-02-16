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
#ifdef __GNUC__
	#include <mint/cookie.h>
	#include <osbind.h>
#else
	#include <tos.h>
	#include <cookie.h>
#endif
#include <gem.h>
#include "include/dhst.h"
#include "diallib.h"

#if USE_DOCUMENTHISTORY == YES

void DhstAddFile(char *path);
void DhstFree(short msg[8]);

void DhstAddFile(char *path)
{
	long value;

	if (!Getcookie(0x44485354L/*'DHST'*/, &value))
	{
		short msg[8];
		char *ptr;
		long ret;
		DHSTINFO *info;
		
		ret = Mxalloc(sizeof(DHSTINFO) + DL_PATHMAX * 2,  MX_PREFTTRAM|MX_MPROT|MX_READABLE);
		if(!ret)
		{
			form_alert(1,tree_addr[DIAL_LIBRARY][DI_MEMORY_ERROR].ob_spec.free_string);
			return;
		}
		info=(DHSTINFO *)ret;
		info->appname=(char *)(ret+sizeof(DHSTINFO));
		strcpy(info->appname,PROGRAM_NAME);
		info->apppath=&info->appname[strlen(info->appname)+1];

		if(!shel_read(info->apppath,info->apppath+DL_PATHMAX))
		{
			Mfree(info);
			Cconout(7);
			return;
		}
		info->docpath=&info->apppath[strlen(info->apppath)+1];
		strcpy(info->docpath,path);
		ptr=strrchr(info->docpath,'\\');
		if(ptr)
			info->docname=ptr+1;
		else
			info->docname=info->docpath;

#if DEBUG == ON
		Debug("DHST-Protocol: %s %s %s %s",
				info->appname,info->apppath,info->docpath,info->docname);
#endif

		msg[0]=DHST_ADD;
		msg[1]=ap_id;
		msg[2]=0;
		*(DHSTINFO **)(&msg[3])=info;
		msg[5]=0;
		msg[6]=0;
		msg[7]=0;
		appl_write((short)value,16,&msg[0]);
	}
}

void DhstFree(short msg[8])
{
	Mfree(*(void **)&msg[3]);
}
#endif
