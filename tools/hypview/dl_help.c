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
	#include <osbind.h>
#else
	#include <tos.h>
#endif
#include <gem.h>
#include "diallib.h"
#include "include/av.h"


#if USE_STGUIDE == YES

char *help_file=STGUIDE_FILE;

void STGuideHelp(void);

void STGuideHelp(void)
{
char *help_article;

	help_article=GetTopic();
	if(help_article)
	{
	char *path, fname[DL_NAMEMAX];
	short helpviewer_id, msg[8]={VA_START,0,0,0,0,0,0,0};

		/*	Look for environment variable which points to the help viewer	*/
		shel_envrn(&path,"HELPVIEWER=");
		if(path)								/*	variable found?	*/
		{
		char *ptr;
			ptr=strrchr(path,'\\');		/*	find start of file name	*/
			if(!ptr++)						/*	No file name found?	*/
				ptr=path;					/*	Treat path as file name	*/

			strcpy(fname,ptr);			/*	copy file name in temp. mem	*/
			ptr=strrchr(fname,'.');		/*	find start of extension (like ".APP")	*/
			if(ptr)							/*	extension exists? ...	*/
				*ptr=0;						/*	... cut away	*/
		}
		else
			strcpy(fname,"ST-GUIDE");	/*	no variable? -> default app is ST-Guide	*/

		/*	Look for the help viewer application in memory	*/
		helpviewer_id=appl_find(fname);

		if(helpviewer_id<0)				/*	not found?	*/
		{
		char command[DL_PATHMAX];

			/*	Build command line with apostrophes and length byte	*/
			strcpy(command+1,help_file);
			strcat(command+1,"'");		
			strcat(command+1,help_article);
			strcat(command+1,"'");
			command[0]=(char)strlen(command+1);

			if(path)							/*	if a path was given: try to start	*/
				helpviewer_id=shel_write(SHW_EXEC,1,SHW_PARALLEL,path,command);

			if(helpviewer_id<0)			/*	no viewer ?	*/
			{
				/*	Alert user	*/
				form_alert(1,tree_addr[DIAL_LIBRARY][DI_HELP_ERROR].ob_spec.free_string);
				return;
			}
		}
		else	/*	Viewer in memory: transfer path and chapter name	*/
		{
		char *command;
			
			/*	Allocate global accessible memory for transfer	*/
			command=(char *)Mxalloc(strlen(help_file)+strlen(help_article)+1, MX_PREFTTRAM|MX_MPROT|MX_GLOBAL);
			if (command == (void *)-32l)
				command=(char *)Malloc(strlen(help_file)+strlen(help_article)+1);
			if(!command)
			{
				/*	Alert user: not enough memory	*/
				form_alert(1,tree_addr[DIAL_LIBRARY][DI_MEMORY_ERROR].ob_spec.free_string);
				return;
			}
		
			/*	Build VA_START-command line	*/
			strcpy(command,help_file);
			strcat(command,help_article);
		
			/*	And send message	*/
			msg[1]=ap_id;
			*(char **)&msg[3]=command;
			appl_write(helpviewer_id, 16, msg);
		}
	}
}

#endif
