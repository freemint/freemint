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
#include <errno.h>
#include <gem.h>
#include "diallib.h"

short CfgOpenFile(char *path);
void CfgCloseFile(void);
void CfgWriteFile(char *path);
char *CfgGetLine(char *src);
char *CfgGetVariable(char *variable);
char *CfgExtractVarPart(char *start,char separator,short num);
short CfgSaveMemory(long len);
short CfgSetLine(char *line);
short CfgSetVariable(char *name,char *value);


char *cfg_memory=NULL;
char cfg_var[CFG_MAXLINESIZE];

char *cfg_savemem=NULL;
long cfg_save_pos;
long cfg_save_size;

char cfg_comment=';';

#if CFG_IN_HOME==YES
short cfg_in_home=TRUE;
#endif


short CfgOpenFile(char *path)
{
short cfg_handle;
long ret,len;
	if(cfg_memory)
		return(FALSE);

#if CFG_IN_HOME==YES
{
char *home_ptr;
	if(shel_envrn(&home_ptr,"HOME=") && home_ptr && cfg_in_home)
	{
	char home_path[DL_PATHMAX];
	char *separator="\\";
	char *add_ptr;

		strcpy(home_path,home_ptr);

		/*	Pfad-Separator ermitteln (evtl. problematisch!!!)	*/
		if(home_path[1]==':')		/*	Absoluter Pfad	*/
			*separator=home_path[2];
		else								/*	relativer Pfad	*/
		{
			if((*home_path=='\\')||(*home_path=='/'))
				*separator=*home_path;
		}
		
		add_ptr=&home_path[strlen(home_path)-1];
		/*	Falls $HOME nicht mit einem Slash-/Backslash endet:	*/
		if(*add_ptr!=*separator)
			*(++add_ptr)=*separator;

		add_ptr++;

		strcpy(add_ptr,"defaults");
		strcat(add_ptr,separator);
		strcat(add_ptr,path);

		/*	Versuche die Datei in $HOME\defaults\<path> zu oeffnen	*/
		ret=Fopen(home_path,O_RDONLY);
		if(ret<0)
		{
			strcpy(add_ptr,path);
			
			/*	Versuche die Datei in $HOME\<path> zu oeffnen	*/
			ret=Fopen(home_path,O_RDONLY);
		}
	}
	else
		ret=-1;

	if(ret<0)
	{
		ret=Fopen(path,O_RDONLY);
		if(ret>=0)
			cfg_in_home=FALSE;
	}
}
#else
	ret=Fopen(path,O_RDONLY);
#endif
	
	if(ret<0)
		return(FALSE);
	cfg_handle=(short)ret;

	len=Fseek(0,cfg_handle,2);
	Fseek(0,cfg_handle,0);

	cfg_memory=(char *)Malloc(len+1);
	if(cfg_memory==NULL)
	{
		form_alert(1,tree_addr[DIAL_LIBRARY][DI_MEMORY_ERROR].ob_spec.free_string);
		return(FALSE);
	}
	Fread(cfg_handle,len,cfg_memory);
	Fclose(cfg_handle);

	cfg_memory[len]=0;

	return(TRUE);
}

void CfgCloseFile(void)
{
	if(!cfg_memory)
		return;

	Mfree(cfg_memory);
	cfg_memory=NULL;
	cfg_var[0]=0;
	return;
}

void CfgWriteFile(char *path)
{
long ret;
	if(!cfg_savemem)
		return;

#if CFG_IN_HOME==YES
{
char *home_ptr;
	if(shel_envrn(&home_ptr,"HOME=") && home_ptr && cfg_in_home)
	{
	char home_path[DL_PATHMAX];
	char *separator="\\";
	char *add_ptr;

		strcpy(home_path,home_ptr);

		/*	Pfad-Separator ermitteln (evtl. problematisch!!!)	*/
		if(home_path[1]==':')		/*	Absoluter Pfad	*/
			*separator=home_path[2];
		else								/*	relativer Pfad	*/
		{
			if((*home_path=='\\')||(*home_path=='/'))
				*separator=*home_path;
		}
		
		add_ptr=&home_path[strlen(home_path)-1];
		/*	Falls $HOME nicht mit einem Slash-/Backslash endet:	*/
		if(*add_ptr!=*separator)
			*(++add_ptr)=*separator;

		add_ptr++;

		strcpy(add_ptr,path);

		/*	Versuche die Datei in $HOME\<path> zu oeffnen	*/
		ret=Fopen(home_path,O_WRONLY|O_TRUNC);
		if(ret == ENOENT)
		{
			strcpy(add_ptr,"defaults");
			strcat(add_ptr,separator);
			strcat(add_ptr,path);
			
			/*	Versuche die Datei in $HOME\defaults\<path> zu oeffnen	*/
			ret = Fopen(home_path,O_WRONLY|O_TRUNC|O_CREAT);
			if(ret == ENOTDIR)			/*	Pfad nicht gefunden?	*/
			{
				/*	Datei in $HOME\<path> erstellen	*/
				strcpy(add_ptr,path);
				ret = Fopen(home_path,O_WRONLY|O_TRUNC|O_CREAT);
			}
		}
	}
	else
		ret=-1;

	if(ret<0)
	{
		ret=Fopen(path,O_WRONLY|O_TRUNC|O_CREAT);
		if(ret>=0)
			cfg_in_home=FALSE;
	}
}
#else
	ret=Fopen(path,O_WRONLY|O_TRUNC|O_CREAT);
#endif

	if(ret>=0)
	{
	short handle=(short)ret;
		Fwrite(handle,cfg_save_pos,cfg_savemem);
		Fclose(handle);
	}

	Mfree(cfg_savemem);
	cfg_savemem=NULL;
}

char *CfgGetLine(char *src)
{
char *dst=cfg_var;
	if(!cfg_memory)
		return(dst);
	while(*src)
	{
		if(*src==cfg_comment)
		{
			/*	Zeilenende suchen	*/
			do
			{
				src++;
			}while(*src && (*src!='\r') && (*src!='\n'));
		}
		else if((*src=='\r')||(*src=='\n'))
		{
			src++;
			while((*src=='\n') || (*src=='\r'))
				src++;

			break;
		}
		else
			*dst++=*src++;
	}
	*dst=0;

	/*	Leerzeichen und Tabulatoren am Anfang ueberspringen	*/
	dst=cfg_var;
	while((*dst==' ')||(*dst=='\t'))
		dst++;
	strcpy(cfg_var,dst);
	return(src);
}

char *CfgGetVariable(char *variable)
{
char *ptr=cfg_memory;
long len=strlen(variable);
	*cfg_var=0;
	if(!cfg_memory)
		return(cfg_var);
	do
	{
		ptr=CfgGetLine(ptr);
		if(strnicmp(cfg_var,variable,len)==0)
		{
			memmove(cfg_var,&cfg_var[len],strlen(&cfg_var[len])+1);
			return(cfg_var);
		}
	}while(*ptr);
	cfg_var[0]=0;
	return(cfg_var);
}

char *CfgExtractVarPart(char *start,char separator,short num)
{
char *pos=start;
	while(num && *pos)
	{
		if(*pos==separator)
			num--;
		pos++;
	}
	return(pos);
}

short CfgSaveMemory(long len)
{
	if(cfg_savemem)
	{
		if(cfg_save_pos+len>=cfg_save_size)
		{
		long ret;
			cfg_save_size+=4096;
			ret=Mxalloc(cfg_save_size,3);
			if(ret<0)
				ret=(long)Malloc(cfg_save_size);
			if(ret>0)
			{
			char *temp=cfg_savemem;
				cfg_savemem=(char *)ret;
				strcpy(cfg_savemem,temp);
				Mfree(temp);
			}
			else
				return(FALSE);
		}
	}
	else
	{
	long ret;
		cfg_save_size=max(4096,len+1);
		ret=Mxalloc(cfg_save_size,3);
		if(ret<0)
			ret=(long)Malloc(cfg_save_size);
		if(ret>0)
		{
			cfg_savemem=(char *)ret;
			*cfg_savemem=0;
			cfg_save_pos=0;
		}
		else
			return(FALSE);
	}
	return(TRUE);
}

short CfgSetLine(char *line)
{
	if(CfgSaveMemory(strlen(line)+2))
	{
		strcat(&cfg_savemem[cfg_save_pos],line);
		strcat(&cfg_savemem[cfg_save_pos],"\r\n");
		cfg_save_pos=strlen(cfg_savemem);
		return(TRUE);
	}
	return(FALSE);
}

short CfgSetVariable(char *name,char *value)
{
	if(CfgSaveMemory(strlen(name)+strlen(value)+2))
	{
		strcat(&cfg_savemem[cfg_save_pos],name);
		strcat(&cfg_savemem[cfg_save_pos],value);
		strcat(&cfg_savemem[cfg_save_pos],"\r\n");
		cfg_save_pos=strlen(cfg_savemem);
		return(TRUE);
	}
	return(FALSE);
}
