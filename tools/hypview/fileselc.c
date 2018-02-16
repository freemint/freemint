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
	#include <fcntl.h>
#else
	#include <tos.h>
#endif
#include <gem.h>
#include "diallib.h"
#include "defs.h"


/*	Erstellt in <dst> eine Liste mit absoluten Pfaden aus <src>.
	Relative Pfade werden ignoriert.	*/
static void
CreatePathList(char *dst,char *src)
{
	short was_path=FALSE;
	while(*src)
	{
		if(src[1]==':')		/*	Absoluter Pfad?	*/
		{
			while(*src)
			{
				if(*src==';')
				{
					if(src[-1]!=dir_separator)
						*dst++=dir_separator;
					*dst++=0;
					src++;
					break;
				}
				else
					*dst++=*src++;
			}
			was_path=TRUE;
		}
		else
		{
			while(*src)
			{
				if(*src++==';')
					break;
			}
			was_path=FALSE;
		}
	}

	if(was_path && src[-1]!=dir_separator)
		*dst++=dir_separator;
	*dst++=0;
	*dst++=0;
}		


static void
OpenFile_FSLX(FILESEL_DATA *fslx, short nfiles)
{
	if(fslx->button && *fslx->name)
	{
	char path[DL_PATHMAX];
		strcpy(path,fslx->path);
		strcat(path,fslx->name);
		OpenFileSW(path,NULL,0);
	}
}

void SelectFileLoad(void)
{
char paths[550];
	CreatePathList(paths,path_list);

	if(OpenFileselector(OpenFile_FSLX,string_addr[FSLX_LOAD],
							NULL,paths,file_extensions,0))
			ModalItem();
}




void *save_data;

static void
SaveFile_FSLX(FILESEL_DATA *fslx, short nfiles)
{
	DOCUMENT *doc=save_data;
	save_data=NULL;
	
	if (fslx->button)
	{
		char path[DL_PATHMAX];
		long ret;
		
		strcpy(path,fslx->path);
		strcat(path,fslx->name);

		ret=Fopen(path,O_RDONLY);
		if(ret>=0)
		{
			Fclose((short)ret);
			if(form_alert(2,string_addr[WARN_FEXIST])==2)
				return;
		}
		BlockAsciiSave(doc, path);
	}
}


void SelectFileSave(DOCUMENT *doc)
{
char paths[550];
char filepath[DL_PATHMAX];
char *ptr;

	save_data=doc;

	CreatePathList(paths,path_list);
	strcpy(filepath,doc->path);
	ptr=strrchr(filepath,'.');
	if(ptr)
		strcpy(ptr,".TXT");

	if(OpenFileselector(SaveFile_FSLX,string_addr[FSLX_SAVE],
							filepath,paths,"*.TXT\0",0))
			ModalItem();
}
