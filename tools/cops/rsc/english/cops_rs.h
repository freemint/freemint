/*
 * resource set indices for cops_rs
 *
 * created by ORCS 2.16
 */

/*
 * Number of Strings:        67
 * Number of Bitblks:        2
 * Number of Iconblks:       1
 * Number of Color Iconblks: 1
 * Number of Color Icons:    1
 * Number of Tedinfos:       4
 * Number of Free Strings:   13
 * Number of Free Images:    0
 * Number of Objects:        60
 * Number of Trees:          9
 * Number of Userblks:       0
 * Number of Images:         4
 * Total file size:          6720
 */

#undef RSC_NAME
#ifndef __ALCYON__
#define RSC_NAME "cops_rs"
#endif
#undef RSC_ID
#ifdef cops_rs
#define RSC_ID cops_rs
#else
#define RSC_ID 0
#endif

#ifndef RSC_STATIC_FILE
# define RSC_STATIC_FILE 0
#endif
#if !RSC_STATIC_FILE
#define NUM_STRINGS 67
#define NUM_FRSTR 13
#define NUM_UD 0
#define NUM_IMAGES 4
#define NUM_BB 2
#define NUM_FRIMG 0
#define NUM_IB 1
#define NUM_CIB 1
#define NUM_TI 4
#define NUM_OBS 60
#define NUM_TREE 9
#endif



extern TEDINFO rs_tedinfo[];
extern CICONBLK rs_ciconblk[];
extern CICON rs_cicon[];
extern ICONBLK rs_iconblk[];
extern BITBLK rs_bitblk[];
extern char *rs_frstr[];
extern BITBLK *rs_frimg[];
extern OBJECT rs_object[];
extern OBJECT *rs_trindex[];


#define GNL_POPUP          0 /* form/dialog */
#define PG_ABOUT           1 /* STRING in tree GNL_POPUP */
#define PG_SETTINGS        3 /* STRING in tree GNL_POPUP */
#define PG_RELOAD          5 /* STRING in tree GNL_POPUP */
#define PG_TIDY_UP         6 /* STRING in tree GNL_POPUP */
#define PG_SELECT_ALL      8 /* STRING in tree GNL_POPUP */
#define PG_HELP           10 /* STRING in tree GNL_POPUP */

#define CPX_POPUP          1 /* form/dialog */
#define CP_OPEN            1 /* STRING in tree CPX_POPUP */
#define CP_DISABLE         3 /* STRING in tree CPX_POPUP */
#define CP_ENABLE          4 /* STRING in tree CPX_POPUP */
#define CP_INFO            6 /* STRING in tree CPX_POPUP */

#define ABOUT_DIALOG       2 /* form/dialog */
#define ABOUT_ICON         4 /* BOX in tree ABOUT_DIALOG */

#define INFO_DIALOG        3 /* form/dialog */
#define CITITLE            1 /* STRING in tree INFO_DIALOG */
#define CIFILE             3 /* STRING in tree INFO_DIALOG */
#define CIVER              5 /* STRING in tree INFO_DIALOG */
#define CIID               7 /* STRING in tree INFO_DIALOG */
#define CIRAM              9 /* BUTTON in tree INFO_DIALOG */
#define CISET             10 /* BUTTON in tree INFO_DIALOG */
#define CIBOOT            11 /* BUTTON in tree INFO_DIALOG */
#define CIAUTO            12 /* BUTTON in tree INFO_DIALOG */
#define CIOK              13 /* BUTTON in tree INFO_DIALOG */
#define CICANCEL          14 /* BUTTON in tree INFO_DIALOG */

#define SET_DIALOG         4 /* form/dialog */
#define ACSELPATH          2 /* BUTTON in tree SET_DIALOG */
#define ACPATH             3 /* FTEXT in tree SET_DIALOG */ /* max len 20 */
#define ACBOOT             4 /* BUTTON in tree SET_DIALOG */
#define ACCLICK            5 /* BUTTON in tree SET_DIALOG */
#define ACSORTNAME         6 /* BUTTON in tree SET_DIALOG */
#define ACTERM             7 /* BUTTON in tree SET_DIALOG */
#define ACAFTER            8 /* FTEXT in tree SET_DIALOG */ /* max len 2 */
#define ACOK               9 /* BUTTON in tree SET_DIALOG */

#define ICONIFIED_DIALOG   5 /* form/dialog */
#define ICFDLG_ROOT        0 /* BOX in tree ICONIFIED_DIALOG */
#define ICFDLG_ICON        1 /* BOX in tree ICONIFIED_DIALOG */

#define EMPTY_TREE         6 /* form/dialog */

#define ICON_DIALOG        7 /* form/dialog */
#define COPS_ICON          1 /* ICON in tree ICON_DIALOG */ /* max len 1 */
#define INACTIVE_IMG       2 /* IMAGE in tree ICON_DIALOG */

#define CICON_DIALOG       8 /* form/dialog */
#define COPS_CICON         1 /* CICON in tree CICON_DIALOG */ /* max len 1 */

#define NOWINDOW_ALERT     0 /* Alert string */
/* [1][No more windows.][Cancel] */

#define RELOAD_ALERT       1 /* Alert string */
/* [2][Reload Control Panels?][Yes|No] */

#define SAVE_DFLT_ALERT    2 /* Alert string */
/* [2][Save Defaults?][Yes|No] */

#define MEM_ERR_ALERT      3 /* Alert string */
/* [3][Not enough memory!][OK] */

#define FILE_ERR_ALERT     4 /* Alert string */
/* [3][File error.][OK] */

#define FNF_ERR_ALERT      5 /* Alert string */
/* [1][File not found.][OK] */

#define NOWDIALOG_ALERT    6 /* Alert string */
/* [1][Please run the system extension|WDIALOG, which is supplied with|NVDI, or use the multitasking|system MagiC 4.][Quit] */

#define CPXLOAD_ALERT      7 /* Alert string */
/* [1][Cannot open the control panel.][OK] */

#define QUIT_ALERT         8 /* Alert string */
/* [2][Terminate COPS?][Yes|No] */

#define NOAES_STR          9 /* Free string */
/* AES is inactive. */

#define CPXPATH_STR       10 /* Free string */
/* Select CPX Path */

#define MENUTITLE_STR     11 /* Free string */
/*   Control Panels   */

#define ICNFTITLE_STR     12 /* Free string */
/*  COPS  */




#ifdef __STDC__
#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  else
#    define _WORD short
#  endif
#endif
extern _WORD cops_rs_rsc_load(_WORD wchar, _WORD hchar);
extern _WORD cops_rs_rsc_gaddr(_WORD type, _WORD idx, void *gaddr);
extern _WORD cops_rs_rsc_free(void);
#endif
