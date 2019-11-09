#include "aestests.h"
#include "colricon.h"

extern short _app;

static _WORD aes_handle;
static _WORD gl_wchar;
static _WORD gl_hchar;
static _WORD gl_wbox;
static _WORD gl_hbox;
static GRECT desk;
static _WORD planes;
static _WORD xworkout[57];
static int rsrc_ok;
static int vdo;
static int mch;


static int aes_supports_coloricons(void)
{
	OBJECT *tree = NULL;
	OBJECT *obj;
	long *lp;

	if (!rsrc_gaddr(R_TREE, 0, &tree) || tree == NULL)
		return FALSE;
	obj = &tree[HARDDISK];
	/*
	 * The ob_spec of a coloricon has an index, not a file offset,
	 * therefore the ob_spec of the first coloricon has
	 * a value of zero in the resource file.
	 * If after loading it is still zero, or points to the resource header,
	 * or the object type was changed, assume there is no coloricon support
	 */
	if (obj->ob_type != G_CICON)
	{
		return FALSE;
	}
	if (obj->ob_spec.index == 0)
	{
		return FALSE;
	}
	lp = (long *)&aes_global[7];
	if (obj->ob_spec.index == *lp)
	{
		return FALSE;
	}
	return TRUE;
}


static long get_jarptr(void)
{
	return *((long *) 0x5a0);
}


static int show_dialog(void)
{
	OBJECT *tree = NULL;
	GRECT gr;
	long *jarptr;

	graf_mouse(ARROW, NULL);
	if (!rsrc_ok)
	{
		form_alert(1, "[1][Resource file not found][Abort]");
		return FALSE;
	}
	if (!aes_supports_coloricons())
	{
		form_alert(1, "[1][Your AES does not|support color icons.][Abort]");
		return FALSE;
	}
	if (!rsrc_gaddr(R_TREE, 0, &tree) || tree == NULL)
	{
		form_alert(1, "[3][Error from rsrc_gaddr][Cancel]");
		return FALSE;
	}

	vq_extnd(aes_handle, 1, xworkout);
	planes = xworkout[4];
	jarptr = (long *) Supexec(get_jarptr);
	if (jarptr)
	{
		while (jarptr[0])
		{
			if (jarptr[0] == 0x5F4d4348L)
			{
				mch = ((int) (jarptr[1] >> 16)) & 0x0f;
			}
			if (jarptr[0] == 0x5F56444fL)
			{
				vdo = ((int) (jarptr[1] >> 16)) & 0x0f;
			}

			jarptr += 2;
		}
	}
	sprintf(tree[VDI_PLANES].ob_spec.tedinfo->te_ptext, "%2u", planes);
	sprintf(tree[COOK_VDO].ob_spec.tedinfo->te_ptext, "%2u", vdo);
	sprintf(tree[COOK_MCH].ob_spec.tedinfo->te_ptext, "%2u", mch);

	form_center_grect(tree, &gr);
	wind_update(BEG_UPDATE);
	form_dial(FMD_START, gr.g_x, gr.g_y, gr.g_w, gr.g_h, gr.g_x, gr.g_y, gr.g_w, gr.g_h);
	objc_draw_grect(tree, ROOT, MAX_DEPTH, &gr);
	form_do(tree, ROOT);
	form_dial(FMD_FINISH, gr.g_x, gr.g_y, gr.g_w, gr.g_h, gr.g_x, gr.g_y, gr.g_w, gr.g_h);
	wind_update(END_UPDATE);
	return TRUE;
}


int main(void)
{
	if (appl_init() < 0)
		return 1;

	aes_handle = graf_handle(&gl_wchar, &gl_hchar, &gl_wbox, &gl_hbox);

	rsrc_ok = rsrc_load("colricon.rsc");

	wind_get_grect(DESK, WF_WORKXYWH, &desk);

	if (_AESnumapps != 1)
		shel_write(9, 1, 1, "", "");
	if (!_app)
	{
		_WORD msg[8];
		_WORD menuid;
		int done = FALSE;

		menuid = menu_register(gl_apid, "  Coloricon Test");

		while (!done)
		{
			evnt_mesag(msg);
			switch (msg[0])
			{
			case AC_OPEN:
				if (msg[4] == menuid)
					show_dialog();
				break;
			case AP_TERM:
				done = TRUE;
				break;
			}
		}
	} else
	{
		show_dialog();
	}
	if (rsrc_ok)
		rsrc_free();

	appl_exit();

	return 0;
}
