#ifndef _MT_GEMLIB_H_
# define _MT_GEMLIB_H_

# include "mint/kcompiler.h"

#ifndef INT16  /* keep compatibility with Mgemlib */
#define INT16 short
#endif

# ifdef __GEMLIB_OLDBIND		/* Backward-compatibility */
#  undef _MT_GEMLIB_H_		/* For old bindings, these header had to be multi-included. */

#  ifndef __GEMLIB_HAVE_DEFS	/* first include via aesbind/vdibind/gemfast */
#   define __GEMLIB_HAVE_DEFS
#  else
#   undef __GEMLIB_DEFS
#  endif

# else				/* New include scheme: one header defines all */
#  define __GEMLIB_DEFS
#  ifndef _GEM_VDI_P_
#   define __GEMLIB_AES
#  endif
#  ifndef _GEM_AES_P_
#   define __GEMLIB_VDI
#  endif
# endif

__BEGIN_DECLS


/*******************************************************************************
 * The version of the gem-lib
 */

/* Major and minor version number of the GEMLib package.  Use these macros
   to test for features in specific releases.  */
#define __GEMLIB__			 __GEMLIB_MAJOR__
#define	__GEMLIB_MAJOR__     0
#define	__GEMLIB_MINOR__     43
#define __GEMLIB_REVISION__  1
#define __GEMLIB_BETATAG__   ""

/* the other name of this release is MGEMLIB 42 */
#define MGEMLIB				42
#define __MGEMLIB__			42


#ifdef __GEMLIB_DEFS

/*******************************************************************************
 * The AES specific stuff from old gemfast.h
 */

#define NIL 				0
#define DESKTOP_HANDLE		0
#define DESK			 	DESKTOP_HANDLE

/* appl_control mode */
#define APC_SYSTEM			 2
#define APC_HIDE			10
#define APC_SHOW			11
#define APC_TOP 			12
#define APC_HIDENOT 			13
#define APC_INFO			14
#define APC_MENU			15
#define APC_WIDGETS			16

/* APC_INFO bits */
#define APCI_HIDDEN			0x01
#define APCI_HASMBAR			0x02
#define APCI_HASDESK			0x04

/* appl_getinfo modes */
#define AES_LARGEFONT			0
#define AES_SMALLFONT			1
#define AES_SYSTEM			2
#define AES_LANGUAGE 			3
#define AES_PROCESS 			4
#define AES_PCGEM			5
#define AES_INQUIRE 			6
#define AES_MOUSE			8
#define AES_MENU			9
#define AES_SHELL			10
#define AES_WINDOW			11
#define AES_MESSAGE			12
#define AES_OBJECT			13
#define AES_FORM			14
#define AES_VERSION			96
#define AES_WOPTS			97
#define AES_FUNCTIONS			98
#define AES_AOPTS			99

/* appl_getinfo return values */
#define SYSTEM_FONT		0
#define OUTLINE_FONT 		1

#define AESLANG_ENGLISH		0
#define AESLANG_GERMAN		1
#define AESLANG_FRENCH		2
#define AESLANG_SPANISH 	4
#define AESLANG_ITALIAN 	5
#define AESLANG_SWEDISH 	6

/* appl_getinfo(AES_VERSION) return values */
#define AES_DEVSTATUS_ALPHA	0
#define AES_DEVSTATUS_BETA	1
#define AES_DEVSTATUS_RELEASE	2
#define AES_FDEVSTATUS_STABLE	0x100

#define AES_ARCH_M68000	    0
#define AES_ARCH_M68010     1
#define AES_ARCH_M68020     2
#define AES_ARCH_M68030    	3
#define AES_ARCH_M68040    	4
#define AES_ARCH_M68060  	  5
#define AES_ARCH_M6802060	  6
#define AES_ARCH_COLDFILRE	7

/* appl_getinfo(AES_FUNCTIONS) return values */
#define AGI_WFORM		1
#define AGI_AOPTS		2
#define AGI_OBJCDATA		4

/* appl_options(AES_AOPTS) return values and appl_options() settings*/
#define AO0_WF_SLIDE		1
#define AO0_OBJC_EDIT		2


/* appl_read modes */
#define APR_NOWAIT			-1

/* appl_search modes */
#define APP_FIRST 			0
#define APP_NEXT			1
#define APP_DESK			2

/* appl_search return values*/
#define APP_SYSTEM		0x01
#define APP_APPLICATION		0x02
#define APP_ACCESSORY		0x04
#define APP_SHELL 		0x08
#define APP_AESSYS		0x10
#define APP_AESTHREAD		0x20

/* appl_trecord types */
#define APPEVNT_TIMER	 	0
#define APPEVNT_BUTTON	 	1
#define APPEVNT_MOUSE	 	2
#define APPEVNT_KEYBOARD 	3

/** struct used by mt_appl_trecord and mt_appl_tplay */
typedef struct pEvntrec
{
	long ap_event;		/**< TODO */
	long ap_value;		/**< TODO */
} EVNTREC;

/* evnt_button flags */
#define LEFT_BUTTON		0x0001
#define RIGHT_BUTTON 	0x0002
#define MIDDLE_BUTTON	0x0004

#define K_RSHIFT		0x0001
#define K_LSHIFT		0x0002
#define K_CTRL 			0x0004
#define K_ALT			0x0008

/* evnt_dclick flags */
#define EDC_INQUIRE		0
#define EDC_SET			1

/* event message values */
#define MN_SELECTED	 	10
#define WM_REDRAW 	 	20
#define WM_TOPPED 	 	21
#define WM_CLOSED 	 	22
#define WM_FULLED 	 	23
#define WM_ARROWED	 	24
#define WM_HSLID		25
#define WM_VSLID		26
#define WM_SIZED		27	/* affected by WCOWORK mode */
#define WM_MOVED		28	/* affected by WCOWORK mode */
#define WM_NEWTOP		29
#define WM_UNTOPPED		30
#define WM_ONTOP		31
#define WM_BOTTOM		33
#define WM_ICONIFY		34	/* Affected by WCOWORK mode */
#define WM_UNICONIFY		35	/* Affected by WCOWORK mode */
#define WM_ALLICONIFY		36	/* Affected by WCOWORK mode */
#define WM_TOOLBAR		37
#define WM_REPOSED		38	/* Affected by WCOWORK mode */
#define AC_OPEN			40
#define AC_CLOSE		41
#define AP_TERM			50
#define AP_TFAIL		51
#define AP_RESCHG 		57

/* old name */
#define WM_BOTTOMED		WM_BOTTOM

/* Xcontrol messages */
#define CT_UPDATE		50
#define CT_MOVE			51
#define CT_NEWTOP		52
#define CT_KEY			53

#define SHUT_COMPLETED		60
#define RESCHG_COMPLETED	61
#define RESCH_COMPLETED		61
#define AP_DRAGDROP 		63
#define SH_WDRAW			72
#define SC_CHANGED			80
#define PRN_CHANGED 		82
#define FNT_CHANGED 		83
#define THR_EXIT			88
#define PA_EXIT 			89
#define CH_EXIT 			90
#define WM_M_BDROPPED		100		/* KAOS 1.4  */
#define SM_M_SPECIAL		101		/* MAG!X     */
#define SM_M_RES2			102		/* MAG!X     */
#define SM_M_RES3			103		/* MAG!X     */
#define SM_M_RES4			104		/* MAG!X     */
#define SM_M_RES5			105		/* MAG!X     */
#define SM_M_RES6			106		/* MAG!X     */
#define SM_M_RES7			107		/* MAG!X     */
#define SM_M_RES8			108		/* MAG!X     */
#define SM_M_RES9			109		/* MAG!X     */
#define WM_SHADED			22360	/* [WM_SHADED apid 0 win 0 0 0 0] */
#define WM_UNSHADED 		22361	/* [WM_UNSHADED apid 0 win 0 0 0 0] */

/* subcodes for SM_M_SPECIAL message for the SCREENMGR
	used:
	INT16 msg[8];

	msg[0] = SM_M_SPECIAL;	 101
	msg[1] = ap_id;	 ap_id, with SMC_TIDY_UP and SMC_UNHIDEALL own
	msg[2] = 0;		 No overrun
	msg[3] = 0;
	msg[4] = 'MA';
	msg[5] = 'GX';
	msg[6] = ..subcode..;
	msg[7] = 0;
	appl_write(msg, 16, 1);		 apid #1 is the SCREENMGR
*/
#define SMC_TIDY_UP		0			/* MagiC 2  */
#define SMC_TERMINATE	1			/* MagiC 2  */
#define SMC_SWITCH		2			/* MagiC 2  */
#define SMC_FREEZE		3			/* MagiC 2  */
#define SMC_UNFREEZE	4			/* MagiC 2  */
#define SMC_RES5		5			/* MagiC 2  */
#define SMC_UNHIDEALL	6			/* MagiC 3.1   */
#define SMC_HIDEOTHERS	7			/* MagiC 3.1   */
#define SMC_HIDEACT		8			/* MagiC 3.1   */

/* bubble-gem */
typedef struct
{
	long   magic;   /* 'BGEM'                                   */
	long   size;    /* Größe dieser Struktur, derzeit 18        */
	short  release; /* derzeit 7, nie kleiner als 5             */
	short  active;  /* <>0, wenn gerade eine Hilfe angezeigt wird;  0  sonst                               */
	struct mouse_form *mhelp;   /* Zeiger auf Hilfe-Mausform                */
	short  dtimer;  /* Dämon-Timer; Default 200ms; ab Release 6 */
} BGEM;

typedef struct
{
#define BGC_FONTCHANGED 0x0001
#define BGC_NOWINSTYLE  0x0002
#define BGC_SENDKEY     0x0004
#define BGC_DEMONACTIVE 0x0008
#define BGC_TOPONLY     0x0010
	short flags;
	short display_time;	/* ms */
}BHLP;

#define C_BGEM 	0x4247454dL

#define BGS7_USRHIDE 0x0001
#define BGS7_USRHIDE2 0x0002	/* XaAES */
#define BGS7_MOUSE   0x0004
#define BGS7_DISPCL  0x0080	/* display client: XaAES-special */

#define BUBBLEGEM_REQUEST  0xBABA
#define BUBBLEGEM_SHOW     0xBABB
#define BUBBLEGEM_ACK      0xBABC
#define BUBBLEGEM_ASKFONT  0xBABD
#define BUBBLEGEM_FONT     0xBABE
#define BUBBLEGEM_HIDE     0xBABF

/* evnt_mouse modes */
#define MO_ENTER		0
#define MO_LEAVE		1

/* evnt_multi flags */
#define MU_KEYBD			0x0001
#define MU_BUTTON			0x0002
#define MU_M1				0x0004
#define MU_M2				0x0008
#define MU_MESAG			0x0010
#define MU_TIMER			0x0020
#define MU_WHEEL			0x0040		/* XaAES */
#define MU_MX				0x0080		/* XaAES */
#define MU_NORM_KEYBD			0x0100		/* XaAES */
#define MU_DYNAMIC_KEYBD		0x0200		/* XaAES */
#define MU_FSELECT			0x0400		/* XaAES */
#define MU_TIMEOUT			0x0800


/* form_dial opcodes */
#define FMD_START 			0
#define FMD_GROW			1
#define FMD_SHRINK			2
#define FMD_FINISH			3

/* form_error modes */
#define FERR_FILENOTFOUND	 2
#define FERR_PATHNOTFOUND	 3
#define FERR_NOHANDLES		 4
#define FERR_ACCESSDENIED	 5
#define FERR_LOWMEM			 8
#define FERR_BADENVIRON 	10
#define FERR_BADFORMAT		11
#define FERR_BADDRIVE		15
#define FERR_DELETEDIR		16
#define FERR_NOFILES 		18

/* fsel_(ex)input return values */
#define FSEL_CANCEL		 0
#define FSEL_OK			 1

/* menu_attach modes */
#define ME_INQUIRE		0
#define ME_ATTACH 		1
#define ME_REMOVE 		2

/* menu_attach attributes */
#define SCROLL_NO 		0
#define SCROLL_YES		1

/* menu_bar modes */
#define MENU_INQUIRE	-1
#define MENU_REMOVE	0
#define MENU_INSTALL	1
#define MENU_GETMODE	3	   /**< Get the menu bar mode */
#define MENU_SETMODE	4	   /**< Set the menu bar mode */
#define MENU_UPDATE 	5	   /**< Update the system part of the menu bar */
#define MENU_INSTL		100    /**< Install a menu without switching the top application (Magic) */


/* MENU_GETMODE and MENU_SETMODE bits */
#define  MENU_HIDDEN	0x0001 /**< menu bar only visible when needed */
#define  MENU_PULLDOWN  0x0002 /**< Pulldown-Menus */
#define  MENU_SHADOWED  0x0004 /**< menu bar with shadows */

/* menu_icheck modes */
#define UNCHECK			0
#define CHECK			1

/* menu_ienable modes */
#define DISABLE			0
#define ENABLE 			1

/* menu_istart modes */
#define MIS_GETALIGN 		0
#define MIS_SETALIGN 		1

/* menu_popup modes */
#define SCROLL_LISTBOX		-1

/* menu_register modes */
#define REG_NEWNAME		-1

/* menu_tnormal modes */
#define HIGHLIGHT		0
#define UNHIGHLIGHT		1

/** menu_settings uses a new structure for setting and inquiring the submenu
    delay values and the menu scroll height.*/
typedef struct _mn_set
{
	long  display;		/**< the submenu display delay in milliseconds */
	long  drag;		/**< the submenu drag delay in milliseconds */
	long  delay;		/**< the single-click scroll delay in milliseconds */
	long  speed;		/**< the continuous scroll delay in milliseconds */
	short height; 		/**< the menu scroll height (in items) */
} MN_SET;


/* shel_get modes */
#define SHEL_BUFSIZE (-1)

/** Struktur fr erweiterte Start-Daten (fr shel_write) */
typedef struct
{
	char *newcmd;		/**< TODO */
	long psetlimit;		/**< TODO */
	long prenice;		/**< TODO */
	char *defdir;		/**< TODO */
	char *env;			/**< TODO */
} SHELW;

/** Andere Definition dafr (aus MagiC-Doku) */
typedef struct
{
	char	*command;	/**< TODO */
	long	limit;		/**< TODO */
	long	nice;		/**< TODO */
	char	*defdir;	/**< TODO */
	char	*env;		/**< TODO */
} XSHW_COMMAND;

/** TODO */
typedef struct
{
	long			__CDECL (*proc)(void *par);	/**< TODO */
	void			*user_stack;				/**< TODO */
	unsigned long	stacksize;					/**< TODO */
	short			mode;						/**< immer auf 0 setzen!  */
	long			res1;						/**< immer auf 0L setzen! */
} THREADINFO;

/** TODO */
typedef  struct
{
	int   dummy;		/**< a null  word 			   */
	long  magic;		/**< 'SHEL', if it's a Shell   */
	int   isfirst;		/**< first call of the Shell   */
	long  lasterr;		/**< last error  			   */
	int   wasgr;		/**< Program was a grafic app. */
} SHELTAIL;


/* shel_write modes */
#define SWM_LAUNCH		0
#define SWM_LAUNCHNOW		1
#define SWM_LAUNCHACC		3
#define SWM_SHUTDOWN		4
#define SWM_REZCHANGE		5
#define SWM_BROADCAST		7
#define SWM_ENVIRON		8
#define SWM_NEWMSG		9
#define SWM_AESMSG		10
#define SWM_THRCREATE		20

/* other names for shel_write modes */
#define SHW_NOEXEC		SWM_LAUNCH
#define SHW_EXEC		SWM_LAUNCHNOW
#define SHW_EXEC_ACC		SWM_LAUNCHACC
#define SHW_SHUTDOWN		SWM_SHUTDOWN
#define SHW_RESCHNG		SWM_REZCHANGE
#define SHW_BROADCAST		SWM_BROADCAST
#define SHW_INFRECGN		SWM_NEWMSG
#define SHW_AESSEND		SWM_AESMSG
#define SHW_THR_CREATE		SWM_THRCREATE

/* shel_write */
#define SHW_IMMED		0	/* PC-GEM  2.x */
#define SHW_CHAIN		1	/* TOS */
#define SHW_DOS			2	/* PC-GEM  2.x */
#define SHW_PARALLEL		100	/* MAG!X */
#define SHW_SINGLE		101	/* MAG!X */

/* command line parser (shel_write: parameter "isover") */
#define CL_NORMAL		0
#define CL_PARSE		1

/* shutdown action (shel_write: mode SWM_SHUTDOWN, parameter "isover") */
#define SD_ABORT		0		/**< Abort shutdown mode */
#define SD_PARTIAL		1		/**< Partial shutdown mode */
#define SD_COMPLETE		2		/**< Complete shutdown mode */

/* shel_write: mode SWM_ENVIRON, parameter 'wisgr' */
#define ENVIRON_SIZE	0
#define ENVIRON_CHANGE	1
#define ENVIRON_COPY	2

/* shel_write: mode SWM_NEWMSG, parameter 'wisgr' */
#define NM_APTERM		0x0001
#define NM_INHIBIT_HIDE		0x0002

/* Werte fr Modus SWM_AESMSG (fr shel_write) */
#define AP_AESTERM		52     /* Mode 10: N.AES komplett terminieren. */

/* extended shel_write() modes and parameter structure */
#define SHW_XMDLIMIT	256
#define SHW_XMDNICE		512
#define SHW_XMDDEFDIR	1024
#define SHW_XMDENV		2048

/* shel_write flags */
#define SW_PSETLIMIT	0x0100
#define SW_PRENICE		0x0200
#define SW_DEFDIR 		0x0400
#define SW_ENVIRON		0x0800

/* rsrc_gaddr structure types */
#define R_TREE			0
#define R_OBJECT		1
#define R_TEDINFO 		2
#define R_ICONBLK 		3
#define R_BITBLK		4
#define R_STRING		5
#define R_IMAGEDATA		6
#define R_OBSPEC		7
#define R_TEPTEXT 		8
#define R_TEPTMPLT		9
#define R_TEPVALID		10
#define R_IBPMASK		11
#define R_IBPDATA		12
#define R_IBPTEXT		13
#define R_BIPDATA		14
#define R_FRSTR			15
#define R_FRIMG			16



/* Window Attributes */
#define NAME			0x0001
#define CLOSER 			0x0002
#define FULLER 			0x0004
#define MOVER			0x0008
#define INFO			0x0010
#define SIZER			0x0020
#define UPARROW			0x0040
#define DNARROW			0x0080
#define VSLIDE 			0x0100
#define LFARROW			0x0200
#define RTARROW			0x0400
#define HSLIDE 			0x0800
#define HOTCLOSEBOX		0x1000      /* GEM 2.x (deprecated) */
#define MENUBAR			0x1000		/* XaAES */
#define BACKDROP		0x2000
#define SMALLER			0x4000
#define BORDER			0x8000
#define ICONIFIER		SMALLER

/* wind_create flags */
#define WC_BORDER 	 	0
#define WC_WORK		 	1

/* wind_get flags */
#define WF_KIND			  1
#define WF_NAME			  2
#define WF_INFO			  3
#define WF_WORKXYWH		  4
#define WF_CURRXYWH		  5
#define WF_PREVXYWH		  6
#define WF_FULLXYWH		  7
#define WF_HSLIDE 		  8
#define WF_VSLIDE 		  9
#define WF_TOP 			 10
#define WF_FIRSTXYWH 		 11
#define WF_NEXTXYWH		 12
#define WF_FIRSTAREAXYWH	 13
#define WF_NEWDESK		 14
#define WF_HSLSIZE		 15
#define WF_VSLSIZE		 16
#define WF_SCREEN 		 17
#define WF_COLOR		 18
#define WF_DCOLOR 		 19
#define WF_OWNER		 20
#define WF_BEVENT 		 24
#define WF_BOTTOM 		 25
#define WF_ICONIFY		 26
#define WF_UNICONIFY 	 27
#define WF_UNICONIFYXYWH 28
#define WF_TOOLBAR		 30
#define WF_FTOOLBAR		 31
#define WF_NTOOLBAR		 32
#define WF_MENU			 33	/* XaAEES */
#define WF_WHEEL		 40	/* XaAES */
#define WF_OPTS			 41	/* XaAES */

#define WF_CALCF2W		42
#define WF_CALCW2F		43
#define WF_CALCF2U		44
#define WF_CALCU2F		45
#define WF_MAXWORKXYWH		46

#define WF_M_BACKDROP		100      /* KAOS 1.4    */
#define WF_M_OWNER		101      /* KAOS 1.4    */
#define WF_M_WINDLIST		102      /* KAOS 1.4    */
#define WF_MINXYWH		103      /* MagiC 6     */
#define WF_INFOXYWH		104      /* MagiC 6.10  */
#define WF_WIDGETS		200		 /* NAES */
#define WF_TOPMOST		232	/* XaAES, MyAES */
#define WF_WINX			22360
#define WF_WINXCFG		22361
#define WF_SHADE		22365      /* WINX 2.3 */
#define WF_STACK		22366      /* WINX 2.3 */
#define WF_TOPALL		22367      /* WINX 2.3 */
#define WF_BOTTOMALL		22368	   /* WINX 2.3 */
#define WF_XAAES		0x5841	   /* XaAES: 'XA' */

/* wind_set(WF_BEVENT) */
#define BEVENT_WORK     0x0001    /* AES 4.0  */
#define BEVENT_INFO     0x0002    /* MagiC 6  */

/* wind_set(WF_OPTIONS) */
#define WO0_WHEEL	0x0001
#define WO0_FULLREDRAW	0x0002
#define WO0_NOBLITW	0x0004
#define WO0_NOBLITH	0x0008
#define WO0_SENDREPOS	0x0010
#define WO0_WCOWORK	0x0020

/* wind_set(WF_WHEEL) */
#define WHL_REALWHEEL	0	/* */
#define WHL_AROWWHEEL	1
#define WHL_SLDRWHEEL	2
#define MAX_WHLMODE	2
#define DEF_WHLMODE	WHL_AROWWHEEL

/* window elements */
#define W_BOX			0
#define W_TITLE			1
#define W_CLOSER		2
#define W_NAME 			3
#define W_FULLER		4
#define W_INFO 			5
#define W_DATA 			6
#define W_WORK 			7
#define W_SIZER			8
#define W_VBAR 			9
#define W_UPARROW		10
#define W_DNARROW		11
#define W_VSLIDE		12
#define W_VELEV			13
#define W_HBAR			14
#define W_LFARROW		15
#define W_RTARROW		16
#define W_HSLIDE		17
#define W_HELEV			18
#define W_SMALLER		19
#define W_BOTTOMER		20      /* MagiC 3     */
#define W_HIDER			30

/* arrow message */
#define WA_UPPAGE 		0
#define WA_DNPAGE 		1
#define WA_UPLINE 		2
#define WA_DNLINE 		3
#define WA_LFPAGE 		4
#define WA_RTPAGE 		5
#define WA_LFLINE 		6
#define WA_RTLINE 		7
/* XaAES only */
#define WA_UPSCAN		8
#define WA_DNSCAN		9
#define WA_LFSCAN		10
#define WA_RTSCAN		11
//#define WA_WHEEL		8	/* XaAES */

/* wind_update flags */
#define END_UPDATE		0
#define BEG_UPDATE		1
#define END_MCTRL		2
#define BEG_MCTRL		3

/* graf_mouse mouse types */
#define ARROW			 0
#define TEXT_CRSR 		 1
#define BEE 			 2
#define BUSY_BEE	  BEE 	/* alias */
#define BUSYBEE		  BEE 	/* alias */
#define HOURGLASS 		 2
#define POINT_HAND		 3
#define FLAT_HAND 		 4
#define THIN_CROSS		 5
#define THICK_CROSS		 6
#define OUTLN_CROSS		 7
#define USER_DEF		  255
#define M_OFF			  256
#define M_ON			  257
#define M_SAVE 			  258
#define M_RESTORE 		  259
#define M_LAST 			  260
#define M_PREVIOUS        M_LAST  /* alias */
#define X_MRESET      1000	//geneva
#define X_MGET        1001	//geneva
#define X_MSET_SHAPE  1100	//geneva
#define M_FORCE       0x8000

/* objects - general */
#define ROOT		 	0 	 /* index of ROOT */
#define MAX_LEN			81 	 /* max string length */
#define MAX_DEPTH		8 	 /* max depth of search or draw */

/* inside fill patterns */
#define IP_HOLLOW		0
#define IP_1PATT		1
#define IP_2PATT		2
#define IP_3PATT		3
#define IP_4PATT		4
#define IP_5PATT		5
#define IP_6PATT		6
#define IP_SOLID		7

/* font types */
#define GDOS_PROP		0
#define GDOS_MONO		1
#define GDOS_BITM		2
#define IBM			3
#define SMALL			5

/* object types */
#define G_BOX			20
#define G_TEXT			21
#define G_BOXTEXT		22
#define G_IMAGE			23
#define G_USERDEF		24
#define G_PROGDEF		G_USERDEF
#define G_IBOX			25
#define G_BUTTON		26
#define G_BOXCHAR		27
#define G_STRING		28
#define G_FTEXT			29
#define G_FBOXTEXT		30
#define G_ICON			31
#define G_TITLE			32
#define G_CICON			33

/* extended object types, MagiC only */
#define G_SWBUTTON		34
#define G_POPUP			35
#define G_WINTITLE		36
#define G_EDIT			37
#define G_SHORTCUT		38


/* object flags */
#define OF_NONE		 	0x0000
#define OF_SELECTABLE		0x0001
#define OF_DEFAULT		0x0002
#define OF_EXIT			0x0004
#define OF_EDITABLE		0x0008
#define OF_RBUTTON		0x0010
#define OF_LASTOB		0x0020
#define OF_TOUCHEXIT		0x0040
#define OF_HIDETREE		0x0080
#define OF_INDIRECT		0x0100
#define OF_FL3DIND		0x0200	/* bit 9 */
#define OF_FL3DBAK		0x0400	/* bit 10 */
#define OF_FL3DACT		0x0600
#define FL3DMASK		(OF_FL3DIND|OF_FL3DBAK|OF_FL3DACT)
#define OF_SUBMENU		0x0800	/* bit 11 */
#define OF_FLAG11		OF_SUBMENU
#define OF_FLAG12		0x1000
#define OF_FLAG13		0x2000
#define OF_FLAG14		0x4000
#define OF_FLAG15		0x8000

/* Object states */
#define OS_NORMAL		0x0000
#define OS_SELECTED		0x0001
#define OS_CROSSED		0x0002
#define OS_CHECKED		0x0004
#define OS_DISABLED		0x0008
#define OS_OUTLINED		0x0010
#define OS_SHADOWED		0x0020
#define OS_WHITEBAK		0x0040
#define OS_DRAW3D		0x0080

#define OS_STATE08		0x0100
#define OS_STATE09		0x0200
#define OS_STATE10		0x0400
#define OS_STATE11		0x0800
#define OS_STATE12		0x1000
#define OS_STATE13		0x2000
#define OS_STATE14		0x4000
#define OS_STATE15		0x8000

/* Object colors - default pall. */
#define G_WHITE			0
#define G_BLACK			1
#define G_RED			2
#define G_GREEN			3
#define G_BLUE			4
#define G_CYAN			5
#define G_YELLOW		6
#define G_MAGENTA		7
#define G_LWHITE		8
#define G_LBLACK		9
#define G_LRED			10
#define G_LGREEN		11
#define G_LBLUE			12
#define G_LCYAN			13
#define G_LYELLOW		14
#define G_LMAGENTA		15


#ifdef __GEMLIB_OLDNAMES

/* object flags */
#define NONE		 	0x0000
#define SELECTABLE		0x0001
#define DEFAULT			0x0002
#define EXIT			0x0004
#define EDITABLE		0x0008
#define RBUTTON			0x0010
#define LASTOB			0x0020
#define TOUCHEXIT		0x0040
#define HIDETREE		0x0080
#define INDIRECT		0x0100
#define FL3DIND			0x0200	/* bit 9 */
#define FL3DBAK			0x0400	/* bit 10 */
#define FL3DACT			0x0600
#define SUBMENU			0x0800	/* bit 11 */
#define FLAG11			SUBMENU
#define FLAG12			0x1000
#define FLAG13			0x2000
#define FLAG14			0x4000
#define FLAG15			0x8000

/* Object states */
#define NORMAL			0x0000
#define SELECTED		0x0001
#define CROSSED			0x0002
#define CHECKED			0x0004
#define DISABLED		0x0008
#define OUTLINED		0x0010
#define SHADOWED		0x0020
#define WHITEBAK		0x0040
#define DRAW3D			0x0080
#define STATE08			0x0100
#define STATE09			0x0200
#define STATE10			0x0400
#define STATE11			0x0800
#define STATE12			0x1000
#define STATE13			0x2000
#define STATE14			0x4000
#define STATE15			0x8000

/* Object colors - default pall. */
#define WHITE			0
#define BLACK			1
#define RED			2
#define GREEN			3
#define BLUE			4
#define CYAN			5
#define YELLOW			6
#define MAGENTA			7
#define LWHITE			8
#define LBLACK			9
#define LRED			10
#define LGREEN			11
#define LBLUE			12
#define LCYAN			13
#define LYELLOW			14
#define LMAGENTA		15

#endif


/* editable text field definitions */
#define EDSTART			0
#define EDINIT 			1
#define EDCHAR 			2
#define EDEND			3

#define ED_START		0
#define ED_INIT			1
#define ED_CHAR			2
#define ED_END 			3
/* New modes in XaAES */
/* XaAES extensions to objc_edit() */
#define ED_DISABLE		5
#define ED_ENABLE		6
#define ED_CRSRON		7
#define ED_CRSROFF		8

#define ED_MARK			9
#define ED_STRING		10
#define ED_SETPTEXT		11
#define ED_SETPTMPLT		12
#define ED_SETPVALID		13
#define ED_GETPTEXT		14
#define ED_GETPTMPLT		15
#define ED_GETPVALID		16

#define ED_CRSR			100		/* MagiC only */
#define ED_DRAW			103		/* MagiC only */

#define ED_REDRAW		200		/* XaAES only */
#define ED_XINIT		201
#define ED_XCHAR		202
#define ED_XEND			203		/* Redraw cursor, XaAES only */
#define ED_CHGTEXTPTR		204
#define ED_CHGTMPLPTR		205

/* editable text justification */
#define TE_LEFT			0
#define TE_RIGHT		1
#define TE_CNTR			2

/* objc_change modes */
#define NO_DRAW			0
#define REDRAW 			1

/* objc_order modes */
#define OO_LAST			-1
#define OO_FIRST		0

/* objc_sysvar modes */
#define SV_INQUIRE		0
#define SV_SET 			1

/* objc_sysvar values */
#define LK3DIND			1
#define LK3DACT			2
#define INDBUTCOL 		3
#define ACTBUTCOL 		4
#define BACKGRCOL 		5
#define AD3DVAL			6
#define MX_ENABLE3D		10      /* MagiC 3.0   */
#define MENUCOL			11      /* MagiC 6     */

/** Mouse Form Definition Block */
typedef struct mouse_form
{
	short		mf_xhot;		/**< TODO */
	short		mf_yhot;		/**< TODO */
	short		mf_nplanes;		/**< TODO */
	short		mf_fg;			/**< TODO */
	short		mf_bg;			/**< TODO */
	short		mf_mask[16];	/**< TODO */
	short		mf_data[16];	/**< TODO */
} MFORM;

#ifndef __PXY
# define __PXY
/** TODO */
typedef struct point_coord
{
	short p_x;			/**< TODO */
	short p_y;			/**< TODO */
} PXY;
#endif

#ifndef __GRECT
# define __GRECT
/** TODO */
typedef struct graphic_rectangle
{
	short g_x;			/**< TODO */
	short g_y;			/**< TODO */
	short g_w;			/**< TODO */
	short g_h;			/**< TODO */
} GRECT;
#endif

/** TODO */
typedef struct objc_colorword
{
	unsigned	borderc : 4;			/**< TODO */
	unsigned	textc   : 4;			/**< TODO */
	unsigned	opaque  : 1;			/**< TODO */
	unsigned	pattern : 3;			/**< TODO */
	unsigned	fillc   : 4;			/**< TODO */
} OBJC_COLORWORD;

/** TODO */
typedef struct text_edinfo
{
	char		*te_ptext;		/**< ptr to text */
	char		*te_ptmplt;		/**< ptr to template */
	char		*te_pvalid;		/**< ptr to validation chrs. */
	short		te_font; 		/**< font */
	short		te_fontid;		/**< font id */
	short		te_just; 		/**< justification */
	short		te_color;		/**< color information word */
	short		te_fontsize;	/**< font size */
	short		te_thickness;	/**< border thickness */
	short		te_txtlen;		/**< length of text string */
	short		te_tmplen;		/**< length of template string */
} TEDINFO;

/** TODO */
typedef struct icon_block
{
	short		*ib_pmask;			/**< TODO */
	short		*ib_pdata;			/**< TODO */
	char		*ib_ptext;			/**< TODO */
	short		ib_char;			/**< TODO */
	short		ib_xchar;			/**< TODO */
	short		ib_ychar;			/**< TODO */
	short		ib_xicon;			/**< TODO */
	short		ib_yicon;			/**< TODO */
	short		ib_wicon;			/**< TODO */
	short		ib_hicon;			/**< TODO */
	short		ib_xtext;			/**< TODO */
	short		ib_ytext;			/**< TODO */
	short		ib_wtext;			/**< TODO */
	short		ib_htext;			/**< TODO */
} ICONBLK;

/** TODO */
typedef struct bit_block
{
	short		*bi_pdata;	/**< ptr to bit forms data  */
	short		bi_wb;		/**< width of form in bytes */
	short		bi_hl;		/**< height in lines */
	short		bi_x; 		/**< source x in bit form */
	short		bi_y; 		/**< source y in bit form */
	short		bi_color;	/**< fg color of blt */
} BITBLK;

/** TODO */
typedef struct cicon_data
{
	short 		num_planes;			/**< TODO */
	short 		*col_data;			/**< TODO */
	short 		*col_mask;			/**< TODO */
	short 		*sel_data;			/**< TODO */
	short 		*sel_mask;			/**< TODO */
	struct cicon_data *next_res;	/**< TODO */
} CICON;

/** TODO */
typedef struct cicon_blk
{
	ICONBLK		monoblk;			/**< TODO */
	CICON 		*mainlist;			/**< TODO */
} CICONBLK;

/** TODO */
typedef struct
{
	 unsigned	character   :  8;			/**< TODO */
	 signed		framesize   :  8;			/**< TODO */
	 unsigned	framecol    :  4;			/**< TODO */
	 unsigned	textcol	    :  4;			/**< TODO */
	 unsigned	textmode    :  1;			/**< TODO */
	 unsigned	fillpattern :  3;			/**< TODO */
	 unsigned	interiorcol :  4;			/**< TODO */
} BFOBSPEC;

struct object;
typedef struct
{
	struct object	*tree;
	short		obnum;
} POPINFO;

/** TODO */
struct user_block;	/* forward declaration */

/** TODO */
typedef union obspecptr
{
	long		index;				/**< TODO */
	union obspecptr	*indirect;		/**< TODO */
	BFOBSPEC 	obspec;				/**< TODO */
	TEDINFO		*tedinfo;			/**< TODO */
	BITBLK		*bitblk;			/**< TODO */
	ICONBLK		*iconblk;			/**< TODO */
	CICONBLK 	*ciconblk;			/**< TODO */
	struct user_block *userblk;		/**< TODO */
	char		*free_string;		/**< TODO */
	POPINFO		*popinfo;
} OBSPEC;

/** TODO */
typedef struct object
{
	short 		ob_next;		/**< object's next sibling */
	short 		ob_head; 		/**< head of object's children */
	short 		ob_tail; 		/**< tail of object's children */
	unsigned short	ob_type; 	/**< type of object */
	unsigned short	ob_flags;	/**< flags */
	unsigned short	ob_state;	/**< state */
	OBSPEC		ob_spec; 		/**< object-specific data */
	short 		ob_x; 			/**< upper left corner of object */
	short 		ob_y; 			/**< upper left corner of object */
	short 		ob_width;		/**< width of obj */
	short 		ob_height;		/**< height of obj */
} OBJECT;

/** TODO */
typedef struct parm_block
{
	OBJECT	*pb_tree;		/* Pointer to the object tree         */
	short	pb_obj;				/* Index of the object                */
	short	pb_prevstate;	/* Previous object status             */
	short	pb_currstate;	/* New object status            			*/
	short	pb_x;					/* X-position of the object           */
	short	pb_y;					/* Y-position of the object           */
	short	pb_w;					/* Width of the object                */
	short	pb_h;					/* Height of the object               */
	short	pb_xc;				/* X-position of the clipping region  */
	short	pb_yc;				/* Y-position of the clipping region  */
	short	pb_wc;				/* Width of the clipping region       */
	short	pb_hc;				/* Height of the clipping region      */
	long	pb_parm;			/* Parameter of USERBLK structure     */
} PARMBLK;

/** TODO */
typedef struct user_block
{
	short __CDECL (*ub_code)(PARMBLK *parmblock);	/**< TODO */
	long ub_parm;									/**< TODO */
} USERBLK;

/** TODO */
typedef struct rshdr
{
	short		rsh_vrsn;			/**< TODO */

	unsigned short	rsh_object;			/**< TODO */
	unsigned short	rsh_tedinfo;		/**< TODO */
	unsigned short	rsh_iconblk;		/**< list of ICONBLKS */
	unsigned short	rsh_bitblk;			/**< TODO */
	unsigned short	rsh_frstr;			/**< TODO */
	unsigned short	rsh_string;			/**< TODO */
	unsigned short	rsh_imdata;			/**< image data */
	unsigned short	rsh_frimg;			/**< TODO */
	unsigned short	rsh_trindex;		/**< TODO */

	short		rsh_nobs;			/**< counts of various structs */
	short		rsh_ntree;			/**< TODO */
	short		rsh_nted;			/**< TODO */
	short		rsh_nib;			/**< TODO */
	short		rsh_nbb;			/**< TODO */
	short		rsh_nstring;		/**< TODO */
	short		rsh_nimages;		/**< TODO */
	unsigned short	rsh_rssize;			/**< total bytes in resource */
} RSHDR;

typedef struct rsxhdr
{
	unsigned short	rsh_vrsn;     /* should be 3                                 */
	unsigned short	rsh_extvrsn;  /* not used, initialised to 'IN' for Interface */

	unsigned long	rsh_object;
	unsigned long	rsh_tedinfo;
	unsigned long	rsh_iconblk;  /* list of ICONBLKS                            */
	unsigned long	rsh_bitblk;
	unsigned long	rsh_frstr;
	unsigned long	rsh_string;
	unsigned long	rsh_imdata;   /* image data                                  */
	unsigned long	rsh_frimg;
	unsigned long	rsh_trindex;

	unsigned long	rsh_nobs;     /* counts of various structs                   */
	unsigned long	rsh_ntree;
	unsigned long	rsh_nted;
	unsigned long	rsh_nib;
	unsigned long	rsh_nbb;
	unsigned long	rsh_nstring;
	unsigned long	rsh_nimages;
	unsigned long	rsh_rssize;   /* total bytes in resource                     */
} RSXHDR;

/** MENU structure, used by a_menu functions */
typedef struct _menu
{
	OBJECT *mn_tree;    /**< Points to the OBJECT tree of the sub-menu */
	short  mn_menu;     /**< Is an index to the parent object of the menu
                             items. */
	short  mn_item;     /**< Is the starting menu item */
	short  mn_scroll;   /**< If SCROLL_NO (0), the menu will not scroll.
                             If SCROLL_YES (1), and the number of menu
                             items exceed the menu scroll height, arrows
                             will appear which allow the user to scroll
                             selections.*/
	short  mn_keystate; /**< key state */
} MENU;

/** TODO */
typedef struct
{
	short m_out;	/**< TODO */
	short m_x;		/**< TODO */
	short m_y;		/**< TODO */
	short m_w;		/**< TODO */
	short m_h;		/**< TODO */
} MOBLK;

#if !( defined(__PUREC__) && defined(__TOS) )
/** TODO */
typedef struct mouse_event_type
{
	short *x;	/**< TODO */
	short *y;	/**< TODO */
	short *b;	/**< TODO */
	short *k;	/**< TODO */
} MOUSE;
#endif

/** structure comprising the most of the input arguments of mt_evnt_multi()
 */
typedef struct {
	short emi_flags;          /**< the event mask to watch */
	short emi_bclicks;		  /**< see mt_evnt_multi() */
	short emi_bmask;		  /**< see mt_evnt_multi() */
	short emi_bstate;		  /**< see mt_evnt_multi() */
	short emi_m1leave;		  /**< TODO */
	GRECT emi_m1;             /**< the first rectangle to watch */
	short emi_m2leave;		  /**< TODO */
	GRECT emi_m2;             /**< the second rectangle to watch */
	short emi_tlow;		  	  /**< see mt_evnt_multi() */
	short emi_thigh;          /**< the timer 32-bit value of interval split into short type member */
} EVMULT_IN;

/** structure comprising the output arguments of mt_evnt_multi()
 *
 * @note For undocumented members consult the mt_evnt_multi() documentation.
 */
typedef struct {
	short emo_events;                          /**< the bitfield of events occured (also a return value of mt_evnt_multi_fast() */
	PXY   emo_mouse;	/**< TODO */
	short emo_mbutton;	/**< TODO */
	short emo_kmeta;	/**< TODO */
	short emo_kreturn;	/**< TODO */
	short emo_mclicks;	/**< TODO */
} EVMULT_OUT;

#endif

#ifdef __GEMLIB_AES

/*******************************************************************************
 * The AES bindings from old aesbind.h
 */

/** @addtogroup a_appl
 *  @{
 */
short	mt_appl_bvset   (short bvdisk, short bvhard, short *global_aes);
short	mt_appl_control (short ap_cid, short ap_cwhat, void *ap_cout, short *global_aes);
short	mt_appl_exit    (short *global_aes);
short	mt_appl_find    (const char *name, short *global_aes);
short	mt_appl_getinfo (short type,
						 short *out1, short *out2, short *out3, short *out4, short *global_aes);
short	mt_appl_init    (short *global_aes);
short	mt_appl_read    (short ap_id, short length, void *ap_pbuff, short *global_aes);
short	mt_appl_search  (short mode, char *fname, short *type, short *ap_id, short *global_aes);
short	mt_appl_tplay   (void *mem, short num, short scale, short *global_aes);
short	mt_appl_trecord (void *mem, short count, short *global_aes);
short	mt_appl_write   (short ap_id, short length, void *ap_pbuff, short *global_aes);
short	mt_appl_yield   (short *global_aes);
/**@}*/

/** @addtogroup a_evnt
 *  @{
 */
short	mt_evnt_button (short Clicks, short WhichButton, short WhichState,
						short *Mx, short *My, short *ButtonState, short *KeyState, short *global_aes);
short	mt_evnt_dclick (short ToSet, short SetGet, short *global_aes);
short	mt_evnt_keybd  (short *global_aes);
short	mt_evnt_mesag  (short MesagBuf[], short *global_aes);
short	mt_evnt_mouse  (short EnterExit, short InX, short InY, short InW, short InH,
						short *OutX, short *OutY, short *ButtonState, short *KeyState, short *global_aes);
short   mt_evnt_multi  (short Type, short Clicks, short WhichButton, short WhichState,
						short EnterExit1, short In1X, short In1Y, short In1W, short In1H,
						short EnterExit2, short In2X, short In2Y, short In2W, short In2H,
						short MesagBuf[], unsigned long Interval, short *OutX, short *OutY,
						short *ButtonState, short *KeyState, short *Key, short *ReturnCount, short *global_aes);
short   mt_evnt_multi_fast (const EVMULT_IN * em_i,
							short MesagBuf[], EVMULT_OUT * em_o, short *global_aes);
short	mt_evnt_timer  (unsigned long Interval, short *global_aes);
/**@}*/

/** @addtogroup a_form
 *  @{
 */
short mt_form_alert  (short DefButton, const char *Str, short *global_aes);
short mt_form_button (OBJECT *, short Bobject, short Bclicks, short *Bnxtobj, short *global_aes);
short mt_form_center (OBJECT *, short *Cx, short *Cy, short *Cw, short *Ch, short *global_aes);
short mt_form_dial   (short Flag, short Sx, short Sy, short Sw, short Sh,
                               short Bx, short By, short Bw, short Bh, short *global_aes);
short mt_form_do     (OBJECT *, short StartObj, short *global_aes);
short mt_form_error  (short ErrorCode, short *global_aes);
short mt_form_keybd  (OBJECT *, short Kobject, short Kobnext, short Kchar,
                             short *Knxtobject, short *Knxtchar, short *global_aes);
/**@}*/

/** @addtogroup a_fsel
 *  @{
 */
short 	mt_fsel_exinput	(char *path, char *file, short *exit_button, const char *title, short *global);
short 	mt_fsel_input	(char *path, char *file, short *exit_button, short *global);
short 	mt_fsel_boxinput(char *path, char *file, short *exit_button, const char *title, void *callback, short *global);
/**@}*/

/** @addtogroup a_graf
 *  @{
 */
short	mt_graf_dragbox	(short Sw, short Sh, short Sx, short Sy, short Bx, short By, short Bw, short Bh, short *Fw, short *Fh, short *global_aes);
short	mt_graf_growbox	(short Sx, short Sy, short Sw, short Sh, short Fx, short Fy, short Fw, short Fh, short *global_aes);
short	mt_graf_handle	(short *Wchar, short *Hchar, short *Wbox, short *Hbox, short *global_aes);
short	mt_graf_xhandle	(short *Wchar, short *Hchar, short *Wbox, short *Hbox, short * device, short *global_aes);
short	mt_graf_mbox	(short Sw, short Sh, short Sx, short Sy, short Dx, short Dy, short *global_aes);
short	mt_graf_mkstate	(short *Mx, short *My, short *ButtonState, short *KeyState, short *global_aes);
short	mt_graf_mouse	(short Form, const MFORM *FormAddress, short *global_aes);
short	mt_graf_rubberbox	(short Ix, short Iy, short Iw, short Ih, short *Fw, short *Fh, short *global_aes);
short	mt_graf_multirubber (short bx, short by, short mw, short mh, GRECT *rec, short *rw, short *rh, short *global_aes);
short	mt_graf_shrinkbox	(short Fx, short Fy, short Fw, short Fh, short Sx, short Sy, short Sw, short Sh, short *global_aes);
short	mt_graf_slidebox	(OBJECT *, short Parent, short Object, short Direction, short *global_aes);
short	mt_graf_watchbox	(OBJECT *, short Object, short InState, short OutState, short *global_aes);
short	mt_graf_wwatchbox	(OBJECT *, short Object, short InState, short OutState, short whandle, short *global_aes);
/**@}*/

/** @addtogroup a_menu
 *  @{
 */
short 	mt_menu_attach		   (short me_flag, OBJECT *me_tree, short me_item, MENU *me_mdata, short *global);
short 	mt_menu_bar 		   (OBJECT *me_tree, short me_mode, short *global);
short	mt_menu_click		   (short click, short setit, short *global);
short 	mt_menu_icheck		   (OBJECT *me_tree, short me_item, short me_check, short *global);
short 	mt_menu_ienable 	   (OBJECT *me_tree, short me_item, short me_enable, short *global);
short 	mt_menu_istart		   (short me_flag, OBJECT *me_tree, short me_imenu, short me_item, short *global);
short 	mt_menu_popup		   (MENU *me_menu, short me_xpos, short me_ypos, MENU *me_mdata, short *global);
short 	mt_menu_register	   (short ap_id, char *me_text, short *global);
short 	mt_menu_settings	   (short me_flag, MN_SET *me_values, short *global);
short 	mt_menu_text		   (OBJECT *me_tree, short me_item, char *me_text, short *global);
short 	mt_menu_tnormal 	   (OBJECT *me_tree, short me_item, short me_normal, short *global);
short	mt_menu_unregister	   (short id, short *global);
/**@}*/

/** @addtogroup a_objc
 *  @{
 */
short	mt_objc_add		(OBJECT *, short Parent, short Child, short *global);
short	mt_objc_change	(OBJECT *, short Object, short Res,
						 short Cx, short Cy, short Cw, short Ch,
						 short NewState,short Redraw, short *global_aes);
short	mt_objc_delete	(OBJECT *, short Object, short *global_aes);
short	mt_objc_draw	(OBJECT *, short Start, short Depth,
						 short Cx, short Cy, short Cw, short Ch, short *global_aes);
short	mt_objc_edit	(OBJECT *, short Object, short Char, short *Index, short Kind, short *global_aes);
short	mt_objc_find	(OBJECT *, short Start, short Depth, short Mx, short My, short *global_aes);
short	mt_objc_xfind	(OBJECT *, short Start, short Depth, short Mx, short My, short *global_aes);
short	mt_objc_offset	(OBJECT *, short Object, short *X, short *Y, short *global_aes);
short	mt_objc_order	(OBJECT *, short Object, short NewPos, short *global_aes);
short	mt_objc_sysvar	(short mode, short which, short in1, short in2, short *out1, short *out2, short *global_aes);
/**@}*/

/** @addtogroup a_rsrc
 *  @{
 */
short	mt_rsrc_free	(short *global_aes);
short	mt_rsrc_gaddr	(short Type, short Index, void *Address, short *global_aes);
short	mt_rsrc_load	(const char *Name, short *global_aes);
short	mt_rsrc_obfix	(OBJECT *, short Index, short *global_aes);
short	mt_rsrc_rcfix	(void *rc_header, short *global_aes);
short	mt_rsrc_saddr	(short Type, short Index, void *Address, short *global_aes);
/**@}*/

/** @addtogroup a_scrp
 *  @{
 */
short	mt_scrp_clear 	(short *global_aes);
short	mt_scrp_read 	(char *Scrappath, short *global_aes);
short	mt_scrp_write 	(const char *Scrappath, short *global_aes);
/**@}*/

/** @addtogroup a_shel
 *  @{
 */
short	mt_shel_envrn 	(char **result, const char *param, short *global_aes);
short	mt_shel_find 	(char *buf, short *global_aes);
short 	mt_shel_get 	(char *Buf, short Len, short *global_aes);
short	mt_shel_help	(short sh_hmode, const char *sh_hfile, const char *sh_hkey, short *global_aes);
short 	mt_shel_put		(const char *Buf, short Len, short *global_aes);
short	mt_shel_rdef	(char *lpcmd, char *lpdir, short *global_aes);
short	mt_shel_read 	(char *Command, char *Tail, short *global_aes);
short	mt_shel_wdef	(const char *lpcmd, const char *lpdir, short *global_aes);
short	mt_shel_write	(short Exit, short Graphic, short Aes, void *Command, char *Tail, short *global_aes);
/**@}*/

/** @addtogroup a_wind
 *  @{
 */
short	mt_wind_calc 	(short Type, short Parts, short InX, short InY, short InW, short InH, short *OutX, short *OutY, short *OutW, short *OutH, short *global_aes);
short	mt_wind_close 	(short WindowHandle, short *global_aes);
short	mt_wind_create 	(short Parts, short Wx, short Wy, short Ww, short Wh, short *global_aes);
short	mt_wind_delete 	(short WindowHandle, short *global_aes);
short	mt_wind_draw 	(short WindowHandle, short startob, short *global_aes);
short	mt_wind_find 	(short X, short Y, short *global_aes);
short	mt_wind_get 	(short WindowHandle, short What, short *W1, short *W2, short *W3, short *W4, short *global_aes);
short	mt_wind_new 	(short *global_aes);
short	mt_wind_open 	(short WindowHandle, short Wx, short Wy, short Ww, short Wh, short *global_aes);
short	mt_wind_set 	(short WindowHandle, short What, short W1, short W2, short W3, short W4, short *global_aes);
short	mt_wind_update 	(short Code, short *global_aes);

/*
 * Some useful extensions
 */
short	mt_wind_calc_grect   (short Type, short Parts, const GRECT *In, GRECT *Out, short *global_aes);
short	mt_wind_create_grect (short Parts, const GRECT *r, short *global_aes);
short	mt_wind_get_grect    (short WindowHandle, short What, GRECT *r, short *global_aes);
short	mt_wind_open_grect   (short WindowHandle, const GRECT *r, short *global_aes);
short	mt_wind_set_grect    (short WindowHandle, short What, const GRECT *r, short *global_aes);
short	mt_wind_set_str      (short WindowHandle, short What, const char *str, short *global_aes);
/**@}*/

/** @addtogroup a_util
 *  @{
 */
short	rc_copy        (const GRECT *src, GRECT *dst);
short	rc_equal       (const GRECT *r1,  const GRECT *r2);
short	rc_intersect   (const GRECT *src, GRECT *dst);
GRECT * array_to_grect (const short *array, GRECT *area);
short * grect_to_array (const GRECT *area, short *array);
/**@}*/


/** @addtogroup AES
 *  @{
 */

/*
 * aes trap interface
 */

/* Array sizes in aes control block */

/** size of the aes_control[] array */
#define AES_CTRLMAX		5
/** size of the aes_global[] array */
#define AES_GLOBMAX		16
/** size of the aes_intin[] array */
#define AES_INTINMAX 		16
/** size of the aes_intout[] array */
#define AES_INTOUTMAX		16
/** size of the aes_addrin[] array */
#define AES_ADDRINMAX		16
/** size of the aes_addrout[] array */
#define AES_ADDROUTMAX		16

/** AES parameter block structure */
typedef struct
{
	short       *control;   /**< aes_control[] array */
	short       *global;    /**< aes_global[]  array */
	const short *intin;     /**< aes_intin[]   array */
	short       *intout;    /**< aes_intout[]  array */
	const long  *addrin;    /**< aes_addrin[]  array */
	long        *addrout;   /**< aes_addrout[] array */
} AESPB;

/** perform AES trap */
extern void aes (AESPB *pb);

/**@}*/

#endif /* AES */



#ifdef __GEMLIB_DEFS

/*******************************************************************************
 * The VDI specific stuff from old gemfast.h
 */

/* normal graphics drawing modes */
#define MD_REPLACE		1
#define MD_TRANS		2
#define MD_XOR			3
#define MD_ERASE		4

/* bit blt rules */
#define ALL_WHITE		0
#define S_AND_D			1
#define S_AND_NOTD		2
#define S_ONLY			3
#define NOTS_AND_D		4
#define D_ONLY			5
#define S_XOR_D			6
#define S_OR_D			7
#define NOT_SORD		8
#define NOT_SXORD		9
#define D_INVERT		10
#define NOT_D			10
#define S_OR_NOTD		11
#define NOT_S			12
#define NOTS_OR_D		13
#define NOT_SANDD		14
#define ALL_BLACK		15

/* v_bez modes */
#define BEZ_BEZIER		0x01
#define BEZ_POLYLINE		0x00
#define BEZ_NODRAW		0x02

/* v_bit_image modes */
#define IMAGE_LEFT		0
#define IMAGE_CENTER		1
#define IMAGE_RIGHT		2
#define IMAGE_TOP 		0
#define IMAGE_BOTTOM		2

/* v_justified modes */
#define NOJUSTIFY		0
#define JUSTIFY			1

/* vq_color modes */
#define COLOR_REQUESTED		0
#define COLOR_ACTUAL		1

/* return values for vq_vgdos() inquiry */
#define GDOS_NONE		(-2L) 		 /* no GDOS installed */
#define GDOS_FSM		0x5F46534DL /* '_FSM' */
#define GDOS_FNT		0x5F464E54L /* '_FNT' */

/* vqin_mode & vsin_mode modes */
#define VINMODE_LOCATOR		1
#define VINMODE_VALUATOR	2
#define VINMODE_CHOICE		3
#define VINMODE_STRING		4

#ifdef __GEMLIB_OLDNAMES
#define LOCATOR			1
#define VALUATOR		2
#define CHOICE			3
#define STRING			4
#endif

/* vqt_cachesize modes */
#define CACHE_CHAR		0
#define CACHE_MISC		1

/* vqt_devinfo return values */
#define DEV_MISSING		0
#define DEV_INSTALLED		1

/* vqt_name return values */
#define BITMAP_FONT		0

/* vsf_interior modes */
#define FIS_HOLLOW		0
#define FIS_SOLID		1
#define FIS_PATTERN		2
#define FIS_HATCH		3
#define FIS_USER		4

/* vsf_perimeter modes */
#define PERIMETER_OFF		0
#define PERIMETER_ON		1

/* vsl_ends modes */
#define SQUARE			0
#define ARROWED			1
/* #define ROUND			2 */

/* other names */
#define LE_SQUARED	SQUARE
#define LE_ARROWED	ARROWED
#define LE_ROUNDED	ROUND

/* vsl_type modes */
#define SOLID			1
#define LDASHED			2
#define DOTTED			3
#define DASHDOT			4
#define DASH			5
#define DASHDOTDOT		6
#define USERLINE		7

/* other names */
#define LT_SOLID		SOLID
#define LT_LONGDASH		LDASHED
#define LT_DOTTED		DOTTED
#define LT_DASHDOT		DASHDOT
#define LT_DASHED		DASH
#define LT_DASHDOTDOT	DASHDOTDOT
#define LT_USERDEF		USERLINE
#define LONGDASH		LDASHED
#define DOT				DOTTED
#define DASH2DOT		DASHDOTDOT

/* vsm_type modes */
#define MRKR_DOT		1
#define MRKR_PLUS 		2
#define MRKR_ASTERISK		3
#define MRKR_BOX		4
#define MRKR_CROSS		5
#define MRKR_DIAMOND		6

/* other names */
#define MT_DOT		MRKR_DOT
#define MT_PLUS		MRKR_PLUS
#define MT_ASTERISK	MRKR_ASTERISK
#define MT_SQUARE	MRKR_BOX
#define MT_DCROSS	MRKR_CROSS
#define MT_DIAMOND	MRKR_DIAMOND

/* vst_alignment modes */
#define TA_LEFT         	0 /* horizontal */
#define TA_CENTER       	1
#define TA_RIGHT        	2
#define TA_BASE         	0 /* vertical */
#define TA_HALF         	1
#define TA_ASCENT       	2
#define TA_BOTTOM       	3
#define TA_DESCENT      	4
#define TA_TOP          	5

/* vst_charmap modes */
#define MAP_BITSTREAM   0
#define MAP_ATARI       1
#define MAP_UNICODE     2 /* for vst_map_mode, NVDI 4 */

/* vst_effects modes */
#define TXT_NORMAL		0x0000
#define TXT_THICKENED		0x0001
#define TXT_LIGHT		0x0002
#define TXT_SKEWED		0x0004
#define TXT_UNDERLINED		0x0008
#define TXT_OUTLINED		0x0010
#define TXT_SHADOWED		0x0020

/* other names */
#define	TF_NORMAL		TXT_NORMAL
#define TF_THICKENED	TXT_THICKENED
#define TF_LIGHTENED	TXT_LIGHT
#define TF_SLANTED		TXT_SKEWED
#define TF_UNDERLINED	TXT_UNDERLINED
#define TF_OUTLINED		TXT_OUTLINED
#define TF_SHADOWED		TXT_SHADOWED


/* vst_error modes */
#define APP_ERROR		0
#define SCREEN_ERROR		1

/* vst_error return values */
#define NO_ERROR		0
#define CHAR_NOT_FOUND		1
#define FILE_READERR 		8
#define FILE_OPENERR 		9
#define BAD_FORMAT		10
#define CACHE_FULL		11
#define MISC_ERROR		(-1)

/* vst_kern tmodes */
#define TRACK_NONE		0
#define TRACK_NORMAL 		1
#define TRACK_TIGHT		2
#define TRACK_VERYTIGHT		3

/* vst_kern pmodes */
#define PAIR_OFF		0
#define PAIR_ON			1

/* vst_scratch modes */
#define SCRATCH_BOTH		0
#define SCRATCH_BITMAP		1
#define SCRATCH_NONE		2

/* v_updwk return values */
#define SLM_OK			0x00
#define SLM_ERROR		0x02
#define SLM_NOTONER		0x03
#define SLM_NOPAPER		0x04

/** VDI Memory Form Definition Block */
typedef struct memory_form
{
	void	*fd_addr;	/**< TODO */
	short 	fd_w;		/**< Form Width in Pixels */
	short 	fd_h; 		/**< Form Height in Pixels */
	short 	fd_wdwidth;	/**< Form Width in shorts(fd_w/sizeof(short) */
	short 	fd_stand;	/**< Form format 0= device spec 1=standard */
	short 	fd_nplanes;	/**< Number of memory planes */
	short 	fd_r1;		/**< Reserved */
	short 	fd_r2;		/**< Reserved */
	short 	fd_r3;		/**< Reserved */
} MFDB;

/** RGB intesities in promille */
typedef struct rgb_1000
{
	short  red;    /**< Red-Intensity in range [0..1000] */
 	short  green;  /**< Green-Intensity in range [0..1000] */
 	short  blue;   /**< Blue-Intensity in range [0..1000] */
} RGB1000;

#endif

#ifdef __GEMLIB_VDI

/*******************************************************************************
 * The VDI bindings from old vdibind.h
 */

typedef short VdiHdl;   /**< for better readability */


/*
 * attribute functions
 */

/** @addtogroup v_attr
 *  @{
 */
void  vs_color  (VdiHdl , short color_idx, short rgb[]);
short vswr_mode (VdiHdl , short mode);

short vsf_color     (VdiHdl , short color_idx);
short vsf_interior  (VdiHdl , short style);
short vsf_perimeter (VdiHdl , short vis);
short vsf_xperimeter(VdiHdl , short vis, short style);
short vsf_style     (VdiHdl , short style);
void  vsf_udpat     (VdiHdl , short pat[], short planes);

short vsl_color (VdiHdl , short color_idx);
void  vsl_ends  (VdiHdl , short begstyle, short endstyle);
short vsl_type  (VdiHdl , short style);
void  vsl_udsty (VdiHdl , short pat);
short vsl_width (VdiHdl , short width);

short vsm_color  (VdiHdl , short color_idx);
short vsm_height (VdiHdl , short height);
short vsm_type   (VdiHdl , short symbol);

void  vst_alignment (VdiHdl , short hin, short vin, short *hout, short *vout);
short vst_color     (VdiHdl , short color_idx);
short vst_effects   (VdiHdl , short effects);
void  vst_error     (VdiHdl , short mode, short *errorvar);
short vst_font      (VdiHdl , short font);
void  vst_height    (VdiHdl , short height, short *charw, short *charh,
                                            short *cellw, short *cellh);
short vst_point     (VdiHdl , short point, short *charw, short *charh,
                                           short *cellw, short *cellh);
short vst_rotation  (VdiHdl , short ang);
void  vst_scratch   (VdiHdl , short mode);
/**@}*/

/*
 * control functions
 */

/** @addtogroup v_ctrl
 *  @{
 */
void  v_clrwk          (VdiHdl );
void  v_clsvwk         (VdiHdl );
void  v_clswk          (VdiHdl );
short v_flushcache     (VdiHdl );
short v_loadcache      (VdiHdl , const char *filename, short mode);
void  v_opnvwk         (short work_in[], VdiHdl *, short work_out[]);
void  v_opnwk          (short work_in[], VdiHdl *, short work_out[]);
short v_savecache      (VdiHdl , const char *filename);
void  v_set_app_buff   (VdiHdl , void *buf_p, short size);
void  v_updwk          (VdiHdl );
void  vs_clip          (VdiHdl , short clip_flag, short pxy[]);
void  vs_clip_pxy      (VdiHdl , PXY pxy[]);
void  vs_clip_off      (VdiHdl );
short vst_load_fonts   (VdiHdl , short /* select */);
void  vst_unload_fonts (VdiHdl , short /* select */);
/**@}*/

/*
 * escape functions
 */

/** @addtogroup v_escp
 *  @{
 */
void  v_bit_image       (VdiHdl , const char *filename, short aspect,
                                  short x_scale, short y_scale,
                                  short h_align, short v_align, short *pxy);
void  v_clear_disp_list (VdiHdl );
short v_copies          (VdiHdl , short count);
void  v_dspcur          (VdiHdl , short x, short y);
void  v_form_adv        (VdiHdl );
void  v_hardcopy        (VdiHdl );
short v_orient          (VdiHdl , short orientation);
void  v_output_window   (VdiHdl , short *pxy);
short v_page_size       (VdiHdl , short page_id);
void  v_rmcur           (VdiHdl );
short v_trays           (VdiHdl , short input, short output,
                                  short *set_input, short *set_output);
short vq_calibrate      (VdiHdl , short *flag);
short vq_page_name      (VdiHdl , short page_id, char *page_name,
                                  long *page_width, long *page_height);
void  vq_scan           (VdiHdl , short *g_slice, short *g_page,
                                  short *a_slice, short *a_page, short *div_fac);
short vq_tabstatus      (VdiHdl );
short vq_tray_names     (VdiHdl , char *input_name, char *output_name,
                                  short *input, short *output);
short vs_calibrate      (VdiHdl , short flag, short *rgb);
short vs_palette        (VdiHdl , short palette);

void  v_sound		(VdiHdl, short freq, short duration);
short vs_mute		(VdiHdl, short action);

void vq_tdimensions (VdiHdl , short *xdimension, short *ydimension);
void vt_alignment   (VdiHdl , short dx, short dy);
void vt_axis        (VdiHdl , short xres, short yres, short *xset, short *yset);
void vt_origin      (VdiHdl , short xorigin, short yorigin);
void vt_resolution  (VdiHdl , short xres, short yres, short *xset, short *yset);

void v_meta_extents (VdiHdl , short min_x, short min_y,
                              short max_x, short max_y);
void v_write_meta   (VdiHdl , short numvdi_intin, short *avdi_intin,
                              short num_ptsin, short *a_ptsin);
void vm_coords      (VdiHdl , short llx, short lly, short urx, short ury);
void vm_filename    (VdiHdl , const char *filename);
void vm_pagesize    (VdiHdl , short pgwidth, short pgheight);

void  vsc_expose	(VdiHdl , short state);
void  vsp_film		(VdiHdl , short color_idx, short lightness);
short vqp_filmname	(VdiHdl , short _index, char * name);

void v_offset		(VdiHdl , short offset);
void v_fontinit		(VdiHdl , const void * font_header);

void v_escape2000 (VdiHdl , short times);

void v_alpha_text  (VdiHdl , const char *str);
void v_curdown     (VdiHdl );
void v_curhome     (VdiHdl );
void v_curleft     (VdiHdl );
void v_curright    (VdiHdl );
void v_curtext     (VdiHdl , const char *str);
void v_curup       (VdiHdl );
void v_eeol        (VdiHdl );
void v_eeos        (VdiHdl );
void v_enter_cur   (VdiHdl );
void v_exit_cur    (VdiHdl );
void v_rvoff       (VdiHdl );
void v_rvon        (VdiHdl );
void vq_chcells    (VdiHdl , short *n_rows, short *n_cols);
void vq_curaddress (VdiHdl , short *cur_row, short *cur_col);
void vs_curaddress (VdiHdl , short row, short col);
/* alternative name for vs_curaddress */
#define v_curaddress vs_curaddress
/**@}*/


/*
 * inquiry functions
 */

/** @addtogroup v_inqr
 *  @{
 */
void  vq_cellarray   (VdiHdl , short pxy[], short row_len, short nrows,
                               short *el_used, short *rows_used,
                               short *status, short color[]);
short vq_color       (VdiHdl , short color_idx, short flag, short rgb[]);
void  vq_extnd       (VdiHdl , short flag, short work_out[]);
void  vqf_attributes (VdiHdl , short atrib[]);
void  vqin_mode      (VdiHdl , short dev, short *mode);
void  vql_attributes (VdiHdl , short atrib[]);
void  vqm_attributes (VdiHdl , short atrib[]);
void  vqt_attributes (VdiHdl , short atrib[]);
void  vqt_cachesize  (VdiHdl , short which_cache, long *size);
void  vqt_extent     (VdiHdl , const char *str, short extent[]);
void  vqt_extent16   (VdiHdl , const short *wstr, short extent[]);
void  vqt_extent16n  (VdiHdl , const short *wstr, short num, short extent[]);
void  vqt_fontinfo   (VdiHdl , short *minade, short *maxade, short distances[],
                               short *maxwidth, short effects[]);
void  vqt_get_table  (VdiHdl , short **map);
short vqt_name       (VdiHdl , short element, char *name);
short vqt_width      (VdiHdl , short chr, short *cw,
                               short *ldelta, short *rdelta);
/** TODO */
short vq_gdos  (void);

/** TODO */
long  vq_vgdos (void);
/**@}*/


/*
 * input function
 */

/** @addtogroup v_inpt
 *  @{
 */
void  v_hide_c     (VdiHdl );
void  v_show_c     (VdiHdl , short reset);
void  vex_butv     (VdiHdl , void *pusrcode, void **psavcode);
void  vex_curv     (VdiHdl , void *pusrcode, void **psavcode);
void  vex_motv     (VdiHdl , void *pusrcode, void **psavcode);
void  vex_wheelv   (VdiHdl , void *pusrcode, void **psavcode);
void  vex_timv     (VdiHdl , void *time_addr,
                             void **otime_addr, short *time_conv);
void  vq_key_s     (VdiHdl , short *state);
void  vq_mouse     (VdiHdl , short *pstatus, short *x, short *y);
void  vrq_choice   (VdiHdl , short cin, short *cout);
void  vrq_locator  (VdiHdl , short x, short y,
                             short *xout, short *yout, short *term);
void  vrq_string   (VdiHdl , short len, short echo, short echoxy[], char *str);
void  vrq_valuator (VdiHdl , short in, short *out, short *term);
void  vsc_form     (VdiHdl , short form[]);
short vsin_mode    (VdiHdl , short dev, short mode);
short vsm_choice   (VdiHdl , short *choice);
short vsm_locator  (VdiHdl , short x, short y,
                             short *xout, short *yout, short *term);
short vsm_string   (VdiHdl , short len, short echo, short echoxy[], char *str);
void  vsm_valuator (VdiHdl , short in, short *out, short *term, short *status);
/**@}*/


/*
 * output functions
 */

/** @addtogroup v_outp
 *  @{
 */
void v_arc         (VdiHdl , short x, short y,
                             short radius, short begang, short endang);
void v_bar         (VdiHdl , short pxy[]);
void v_cellarray   (VdiHdl , short pxy[], short row_length, short elements,
                             short nrows, short write_mode, short colarray[]);
void v_circle      (VdiHdl , short x, short y, short radius);
void v_contourfill (VdiHdl , short x, short y, short color_idx);
void v_ellarc      (VdiHdl , short x, short y, short xrad, short yrad,
                             short begang, short endang);
void v_ellipse     (VdiHdl , short x, short y, short xrad, short yrad);
void v_ellpie      (VdiHdl , short x, short y, short xrad, short yrad,
                             short begang, short endang);
void v_fillarea    (VdiHdl , short count, short pxy[]);
void v_gtext       (VdiHdl , short x, short y, const char *str);
void v_gtext16     (VdiHdl , short x, short y, const short *wstr);
void v_gtext16n    (VdiHdl , PXY pos, const short *wstr, short num);
void v_justified   (VdiHdl , short x, short y, const char *str,
                             short len, short word_space, short char_space);
void v_pieslice    (VdiHdl , short x, short y,
                             short radius, short begang, short endang);
void v_pline       (VdiHdl , short count, short pxy[]);
void v_pmarker     (VdiHdl , short count, short pxy[]);
void v_rbox        (VdiHdl , short pxy[]);
void v_rfbox       (VdiHdl , short pxy[]);
void vr_recfl      (VdiHdl , short pxy[]);
/**@}*/

/*
 * raster functions
 */

/** @addtogroup v_rstr
 *  @{
 */
void v_get_pixel (VdiHdl , short x, short y, short *pel, short *color_idx);
void vr_trnfm    (VdiHdl , MFDB *src, MFDB *dst);
void vro_cpyfm   (VdiHdl , short mode, short pxy[], MFDB *src, MFDB *dst);
void vrt_cpyfm   (VdiHdl , short mode, short pxy[], MFDB *src, MFDB *dst,
                           short color[]);
/**@}*/


/*
 * Some usefull extensions.
 */

/** @addtogroup v_util
 *  @{
 */
void  vdi_array2str (const short *src, char  *des, short len);
short vdi_str2array (const char  *src, short *des);
short vdi_wstrlen   (const short *wstr);
/**@}*/

/*
 * vdi trap interface
 */

/** @addtogroup VDI
 *  @{
 */

/* Array sizes in vdi control block */
#define VDI_CNTRLMAX     15		/**< max size of vdi_control[] */
#define VDI_INTINMAX   1024		/**< max size of vdi_intin[] */
#define VDI_INTOUTMAX   256		/**< max size of vdi_intout[] */
#define VDI_PTSINMAX    256		/**< max size of vdi_ptsin[] */
#define VDI_PTSOUTMAX   256		/**< max size of vdi_ptsout[] */

/** TODO */
typedef struct
{
	short       *control;	/**< TODO */
	const short *intin;		/**< TODO */
	const short *ptsin;		/**< TODO */
	short       *intout;	/**< TODO */
	short       *ptsout;	/**< TODO */
} VDIPB;

/** TODO */
void vdi (VDIPB *pb);
/**@}*/

#endif /* VDI */


__END_DECLS

#endif /* _MT_GEMLIB_H_ */
