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
#include <gemx.h>
#include "diallib.h"

#if USE_FILESELECTOR == YES

void *OpenFileselector(HNDL_FSL proc,char *comment,char *filepath,char *path,char *pattern,short mode);
void FileselectorEvents(FILESEL_DATA *ptr,EVNT *event);
void RemoveFileselector(FILESEL_DATA *ptr);

void *OpenFileselector(HNDL_FSL proc,char *comment,char *filepath,char *path,char *pattern,short mode)
{
FILESEL_DATA *ptr;
char *fslx_paths="C:\\\0",*fslx_pattern="*\0";

	ptr=(FILESEL_DATA *)Mxalloc(sizeof(FILESEL_DATA),3);
	if(ptr==NULL)
	{
		form_alert(1,tree_addr[DIAL_LIBRARY][DI_MEMORY_ERROR].ob_spec.free_string);
		return(NULL);
	}
	memset(ptr,0,sizeof(FILESEL_DATA));

	if(filepath)
	{
	char *f;
		strcpy(ptr->path,filepath);

		f=strrchr(ptr->path,'\\');
		if(f==NULL)
			f=ptr->path;
		else
			f++;

		strcpy(ptr->name,f);
		*f=0;
	}

	if(path)
		fslx_paths=path;

	if(!*ptr->path)
		strcpy(ptr->path,fslx_paths);

	if(has_wlffp & 8)
	{
		if(pattern)
			fslx_pattern=pattern;

		ptr->dialog=fslx_open(comment,-1,-1,&ptr->whandle,ptr->path,DL_PATHMAX,
				ptr->name,DL_NAMEMAX,fslx_pattern,0L,fslx_paths,SORTBYNAME,mode);

		if(ptr->dialog)
		{
			ptr->type=WIN_FILESEL;
			ptr->status=WIS_OPEN;
			ptr->proc=proc;
			add_item((CHAIN_DATA *)ptr);
		}
		else
		{
			Mfree(ptr);
			form_alert(1,tree_addr[DIAL_LIBRARY][DI_MEMORY_ERROR].ob_spec.free_string);
		}

		return(ptr->dialog);
	}
	else
	{
	short result;
		if(pattern && !strchr(pattern,',')) /*	Nur ein Suchmuster angegeben?	*/
			strcat(ptr->path,pattern);
		else
			strcat(ptr->path,"*.*");
		ptr->dialog=NULL;

		result=fsel_exinput(ptr->path,ptr->name,&ptr->button,comment);
		if(!result)									/* Fileselectbox  */
			return(NULL);							/* Fehler zurueck  */

		*(strrchr(ptr->path,'\\')+1)=0;		/*	Datei-Maske entfernen	*/
		proc(ptr,1);
		Mfree(ptr);
		return(NULL);
	}
}

void FileselectorEvents(FILESEL_DATA *ptr,EVNT *event)
{
char *pattern;

	if(!fslx_evnt(ptr->dialog,event,ptr->path,ptr->name,
				&ptr->button,&ptr->nfiles,&ptr->sort_mode,&pattern))
	{
		ptr->proc(ptr,ptr->nfiles);
		RemoveFileselector(ptr);
	}
}

void RemoveFileselector(FILESEL_DATA *ptr)
{
	fslx_close(ptr->dialog);
	remove_item((CHAIN_DATA *)ptr);
	Mfree(ptr);
	if(modal_items>=0)
		modal_items--;
}
#endif
