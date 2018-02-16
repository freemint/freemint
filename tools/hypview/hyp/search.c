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
#include "../diallib.h"
#include "../defs.h"
#include "../hyp.h"

extern WINDOW_DATA *Win;

LIST_BOX *entrie_box;

short nclicks;

DIALOG *Search_Dialog;
DIALOG *SearchResult_Dialog;

RESULT_ENTRY *Result_List;

static void free_results(RESULT_ENTRY **result_list);


/* Objects of the list boxes */
#define	NO_ENTRIE	10
short entrie_ctrl[5] = { SR_BOX, SR_FSTL_UP, SR_FSTL_DOWN, SR_FSTL_BACK, SR_FSTL_WHITE };
short	entrie_objs[10] = { SR_FSTL_0, SR_FSTL_1, SR_FSTL_2, SR_FSTL_3, SR_FSTL_4, SR_FSTL_5, SR_FSTL_6, SR_FSTL_7, SR_FSTL_8, SR_FSTL_9 };


/*----------------------------------------------------------------------------------------*/ 
/* n entry in the list box has been selected...				  */
/* Function result:	-								  */
/*	box:				Pointer to the list box structure			  */
/*	tree:				Pointer to the object tree		  */
/*	item:				Pointer to the entry		  */
/*	user_data:			...						  */
/*	obj_index:			Index of the object, poss. | 0x8000, poss. 0 (not visible) */
/*	last_state:			The previous state				  */
/*----------------------------------------------------------------------------------------*/ 

static void __CDECL
select_item( struct SLCT_ITEM_args args)
{
	WINDOW_DATA *win = Win;
	RESULT_ENTRY *my_item = (RESULT_ENTRY *)args.item;

	if( nclicks > 1)
	{
		if( OpenFileSW(my_item->path, my_item->node_name, 0))
		{
			if(my_item->line > 0)
			{
				DOCUMENT *doc;

				doc = win->data;

				graf_mouse(BUSYBEE,NULL);
				doc->gotoNodeProc(doc, my_item->node_name, 0);
				doc->start_line= my_item->line;
				graf_mouse(ARROW,NULL);
				ReInitWindow(win,doc);
			}
		}
		SendCloseDialog( SearchResult_Dialog );
	}


	if ( args.item->selected && ( args.item->selected != args.last_state ))
	{
	}
}

/*----------------------------------------------------------------------------------------*/ 
/* Set string and object status of a GTEXT object in the list box					  */
/* Function result:	Number of the start object to be drawn							  */
/*	box:				Pointer to the list box structure 								  */
/*	tree:				Pointer to the object tree										  */
/*  item:				Pointer to the entry											  */
/*  index:				Object number													  */
/*  user_data:			...																  */
/*	rect:				GRECT for selection/deselection or 0L (not alterable)		  */					
/*  offset:																				  */
/*----------------------------------------------------------------------------------------*/ 

static short __CDECL
set_str_item( struct SET_ITEM_args args)
{
	char *ptext;
	char *str;

	ptext = args.tree[args.obj_index].ob_spec.tedinfo->te_ptext;	/* Pointer to string of the GTEXT object */

	if ( args.item )										/* LBOX_ITEM present?				*/
	{
		if ( args.item->selected )						/* Selected? 						*/
			args.tree[args.obj_index].ob_state |= OS_SELECTED;
		else
			args.tree[args.obj_index].ob_state &= ~OS_SELECTED;

		str = ((RESULT_ENTRY *)args.item)->str;			/* Pointer to the string 			*/

		if ( args.first == 0 )
		{
			if ( *ptext )
				*ptext++ = ' ';						/* Prefixed space character 		*/
		}
		else
			args.first -= 1;
		
		if ( args.first <= strlen( str ))
		{
			str += args.first;

			while ( *ptext && *str )
				*ptext++ = *str++;
		}
	}
	else											/* Unused entry 			  */
		args.tree[args.obj_index].ob_state &= ~OS_SELECTED;

	while ( *ptext )
		*ptext++ = ' ';								/* Pad string end with space characters */

	return( args.obj_index );								/* Object number of the start object 	*/
}

/*----------------------------------------------------------------------------------------*/ 
/* Eintrag in der Listenbox erzeugen													  */

static void
make_results(RESULT_ENTRY *ptr)
{
	long i;

	while(ptr) {
		strncpy ( ptr->str, ptr->node_name, 30 );
		ptr->str[30]=0;
		i = strlen ( ptr->str );
		for ( ; i < 30; i++ )
			strcat ( ptr->str, " ");
		strcat ( ptr->str, "\0");
		strncat ( ptr->str, ptr->dbase_description, 235 );
		ptr->str[255]=0;
		ptr->selected = 0;
		ptr=ptr->next;
	}
}

/*----------------------------------------------------------------------------------------*/ 
/* Service routine for window dialog 													  */
/* Function resul:	0: Close dialog     1: Continue								  */
/*	dialog:				Pointer to the dialog structure									  */
/*	events:				Pointer to EVNT structure or 0L								  */
/*	obj:				Number of the object or event number							  */
/*	clicks:				Number of mouse clicks											  */
/*	data:				Pointer to additional data									  */
/*----------------------------------------------------------------------------------------*/ 

static short __CDECL
SearchResultHandle( struct HNDL_OBJ_args args)
{
	OBJECT *tree;
	GRECT	rect;

	wdlg_get_tree(args.dialog, &tree, &rect);

	if ( args.obj < 0 )
	{
		if( args.obj == HNDL_INIT)
		{
			/* vertikale Listbox mit Auto-Scrolling und Real-Time-Slider anlegen */
			make_results( Result_List );
			entrie_box = lbox_create( tree_addr[SEARCH_RESULT], 
				select_item, set_str_item, (LBOX_ITEM *) Result_List,
				NO_ENTRIE, 0, entrie_ctrl, entrie_objs, 
				LBOX_VERT + LBOX_AUTO + LBOX_AUTOSLCT + LBOX_REAL + LBOX_SNGL, 
				40, SearchResult_Dialog, SearchResult_Dialog, 0, 0, 0, 0 );
		}
		else if ( args.obj == HNDL_CLSD )
		{
			lbox_free_items ( entrie_box );
			lbox_delete ( entrie_box );
			doneFlag = TRUE;
			return 0;
		}
		else if(args.obj == HNDL_MESG)
			SpecialMessageEvents(args.dialog , args.events);
	}
	else
	{
		nclicks = args.clicks;
		switch (args.obj)
		{
			case SR_ABORT:
			{
				tree[args.obj].ob_state &= (~OS_SELECTED);
				SendCloseDialog(args.dialog);
				break;
			}
			case SR_FSTL_UP:
			case SR_FSTL_BACK:
			case SR_FSTL_WHITE:
			case SR_FSTL_DOWN:
			case SR_BOX:
			case SR_FSTL_0:
			case SR_FSTL_1:
			case SR_FSTL_2:
			case SR_FSTL_3:
			case SR_FSTL_4:
			case SR_FSTL_5:
			case SR_FSTL_6:
			case SR_FSTL_7:
			case SR_FSTL_8:
			case SR_FSTL_9:
			{
				lbox_do(entrie_box, args.obj);
			}
		}
	}
	return 1;
}

/*----------------------------------------------------------------------------------------*/ 
/* Dialag fuer das Suchergebnis vorbereiten        										  */

static short
SearchResult (RESULT_ENTRY **result_list)
{
	SearchResult_Dialog = OpenDialog( SearchResultHandle, tree_addr[SEARCH_RESULT], "Search Result...", -1, -1);
	if ( SearchResult_Dialog == NULL )
	{
		free_results ( result_list );
		return 0;
	}
	return 1;
}

/* Ende */


/*----------------------------------------------------------------------------------------*/ 
/* Sucht einen String, ber die ALL.REF 												  */

#if DEBUG == ON
static void
print_results(RESULT_ENTRY *ptr)
{
	while(ptr) {
		Debug("Path=%s",ptr->path);
		Debug("Node:%s",ptr->node_name);
		Debug("Label:%d",ptr->is_label);
		Debug("Line:%d",ptr->line);
		Debug("Descr:%s",ptr->dbase_description);
		ptr=ptr->next;
	}
}
#endif


void free_results(RESULT_ENTRY **result_list)
{
RESULT_ENTRY *ptr,*next;
	
	ptr = *result_list;
	*result_list = NULL;
	
	while(ptr) {
		next = ptr->next;
		Mfree(ptr);
		ptr = next;
	}
}


/*----------------------------------------------------------------------------------------*/ 
/* Speicher in der "all.ref" Datei 														  */

void search_allref(char *string, short no_message )
{
	WINDOW_DATA *win = Win;
	long ret;
	REF_FILE *allref;
	short results=0;

#if DEBUG==ON
	Debug("All.ref Suche nach: %s ", string);
#endif

	/* Falls keine ALL.REF definiert wurde: abbruch */
	if(*all_ref == 0) {
		Debug("No ref file defined");
		return;
	}

	/*	REF-Datei oeffnen und laden	*/
	ret=Fopen(all_ref,O_RDONLY);
	if(ret < 0)
	{
		Debug("Error %ld in %s",ret,all_ref);
		return;
	}
		
	graf_mouse(BUSYBEE,NULL);
	allref = ref_load((short)ret);
	Fclose((short)ret);

	/*	Fehler beim Laden der Datei?	*/
	if(allref == NIL)
	{
		graf_mouse(ARROW,NULL);
		return;
	}

	Result_List = ref_findall(allref, string, &results);
	
#if DEBUG==ON
	print_results(Result_List);
#endif

	/*	Oeffne Resultat	*/	
	if(results > 0)
	{
		/* Nur ein Ergebnis gefunden */
		if ( results == 1 )
		{
			if(OpenFileSW(Result_List->path, Result_List->node_name, 0))
			{
				if(Result_List->line > 0)
					win->y_pos = Result_List->line;
			}
			free_results(&Result_List);
		}
		else
		{
			if(has_wlffp & 3)
			{
				if ( !SearchResult ( &Result_List ) )
					free_results ( &Result_List );
			}
			else
			{
				if(OpenFileSW(Result_List->path, Result_List->node_name, 0))
				{
					if(Result_List->line > 0)
						win->y_pos = Result_List->line;
				}
				free_results(&Result_List);
			}
		}
	}
	else
	{
		if ( !no_message )
		{
			char ZStr[255];
			
			sprintf ( ZStr, string_addr[WARN_NORESULT], string );
			form_alert (1, ZStr );
		}
	}
	
	graf_mouse(ARROW,NULL);
	
	Mfree(allref);
}
