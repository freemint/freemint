/*
 * resource set indices for xaaes
 *
 * created by ORCS 2.18
 */

/*
 * Number of Strings:        412
 * Number of Bitblks:        0
 * Number of Iconblks:       2
 * Number of Color Iconblks: 30
 * Number of Color Icons:    38
 * Number of Tedinfos:       29
 * Number of Free Strings:   115
 * Number of Free Images:    0
 * Number of Objects:        290
 * Number of Trees:          13
 * Number of Userblks:       0
 * Number of Images:         4
 * Total file size:          74674
 */

#undef RSC_NAME
#ifndef __ALCYON__
#define RSC_NAME "xaaes"
#endif
#undef RSC_ID
#ifdef xaaes
#define RSC_ID xaaes
#else
#define RSC_ID 0
#endif

#ifndef RSC_STATIC_FILE
# define RSC_STATIC_FILE 0
#endif
#if !RSC_STATIC_FILE
#define NUM_STRINGS 412
#define NUM_FRSTR 115
#define NUM_UD 0
#define NUM_IMAGES 4
#define NUM_BB 0
#define NUM_FRIMG 0
#define NUM_IB 2
#define NUM_CIB 30
#define NUM_TI 29
#define NUM_OBS 290
#define NUM_TREE 13
#endif



#define SYSTEM_MENU                        0 /* menu */
#define SYS_DESK                           3 /* TITLE in tree SYSTEM_MENU */
#define SYS_MN_ABOUT                       7 /* STRING in tree SYSTEM_MENU */
#define SYS_MN_LAUNCH                     16 /* STRING in tree SYSTEM_MENU */
#define SYS_MN_DESK                       17 /* STRING in tree SYSTEM_MENU */
#define SYS_MN_TASKMNG                    19 /* STRING in tree SYSTEM_MENU */
#define SYS_MN_SYSTEM                     20 /* STRING in tree SYSTEM_MENU */
#define SYS_MN_QUITAPP                    22 /* STRING in tree SYSTEM_MENU */
#define SYS_MN_RESTART                    23 /* STRING in tree SYSTEM_MENU */
#define SYS_MN_QUIT                       24 /* STRING in tree SYSTEM_MENU */

#define FSEL_MENU                          1 /* menu */
#define FSEL_DRV                           3 /* TITLE in tree FSEL_MENU */
#define FSEL_FILTER                        4 /* TITLE in tree FSEL_MENU */
#define FSEL_OPTS                          5 /* TITLE in tree FSEL_MENU */
#define FSEL_FILE                          6 /* TITLE in tree FSEL_MENU */
#define FSEL_DRVBOX                        8 /* BOX in tree FSEL_MENU */
#define FSEL_DRVA                          9 /* STRING in tree FSEL_MENU */
#define FSEL_PATBOX                       45 /* BOX in tree FSEL_MENU */
#define FSEL_PATA                         46 /* STRING in tree FSEL_MENU */
#define FSM_RTBUILD                       70 /* STRING in tree FSEL_MENU */
#define FSM_TREEVIEW                      71 /* STRING in tree FSEL_MENU */
#define FSM_SORTBYNAME                    73 /* STRING in tree FSEL_MENU */
#define FSM_SORTBYDATE                    74 /* STRING in tree FSEL_MENU */
#define FSM_SORTBYSIZE                    75 /* STRING in tree FSEL_MENU */
#define FSM_SORTBYEXT                     76 /* STRING in tree FSEL_MENU */
#define FSM_NOSORT                        77 /* STRING in tree FSEL_MENU */
#define FSM_REVSORT                       79 /* STRING in tree FSEL_MENU */
#define FSM_COPY                          81 /* STRING in tree FSEL_MENU */
#define FSM_NEW                           82 /* STRING in tree FSEL_MENU */
#define FSM_RENAME                        83 /* STRING in tree FSEL_MENU */
#define FSM_VIEW                          84 /* STRING in tree FSEL_MENU */
#define FSM_DELETE                        85 /* STRING in tree FSEL_MENU */

#define ALERT_ICONS                        2 /* form/dialog */
#define ALR_IC_WARNING                     1 /* CICON in tree ALERT_ICONS */
#define ALR_IC_QUESTION                    2 /* CICON in tree ALERT_ICONS */
#define ALR_IC_STOP                        3 /* CICON in tree ALERT_ICONS */
#define ALR_IC_INFO                        4 /* CICON in tree ALERT_ICONS */
#define ALR_IC_DRIVE                       5 /* CICON in tree ALERT_ICONS */
#define ALR_IC_BOMB                        6 /* CICON in tree ALERT_ICONS */
#define ALR_IC_SYSTEM                      7 /* CICON in tree ALERT_ICONS */

#define ALERT_BOX                          3 /* form/dialog */
#define ALERT_BACK                         0 /* BOX in tree ALERT_BOX */
#define ALERT_D_ICON                       1 /* CICON in tree ALERT_BOX */
#define ALERT_T1                           2 /* STRING in tree ALERT_BOX */
#define ALERT_T2                           3 /* STRING in tree ALERT_BOX */
#define ALERT_T3                           4 /* STRING in tree ALERT_BOX */
#define ALERT_T4                           5 /* STRING in tree ALERT_BOX */
#define ALERT_T5                           6 /* STRING in tree ALERT_BOX */
#define ALERT_T6                           7 /* STRING in tree ALERT_BOX */
#define ALERT_BUT1                         8 /* BUTTON in tree ALERT_BOX */
#define ALERT_BUT2                         9 /* BUTTON in tree ALERT_BOX */
#define ALERT_BUT3                        10 /* BUTTON in tree ALERT_BOX */
#define ALERT_BUT4                        11 /* BUTTON in tree ALERT_BOX */

#define ABOUT_XAAES                        4 /* form/dialog */
#define ABOUT_LIST                         5 /* BOX in tree ABOUT_XAAES */
#define ABOUT_VERSION                      7 /* STRING in tree ABOUT_XAAES */
#define ABOUT_DATE                         9 /* STRING in tree ABOUT_XAAES */
#define ABOUT_OK                          10 /* BUTTON in tree ABOUT_XAAES */
#define ABOUT_TARGET                      12 /* STRING in tree ABOUT_XAAES */

#define DEF_DESKTOP                        5 /* form/dialog */
#define DESKTOP                            0 /* BOX in tree DEF_DESKTOP */
#define DESKTOP_LOGO                       1 /* CICON in tree DEF_DESKTOP */

#define TASK_MANAGER                       6 /* form/dialog */
#define TM_ICONS                           1 /* IBOX in tree TASK_MANAGER */
#define TM_ICN_HALTED                      2 /* CICON in tree TASK_MANAGER */
#define TM_ICN_LOCK                        3 /* CICON in tree TASK_MANAGER */
#define TM_ICN_MENU                        4 /* ICON in tree TASK_MANAGER */
#define TM_ICN_MESSAGE                     5 /* CICON in tree TASK_MANAGER */
#define TM_ICN_XAAES                       6 /* ICON in tree TASK_MANAGER */
#define TM_PICS                            7 /* IBOX in tree TASK_MANAGER */
#define TM_KILL                            8 /* CICON in tree TASK_MANAGER */
#define TM_TERM                            9 /* CICON in tree TASK_MANAGER */
#define TM_SLEEP                          10 /* CICON in tree TASK_MANAGER */
#define TM_WAKE                           11 /* CICON in tree TASK_MANAGER */
#define XAAES_32                          12 /* CICON in tree TASK_MANAGER */
#define TM_LIST                           13 /* BOX in tree TASK_MANAGER */
#define TM_QUITAPPS                       14 /* BUTTON in tree TASK_MANAGER */
#define TM_REBOOT                         15 /* BUTTON in tree TASK_MANAGER */
#define TM_QUIT                           16 /* BUTTON in tree TASK_MANAGER */
#define TM_HALT                           17 /* BUTTON in tree TASK_MANAGER */
#define TM_OK                             18 /* BUTTON in tree TASK_MANAGER */
#define TM_COLD                           19 /* BUTTON in tree TASK_MANAGER */
#define TM_RESCHG                         20 /* BUTTON in tree TASK_MANAGER */
#define TM_CHART                          21 /* BOXTEXT in tree TASK_MANAGER */
#define TM_CHARTTTL                       22 /* TEXT in tree TASK_MANAGER */
#define TM_CPU                            23 /* BOX in tree TASK_MANAGER */
#define TM_TICK3                          24 /* BOXTEXT in tree TASK_MANAGER */
#define TM_TICK2                          25 /* BOXTEXT in tree TASK_MANAGER */
#define TM_TICK1                          26 /* BOXTEXT in tree TASK_MANAGER */
#define TM_LOAD3                          27 /* BOXTEXT in tree TASK_MANAGER */
#define TM_LOAD2                          28 /* BOXTEXT in tree TASK_MANAGER */
#define TM_LOAD1                          29 /* BOXTEXT in tree TASK_MANAGER */
#define TM_ACTLD                          30 /* BOXTEXT in tree TASK_MANAGER */
#define TM_CPUTTL                         31 /* TEXT in tree TASK_MANAGER */
#define TM_RAM                            32 /* BOX in tree TASK_MANAGER */
#define TM_CORE                           33 /* BOXTEXT in tree TASK_MANAGER */
#define TM_FAST                           34 /* BOXTEXT in tree TASK_MANAGER */
#define TM_MEM                            35 /* BOXTEXT in tree TASK_MANAGER */
#define TM_RAMTTL                         36 /* TEXT in tree TASK_MANAGER */

#define SYS_ERROR                          7 /* form/dialog */
#define SYSALERT_LIST                      1 /* BOX in tree SYS_ERROR */
#define SALERT_ICONS                       2 /* IBOX in tree SYS_ERROR */
#define SALERT_IC1                         3 /* CICON in tree SYS_ERROR */
#define SALERT_IC2                         4 /* CICON in tree SYS_ERROR */
#define SALERT_IC3                         5 /* CICON in tree SYS_ERROR */
#define SALERT_IC4                         6 /* CICON in tree SYS_ERROR */
#define SALERT_CLEAR                       7 /* BUTTON in tree SYS_ERROR */
#define SALERT_OK                          8 /* BUTTON in tree SYS_ERROR */

#define FILE_SELECT                        8 /* form/dialog */
#define FS_FILE                            1 /* FBOXTEXT in tree FILE_SELECT */
#define FS_LIST                            2 /* BOX in tree FILE_SELECT */
#define FS_ICONS                           3 /* IBOX in tree FILE_SELECT */
#define FS_ICN_EXE                         4 /* CICON in tree FILE_SELECT */
#define FS_ICN_DIR                         5 /* CICON in tree FILE_SELECT */
#define FS_ICN_PRG                         6 /* CICON in tree FILE_SELECT */
#define FS_ICN_FILE                        7 /* CICON in tree FILE_SELECT */
#define FS_ICN_SYMLINK                     8 /* CICON in tree FILE_SELECT */
#define FS_CANCEL                          9 /* BUTTON in tree FILE_SELECT */
#define FS_OK                             10 /* BUTTON in tree FILE_SELECT */
#define FS_FNTSIZE                        11 /* IBOX in tree FILE_SELECT */
#define FS_LFNTSIZE                       12 /* BUTTON in tree FILE_SELECT */
#define FS_HFNTSIZE                       14 /* BUTTON in tree FILE_SELECT */

#define KILL_OR_WAIT                       9 /* form/dialog */
#define KORW_WAIT                          3 /* BUTTON in tree KILL_OR_WAIT */
#define KORW_KILL                          4 /* BUTTON in tree KILL_OR_WAIT */
#define KORW_APPNAME                       5 /* TEXT in tree KILL_OR_WAIT */
#define KORW_KILLEMALL                     7 /* BUTTON in tree KILL_OR_WAIT */

#define WDLG_FNTS                         10 /* form/dialog */
#define FNTS_XCANCEL                       1 /* BUTTON in tree WDLG_FNTS */
#define FNTS_XOK                           2 /* BUTTON in tree WDLG_FNTS */
#define FNTS_XSET                          3 /* BUTTON in tree WDLG_FNTS */
#define FNTS_XMARK                         4 /* BUTTON in tree WDLG_FNTS */
#define FNTS_XUDEF                         5 /* BUTTON in tree WDLG_FNTS */
#define FNTS_SHOW                          6 /* BOX in tree WDLG_FNTS */
#define FNTS_FNTLIST                       7 /* BOX in tree WDLG_FNTS */
#define FNTS_TYPE                          8 /* BOX in tree WDLG_FNTS */
#define FNTS_POINTS                        9 /* BOX in tree WDLG_FNTS */
#define FNTS_EDSIZE                       10 /* FBOXTEXT in tree WDLG_FNTS */
#define FNTS_EDRATIO                      11 /* FBOXTEXT in tree WDLG_FNTS */
#define FNTS_CBNAMES                      14 /* BOX in tree WDLG_FNTS */
#define FNTS_CBSTYLES                     15 /* BOX in tree WDLG_FNTS */
#define FNTS_CBSIZES                      16 /* BOX in tree WDLG_FNTS */
#define FNTS_CBRATIO                      17 /* BOX in tree WDLG_FNTS */
#define FNTS_XDISPLAY                     18 /* BUTTON in tree WDLG_FNTS */
#define FNTS_ID                           20 /* TEXT in tree WDLG_FNTS */
#define FNTS_W                            22 /* TEXT in tree WDLG_FNTS */
#define FNTS_H                            24 /* TEXT in tree WDLG_FNTS */
#define FNTS_LSIZE                        25 /* BUTTON in tree WDLG_FNTS */
#define FNTS_HSIZE                        26 /* BUTTON in tree WDLG_FNTS */

#define WDLG_PDLG                         11 /* form/dialog */
#define XPDLG_LIST                         1 /* BOX in tree WDLG_PDLG */
#define XPDLG_DRIVER                       3 /* BUTTON in tree WDLG_PDLG */
#define XPDLG_DIALOG                       4 /* IBOX in tree WDLG_PDLG */
#define XPDLG_PRINT                        5 /* BUTTON in tree WDLG_PDLG */
#define XPDLG_CANCEL                       6 /* BUTTON in tree WDLG_PDLG */

#define PDLG_DIALOGS                      12 /* form/dialog */
#define PDLG_GENERAL                       1 /* BOX in tree PDLG_DIALOGS */
#define PDLG_G_QUALITY                     2 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_G_COLOR                       3 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_G_SELALL                      4 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_G_SELFROM                     5 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_G_FROM                        6 /* FBOXTEXT in tree PDLG_DIALOGS */
#define PDLG_G_TO                          7 /* FBOXTEXT in tree PDLG_DIALOGS */
#define PDLG_G_EVEN                        8 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_G_ODD                         9 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_G_COPIES                     10 /* FBOXTEXT in tree PDLG_DIALOGS */
#define PDLG_PAPER                        16 /* BOX in tree PDLG_DIALOGS */
#define PDLG_P_FORMAT                     17 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_P_TYPE                       18 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_P_INTRAY                     19 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_P_OUTTRAY                    20 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_P_LETTER                     21 /* CICON in tree PDLG_DIALOGS */
#define PDLG_G_LAND                       22 /* CICON in tree PDLG_DIALOGS */
#define PDLG_P_SCALE                      23 /* FBOXTEXT in tree PDLG_DIALOGS */
#define PDLG_RASTER                       28 /* BOX in tree PDLG_DIALOGS */
#define PDLG_R_RASTER                     29 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_R_C                          30 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_R_M                          31 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_R_Y                          32 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_R_K                          33 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_PORT                         35 /* BOX in tree PDLG_DIALOGS */
#define PDLG_I_PORT                       36 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_I_BG                         38 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_R_FG                         39 /* BUTTON in tree PDLG_DIALOGS */

#define ABOUT_LINE0                        0 /* Free string */

#define ABOUT_LINE1                        1 /* Free string */

#define ABOUT_LINE2                        2 /* Free string */

#define ABOUT_LINE3                        3 /* Free string */

#define ABOUT_LINE4                        4 /* Free string */

#define ABOUT_LINE5                        5 /* Free string */

#define ABOUT_LINE6                        6 /* Free string */

#define ABOUT_LINE7                        7 /* Free string */

#define WCTXT_WINDOWS                      8 /* Free string */

#define WCTXT_ADVANCED                     9 /* Free string */

#define WCTXT_TODESK                      10 /* Free string */

#define WCTXT_CLOSE                       11 /* Free string */

#define WCTXT_HIDE                        12 /* Free string */

#define WCTXT_ICONIFY                     13 /* Free string */

#define WCTXT_SHADE                       14 /* Free string */

#define WCTXT_MOVE                        15 /* Free string */

#define WCTXT_RESIZE                      16 /* Free string */

#define WCTXT_QUIT                        17 /* Free string */

#define WCTXT_KILL                        18 /* Free string */

#define WCTXT_KEEPABOVE                   19 /* Free string */

#define WCTXT_KEEPBELOW                   20 /* Free string */

#define WCTXT_TOOLBOX                     21 /* Free string */

#define WCTXT_DENY_KEYBOARD               22 /* Free string */

#define WCTXT_CLOSE_THIS                  23 /* Free string */

#define WCTXT_CLOSE_ALL                   24 /* Free string */

#define WCTXT_CLOSE_OTHERS                25 /* Free string */

#define WCTXT_RESTORE_ALL                 26 /* Free string */

#define WCTXT_RESTORE_OTHERS              27 /* Free string */

#define WCTXT_HIDE_THIS                   28 /* Free string */

#define WCTXT_HIDE_APP                    29 /* Free string */

#define WCTXT_HIDE_OTHERS                 30 /* Free string */

#define WCTXT_HIDE_SHOW_OTHERS            31 /* Free string */

#define RS_ABOUT                          32 /* Free string */

#define RS_TASKMANAGER                    33 /* Free string */

#define RS_SYS                            34 /* Free string */

#define RS_APPLST                         35 /* Free string */

#define RS_ALERTS                         36 /* Free string */

#define RS_ENV                            37 /* Free string */

#define RS_SYSTEM                         38 /* Free string */

#define RS_VIDEO                          39 /* Free string */

#define RS_MEMPROT                        40 /* Free string */

#define RS_OFF                            41 /* Free string */

#define RS_ON                             42 /* Free string */

#define SW_KEYBD                          43 /* Free string */

#define SW_DEFAULT                        44 /* Free string */

#define ASK_QUITALL_ALERT                 45 /* Alert string */

#define ASK_QUIT_ALERT                    46 /* Alert string */

#define ASK_RESTART_ALERT                 47 /* Alert string */

#define ASK_SHUTDOWN_ALERT                48 /* Alert string */

#define HAD_A_PROBLEM                     49 /* Alert string */

#define AL_RSCSZ                          50 /* Alert string */

#define RS_LAUNCH                         51 /* Free string */

#define RS_LDGRAD                         52 /* Free string */

#define RS_LDIMG                          53 /* Free string */

#define RS_LDPAL                          54 /* Free string */

#define RS_LDCNF                          55 /* Free string */

#define AL_SDMASTER                       56 /* Free string */

#define AL_KBD                            57 /* Free string */

#define AL_TERMAES                        58 /* Free string */

#define AL_KILLAES                        59 /* Free string */

#define AL_NOPB                           60 /* Free string */

#define AL_NOAESPR                        61 /* Free string */

#define AL_VALOBTREE                      62 /* Free string */

#define AL_ATTACH                         63 /* Free string */

#define AL_MEM                            64 /* Free string */

#define AL_PDLG                           65 /* Free string */

#define AL_SINGLETASK                     66 /* Free string */

#define AL_NOGLOBPTR                      67 /* Free string */

#define AL_INVALIDP                       68 /* Free string */

#define AL_SHELLRUNS                      69 /* Free string */

#define AL_NOSHELL                        70 /* Free string */

#define MNU_CLIENTS                       71 /* Free string */

#define XA_HELP_FILE                      72 /* Free string */

#define XS_KEY_CURSOR_UP                  73 /* Free string */

#define XS_KEY_CURSOR_DOWN                74 /* Free string */

#define XS_KEY_CURSOR_RIGHT               75 /* Free string */

#define XS_KEY_CURSOR_LEFT                76 /* Free string */

#define XS_KEY_SPACE                      77 /* Free string */

#define XS_KEY_ESC                        78 /* Free string */

#define XS_KEY_DOT                        79 /* Free string */

#define XS_KEY_BACKSPACE                  80 /* Free string */

#define XS_KEY_TAB                        81 /* Free string */

#define XS_KEY_ENTER                      82 /* Free string */

#define XS_KEY_INSERT                     83 /* Free string */

#define XS_KEY_HOME                       84 /* Free string */

#define XS_KEY_RETURN                     85 /* Free string */

#define XS_KEY_DELETE                     86 /* Free string */

#define XS_KEY_HELP                       87 /* Free string */

#define XS_KEY_UNDO                       88 /* Free string */

/* Border */
#define BBL_BORDER                        89 /* Free string */

#define BBL_MOVE                          90 /* Free string */

#define BBL_WCONTEXT                      91 /* Free string */

#define BBL_WAPPICN                       92 /* Free string */

#define BBL_CLOSE                         93 /* Free string */

#define BBL_FULL                          94 /* Free string */

#define BBL_INFO                          95 /* Free string */

#define BBL_RESIZE                        96 /* Free string */

#define BBL_UPLN                          97 /* Free string */

#define BBL_UPLN1                         98 /* Free string */

#define BBL_DNLN                          99 /* Free string */

#define BBL_VSLIDE                       100 /* Free string */

#define BBL_LFLN                         101 /* Free string */

#define BBL_LFLN1                        102 /* Free string */

#define BBL_RTLN                         103 /* Free string */

#define BBL_HSLIDE                       104 /* Free string */

#define BBL_ICONIFY                      105 /* Free string */

#define BBL_HIDE                         106 /* Free string */

#define BBL_TOOLBAR                      107 /* Free string */

#define BBL_MENU                         108 /* Free string */

#define BBL_MOVER                        109 /* Free string */

#define BBL_UPPAGE                       110 /* Free string */

#define BBL_DNPAGE                       111 /* Free string */

#define BBL_LFPAGE                       112 /* Free string */

#define BBL_RTPAGE                       113 /* Free string */

#define _RSM_CRC_                        114 /* Free string */
#define _RSM_CRC_STRING_ "& RSM-crc >1D50< crc-MSR $"



#ifdef __STDC__
#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  else
#    define _WORD short
#  endif
#endif
extern _WORD xaaes_rsc_load(_WORD wchar, _WORD hchar);
extern _WORD xaaes_rsc_gaddr(_WORD type, _WORD idx, void *gaddr);
extern _WORD xaaes_rsc_free(void);
#endif
