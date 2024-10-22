/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for FreeMiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "global.h"
#include "xa_global.h"

#include "xaaes.h"

#include "xa_pdlg.h"
#include "xa_wdlg.h"
#include "xa_fsel.h"
#include "k_main.h"
#include "k_mouse.h"

#include "app_man.h"
#include "form.h"
#include "obtree.h"
#include "draw_obj.h"
#include "c_window.h"
#include "rectlist.h"
#include "widgets.h"
#include "xa_graf.h"
#include "xa_rsrc.h"
#include "xa_form.h"
#include "scrlobjc.h"

#include "xa_user_things.h"
#include "nkcc.h"
#include "mint/signal.h"

/*
 * WDIALOG FUNCTIONS (pdlg)
 *
 * documentation about this can be found in:
 *
 * - the GEMLIB documentation:
 *   http://arnaud.bercegeay.free.fr/gemlib/
 *
 * - the MagiC documentation project:
 *   http://www.bygjohn.fsnet.co.uk/atari/mdp/
 */

#if WDIALOG_PDLG

extern struct toolbar_handlers wdlg_th;


static char selafile[] = ">> No file selected <<";

static char *outfiles[] =
{
	"FILE:", selafile,
	"AUX:", "Serial device",
	"PRN:", "Parallel device",
	"ACSI:", "ACSI device",
	"SCSI:", "SCSI device",
	NULL, NULL,
};

#if 0
static void
display_pset(PRN_SETTINGS *p)
{
	display("PRN_SETTINGS: -----------------------------");
	display("Magic        %lx", p->magic);
	display("lenght       %ld", p->length);
	display("format       %ld", p->format);
	display("page flags   %lx", p->page_flags);
	display("first page   %d", p->first_page);
	display("last page    %d", p->last_page);
	display("copies       %d", p->no_copies);
	display("orientation  %d", p->orientation);
	display("scale        %lx", p->scale);
	display("driver id    %d", p->driver_id);
	display("driver type  %d", p->driver_type);
	display("driver mode  %ld", p->driver_mode);
	display("printer id   %ld", p->printer_id);
	display("modes id     %ld", p->mode_id);
	display("modes hdpi   %d", p->mode_hdpi);
	display("modes vdpi   %d", p->mode_vdpi);
	display("quality id   %ld", p->quality_id);
	display("color mode   %ld (%lx)", p->color_mode, p->color_mode);
	display("plane flags  %lx", p->plane_flags);
	display("dither mode  %ld", p->dither_mode);
	display("dither value %ld", p->dither_value);
	display("size id      %ld", p->size_id);
	display("type id      %ld", p->type_id);
	display("input id     %ld", p->input_id);
	display("output id    %ld", p->output_id);
	display("contrast     %ld", p->contrast);
	display("brightness   %ld", p->brightness);
	display("device       '%s'", p->device);
	display("-------------------------------------------");

}
#endif

#define CALL_INIT_DLG	0
#define CALL_DO_DLG	1
#define CALL_RESET_DLG	2

static long
callout_pdlg_sub(struct xa_pdlg_info *pdlg, short which, PDLG_SUB *sub, PRN_SETTINGS *pset, short obj)
{
	long ret = 0;

	if (sub->tree)
	{
		GRECT r;
		obj_rectangle(pdlg->mwt, aesobj(pdlg->mwt->tree, XPDLG_DIALOG), &r);
		sub->tree->ob_x = r.g_x;
		sub->tree->ob_y = r.g_y;
	}

	switch( which )
	{
	case CALL_INIT_DLG:
		if( sub->init_dlg && pdlg->n_printers > 0 )	/* 0 would qed or fvdi (and XaAES) crash */
			ret = sub->init_dlg( pset, sub );
		break;
	case CALL_DO_DLG:
		if( sub->do_dlg )
		{
			struct PDLG_HNDL_args args;
			args.settings = pset;
			args.sub = sub;
			args.exit_obj = obj;
			ret = sub->do_dlg(args);
		}
		break;
	case CALL_RESET_DLG:
		if( sub->reset_dlg )
			ret = sub->reset_dlg( pset, sub );
		break;
	}
	return ret;
}

static void
delete_driver_list(struct xa_pdlg_info *pdlg)
{
	struct xa_prndrv_info *p;
	while ((p = pdlg->drivers))
	{
		pdlg->drivers = p->next;

		if (!(p->flags & DRVINFO_KMALLOC))
			v_delete_driver_info(C.P_handle, p->d);
		else
		{
			kfree(p->d);
		}

		kfree(p);
	}
}

#if 0
static void
display_driver_list(struct xa_pdlg_info *pdlg)
{
	struct xa_prndrv_info *p = pdlg->drivers;
	PRN_ENTRY *printer;

	while (p)
	{
		display(" ----------------------------------------- ");
		display(" dname : '%s'", p->dname);
		display(" path / file : '%s'/'%s'", p->path, p->fname);
		display("drvinf at %lx, id=%d, type=%d, format=%ld, device '%s'", p->d, p->d->driver_id, p->d->driver_type, p->d->format, p->d->device);

		printer = p->d->printers;

		while (printer)
		{
			PRN_TRAY *tray;

			display("  printer '%s'", printer->name);
			display("  -- subdlg %lx, setup_panel=%lx, close_panel=%lx", printer->sub_dialogs, printer->setup_panel, printer->close_panel);
			tray = printer->input_trays;
			ndisplay("   input trays;");
			while (tray)
			{
				ndisplay("'%s' ", tray->name);
				tray = tray->next;
			}
			display("");
			tray = printer->output_trays;
			ndisplay("   output trays;");
			while (tray)
			{
				ndisplay("'%s' ", tray->name);
				tray = tray->next;
			}
			display("");
			printer = printer->next;
		}
		p = p->next;
	}
}
#endif

static void
count_printers(struct xa_pdlg_info *pdlg)
{
	short count = 0;
	struct xa_prndrv_info *p = pdlg->drivers;
	PRN_ENTRY *printer;

	while (p)
	{
		if ((printer = p->d->printers))
		{
			while (printer)
			{
				count++;
				printer = printer->next;
			}
		}
		else
			count++;
		p = p->next;
	}
	pdlg->n_printers = count;

}

static void
get_driver_list(struct xa_pdlg_info *pdlg)
{
	int i;
	char path[128], fname[32], dname[128];
	DRV_INFO *drvinf;
	struct xa_prndrv_info *start, *prev, *xad;
	short exists;

	delete_driver_list(pdlg);
	start = prev = xad = NULL;
	pdlg->n_drivers = 0;

	for (i = 21; i < 100; i++)
	{
		vq_ext_devinfo(C.P_handle, i, &exists, path, fname, dname);
		if (exists)
		{
			long flags = 0L;

			drvinf = v_create_driver_info(C.P_handle, i);

			if (!drvinf && i == 31)	/* Metafile */
			{
				if ((drvinf = kmalloc(sizeof(*drvinf))))
				{
					bzero(drvinf, sizeof(*drvinf));

					drvinf->magic = 0x70626e66;
					drvinf->length = sizeof(*drvinf);
					drvinf->format = 0L;
					drvinf->driver_id = 31;
					drvinf->driver_type = 4;
					strcpy(drvinf->device, "gemfile.gem");
					flags |= DRVINFO_KMALLOC;
				}
			}

			if (drvinf)
			{
				xad = kmalloc(sizeof(*xad));
				assert(xad);
				if (!prev)
					start = prev = xad;
				else
				{
					prev->next = xad;
					prev = xad;
				}

				bzero(xad, sizeof(*xad));
				xad->flags = flags;
				strncpy(xad->path, path, 127);
				strncpy(xad->fname, fname, 31);
				strncpy(xad->dname, dname, 127);
				xad->d = drvinf;
				pdlg->n_drivers++;
			}
		}
		/* Ozk: Memory drivers have IDs 61 - 70,
		 *	and I promise, everyone, this _will_ skip ALL memory drivers!!! */
		if (i == 60) i = 70;
	}

	pdlg->drivers = start;

	count_printers(pdlg);
}

struct xa_window *
get_pdlg_wind(struct xa_client *client, void *pdlg_ptr)
{
	struct xa_pdlg_info *pdlg = lookup_xa_data(&client->xa_data, pdlg_ptr);

	if (pdlg)
		return pdlg->wind;
	else
		return NULL;
}

static void _cdecl
delete_pdlg_info(void *_pdlg)
{
	struct xa_pdlg_info *pdlg = _pdlg;

	if (pdlg)
	{
		delete_driver_list(pdlg);

		if (pdlg->dwt)
		{
			pdlg->dwt->links--;
			remove_wt(pdlg->dwt, false);
			pdlg->dwt = NULL;
		}
		if (pdlg->mwt)
		{
			pdlg->mwt->links--;
			remove_wt(pdlg->mwt, false);
			pdlg->mwt = NULL;
		}

		if (pdlg->drv_wt)
		{
			pdlg->drv_wt->links--;
			remove_wt(pdlg->drv_wt, false);
		}

		if (pdlg->mode_wt)
		{
			pdlg->mode_wt->links--;
			remove_wt(pdlg->mode_wt, false);
		}
		if (pdlg->color_wt)
		{
			pdlg->color_wt->links--;
			remove_wt(pdlg->color_wt, false);
		}
		if (pdlg->dither_wt)
		{
			pdlg->dither_wt->links--;
			remove_wt(pdlg->dither_wt, false);
		}
		if (pdlg->type_wt)
		{
			pdlg->type_wt->links--;
			remove_wt(pdlg->type_wt, false);
		}
		if (pdlg->size_wt)
		{
			pdlg->size_wt->links--;
			remove_wt(pdlg->size_wt, false);
		}
		if (pdlg->itray_wt)
		{
			pdlg->itray_wt->links--;
			remove_wt(pdlg->itray_wt, false);
		}
		if (pdlg->otray_wt)
		{
			pdlg->otray_wt->links--;
			remove_wt(pdlg->otray_wt, false);
		}
		if (pdlg->outfile_wt)
		{
			pdlg->outfile_wt->links--;
			remove_wt(pdlg->outfile_wt, false);
		}

		kfree(pdlg);
	}
}
static bool
set_oblink(struct xa_pdlg_info *pdlg, OBJECT *to_tree, short to_obj)
{
	struct oblink_spec *obl;
	struct widget_tree *wt = pdlg->mwt;
	bool init = false, upd = false, resize = false;

	if ((obl = object_get_oblink(wt->tree + XPDLG_DIALOG)))
	{
		if (aesobj_tree(&wt->focus) == aesobj_tree(&obl->to))
			wt->focus = inv_aesobj();
		if (aesobj_tree(&wt->e.o) == aesobj_tree(&obl->to))
			wt->e.o = inv_aesobj();
		if (wt->ei && aesobj_tree(&wt->ei->o) == aesobj_tree(&obl->to))
			obj_edit(wt, pdlg->wind->vdi_settings, ED_END, aesobj(wt->tree, 0), 0, 0, NULL, false, NULL, NULL, NULL, NULL);
	}
	else
	{
		obl = kmalloc(sizeof(*obl));
		init = true;
	}

	if (obl)
	{
		struct xa_aes_object to = aesobj(to_tree, to_obj);
		GRECT r;

		if (init || !same_aesobj(&obl->to, &to))
		{

			obj_rectangle(wt, to, &r);

			if (aesobj_ob(&to)->ob_width < pdlg->min_w)
				aesobj_ob(&to)->ob_width = pdlg->min_w;

			if (aesobj_ob(&to)->ob_width != wt->tree[XPDLG_DIALOG].ob_width)
			{
				wt->tree[XPDLG_DIALOG].ob_width = aesobj_ob(&to)->ob_width;
				wt->tree->ob_width = wt->tree[XPDLG_DIALOG].ob_x + aesobj_ob(&to)->ob_width + 4;
				resize = true;
			}

			if (aesobj_ob(&to)->ob_height < pdlg->min_h)
				aesobj_ob(&to)->ob_height = pdlg->min_h;

			if (aesobj_ob(&to)->ob_height != wt->tree[XPDLG_DIALOG].ob_height)
			{
				wt->tree[XPDLG_DIALOG].ob_height = aesobj_ob(&to)->ob_height;
				wt->tree->ob_height = wt->tree[XPDLG_DIALOG].ob_y + wt->tree[XPDLG_DIALOG].ob_height + wt->tree[XPDLG_PRINT].ob_height + 16;

				wt->tree[XPDLG_PRINT].ob_y = wt->tree[XPDLG_DIALOG].ob_y + wt->tree[XPDLG_DIALOG].ob_height + 8;
				wt->tree[XPDLG_CANCEL].ob_y = wt->tree[XPDLG_PRINT].ob_y;
				resize = true;
			}

			obl->to   = to;
			upd = true;
		}
		if (init)
		{
			obl->from = aesobj(wt->tree, XPDLG_DIALOG);
			obl->save_obj = *(wt->tree + XPDLG_DIALOG);
			wt->tree[XPDLG_DIALOG].ob_type = G_OBLINK;
			object_set_spec(wt->tree + XPDLG_DIALOG, (unsigned long)obl);
		}
		if (resize)
		{
			obj_rectangle(wt, aesobj(wt->tree, 0), &r);
			if (wt->widg)
			{
				wt->widg->r.g_w = r.g_w;
				wt->widg->r.g_h = r.g_h;
			}

			if (!pdlg->flags & 1)
				r = w2f(&pdlg->wind->delta, &r, true);
			else
				r = w2f(&pdlg->wind->wadelta, &r, true);


			move_window(0, pdlg->wind, false, -1, r.g_x, r.g_y, r.g_w, r.g_h);
			upd = false;
		}
		if ((upd || resize) && (!wt->ei || !edit_set(wt->ei)))
		{
			struct xa_aes_object new_eobj;
			new_eobj = ob_find_next_any_flagstate(wt, aesobj(wt->tree, 0), inv_aesobj(), OF_EDITABLE, OF_HIDETREE, 0, OS_DISABLED, 0, 0, OBFIND_FIRST);
			if (valid_aesobj(&new_eobj))
				obj_edit(wt, pdlg->wind->vdi_settings, ED_INIT, new_eobj, 0, -1, NULL, false, NULL,NULL, NULL,NULL);
		}
	}
	return upd;
}

static int
click_list(SCROLL_INFO *list, SCROLL_ENTRY *this, const struct moose_data *md)
{
	if (this)
	{
		struct xa_pdlg_info *pdlg = list->data;
		struct xa_window *wind = pdlg->wind;
		PDLG_SUB *sub = this->data;

		if (sub && pdlg->current_subdlg != sub)
		{
			if (pdlg->current_subdlg)
				callout_pdlg_sub(pdlg, CALL_RESET_DLG, pdlg->current_subdlg, pdlg->settings, 0);

			callout_pdlg_sub(pdlg, CALL_INIT_DLG, sub, pdlg->settings, 0);

			if (set_oblink(pdlg, sub->tree, sub->index_offset))
			{
				sub->dialog = pdlg->handle;
				wdialog_redraw(0, wind, aesobj(pdlg->mwt->tree, XPDLG_DIALOG), 7, NULL);
			}

			pdlg->current_subdlg->dialog = NULL;
			pdlg->current_subdlg = sub;
		}
	}
	return 0;
}

static void
set_driver_pset(DRV_INFO *driver, PRN_SETTINGS *ps)
{
	if (driver)
	{
		ps->driver_id = driver->driver_id;
		ps->driver_type = driver->driver_type;
	}
	else
	{
		ps->driver_id = 0;
		ps->driver_type = 0;
	}
}

static void
set_size_pset(MEDIA_SIZE *size, PRN_SETTINGS *ps)
{
	if (size)
		ps->size_id = size->size_id;
	else
		ps->size_id = 0;
}

static void
set_type_pset(MEDIA_TYPE *type, PRN_SETTINGS *ps)
{
	if (type)
		ps->type_id = type->type_id;
	else
		ps->type_id = 0;
}

static void
set_printer_pset(PRN_ENTRY *printer, PRN_SETTINGS *ps)
{
	if (printer)
		ps->printer_id = printer->printer_id;
	else
		ps->printer_id = 0;
}

static void
set_mode_pset(PRN_MODE *mode, PRN_SETTINGS *ps)
{
	if (mode)
	{
		ps->mode_id	= mode->mode_id;
		ps->mode_hdpi	= mode->hdpi;
		ps->mode_vdpi	= mode->vdpi;
	}
	else
	{
		ps->mode_id	= 0;
		ps->mode_hdpi	= 0;
		ps->mode_vdpi	= 0;
	}
}

static void
set_dither_pset(DITHER_MODE *dither, PRN_SETTINGS *ps)
{
	if (dither)
	{
		ps->dither_mode = dither->dither_id;
	}
	else
	{
		ps->dither_mode = 0;
	}
}

static void
set_color_mode_pset(long cm, PRN_SETTINGS *ps)
{
	ps->color_mode = cm;
}

static void
set_trays_pset(PRN_TRAY *itray, PRN_TRAY *otray, PRN_SETTINGS *ps)
{
	if (itray)
		ps->input_id = itray->tray_id;
	else
		ps->input_id = 0;
	if (otray)
		ps->output_id = otray->tray_id;
	else
		ps->output_id = 0;
}

static void
set_outfile_pset(char *f, PRN_SETTINGS *ps)
{
	strcpy(ps->device, f);
}

/* ------------------------------------------------- */
/* -    Dialog tree modification functions         - */
/* ------------------------------------------------- */
static long
d_get_page_sel(struct xa_pdlg_info *pdlg)
{
	unsigned long flags = 0L;

	if (issel(pdlg->dwt->tree + PDLG_G_EVEN))
		flags |= PG_EVEN_PAGES;
	if (issel(pdlg->dwt->tree + PDLG_G_ODD))
		flags |= PG_ODD_PAGES;
	return flags;
}
static void
d_set_page_sel(struct xa_pdlg_info *pdlg, long flags)
{
	setsel(pdlg->dwt->tree + PDLG_G_EVEN, (flags & PG_EVEN_PAGES));
	setsel(pdlg->dwt->tree + PDLG_G_ODD,  (flags & PG_ODD_PAGES));
}

static void
d_set_fromto_page(struct xa_pdlg_info *pdlg, short which, short num)
{
	char *t = object_get_tedinfo(pdlg->dwt->tree + (which ? PDLG_G_TO : PDLG_G_FROM), NULL)->te_ptext;
	sprintf(t, 5, "%d", num);
}
static short
d_get_fromto_page(struct xa_pdlg_info *pdlg, short which)
{
	if (issel(pdlg->dwt->tree + PDLG_G_SELALL))
		return which ? 9999 : 1;
	return  atol(object_get_tedinfo(pdlg->dwt->tree + (which ? PDLG_G_TO : PDLG_G_FROM), NULL)->te_ptext);
}
static void
d_set_allfrom_sel(struct xa_pdlg_info *pdlg, short which)
{
	obj_set_radio_button(pdlg->mwt, pdlg->wind->vdi_settings, aesobj(pdlg->dwt->tree, which ? PDLG_G_SELFROM : PDLG_G_SELALL), false, NULL, pdlg->wind->rect_list.start);
}
static void
d_set_copies(struct xa_pdlg_info *pdlg, short copies)
{
	char *t = object_get_tedinfo(pdlg->dwt->tree + PDLG_G_COPIES, NULL)->te_ptext;
	sprintf(t, 5, "%d", copies);
}
static short
d_get_copies(struct xa_pdlg_info *pdlg)
{
	OBJECT *ob = pdlg->dwt->tree + PDLG_G_COPIES;

	if (object_disabled(ob))
		return 1;
	else
		return  atol(object_get_tedinfo(ob, NULL)->te_ptext);
}

static short
d_get_orientation(struct xa_pdlg_info *pdlg)
{
	short o = 0;

	if (issel(pdlg->dwt->tree + PDLG_P_LETTER))
		o |= PG_PORTRAIT;
	else if (issel(pdlg->dwt->tree + PDLG_G_LAND))
		o |= PG_LANDSCAPE;
	return o;
}
static void
d_set_orientation(struct xa_pdlg_info *pdlg, short o)
{
	setsel(pdlg->dwt->tree + PDLG_P_LETTER, (o & PG_PORTRAIT));
	setsel(pdlg->dwt->tree + PDLG_G_LAND, (o & PG_LANDSCAPE));
}

static void
d_set_scale(struct xa_pdlg_info *pdlg, long _scale)
{
	unsigned long scale = _scale;
	char *t = object_get_tedinfo(pdlg->dwt->tree + PDLG_P_SCALE, NULL)->te_ptext;

	scale *= 100;
	scale >>= 16;

	sprintf(t, 4, "%lu", scale);
}
static long
d_get_scale(struct xa_pdlg_info *pdlg)
{
	OBJECT *ob = pdlg->dwt->tree + PDLG_P_SCALE;

	if (object_disabled(ob))
		return 0x10000L;
	else
	{
		unsigned long scale =  atol(object_get_tedinfo(ob, NULL)->te_ptext);

		scale <<= 16;
		scale /= 100;

		return scale;
	}
}

static void
d_set_outfile(struct xa_pdlg_info *pdlg, char *tofile)
{
	char **files, *txt = NULL;
	int idx = 0;

	files = pdlg->outfiles;

	while (*files)
	{
		if (!strcmp(tofile, *files))
		{
			break;
		}
		idx++;
		files += 2;
	}

	if (*files)
	{
		if (!strcmp(*files, "FILE:"))
		{
			char *t;

			if (!(t = files[1]))
				t = *files;
			else
				txt = t;
			idx++;
		}
	}
	else
	{
		idx = 1;
		files = pdlg->outfiles;

		while (*files)
		{
			if (!strcmp(*files, "FILE:"))
				break;
			files += 2;
			idx++;
		}
		if (*files)
		{
			strcpy(pdlg->filesel, tofile);
			files[1] = pdlg->filesel;
			txt = pdlg->filesel;
		}
	}

	if (txt)
	{
		int len0, len1;

		len0 = strlen(selafile);
		len1 = strlen(txt);

		if (len1 > len0)
		{
			char nt[len0];

			strncpy(nt, txt, len0);
			txt = &nt[len0];
			*txt-- = '\0';
			*txt-- = '.';
			*txt-- = '.';
			*txt-- = '.';
			obj_change_popup_entry(aesobj(pdlg->dwt->tree, PDLG_I_PORT), idx, nt);
		}
		else
			obj_change_popup_entry(aesobj(pdlg->dwt->tree, PDLG_I_PORT), idx, txt);
	}
	obj_draw(pdlg->mwt, pdlg->wind->vdi_settings, aesobj(pdlg->dwt->tree, PDLG_I_PORT), -1, NULL, pdlg->wind->rect_list.start, 0);
}
static void
d_set_prn_fgbg(struct xa_pdlg_info *pdlg, bool bg)
{
	obj_set_radio_button(pdlg->mwt, pdlg->wind->vdi_settings, aesobj(pdlg->dwt->tree, bg ? PDLG_I_BG : PDLG_R_FG), false, NULL, pdlg->wind->rect_list.start);
}
static long
d_get_prn_fgbg(struct xa_pdlg_info *pdlg)
{
	OBJECT *ob = pdlg->dwt->tree + PDLG_I_BG;

	return (!object_disabled(ob) && issel(ob)) ? (long)DM_BG_PRINTING : 0L;
}

static void
d_set_plane_flags(struct xa_pdlg_info *pdlg, long plane_flags)
{
	setsel(pdlg->dwt->tree + PDLG_R_C, (plane_flags & PLANE_CYAN));
	setsel(pdlg->dwt->tree + PDLG_R_M, (plane_flags & PLANE_MAGENTA));
	setsel(pdlg->dwt->tree + PDLG_R_Y, (plane_flags & PLANE_YELLOW));
	setsel(pdlg->dwt->tree + PDLG_R_K, (plane_flags & PLANE_BLACK));
}

static long
d_get_plane_flags(struct xa_pdlg_info *pdlg)
{
	long plane_flags = 0L;

	if (object_disabled(pdlg->dwt->tree + PDLG_R_C))
		plane_flags = PLANE_MASK;
	else
	{
		if (issel(pdlg->dwt->tree + PDLG_R_C))
			plane_flags |= PLANE_CYAN;
		if (issel(pdlg->dwt->tree + PDLG_R_M))
			plane_flags |= PLANE_MAGENTA;
		if (issel(pdlg->dwt->tree + PDLG_R_Y))
			plane_flags |= PLANE_YELLOW;
		if (issel(pdlg->dwt->tree + PDLG_R_K))
			plane_flags |= PLANE_BLACK;
	}
	return plane_flags;
}

/* ------------------------------------------------- */

static void
build_prn_settings(struct xa_pdlg_info *pdlg, PRN_SETTINGS *ps)
{
	ps->magic	= 0x70736574;		/* 'pset' */
	ps->length	= sizeof(*ps);
	ps->format	= 1;

	ps->page_flags	= d_get_page_sel(pdlg);

	ps->first_page = d_get_fromto_page(pdlg, 0);
	ps->last_page = d_get_fromto_page(pdlg, 1);

	ps->no_copies = d_get_copies(pdlg);

	ps->orientation = d_get_orientation(pdlg);
	ps->scale = d_get_scale(pdlg);

	set_driver_pset(pdlg->curr_drv, ps);

	ps->driver_mode &= ~1L;
	ps->driver_mode |= d_get_prn_fgbg(pdlg);

	set_printer_pset(pdlg->curr_prn, ps);
	set_mode_pset(pdlg->curr_mode, ps);

	ps->quality_id	= 0;
	ps->plane_flags = d_get_plane_flags(pdlg);

	set_dither_pset(pdlg->curr_dither, ps);

	set_size_pset(pdlg->curr_size, ps);
	set_type_pset(pdlg->curr_type, ps);

	set_trays_pset(pdlg->curr_itray, pdlg->curr_otray, ps);

	ps->contrast = 0x10000L;
	ps->brightness = 0x10000L;
}

struct np_parms
{
	struct xa_pdlg_info *pdlg;
	struct xa_prndrv_info *curr_drv;
	PRN_ENTRY *curr_prn;
};

/*
 * create_pop_tree calls this twice, once to calculate memneeds,
 * and a second time to actually create the popup. 'item' is 0
 * each time a 'new run' starts.
 */
static void *
next_printer(short item, void **data)
{
	struct np_parms *parm = *data;
	PRN_ENTRY *printer;
	char *ret;

	if (!item)
	{
		struct xa_pdlg_info *pdlg = parm->pdlg;

		parm->curr_drv = pdlg->drivers;
		parm->curr_prn = pdlg->drivers->d->printers;
	}

	/*
	 * If device have no printer entries, use dname
	 */
	if (!parm->curr_drv->d->printers)
	{
		ret = parm->curr_drv->dname;
		parm->curr_drv = parm->curr_drv->next;
		parm->curr_prn = parm->curr_drv->d->printers;
	}
	/*
	 * else use the individual printer entry names
	 */
	else
	{
		printer = parm->curr_prn;
		ret = printer->name;
		if (!(printer = printer->next))
		{
			parm->curr_drv = parm->curr_drv->next;
			parm->curr_prn = parm->curr_drv->d->printers;
		}
		else
			parm->curr_prn = printer;
	}

	return ret;
}

static void
build_printer_popup(struct xa_pdlg_info *pdlg)
{
	OBJECT *tree;
	struct np_parms parms;
	void *p;

	p = &parms;

	parms.pdlg = pdlg;

	tree = create_popup_tree(pdlg->client, 0, pdlg->n_printers, pdlg->mwt->tree[XPDLG_DRIVER].ob_width, 4, &next_printer, &p);

	if (tree)
	{
		struct widget_tree *nwt = new_widget_tree(pdlg->client, tree);
		if (nwt)
		{
			pdlg->drv_pinf.tree = tree;
			pdlg->drv_pinf.obnum = pdlg->drv_obnum = 1;
			obj_set_g_popup(pdlg->mwt, aesobj(pdlg->mwt->tree, XPDLG_DRIVER), &pdlg->drv_pinf);
		}
		else
			free_object_tree(pdlg->client, tree);
	}
#if 0
	obj_draw(pdlg->mwt, pdlg->wind->vdi_settings, aesobj(pdlg->mwt->tree, XPDLG_DRIVER), -1, NULL, pdlg->wind->rect_list.start, 0);
#endif
}

static char errtxt[] = " -- ERROR -- ";
static char nosel[] = "  -  ";

struct nxtparm
{
	void *first;
	void *curr;
};

static void *
next_mode(short item, void **data)
{
	struct nxtparm *parm = *data;
	PRN_MODE *mode;
	char *ret;

	if (!parm->first)
		return nosel;

	if (!item)
		parm->curr = parm->first;

	mode = parm->curr;
	ret = mode->name;
	parm->curr = mode->next;
	return ret;
}

static void
build_mode_popup(struct xa_pdlg_info *pdlg, PRN_SETTINGS *pset)
{
	PRN_ENTRY *printer = pdlg->curr_prn;
	OBJECT *tree;
	short count, obnum = 1;
	struct nxtparm parm;
	void *p;

	p = &parm;

	if (printer)
	{
		PRN_MODE *mode = printer->modes;

		pdlg->curr_mode = mode;

		parm.first = mode;
		parm.curr = mode;

		count = 0;
		while (mode)
		{
			if (pset && mode->mode_id == pset->mode_id)
			{
				obnum = count + 1;
				pdlg->curr_mode = mode;
			}
			count++;
			mode = mode->next;
		}
	}
	else
	{
		pdlg->curr_mode = NULL;
		count = 1;
		parm.first = NULL;
	}

	if (pdlg->mode_wt)
	{
		pdlg->mode_wt->links--;
		remove_wt(pdlg->mode_wt, false);
		pdlg->mode_wt = NULL;
	}

	tree = create_popup_tree(pdlg->client, 0, count, pdlg->dwt->tree[PDLG_G_QUALITY].ob_width, 4, &next_mode, &p);

	if (tree)
	{
		pdlg->mode_wt = new_widget_tree(pdlg->client, tree);
		if (pdlg->mode_wt)
		{
			pdlg->mode_wt->links++;
			pdlg->mode_pinf.tree = tree;
			pdlg->mode_pinf.obnum = pdlg->mode_obnum = obnum;
			obj_set_g_popup(pdlg->dwt, aesobj(pdlg->dwt->tree, PDLG_G_QUALITY), &pdlg->mode_pinf);
		}
		disable_object(pdlg->dwt->tree + PDLG_G_QUALITY, (!pdlg->curr_mode));
	}
	if (!tree || !pdlg->mode_wt)
	{
		obj_unset_g_popup(pdlg->dwt, aesobj(pdlg->dwt->tree, PDLG_G_QUALITY), errtxt);

		if (tree)
			free_object_tree(pdlg->client, tree);
	}
	obj_draw(pdlg->mwt, pdlg->wind->vdi_settings, aesobj(pdlg->dwt->tree, PDLG_G_QUALITY), -1, NULL, pdlg->wind->rect_list.start, 0);
}
static char *colornames[] =
{
	"Black/White",		/*  0  - 0x0001 */
	"4 greytones",		/*  1  - 0x0002 */
	"8 greytones",		/*  2  - 0x0004 */
	"16 gerytones",		/*  3  - 0x0008 */
	"256 greytones",	/*  4  - 0x0010 */
	"32k greytones",	/*  5  - 0x0020 */
	"65K greytones",	/*  6  - 0x0040 */
	"16M greytones",	/*  7  - 0x0080 */

	"White/Black",		/*  8  - 0x0100 */
	"4 colors",		/*  9  - 0x0200 */
	"8 colors",		/* 10  - 0x0400 */
	"16 colors",		/* 11  - 0x0800 */
	"256 colors",		/* 12  - 0x1000 */
	"32K colors",		/* 13  - 0x2000 */
	"65K colors",		/* 14  - 0x4000 */
	"16M colors",		/* 15  - 0x8000 */

	"16 - Full color",
	"Unknown 17",
	"Unknown 18",
	"Unknown 19",
	"Unknown 20",
	"Unknown 21",
	"Unknown 22",
	"Unknown 23",

	"24 - What is this?",
	"Unknown 25",
	"Unknown 26",
	"Unknown 27",
	"Unknown 28",
	"Unknown 29",
	"Unknown 30",
	"Unknown 31",
	"",
  "",
};

static long colormodes[] =
{
	0x00000001,	/* Black/white */
	0x00000002,
	0x00000004,
	0x00000008,
	0x00000010,
	0x00000020,
	0x00000040,
	0x00000080,
	0x00000100,
	0x00000200,
	0x00000400,
	0x00000800,
	0x00001000,
	0x00002000,
	0x00004000,
	0x00008000,
	0x00010000,
	0x00020000,
	0x00040000,
	0x00080000,
	0x00100000,
	0x00200000,
	0x00400000,
	0x00800000,
	0x01000000,
	0x02000000,
	0x04000000,
	0x08000000,
	0x10000000,
	0x20000000,
	0x40000000,
	0x80000000
};


static void *
next_color(short item, void **data)
{
	struct nxtparm *parm = *data;
	long i;
	char **names, *ret;

	if (!item)
		parm->curr = 0;

	i = (long)parm->curr;
	names = (char **)parm->first;
	ret = names[i++];
	parm->curr = (void *)i;

	return ret;
}

static void
build_color_popup(struct xa_pdlg_info *pdlg, PRN_SETTINGS *pset)
{
	PRN_MODE *mode = pdlg->curr_mode;
	OBJECT *tree;
	short count = 0, obnum = 1;
	struct nxtparm parm;
	void *p;
	char *coln[32];

	if (mode)
	{
		short i = 0;
		unsigned long cols = (unsigned long)mode->color_capabilities;

		count = 0;
		for (i = 0; (i < 32) && cols; i++)
		{
			if (cols & 1)
			{
				coln[count] = colornames[i];
				pdlg->colormodes[count++] = colormodes[i];
				if (pset && pset->color_mode == colormodes[i])
					obnum = count;
			}
			cols >>= 1;
		}
	}

	pdlg->n_colmodes = count;

	if (!count)
		coln[count++] = nosel;

	coln[count] = NULL;

	parm.first = &coln;
	parm.curr = 0;
	p = &parm;

	if (pdlg->color_wt)
	{
		pdlg->color_wt->links--;
		remove_wt(pdlg->color_wt, false);
		pdlg->color_wt = NULL;
	}

	tree = create_popup_tree(pdlg->client, 0, count, pdlg->dwt->tree[PDLG_G_COLOR].ob_width, 4, &next_color, &p);

	if (tree)
	{
		pdlg->color_wt = new_widget_tree(pdlg->client, tree);
		if (pdlg->color_wt)
		{
			pdlg->color_wt->links++;
			pdlg->color_pinf.tree = tree;
			pdlg->color_pinf.obnum = pdlg->color_obnum = obnum;
			obj_set_g_popup(pdlg->dwt, aesobj(pdlg->dwt->tree, PDLG_G_COLOR), &pdlg->color_pinf);
		}
		disable_object(pdlg->dwt->tree + PDLG_G_COLOR, (coln[0] == nosel));
	}
	if (!tree || !pdlg->color_wt)
	{
		obj_unset_g_popup(pdlg->dwt, aesobj(pdlg->dwt->tree, PDLG_G_COLOR), errtxt);

		if (tree)
			free_object_tree(pdlg->client, tree);
	}
	obj_draw(pdlg->mwt, pdlg->wind->vdi_settings, aesobj(pdlg->dwt->tree, PDLG_G_COLOR), -1, NULL, pdlg->wind->rect_list.start, 0);
}
static void *
next_size(short item, void **data)
{
	struct nxtparm *parm = *data;
	MEDIA_SIZE *paper;
	char *ret;

	if (!parm->first)
		return nosel;

	if (!item)
		parm->curr = parm->first;

	paper = parm->curr;
	ret = paper->name;
	parm->curr = paper->next;
	return ret;
}

/*
 * Depends on curr_prn to be correct
 */
static void
build_size_popup(struct xa_pdlg_info *pdlg, PRN_SETTINGS *pset)
{
	PRN_ENTRY *printer = pdlg->curr_prn;
	short count, obnum = 1;
	OBJECT *tree;
	struct nxtparm parm;
	void *p;

	if (printer && printer->papers)
	{
		MEDIA_SIZE *paper = printer->papers;

		pdlg->curr_size = paper;

		parm.first = paper;
		parm.curr = paper;

		count = 0;
		while (paper)
		{
			if (pset && pset->size_id == paper->size_id)
			{
				pdlg->curr_size = paper;
				obnum = count + 1;
			}
			count++;
			paper = paper->next;
		}
	}
	else
	{
		pdlg->curr_size = NULL;
		count = 1;
		parm.first = NULL;
	}

	if (pdlg->size_wt)
	{
		pdlg->size_wt->links--;
		remove_wt(pdlg->size_wt, false);
		pdlg->size_wt = NULL;
	}

	p = &parm;
	tree = create_popup_tree(pdlg->client, 0, count, pdlg->dwt->tree[PDLG_P_FORMAT].ob_width, 4, &next_size, &p);

	if (tree)
	{
		pdlg->size_wt = new_widget_tree(pdlg->client, tree);
		if (pdlg->size_wt)
		{
			pdlg->size_wt->links++;
			pdlg->size_pinf.tree = tree;
			pdlg->size_pinf.obnum = pdlg->size_obnum = obnum;
			obj_set_g_popup(pdlg->dwt, aesobj(pdlg->dwt->tree, PDLG_P_FORMAT), &pdlg->size_pinf);
		}
		disable_object(pdlg->dwt->tree + PDLG_P_FORMAT, (!pdlg->curr_size));
	}
	if (!tree || !pdlg->size_wt)
	{
		obj_unset_g_popup(pdlg->dwt, aesobj(pdlg->dwt->tree, PDLG_P_FORMAT), errtxt);

		if (tree)
			free_object_tree(pdlg->client, tree);
	}
	obj_draw(pdlg->mwt, pdlg->wind->vdi_settings, aesobj(pdlg->dwt->tree, PDLG_P_FORMAT), -1, NULL, pdlg->wind->rect_list.start, 0);
}

static void *
next_type(short item, void **data)
{
	struct nxtparm *parm = *data;
	MEDIA_TYPE *type;
	char *ret;

	if (!parm->first)
		return nosel;

	if (!item)
		parm->curr = parm->first;

	type = parm->curr;
	ret = type->name;
	parm->curr = type->next;
	return ret;
}

/*
 * Depends on curr_mode to be correctly set
 */
static void
build_type_popup(struct xa_pdlg_info *pdlg, PRN_SETTINGS *pset)
{
	PRN_MODE *mode = pdlg->curr_mode;
	short count, obnum = 1;
	OBJECT *tree;
	struct nxtparm parm;
	void *p;

	if (mode && mode->paper_types)
	{
		MEDIA_TYPE *type = mode->paper_types;

		pdlg->curr_type = type;

		parm.first = type;
		parm.curr = type;

		count = 0;
		while (type)
		{
			if (pset && pset->type_id == type->type_id)
			{
				pdlg->curr_type = type;
				obnum = count + 1;
			}
			count++;
			type = type->next;
		}
	}
	else
	{
		pdlg->curr_type = NULL;
		count = 1;
		parm.first = NULL;
	}

	if (pdlg->type_wt)
	{
		pdlg->type_wt->links--;
		remove_wt(pdlg->type_wt, false);
		pdlg->type_wt = NULL;
	}

	p = &parm;
	tree = create_popup_tree(pdlg->client, 0, count, pdlg->dwt->tree[PDLG_P_TYPE].ob_width, 4, &next_type, &p);

	if (tree)
	{
		pdlg->type_wt = new_widget_tree(pdlg->client, tree);
		if (pdlg->type_wt)
		{
			pdlg->type_wt->links++;
			pdlg->type_pinf.tree = tree;
			pdlg->type_pinf.obnum = pdlg->type_obnum = obnum;
			obj_set_g_popup(pdlg->dwt, aesobj(pdlg->dwt->tree, PDLG_P_TYPE), &pdlg->type_pinf);
		}
		disable_object(pdlg->dwt->tree + PDLG_P_TYPE, (!pdlg->curr_type));
	}
	if (!tree || !pdlg->type_wt)
	{
		obj_unset_g_popup(pdlg->dwt, aesobj(pdlg->dwt->tree, PDLG_P_TYPE), errtxt);

		if (tree)
			free_object_tree(pdlg->client, tree);
	}
	obj_draw(pdlg->mwt, pdlg->wind->vdi_settings, aesobj(pdlg->dwt->tree, PDLG_P_TYPE), -1, NULL, pdlg->wind->rect_list.start, 0);
}

static char deftray[] = "std";

static void *
next_tray(short item, void **data)
{
	struct nxtparm *parm = *data;
	PRN_TRAY *tray;
	char *ret;

	if (!parm->first)
		return nosel;

	if (!item)
		parm->curr = parm->first;

	tray = parm->curr;
	ret = tray->name;
	if (ret[0] == '-')
		ret = deftray;
	parm->curr = tray->next;
	return ret;
}

/*
 * Depends on curr_mode to be correctly set
 */
static void
build_tray_popup(struct xa_pdlg_info *pdlg, short which, PRN_SETTINGS *pset)
{
	PRN_ENTRY *printer = pdlg->curr_prn;
	short count, obnum = 1, obidx = (!which) ? PDLG_P_INTRAY : PDLG_P_OUTTRAY;
	short *obn = (!which) ? &pdlg->itray_obnum : &pdlg->otray_obnum;
	struct widget_tree **pwt = ((!which) ? &pdlg->itray_wt : &pdlg->otray_wt);
	OBJECT *tree;
	POPINFO *pi = ((!which) ? &pdlg->itray_pinf : &pdlg->otray_pinf);
	PRN_TRAY **tptr = (!which) ? &pdlg->curr_itray : &pdlg->curr_otray;
	struct nxtparm parm;
	void *p;

	if (printer && (which ? printer->input_trays : printer->output_trays))
	{
		PRN_TRAY *tray = ((!which) ? printer->input_trays : printer->output_trays);

		*tptr = tray;

		parm.first = tray;
		parm.curr = tray;

		count = 0;
		while (tray)
		{
			if (pset)
			{
				if ((!which && pset->input_id  == tray->tray_id) ||
				    ( which && pset->output_id == tray->tray_id))
				{
					*tptr = tray;
					obnum = count + 1;
				}
			}
			count++;
			tray = tray->next;
		}
	}
	else
	{
		*tptr = NULL;
		count = 1;
		parm.first = NULL;
	}

	if (*pwt)
	{
		(*pwt)->links--;
		remove_wt(*pwt, false);
		*pwt = NULL;
	}

	p = &parm;
	tree = create_popup_tree(pdlg->client, 0, count, pdlg->dwt->tree[obidx].ob_width, 4, &next_tray, &p);

	if (tree)
	{
		*pwt = new_widget_tree(pdlg->client, tree);
		if (*pwt)
		{
			(*pwt)->links++;
			pi->tree = tree;
			pi->obnum = *obn = obnum;
			obj_set_g_popup(pdlg->dwt, aesobj(pdlg->dwt->tree, obidx), pi);
		}
		disable_object(pdlg->dwt->tree + obidx, (!*tptr));
	}
	if (!tree || !*pwt)
	{
		obj_unset_g_popup(pdlg->dwt, aesobj(pdlg->dwt->tree, obidx), errtxt);

		if (tree)
			free_object_tree(pdlg->client, tree);
	}
	obj_draw(pdlg->mwt, pdlg->wind->vdi_settings, aesobj(pdlg->dwt->tree, obidx), -1, NULL, pdlg->wind->rect_list.start, 0);
}

static void *
next_dither(short item, void **data)
{
	struct nxtparm *parm = *data;
	DITHER_MODE *dither;
	char *ret;

	if (!parm->first)
		return nosel;

	if (!item)
		parm->curr = parm->first;

	dither = parm->curr;
	ret = dither->name;
	parm->curr = dither->next;
	return ret;
}

/*
 * Depends on curr_drv to be correct
 */
static void
build_dither_popup(struct xa_pdlg_info *pdlg, PRN_SETTINGS *pset)
{
	DRV_INFO *drv = pdlg->curr_drv;
	short count, obnum = 1;
	OBJECT *tree;
	struct nxtparm parm;
	void *p;

	if (drv && drv->dither_modes)
	{
		DITHER_MODE *dither = drv->dither_modes;

		pdlg->curr_dither = dither;

		parm.first = dither;
		parm.curr = dither;

		count = 0;
		while (dither)
		{
			if (pset && pset->dither_mode == dither->dither_id)
			{
				pdlg->curr_dither = dither;
				obnum = count + 1;
			}
			count++;
			dither = dither->next;
		}
	}
	else
	{
		pdlg->curr_dither = NULL;
		count = 1;
		parm.first = NULL;
	}

	if (pdlg->dither_wt)
	{
		pdlg->dither_wt->links--;
		remove_wt(pdlg->dither_wt, false);
		pdlg->dither_wt = NULL;
	}

	p = &parm;
	tree = create_popup_tree(pdlg->client, 0, count, pdlg->dwt->tree[PDLG_R_RASTER].ob_width, 4, &next_dither, &p);

	if (tree)
	{
		pdlg->dither_wt = new_widget_tree(pdlg->client, tree);
		if (pdlg->dither_wt)
		{
			pdlg->dither_wt->links++;
			pdlg->dither_pinf.tree = tree;
			pdlg->dither_pinf.obnum = pdlg->dither_obnum = obnum;
			obj_set_g_popup(pdlg->dwt, aesobj(pdlg->dwt->tree, PDLG_R_RASTER), &pdlg->dither_pinf);
		}
		disable_object(pdlg->dwt->tree + PDLG_R_RASTER, (!pdlg->curr_dither));
	}
	if (!tree || !pdlg->dither_wt)
	{
		obj_unset_g_popup(pdlg->dwt, aesobj(pdlg->dwt->tree, PDLG_R_RASTER), errtxt);

		if (tree)
			free_object_tree(pdlg->client, tree);
	}
	obj_draw(pdlg->mwt, pdlg->wind->vdi_settings, aesobj(pdlg->dwt->tree, PDLG_R_RASTER), -1, NULL, pdlg->wind->rect_list.start, 0);
}

static void *
next_outfile(short item, void **data)
{
	struct nxtparm *parm = *data;
	char **outfile;
	char *ret;

	if (!parm->first)
		return nosel;

	if (!item)
		parm->curr = parm->first;

	outfile = parm->curr;

	if (!strcmp(*outfile, "FILE:"))
		ret = selafile;
	else
	{
		if (outfile[1])
			ret = outfile[1];
		else
			ret = *outfile;
	}

	outfile += 2;

	parm->curr = outfile;
	return ret;
}

/*
 * Depends on curr_drv to be correct
 */
static void
build_outfile_popup(struct xa_pdlg_info *pdlg, DRV_INFO *drv, PRN_ENTRY *prn, PRN_SETTINGS *pset)
{
	char **files, **ourfiles;
	short count, obnum = -1, sel_obnum = -1;
	OBJECT *tree;
	long outcapabilities;
	struct nxtparm parm;
	void *p;

	files = outfiles;
	ourfiles = pdlg->outfiles;
	count = 0;
	outcapabilities = prn ? prn->printer_capabilities : PC_FILE;
	outcapabilities &= (PC_FILE|PC_SERIAL|PC_PARALLEL|PC_ACSI|PC_SCSI);

	if (!outcapabilities)
	{
		*ourfiles++ = nosel;
		*ourfiles++ = NULL;
		count = 1;
	}
	else
	{
		while (*files)
		{
			if (outcapabilities & 1)
			{
				if (!strcmp(*files, "FILE:"))
					sel_obnum = count + 1;

				if (pset && !strcmp(*files, pset->device))
					obnum = count + 1;

				if (obnum == -1 && drv && !strcmp(*files, drv->device))
					obnum = count + 1;

				*ourfiles++ = files[0];
				*ourfiles++ = files[1];
				count++;
			}
			outcapabilities >>= 1;
			files += 2;
		}
	}

	ourfiles[0] = NULL;
	ourfiles[1] = NULL;

	if (obnum == -1)
		obnum = 1;

	if (pdlg->outfile_wt)
	{
		pdlg->outfile_wt->links--;
		remove_wt(pdlg->outfile_wt, false);
		pdlg->outfile_wt = NULL;
	}

	parm.first = &pdlg->outfiles;
	p = &parm;
	tree = create_popup_tree(pdlg->client, 0, count, pdlg->dwt->tree[PDLG_I_PORT].ob_width, 4, &next_outfile, &p);

	if (tree)
	{
		pdlg->outfile_wt = new_widget_tree(pdlg->client, tree);
		if (pdlg->outfile_wt)
		{
			pdlg->outfile_wt->links++;
			pdlg->outfile_pinf.tree = tree;
			pdlg->outfile_pinf.obnum = pdlg->outfile_obnum = obnum;
			pdlg->sel_outfile_obnum = sel_obnum;
			obj_set_g_popup(pdlg->dwt, aesobj(pdlg->dwt->tree, PDLG_I_PORT), &pdlg->outfile_pinf);
		}
		disable_object(pdlg->dwt->tree + PDLG_I_PORT, (pdlg->outfiles[0] == nosel));
	}
	if (!tree || !pdlg->outfile_wt)
	{
		obj_unset_g_popup(pdlg->dwt, aesobj(pdlg->dwt->tree, PDLG_I_PORT), errtxt);

		if (tree)
			free_object_tree(pdlg->client, tree);
	}
	obj_draw(pdlg->mwt, pdlg->wind->vdi_settings, aesobj(pdlg->dwt->tree, PDLG_I_PORT), -1, NULL, pdlg->wind->rect_list.start, 0);
}

static void
change_mode(struct xa_pdlg_info *pdlg, short idx)
{
	PRN_MODE *mode = NULL;

	pdlg->mode_obnum = idx + 1;

	if (pdlg->curr_prn)
	{
		if ((mode = pdlg->curr_prn->modes))
		{
			short i;

			for (i = 0; i < idx; i++)
			{
				if (!(mode = mode->next))
					break;
			}
			if (!mode)
				mode = pdlg->curr_prn->modes;
		}
	}

	if (mode != pdlg->curr_mode)
	{
		pdlg->curr_mode = mode;
		set_mode_pset(mode, &pdlg->current_settings);
		build_type_popup(pdlg, &pdlg->current_settings);
		set_type_pset(pdlg->curr_type, &pdlg->current_settings);
		build_color_popup(pdlg, &pdlg->current_settings);
		set_color_mode_pset(pdlg->colormodes[pdlg->color_obnum - 1], &pdlg->current_settings);
	}
}

static void
change_color(struct xa_pdlg_info *pdlg, short idx)
{
	pdlg->color_obnum = idx + 1;
	set_color_mode_pset(pdlg->colormodes[idx], &pdlg->current_settings);
}

static void
change_size(struct xa_pdlg_info *pdlg, short idx)
{
	MEDIA_SIZE *size = NULL;

	pdlg->size_obnum = idx + 1;

	if (pdlg->curr_prn)
	{
		if ((size = pdlg->curr_prn->papers))
		{
			short i;

			for (i = 0; i < idx; i++)
			{
				if (!(size = size->next))
					break;
			}

			if (!size)
				size = pdlg->curr_prn->papers;
		}
	}

	if (size != pdlg->curr_size)
	{
		pdlg->curr_size = size;
		set_size_pset(size, &pdlg->current_settings);
	}
}
static void
change_type(struct xa_pdlg_info *pdlg, short idx)
{
	MEDIA_TYPE *type = NULL;

	pdlg->type_obnum = idx + 1;

	if (pdlg->curr_mode)
	{
		if ((type = pdlg->curr_mode->paper_types))
		{
			short i;

			for (i = 0; i < idx; i++)
			{
				if (!(type = type->next))
					break;
			}
			if (!type)
				type = pdlg->curr_mode->paper_types;
		}
	}
	if (type != pdlg->curr_type)
	{
		pdlg->curr_type = type;
		set_type_pset(type, &pdlg->current_settings);
	}
}

static void
change_tray(struct xa_pdlg_info *pdlg, short idx, short which)
{
	PRN_TRAY *tray = NULL;
	if (!which)
	{
		pdlg->itray_obnum = idx + 1;

		if (pdlg->curr_prn)
		{
			if ((tray = pdlg->curr_prn->input_trays))
			{
				short i;

				for (i = 0; i < idx; i++)
				{
					if (!(tray = tray->next))
						break;
				}
				if (!tray)
					tray = pdlg->curr_prn->input_trays;
			}
		}
		if (tray != pdlg->curr_itray)
		{
			pdlg->curr_itray = tray;
			set_trays_pset(tray, pdlg->curr_otray, &pdlg->current_settings);
		}
	}
	else
	{
		pdlg->otray_obnum = idx + 1;

		if (pdlg->curr_prn)
		{
			if ((tray = pdlg->curr_prn->output_trays))
			{
				short i;

				for (i = 0; i < idx; i++)
				{
					if (!(tray = tray->next))
						break;
				}
				if (!tray)
					tray = pdlg->curr_prn->output_trays;
			}
		}
		if (tray != pdlg->curr_otray)
		{
			pdlg->curr_otray = tray;
			set_trays_pset(pdlg->curr_itray, tray, &pdlg->current_settings);
		}
	}
}
static void
change_dither(struct xa_pdlg_info *pdlg, short idx)
{
	DITHER_MODE *dither = NULL;

	pdlg->dither_obnum = idx + 1;

	if (pdlg->curr_drv)
	{
		if ((dither = pdlg->curr_drv->dither_modes))
		{
			short i;

			for (i = 0; i < idx; i++)
			{
				if (!(dither = dither->next))
					break;
			}
			if (!dither)
				dither = pdlg->curr_drv->dither_modes;
		}
	}
	if (dither != pdlg->curr_dither)
	{
		pdlg->curr_dither = dither;
		set_dither_pset(dither, &pdlg->current_settings);
	}
}

static void
handle_fs(int lock, struct fsel_data *fs, const char *path, const char *file)
{
	struct xa_pdlg_info *pdlg = fs->data;

	strcpy(pdlg->filesel, file);

	close_fileselector(lock, fs);
	pdlg->client->usr_evnt = 2;
}
static void
cancel_fs(int lock, struct fsel_data *fs, const char *p, const char *f)
{
	struct xa_pdlg_info *pdlg = fs->data;
	close_fileselector(lock, fs);
	pdlg->client->usr_evnt = 2;
}

static void
change_outfile(struct xa_pdlg_info *pdlg, short idx)
{
	char **files = pdlg->outfiles + (idx << 1);
	short index = idx << 1;

	pdlg->outfile_obnum = idx + 1;

	if (!strcmp(files[index], "FILE:"))
	{
		struct fsel_data *fs;

		fs = kmalloc(sizeof(*fs));
		if (fs)
		{
			char file[512];
			strcpy(file, pdlg->filesel);
			pdlg->filesel[0] = '\0';
			open_fileselector(0, pdlg->client, fs,
					  pdlg->client->home_path,
					  file, "Select output file",
					  handle_fs, cancel_fs, pdlg);
			pdlg->client->status |= CS_FSEL_INPUT;
			(*pdlg->client->block)(pdlg->client);
			pdlg->client->status &= ~CS_FSEL_INPUT;

			if (pdlg->filesel[0])
			{
				files[index + 1] = pdlg->filesel;
			}
			else
			{
				if (file[0])
				{
					strcpy(pdlg->filesel, file);
					files[index + 1] = pdlg->filesel;
				}
				else
					files[index + 1] = selafile;
			}

			kfree(fs);
		}
	}
	if (*files)
	{
		d_set_outfile(pdlg, *files);

		if (!strcmp(*files, "FILE:"))
		{
			pdlg->outfile = files[1];
			set_outfile_pset(files[1], &pdlg->current_settings);
		}
		else
		{
			pdlg->outfile = *files;
			set_outfile_pset(*files, &pdlg->current_settings);
		}
	}
}

static void
getfirst_printer(struct xa_pdlg_info *pdlg, struct xa_prndrv_info **p, DRV_INFO **drv, PRN_ENTRY **prn)
{

	if ((*p = pdlg->drivers))
	{
		*drv = (*p)->d;
		*prn = (*drv)->printers;
	}
	else
	{
		*drv = NULL;
		*prn = NULL;
	}
}

static void
getnext_printer(struct xa_prndrv_info **p, DRV_INFO **drv, PRN_ENTRY **prn)
{
	if (!(*drv)->printers || !(*prn = (*prn)->next))
	{
		if ((*p = (*p)->next))
		{
			*drv = (*p)->d;
			*prn = (*drv)->printers;
		}
		else
		{
			*drv = NULL;
			*prn = NULL;
		}
	}
}

static bool
find_printer(struct xa_pdlg_info *pdlg, long driver_id, long printer_id, short *idx, DRV_INFO **ret_drv, PRN_ENTRY **ret_prn)
{
	short i = 0;
	struct xa_prndrv_info *p;
	DRV_INFO *drv;
	PRN_ENTRY *prn;

	getfirst_printer(pdlg, &p, &drv, &prn);

	while (drv)
	{
		if (prn)
		{
			if (drv->driver_id == driver_id && prn->printer_id == printer_id)
				break;
		}
		else if (printer_id == 0L)
		{
			if (drv->driver_id == driver_id)
				break;
		}
		getnext_printer(&p, &drv, &prn);
		i++;
	}

	if (drv)
	{
		if (idx) *idx = i;
		if (ret_drv) *ret_drv = drv;
		if (ret_prn) *ret_prn = prn;
		return true;
	}
	else
	{
		getfirst_printer(pdlg, &p, &drv, &prn);
		if (idx) *idx = 0;
		if (ret_drv) *ret_drv = drv;
		if (ret_prn) *ret_prn = prn;
		return false;
	}
}
/*
 * Select a printer based on the 'idx'
 *
 */
static void
change_printer(struct xa_pdlg_info *pdlg, short idx, PRN_SETTINGS *pset)
{
	short i;
	long prn_cap, mode_cap;
	struct xa_prndrv_info *p = pdlg->drivers;
	DRV_INFO *curr_drv;

	pdlg->drv_obnum = idx + 1;

	if (p)
	{
		PRN_ENTRY *curr_prn;

		curr_drv = p->d;
		curr_prn = curr_drv->printers;

		for (i = 0; i < idx; i++)
		{
			if (!curr_drv->printers || !(curr_prn = curr_prn->next))
			{
				p = p->next;
				curr_drv = p->d;
				curr_prn = curr_drv->printers;
			}
		}
		/* Ozk:
		 * curr_drv does not necessarily have a printer entry,
		 * in which case curr_prn is NULL. In that case I think
		 * that nothing is adjustable, and we will disable all
		 * possible settings in the dialog.
		 */
		pdlg->curr_drv = curr_drv;
		pdlg->curr_prn = curr_prn;

		set_driver_pset(pdlg->curr_drv, &pdlg->current_settings);
		set_printer_pset(pdlg->curr_prn, &pdlg->current_settings);

		build_mode_popup(pdlg, pset);
		set_mode_pset(pdlg->curr_mode, &pdlg->current_settings);

		build_color_popup(pdlg, pset);
		set_color_mode_pset(pdlg->colormodes[pdlg->color_obnum - 1], &pdlg->current_settings);

		build_size_popup(pdlg, pset);
		set_size_pset(pdlg->curr_size, &pdlg->current_settings);

		build_type_popup(pdlg, pset);
		set_type_pset(pdlg->curr_type, &pdlg->current_settings);

		build_tray_popup(pdlg, 0, pset);
		build_tray_popup(pdlg, 1, pset);
		set_trays_pset(pdlg->curr_itray, pdlg->curr_otray, &pdlg->current_settings);

		build_dither_popup(pdlg, pset);
		set_dither_pset(pdlg->curr_dither, &pdlg->current_settings);


		build_outfile_popup(pdlg, curr_drv, curr_prn, pset);
		set_outfile_pset(curr_drv->device, &pdlg->current_settings);
		d_set_outfile(pdlg, curr_drv->device);

		prn_cap = 0L;
		mode_cap = 0L;
		if (curr_prn)
			prn_cap = curr_prn->printer_capabilities;

		if (disable_object(pdlg->dwt->tree + PDLG_I_BG, !(prn_cap & PC_BACKGROUND)))
			obj_draw(pdlg->mwt, pdlg->wind->vdi_settings, aesobj(pdlg->dwt->tree, PDLG_I_BG), -1, NULL, pdlg->wind->rect_list.start, 0);

		if (disable_object(pdlg->dwt->tree + PDLG_R_FG, !(prn_cap & PC_BACKGROUND)))
			obj_draw(pdlg->mwt, pdlg->wind->vdi_settings, aesobj(pdlg->dwt->tree, PDLG_R_FG), -1, NULL, pdlg->wind->rect_list.start, 0);

		if (disable_object(pdlg->dwt->tree + PDLG_P_SCALE, !(prn_cap & PC_SCALING)))
			obj_draw(pdlg->mwt, pdlg->wind->vdi_settings, aesobj(pdlg->dwt->tree, PDLG_P_SCALE), -1, NULL, pdlg->wind->rect_list.start, 0);

		if (disable_object(pdlg->dwt->tree + PDLG_G_COPIES, !(prn_cap & PC_COPIES)))
			obj_draw(pdlg->mwt, pdlg->wind->vdi_settings, aesobj(pdlg->dwt->tree, PDLG_G_COPIES), -1, NULL, pdlg->wind->rect_list.start, 0);

		if (pdlg->curr_mode)
			mode_cap = pdlg->curr_mode->mode_capabilities;

		for (i = 0; i < 4; i++) {
			if (disable_object(pdlg->dwt->tree + PDLG_R_C + i, !(mode_cap & MC_SLCT_CMYK)))
				obj_draw(pdlg->mwt, pdlg->wind->vdi_settings, aesobj(pdlg->dwt->tree, PDLG_R_C + i), -1, NULL, pdlg->wind->rect_list.start, 0);
		}
	}
}

static int
check_internal_objects(struct xa_pdlg_info *pdlg, struct xa_aes_object obj)
{
	int ret = 0;
	OBJECT *tree;

	if (aesobj_tree(&obj) == pdlg->mwt->tree)
	{
		if (is_aesobj(pdlg->mwt->tree, XPDLG_DRIVER, &obj))
		{
			if (pdlg->drv_obnum != pdlg->drv_pinf.obnum)
				change_printer(pdlg, pdlg->drv_pinf.obnum - 1, &pdlg->current_settings);
			ret = 1;
		}
	}
	else
	{
		tree = pdlg->dwt->tree;

		if (aesobj_tree(&obj) == tree)
		{
			switch (aesobj_item(&obj))
			{
				case PDLG_G_QUALITY:
				{
					if (pdlg->mode_obnum != pdlg->mode_pinf.obnum)
						change_mode(pdlg, pdlg->mode_pinf.obnum - 1);
					ret = 1;
					break;
				}
				case PDLG_G_COLOR:
				{
					if (pdlg->color_obnum != pdlg->color_pinf.obnum)
						change_color(pdlg, pdlg->color_pinf.obnum - 1);
					ret = 1;
					break;
				}
				case PDLG_P_FORMAT:
				{
					if (pdlg->size_obnum != pdlg->size_pinf.obnum)
						change_size(pdlg, pdlg->size_pinf.obnum - 1);
					ret = 1;
					break;
				}
				case PDLG_P_TYPE:
				{
					if (pdlg->type_obnum != pdlg->type_pinf.obnum)
						change_type(pdlg, pdlg->type_pinf.obnum - 1);
					ret = 1;
					break;
				}
				case PDLG_P_INTRAY:
				{
					if (pdlg->itray_obnum != pdlg->itray_pinf.obnum)
						change_tray(pdlg, pdlg->itray_pinf.obnum - 1, 0);
					ret = 1;
					break;
				}
				case PDLG_P_OUTTRAY:
				{
					if (pdlg->otray_obnum != pdlg->otray_pinf.obnum)
						change_tray(pdlg, pdlg->otray_pinf.obnum - 1, 1);
					ret = 1;
					break;
				}
				case PDLG_R_RASTER:
				{
					if (pdlg->dither_obnum != pdlg->dither_pinf.obnum)
						change_dither(pdlg, pdlg->dither_pinf.obnum - 1);
					ret = 1;
					break;
				}
				case PDLG_I_PORT:
				{
					if (pdlg->outfile_obnum != pdlg->outfile_pinf.obnum || pdlg->outfile_pinf.obnum == pdlg->sel_outfile_obnum)
						change_outfile(pdlg, pdlg->outfile_pinf.obnum - 1);
					ret = 1;
					break;
				}
			}
		}
		else if (aesobj_tree(&obj) == pdlg->current_subdlg->tree)
		{
			callout_pdlg_sub(pdlg, CALL_DO_DLG, pdlg->current_subdlg, pdlg->settings, aesobj_item(&obj));
			ret = 1;
		}
	}

	/* return 1 if internal button, 0 otherwise */
	return ret;
}

static void
delete_pdlgsub(void *_sub)
{
	kfree(_sub);
}

static void
add_dialog(struct xa_pdlg_info *pdlg, PDLG_SUB *new, char *name, OBJECT *to_tree, short to_object)
{
	PDLG_SUB *sub;
	SCROLL_INFO *list = pdlg->list;
	struct scroll_content sc = {{ 0 }};

	if ((sub = new))
	{
		sub->sub_id = pdlg->nxt_subdlgid;
		pdlg->nxt_subdlgid++;
		sub->tree = sub->sub_tree;
		sub->index_offset = 0;

		sc.t.strings = 1;
		sc.t.text = name;
		sc.data = sub;
		sc.usr_flags = 1L;

		list->add(list, NULL, NULL, &sc, 0,0, NOREDRAW);
	}
	else
	{
		sub = kmalloc(sizeof(*sub));
		assert(sub);
		bzero(sub, sizeof(*sub));
		sub->sub_tree = sub->tree = to_tree;
		sub->index_offset = to_object;
		sc.t.strings = 1;
		sc.t.text = name;
		sc.data = sub;
		sc.usr_flags = 0L;
		sc.data_destruct = delete_pdlgsub;

		list->add(list, NULL, NULL, &sc, 0, 0, NOREDRAW);
	}
}

static struct scroll_entry *
get_entry_byidx(struct scroll_info *list, int idx)
{
	struct sesetget_params p;

	p.idx = idx;
	p.level.flags = 0;
	p.level.maxlevel = -1;
	p.level.curlevel = 0;

	list->get(list, NULL, SEGET_ENTRYBYIDX, &p);

	return p.e;
}

static struct scroll_entry *
get_app_added(struct scroll_info *list)
{
	struct sesetget_params p;

	p.idx = 0;
	p.arg.usr_flags.method = ANYBITS;
	p.arg.usr_flags.bits = 1;
	p.arg.usr_flags.mask = 1;
	p.level.flags = 0;
	p.level.maxlevel = -1;
	p.level.curlevel = 0;

	list->get(list, NULL, SEGET_ENTRYBYUSRFLAGS, &p);
	return p.e;
}

static void
remove_app_dialogs(struct xa_pdlg_info *pdlg)
{
	SCROLL_INFO *list = pdlg->list;
	SCROLL_ENTRY *this;

	while ((this = get_app_added(list)))
	{
		list->del(list, this, NORMREDRAW);
	}
}


bool pdlg_available(void)
{
	/*
	 * FIXME: should try v_read_seetings(), but for that
	 * we need a printer handle first
	 */
	if (C.nvdi_version < 0x0410)
	{
		DIAGS((" -- VDI not capable of PRN_SETTINGS!"));
		return false;
	}
	return true;
}


static struct xa_pdlg_info *
create_new_pdlg(struct xa_client *client, XA_WIND_ATTR tp)
{
	struct xa_window *wind = NULL;
	struct xa_pdlg_info *pdlg;

	DIAG((D_pdlg, client, "create_new_pdlg"));

	if (!pdlg_available())
	{
		return (void *)-1L;
	}

	pdlg = kmalloc(sizeof(*pdlg));

	if (pdlg)
	{
		OBJECT *mtree = NULL, *dtree = NULL;
		struct widget_tree *mwt = NULL, *dwt = NULL;
		struct scroll_info *list = NULL;

		bzero(pdlg, sizeof(*pdlg));

		mtree = duplicate_obtree(C.Aes, ResourceTree(C.Aes_rsc, WDLG_PDLG), 0);

		if (mtree)
		{
			if ((mwt = new_widget_tree(client, mtree)))
			{
				mwt->flags |= WTF_AUTOFREE | WTF_TREE_ALLOC;
				obj_init_focus(mwt, OB_IF_RESET);
				mwt->links++;
			}
			else goto fail;
		}

		dtree = duplicate_obtree(C.Aes, ResourceTree(C.Aes_rsc, PDLG_DIALOGS), 0);
		if (dtree)
		{
			if ((dwt = new_widget_tree(client, dtree)))
			{
				dwt->flags |= WTF_AUTOFREE | WTF_TREE_ALLOC;
				dwt->links++;
			}
			else goto fail;
		}
		{
			GRECT r, or;

			obj_rectangle(mwt, aesobj(mwt->tree, 0), &or);

			r = calc_window(0, client, WC_BORDER,
					tp, created_for_WDIAL,
					client->options.thinframe,
					client->options.thinwork,
					&or);

			if (!(wind = create_window(0,
					     send_app_message,
					     NULL,
					     client,
					     false,
					     tp,
					     created_for_WDIAL,
					     client->options.thinframe,
					     client->options.thinwork,
					     &r, NULL, NULL)))
				goto fail;

			/* set a default-name */
			sprintf( wind->wname, sizeof( wind->wname), "%s- Print-Dialog", client->name+1 );
			set_toolbar_widget(0, wind, client, mwt->tree, aesobj(mwt->tree, -2), 0, STW_ZEN, &wdlg_th, &or);
		}
		if (mwt && dwt)
		{
			int len;
			struct sc2 scp;
			pdlg->handle = (void *)((unsigned long)pdlg >> 16 | (unsigned long)pdlg << 16);
			pdlg->client = client;
			pdlg->wind = wind;
			pdlg->min_w = mwt->tree[XPDLG_DIALOG].ob_width;
			pdlg->min_h = mwt->tree[XPDLG_DIALOG].ob_height;
			pdlg->dwt = dwt;
			pdlg->mwt = mwt;

			list = set_slist_object( 0,
						 mwt,
						 wind,
						 XPDLG_LIST,
						 SIF_SELECTABLE|SIF_AUTOSELECT|SIF_KEYBDACT,
						 NULL, NULL,			/* scrl_widget closer, fuller*/
						 NULL, click_list, NULL, NULL,	/* scrl_click dclick, click, click_nesticon, keybd */
						 NULL, NULL, NULL, NULL,	/* add, del, empty, destroy */
						 NULL, NULL, pdlg, 1);		/* title, info, data, lmax */
			if (!list)
				goto fail;

			pdlg->list = list;

			add_dialog(pdlg, NULL, "General", pdlg->dwt->tree, PDLG_GENERAL); /* TODO: move strings to resource file */
			add_dialog(pdlg, NULL, "Paper", pdlg->dwt->tree, PDLG_PAPER);
			add_dialog(pdlg, NULL, "Raster", pdlg->dwt->tree, PDLG_RASTER);
			add_dialog(pdlg, NULL, "Interface", pdlg->dwt->tree, PDLG_PORT);

			scp.objs = ob_count_objs(pdlg->mwt->tree, 0, -1);
			scp.objs += ob_count_objs(pdlg->dwt->tree, 0, -1);

			len = scp.objs * sizeof(struct sc);
			if( !(scp.sc = kmalloc(len) ) )
			{
				BLOG((0,"create_new_pdlg:kmalloc(%d) failed.", len));
				return 0;
			}

			ob_fix_shortcuts( pdlg->mwt->tree, false, &scp);
			ob_fix_shortcuts( pdlg->dwt->tree, false, &scp);
			kfree( scp.sc );

			set_oblink(pdlg, pdlg->dwt->tree, PDLG_GENERAL);

			get_driver_list(pdlg);

			add_xa_data(&client->xa_data, pdlg, 0, NULL, delete_pdlg_info);
		}
		else
		{
fail:
			if (wind)
				delete_window(0, wind);

			if (mwt)
			{
				mwt->links--;
				remove_wt(mwt, false);
			}
			else if (mtree)
				free_object_tree(client, mtree);

			if (dwt)
			{
				dwt->links--;
				remove_wt(dwt, false);
			}
			else if (dtree)
				free_object_tree(client, dtree);

			if (pdlg)
			{
				kfree(pdlg);
				pdlg = NULL;
			}
		}
	}
	return pdlg;
}
unsigned long
XA_pdlg_create(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_pdlg_info *pdlg = NULL;

	DIAG((D_pdlg, client, "XA_pdlg_create"));
	pb->addrout[0] = 0L;
	pdlg = create_new_pdlg(client, MOVER|NAME);
	if (!pdlg || pdlg == (void *)-1L)
		return XAC_DONE;

	pb->addrout[0] = (long)pdlg->handle;

	return XAC_DONE;
}

unsigned long
XA_pdlg_delete(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_pdlg_info *pdlg;
	struct xa_window *wind;

	DIAG((D_pdlg, client, "XA_pdlg_delete"));


	pb->intout[0] = 0;

	pdlg = (struct xa_pdlg_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (pdlg && (wind = get_pdlg_wind(client, pdlg)))
	{
		if (wind->window_status & XAWS_OPEN)
			close_window(lock, wind);

		delayed_delete_window(lock, wind);
		delete_xa_data(&client->xa_data, pdlg);

		pb->intout[0] = 1;
	}
	return XAC_DONE;
}

static void
get_document_name(struct xa_pdlg_info *pdlg, const char *name)
{
	strncpy(pdlg->document_name, name, 255);
}

static void
apply_option_flags(struct xa_pdlg_info *pdlg, short options)
{
	SCROLL_ENTRY *this;

	OBJECT *tree = pdlg->dwt->tree;

	disable_object(tree + PDLG_G_COPIES, (options & PDLG_ALWAYS_COPIES) ? false : true);

	disable_object(tree + PDLG_P_LETTER, (options & PDLG_ALWAYS_ORIENT) ? false : true);
	disable_object(tree + PDLG_G_LAND, (options & PDLG_ALWAYS_ORIENT) ? false : true);

	disable_object(tree + PDLG_P_SCALE, (options & PDLG_ALWAYS_SCALE) ? false : true);

	disable_object(tree + PDLG_G_EVEN, (options & PDLG_EVENODD) ? false : true);
	disable_object(tree + PDLG_G_ODD , (options & PDLG_EVENODD) ? false : true);

	this = get_entry_byidx(pdlg->list, (options & PDLG_PRINT) ? 1 : 2);
	if (this)
	{
		pdlg->list->cur = NULL;
		pdlg->list->set(pdlg->list, this, SESET_ACTIVE, 0, NOREDRAW);
	}
}

static short
find_mode(PRN_ENTRY *prn, long mode_id, short *idx, PRN_MODE **ret_mode)
{
	short ret, index = 0;
	PRN_MODE *mode = prn->modes;

	while (mode)
	{
		if (mode->mode_id == mode_id)
			break;
		index++;
		mode = mode->next;
	}
	if (!mode)
	{
		mode = prn->modes;
		index = 0;
		ret = 0;
	}
	else
		ret = 1;

	if (idx) *idx = index;
	if (ret_mode) *ret_mode = mode;
	return ret;
}

static short
find_size(PRN_ENTRY *prn, long size_id, short *idx, MEDIA_SIZE **ret_size)
{
	short ret, index = 0;
	MEDIA_SIZE *size = prn->papers;

	while (size)
	{
		if (size->size_id == size_id)
			break;
		index++;
		size = size->next;
	}
	if (!size)
	{
		size = prn->papers;
		index = 0;
		ret = 0;
	}
	else
		ret = 1;

	if (idx) *idx = index;
	if (ret_size) *ret_size = size;
	return ret;
}

static short
find_tray(PRN_ENTRY *prn, short which, long tray_id, short *idx, PRN_TRAY **ret_tray)
{
	short ret, index = 0;
	PRN_TRAY *tray = (!which) ? prn->input_trays : prn->output_trays;

	while (tray)
	{
		if (tray->tray_id == tray_id)
			break;
		index++;
		tray = tray->next;
	}
	if (!tray)
	{
		tray = (!which) ? prn->input_trays : prn->output_trays;
		index = 0;
		ret = 0;
	}
	else
		ret = 1;

	if (idx) *idx = index;
	if (ret_tray) *ret_tray = tray;
	return ret;
}

static short
find_type(PRN_MODE *mode, long type_id, short *idx, MEDIA_TYPE **ret_type)
{
	short ret, index = 0;
	MEDIA_TYPE *type = mode->paper_types;

	while (type)
	{
		if (type->type_id == type_id)
			break;
		index++;
		type = type->next;
	}
	if (!type)
	{
		type = mode->paper_types;
		index = 0;
		ret = 0;
	}
	else
		ret = 1;

	if (idx) *idx = index;
	if (ret_type) *ret_type = type;

	return ret;
}

static void
validate_settings(struct xa_pdlg_info *pdlg, PRN_SETTINGS *pset)
{
	DRV_INFO *drv;
	PRN_ENTRY *prn;

	find_printer(pdlg, pset->driver_id, pset->printer_id, NULL, &drv, &prn);

	if (drv)
	{
		set_driver_pset(drv, pset);
		set_printer_pset(prn, pset);

		if (prn)
		{
			PRN_MODE *mode;
			MEDIA_SIZE *size;
			PRN_TRAY *itray, *otray;

			pset->printer_id = prn->printer_id;

			find_mode(prn, pset->mode_id, NULL, &mode);
			if (mode)
			{
				MEDIA_TYPE *type;

				set_mode_pset(mode, pset);
				find_type(mode, pset->type_id, NULL, &type);
				set_type_pset(type, pset);
			}
			else
			{
				set_mode_pset(NULL, pset);
				set_type_pset(NULL, pset);
			}

			find_size(prn, pset->size_id, NULL, &size);
			set_size_pset(size, pset);

			find_tray(prn, 0, pset->input_id,  NULL, &itray);
			find_tray(prn, 1, pset->output_id, NULL, &otray);
			set_trays_pset(itray, otray, pset);
		}
		else
		{
			pset->printer_id = 0;
			set_mode_pset(NULL, pset);
			set_type_pset(NULL, pset);
			set_size_pset(NULL, pset);
			set_trays_pset(NULL, NULL, pset);
		}
		if (!(pset->page_flags & 3))
			pset->page_flags = (PG_EVEN_PAGES|PG_ODD_PAGES);

		if (pset->first_page < PG_MIN_PAGE)
			pset->first_page = PG_MIN_PAGE;
		else if (pset->first_page > PG_MAX_PAGE)
			pset->first_page = PG_MAX_PAGE;

		if (pset->last_page < PG_MIN_PAGE)
			pset->last_page = PG_MIN_PAGE;
		else if (pset->last_page > PG_MAX_PAGE)
			pset->last_page = PG_MAX_PAGE;

		if (pset->no_copies < 1)
			pset->no_copies = 1;
		else if (pset->no_copies > 999)
			pset->no_copies = 999;


	}
}

static void
init_dialog(struct xa_pdlg_info *pdlg)
{
	PRN_SETTINGS *ps;
	short idx;

#if 1
	ps = &pdlg->current_settings;
	*ps = *pdlg->settings;

	build_printer_popup(pdlg);
	find_printer(pdlg, ps->driver_id, ps->printer_id, &idx, NULL,NULL);
	pdlg->drv_pinf.obnum = idx + 1;
	change_printer(pdlg, idx, ps);

	d_set_page_sel(pdlg, ps->page_flags);
	d_set_fromto_page(pdlg, 0, ps->first_page);
	d_set_fromto_page(pdlg, 1, ps->last_page);
	d_set_copies(pdlg, ps->no_copies);

	d_set_scale(pdlg, ps->scale);
	d_set_orientation(pdlg, ps->orientation);
	d_set_allfrom_sel(pdlg, 0);
	d_set_prn_fgbg(pdlg, (ps->driver_mode & 1));
	d_set_plane_flags(pdlg, ps->plane_flags);
	apply_option_flags(pdlg, pdlg->option_flags);
	pdlg->mwt->tree[XPDLG_PRINT].ob_spec.free_string = pdlg->option_flags & PDLG_PRINT ? "Print" : "OK"; /* TODO: move strings to resource file */

#endif
}

unsigned long
XA_pdlg_open(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_pdlg_info *pdlg;
	struct xa_window *wind;

	DIAG((D_pdlg, client, "XA_pdlg_open"));

	pb->intout[0] = 0;

	pdlg = (struct xa_pdlg_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (pdlg && (wind = get_pdlg_wind(client, pdlg)))
	{
		if (!(wind->window_status & XAWS_OPEN))
		{
			struct xa_widget *widg = get_widget(wind, XAW_TOOLBAR);
			struct widget_tree *wt = pdlg->mwt;
			GRECT r = wind->wa;
			XA_WIND_ATTR tp = wind->active_widgets | MOVER|NAME;

			widg->m.properties |= WIP_NOTEXT;
			set_toolbar_handlers(&wdlg_th, wind, widg, widg->stuff.wt);

			obj_init_focus(wt, OB_IF_RESET);

			if (pb->intin[1] == -1 || pb->intin[2] == -1)
			{
				r.g_x = (root_window->wa.g_w - r.g_w) / 2;
				r.g_y = (root_window->wa.g_h - r.g_h) / 2;
			}
			else
			{
				r.g_x = pb->intin[1];
				r.g_y = pb->intin[2];
			}
			{
				GRECT or;

				obj_rectangle(wt, aesobj(wt->tree, 0), &or);
				or.g_x = r.g_x;
				or.g_y = r.g_y;
				change_window_attribs(lock, client, wind, tp, true, true, 2, or, NULL);
			}


			pdlg->option_flags = pb->intin[0];
			pdlg->settings = (PRN_SETTINGS *)pb->addrin[1];
			get_document_name(pdlg, (const char *)pb->addrin[2]);

			init_dialog(pdlg);

			set_window_title(pdlg->wind, pdlg->document_name, false);

			wt->tree->ob_x = wind->wa.g_x;
			wt->tree->ob_y = wind->wa.g_y;
			if (!wt->zen)
			{
				wt->tree->ob_x += wt->ox;
				wt->tree->ob_y += wt->oy;
			}
			open_window(lock, wind, wind->rc);
		}
		pb->intout[0] = wind->handle;
	}

	return XAC_DONE;
}

unsigned long
XA_pdlg_close(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_pdlg_info *pdlg;
	struct xa_window *wind;

	DIAG((D_pdlg, client, "XA_pdlg_close"));
	pb->intout[0] = 0;

	pdlg = (struct xa_pdlg_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (pdlg && (wind = get_pdlg_wind(client, pdlg)))
	{
		pb->intout[0] = 1;
		pb->intout[1] = pdlg->mwt->tree->ob_x;
		pb->intout[2] = pdlg->mwt->tree->ob_y;
		close_window(lock, wind);
	}
	return XAC_DONE;
}

unsigned long
XA_pdlg_get(int lock, struct xa_client *client, AESPB *pb)
{
	long *o = (long *)&pb->intout[0];

	DIAG((D_pdlg, client, "XA_pdlg_get"));

	pb->intout[0] = 1;

	switch (pb->intin[0])
	{
		case 0:		/* pdlg_get_setsize */
		{
			*o = sizeof(PRN_SETTINGS);
			break;
		}
		default:
		{
			*o = 0L;
		}
	}

	return XAC_DONE;
}

/*
 * Called by delete_xa_data()
 */
static void
delete_usr_settings(void *_us)
{
	struct xa_usr_prn_settings *us = _us;

	if (us->settings)
	{
		ufree(us->settings);
	}
	kfree(us);
}

unsigned long
XA_pdlg_set(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_pdlg_info *pdlg;
	struct xa_window *wind;
	short ret = -1;

	DIAG((D_pdlg, client, "XA_pdlg_set"));

	switch (pb->intin[0])
	{
		case 6:		/* pdlg_free_settings	*/
			{
				struct xa_usr_prn_settings *us;
				PRN_SETTINGS *s = (PRN_SETTINGS *)pb->addrin[0];
	
				us = lookup_xa_data_byid(&client->xa_data, (long)s);
	
				ret = 0;
	
				if (us)
				{
					delete_xa_data(&client->xa_data, us);
					ret = 1;
				}
	
			}
			break;
		default:;
	}

	if (ret == -1)
	{
		pdlg = (struct xa_pdlg_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
		if (pdlg && (wind = get_pdlg_wind(client, pdlg)) != 0)
		{
			switch (pb->intin[0])
			{
				case 0:		/* pdlg_add_printers	*/
					ret = 0;
					break;
				case 1:		/* pdlg_remove_printers	*/
					ret = 0;
					break;
				case 2:		/* Update		*/
					get_document_name(pdlg, (const char *)pb->addrin[2]);
					set_window_title(pdlg->wind, pdlg->document_name, true);
					ret = 1;
					break;
				case 3:		/* pdlg_add_dialogs	*/
					{
						PDLG_SUB *sub;
	
						sub = (PDLG_SUB *)pb->addrin[1];
	
						while (sub)
						{
							char *txt = object_get_string(sub->sub_icon);
							char t[14];
	
							strncpy(t, txt, 14);
	
							add_dialog(pdlg, sub, t/*"App added"*/, NULL, 0);
							sub = sub->next;
						}
						ret = 1;
					}
					break;
				case 4:		/* pdlg_remove_dialogs	*/
					remove_app_dialogs(pdlg);
					ret = 1;
					break;
				case 5:		/* pdlg_new_settings	*/
					{
						struct xa_usr_prn_settings *us;
						PRN_SETTINGS *new;
	
						us = kmalloc(sizeof(*us));
						new = umalloc(sizeof(*new));
						if (us && new)
						{
							us->flags = 1;
							us->settings = new;
	
							*new = pdlg->current_settings;
	
							add_xa_data(&client->xa_data, us, (long)new, "pdlg_usersett", delete_usr_settings);
						}
						else if (us)
						{
							kfree(us);
							us = NULL;
						}
						else if (new)
						{
							ufree(new);
							new = NULL;
						}
						pb->addrout[0] = (long)new;
					}
					break;
				case 7:		/* pdlg_dflt_settings	*/
					{
						PRN_SETTINGS *pset = (PRN_SETTINGS *)pb->addrin[1];
	
						v_read_default_settings(C.P_handle, pset);
					}
					break;
				case 8:		/* pdlg_validate_settings */
					{
						PRN_SETTINGS *pset = (PRN_SETTINGS *)pb->addrin[1];
	
						validate_settings(pdlg, pset);
						ret = 1;
					}
					break;
				case 9:		/* pdlg_use_settings	*/
					{
						PRN_SETTINGS *pset = (PRN_SETTINGS *)pb->addrin[1];
	
						if (pset)
						{
							validate_settings(pdlg, pset);
							v_write_default_settings(C.P_handle, pset);
							ret = 1;
						}
						else
							ret = 1;
					}
					break;
				case 10:	/* pdlg_save_default_settings */
					{
						PRN_SETTINGS *pset = (PRN_SETTINGS *)pb->addrin[1];
	
						if (pset)
						{
							validate_settings(pdlg, pset);
							v_write_default_settings(C.P_handle, pset);
							ret = 1;
						}
						else
							ret = 0;
					}
					break;
				default:
					break;
			}
		}
		else
		{
			switch (pb->intin[0])
			{
				case 0 ... 4:
				case 7 ... 10:
					ret = 0;
					break;
				case 5:
					pb->addrout[0] = 0;
					break;
				default:
					break;
			}
		}
	}

	if (ret != -1)
		pb->intout[0] = ret;

	return XAC_DONE;
}

unsigned long
XA_pdlg_evnt(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_pdlg_info *pdlg;
	struct xa_window *wind;
	short ret = 0;
	short button = 0;

	DIAG((D_pdlg, client, "XA_pdlg_evnt"));

	pdlg = (struct xa_pdlg_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (pdlg && (wind = get_pdlg_wind(client, pdlg)))
	{
		PRN_SETTINGS *out = (PRN_SETTINGS *)pb->addrin[1];
		struct wdlg_evnt_parms wep;

		wep.wind	= wind;
		wep.wt		= get_widget(wind, XAW_TOOLBAR)->stuff.wt;
		wep.ev		= (EVNT *)pb->addrin[2];
		wep.wdlg	= NULL;
		wep.callout	= NULL;
		wep.redraw	= wdialog_redraw;
		wep.obj		= inv_aesobj();

		ret = wdialog_event(lock, client, &wep);

		if (check_internal_objects(pdlg, wep.obj))
		{
			ret = 1;
		}
		else
		{
			if (valid_aesobj(&wep.obj) && (aesobj_ob(&wep.obj)->ob_state & OS_SELECTED))
			{
				obj_change(pdlg->mwt, wind->vdi_settings, wep.obj, -1, aesobj_ob(&wep.obj)->ob_state & ~OS_SELECTED, aesobj_ob(&wep.obj)->ob_flags, true, &wind->wa, wind->rect_list.start, 0);
			}
			/* prepare return stuff here */
		}
		ret = 1;
		if( valid_aesobj(&wep.obj) && aesobj_tree(&wep.obj) == pdlg->mwt->tree )
		{
			switch( aesobj_item(&wep.obj) )
			{
				case XPDLG_PRINT:
					button =  PDLG_OK;
					ret = 0;
				break;
				case XPDLG_CANCEL:
					button =  PDLG_CANCEL;
					ret = 0;
				break;
			}
		}

		if (button == PDLG_OK && out)
		{
			build_prn_settings(pdlg, &pdlg->current_settings);
			*out = pdlg->current_settings;
		}
	}
	pb->intout[0] = ret;
	pb->intout[1] = button;

	DIAG((D_pdlg, client, " --- return %d, obj=%d",
		pb->intout[0], pb->intout[1]));

	return XAC_DONE;
}
static FormKeyInput	Keypress;
static FormExit		Formexit;

static bool
Keypress(int lock,
	 struct xa_client *client,
	 struct xa_window *wind,
	 struct widget_tree *wt,
	 const struct rawkey *key,
	 struct fmd_result *ret_fr)
{
	bool no_exit;

	if ((no_exit = Key_form_do(lock, client, wind, wt, key, NULL)))
	{
		struct scroll_info *list;
		struct xa_pdlg_info *pdlg;

		wt = get_widget(wind, XAW_TOOLBAR)->stuff.wt;
		list = object_get_slist(wt->tree + XPDLG_LIST);
		pdlg = list->data;
		UNUSED(pdlg);
	}
	return no_exit;
}

static void
Formexit( struct xa_client *client,
	  struct xa_window *wind,
	  XA_TREE *wt,
	  struct fmd_result *fr)
{
	struct xa_pdlg_info *pdlg = (struct xa_pdlg_info *)(object_get_slist(wt->tree + XPDLG_LIST))->data;

	if (!check_internal_objects(pdlg, fr->obj))
	{
		if (aesobj_item(&fr->obj) == XPDLG_PRINT || aesobj_item(&fr->obj) == XPDLG_CANCEL)
		{
			pdlg->exit_button = valid_aesobj(&fr->obj) ? aesobj_item(&fr->obj) : 0;
			client->usr_evnt = 1;
		}
		if (valid_aesobj(&fr->obj) && (aesobj_ob(&fr->obj)->ob_state & OS_SELECTED))
		{
			obj_change(pdlg->mwt, wind->vdi_settings, fr->obj, -1, aesobj_ob(&fr->obj)->ob_state & ~OS_SELECTED, aesobj_ob(&fr->obj)->ob_flags, true, &wind->wa, wind->rect_list.start, 0);
		}
	}
}

static struct toolbar_handlers pdlg_th =
{
	Formexit,		/* FormExit		*exitform;	*/
	Keypress,		/* FormKeyInput		*keypress;	*/

	NULL,			/* DisplayWidget	*display;	*/
	NULL,			/* WidgetBehaviour	*click;		*/
	NULL,			/* WidgetBehaviour	*drag;		*/
	NULL,			/* WidgetBehaviour	*release;	*/
	NULL,			/* void (*destruct)(struct xa_widget *w); */
};

unsigned long
XA_pdlg_do(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_pdlg_info *pdlg;
	struct xa_window *wind;

	DIAG((D_pdlg, client, "XA_pdlg_do"));

	pb->intout[0] = 0;

	pdlg = (struct xa_pdlg_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (pdlg && (wind = get_pdlg_wind(client, pdlg)))
	{
		PRN_SETTINGS *outpset = (PRN_SETTINGS *)pb->addrin[1];
		XA_WIDGET *widg = get_widget(wind, XAW_TOOLBAR);
		XA_TREE *wt = pdlg->mwt;
		OBJECT *obtree = wt->tree;
		GRECT or;
		XA_WIND_ATTR tp = wind->active_widgets & ~STD_WIDGETS;
		short ret = PDLG_CANCEL;

		obj_rectangle(wt, aesobj(obtree, 0), &or);
		center_rect(&or);

		change_window_attribs(lock, client, wind, tp, true, true, 2, or, NULL);

		widg->m.properties &= ~WIP_NOTEXT;
		set_toolbar_handlers(&pdlg_th, wind, widg, widg->stuff.wt);
		wt->flags |= WTF_FBDO_SLIST;
	/* ............. */
		pdlg->flags |= 1;
		pdlg->option_flags = pb->intin[0];
		pdlg->settings = outpset;
		get_document_name(pdlg, (const char *)pb->addrin[2]);
		init_dialog(pdlg);

		open_window(lock, wind, wind->rc);

		client->status |= CS_FORM_DO;
		(*client->block)(client);
		client->status &= ~CS_FORM_DO;

		pdlg->flags &= ~1;

		wt->flags &= ~WTF_FBDO_SLIST;

		close_window(lock, wind);

		if (pdlg->exit_button == XPDLG_PRINT)
		{
			build_prn_settings(pdlg, &pdlg->current_settings);
			*outpset = pdlg->current_settings;
			ret = PDLG_OK;
		}
		pb->intout[0] = ret;
	}
	return XAC_DONE;
}

#endif
