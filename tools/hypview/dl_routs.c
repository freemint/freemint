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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __GNUC__
	#include <stdarg.h>
	#include <unistd.h>
	#include <mintbind.h>
#else
	#include <tos.h>
#endif
#include <gemx.h>
#include "include/scancode.h"
#include "diallib.h"

void ConvertKeypress(short *key,short *kstate);
void CopyMaximumChars(OBJECT *obj,char *str);
char *ParseData(char *start);
void Debug(const char *str,...);


void ConvertKeypress(short *key,short *kstate)
{
	short ascii=*key & 0xff;
	short scan=(*key>>8) & 0x7f;			/*	Scan code immer < 128+	*/
/*
	Debug("Key: %d Scan: %d  Ascii: %d -- %c %c %c %c %s%s%s%s%s",
				*key,scan,ascii,ascii,key_table->unshift[scan],
				key_table->shift[scan],key_table->capslock[scan],
				((*kstate & KbCTRL) ? ("+CTRL"):("")),
				((*kstate & KbALT) ? ("+ALT"):("")),
				((*kstate & KbLSHIFT) ? ("+LSHIFT"):("")),
				((*kstate & KbRSHIFT) ? ("+RSHIFT"):("")),
				((*kstate & KbNUM) ? ("+NUM"):("")));
*/
	if(!scan)								/*	Falls kein Scancode vorhanden ist,	*/
		return;								/*	dann gibt's nichts zu tun!	*/

	if((scan >= KbAlt1) && (scan <= 0x83))	/*	Alt+Numerische Tasten...	*/
		scan -= 0x76;							/*	in Numerische Tasten wandeln	*/

	if(((scan >= 99) && (scan <= 114))		/*	Zahlenblock Tasten...	*/
			|| (scan == 74) || (scan == 78))
		*kstate |= KbNUM;

	if((scan >= 115) && (scan <= 119))		/*	CTRL+Kombinationen...	*/
	{
		*kstate |= KbCTRL;
		switch(scan)
		{
			case 115:
				scan=KbLEFT;
				break;
			case 116:
				scan=KbRIGHT;
				break;
			case 117:			/*	CTRL + END	*/
				break;
			case 118:			/*	CTRL + PAGE DOWN	*/
				break;
			case 119:
				scan=KbHOME;
				break;
		}
	}

	if(*kstate & (KbCTRL|KbALT))			/*	CTRL und ALT Kombinationen ?	*/
	{
		if(ascii<32)
			ascii= key_table->caps[scan];	/*	in Grossbuchstaben umwandeln	*/
	}

	*key=(scan<<8)+ascii;
}

void CopyMaximumChars(OBJECT *obj,char *str)
{
short max_size=obj->ob_spec.tedinfo->te_txtlen-1;
	strncpy(obj->ob_spec.tedinfo->te_ptext,str,max_size);
	obj->ob_spec.tedinfo->te_ptext[max_size]=0;
}

char *ParseData(char *start)
/*
	Liefert in <start> den ersten Parameter und gibt den Pointer
	auf den nchsten Parameter zurck.
	Um alle Parameter zu erfahren muss man folgendermassen vorgehen:

	{
	char *next,*ptr=data;
		do
		{
			next=ParseData(ptr);
			DoSomething(ptr);
			ptr=next;
		}while(*next);
	}
*/
{
short in_quote=0, more=FALSE;
	while(*start)
	{
		if(*start==' ')
		{
			if(!in_quote)
			{
				*start=0;
				more=TRUE;
			}
			else
				start++;
		}
		else if(*start=='\'')
		{
			strcpy(start,start+1);
			if(*start=='\'')
				start++;
			else
				in_quote=1-in_quote;
		}
		else
			start++;
	}
	if(more)
		return(start+1);
	else
		return(start);
}

#if 1
short rc_intersect_my ( GRECT *p1, GRECT *p2 )
{
	short tx,ty,tw,th;
	tw=min((p2->g_x+p2->g_w),(p1->g_x+p1->g_w));
	th=min(p2->g_y+p2->g_h,p1->g_y+p1->g_h);
	tx=max(p2->g_x,p1->g_x);
	ty=max(p2->g_y,p1->g_y);
	p2->g_x=tx;
	p2->g_y=ty;
	p2->g_w=tw-tx;
	p2->g_h=th-ty;
	return((tw>tx)&&(th>ty));
};
#endif

#ifndef __GNUC__
void
Debug(const char *str, ...)
{
	void *list = ...;
	char temp[120], c, *ptr;
	short is_l, done;
	
	ptr=temp;
	while(*str)
	{
		c = *str++;
		if(c == '%')
		{
			done = FALSE;
			is_l = FALSE;
			do
			{
				c = *str++;
				if(c == 'l')
					is_l = TRUE;
				else if(c == 'd' || c == 'i' )
				{
					if(is_l)
						ltoa((*((long *)(list))++), ptr, 10);
					else
						itoa((*((short *)(list))++), ptr, 10);
					done = TRUE;
				}
				else if(c=='u')
				{
					if(is_l)
						ultoa((*((unsigned long *)(list))++),ptr,10);
					else
						ultoa((unsigned long)(*((unsigned short *)(list))++),ptr,10);
					done=TRUE;
				}
				else if(c=='x')
				{
					*ptr++='0';
					*ptr++='x';
					if(is_l)
						ltoa((*((unsigned long *)(list))++),ptr,16);
					else
						itoa((*((unsigned short *)(list))++),ptr,16);
					done=TRUE;
				}
				else if(c=='s')
				{
				char *src=(*((char **)(list))++);
					while(*src)
						*ptr++=*src++;
					*ptr=0;
					done=TRUE;
				}
				else if(c=='c')
				{
					*ptr=(char)(*((short *)(list))++);
					if(*ptr)
						*(++ptr)=0;
					done=TRUE;
				}
				else
					done=TRUE;
			}while(!done);

			while(*ptr)
				ptr++;
		}
		else
			*ptr++=c;
	};
	*ptr=0;

	Fwrite(STDERR_FILENO,strlen(temp),temp);
	Fwrite(STDERR_FILENO,2,"\r\n");
}
#else
void
Debug(const char *fmt, ...)
{
	char buf[1024];
	va_list args;
	long l;

	va_start(args, fmt);
	l = vsprintf(buf, fmt, args);
	va_end(args);

	buf[l++] = '\r';
	buf[l++] = '\n';
	buf[l  ] = '\0';

	Fwrite(STDERR_FILENO, l, buf);
}
#endif

#ifdef GENERATE_DIAGS
void
display(const char *fmt, ...)
{
	char buf[1024];
	va_list args;
	long l;

	va_start(args, fmt);
	l = vsprintf(buf, fmt, args);
	va_end(args);

#if 1
	buf[l] = '\r';
	buf[l+1] = '\n';
	buf[l+2] = '\0';
	Cconws(buf);
#endif
}

void
ndisplay(const char *fmt, ...)
{
	char buf[1024];
	va_list args;
	long l;

	va_start(args, fmt);
	l = vsprintf(buf, fmt, args);
	va_end(args);

	Cconws(buf);
}
#endif

char has_ssystem=1;
char key_state=0;

static long
ChangeSysKeyconfig(void)
{
	char *conterm = (char *)0x0484;
	char val = *conterm;
	*conterm = (val & 0xfd) | key_state;
	key_state = val & 2;
	return(0);
}

void KeyboardOnOff(void)
{
	if(has_ssystem)
	{
		long krep;
		krep = Ssystem(0x0C,0x484L,0);
		if(krep < 0)
		{
			has_ssystem = 0;
			Supexec(ChangeSysKeyconfig);
		}
		else
		{
			Ssystem(0x0F,0x484L,(krep & 0xfd) | key_state);
			key_state=(char)krep & 2;
		}
	}
	else
		Supexec(ChangeSysKeyconfig);
}
