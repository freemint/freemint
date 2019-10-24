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
#include <ctype.h>
#include <gem.h>
#include "../diallib.h"
#include "../defs.h"
#include "../include/av.h"
#include "../hyp.h"

extern HYP_DOCUMENT *Hyp;
extern WINDOW_DATA *Win;

static void
get_word(char *min,char *src,char *dst)
{
	unsigned char *ptr=(unsigned char *)src;

	while(((char *)ptr>=min)&&
			(isalnum(*ptr) ||
			((*ptr>=128)&&(*ptr<=154)) ||
			((*ptr>=160)&&(*ptr<=167)) ||
			((*ptr>=176)&&(*ptr<=184)) ||
			(*ptr==192)||(*ptr==193)||
			(*ptr==158)||
			(*ptr=='-')))
	{
		ptr--;
	}

	if((char *)ptr!=src)
	{
		ptr++;
	
		do
		{
			*dst++=*ptr++;
		}while((isalnum(*ptr) ||
				((*ptr>=128)&&(*ptr<=154)) ||
				((*ptr>=160)&&(*ptr<=167)) ||
				((*ptr>=176)&&(*ptr<=184)) ||
				(*ptr==192)||(*ptr==193)||
				(*ptr==158)||
				(*ptr=='-')));
	}
	else
		*dst++=*ptr;
	*dst=0;	
}
	
void HypClick(DOCUMENT *doc, EVNTDATA *m)
{
	WINDOW_DATA *win = Win;
	HYP_DOCUMENT *hyp;
	LOADED_NODE *node;
	short xy[8], x_pos = font_cw;
	short curr_txt_effect = 0;
	char test_mem[2];
	char *src, *last_esc;
	LINEPTR *line_ptr;

	Hyp = hyp = doc->data;

	wind_get_grect(win->whandle, WF_WORKXYWH, &win->work);
	m->x += (short)win->x_pos * win->x_raster - win->work.g_x;
	m->y -= win->work.g_y + win->y_offset;

	node = (LOADED_NODE *)hyp->entry;

	line_ptr = HypGetYLine(node, m->y + (win->y_pos * font_ch));
	src = line_ptr ? line_ptr->txt : NULL;

	if(!src)
		return;
	
	last_esc = src;
	test_mem[1] = 0;
	vst_effects(vdi_handle, curr_txt_effect);

	while (*src)
	{
		if (*src == 27)
		{
			src++;
			if (*src == 27)
			{
				*test_mem = 27;
				goto check_char;
			}
			else if (*src >= LINK && *src <= ALINK + 1)
			{
				char link_type = *src;		/*	Linktyp merken	*/
				unsigned short line_nr = 0;	/*	anzuspringende Zeilennummer	*/
				unsigned short dst_page;	/*	Index auf die Zielseite	*/
				short link_len;

				src++;
				
				if (link_type & 1)		/*	Zeilennummer ueberspringen	*/
				{
					line_nr = DEC_255(src);
					src += 2;
				}

				dst_page = DEC_255(src);	/*	Index	*/
				src += 2;
				
				vst_effects(vdi_handle, link_effect);

				link_len = (*(unsigned char *)src++) - 32;
				
				if (link_len)
				{
					*test_mem = src[link_len];
					src[link_len] = 0;
					vqt_extent(vdi_handle, src, xy);
					src[link_len] = *test_mem;
				}
				else
					vqt_extent(vdi_handle, &hyp->indextable[dst_page]->name, xy);

				vst_effects(vdi_handle, curr_txt_effect);

				x_pos += xy[0] + xy[2];
				
				if(x_pos > m->x)
				{
					short dst_type = hyp->indextable[dst_page]->type;
					
/*
					Debug("Link to node %d line %d (=%s type %d): Type is %d",dst_page,line_nr,&hyp->indextable[dst_page]->name,dst_type,link_type);
*/
					if(dst_type == INTERNAL)
					{
						if (m->kstate & K_CTRL || (link_type >= ALINK))
							OpenFileNW(doc->path, &hyp->indextable[dst_page]->name, 0);
						else
						{
							AddHistoryEntry(win);
							GotoPage(doc,dst_page,line_nr, 1);
						}
					}
					else if (dst_type == POPUP)
						OpenPopup(doc, dst_page, x_pos - xy[2], m->y - m->y % font_ch);
					else if (dst_type == EXTERNAL_REF)
					{
						HypOpenExtRef(&hyp->indextable[dst_page]->name,
								m->kstate & K_CTRL || (link_type>=ALINK));
					}
					else if (dst_type == REXX_COMMAND)
					{
						short dst_id;
						/*	Host suchen...	*/
						dst_id = appl_find(hyp->hostname);
						if (dst_id < 0)		/*	Host nicht gefunden?	*/
						{
							Cconout(7);
							break;			/*	... Abbruch	*/
						}
							
						/*	Parameter per VA_START an Host senden	*/
						if (av_parameter)
							Debug("An AV action is already running");
						else
						{
							char temp[DL_PATHMAX];
							char *prog = &hyp->indextable[dst_page]->name;
							char *fn = hyp->file, *dfn = temp;
	
							/*	Suche die Datei im Verzeichnis des Hypertexts	*/
							while (*fn)
								*dfn++ = *fn++;
							while(*--dfn != dir_separator)
								*dfn = 0;
							strcpy(++dfn, prog);
	/*						Debug("temp=%s",temp);
	*/
							/*	Datei nicht mit shel_find() auffindbar?	*/
							if (!shel_find(temp))
							{
								strcpy (temp,prog);		/*	Parameter so wie angegeben senden	*/
	/*							Debug("temp2=%s",temp);
	*/						}

							av_parameter = (char *)Mxalloc(strlen(temp) + 1, MX_PREFTTRAM|MX_MPROT|MX_GLOBAL);
							if ((long)av_parameter == -32l)
								av_parameter = (char *)Malloc(strlen(temp) + 1L);
							
							if (av_parameter)
							{
								short msg[8] = {VA_START,0,0,0,0,0,0,0};
								strcpy(av_parameter, temp);

								msg[1] = ap_id;
								msg[3] = (short)((long)av_parameter >> 16);
								msg[4] = (short)((long)av_parameter);
								appl_write(dst_id, 16, msg);
							}
						}
					}
					else if ((dst_type == SYSTEM_ARGUMENT) || (dst_type == REXX_SCRIPT))
					{
						/*	Parameter per AV_STARTPROG vom Server starten lassen	*/
						if (av_parameter)
							Debug("An AV action is already running");
						else
						{
							char temp[DL_PATHMAX];
							char *prog = &hyp->indextable[dst_page]->name;
							char *fn = hyp->file, *dfn = temp;
	
							/*	Suche die Datei im Verzeichnis des Hypertexts	*/
							while (*fn)
								*dfn++ = *fn++;
							while (*--dfn != dir_separator)
								*dfn = 0;
							strcpy(++dfn, prog);
/*							Debug("temp=%s",temp);
*/	
							if(!shel_find(temp))		/*	Datei nicht auffindbar?	*/
							{
								strcpy(temp, prog);
/*								Debug("temp2=%s",temp);
*/							}
	
							av_parameter = (char *)Mxalloc(strlen(temp)+1,  MX_PREFTTRAM|MX_MPROT|MX_GLOBAL);
							if ((long)av_parameter == -32l)
								av_parameter = (char *)Malloc(strlen(temp)+1);
							if(av_parameter)
							{
								strcpy(av_parameter,temp);
								DoAV_STARTPROG(av_parameter, NULL, 0x1234);
							}
						}
					}
					else if(dst_type == QUIT)		/*	Fenster schliessen?	*/
					{
						SendClose(win);
					}
					else
					{
						Debug("Link to node %d (=%s type %d): Type is %d",dst_page, &hyp->indextable[dst_page]->name,dst_type,link_type);
					}
					break;
				}
				src += link_len;
			}
			else
			{

				if ((*(unsigned char *)src) >= 100)		/*	Text-Attribute	*/
				{
					curr_txt_effect = *((unsigned char *)src) - 100;
					vst_effects(vdi_handle, curr_txt_effect);
					src++;
				}
				else
					src = skip_esc(src - 1);
			}
			last_esc = src;
		}
		else
		{
			*test_mem = *src;
check_char:
			vqt_extent(vdi_handle, test_mem, xy);
			x_pos += xy[0] + xy[2];
			src++;
		}

		if(x_pos > m->x)
		{
			if ( !refonly )
			{
				long ret;
				char buffer[256];
				
				get_word(last_esc, src - 1, buffer);
				ret = HypFindNode(doc, buffer);
				if ( ret >= 0 )
				{
					doc->gotoNodeProc(doc, NULL, ret);
					ReInitWindow(win, doc);
				}
				else
					search_allref( buffer, TRUE );
			}

			break;
		}
	}
}

