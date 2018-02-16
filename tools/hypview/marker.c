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
#include <string.h>
#ifdef __GNUC__
	#include <osbind.h>
	#include <fcntl.h>
#else
	#include <tos.h>
#endif
#include <gemx.h>
#include "include/scancode.h"
#include "diallib.h"
#include "defs.h"

#define MAX_MARKEN		10
#define UNKNOWN_LEN		10
#define PATH_LEN		128
#define NODE_LEN		40

extern WINDOW_DATA *Win;

typedef struct
{
	short  node_num;
 	short  line;
 	char unknown[UNKNOWN_LEN];		/* Unbekannt           							*/
 	char path[PATH_LEN];			/* Kompletter Pfad+Datei 						*/
 	char node_name[NODE_LEN];		/* Titel der Seite, nullterminiert     			*/
}MARKEN;

short marken_change;
MARKEN marken[MAX_MARKEN];

/*----------------------------------------------------------------------------------*/

static void
MarkerDelete(short num)
{
	memset(&marken[num], 0, sizeof(MARKEN));
	marken[num].node_num = -1;
	strcpy ( marken[num].node_name, "   free   " );
}

/*----------------------------------------------------------------------------------*/

void MarkerSave ( DOCUMENT *doc, short num )
{
	WINDOW_DATA *win = Win;
	char *src,*dst, *end;
	
	/* Vermeide illegale Aufrufe	*/
	if(num<0 || num>=MAX_MARKEN)
		return;
	
	marken[num].node_num = (short) doc->getNodeProc(doc);
	marken[num].line = (short)win->y_pos;
	strncpy ( marken[num].path, doc->path, PATH_LEN-1 );
	marken[num].path[PATH_LEN-1]=0;

	/* Kopiere Marken-Titel	*/
	src=doc->window_title;
	dst=marken[num].node_name;
	end=&marken[num].node_name[NODE_LEN-1];
	*dst++=' ';
	while(dst < end)
	{
		if(*src) 
			*dst++ = *src++;
		else
			break;
	}
	if(dst < end)
		*dst++=' ';
	if(dst < end)
		*dst++=' ';
	*dst=0;

	{
		char ZStr[255];
		long len;
		
		strcpy (ZStr, "(");
		src = strrchr ( marken[num].path, '\\' );
		strcat (ZStr, src + 1 );
		strcat (ZStr, ") ");
		len = strlen ( ZStr );
		if ( strlen ( marken[num].node_name ) + len > NODE_LEN )
			marken[num].node_name[NODE_LEN - len] = '\0';
		strcat ( marken[num].node_name, ZStr );
	}

	marken_change = 1;
    Bconout (2, 0x07);							/* Glocke							*/
}

/*----------------------------------------------------------------------------------*/

void MarkerShow ( DOCUMENT *doc, short num, short new_window )
{
	WINDOW_DATA *win = Win;

	/* Vermeide illegale Aufrufe	*/
	if(num<0 || num>=MAX_MARKEN)
		return;

	if ( marken[num].node_num != -1)
	{
		short open_success;

		if(new_window)
			open_success = OpenFileNW( marken[num].path, NULL, marken[num].node_num );
		else
			open_success = OpenFileSW( marken[num].path, NULL, 0);
		if ( open_success )
		{
			doc = win->data;
			GotoPage( doc, marken[num].node_num, marken[num].line, 0 );
		}
	}
}

/*----------------------------------------------------------------------------------*/

void MarkerPopup ( DOCUMENT *doc, short x, short y)
{
	OBJECT *tree=tree_addr[EMPTYPOPUP];
	short i;
	EVNTDATA event;
	long len=0;

	for ( i = EM_1; i <= EM_10; i++ )
	{
		tree[i].ob_spec.free_string=&marken[i - EM_1].node_name[0];
		tree[i].ob_flags &= ~OF_HIDETREE;

		len = max( strlen(marken[i - EM_1].node_name), len);
	}
	tree[EM_11].ob_flags |= OF_HIDETREE;
	tree[EM_12].ob_flags |= OF_HIDETREE;
	
	len=max(len,14);

	len=len*pwchar;
	tree[EM_BACK].ob_x = x;
	tree[EM_BACK].ob_y = y + 5;
	tree[EM_BACK].ob_width=(short)len;
	tree[EM_BACK].ob_height=(i-EM_1)*phchar;
	
	/* Setze Popup-Breite fr alle Eintrge */
	while(--i>=EM_1)
	{
		tree[i].ob_width=(short)len;
		tree[i].ob_height=phchar;
	}
/*
Debug ( "len: %d; x: %d:%d, Y:%d", (short)len, x, x+(short)(len>>1), y);
Debug ( "h: %d", tree[EM_BACK].ob_height );*/

	i=form_popup(tree_addr[EMPTYPOPUP], 0, 0);
	graf_mkstate ( &event.x, &event.y, &event.bstate, &event.kstate );
	if ( i != -1 )
	{
		i -=EM_1;
		
		if (event.kstate & KbSHIFT)
			MarkerSave(doc, i);
		else if (marken[i].node_num == -1)
		{
			char buff[256];
			sprintf(buff,string_addr[ASK_SETMARK], doc->window_title);
			if(form_alert(1,buff) == 1)
				MarkerSave(doc, i);
		}
		else
		{
			if (event.kstate & KbALT)
			{
				char buff[256];
				sprintf(buff,string_addr[WARN_ERASEMARK], marken[i].node_name+1);
				if(form_alert(1,buff) == 1)
				{
					MarkerDelete(i);
					marken_change = 1;
				}
			}
			else
				MarkerShow(doc, i, event.kstate & KbCTRL);
		}
	}
}

/*----------------------------------------------------------------------------------*/

void MarkerSaveToDisk ( void )
{
	if ( !marken_change)
		return;
		
	if ( marken_path[0] != '\0' )
	{
		long ret;

		if ( marken_save_ask )
		{
			if ( form_alert ( 1, string_addr[ASK_SAVEMARKFILE]) == 2 )
				return;
		}
		ret=Fopen( marken_path, O_WRONLY|O_TRUNC|O_CREAT);
		if(ret >= 0)
		{
			Fwrite( (short)ret, sizeof (MARKEN) * MAX_MARKEN, marken);
			Fclose((short)ret);
		}
		else
			Debug("Error %ld: saving %s",ret,marken_path);
	}
}


/*----------------------------------------------------------------------------------*/

void MarkerInit ( void )
{
	short i;
	long ret;

	/* Initialisiere Marken	*/
	for ( i = 0; i < MAX_MARKEN; i++ )
	{
		MarkerDelete(i);
	}
	
	/* Pfad vorhanden, dann diese Marken-Datei laden */
	if ( marken_path[0] != '\0' )			
	{
		if ( strncmp ( marken_path, "$HOME\\", 6 ) == 0 )
		{
			char home_path[DL_PATHMAX];
			char *home_ptr;
			char *separator="\\";
			char *add_ptr;
			
			shel_envrn(&home_ptr,"HOME=");
			strcpy ( home_path, home_ptr );
			if(home_path[1]==':')		/*	Absoluter Pfad	*/
				*separator=home_path[2];
			else						/*	relativer Pfad	*/
			{
				if((*home_path=='\\')||(*home_path=='/'))
					*separator=*home_path;
			}
			
			add_ptr=&home_path[strlen(home_path)-1];
			/*	Falls $HOME nicht mit einem Slash-/Backslash endet:	*/
			if(*add_ptr!=*separator)
				*(++add_ptr)=*separator;
		
			add_ptr++;
			strcpy ( add_ptr, &marken_path[6] );
			strcpy ( marken_path, home_path );
		}
		ret=Fopen( marken_path, O_RDONLY);
		if(ret >= 0)
		{
			Fread( (short)ret, sizeof (MARKEN) * MAX_MARKEN, marken);
			for(i=0; i<MAX_MARKEN; i++)
			{
				if(marken[i].node_name[0] == 0)
					MarkerDelete(i);
			}
			Fclose((short)ret);

		}
	}
	
	marken_change = 0;
}
