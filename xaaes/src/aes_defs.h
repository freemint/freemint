/*
 * XaAES - XaAES Ain't the AES
 *
 * A multitasking AES replacement for MiNT
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
 * GEM AES Definitions
 */

#ifndef _aes_defs_h
#define _aes_defs_h

#include "global.h"


/****** Event definitions ***********************************************/

enum message_types
{
	MN_SELECTED	= 10,
	WM_REDRAW	= 20,
	WM_TOPPED,
	WM_CLOSED,
	WM_FULLED,
	WM_ARROWED,
	WM_HSLID,
	WM_VSLID,
	WM_SIZED,
	WM_MOVED,
	WM_NEWTOP,
	WM_UNTOPPED	= 30,		/* GEM  2.x   */
	WM_ONTOP,    			/* AES 4.0    */
	WM_UNKNOWN,			/* AES 4.1    */
	WM_BACK,			/* WINX       */
	WM_BOTTOMED = WM_BACK,		/* AES 4.1    */
	WM_ICONIFY,			/* AES 4.1    */
	WM_UNICONIFY,			/* AES 4.1    */
	WM_ALLICONIFY,			/* AES 4.1    */
	WM_TOOLBAR,			/* AES 4.1    */
	AC_OPEN		= 40,
	AC_CLOSE,
	CT_UPDATE	= 50,
	CT_MOVE,
	CT_NEWTOP,
	CT_KEY,
	AP_TERM		= 50,
	AP_TFAIL,
	AP_AESTERM,			/* AES 4.1    */
	AP_RESCHG	= 57,
	SHUT_COMPLETED	= 60,
	RESCH_COMPLETED,
	AP_DRAGDROP	= 63,
	SH_WDRAW	= 72,		/* MultiTOS   */
	SC_CHANGED	= 80,		/* MultiTOS   */
	PRN_CHANGED	= 82,
	FNT_CHANGED,
	THR_EXIT	= 88,		/* MagiC 4.5  */
	PA_EXIT,			/* MagiC 3    */
	CH_EXIT		= 90,		/* MultiTOS   */
	WM_M_BDROPPED	= 100,		/* KAOS 1.4   */
	SM_M_SPECIAL,			/* MAG!X      */
	SM_M_RES2,			/* MAG!X      */
	SM_M_RES3,			/* MAG!X      */
	SM_M_RES4,			/* MAG!X      */
	SM_M_RES5,			/* MAG!X      */
	SM_M_RES6,			/* MAG!X      */
	SM_M_RES7,			/* MAG!X      */
	SM_M_RES8,			/* MAG!X      */
	SM_M_RES9,			/* MAG!X      */
#if 0
	XA_M_CNF	= 200,		/* XaAES  scl */
	XA_M_SCL,
	XA_M_OPT,
	XA_M_GETSYM,
	XA_M_DESK,
	XA_M_EXEC	= 250,
	XA_M_OK		= 300,
#endif
	WM_SHADED	= 22360,	/* [WM_SHADED apid 0 win 0 0 0 0] */
	WM_UNSHADED	= 22361		/* [WM_UNSHADED apid 0 win 0 0 0 0] */
};


/****** Application definitions *****************************************/

/* Keybord states */
enum k_state
{
	K_RSHIFT	= 0x0001,
	K_LSHIFT	= 0x0002,
	K_CTRL  	= 0x0004,
	K_ALT   	= 0x0008
};


/****** Object definitions **********************************************/

/* inside fill patterns */
#define IP_HOLLOW	0
#define IP_1PATT	1
#define IP_2PATT	2
#define IP_3PATT	3
#define IP_4PATT	4
#define IP_5PATT	5
#define IP_6PATT	6
#define IP_SOLID	7

enum object_types
{
	G_BOX		= 20,
	G_TEXT,
	G_BOXTEXT,
	G_IMAGE,
	G_USERDEF,
	G_PROGDEF	= G_USERDEF,
	G_IBOX,
	G_BUTTON,
	G_BOXCHAR,
	G_STRING,
	G_FTEXT,
	G_FBOXTEXT,
	G_ICON,
	G_TITLE,
	G_CICON,

	/* for completeness */
	G_SWBUTTON,			/* MAG!X */
	G_POPUP,			/* MAG!X */
	G_WINTITLE,			/* MagiC 3.1 */
	G_EDIT,
	G_SHORTCUT,			/* MagiC 6 */
	G_SLIST,			/* XaAES extended object - scrolling list */
	G_UNKNOWN,
	G_MAX				/* Maximum number of object types */
};

/* Object flags */
enum object_flags
{
	NONE	  = 0x0000,
	SELECTABLE= 0x0001,
	DEFAULT	  = 0x0002,
	EXIT      = 0x0004,
	EDITABLE  = 0x0008,
	RBUTTON	  = 0x0010,
	LASTOB    = 0x0020,
	TOUCHEXIT = 0x0040,
	HIDETREE  = 0x0080,
	INDIRECT  = 0x0100,
	FLD3DIND  = 0x0200,		/* 3D Indicator  AES 4.0 */
	FLD3DBAK  = 0x0400,		/* 3D Background AES 4.0 */
	FLD3DACT  = 0x0600,		/* 3D Activator  AES 4.0 */
	FLD3DMASK = 0x0600,
	FLD3DANY  = 0x0600,
	SUBMENU   = 0x0800
};

/* Object states */
enum object_states
{
	NORMAL   = 0x00,
	SELECTED = 0x01,
	CROSSED  = 0x02,
	CHECKED  = 0x04,
	DISABLED = 0x08,
	OUTLINED = 0x10,
	SHADOWED = 0x20,
	WHITEBAK = 0x40,		/* TOS & MagiC */
	DRAW3D   = 0x80			/* GEM 2.x */
};
typedef enum object_states OB_STATE;

/* objc_sysvar */
enum object_sysvar
{
	SV_INQUIRE,
	SV_SET,
	LK3DIND     = 1,		/* AES 4.0   */
	LK3DACT,			/* AES 4.0   */
	INDBUTCOL,			/* AES 4.0   */
	ACTBUTCOL,			/* AES 4.0   */
	BACKGRCOL,			/* AES 4.0   */
	AD3DVAL,			/* AES 4.0   */
	MX_ENABLE3D = 10		/* MagiC 3.0 */
};

/* Object colors */
enum object_colors
{
	WHITE,
	BLACK,
	RED,
	GREEN,
	BLUE,
	CYAN,
	YELLOW,
	MAGENTA,
	LWHITE,
	LBLACK,
	LRED,
	LGREEN,
	LBLUE,
	LCYAN,
	LYELLOW,
	LMAGENTA
};

#define ROOT             0
#define MAX_LEN         81              /* max string length */
#define MAX_DEPTH       32              /* max depth of search or draw */

#define IBM             3               /* font types */
#define SMALL           5


/****** Form library definitions*****************************************/

/* Editable text field definitions */
enum ed_mode
{
	ED_START,
	EDSTART = ED_START,
	ED_INIT,
	EDINIT	= ED_INIT,
	ED_CHAR,
	EDCHAR	= ED_CHAR,
	ED_END,
	EDEND	= ED_END
};

/* editable text justification */
enum te_just
{
	TE_LEFT,
	TE_RIGHT,
	TE_CNTR
};


typedef struct { short x, y, w, h; } RECT;


/* Object structures */

/* From Atari Compendium - not sure if the bitfield stuff works properly with
   GNU or Pure C, but it's fine with Lattice */
/* HR: its perfectly good for Pure C. (But it *must* be 'int'
       so I parameterized this mode using bits & ubits */
struct objc_coloours
{
	ubits borderc	:4;
	ubits textc	:4;
	ubits opaque	:1;
	ubits pattern	:3;
	ubits fillc	:4;
};
typedef struct objc_coloours OBJC_COLOURS;

typedef struct
{
	char *te_ptext;
	char *te_ptmplt;
	char *te_pvalid;
	short te_font;
	short te_fontid;		/* AES 4.1 extension */
	short te_just;
	OBJC_COLOURS te_color;
	short te_fontsize;		/* AES 4.1 extension */
	short te_thickness;
	short te_txtlen;
	short te_tmplen;
} TEDINFO;

struct iconblk
{
	short *ib_pmask;
	short *ib_pdata;
	char  *ib_ptext;
	short  ib_char;
	short  ib_xchar;
	short  ib_ychar;
	RECT   ic;
	RECT   tx;
};
typedef struct iconblk ICONBLK;

struct cicon_data
{
        short num_planes;		/* Number of planes in the following data          */
        short *col_data;		/* Pointer to color bitmap in standard form        */
        short *col_mask;		/* Pointer to single plane mask of col_data        */
        short *sel_data;		/* Pointer to color bitmap of selected icon        */
        short *sel_mask;		/* Pointer to single plane mask of selected icon   */
        struct cicon_data *next_res;	/* Pointer to next icon for a different resolution */
};
typedef struct cicon_data CICON;


struct cicon_blk
{
        ICONBLK monoblk;
        CICON *mainlist;
};
typedef struct cicon_blk CICONBLK;

typedef struct
{
	short *bi_pdata;
	short  bi_wb;
	short  bi_hl;
	short  bi_x;
	short  bi_y;
	short  bi_color;
} BITBLK;

struct parmblk
{
	struct object *pb_tree;
	short pb_obj;
	short pb_prevstate;
	short pb_currstate;
	RECT r, c;
	long pb_parm;
};
typedef struct parmblk PARMBLK;


struct __parmblk
{
	int _cdecl (*ub_code)(PARMBLK *parmblock);
	long ub_parm;
};
typedef struct __parmblk USERBLK;

/* common used different naming */
struct parm_blk
{
	int _cdecl (*ab_code)(PARMBLK *parmblock);
	long ab_parm;
};
typedef struct parm_blk APPLBLK;

typedef struct
{
	unsigned char character;
	unsigned char framesize;
	OBJC_COLOURS colours;
} SPEC;

typedef struct
{
	unsigned character   :  8;
	signed   framesize   :  8;
	unsigned framecol    :  4;
	unsigned textcol     :  4;
	unsigned textmode    :  1;
	unsigned fillpattern :  3;
	unsigned interiorcol :  4;
} bfobspec;

union obspecptr
{
	long index;
	long lspec;
	union obspecptr *indirect;
	bfobspec obspec;
	SPEC this;
	TEDINFO *tedinfo;
	ICONBLK *iconblk;
	CICONBLK *ciconblk;
	BITBLK *bitblk;
	USERBLK *userblk;
	APPLBLK *appblk;
	struct scroll_info *listbox;
	char *free_string;
	char *string;
	void *v;
};
typedef union obspecptr OBSPEC;

struct object
{
	short ob_next, ob_head, ob_tail;
	unsigned short ob_type, ob_flags, ob_state;
	OBSPEC ob_spec;
	RECT r;
};
typedef struct object OBJECT;

typedef struct
{
	OBJECT *mn_tree;
	short mn_menu;
	short mn_item;
	short mn_scroll;
	short mn_keystate;
} MENU;

typedef struct
{
	long display;
	long drag;
	long delay;
	long speed;
	short height;
} MN_SET;

typedef struct
{	
	char *string;			/* etwa "TOS|KAOS|MAG!X" */	
	short num;			/* Nr. der aktuellen Zeichenkette */
	short maxnum;			/* maximal erlaubtes <num> */
} SWINFO;

typedef struct
{
	OBJECT *tree;			/* Popup-Menu */
	short obnum;			/* aktuelles Objekt von <tree> */
} POPINFO;


/****** Menu definitions ************************************************/

/* menu_bar modes */

/* Menu bar install/remove codes */
enum mbar_do
{
	MENU_INQUIRE =-1,
	MENU_HIDE,			/* TOS */
	MENU_REMOVE = MENU_HIDE,
	MENU_SHOW,			/* TOS */
	MENU_INSTALL = MENU_SHOW,
	MENU_INSTL = 100		/* MAG!X */
};

/* menu_attach codes */
#define ME_INQUIRE   0
#define ME_ATTACH    1
#define ME_REMOVE    2

/****** Form definitions ************************************************/

enum fmd_ty
{
	FMD_START,
	FMD_GROW,
	FMD_SHRINK,
	FMD_FINISH
};

/* form_xdo definitions */
struct scanx
{
	char scancode;
	char nclicks;
	short  objnr;
};

struct xdo_inf
{
	struct scanx *unsh;
	struct scanx *shift;
	struct scanx *ctrl;
 	struct scanx *alt;
	void *resvd;
};

struct xted
{
	char *xte_ptmplt;
	char *xte_pvalid;
	short xte_vislen;
	short xte_scroll;
};


/****** Graph definitions ************************************************/

/* Mouse forms */

enum mice
{
	ARROW,
	TEXT_CRSR,
	HOURGLASS,
	BUSYBEE		= HOURGLASS,
	BUSY_BEE	= HOURGLASS,
	POINT_HAND,
	FLAT_HAND,
	THIN_CROSS,
	THICK_CROSS,
	OUTLN_CROSS,
	USER_DEF	= 255,
	M_OFF,
	M_ON,
	M_SAVE,
	M_LAST,
	M_RESTORE,
	M_FORCE		= 0x8000,
	M_PUSH		= 100,
	M_POP
};

/* Mouse form definition block */
struct mfstr
{
	short mf_xhot;
	short mf_yhot;
	short mf_nplanes;
	short mf_fg;
	short mf_bg;
	short mf_mask[16];
	short mf_data[16];
};
typedef struct mfstr MFORM;



/****** Window definitions **********************************************/

enum wind_attr
{
	NAME		= 0x0001,
	CLOSE		= 0x0002,
	CLOSER		= CLOSE,
	FULL		= 0x0004,
	FULLER		= FULL,
	MOVE		= 0x0008,
	MOVER		= MOVE,
	INFO		= 0x0010,
	SIZE		= 0x0020,
	SIZER		= SIZE,
	UPARROW		= 0x0040,
	DNARROW		= 0x0080,
	VSLIDE		= 0x0100,
	LFARROW		= 0x0200,
	RTARROW		= 0x0400,
	HSLIDE		= 0x0800,
	MENUBAR		= 0x1000,	/* XaAES */
	HOTCLOSEBOX	= 0x1000,	/* GEM 2.x */
	BACKDROP	= 0x2000,	/* KAOS 1.4 */
	ICONIFY		= 0x4000,	/* iconifier */
	ICONIFIER	= ICONIFY,
	SMALLER		= ICONIFY
};

enum wind_code
{
	WF_KIND		= 1,
	WF_NAME,
	WF_INFO,
	WF_WXYWH,
	WF_WORKXYWH	= WF_WXYWH,
	WF_CXYWH,
	WF_CURRXYWH	= WF_CXYWH,
	WF_PXYWH,
	WF_PREVXYWH	= WF_PXYWH,
	WF_FXYWH,
	WF_FULLXYWH	= WF_FXYWH,
	WF_HSLIDE,
	WF_VSLIDE,
	WF_TOP,
	WF_FIRSTXYWH,
	WF_NEXTXYWH,
	WF_RESVD,
	WF_NEWDESK,
	WF_HSLSIZE,
	WF_HSLSIZ	= WF_HSLSIZE,
	WF_VSLSIZE,
	WF_VSLSIZ	= WF_VSLSIZE,
	WF_SCREEN,
	WF_COLOR,
	WF_DCOLOR,
	WF_OWNER,			/* AES 4 */
	WF_BEVENT	= 24,
	WF_BOTTOM,
	WF_ICONIFY,			/* AES 4.1 */
	WF_UNICONIFY,			/* AES 4.1 */
	WF_UNICONIFYXYWH,		/* AES 4.1 */
	WF_TOOLBAR	= 30,		/* compatible */
	WF_FTOOLBAR,
	WF_NTOOLBAR,
	WF_MENU,
	WF_WIDGET,
	WF_LAST,			/* end of contiguous range */
	WF_WHEEL	= 40,		/* wheel support */
	WF_M_BACKDROP	= 100,		/* KAOS 1.4 */
	WF_M_OWNER,			/* KAOS 1.4 */
	WF_M_WINDLIST,			/* KAOS 1.4 */
	WF_SHADE	= 0x575d,	/* WINX 2.3 */
	WF_STACK	= 0x575e,	/* WINX 2.3 */
	WF_TOPALL	= 0x575f,	/* WINX 2.3 */
	WF_BOTTOMALL	= 0x5760	/* WINX 2.3 */
};

enum widget_t
{
	W_BOX ,
	W_TITLE,
	W_CLOSER,
	W_NAME,
	W_FULLER,
	W_INFO,
	W_DATA,
	W_WORK,
	W_SIZER,
	W_VBAR,
	W_UPARROW,
	W_DNARROW,
	W_VSLIDE,
	W_VELEV,
	W_HBAR,
	W_LFARROW,
	W_RTARROW,
	W_HSLIDE,
	W_HELEV,
	W_SMALLER,			/* AES 4.1 */
	W_BOTTOMER,			/* MagiC 3 */
	W_HIDER  = W_BOTTOMER,
	W_HIGHEST
};

/* wind_set(WF_BEVENT) */
enum bevent
{
	BEVENT_WORK=1,			/* AES 4.0 */
	BEVENT_INFO,			/* MagiC 6 */
};

enum wind_arrow
{
	WA_UPPAGE,
	WA_DNPAGE,
	WA_UPLINE,
	WA_DNLINE,
	WA_LFPAGE,
	WA_RTPAGE,
	WA_LFLINE,
	WA_RTLINE,
	WA_WHEEL			/* wheel support */
};

/* wind calc flags */
enum wind_calc_c
{
	WC_BORDER,
	WC_WORK
};

enum wind_update
{
	END_UPDATE,
	BEG_UPDATE,
	END_MCTRL,
	BEG_MCTRL,
	BEG_EMERG,
	END_EMERG
};


/****** Resource definitions ********************************************/

/* data structure types */
enum rsrc_type
{
	R_TREE,
	R_OBJECT,
	R_TEDINFO,
	R_ICONBLK,
	R_BITBLK,
	R_STRING,			/* gets pointer to free strings */
	R_IMAGEDATA,			/* gets pointer to free images */
	R_OBSPEC,
	R_TEPTEXT,			/* sub ptrs in TEDINFO */
	R_TEPTMPLT,
	R_TEPVALID,
	R_IBPMASK,			/* sub ptrs in ICONBLK */
	R_IBPDATA,
	R_IBPTEXT,
	R_BIPDATA,			/* sub ptrs in BITBLK */
	R_FRSTR,			/* gets addr of ptr to free strings */
	R_FRIMG,			/* gets addr of ptr to free images  */
};

struct rsc_hdr
{
	short  rsh_vrsn;		/* RCS version no. */
	ushort rsh_object;		/* Offset to object[] */
	ushort rsh_tedinfo;		/* Offset to tedinfo[] */
	ushort rsh_iconblk;		/* Offset to iconblk[] */
	ushort rsh_bitblk;		/* Offset to bitblk[] */
	ushort rsh_frstr;		/* Offset to free string index */
	ushort rsh_string;		/* unused (Offset to first string) */
	ushort rsh_imdata;		/* Offset to image data */
	ushort rsh_frimg;		/* Offset to free image index */
	ushort rsh_trindex;		/* Offset to object tree index */
	short  rsh_nobs;		/* Number of objects */
	short  rsh_ntree;		/* Number of trees */
	short  rsh_nted;		/* Number of tedinfos */
	short  rsh_nib;			/* Number of icon blocks */
	short  rsh_nbb;			/* Number of blt blocks */
	short  rsh_nstring;		/* Number of free strings */
	short  rsh_nimages;		/* Number of free images */
	ushort rsh_rssize;		/* Total bytes in resource */
};
typedef struct rsc_hdr RSHDR;


/****** Shell definitions ***********************************************/

/* tail for default shell */
struct sheltail
{
	short dummy;			/* ein Nullwort               */
	long magic;			/* 'SHEL', wenn ist Shell     */
	short isfirst;			/* erster Aufruf der Shell    */
	long lasterr;			/* letzter Fehler             */
	short wasgr;			/* Programm war Grafikapp.    */
};

/* shel_write modes for parameter "isover" */
enum shw_isover
{
	SHW_IMMED,			/* PC-GEM 2.x  */
	SHW_CHAIN,			/* TOS         */
	SHW_DOS,			/* PC-GEM 2.x  */
	SHW_PARALLEL = 100,		/* MAG!X       */
	SHW_SINGLE			/* MAG!X       */
};

/* shel_write modes for parameter "doex" */
enum shw_doex
{
	SHW_NOEXEC,
	SHW_EXEC,
	SHW_EXEC_ACC =3,		/* AES 3.3	*/
	SHW_SHUTDOWN,			/* AES 3.3     */
	SHW_RESCHNG,			/* AES 3.3     */
	SHW_BROADCAST=7,		/* AES 4.0     */
	SHW_INFRECGN =9,		/* AES 4.0     */
	SHW_AESSEND,			/* AES 4.0     */
	SHW_THR_CREATE=20		/* MagiC 4.5	*/
};

/* extended shel_write() modes and parameter structure */
enum shw_xmd
{
	SHW_XMDLIMIT  =  256,
	SHW_XMDNICE   =  512,
	SHW_XMDDEFDIR = 1024,
	SHW_XMDENV    = 2048,
	SHW_XMDFLAGS  = 4096
};

struct xshw_command
{
	char *command;
	long limit;
	long nice;
	char *defdir;
	char *env;
	long flags;			/* ab MagiC 6 */
};

/*
 * XaAES/oAESis Extended Shell Write structure 
 * Extra fields for UID/GID setting of spawned clients.
 * Different naming.
 */
struct xshelw
{
	char *newcmd;
	long psetlimit;
	long prenice;
	char *defdir;
	char *env;
	short uid;			/* New child's UID */
	short gid;			/* New child's GID */
};
typedef struct xshelw XSHELW;

enum xshel_sw
{
	SW_PSETLIMIT	= 0x0100,
	SW_PRENICE	= 0x0200,
	SW_PDEFDIR	= 0x0400,
	SW_ENVIRON	= 0x0800,
	SW_UID 		= 0x1000,	/* Set user id of launched child */
	SW_GID		= 0x2000	/* Set group id of launched child */
};


/****** additional definitions ******************************************/

union private
{
	void *spec;			/* PC_GEM */
	long l;
	short pi[2];
};

/* At last give in to the fact that it is a struct, NOT an array */
struct aes_global
{
	short	version;
	short	count;
	short	id;
	union private *pprivate;	/*  3,  4 */
	void	*ptree;			/*  5,  6 */
	void	*rshdr;			/*  7,  8 */
	short	lmem;			/*  9     */
	short	nplanes;		/* 10     */
	short	res1, client_end;	/* 11, 12 */
	short	c_max_h, bvhard;	/* 13, 14 */
};

/* Event library definitions */
enum event_types
{
	MU_KEYBD		= 0x0001,
	MU_BUTTON		= 0x0002,
	MU_M1	 		= 0x0004,
	MU_M2	 		= 0x0008,
	MU_MESAG		= 0x0010,
	MU_TIMER		= 0x0020,
	MU_WHEEL		= 0x0040, /* AES wheel support */
	MU_MX			= 0x0080, /* HR: XaAES extension: report any mouse movement */
					  /*                      used for recursive menu's  */
	MU_NORM_KEYBD		= 0x0100, /* return normalized key code. */
	MU_DYNAMIC_KEYBD	= 0x0200  /* keybd as a bunch of buttons, includes release of key */
};

/* Event structure for EVNT_multi(), window dialogs, etc. */
typedef struct
{
	short mwhich;
	short mx;
	short my;
	short mbutton;
	short kstate;
	short key;
	short mclicks;
	short reserved[9];
	short msg[16];
} EVNT;

#endif /* _aes_defs_h */
