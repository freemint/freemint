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


void RefGotoNode(DOCUMENT *doc, char *chapter,long node_num);


REF_FILE *ref_load(short handle)
{
	REF_FILE *ref = NIL;
	long size,pos;
	unsigned long magic;

	Fread(handle,4,&magic);

	/*	"magischer Wert" im Datei-Kopf?	*/
	if(magic != 0x48524546L) /*'HREF'*/
	{
		Debug("ERROR: Illegal REF file. Magic value not found.");
		return(NIL);
	}
	pos = Fseek(0, handle, 1);
	size = Fseek(0, handle, 2) - pos;
	Fseek(pos, handle, 0);
	
	/*	Speicher fuer Header und Indexdaten reservieren	*/
	ref = (REF_FILE *)Malloc(size);
	if(!ref)
	{
		Debug("Error while allocating %ld bytes for REF file.",size);
		return(NIL);
	}

	Fread(handle,size,ref);

	return(ref);
}


char *ref_find(REF_FILE *ref, short type)
{
char *ptr=&ref->start;
unsigned char size;
	if(ref == NIL)
		return(NULL);

	do
	{
		size=ptr[1];

/*
		Debug("%d (size %d) <%s>",*ptr,size,(size?ptr+2:""));
*/		if(*ptr==type)
			return(ptr+2);
		ptr+=size;
	}while(size);
	return(NULL);
}

short ref_findnode(REF_FILE *ref, char *string, long *node, short *line)
{
	char *ptr = &ref->start;
	unsigned char size;
	long node_num=0;

	if(!ref || ref == NIL)
		return(FALSE);

	do
	{
		if (*ptr == REF_FILENAME)
			node_num=-1;
		else if(*ptr == REF_NODENAME)
			node_num++;
		size=ptr[1];
/*
		Debug("%d (size %d) <%s>",*ptr,size,(size?ptr+2:""));
*/		if(size)
		{
			if(strcmp(ptr+2,string)==0)
			{
				if(*ptr == REF_ALIASNAME)
					*line=*(short *)(ptr+2+strlen(ptr+2));
				else
					*line=0;
				*node=node_num;
				return(TRUE);
			}
		}
		ptr+=size;
	}while(size);
	return(FALSE);
}

/*	Sucht in einer REF Datei nach dem Suchwort <string> und liefert
	eine verkettete Liste auf die passenden Nodes	*/
RESULT_ENTRY *ref_findall(REF_FILE *ref, char *string, short *num_results)
{
RESULT_ENTRY *list=NULL, *last_entry=NULL, prototype;
char *pos = &ref->start;
long ref_entries = ref->num_entries;
short found_one = FALSE;

	*num_results = 0;

	prototype.next=NULL;
	*prototype.path=0;
	*prototype.dbase_description=0;
	*prototype.node_name=0;
	prototype.line=0;

	do
	{
		while(ref_entries--) 
		{
		unsigned char size;
		
			size=pos[1];	
			switch(*pos)
			{
				case REF_FILENAME:
				{
					char *cptr;
					strcpy(prototype.path, pos+2);
			
					/*	Falls Datei-Erweiterung fehlt: hyp anfuegen	*/
					cptr=strrchr(prototype.path, '.');
					if(cptr == NULL)
						strcat(prototype.path, ".hyp" );
					break;
				}
				case REF_NODENAME:
					strcpy(prototype.node_name, pos+2);
					if(strcmp(string, pos+2) == 0) {
						found_one = TRUE;
						prototype.is_label=0;
					}
					break;
				case REF_ALIASNAME:
					if(strcmp(string, pos+2) == 0) {
						found_one = TRUE;
						prototype.is_label=0;
					}
					break;
				case REF_LABELNAME:
					if(strcmp(string, pos+2) == 0) {
						found_one = TRUE;
						prototype.is_label=1;
						prototype.line = *(unsigned short *)(pos+size-2);
					}					
					break;
				case REF_DATABASE:
					strcpy(prototype.dbase_description, pos+2);
					break;
				case REF_OS:
					break;
			}
			if(found_one)
			{
			RESULT_ENTRY *entry;
				entry=(RESULT_ENTRY *)Malloc(sizeof(RESULT_ENTRY));
				if(entry == NULL)
				{
					Debug("Out of Memory in ref_findall()");
					return list;
				}
				*entry = prototype;
				(*num_results)++;
				
				if(last_entry == NULL)
					list = entry;
				else
					last_entry->next = entry;
				last_entry = entry;
				found_one = FALSE;
			}
			pos=pos+size;
		}

		ref_entries = ((long *)pos)[1];
		pos+=2*sizeof(long);
	}while(ref_entries>0);
	
	return list;
}
