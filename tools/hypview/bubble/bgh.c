#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __GNUC__
	#include <osbind.h>
	#include <fcntl.h>

	/* Mxalloc mode: global memory */
#else
	#include <tos.h>
#endif
#ifndef MX_GLOBAL
	#define MX_GLOBAL	(2<<4)
#endif
#include <gem.h>
#include "bubble.h"
#include "bgh.h"

#define BGH_MAGIC 0x23424748L

#define IS_SPACE(a)		((a==' ') || (a=='\t'))
#define IS_EOL(a)			((a==10) || (a==13))
#define IS_NUMBER(a)		((a>='0') && (a<='9'))

#ifndef TRUE
	#define	TRUE	1
	#define	FALSE	0
#endif

struct bgh_group;
struct bgh_object;

typedef struct
{
	struct bgh_group *section[4];
}BGH_head;

typedef struct bgh_group
{
	int index;
	struct bgh_group *next;
	struct bgh_object *first;
	char *help_string;
}BGH_group;

typedef struct bgh_object
{
	int index;
	struct bgh_object *next;
	char *help_string;
}BGH_object;


char *Goto_Lineend(char *ptr);
char *Skip_Lineend(char *ptr);
char *Skip_Spaces(char *ptr);
char *Skip_Number(char *ptr);
void Debug(char *str,...);



void *BGH_load(const char *Name)
{
long ret,len;
int handle,not_good=FALSE,i,sect=0;
BGH_head *head;
BGH_group *group=NULL,*new_group;
BGH_object *obj=NULL,*new_obj;
char *read,*write=NULL;
	ret=Fopen(Name,O_RDONLY);
	if(ret<0)
		return NULL;
	
	handle=(int)ret;
	len=Fseek(0,handle,2);
	Fseek(0,handle,0);
	
	if(len<sizeof(BGH_head))
		return NULL;
	
	head=(BGH_head *)Mxalloc(2*len+2, MX_GLOBAL);
	if(head)
	{
		/* 68000er Bug (keine WORD/LONG Zugriffe auf ungerade Adressen)	*/
		read=(char *)(((long)head+len+1)&0xfffffffeL);

		if(Fread(handle,len,read)==len)
		{
			if(*(long *)read==BGH_MAGIC)
			{
				read[len]=0;
				write=(char *)head+sizeof(BGH_head);
				for(i=0;i<4;i++)
					head->section[i]=NULL;
				
				while(*read)
				{
					read=Goto_Lineend(read);	/*	Zeilenende suchen	*/
					read=Skip_Lineend(read);	/*	zur naechsten Zeile gehen	*/

					if(*read)
					{
						if(*read=='#')
						{
							read++;
							read=Skip_Spaces(read);

							if(IS_NUMBER(*read))		/*	Objekt gefunden	*/
							{
								if(group)
								{
									new_obj=(BGH_object *)write;
									write+=sizeof(BGH_object);
	
									new_obj->next=NULL;
									new_obj->help_string=NULL;
									new_obj->index=atoi(read);
									
									read=Skip_Number(read);
									read=Skip_Spaces(read);
									
									if(obj && *read=='^')
									/*	Referenz zum vorherigen Objekt ?	*/
									{
										new_obj->help_string=obj->help_string;
									}
									else if(head->section[3] && *read=='>')
									/*	Referenz zum Objekt mit folgender Nummer	*/
									{
									BGH_group *ref_group=head->section[3];
										while(ref_group && ref_group->index!=0)
											ref_group=ref_group->next;
										
										if(ref_group)
										{
										BGH_object *ref_obj=ref_group->first;
											read++;
											i=atoi(read);
											read=Skip_Number(read);
											while(ref_obj && ref_obj->index!=i)
												ref_obj=ref_obj->next;

											if(ref_obj)
												new_obj->help_string=ref_obj->help_string;
										}							
									}
									else if(!IS_EOL(*read))
									/*	Normaler Hilfe-String folgt	*/
									{
										new_obj->help_string=write;
										while(*read && !IS_EOL(*read))
											*write++=*read++;
										*write++=0;

										/* 68000er Bug (keine WORD/LONG Zugriffe 
											auf ungerade Adressen)	*/
										if((long)write & 1)
											write++;
									}
									
									if(group->first)
									{
										obj=group->first;
										while(obj->next)
											obj=obj->next;
										obj->next=new_obj;
									}
									else
										group->first=new_obj;
									obj=new_obj;
								}
							}
							else
							{
							const char *ptr = "N/A";
								i=0;
								switch(*read)
								{
									case 'd':
									case 'D':
										i=4;
										ptr="dial";
										sect=0;
										break;
									case 'a':
									case 'A':
										i=5;
										ptr="alert";
										sect=1;
										break;
									case 'u':
									case 'U':
										i=4;
										ptr="user";
										sect=2;
										break;
									case 'm':
									case 'M':
										i=4;
										ptr="more";
										sect=3;
										break;
								}
								/*	Gruppe gefunden ??	*/
								if(i && !strnicmp(read,ptr,i))
								{
									read+=i;
									read=Skip_Spaces(read);
	
									new_group=(BGH_group *)write;
									write+=sizeof(BGH_group);
	
									new_group->next=NULL;
									new_group->first=NULL;
									new_group->index=atoi(read);
									read=Skip_Number(read);
									if(IS_SPACE(*read))
									{
										read=Skip_Spaces(read);
										new_group->help_string=write;
										while(*read && !IS_EOL(*read))
											*write++=*read++;
										*write++=0;

										/* 68000er Bug (keine WORD/LONG Zugriffe 
											auf ungerade Adressen)	*/
										if((long)write & 1)
											write++;
									}
									else
										new_group->help_string=NULL;
									
									if(head->section[sect])
									{
										group=head->section[sect];
										while(group->next)
											group=group->next;
										group->next=new_group;
									}
									else
										head->section[sect]=new_group;
									group=new_group;
									obj=NULL;
								}
							}
						}
					}
				}
			}
			else
				not_good=TRUE;
			
		}
		else
			not_good=TRUE;

		if(not_good)
		{
			Mfree(head);
			head=NULL;
		}
	}
	Fclose(handle);

	Mshrink(head,(unsigned long)write-(unsigned long)head);
	return head;
}

void BGH_free(void *bgh_handle)
{
	Mfree(bgh_handle);
}

char *BGH_gethelpstring(void *bgh_handle, int typ, int gruppe, int idx)
{
BGH_head *head=bgh_handle;
BGH_group *group;
char *help_string=NULL;
	if(typ<0 || typ>3)
		return NULL;
	
	group=head->section[typ];
	while(group && group->index!=gruppe)
		group=group->next;
	if(group)
	{
	BGH_object *obj=group->first;
		if(idx==-1)
		{
			help_string=group->help_string;
		}
		else
		{
			while(obj && obj->index!=idx)
				obj=obj->next;
			if(obj)
				help_string=obj->help_string;
		}
	}
	return help_string;
}

void BGH_action(void *bgh_handle, int mx, int my, int typ, int gruppe, int idx)
{
extern int ap_id;
int bubble_id;
int msg[8];
char *help_string;
char *path=NULL;

	help_string=BGH_gethelpstring(bgh_handle, typ, gruppe, idx);

	if(help_string)
	{
		bubble_id=appl_find("BUBBLE  ");
		if(bubble_id<0)
		{
			shel_envrn(&path,"BUBBLEGEM=");
			if(path)
			{
				bubble_id=shel_write(SHW_EXEC,1,SHW_PARALLEL,path,NULL);
				evnt_timer(500);
			}
		}

		if(bubble_id >= 0)
		{
			msg[0]=BUBBLEGEM_SHOW;
			msg[1]=ap_id;
			msg[2]=0;
			msg[3]=mx;
			msg[4]=my;
			msg[5]=(int)(((long)help_string >> 16) & 0x0000ffff);
			msg[6]=(int)((long)help_string & 0x0000ffff);
			msg[7]=0;
			if(appl_write(bubble_id, 16, msg) == 0)
			{
			/* Fehler */
			}
		}
	}
}






char *Goto_Lineend(char *ptr)
{
	while(*ptr && !IS_EOL(*ptr))
		ptr++;
	return ptr;
}

char *Skip_Lineend(char *ptr)
{
	while(*ptr && IS_EOL(*ptr))
		ptr++;
	return ptr;
}

char *Skip_Spaces(char *ptr)
{
	while(*ptr && IS_SPACE(*ptr))
		ptr++;
	return ptr;
}

char *Skip_Number(char *ptr)
{
	while(*ptr && IS_NUMBER(*ptr))
		ptr++;
	return ptr;
}
