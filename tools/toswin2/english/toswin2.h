/*
 * resource set indices for toswin2
 *
 * created by ORCS 2.16
 */

/*
 * Number of Strings:        182
 * Number of Bitblks:        0
 * Number of Iconblks:       0
 * Number of Color Iconblks: 3
 * Number of Color Icons:    3
 * Number of Tedinfos:       28
 * Number of Free Strings:   17
 * Number of Free Images:    0
 * Number of Objects:        137
 * Number of Trees:          10
 * Number of Userblks:       0
 * Number of Images:         0
 * Total file size:          11518
 */

#undef RSC_NAME
#ifndef __ALCYON__
#define RSC_NAME "toswin2"
#endif
#undef RSC_ID
#ifdef toswin2
#define RSC_ID toswin2
#else
#define RSC_ID 0
#endif

#ifndef RSC_STATIC_FILE
# define RSC_STATIC_FILE 0
#endif
#if !RSC_STATIC_FILE
#define NUM_STRINGS 182
#define NUM_FRSTR 17
#define NUM_UD 0
#define NUM_IMAGES 0
#define NUM_BB 0
#define NUM_FRIMG 0
#define NUM_IB 0
#define NUM_CIB 3
#define NUM_TI 28
#define NUM_OBS 137
#define NUM_TREE 10
#endif



#define VERSION            0 /* form/dialog */
#define RSC_VERSION        3 /* STRING in tree VERSION */

#define MENUTREE           1 /* menu */
#define TDESK              3 /* TITLE in tree MENUTREE */
#define TDATEI             4 /* TITLE in tree MENUTREE */
#define TFENSTER           5 /* TITLE in tree MENUTREE */
#define TOPTION            6 /* TITLE in tree MENUTREE */
#define MABOUT             9 /* STRING in tree MENUTREE */
#define MPROG             18 /* STRING in tree MENUTREE */
#define MSHELL            19 /* STRING in tree MENUTREE */
#define MENDE             21 /* STRING in tree MENUTREE */
#define MCONSOLE          23 /* STRING in tree MENUTREE */
#define MCYCLE            25 /* STRING in tree MENUTREE */
#define MCLOSE            26 /* STRING in tree MENUTREE */
#define MAVCYCLE          28 /* STRING in tree MENUTREE */
#define MSHORTCUT         29 /* STRING in tree MENUTREE */
#define MALLOGIN          30 /* STRING in tree MENUTREE */
#define MCCONFIG          32 /* STRING in tree MENUTREE */
#define MWCONFIG          33 /* STRING in tree MENUTREE */
#define MSAVE             35 /* STRING in tree MENUTREE */

#define ABOUT              2 /* form/dialog */
#define AVERSION           4 /* FTEXT in tree ABOUT */ /* max len 4 */
#define ADATUM             5 /* TEXT in tree ABOUT */ /* max len 11 */
#define AVT100             9 /* TEXT in tree ABOUT */ /* max len 33 */
#define ACOMP             11 /* TEXT in tree ABOUT */ /* max len 5 */
#define AMINT             13 /* TEXT in tree ABOUT */ /* max len 11 */
#define AGEM              15 /* TEXT in tree ABOUT */ /* max len 11 */
#define ACF               17 /* TEXT in tree ABOUT */ /* max len 11 */

#define ARG                3 /* form/dialog */
#define ARGPROG            2 /* STRING in tree ARG */
#define ARGSTR             3 /* FTEXT in tree ARG */ /* max len 50 */
#define ARGOK              4 /* BUTTON in tree ARG */
#define ARGABBRUCH         5 /* BUTTON in tree ARG */

#define CONCONFIG          4 /* form/dialog */
#define CAUTO              3 /* BUTTON in tree CONCONFIG */
#define COUTPUT            4 /* BUTTON in tree CONCONFIG */
#define CLOGACTIVE         6 /* BUTTON in tree CONCONFIG */
#define CLOGSEL            7 /* BUTTON in tree CONCONFIG */
#define CLOGNAME           8 /* TEXT in tree CONCONFIG */ /* max len 19 */
#define CHELP              9 /* BOXTEXT in tree CONCONFIG */ /* max len 4 */
#define COK               10 /* BUTTON in tree CONCONFIG */
#define CABBRUCH          11 /* BUTTON in tree CONCONFIG */

#define WINCONFIG          5 /* form/dialog */
#define WPROG              2 /* TEXT in tree WINCONFIG */ /* max len 40 */
#define WCOL               5 /* FTEXT in tree WINCONFIG */ /* max len 3 */
#define WROW               7 /* FTEXT in tree WINCONFIG */ /* max len 3 */
#define WBUFFER           10 /* FTEXT in tree WINCONFIG */ /* max len 5 */
#define WGBOX             13 /* BOX in tree WINCONFIG */
#define WGCLOSER          14 /* BOXCHAR in tree WINCONFIG */
#define WGTITLE           15 /* BOXTEXT in tree WINCONFIG */ /* max len 5 */
#define WGSMALLER         16 /* BOXCHAR in tree WINCONFIG */
#define WGFULLER          17 /* BOXCHAR in tree WINCONFIG */
#define WGSLVERT          18 /* BOX in tree WINCONFIG */
#define WGSLHOR           19 /* BOX in tree WINCONFIG */
#define WGSIZER           20 /* BOXCHAR in tree WINCONFIG */
#define WTITLE            22 /* FTEXT in tree WINCONFIG */ /* max len 23 */
#define WFBOX             23 /* BUTTON in tree WINCONFIG */
#define WFNAME            24 /* TEXT in tree WINCONFIG */ /* max len 24 */
#define WFSIZE            26 /* TEXT in tree WINCONFIG */ /* max len 3 */
#define WFSEL             27 /* BUTTON in tree WINCONFIG */
#define WTABSTR           28 /* STRING in tree WINCONFIG */
#define WTAB              29 /* BOXTEXT in tree WINCONFIG */ /* max len 11 */
#define WTERMSTR          31 /* STRING in tree WINCONFIG */
#define WTERM             32 /* BOXTEXT in tree WINCONFIG */ /* max len 6 */
#define WFGSTR            33 /* STRING in tree WINCONFIG */
#define WFGCOL            34 /* BOX in tree WINCONFIG */
#define WBGSTR            35 /* STRING in tree WINCONFIG */
#define WBGCOL            36 /* BOX in tree WINCONFIG */
#define WWRAP             38 /* BUTTON in tree WINCONFIG */
#define WCLOSE            39 /* BUTTON in tree WINCONFIG */
#define WHELP             40 /* BOXTEXT in tree WINCONFIG */ /* max len 4 */
#define WOK               41 /* BUTTON in tree WINCONFIG */
#define WABBRUCH          42 /* BUTTON in tree WINCONFIG */

#define WINICON            6 /* form/dialog */
#define WIBOX              0 /* BOX in tree WINICON */
#define WICON              1 /* CICON in tree WINICON */ /* max len 1 */

#define CONICON            7 /* form/dialog */

#define STRINGS            8 /* form/dialog */
#define STRPROGSEL         1 /* STRING in tree STRINGS */
#define STRFONTSEL         2 /* STRING in tree STRINGS */
#define STRCFGPROG         3 /* STRING in tree STRINGS */
#define STRCFGSTD          4 /* STRING in tree STRINGS */
#define STRLOGSEL          5 /* STRING in tree STRINGS */

#define POPUPS             9 /* form/dialog */
#define TERMPOP            1 /* BOX in tree POPUPS */
#define TP52               2 /* STRING in tree POPUPS */
#define TP100              3 /* STRING in tree POPUPS */
#define TABPOP             4 /* BOX in tree POPUPS */
#define TPATARI            5 /* STRING in tree POPUPS */
#define TPISO              6 /* STRING in tree POPUPS */

#define NOAES41            0 /* Alert string */
/* [3][TosWin2 needs|MiNT and AES 4.1!][Test|Cancel] */

#define NOSCRAP            1 /* Alert string */
/* [1][Path to clipboard|is not set.][Cancel] */

#define SCRAPERR           2 /* Alert string */
/* [1][Error while writing|to clipboard.][Cancel] */

#define SCRAPMEM           3 /* Alert string */
/* [1][Not enough memory|for clipboard.][Cancel] */

#define SCRAPEMPTY         4 /* Alert string */
/* [1][Clipboard is empty.][OK] */

#define NOXFSL             5 /* Alert string */
/* [1][No Fontselector|installed!][Cancel] */

#define NOTEXT             6 /* Alert string */
/* [1][Not enough memory for|another window.][Cancel] */

#define NOPTY              7 /* Alert string */
/* [1][No more free PTY.][Cancel] */

#define NOFORK             8 /* Alert string */
/* [1][Error at fork().][Cancel] */

#define NOPWD              9 /* Alert string */
/* [1][Userdata in/etc/passwd|invalid or not present.][Cancel] */

#define LOADERR           10 /* Alert string */
/* [1][Error while reading|configuration file.][Cancel] */

#define SAVEERR           11 /* Alert string */
/* [1][Error while writing|configuration file.][Cancel] */

#define XCONERR           12 /* Alert string */
/* [1][Error while opening|Xconout2-device.][Cancel] */

#define IDERR             13 /* Alert string */
/* [1][Configuration file|invalid (wrong ID).][Cancel] */

#define NOSTGUIDE         14 /* Alert string */
/* [1][ST-Guide is not|installed!][OK] */

#define NODDSEND          15 /* Alert string */
/* [3][WF_OWNER is not handled|correct, Drag&Drop is not|possible.][Shame] */

#define NOTWCALL          16 /* Alert string */
/* [3][Communication error:|tw-call don't respond!?][Cancel] */




#ifdef __STDC__
#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  else
#    define _WORD short
#  endif
#endif
extern _WORD toswin2_rsc_load(_WORD wchar, _WORD hchar);
extern _WORD toswin2_rsc_gaddr(_WORD type, _WORD idx, void *gaddr);
extern _WORD toswin2_rsc_free(void);
#endif
