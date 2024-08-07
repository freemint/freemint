/* DEMO.C
 * ================================================================
 * DESCRIPTION: Test program for:
 *		   1) Hierarchical Menus
 *		   2) Pop-Up Menus
 *		   3) Drop-Down List Menus
 *		   4) Removing a SubMenu
 *		   5) Attaching a SubMenu
 *		   6) Disabling and Changing Text on the
 *		      About Menu Item.
 *		   7) ToolBars
 *
 * Last Modified:  05/14/93 cjg - Put in Drop-Down List Menu Test
 *		   05/19/93 cjg - Cleaned up for Public Viewing
 *		   06/01/93 cjg - Added test for ToolBoxes
 *		   06/07/93 cjg - Changed name to Toolbars
 */



/* INCLUDES
 * ================================================================
 */
#include <string.h>
#ifdef __PUREC__
#include <aes.h>
#include <vdi.h>
#else
#include <gem.h>
#endif

#include "toolbar.h"				/* Resource 'H' file   */


#ifndef _WORD
# ifdef __PUREC__
#  define _WORD int
# else
#  define _WORD short
# endif
#endif

#ifndef FALSE
# define FALSE 0
# define TRUE 1
#endif

/* GLOBALS
 * ================================================================
 */

/* VDI Variables */
static _WORD work_in[12];
static _WORD work_out[57];
static _WORD vhandle;
static _WORD xres, yres;
static _WORD nplanes;
static _WORD phys_handle;


/* AES variables */
_WORD gl_apid;
static _WORD gl_wchar, gl_hchar;
static _WORD gl_wbox, gl_hbox;


/* OBJECT Tree pointers */
static OBJECT *ad_tree;						/* About Dialog Box    */
static OBJECT *ad_menubar;					/* MenuBar Resource Ptr    */
static OBJECT *ad_fonts;					/* Fonts Menu Pointer      */
static OBJECT *ad_style;					/* Fonts Style Menu Pointer */
static OBJECT *ad_position;					/* Fonts Position Menu Ptr */
static OBJECT *ad_color;					/* Color Selection Menu Ptr */
static OBJECT *ad_pattern;					/* Patterns Menu Ptr       */
static OBJECT *ad_modem;					/* Modem Port Dialog Box   */

static OBJECT *ad_baudrate;					/* BaudRate Menu           */
static OBJECT *ad_parity;					/* Parity Bit Menu         */
static OBJECT *ad_bittree;					/* Bits Menu           */
static OBJECT *ad_stoptree;					/* Stop Bits Menu      */
static OBJECT *ad_porttree;					/* Modem Port Menu     */
static OBJECT *ad_flowtree;					/* Flow Control Menu       */

static OBJECT *ad_list;						/* Drop Down List Dialog   */
static OBJECT *ad_tools;					/* ToolBar...          */
static OBJECT *ad_blank;
static OBJECT *ad_box2;
static OBJECT *cur_tree;
static OBJECT *ad_box3;
static OBJECT *ad_font2;

static _WORD CurBaudRate = 1;				/* Current BaudRate    */
static _WORD CurParity = 1;					/* Current Parity      */
static _WORD CurBits = 1;					/* Current Modem Bits      */
static _WORD CurStopBits = 1;				/* Current Stop Bits       */
static _WORD CurPort = 1;					/* Current Modem Port      */
static _WORD CurFlow = 1;					/* Current Flow Control    */
static _WORD CurStyle = 1;					/* Current Font Style      */
static _WORD CurPos = 1;					/* Current Font Position   */
static _WORD CurFonts = 1;					/* Current Font            */
static _WORD CurFont2 = 2;

static OBJECT **ptr;						/* Global Temp Object ptr  */

static _WORD SubFlag;						/* Toggle Flag for Submenus ON/OFF        */
static _WORD menu_flag;						/* Toggle Flag for About Box enabled/Disabled */
static _WORD ToolFlag;						/* Toggle Flag for Tool Box Enable/Disable    */
static char *textptr;						/* Temp Ptr to ObString Menu Items...         */

static MENU Menu;							/* Menu Structure Info Passed TO menu_popup() */
static MENU MData;							/* Menu Structure Info Returned FROM Menu_popup */

static _WORD wid;							/* window handle */
static GRECT desk;
static GRECT CurRect;
static char tbuff[30];


/* Text Arrays
 * ================================================================
 * The following arrays contain text destined for the buttons on a
 * dialog representing the active menu item.
 */

/* BaudRate Text */
static char *TextBaudRate[] = {
	"19200", "9600", "4800", "3600", "2400",
	"2000", "1800", "1200", "600", "300",
	"200", "150", "134", "110", "75", "50"
};

/* Parity Text */
static char *TextParity[] = { "None", "Odd", "Even" };


/* Modem Bits Text */
static char *TextBits[] = { "8", "7", "6", "5" };


/* Modem Stop Bits Text */
static char *TextStopBits[] = { "1", "1.5", "2" };


/* Modem Port Text */
static char *TextPort[] = { "Modem1", "Modem2", "Serial1", "Serial2" };


/* Modem Flow Control Text */
static char *TextFlow[] = { "None", "Xon/Xoff", "Rts/Cts" };


/* Text for the About Menu Item to Enable or Disable */
static char *TextAbout[] = { "  Enable About...     ", "  Disable About...    " };


/* Text for Enable/Disable Submenu Menu Items */
static char *TextSubMenu[] = { "  Enable Submenus...  ", "  Disable Submenus... " };

/* Text for Enable/Disable ToolBar Menu Items */
static char *TextToolBox[] = { "  Enable ToolBar...   ", "  Disable ToolBar...  " };



/* Functions
 * ================================================================
 */

/* MenuCheck
 * ================================================================
 * This routine will update the current variables, reset checkmarks
 * and reset the starting index.  This routine is called for a popup.
 */
static void MenuCheck(OBJECT *ptree, _WORD pmenu, _WORD pitem)
{
	(void)pmenu;
	/* Update the Baudrate current variable */
	if (ad_baudrate == ptree)
	{
		menu_icheck(ad_baudrate, CurBaudRate, 0);
		menu_icheck(ad_baudrate, pitem, 1);
		CurBaudRate = pitem;
		menu_istart(1, ad_baudrate, ROOT, CurBaudRate);
	}


	/* Update the Parity current variable */
	if (ad_parity == ptree)
	{
		menu_icheck(ad_parity, CurParity, 0);
		menu_icheck(ad_parity, pitem, 1);
		CurParity = pitem;
		menu_istart(1, ad_parity, ROOT, CurParity);
	}


	/* Update the bits/char current variable */
	if (ad_bittree == ptree)
	{
		menu_icheck(ad_bittree, CurBits, 0);
		menu_icheck(ad_bittree, pitem, 1);
		CurBits = pitem;
		menu_istart(1, ad_bittree, ROOT, CurBits);
	}


	/* Update the stop bits current variable */
	if (ad_stoptree == ptree)
	{
		menu_icheck(ad_stoptree, CurStopBits, 0);
		menu_icheck(ad_stoptree, pitem, 1);
		CurStopBits = pitem;
		menu_istart(1, ad_stoptree, ROOT, CurStopBits);
	}



	/* Update the MOdem Port variable */
	if (ad_porttree == ptree)
	{
		menu_icheck(ad_porttree, CurPort, 0);
		menu_icheck(ad_porttree, pitem, 1);
		CurPort = pitem;
		menu_istart(1, ad_porttree, ROOT, CurPort);
	}


	/* Update the Flow Control current variable */
	if (ad_flowtree == ptree)
	{
		menu_icheck(ad_flowtree, CurFlow, 0);
		menu_icheck(ad_flowtree, pitem, 1);
		CurFlow = pitem;
		menu_istart(1, ad_flowtree, ROOT, CurFlow);
	}
}





/* execform()
 * ================================================================
 * Custom routine to put up a standard dialog box and wait for a key.
 * Used by the ABOUT Dialog Box.
 */
static int execform(OBJECT *tree, int start_obj)
{
	GRECT rect;
	GRECT xrect;
	int button;

	xrect.g_x = xrect.g_y = 10;
	xrect.g_w = xrect.g_h = 36;
	form_center(tree, &rect.g_x, &rect.g_y, &rect.g_w, &rect.g_h);
	form_dial(FMD_START, xrect.g_x, xrect.g_y, xrect.g_w, xrect.g_h, rect.g_x, rect.g_y, rect.g_w, rect.g_h);
	form_dial(FMD_GROW, xrect.g_x, xrect.g_y, xrect.g_w, xrect.g_h, rect.g_x, rect.g_y, rect.g_w, rect.g_h);

	objc_draw(tree, ROOT, MAX_DEPTH, rect.g_x, rect.g_y, rect.g_w, rect.g_h);
	button = form_do(tree, start_obj);
	form_dial(FMD_SHRINK, xrect.g_x, xrect.g_y, xrect.g_w, xrect.g_h, rect.g_x, rect.g_y, rect.g_w, rect.g_h);

	form_dial(FMD_FINISH, xrect.g_x, xrect.g_y, xrect.g_w, xrect.g_h, rect.g_x, rect.g_y, rect.g_w, rect.g_h);
	tree[button].ob_state &= ~OS_SELECTED;
	return button;
}


static void objc_xywh(OBJECT *tree, _WORD obj, GRECT *gr)
{
	objc_offset(tree, obj, &gr->g_x, &gr->g_y);
	gr->g_w = tree[obj].ob_width;
	gr->g_h = tree[obj].ob_height;
}



/* DoPopup()
 * ================================================================
 * Generic Popup Routines for the Modem Dialog Box and all of its menus.
 */
static void DoPopup(int titlenum, _WORD button, OBJECT *poptree, _WORD *CurValue, char *Text[])
{
	GRECT box;
	GRECT title;
	_WORD flag;
	OBJECT *tree;

	tree = ad_modem;				/* sets tree = ad_modem - used by TedText macro */

	/* Select and Redraw the Text Title preceding the button */
	ad_modem[titlenum].ob_state |= OS_SELECTED;
	objc_xywh(ad_modem, titlenum, &title);
	objc_draw(ad_modem, titlenum, 1, title.g_x, title.g_y, title.g_w, title.g_h);

	/* Get the GRECT of the button */
	objc_xywh(ad_modem, button, &box);

	Menu.mn_tree = poptree;				/* OBJECT tree of the menu           */
	Menu.mn_menu = ROOT;				/* The Parent of the menu items      */
	Menu.mn_item = *CurValue;			/* The Current Value of the menu     */
	Menu.mn_scroll = FALSE;				/* Scroll if > than height limit?-NO! */

	/* Jump to and execute the Popup menu */
	flag = menu_popup(&Menu, box.g_x, box.g_y, &MData);

	if (flag)							/* NON-ZERO - the user clicked on a valid menu item  */
	{
		/* Update the Current Value based upon the tree, menu and new item selected */
		MenuCheck(MData.mn_tree, MData.mn_menu, MData.mn_item);

		/* Update the Text within the button */
		tree[button].ob_spec.tedinfo->te_ptext = Text[*CurValue - 1];
	}

	/* Deselect and REDRAW the title in front of the button */
	ad_modem[titlenum].ob_state &= ~OS_SELECTED;
	objc_draw(ad_modem, titlenum, 1, title.g_x, title.g_y, title.g_w, title.g_h);


	/* Deselect and Redraw the button */
	ad_modem[button].ob_state &= ~OS_SELECTED;
	box.g_x -= 1;
	box.g_y -= 1;
	box.g_w += 2;
	box.g_h += 2;
	objc_draw(ad_modem, ROOT, MAX_DEPTH, box.g_x, box.g_y, box.g_w, box.g_h);
}




/* do_modem()
 * ================================================================
 * This is a dialog box that will test POP-UP MENUS
 */
static void do_modem(void)
{
	GRECT rect;
	GRECT xrect;
	int button;
	OBJECT *tree;

	wind_update(BEG_UPDATE);

	tree = ad_modem;
	tree[M1BUTTON].ob_spec.tedinfo->te_ptext = TextBaudRate[CurBaudRate - 1];
	tree[M2BUTTON].ob_spec.tedinfo->te_ptext = TextParity[CurParity - 1];

	xrect.g_x = xrect.g_y = 10;
	xrect.g_w = xrect.g_h = 36;
	form_center(ad_modem, &rect.g_x, &rect.g_y, &rect.g_w, &rect.g_h);
	form_dial(FMD_START, xrect.g_x, xrect.g_y, xrect.g_w, xrect.g_h, rect.g_x, rect.g_y, rect.g_w, rect.g_h);
	form_dial(FMD_GROW, xrect.g_x, xrect.g_y, xrect.g_w, xrect.g_h, rect.g_x, rect.g_y, rect.g_w, rect.g_h);

	objc_draw(ad_modem, ROOT, MAX_DEPTH, rect.g_x, rect.g_y, rect.g_w, rect.g_h);
	do
	{
		button = form_do(ad_modem, 0);

		switch (button)
		{
		case M1BUTTON:
			DoPopup(M1TITLE, M1BUTTON, ad_baudrate, &CurBaudRate, TextBaudRate);
			break;

		case M2BUTTON:
			DoPopup(M2TITLE, M2BUTTON, ad_parity, &CurParity, TextParity);
			break;

		case M3BUTTON:
			DoPopup(M3TITLE, M3BUTTON, ad_bittree, &CurBits, TextBits);
			break;

		case M4BUTTON:
			DoPopup(M4TITLE, M4BUTTON, ad_stoptree, &CurStopBits, TextStopBits);
			break;

		case M5BUTTON:
			DoPopup(M5TITLE, M5BUTTON, ad_porttree, &CurPort, TextPort);
			break;

		case M6BUTTON:
			DoPopup(M6TITLE, M6BUTTON, ad_flowtree, &CurFlow, TextFlow);
			break;

		default:
			break;
		}

	} while (button != MOK);

	form_dial(FMD_SHRINK, xrect.g_x, xrect.g_y, xrect.g_w, xrect.g_h, rect.g_x, rect.g_y, rect.g_w, rect.g_h);

	form_dial(FMD_FINISH, xrect.g_x, xrect.g_y, xrect.g_w, xrect.g_h, rect.g_x, rect.g_y, rect.g_w, rect.g_h);
	ad_modem[button].ob_state &= ~OS_SELECTED;
	wind_update(END_UPDATE);
}





/* AttachMenus()
 * ================================================================
 * Attach Submenus for this program
 */
static void AttachMenus(void)
{

	/* Attach the FONTS menu to the menu item FONTS1 in the menubar */
	Menu.mn_tree = ad_fonts;
	Menu.mn_menu = ROOT;
	Menu.mn_item = 1;
	Menu.mn_scroll = TRUE;
	menu_attach(1, ad_menubar, FONTS1, &Menu);
	menu_istart(1, ad_fonts, ROOT, CurFonts);

	/* Attach the Font Style Menu to the menu item STYLE in the menubar */
	Menu.mn_tree = ad_style;
	Menu.mn_menu = ROOT;
	Menu.mn_item = 1;
	Menu.mn_scroll = FALSE;
	menu_attach(1, ad_menubar, STYLE, &Menu);
	menu_istart(1, ad_style, ROOT, CurStyle);

	/* Attach the Fonts Position menu to the menu item POSITION in the menubar */
	Menu.mn_tree = ad_position;
	Menu.mn_menu = ROOT;
	Menu.mn_item = 1;
	Menu.mn_scroll = FALSE;
	menu_attach(1, ad_menubar, POSITION, &Menu);
	menu_istart(1, ad_position, ROOT, CurPos);

	/* Attach the COLOR Menu to the menu item COLOR in the menubar */
	Menu.mn_tree = ad_color;
	Menu.mn_menu = ROOT;
	Menu.mn_item = 1;
	Menu.mn_scroll = FALSE;
	menu_attach(1, ad_menubar, COLOR, &Menu);

	/* Attach the PATTERN menu to the menu item PATTERN1 in the menubar */
	Menu.mn_tree = ad_pattern;
	Menu.mn_menu = ROOT;
	Menu.mn_item = 1;
	Menu.mn_scroll = FALSE;
	menu_attach(1, ad_menubar, PATTERN1, &Menu);

	Menu.mn_tree = ad_font2;
	Menu.mn_menu = F2MENU;
	Menu.mn_item = 1;
	Menu.mn_scroll = TRUE;
	menu_attach(1, ad_fonts, 10, &Menu);
	menu_istart(1, ad_font2, F2MENU, CurFont2);
}



/* DetachMenus()
 * ================================================================
 * Detach ALL submenus for this program.
 */
static void DetachMenus(void)
{
	/* Detach the menus (regardless of what) from the menu items below */
	menu_attach(1, ad_menubar, FONTS1, NULL);
	menu_attach(1, ad_menubar, STYLE, NULL);
	menu_attach(1, ad_menubar, POSITION, NULL);
	menu_attach(1, ad_menubar, COLOR, NULL);
	menu_attach(1, ad_menubar, PATTERN1, NULL);
	menu_attach(1, ad_fonts, 10, NULL);
}



/* DoList()
 * ================================================================
 * This is a Dialog Box that will test Drop Down List Menus
 */
static void DoList(void)
{
	GRECT rect;
	GRECT xrect;
	int button;
	OBJECT *tree;
	GRECT box;
	GRECT title;
	_WORD flag;

	wind_update(BEG_UPDATE);


	/* Put the Current Font Text String into the Button */
	tree = ad_fonts;
	textptr = tree[CurFonts].ob_spec.free_string;

	tree = ad_list;
	tree[LBUTTON].ob_spec.tedinfo->te_ptext = textptr;


	/* Prepare to display the dialog box */
	xrect.g_x = xrect.g_y = 10;
	xrect.g_w = xrect.g_h = 36;
	form_center(ad_list, &rect.g_x, &rect.g_y, &rect.g_w, &rect.g_h);
	form_dial(FMD_START, xrect.g_x, xrect.g_y, xrect.g_w, xrect.g_h, rect.g_x, rect.g_y, rect.g_w, rect.g_h);
	form_dial(FMD_GROW, xrect.g_x, xrect.g_y, xrect.g_w, xrect.g_h, rect.g_x, rect.g_y, rect.g_w, rect.g_h);
	objc_draw(ad_list, ROOT, MAX_DEPTH, rect.g_x, rect.g_y, rect.g_w, rect.g_h);

	do
	{
		button = form_do(ad_list, 0);

		/* See if we clicked on the BUTTON or the HOT BUTTON
		 * The HOT BUTTON is the 3 line box to the right of the button.
		 */
		if ((button == LBUTTON) || (button == LHOT))
		{

			/* SELECT and REDRAW the text title */
			ad_list[LTITLE].ob_state |= OS_SELECTED;
			objc_xywh(ad_list, LTITLE, &title);
			objc_draw(ad_list, LTITLE, 1, title.g_x, title.g_y, title.g_w, title.g_h);

			/* SELECT and REDRAW the BUTTON */
			ad_list[LBUTTON].ob_state |= OS_SELECTED;
			objc_xywh(ad_list, LBUTTON, &box);
			objc_draw(ad_list, LBUTTON, 1, box.g_x, box.g_y, box.g_w, box.g_h);
			box.g_x = ((box.g_x + 7) / 8) * 8;

			/* Set up the Data Structure for a Drop Down List Menu */
			Menu.mn_tree = ad_fonts;
			Menu.mn_menu = ROOT;
			Menu.mn_item = CurFonts;
			Menu.mn_scroll = -1;		/* <---------DROP DOWN LIST (-1) */
			flag = menu_popup(&Menu, box.g_x, box.g_y + box.g_h + 1, &MData);
			if (flag)
			{
				menu_icheck(ad_fonts, CurFonts, 0);	/* Turn off CheckMark on Old item */
				menu_icheck(ad_fonts, MData.mn_item, 1);	/* Turn on checkMark on NEW item */
				CurFonts = MData.mn_item;	/* Update Variable */
				menu_istart(1, ad_fonts, ROOT, CurFonts);	/* Reset starting value in menu  */

				tree = ad_fonts;	/* Change the string in the button */
				textptr = tree[CurFonts].ob_spec.free_string;

				tree = ad_list;
				tree[LBUTTON].ob_spec.tedinfo->te_ptext = textptr;
			}

			/* Deselect the TITLE */
			ad_list[LTITLE].ob_state &= ~OS_SELECTED;
			objc_draw(ad_list, LTITLE, 1, title.g_x, title.g_y, title.g_w, title.g_h);


			/* Deselect the button */
			ad_list[LBUTTON].ob_state &= ~OS_SELECTED;
			objc_xywh(ad_list, LBUTTON, &box);
			box.g_x -= 1;
			box.g_y -= 1;
			box.g_w += 2;
			box.g_h += 2;
			objc_draw(ad_list, ROOT, MAX_DEPTH, box.g_x, box.g_y, box.g_w, box.g_h);
		}

	} while (button != LOK);

	form_dial(FMD_SHRINK, xrect.g_x, xrect.g_y, xrect.g_w, xrect.g_h, rect.g_x, rect.g_y, rect.g_w, rect.g_h);

	form_dial(FMD_FINISH, xrect.g_x, xrect.g_y, xrect.g_w, xrect.g_h, rect.g_x, rect.g_y, rect.g_w, rect.g_h);
	ad_list[button].ob_state &= ~OS_SELECTED;
	wind_update(END_UPDATE);
}



/* InitWindow()
 * ================================================================
 * Initialize the window
 */
static void InitWindow(void)
{
	_WORD offset;

	wid = 0;

	wid = wind_create((NAME | CLOSER | FULLER | MOVER | INFO | SIZER |
					   UPARROW | DNARROW | VSLIDE | LFARROW | SMALLER |
					   RTARROW | HSLIDE), desk.g_x, desk.g_y, desk.g_w, desk.g_h);

	if (wid > 0)
	{
		wind_set_str(wid, WF_NAME, " ATARI TOOLBAR TEST ");
		wind_set_str(wid, WF_INFO, " Information Line ");

		ad_tools[ROOT].ob_width = xres - 2;

		ad_box2[ROOT].ob_width = xres - 2;
		offset = (ad_box2[BASE2].ob_x - ad_box2[BASE1].ob_x) * 2;
		ad_box2[BASE1].ob_width = xres - 4;
		ad_box2[BASE2].ob_width = xres - 4 - offset;

		ad_box3[ROOT].ob_width = xres - 2;
		ad_box3[T3BASE1].ob_width = xres - 2;
		ad_box3[T3BASE2].ob_width = xres - 2;

		if (ToolFlag)
		{
			wind_set_ptr(wid, WF_TOOLBAR, cur_tree);
		}
		wind_open(wid, CurRect.g_x, CurRect.g_y, CurRect.g_w, CurRect.g_h);
	}
}


/* DoFull();
 *==========================================================================
 * Full or Shrink the window
 */
static void DoFull(_WORD id)
{
	GRECT curr;
	GRECT full;
	GRECT prev;

	if (id == wid)
	{

		wind_get_grect(id, WF_CURRXYWH, &curr);
		wind_get_grect(id, WF_FULLXYWH, &full);

		if (rc_equal(&curr, &full))
		{
			wind_get_grect(id, WF_PREVXYWH, &prev);
			graf_shrinkbox(prev.g_x, prev.g_y, prev.g_w, prev.g_h, full.g_x, full.g_y, full.g_w, full.g_h);
			wind_set_grect(id, WF_CURRXYWH, &prev);
		} else
		{
			graf_growbox(curr.g_x, curr.g_y, curr.g_w, curr.g_h, full.g_x, full.g_y, full.g_w, full.g_h);
			wind_set_grect(id, WF_CURRXYWH, &full);
		}
	}
}


/* do_redraw()
 * ================================================================
 * Redraw the buttons
 */
static void do_redraw(OBJECT *tree, _WORD obj, GRECT *r, _WORD *msg)
{
	GRECT rect;

	wind_update(1);
	if (msg[3] == wid)
	{
		wind_get_grect(msg[3], WF_FTOOLBAR, &rect);
		while (rect.g_w && rect.g_h)
		{
			if (rc_intersect(r, &rect))
			{
				objc_draw(tree, obj, MAX_DEPTH, rect.g_x, rect.g_y, rect.g_w, rect.g_h);
			}
			wind_get_grect(msg[3], WF_NTOOLBAR, &rect);
		}
	}
	wind_update(0);
}



/* DoRedraw()
 * ================================================================
 * DoRedraw of the window...plain white...
 */
static void DoRedraw(_WORD msg[])
{
	GRECT r;
	GRECT rect;
	_WORD xy[4];

	r.g_x = msg[4];
	r.g_y = msg[5];
	r.g_w = msg[6];
	r.g_h = msg[7];

	wind_update(1);
	if (msg[3] == wid)
	{
		wind_get_grect(msg[3], WF_FIRSTXYWH, &rect);
		while (rect.g_w && rect.g_h)
		{
			if (rc_intersect(&r, &rect))
			{
				graf_mouse(M_OFF, NULL);
				xy[0] = rect.g_x;
				xy[1] = rect.g_y;
				xy[2] = rect.g_x + rect.g_w - 1;
				xy[3] = rect.g_y + rect.g_h - 1;
				vs_clip(vhandle, 1, xy);
				vswr_mode(vhandle, MD_REPLACE);

				if (cur_tree != ad_box3)
				{
					vsf_interior(vhandle, 2);
					vsf_style(vhandle, 1);
				} else
				{
					vsf_interior(vhandle, 0);
					vsf_style(vhandle, 1);
				}
				vsf_perimeter(vhandle, 0);
				v_bar(vhandle, xy);
				graf_mouse(M_ON, NULL);
			}
			wind_get_grect(msg[3], WF_NEXTXYWH, &rect);
		}
	}
	wind_update(0);
}

/* DoSizer()
 * ================================================================
 */
static void DoSizer(_WORD msg[])
{
	GRECT rect;

	if (msg[3] == wid)
	{
		wind_set_grect(msg[3], WF_CURRXYWH, (GRECT *)&msg[4]);
		wind_get_grect(msg[3], WF_PREVXYWH, &rect);
		if ((rect.g_w > msg[6] && rect.g_h >= msg[7]) || (rect.g_w >= msg[6] && rect.g_h > msg[7]))
			DoRedraw(msg);
	}
}


/* DoFonts
 * ================================================================
 * This is a Dialog Box that will test Drop Down List Menus
 */
static void DoFonts(_WORD *msg)
{
	OBJECT *tree;
	GRECT box;
	_WORD flag;

	wind_update(BEG_UPDATE);


	/* Put the Current Font Text String into the Button */
	tree = ad_font2;
	textptr = tree[CurFont2].ob_spec.free_string;
	strncpy(&tbuff[0], &textptr[1], 28);

	tree = ad_box3;
	tree[FBUTT1].ob_spec.tedinfo->te_ptext = &tbuff[0];


	/* SELECT and REDRAW the BUTTON */
	ad_box3[FBUTT1].ob_state |= OS_SELECTED;
	objc_xywh(ad_box3, FBUTT1, &box);
	do_redraw(ad_box3, FBUTT1, &box, msg);

	box.g_x = ((box.g_x + 7) / 8) * 8;

	/* Set up the Data Structure for a Drop Down List Menu */
	Menu.mn_tree = ad_font2;
	Menu.mn_menu = F2MENU;
	Menu.mn_item = CurFont2;
	Menu.mn_scroll = -1;				/* <---------DROP DOWN LIST (-1) */
	flag = menu_popup(&Menu, box.g_x, box.g_y + box.g_h + 1, &MData);
	if (flag)
	{
		menu_icheck(ad_font2, CurFont2, 0);	/* Turn off CheckMark on Old item */
		menu_icheck(ad_font2, MData.mn_item, 1);	/* Turn on checkMark on NEW item */
		CurFont2 = MData.mn_item;		/* Update Variable */
		menu_istart(1, ad_font2, F2MENU, CurFonts);	/* Reset starting value in menu  */

		tree = ad_font2;			/* Change the string in the button */
		textptr = tree[CurFont2].ob_spec.free_string;
		strncpy(&tbuff[0], &textptr[1], 28);

		tree = ad_box3;
		tree[FBUTT1].ob_spec.tedinfo->te_ptext = &tbuff[0];
	}
	/* Deselect the button */
	ad_box3[FBUTT1].ob_state &= ~OS_SELECTED;

	objc_xywh(ad_box3, FBUTT1, &box);
	box.g_x -= 2;
	box.g_y -= 2;
	box.g_w += 4;
	box.g_h += 4;
	do_redraw(ad_box3, FBUTT1, &box, msg);

	wind_update(END_UPDATE);
}



/* open_vwork();
 *==========================================================================
 * Open the Virtual Workstation
 */
static _WORD open_vwork(void)
{
	_WORD i;

	for (i = 0; i < 10; work_in[i++] = 1)
		;
	work_in[10] = 2;					/* raster coordinates */
	vhandle = phys_handle;
	v_opnvwk(work_in, &vhandle, work_out);
	xres = work_out[0];
	yres = work_out[1];

	vq_extnd(vhandle, 1, work_out);
	nplanes = work_out[4];

	return (vhandle);
}


/* close_vwork();
 *==========================================================================
 * Close the virtual workstation
 */
static void close_vwork(void)
{
	if (vhandle != -1)
	{
		v_clsvwk(vhandle);
		vhandle = -1;
	}
}



/* InitObjects()
 * ================================================================
 * Set the color of the trees to WHITE if we have less than 16 colors
 */
static void InitObjects(void)
{
	OBJECT *tree;

	if (nplanes < 4)
	{
		tree = ad_tools;

		tree[ROOT].ob_spec.obspec.interiorcol = G_WHITE;
		tree[B1B1].ob_spec.obspec.interiorcol = G_WHITE;
		tree[B1B2].ob_spec.obspec.interiorcol = G_WHITE;
		tree[B1B3].ob_spec.obspec.interiorcol = G_WHITE;
		tree[B1B4].ob_spec.obspec.interiorcol = G_WHITE;
		tree[B1B5].ob_spec.obspec.interiorcol = G_WHITE;

		tree = ad_box2;
		tree[ROOT].ob_spec.obspec.interiorcol = G_WHITE;
		tree[T2B1].ob_spec.obspec.interiorcol = G_WHITE;
		tree[T2B2].ob_spec.obspec.interiorcol = G_WHITE;
		tree[T2B3].ob_spec.obspec.interiorcol = G_WHITE;
		tree[T2B4].ob_spec.obspec.interiorcol = G_WHITE;
		tree[T2B5].ob_spec.obspec.interiorcol = G_WHITE;

		tree = ad_box3;
		tree[ROOT].ob_spec.obspec.interiorcol = G_WHITE;
		tree[T3BASE1].ob_spec.obspec.interiorcol = G_WHITE;
		tree[T3BASE2].ob_spec.obspec.interiorcol = G_WHITE;
	}
}


/* MAIN()
 * ================================================================
 */
int main(void)
{
	_WORD button;
	int done;
	OBJECT *tree;
	_WORD msg[8];							/* Evnt_mesag()        */

	appl_init();

	phys_handle = graf_handle(&gl_wchar, &gl_hchar, &gl_wbox, &gl_hbox);

	open_vwork();

	graf_mouse(ARROW, NULL);

	rsrc_load("toolbar.rsc");

	rsrc_gaddr(R_TREE, MAINMENU, &ad_menubar);
	rsrc_gaddr(R_TREE, ATREE, &ad_tree);
	rsrc_gaddr(R_TREE, FONTTREE, &ad_fonts);
	rsrc_gaddr(R_TREE, STREE, &ad_style);
	rsrc_gaddr(R_TREE, POSTREE, &ad_position);
	rsrc_gaddr(R_TREE, CTREE, &ad_color);
	rsrc_gaddr(R_TREE, PTREE, &ad_pattern);
	rsrc_gaddr(R_TREE, MTREE, &ad_modem);

	rsrc_gaddr(R_TREE, BAUDRATE, &ad_baudrate);
	rsrc_gaddr(R_TREE, PARTREE, &ad_parity);
	rsrc_gaddr(R_TREE, BITTREE, &ad_bittree);
	rsrc_gaddr(R_TREE, STOPTREE, &ad_stoptree);
	rsrc_gaddr(R_TREE, PORTTREE, &ad_porttree);
	rsrc_gaddr(R_TREE, FLOWTREE, &ad_flowtree);

	rsrc_gaddr(R_TREE, LTREE, &ad_list);
	rsrc_gaddr(R_TREE, TOOLBOX, &ad_tools);
	rsrc_gaddr(R_TREE, BLANK, &ad_blank);
	rsrc_gaddr(R_TREE, TOOLBOX2, &ad_box2);
	rsrc_gaddr(R_TREE, TOOLBAR, &ad_box3);

	rsrc_gaddr(R_TREE, FONT2, &ad_font2);


	/* Attach all Submenus that are being attached */
	AttachMenus();


	/* CheckMark the Current Menu Items in their respective menus */
	menu_icheck(ad_baudrate, CurBaudRate, 1);
	menu_icheck(ad_parity, CurParity, 1);
	menu_icheck(ad_bittree, CurBits, 1);
	menu_icheck(ad_stoptree, CurStopBits, 1);
	menu_icheck(ad_porttree, CurPort, 1);
	menu_icheck(ad_flowtree, CurFlow, 1);

	menu_icheck(ad_style, CurStyle, 1);
	menu_icheck(ad_position, CurPos, 1);
	menu_icheck(ad_fonts, CurFonts, 1);
	menu_icheck(ad_font2, CurFont2, 1);

	/* display menubar stuff here */
	menu_bar(ad_menubar, TRUE);

	/* initialize windows */
	wind_get_grect(0, WF_FULLXYWH, &desk);
	cur_tree = ad_tools;
	menu_flag = TRUE;					/* Set Toggle Flags */
	SubFlag = TRUE;
	ToolFlag = TRUE;

	CurRect.g_x = desk.g_x;
	CurRect.g_y = desk.g_y;
	CurRect.g_w = 320;
	CurRect.g_h = 200;

	InitObjects();

	InitWindow();
	menu_ienable(ad_menubar, FOPEN, 0);
	menu_ienable(ad_menubar, FCLOSE, 1);



	done = FALSE;
	do
	{

		evnt_mesag(msg);

		wind_update(BEG_UPDATE);

		if (msg[0] == MN_SELECTED)
		{
			/* msg[7] is the parent of FQUIT - which the user can't know */
			ptr = (OBJECT **) & msg[5];
			if (*ptr == ad_menubar)
			{
				switch (msg[4])
				{
				case FQUIT:
					button = form_alert(1, "[1][ |  EXIT PROGRAM? ][OK|Cancel]");
					if (button == 1)
						done = TRUE;
					break;

				case ABOUTX:
					execform(ad_tree, 0);
					break;

				case PHONE:
					do_modem();
					break;

				case TABOUT:			/* Enable, Disable About PLUS change TEXT */
					menu_flag ^= 1;
					menu_ienable(ad_menubar, ABOUTX, menu_flag);
					menu_text(ad_menubar, TABOUT, TextAbout[menu_flag]);

					if (menu_flag)
						menu_text(ad_menubar, ABOUTX, "  About Demo...     ");
					else
						menu_text(ad_menubar, ABOUTX, "  Disabled...       ");
					break;

				case TSUB:				/* Enable/Disable all Submenus */
					SubFlag ^= 1;
					menu_text(ad_menubar, TSUB, TextSubMenu[SubFlag]);
					if (SubFlag)
						AttachMenus();
					else
						DetachMenus();
					break;

				case SLISTS:
					DoList();			/* Do A Drop Down List Dialog */
					break;

				case TOOLFLAG:			/* Enable/Disable ToolBox */
					ToolFlag ^= 1;
					menu_text(ad_menubar, TOOLFLAG, TextToolBox[ToolFlag]);
					if (ToolFlag)
					{
						wind_set_ptr(wid, WF_TOOLBAR, cur_tree);
					} else
					{
						wind_set_ptr(wid, WF_TOOLBAR, NULL);
					}
					break;

				case SWITCH:			/* Switch ToolBoxes */
					if (cur_tree == ad_tools)
					{
						cur_tree = ad_box2;
					} else if (cur_tree == ad_box2)
					{
						cur_tree = ad_box3;

						tree = ad_fonts;
						textptr = tree[CurFonts].ob_spec.free_string;
						strncpy(&tbuff[0], &textptr[1], 28);

						tree = ad_box3;
						tree[FBUTT1].ob_spec.tedinfo->te_ptext = &tbuff[0];
					} else
					{
						cur_tree = ad_tools;
					}
					ToolFlag = TRUE;
					menu_text(ad_menubar, TOOLFLAG, TextToolBox[ToolFlag]);
					wind_set_ptr(wid, WF_TOOLBAR, cur_tree);
					break;

				case FCLOSE:			/* close Window */
					if (wid)
					{
						wind_get_grect(wid, WF_CURRXYWH, &CurRect);
						wind_close(wid);
						wind_delete(wid);
						wid = 0;
						menu_ienable(ad_menubar, FCLOSE, 0);
						menu_ienable(ad_menubar, FOPEN, 1);
					}
					break;

				case FOPEN:			/* open Window */
					if (!wid)
					{
						InitWindow();
						menu_ienable(ad_menubar, FOPEN, 0);
						menu_ienable(ad_menubar, FCLOSE, 1);
					}
					break;

				default:
					break;
				}
			}


			/* MENU SELECTED -> Font Style Menu Clicked on as a SUBMENU from the Menubar */
			if (*ptr == ad_style)
			{
				menu_icheck(ad_style, CurStyle, 0);	/* Turn OFF Old Checkmark */
				menu_icheck(ad_style, msg[4], 1);	/* Turn ON New CheckMark  */
				CurStyle = msg[4];		/* Update Current Var     */
				menu_istart(1, ad_style, ROOT, CurStyle);	/* Reset Starting Position */
			}


			/* MENU SELECTED -> Font Position Clicked on as a SUBMENU from the menubar */
			if (*ptr == ad_position)
			{
				menu_icheck(ad_position, CurPos, 0);
				menu_icheck(ad_position, msg[4], 1);
				CurPos = msg[4];
				menu_istart(1, ad_position, ROOT, CurPos);
			}


			/* MENU SELECTED -> Fonts Menu Clicked on as a SUBMENU from the menubar */
			if (*ptr == ad_fonts)
			{
				menu_icheck(ad_fonts, CurFonts, 0);
				menu_icheck(ad_fonts, msg[4], 1);
				CurFonts = msg[4];
				menu_istart(1, ad_fonts, ROOT, CurFonts);
			}

			menu_tnormal(ad_menubar, msg[3], TRUE);
		}

		if (msg[0] != MN_SELECTED)
		{
			switch (msg[0])
			{
			case WM_FULLED:
				DoFull(msg[3]);
				break;

			case WM_REDRAW:
				DoRedraw(msg);
				break;

			case WM_ARROWED:
			case WM_HSLID:
			case WM_VSLID:
				break;

			case WM_MOVED:
				if (msg[3] == wid)
				{
					wind_set_grect(wid, WF_CURRXYWH, (GRECT *)&msg[4]);
				}
				break;

			case WM_TOPPED:
				if (msg[3] == wid)
				{
					wind_set_int(wid, WF_TOP, 0);
				}
				break;

			case WM_CLOSED:
				if (msg[3] == wid)
				{
					wind_get_grect(wid, WF_CURRXYWH, &CurRect);

					wind_close(wid);
					wind_delete(wid);
					wid = 0;
					menu_ienable(ad_menubar, FCLOSE, 0);
					menu_ienable(ad_menubar, FOPEN, 1);
				}
				break;

			case WM_SIZED:
				DoSizer(msg);
				break;

			case WM_TOOLBAR:
				if (msg[3] == wid)
				{
					/* Button Handling for ToolBox #1 */
					button = -1;
					if (cur_tree == ad_tools)
					{
						switch (msg[4])
						{
						case T1B1:
							button = B1B1;
							break;
						case T1B2:
							button = B1B2;
							break;
						case T1B3:
							button = B1B3;
							break;
						case T1B4:
							button = B1B4;
							break;
						case T1B5:
							button = B1B5;
							break;
						}
					}

					/* Button Handling for TOOLBOX 2 */
					if (cur_tree == ad_box2)
					{
						switch (msg[4])
						{
						case T2I1:
							button = T2B1;
							break;
						case T2I2:
							button = T2B2;
							break;
						case T2I3:
							button = T2B3;
							break;
						case T2I4:
							button = T2B4;
							break;
						case T2I5:
							button = T2B5;
							break;
						}
					}

					/* Word Processing Tree */
					if (cur_tree == ad_box3)
					{
						switch (msg[4])
						{
						case FBUTT1:
						case FBUTT2:
							DoFonts(msg);
							break;

						default:
							break;
						}
					}


					if (cur_tree != ad_box3 && button > 0)
					{
						GRECT r;

						if (cur_tree[button].ob_state & OS_SELECTED)
							cur_tree[button].ob_state &= ~OS_SELECTED;
						else
							cur_tree[button].ob_state |= OS_SELECTED;

						objc_offset(cur_tree, button, &r.g_x, &r.g_y);
						r.g_x -= 2;
						r.g_y -= 2;
						r.g_w = cur_tree[button].ob_width + 4;
						r.g_h = cur_tree[button].ob_height + 4;
						do_redraw(cur_tree, button, &r, msg);
					}

				}
				break;

			case WM_ICONIFY:
				if (msg[3] == wid)
					wind_set_grect(msg[3], WF_ICONIFY, (GRECT *)&msg[4]);
				break;

			case WM_UNICONIFY:
				if (msg[3] == wid)
					wind_set_grect(msg[3], WF_UNICONIFY, (GRECT *)&msg[4]);
				break;

			default:
				break;
			}
		}
		wind_update(END_UPDATE);

	} while (!done);
	if (wid > 0)
		wind_delete(wid);

	menu_bar(ad_menubar, FALSE);

	graf_mouse(ARROW, NULL);

	rsrc_free();

	close_vwork();
	appl_exit();

	return 0;
}
