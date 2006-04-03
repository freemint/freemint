/*
 * $Id$
 * 
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

/*
 * FORM HANDLERS INTERFACE
 */

#include RSCHNAME

#include "xa_form.h"
#include "xa_global.h"

#include "form.h"
#include "c_window.h"
#include "k_main.h"
#include "k_mouse.h"
#include "k_keybd.h"
#include "nkcc.h"
#include "obtree.h"
#include "draw_obj.h"
#include "rectlist.h"
#include "scrlobjc.h"
#include "widgets.h"

#include "xa_evnt.h"
#include "xa_graf.h"
#include "xa_rsrc.h"

#if 0
bool
key_alert_widget(enum locks lock, struct xa_client *client, struct xa_window *wind,
	    struct widget_tree *wt, const struct rawkey *key, struct fmd_result *res_fmd);
#endif
/*
 * Create a copy of an object tree
 * - Intended for using the form templates in SYSTEM.RSC (we can't use them
 * directly as this would cause problems when (for example) two apps do a form_alert()
 * at the same time.
 */
static OBJECT *
CloneForm(OBJECT *form)
{
	int num_objs = 0;
	OBJECT *new_form;

	while ((form[num_objs++].ob_flags & OF_LASTOB) == 0)
		;

	new_form = kmalloc(sizeof(OBJECT) * num_objs);
	DIAGS(("CloneForm: new obtree at %lx", new_form));
	if (new_form)
	{
		int o;

		for (o = 0; o < num_objs; o++)
			new_form[o] = form[o];
	}

	return new_form;
}

/*
 * Free up a copy of a form template
 */
static int
alert_destructor(enum locks lock, struct xa_window *wind)
{
	DIAG((D_form, NULL, "alert_destructor"));
	remove_widget(lock, wind, XAW_TOOLBAR);
	return true;
}

/*
 * Form_alert handler v2.1
 */

static unsigned char *ln = "";	/* current line */
static int mc; 			/* current string max */

static void
ipff_init(int m, char *l)
{
	if (l)
		ln = (unsigned char *)l;
	mc = m;
}

/* skip white space */
static int
sk(void)
{
	int c;

	while ((c = *ln) == ' ' 
		|| c == '\t' 
		|| c == '\r' 
		|| c == '\n'
             ) ++ln;

	/* can be zero: end input */
	return c;
}

/* skip analysed character and following white space */
static int
skc(void)
{
	if (*ln) ++ln;
	return sk();
}

/* skip only analysed character */
static int
sk1(void)
{
	if (*ln)
		return *(++ln);
	return 0;
}

/* give delimited string */
static int
lstr(char *w, unsigned long lim)
{
	int i = 0, c = *ln;
	char *s = (char *)&lim;

	while (i < mc)
	{
		c = *ln;

		if (c == 0     ) break;	/* einde input */
		if (c == *(s+3)) break;
		if (c == *(s+2)) break;
		if (c == *(s+1)) break;
		if (c == * s   ) break;

		*w++ = c;
		ln++;
		i++;
	}

	*w = 0;

	/* return stop ch for analysis */
	return c;
}

/* only there to avoid long mul */
static int
idec(void)
{
	int n = 0; bool t = false;

	if (*ln == '-')
	{
		ln++;
		t = true;
	}

	while (*ln >= '0' && *ln <= '9')
		n = 10 * n + *ln++ - '0';

	if (t) return -n;
	else   return  n;
}

static int
get_parts(int m, char to[][MAX_X+1], int *retv)
{
	int n = 0;

	while (n < m)
	{
		int s;

		/* delimited string, no translation */
		s = lstr(to[n++], 0x7c5d /* '|]' */);
		if (s == '|')
		{
			sk1();
		}
		else if (s == ']')
		{
			skc();
			break;
		}
		else
		{
			/* must be end of string */
			*retv = 0;
			break;
		}
	}

	return n;
}

static int
max_w(int m, char to[][MAX_X+1], int *tot)
{
	int n = 0, x = 2, t = 0;

	while (n < m)
	{
		int l = strlen(to[n]);
		if (l > x)
			x = l;
		t += l+3;
		n++;
	}

	if (tot)
		*tot = t * screen.c_max_w;

	return x * screen.c_max_w;
}

/* changed thus, that a alert is always displayed, whether format error or not.
 * otherwise the app is suspended and the screen & keyb are locked,
 * and you can do nothing but a reset. :-(
 * 
 * reactivated the STORE_BACK facility for form_alert only.
 * If a application has locked the screen, there might be something
 * under the alert that is not a window.
 */
int
do_form_alert(enum locks lock, struct xa_client *client, int default_button, char *alert, char *title)
{
	XA_WIND_ATTR kind = MOVER|NAME|TOOLBAR|USE_MAX;
	struct xa_window *alert_window;
	XA_WIDGET *widg;
	XA_TREE *wt;
	OBJECT *alert_form;
	OBJECT *alert_icons;
	ALERTXT *alertxt;
	RECT or;
	short x, w;
	int n_lines, n_buttons, icon = 0, m_butt_w;
	int retv = 1, b, f;

	DIAG((D_form, client, "called do_form_alert(%s)", alert));

	/* Create a copy of the alert box templates */
	alert_form = CloneForm(ResourceTree(C.Aes_rsc, ALERT_BOX));
	if (!alert_form)
	{
		DIAGS(("CloneForm failed, out of memory?"));
		return 0;
	}

	alertxt = kmalloc(sizeof(*alertxt));
	if (!alertxt)
	{
		kfree(alert_form);

		DIAGS(("kmalloc(%i) failed, out of memory?", sizeof(*alertxt)));
		return 0;
	}

	for (f = 0; f < ALERT_LINES; f++)
		alertxt->text[f][0] = '\0';

	ipff_init(MAX_X, alert);

	if (sk() == '[')		/* skip white space and give non_white char */
	{
		skc();			/* skip char & following white space, give non_white char */
		icon = idec();		/* give decimal number */
		if (sk() == ']')
			skc();
	}

	if (sk() != '[')
		retv = 0;
	else
		sk1();

	/* parse ...|...|... ... ] */
	n_lines = get_parts(ALERT_LINES, alertxt->text, &retv);

	if (sk() != '[')
		retv = 0;
	else
		sk1();

	ipff_init(MAX_B, NULL);

	n_buttons = get_parts(ALERT_BUTTONS, alertxt->button, &retv);

	w = max_w(n_lines,   alertxt->text, NULL);
	    max_w(n_buttons, alertxt->button, &m_butt_w);

	if (m_butt_w > w)
		w = m_butt_w;

	w += 66;
/* >  */

	alert_icons = ResourceTree(C.Aes_rsc, ALERT_ICONS);

	alert_form->ob_width = w;
// 	form_center(alert_form, ICON_H);

	{	/* HR */
		int icons[7] = {ALR_IC_SYSTEM, ALR_IC_WARNING, ALR_IC_QUESTION, ALR_IC_STOP,
						  ALR_IC_INFO,   ALR_IC_DRIVE,   ALR_IC_BOMB};
		if (icon > 7 || icon < 0)
			icon = 0;
		
		for (f = 0; f < 7; f++)
		{
			ICONBLK *ai = object_get_spec(alert_icons + icons[f]    )->iconblk;
			ICONBLK *af = object_get_spec(alert_form  + ALERT_D_ICON)->iconblk;
			ai->ib_xicon = af->ib_xicon;
			ai->ib_yicon = af->ib_yicon;
		}

		(alert_form + ALERT_D_ICON)->ob_spec = (alert_icons + icons[icon])->ob_spec;
	}

	/* Fill in texts */
	for (f = 0; f < ALERT_LINES; f++)
	{
		alert_form[ALERT_T1 + f].ob_spec.free_string = alertxt->text[f];
		if (*alertxt->text[f] == 0)
			alert_form[ALERT_T1 + f].ob_flags |= OF_HIDETREE;
		else
			alert_form[ALERT_T1 + f].ob_flags &= ~OF_HIDETREE;
	}

	/* Space the buttons evenly */
	x = w - m_butt_w;
	b = x / (n_buttons + 1);	
	x = b;

	/* Fill in & show buttons */
	for (f = 0; f < n_buttons; f++)
	{
		int width = strlen(alertxt->button[f])+3;
		width *= screen.c_max_w;
		DIAGS(("button %d, text '%s'", f, alertxt->button[f]));

		alert_form[ALERT_BUT1 + f].ob_spec.free_string = alertxt->button[f];
		alert_form[ALERT_BUT1 + f].ob_width = width;
		alert_form[ALERT_BUT1 + f].ob_x = x;
		alert_form[ALERT_BUT1 + f].ob_flags &= ~(OF_HIDETREE|OF_DEFAULT);
		alert_form[ALERT_BUT1 + f].ob_state = OS_WHITEBAK;
		x += width + b;
	}

	{
		int nl = n_lines, dh;

		if (n_lines < 2)
			nl = 2;
		dh = (ALERT_LINES - nl)*screen.c_max_h;
		alert_form[0].ob_height -= dh;
		alert_form[ALERT_BUT1].ob_y -= dh;
		alert_form[ALERT_BUT2].ob_y -= dh;
		alert_form[ALERT_BUT3].ob_y -= dh;
	}

	/* Set the default button if it was specified */
	if (default_button > 0)
	{
		if (default_button > n_buttons)
			default_button = n_buttons;

		alert_form[ALERT_BUT1 + default_button - 1].ob_flags |= OF_DEFAULT;
	}

	for (f = n_buttons; f < ALERT_BUTTONS; f++)
		/* Hide unused buttons */
		alert_form[ALERT_BUT1 + f].ob_flags |= OF_HIDETREE;

	ob_fix_shortcuts(alert_form, true);

	/* Create a window and attach the alert object tree centered to it */
	{
		bool nolist = false;
		RECT r;

		wt = new_widget_tree(client, alert_form);
		assert(wt);
		wt->flags |= WTF_XTRA_ALLOC | WTF_TREE_ALLOC | WTF_AUTOFREE;
		obj_rectangle(wt, aesobj(alert_form, 0), &or);
		center_rect(&or);
		
		if (C.update_lock && C.update_lock == client->p)
		{
			kind |= STORE_BACK;
			nolist = true;
		}
			
		//if (client->fmd.lock)
		//	kind |= STORE_BACK;

		r = calc_window(lock, client, WC_BORDER,
				kind, created_for_AES,
				client->options.thinframe,
				client->options.thinwork,
				*(RECT *)&or); //*(RECT *)&alert_form->ob_x);

		alert_window = create_window(lock,
					     do_winmesag,
					     do_formwind_msg,
					     client,
					     nolist,
					     kind,
					     created_for_AES|created_for_ALERT,
					     client->options.thinframe,
					     client->options.thinwork,
					     r, NULL, NULL);

		if (alert_window)
		{
			widg = get_widget(alert_window, XAW_TOOLBAR);

			set_toolbar_widget(lock, alert_window, client, alert_form, inv_aesobj(), WIP_NOTEXT, STW_ZEN, NULL, &or); //(RECT *)&alert_form->ob_x);
			wt->extra = alertxt;

			set_window_title(alert_window, title ? title : client->name, false);
			alert_window->destructor = alert_destructor;

			if (nolist)
				client->alert = alert_window;

			open_window(lock, alert_window, alert_window->r);
	
			/* For if the app has hidden the mouse */
			forcem();
		}
		else
			remove_wt(wt, false);
	}

	return retv;
}

/*
 * Primitive version of form_center....
 * - This ignores shadows & stuff
 *
 * HR: It seems that every app knows that :-)
 *
 */
unsigned long
XA_form_center(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree = (OBJECT*)pb->addrin[0];
	short *o = pb->intout;

	CONTROL(0,5,1)

	DIAG((D_form, client, "XA_form_center for %s ob=%lx o=%lx",
		c_owner(client), obtree, o));

	if (validate_obtree(client, obtree, "XA_form_center:") && o)
	{
		RECT r;

		r.w = obtree->ob_width;
		r.h = obtree->ob_height;

		/* desktop work area */
		r.x = root_window->wa.x + ((root_window->wa.w - r.w) / 2);
		r.y = root_window->wa.y + ((root_window->wa.h - r.h) / 2);

		obtree->ob_x = r.x;
		obtree->ob_y = r.y;

 		if (obtree->ob_state & OS_OUTLINED)
			/* This is what other AES's do */
 			adjust_size(3, &r);

		*o++ = 1;
		*(RECT *)o = r;

		DIAG((D_form, client, "   -->    %d/%d,%d/%d", r));
	}

	return XAC_DONE;
}

static short
_form_keybd(	struct xa_client *client,
		struct widget_tree *wt,
		struct xa_window *wind,
		struct xa_aes_object nextobj,
		short *keycode,
		struct xa_aes_object *newobj)
{
	unsigned short ks;
	short cont;
	struct xa_aes_object edit_focus = *newobj;
	struct rawkey key;

	vq_key_s(C.P_handle, &cont);
	ks = (unsigned short)cont;
	key.raw.conin.state = ks;
	key.norm = normkey(ks, *keycode);
	key.aes = *keycode;

	cont = form_keyboard(wt,					/* widget tree	*/
			     client->vdi_settings,
			     edit_focus,				/* obj		*/
			     &key,					/* rawkey	*/
			     true,					/* redraw flag	*/
			     wind ? &wind->rect_list.start : NULL,	/* rect list	*/
			     newobj,					/* nxt obj	*/
			     NULL,					/* newstate	*/
			     keycode);					/* nxtkey	*/

	/*
	 * Ozk: What a mess! If no exitobjects, AES should return whats passed in 'nextobj' when
	 *      the key was not used. XaAES will pass a new editfocus instead if the focus passed
	 *      in intin[0] isnt a valid editable object. If the key was used, then the AES should
	 *      either pass back a new editfocus, or the same value given to us in intin[0].
	 *
	 */
	if (cont)
	{
		if (!*keycode)
		{
			if (aesobj_item(newobj) <= 0)
				*newobj = edit_focus;
		}
		else if (aesobj_item(newobj) <= 0)
			*newobj = nextobj;
	}
	return cont;
}

unsigned long
XA_form_keybd(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	short keyout = 0, cont = 0;
	struct xa_aes_object newobj = aesobj(obtree, 0);

	CONTROL(3,3,1)

	DIAG((D_keybd, client, "XA_form_keybd for %s %lx: obj:%d, k:%x, nob:%d",
		c_owner(client), obtree, pb->intin[0], pb->intin[1], pb->intin[2]));

	if (validate_obtree(client, obtree, "XA_form_keybd:"))
	{
		XA_TREE *wt;

		if (!(wt = obtree_to_wt(client, obtree)))
			wt = new_widget_tree(client, obtree);

// 	 	display("XA_form_keybd: intin0=%d, intin1=%d, intin2=%d", pb->intin[0], pb->intin[1], pb->intin[2]);
		
		newobj = aesobj(wt->tree, pb->intin[0]);
		keyout = pb->intin[1];
		cont = _form_keybd(client, wt, NULL, aesobj(wt->tree,pb->intin[2]), &keyout, &newobj);	
	}
	
	pb->intout[0] = cont;
	pb->intout[1] = aesobj_item(&newobj);
	pb->intout[2] = keyout;

	return XAC_DONE;
}

unsigned long
XA_form_wkeybd(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	short cont = 0, keyout = 0;
	struct xa_aes_object newobj = aesobj(obtree, 0);
	
	CONTROL(4,3,1)

	DIAG((D_keybd, client, "XA_form_wkeybd for %s %lx: obj:%d, k:%x, nob:%d",
		c_owner(client), obtree, pb->intin[0], pb->intin[1], pb->intin[2]));

	cont = keyout = 0;

	if (validate_obtree(client, obtree, "XA_form_wkeybd:"))
	{
		XA_TREE *wt;
		struct xa_window *wind;

		if ((wind = get_wind_by_handle(lock, pb->intin[3])))
		{
			if (!(wt = obtree_to_wt(client, obtree)))
				wt = new_widget_tree(client, obtree);

// 		 	display("XA_form_wkeybd: intin0=%d, intin1=%d, intin2=%d", pb->intin[0], pb->intin[1], pb->intin[2]);
			newobj = aesobj(obtree, pb->intin[0]);
			keyout = pb->intin[1];
			cont = _form_keybd(client, wt, wind, aesobj(obtree, pb->intin[2]), &keyout, &newobj);	
		}
	}
	
	pb->intout[0] = cont;
	pb->intout[1] = aesobj_item(&newobj);
	pb->intout[2] = keyout;

	return XAC_DONE;
}

unsigned long
XA_form_alert(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(1,1,1)

	client->waiting_pb = pb;
	
	DIAG((D_form, client, "XA_alert %s", (char *)pb->addrin[0]));
	client->status |= CS_FORM_ALERT;
	do_form_alert(lock, client, pb->intin[0], (char *)pb->addrin[0], NULL);
	Block(client, 0);
	client->status &= ~CS_FORM_ALERT;

	return XAC_DONE;
}

struct
{
	const char *msg;
	char icon;
}
form_error_msgs[64] =
{
/*   0 -     -              */ { NULL },
/*   1 -  32 - ENOSYS       */ { "Function not implemented",             '5' },
/*   2 -  33 - ENOENT       */ { "No such file or directory",            '5' },
/*   3 -  34 - ENOTDIR      */ { "Not a directory",                      '5' },
/*   4 -  35 - EMFILE       */ { "Too many open files",                  '5' },
/*   5 -  36 - EACCES       */ { "Permission denied",                    '5' },
/*   6 -  37 - EBADF        */ { "Bad file descriptor",                  '5' },
/*   7 -  38 - EPERM        */ { "Operation not permitted",              '5' },
/*   8 -  39 - ENOMEM       */ { "Cannot allocate memory",               '6' },
/*   9 -  40 - EFAULT       */ { "Bad address",                          '5' },
/*  10 -  41 -              */ { "Invalid enviroment.",                  '6' },
/*  11 -  42 -              */ { "Invalid format.",                      '6' },
/*  12 -  43 -              */ { NULL },
/*  13 -  44 -              */ { NULL },
/*  14 -  45 -              */ { NULL },
/*  15 -  46 - ENXIO        */ { "No such device or address",            '5' },
/*  16 -  47 -              */ { "Attempt to delete working directory.", '5' },
/*  17 -  48 - EXDEV        */ { "Cross-device link",                    '5' },
/*  18 -  49 - ENMFILES     */ { "No more matching filenames",           '5' },
/*  19 -  50 - ENFILE       */ { "File table overflow",                  '5' },
/*  20 -  51 -              */ { NULL },
/*  21 -  52 -              */ { NULL },
/*  22 -  53 -              */ { NULL },
/*  23 -  54 -              */ { NULL },
/*  24 -  55 -              */ { NULL },
/*  25 -  56 -              */ { NULL },
/*  26 -  57 -              */ { NULL },
/*  27 -  58 - ELOCKED      */ { "Locking conflict",                     '5' },
/*  28 -  59 - ENSLOCK      */ { "No such lock",                         '5' },
/*  29 -  60 -              */ { NULL },
/*  30 -  61 -              */ { NULL },
/*  31 -  62 -              */ { NULL },
/*  32 -  63 -              */ { NULL },
/*  33 -  64 - EBADARG      */ { "Bad argument",                         '5' },
/*  34 -  65 - EINTERNAL    */ { "Internal error",                       '5' },
/*  35 -  66 - ENOEXEC      */ { "Invalid executable file format",       '5' },
/*  36 -  67 - ESBLOCK      */ { "Memory block growth failure",          '5' },
/*  37 -  68 - EBREAK       */ { "Aborted by user",                      '5' },
/*  38 -  69 - EXCPT        */ { "Terminated with bombs",                '5' },
/*  39 -  70 - ETXTBSY      */ { "Text file busy",                       '5' },
/*  40 -  71 - EFBIG        */ { "File too big",                         '5' },
/*  41 -  72 -              */ { NULL },
/*  42 -  73 -              */ { NULL },
/*  43 -  74 -              */ { NULL },
/*  44 -  75 -              */ { NULL },
/*  45 -  76 -              */ { NULL },
/*  46 -  77 -              */ { NULL },
/*  47 -  78 -              */ { NULL },
/*  48 -  79 -              */ { NULL },
/*  49 -  80 - ELOOP        */ { "Too many symbolic links",              '5' },
/*  50 -  81 - EPIPE        */ { "Broken pipe",                          '5' },
/*  51 -  82 - EMLINK       */ { "Too many links",                       '5' },
/*  52 -  83 - ENOTEMPTY    */ { "Directory not empty",                  '5' },
/*  53 -  84 -              */ { NULL },
/*  54 -  85 - EEXIST       */ { "File exists",                          '5' },
/*  55 -  86 - ENAMETOOLONG */ { "Name too long",                        '5' },
/*  56 -  87 - ENOTTY       */ { "Not a tty",                            '5' },
/*  57 -  88 - ERANGE       */ { "Range error",                          '5' },
/*  58 -  89 - EDOMAIN      */ { "Domain error",                         '5' },
/*  59 -  90 - EIO          */ { "I/O error",                            '5' },
/*  60 -  91 - ENOSPC       */ { "No space on device",                   '5' },
/*  61 -  92 -              */ { NULL },
/*  62 -  93 -              */ { NULL },
/*  63 -  94 -              */ { NULL },
};

unsigned long
XA_form_error(enum locks lock, struct xa_client *client, AESPB *pb)
{
	static char error_alert[256]; // XXX

	const char *msg = "Unknown error.";
	char icon = '7';
	int num;

	CONTROL(1,1,0)

	client->waiting_pb = pb;
	num = pb->intin[0];

	if (num >= 0 && num < (sizeof(form_error_msgs) / sizeof(form_error_msgs[0])))
	{
		if (form_error_msgs[num].msg)
		{
			msg = form_error_msgs[num].msg;
			icon = form_error_msgs[num].icon;
		}
	}

	sprintf(error_alert, sizeof(error_alert), "[%c][ ERROR: | %s ][ Ok ]", icon, msg);

	DIAG((D_form, client, "alert_err %s", error_alert));
	client->status |= CS_FORM_ALERT;
	do_form_alert(lock, client, 1, error_alert, NULL);
	Block(client, 0);
	client->status &= ~CS_FORM_ALERT;
	
	return XAC_DONE;
}

/*
 *  Begin/end form handler
 *  This is important - I hope most programs call this, as XaAES puts its dialogs
 *  in windows, and the windows are created here...
 */
unsigned long
XA_form_dial(enum locks lock, struct xa_client *client, AESPB *pb)
{	
	struct xa_window *wind;

	CONTROL(9,1,0)
	
	switch(pb->intin[0])
	{
	case FMD_START:
	{
		DIAG((D_form, client, "form_dial(FMD_START,%d,%d,%d,%d) for %s",
			pb->intin[5], pb->intin[6], pb->intin[7], pb->intin[8], c_owner(client)));

		client->fmd.state = 1;
		break;
	}
	case FMD_GROW:
	{
		break;
	}
	case FMD_SHRINK:
	{
		break;
	}
	case FMD_FINISH:
	{
		DIAG((D_form,client,"form_dial(FMD_FINISH) for %s", c_owner(client)));

		if (client->fmd.wind)
		{
			/* If the client's dialog window is still hanging around, dispose of it... */
			wind = client->fmd.wind;
			DIAG((D_form,client,"Finish fmd.wind %d", wind->handle));
			close_window(lock, wind);
			delete_window(lock, wind);
		}
		else
			/* This was just a redraw request */
			update_windows_below(lock, (const RECT *)(&(pb->intin[5])), NULL, window_list, NULL);

		bzero(&client->fmd, sizeof(client->fmd));
		break;
	}
	}
	
	pb->intout[0] = 1;
	
	return XAC_DONE;
}


/*
 * Non-blocking form_do
 * - Uses an object tree widget in a window to implement the form handler.
 *   unless the client already locked the screen, or option 'xa_windows none' is set.
 */
unsigned long
XA_form_do(enum locks lock, struct xa_client *client, AESPB *pb)
{
// 	bool d = (!strnicmp(client->proc_name, "stzip", 5)) ? true : false;
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	CONTROL(1,1,1)

	DIAG((D_form, client, "XA_form_do() for %s. obtree %lx", client->name, obtree));
	
	if (validate_obtree(client, obtree, "XA_form_do:"))
	{
		struct xa_window *wind;
		struct xa_aes_object nextobj;

		nextobj = aesobj(obtree, pb->intin[0] == 0 ? -2 : pb->intin[0]);
		client->waiting_pb = pb;

// 		if (d) display("obtree rect = %d/%d/%d/%d", *(RECT *)&obtree->ob_x);
	
		if (Setup_form_do(client, obtree, nextobj, &wind, &nextobj))
		{
			client->status |= CS_FORM_DO;
			if (wind)
			{
				if (!(wind->window_status & XAWS_OPEN))
					open_window(lock, wind, wind->rc);
				else if (!wind->nolist && !is_topped(wind))
					top_window(lock, true, false, wind, (void *)-1L);
				else
					display_window(lock, 4, wind, NULL);

			}
// 			display("wait for form_do...");
			Block(client, 0);
			client->status &= ~CS_FORM_DO;
// 			display(" ... return from form_do");
			return XAC_DONE;
		}
		/* XXX - Ozk:
		 *  If client didnt fetch update lock calling wind_upd(), XaAES uses
		 *  windowed dialog. If something goes wrong opening a window
		 *  like no more memory, we need to act upon that here. Lets try
		 *  to wait for the update_lock and force non-windowed form_do handling
		 *  later...
		 */
		
	}
	else
	{
		/* XXX - Ozk:
		 * Here we have another situation where the AES app passed a NULL ptr
		 * to obtree. Dont know if this ever happens, but ....
		 */
	}
		return XAC_DONE;
}

unsigned long
XA_form_button(enum locks lock, struct xa_client *client, AESPB *pb)
{
	XA_TREE *wt;
	OBJECT *obtree = (OBJECT*)pb->addrin[0];
	struct moose_data md;
	bool retv;

	CONTROL(2,2,1)

	DIAG((D_form, client, "XA_form_button %lx: obj:%d, clks:%d",
		obtree, pb->intin[0], pb->intin[1]));

	if (validate_obtree(client, obtree, "XA_form_button:"))
	{
		short newstate, clickmask;
		struct xa_aes_object obj, nextobj;

		if (!(wt = obtree_to_wt(client, obtree)))
			wt = new_widget_tree(client, obtree);
		/*
		 * Create a moose_data packet...
		 */
		check_mouse(client, &md.cstate, &md.x, &md.y);
		if ((md.clicks = pb->intin[1]))
			md.state = MBS_LEFT;

		obj = aesobj(wt->tree, pb->intin[0]);

		/* XXX - Ozk:
		 * I think we need to look for this obtree to see if we have it in
		 * a window, in which case we need to use that windows rectangle list
		 * during updates of the window.
		 */
		retv = form_button(wt,		/* widget tree	*/
				   client->vdi_settings,
				   obj,		/* obj idx	*/
				   &md,		/* moose data	*/
				   FBF_REDRAW|FBF_CHANGE_FOCUS,	/* redraw flag	*/
				   NULL,	/* rect list	*/
				   /* output */
				   &newstate,	/* new state	*/
				   &nextobj,	/* next obj	*/
				   &clickmask);	/* click mask	*/

		pb->intout[0] = retv;
		pb->intout[1] = aesobj_item(&nextobj) | clickmask;
	}
	else
		pb->intout[0] = pb->intout[1] = 0;


	return XAC_DONE;
}

unsigned long
XA_form_wbutton(enum locks lock, struct xa_client *client, AESPB *pb)
{
	XA_TREE *wt;
	OBJECT *obtree = (OBJECT*)pb->addrin[0];
	struct xa_window *wind;
	short obj = pb->intin[0], newstate, objout = 0, clickmask;
	struct moose_data md;
	bool retv = false;

	CONTROL(3,2,1)

	DIAG((D_form, client, "XA_form_button %lx: obj:%d, clks:%d",
		obtree, obj, pb->intin[1]));

	if (validate_obtree(client, obtree, "XA_form_button:"))
	{
		if ((wind = get_wind_by_handle(lock, pb->intin[2])))
		{
			struct xa_aes_object nextobj;
			
			if (!(wt = obtree_to_wt(client, obtree)))
				wt = new_widget_tree(client, obtree);
			
			nextobj = aesobj(obtree, obj);
			/*
			 * Create a moose_data packet...
			 */
			check_mouse(client, &md.cstate, &md.x, &md.y);
			if ((md.clicks = pb->intin[1]))
				md.state = MBS_LEFT;
	
			/* XXX - Ozk:
			 * I think we need to look for this obtree to see if we have it in
			 * a window, in which case we need to use that windows rectangle list
			 * during updates of the window.
			 */
			retv = form_button(wt,				/* widget tree	*/
					   client->vdi_settings,
					   nextobj,			/* obj idx	*/
					   &md,				/* moose data	*/
					   FBF_REDRAW|FBF_CHANGE_FOCUS,	/* redraw flag	*/
					   &wind->rect_list.start,	/* rect list	*/
					   /* output */
					   &newstate,			/* new state	*/
					   &nextobj,			/* next obj	*/
					   &clickmask);			/* click mask	*/

			objout = aesobj_item(&nextobj) | clickmask;
		}
	}
	pb->intout[0] = retv;
	pb->intout[1] = objout;
	return XAC_DONE;
}
