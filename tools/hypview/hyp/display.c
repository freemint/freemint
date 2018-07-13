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

#include <gem.h>
#include "../diallib.h"
#include "../defs.h"
#include "../hyp.h"

extern WINDOW_DATA *Win;

/*long limage_add; */
HYP_DOCUMENT *Hyp;


static void
DrawPicture(WINDOW_DATA *win, char *src, long x, long y)
{
	LOADED_PICTURE *pic;
	short num = DEC_255(&src[1]);
	HYP_DOCUMENT *hyp = Hyp;

	pic = hyp->cache[num];

	if (pic)
	{
		short xy[8], tx, ty;
		MFDB screen = {NULL};

		tx = ((unsigned char *)src)[3];
		ty = dec_from_chars((unsigned char *) &src[4]);
		
		/*	Quellrechteck	*/
		xy[0] = 0;
		xy[1] = 0;
		xy[2] = pic->mfdb.fd_w - 1;
		xy[3] = pic->mfdb.fd_h - 1;
	
		/*	Zielrechteck	*/
		xy[4] = x + (tx * font_cw);
		xy[5] = y + ty;
		xy[6] = xy[4] + xy[2];
		xy[7] = xy[5] + xy[3];
	
		DIAG(("draw image at %d, %d, %d, %d", tx, ty, xy[2], xy[3]));
		
		if (xy[4] >= win->work.g_x + win->work.g_w || xy[6] < win->work.g_x)
			return;
		if (xy[5] >= win->work.g_y + win->y_offset + win->work.g_h || xy[7] < win->work.g_y)
			return;
/*******************************
{
		vsf_interior(vdi_handle,FIS_HOLLOW);
		v_bar(vdi_handle,&xy[4]);
}
*******************************/

		if (pic->mfdb.fd_nplanes == 1)
		{
			short col[2] = {G_BLACK,G_WHITE};
			vrt_cpyfm(vdi_handle, MD_TRANS, xy, &pic->mfdb, &screen, col);
		}
		else
			vro_cpyfm(vdi_handle, S_ONLY, xy, &pic->mfdb, &screen);
	}
}

static void
DrawLine(WINDOW_DATA *win, char *src, long x, long y)
{
	short xy[4], tx, ty, w, h, mode;

	tx = ((unsigned char *)src)[1];
	ty = dec_from_chars((unsigned char *) &src[2]);
	w = (((unsigned char *)src)[4]) - 128;
	h = (((unsigned char *)src)[5]) - 1;
	mode = ((unsigned char *)src)[6] - 1;

	/*	Rechteck	*/
	xy[0] = x + ((tx - 1) * font_cw);
	xy[1] = y + ty;
	xy[2] = xy[0] + w * font_cw;
	xy[3] = xy[1] + h * font_ch;
	
	if (w)
		xy[2]--;
	if (h)
		xy[3]--;
	
	if (xy[0] >= win->work.g_x + win->work.g_w || xy[2] < win->work.g_x)
		return;
	if (xy[1] >= win->work.g_y + win->y_offset + win->work.g_h || xy[3] < win->work.g_y)
		return;

/*
	Debug("LINE: X: %d Y: %d W: %d H: %d Type: %x (Arrows: %x, Style: %x",tx,ty,w,h,mode,mode&3,(mode>>3)+1);
	Debug("v_pline(%d,%d,[%d,%d][%d,%d]",vdi_handle,2,xy[0],xy[1],xy[2],xy[3]);
*/								
	DIAG(("Draw LINE at %d/%d/%d/%d", xy[0], xy[1], xy[2], xy[3]));

	vsl_ends(vdi_handle, mode & 1,(mode >> 1) & 1);
	vsl_type(vdi_handle, (mode >> 3) + 1);
	v_pline(vdi_handle, 2, xy);
}

static void
DrawBox(WINDOW_DATA *win, char *src, long x, long y)
{
	short xy[4], tx, ty, w, h, mode;

	tx = ((unsigned char *)src)[1];
	ty = dec_from_chars((unsigned char *) &src[2]);
	w = (((unsigned char *)src)[4]);
	h = (((unsigned char *)src)[5]);
	mode = ((unsigned char *)src)[6] - 1;

/*
	Debug("BOX %d: X: %d Y: %d W: %d H: %d Mode: %x",*src,tx,ty,w,h,mode);
*/

	/*	Rechteck	*/
	xy[0] = x + ((tx - 1) * font_cw);
	xy[1] = y + ty;
	xy[2] = xy[0] + w * font_cw - 1;
	xy[3] = xy[1] + h * font_ch - 1;

	if (xy[0] >= win->work.g_x + win->work.g_w || xy[2] < win->work.g_x)
		return;
	if (xy[1] >= win->work.g_y + win->y_offset + win->work.g_h || xy[3] < win->work.g_y)
		return;

	if(mode)
	{
		vsf_style(vdi_handle, mode);
		vsf_interior(vdi_handle, FIS_PATTERN);
	}
	else
	{
		vsf_interior(vdi_handle, FIS_HOLLOW);
	}

	if (*src == BOX)
	{
		DIAG(("Draw BOX at %d/%d/%d/%d", xy[0], xy[1], xy[2], xy[3]));
		v_bar(vdi_handle, xy);
	}
	else
	{
		DIAG(("Draw RBOX at %d/%d/%d/%d", xy[0], xy[1], xy[2], xy[3]));
		v_rfbox(vdi_handle, xy);
	}
}

void
HypDisplayPage(DOCUMENT *doc)
{
	WINDOW_DATA *win = Win;
	HYP_DOCUMENT *hyp = doc->data;
	LOADED_NODE *node = (LOADED_NODE *)hyp->entry;
	LINEPTR *line_ptr = node->line_ptr;
	long sx, sy, end_y;
	short line = 0;
	short org_y;
	char *src;

	Hyp = hyp;

	if(!hyp->entry)		/*	Abbruch falls keine Seite geladen	*/
		return;

	src = node->start;

	wind_get_grect(win->whandle,WF_WORKXYWH, &win->work);

	org_y = win->work.g_y + win->y_offset;
	win->work.g_h -= win->y_offset;

	sy = org_y - (win->y_pos * font_ch);
	end_y = sy + node->height;
	sx = win->work.g_x - (short)((long)(win->x_pos - 1) * font_cw);

	/*	Zuerst mssen die Grafik-Objekte dargestellt werden	*/
	vswr_mode(vdi_handle,MD_TRANS);
	vsf_color(vdi_handle,G_BLACK);
	vsf_perimeter(vdi_handle,1);
	vsl_color(vdi_handle,G_BLACK);
	
	/*	Der Grafikbereich muss (!) sich am Anfang der Seite befinden	*/
	while (*src == 27)
	{
		switch (src[1])
		{
			case PIC:
			{
				src++;
				DrawPicture(win, src, sx, sy);
				src += 8;
				break;
			}
			case LINE:
			{
				src++;
				DrawLine(win, src, sx, sy);
				src += 7;
				break;
			}
			case BOX:
			case RBOX:
			{
				src++;
				DrawBox(win, src, sx, sy);
				src += 7;
				break;
			}
			default:
				src = skip_esc(src);
		}
	}


	/*	Ab hier kommen die Text-Ausgaben	*/
	{
		DIAG(("0 DISPLAY: sy = %ld, endy = %ld", sy, end_y));

		if (end_y > org_y + win->work.g_h)
			end_y = org_y + win->work.g_h;
	
		DIAG(("1 DISPLAY: sy = %ld, endy = %ld", sy, end_y));
		
		/*	Standard Text-Farbe	*/
		vst_color(vdi_handle, text_col);
		
		while (line < node->lines)
		{
			long y2;
			y2 = sy + line_ptr->y + line_ptr->h;
			if ((sy + line_ptr->y) >= org_y || (y2 > org_y))
				break;
			sy = y2;
			line_ptr++;
			line++;
		}
		
		while (line < node->lines && sy < end_y && (sy += line_ptr->y) < end_y)
		{
			DIAG((" -- line %d, sy = %ld(%ld) (y=%d,h=%d)", line, sy, sy - org_y, line_ptr->y, line_ptr->h));
			
			if (line_ptr->txt)
			{
				char *end;
				short curr_txt_effect = 0;
				short x = sx;
				/*	Standard Text-Effekt	*/
				vst_effects(vdi_handle,0);
				src = line_ptr->txt;
				end = src;

				while (*end)
				{
					if (*end == 27)					/*	ESC-Sequenz ??	*/
					{

						*end = 0;				/*	Pufferende schreiben	*/
						
						/*	ungeschriebene Daten?	*/
						if (*src)
						{
							short ext[8];
							/*	noch fehlende Daten ausgeben	*/
							v_gtext(vdi_handle, x, sy, src);
							vqt_extent(vdi_handle, src, ext);
							x += (ext[2] + ext[4]) >> 1;
						}
						*end++ = 27;

						switch (*end)
						{
							case 27:				/*	ESC	*/
								src = end++;
								break;
							case LINK:				/* 36   */
							case 37:
							case ALINK:				/* 38	*/
							case 39:
							{
								unsigned short i;		/*	Index auf die Zielseite	*/
		
								if (*end & 1)			/*	Zeilennummer berspringen	*/
									end += 2;
		
								i = DEC_255(&end[1]);
								end += 3;

		
								/*	Verknpfungstext mit entsprechendem Texteffekt ausgeben	*/
								vst_color(vdi_handle,link_col);
								vst_effects(vdi_handle,link_effect|curr_txt_effect);
		
								/*	Verknuepfungstext ermitteln und ausgeben	*/
								if (*end == 32)		/*	Kein Text angegeben	*/
								{
									short ext[8];
									src = &hyp->indextable[i]->name;
									v_gtext(vdi_handle, x, sy, src);
									vqt_extent(vdi_handle, src, ext);
									x += (ext[2] + ext[4]) >> 1;
									end++;
								}
								else
								{
									short ext[8];
									char save_char;
									src = end + 1;
									end += (*(unsigned char *)end) - 32 + 1;
									save_char = *end;
									*end = 0;
									v_gtext(vdi_handle, x, sy, src);
									vqt_extent(vdi_handle, src, ext);
									x += (ext[2] + ext[4]) >> 1;
									*end = save_char;
								}

								vst_color(vdi_handle, text_col);
								vst_effects(vdi_handle, curr_txt_effect);
								src = end;
		
								break;
							}
							default:
							{
								if (*(unsigned char *)end >= 100)		/*	Text-Attribute	*/
								{
									curr_txt_effect = *((unsigned char *)end) - 100;
									vst_effects(vdi_handle, curr_txt_effect);
									end++;
								}
								else
									end = skip_esc(end - 1);
								src = end;
								break;
							}
						}
					}
					else
						end++;
				}
				*end = 0;				/*	Pufferende schreiben	*/
				if (*src)
				{
					v_gtext(vdi_handle, x, sy, src);
				}

			}
			line++;
			sy += line_ptr->h;
			line_ptr++;
		}
	}
}
