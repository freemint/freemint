/*
 * HypView - (c) 2001 - 2006 Philipp Donze
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

#ifndef _diallib_h
#define _diallib_h

#define	ON                      1
#define	OFF                     0
#define	YES                     ON
#define	NO                      OFF

#define	GERMAN                  1
#define	ENGLISH                 2
#define	FRENCH                  3

#include "dl_user.h"            /* Application specific definitions */

#include RESOURCE_HEADER_FILE   /* Include resource header file */

#include "dl_miss.h"            /* Include missing type definitions */

#define	USE_ITEM       (USE_DIALOG|USE_WINDOW|USE_FILESELECTOR|USE_FONTSELECTOR)

/*	Window types	*/
#define	WIN_WINDOW  1
#define	WIN_DIALOG  2
#define	WIN_FILESEL 3
#define	WIN_FONTSEL 4
#define	WIN_PRINTER 5

/*	USER_DATA status flags	*/
#define	WIS_OPEN        0x01
#define	WIS_ICONIFY     0x02
#define	WIS_ALLICONIFY  0x04
#define	WIS_FULL        0x10
#define WIS_MFCLOSE     0x20	/* Marked for CLOSE */


/*	Window messages	*/
#define	WIND_INIT                -1
#define	WIND_OPEN                -2
#define	WIND_OPENSIZE            -3
#define	WIND_EXIT                -4
#define	WIND_CLOSE               -5

#define	WIND_KEYPRESS           -10
#define	WIND_CLICK              -11

#define	WIND_REDRAW             -20
#define	WIND_SIZED              -21
#define	WIND_MOVED              -22
#define	WIND_TOPPED             -23
#define	WIND_NEWTOP             -24
#define	WIND_UNTOPPED           -25
#define	WIND_ONTOP              -26
#define	WIND_BOTTOM             -27
#define	WIND_FULLED             -28

#define	WIND_DRAGDROPFORMAT		-30
#define	WIND_DRAGDROP           -31

#define	WIND_ICONIFY            -40
#define	WIND_UNICONIFY          -41
#define	WIND_ALLICONIFY         -42

#define	WIND_BUBBLE             -50

/*	Toolbar messages	*/
#define	WIND_TBCLICK            -60
#define	WIND_TBUPDATE           -61

/*	File maximum	*/
#define	DL_PATHMAX              256
#define	DL_NAMEMAX               64

/*	AV protokol flags	*/
#define	AV_P_VA_SETSTATUS       0x01
#define	AV_P_VA_START           0x02
#define	AV_P_AV_STARTED         0x04
#define	AV_P_VA_FONTCHANGED     0x08
#define	AV_P_QUOTING            0x10
#define	AV_P_VA_PATH_UPDATE     0x20

typedef struct _chain_data_
{
	struct _chain_data_	*next;
	struct _chain_data_	*previous;
	short	type;
	short	whandle;
	short	status;
	GRECT last;
}CHAIN_DATA;

typedef struct _dialog_data_
{
	struct _dialog_data_	*next;
	struct _dialog_data_	*previous;
	short	type;
	short	whandle;
	short	status;
	GRECT last;
	DIALOG *dial;
	OBJECT *obj;
	char	*title;
}DIALOG_DATA;


#if USE_LOGICALRASTER == YES
	#define	WP_UNIT	long
#else
	#define	WP_UNIT	short
#endif	

struct _window_data_;
typedef	short (*HNDL_WIN)(struct _window_data_ *wptr, short obj, void *data);

typedef struct _window_data_
{
	struct _window_data_	*next;
	struct _window_data_	*previous;
	short	type;
	short	whandle;
	short	status;
	GRECT last;
	GRECT full;
	GRECT work;
	HNDL_WIN proc;
	char	*title;
	short	kind;
#if OPEN_VDI_WORKSTATION == YES
	short	vdi_handle;
	short	workout[57];
	short	ext_workout[57];
#endif
	WP_UNIT	max_width;
	WP_UNIT	max_height;
	WP_UNIT	x_pos;
	WP_UNIT	y_pos;
	short	x_speed;
	short	y_speed;
	short	xpos_res, ypos_res;
#if USE_LOGICALRASTER==YES
	short	x_raster;
	short	y_raster;
#endif
	short	font_cw;
	short	font_ch;
#if USE_TOOLBAR==YES
	OBJECT *toolbar;
	short	x_offset;
	short	y_offset;
#endif
	void	*data;
}WINDOW_DATA;

struct _filesel_data_;
typedef	void (*HNDL_FSL)(struct _filesel_data_ *fslx,short nfiles);

typedef struct _filesel_data_
{
	struct _filesel_data_	*next;
	struct _filesel_data_	*previous;
	short	type;
	short	whandle;
	short	status;
	GRECT last;
	HNDL_FSL proc;
	void *dialog;
	char path[DL_PATHMAX];
	char name[DL_NAMEMAX];
	short	button;
	short	sort_mode;
	short	nfiles;
	void *data;
}FILESEL_DATA;

struct _fontsel_data_;
typedef	short (*HNDL_FONT)(struct _fontsel_data_ *fnts_data);

typedef struct _fontsel_data_
{
	struct _fontsel_data_	*next;
	struct _fontsel_data_	*previous;
	short	type;
	short	whandle;
	short	status;
	GRECT last;
	HNDL_FONT proc;
	FNT_DIALOG *dialog;
	short	font_flag;
	char 	*opt_button;
	short	button,check_boxes;
	long	id,pt,ratio;
	short	vdi_handle;
	void *data;
}FONTSEL_DATA;

/*
 *		DL_INIT.C
 */
extern	short	ap_id;
extern	short	aes_handle,pwchar,phchar,pwbox,phbox;
extern	short	has_agi,has_wlffp,has_iconify;
#if USE_GLOBAL_VDI == YES
extern	short	vdi_handle;
extern	short	workin[11];
extern	short	workout[57];
extern	short	ext_workout[57];
#if SAVE_COLORS==YES
extern	RGB1000 save_palette[256];
#endif
#endif
extern	char	*resource_file,*err_loading_rsc;
extern	RSHDR	*rsh;
extern	OBJECT	**tree_addr;
extern	short	tree_count;
extern	char	**string_addr;
#if USE_MENU == YES
extern	OBJECT	*menu_tree;
#endif
extern	KEYTAB *key_table;

short DoAesInit(void);
short DoInitSystem(void);
void DoExitSystem(void);

/*
 *		DL_EVENT.C
 */
extern	short	doneFlag;
extern	short	quitApp;
void DoEventDispatch(EVNT *event);
void DoEvent(void);

/*
 *		DL_ITEMS.C
 */
extern	char	*iconified_name;
extern	OBJECT	*iconified_tree;
extern	short	iconified_icon;
extern	CHAIN_DATA	*iconified_list[MAX_ICONIFY_PLACE];
extern	short	iconified_count;
extern	CHAIN_DATA	*all_list;
extern	short	modal_items;
extern	short	modal_stack[MAX_MODALRECURSION];

void add_item(CHAIN_DATA *item);
void remove_item(CHAIN_DATA *item);
void FlipIconify(void);
void AllIconify(short handle, GRECT *r);
void CycleItems(void);
void RemoveItems(void);
void ModalItem(void);
void ItemEvent(EVNT *event);
CHAIN_DATA *find_ptr_by_whandle(short handle);
CHAIN_DATA *find_ptr_by_status(short mask, short status);


/*
 *		DL_MENU.C
 */
void ChooseMenu(short title, short entry);

/*
 *		DL_DIAL.C
 */
DIALOG *OpenDialog(HNDL_OBJ proc, OBJECT *tree, char *title, short x, short y);
void SendCloseDialog(DIALOG *dial);
void CloseDialog(DIALOG_DATA *ptr);
void CloseAllDialogs(void);
void RemoveDialog(DIALOG_DATA *ptr);

void DialogEvents(DIALOG_DATA *ptr,EVNT *event);
void SpecialMessageEvents(DIALOG *dialog,EVNT *event);

void dialog_iconify(DIALOG *dialog, GRECT *r);
void dialog_uniconify(DIALOG *dialog, GRECT *r);

DIALOG_DATA *find_dialog_by_obj(OBJECT *tree);
DIALOG_DATA *find_dialog_by_whandle(short handle);

/*
 *		DL_WIN.C
 */
WINDOW_DATA *OpenWindow(HNDL_WIN proc, short kind, char *title, 
					WP_UNIT max_w, WP_UNIT max_h,void *user_data);
void CloseWindow(WINDOW_DATA *ptr);
void CloseAllWindows(void);
void RemoveWindow(WINDOW_DATA *ptr);
short ScrollWindow(WINDOW_DATA *ptr, short *rel_x, short *rel_y);
void WindowEvents(WINDOW_DATA *ptr, EVNT *event);
void SetWindowSlider(WINDOW_DATA *ptr);
void ResizeWindow(WINDOW_DATA *ptr, WP_UNIT max_w, WP_UNIT max_h);
void IconifyWindow(WINDOW_DATA *ptr,GRECT *r);
void UniconifyWindow(WINDOW_DATA *ptr);
void DrawToolbar(WINDOW_DATA *win);
WINDOW_DATA *find_openwindow_by_whandle(short handle);
WINDOW_DATA *find_window_by_whandle(short handle);
WINDOW_DATA *find_window_by_proc(HNDL_WIN proc);
WINDOW_DATA *find_window_by_data(void *data);
short count_window(void);

/*
 *		DL_FILSL.C
 */
void *OpenFileselector(HNDL_FSL proc,char *comment,char *filepath,char *path,char *pattern,short mode);
void FileselectorEvents(FILESEL_DATA *ptr,EVNT *event);
void RemoveFileselector(FILESEL_DATA *ptr);

/*
 *		DL_FONSL.C
 */
extern char fnts_std_text[80];
FONTSEL_DATA *CreateFontselector(HNDL_FONT proc,short font_flag,char *sample_text,char *opt_button);
short OpenFontselector(FONTSEL_DATA *ptr,short button_flag,long id,long pt,long ratio);
void CloseFontselector(FONTSEL_DATA *ptr);
void RemoveFontselector(FONTSEL_DATA *ptr);
void FontselectorEvents(FONTSEL_DATA *ptr,EVNT *event);

/*
 *		DL_AV.C
 */
#if USE_AV_PROTOCOL != NO
void DoVA_START(short msg[8]);			/*	minimales AV-Protokoll	*/

#if USE_AV_PROTOCOL >= 2				/*	normales/maximales AV-Protokoll	*/

extern short server_id;					/*	Programm ID des Servers	*/
extern long server_cfg;

void DoVA_PROTOSTATUS(short msg[8]);
void DoAV_PROTOKOLL(short flags);
void DoAV_EXIT(void);
#endif

#endif

/*
 *		DL_AVCMD.C
 */
void DoAV_GETSTATUS(void);
void DoAV_STATUS(char *string);
void DoAV_SENDKEY(short kbd_state, short code);
void DoAV_ASKFILEFONT(void);
void DoAV_ASKCONFONT(void);
void DoAV_ASKOBJECT(void);
void DoAV_OPENCONSOLE(void);
void DoAV_OPENWIND(char *path, char *wildcard);
void DoAV_STARTPROG(char *path, char *commandline, short id);
void DoAV_ACCWINDOPEN(short handle);
void DoAV_ACCWINDCLOSED(short handle);
void DoAV_COPY_DRAGGED(short kbd_state, char *path);
void DoAV_PATH_UPDATE(char *path);
void DoAV_WHAT_IZIT(short x,short y);
void DoAV_DRAG_ON_WINDOW(short x,short y, short kbd_state, char *path);
void DoAV_STARTED(char *ptr);
void DoAV_XWIND(char *path, char *wild_card, short bits);
void DoAV_VIEW(char *path);
void DoAV_FILEINFO(char *path);
void DoAV_COPYFILE(char *file_list, char *dest_path,short bits);
void DoAV_DELFILE(char *file_list);
void DoAV_SETWINDPOS(short x,short y,short w,short h);
void DoAV_SENDCLICK(EVNTDATA *mouse, short ev_return);

/*
 *		DL_DRAG.C
 */
void DragDrop(short msg[]);

/*
 *		DL_BUBBL.C
 */
void DoInitBubble(void);
void DoExitBubble(void);
void Bubble(short mx,short my);

/*
 *		DL_HELP.C
 */
void STGuideHelp(void);

/*
 *		DL_LEDIT.C
 */
void DoInitLongEdit(void);
void DoExitLongEdit(void);

/*
 *		DL_CFGRD.C
 */
short CfgOpenFile(char *path);
void CfgCloseFile(void);
void CfgWriteFile(char *path);
char *CfgGetLine(char *src);
char *CfgGetVariable(char *variable);
char *CfgExtractVarPart(char *start,char separator,short num);
short CfgSaveMemory(long len);
short CfgSetLine(char *line);
short CfgSetVariable(char *name,char *value);

/*
 *		DL_DHST.C
 */
void DhstAddFile(char *path);
void DhstFree(short msg[]);

/*
 *		DL_ROUTS.C
 */
void ConvertKeypress(short *key,short *kstate);
void CopyMaximumChars(OBJECT *obj,char *str);
char *ParseData(char *start);
short rc_intersect_my(GRECT *p1,GRECT *p2);
void Debug(const char *str,...);

#ifdef GENERATE_DIAGS

#define DIAG(x) display x
#define NDIAG(x) ndisplay x

void display(const char *fmt, ...);
void ndisplay(const char *fmt, ...);

#else

#define DIAG(x)
#define NDIAG(x)

#endif

void KeyboardOnOff(void);

/*
 *		DL_USER.C
 */
typedef struct
{
	short	tree;
	short	obj;
	short	len;
	XTED	xted;
}LONG_EDIT;

extern LONG_EDIT long_edit[];
extern short long_edit_count;

short DoUserKeybd(short kstate, short scan, short ascii);
void DoButton(EVNT *event);
void DoUserEvents(EVNT *event);
void SelectMenu(short title, short entry);
void DD_Object(DIALOG *dial,GRECT *rect,OBJECT *tree,short obj, char *data, unsigned long format);
void DD_DialogGetFormat(OBJECT *tree,short obj, unsigned long format[]);
char *GetTopic(void);
void DoVA_START(short msg[8]);
void DoVA_Message(short msg[8]);


#endif       /* _diallib_h */
