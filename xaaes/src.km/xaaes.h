/*
 * resource set indices for xaaes
 *
 * created by ORCS 2.16
 */

/*
 * Number of Strings:        300
 * Number of Bitblks:        0
 * Number of Iconblks:       2
 * Number of Color Iconblks: 30
 * Number of Color Icons:    38
 * Number of Tedinfos:       30
 * Number of Free Strings:   0
 * Number of Free Images:    0
 * Number of Objects:        291
 * Number of Trees:          13
 * Number of Userblks:       0
 * Number of Images:         4
 * Total file size:          71804
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
#define NUM_STRINGS 300
#define NUM_FRSTR 0
#define NUM_UD 0
#define NUM_IMAGES 4
#define NUM_BB 0
#define NUM_FRIMG 0
#define NUM_IB 2
#define NUM_CIB 30
#define NUM_TI 30
#define NUM_OBS 291
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
#define ALR_IC_WARNING                     1 /* CICON in tree ALERT_ICONS */ /* max len 1 */
#define ALR_IC_QUESTION                    2 /* CICON in tree ALERT_ICONS */ /* max len 1 */
#define ALR_IC_STOP                        3 /* CICON in tree ALERT_ICONS */ /* max len 1 */
#define ALR_IC_INFO                        4 /* CICON in tree ALERT_ICONS */ /* max len 1 */
#define ALR_IC_DRIVE                       5 /* CICON in tree ALERT_ICONS */ /* max len 1 */
#define ALR_IC_BOMB                        6 /* CICON in tree ALERT_ICONS */ /* max len 1 */
#define ALR_IC_SYSTEM                      7 /* CICON in tree ALERT_ICONS */ /* max len 1 */

#define ALERT_BOX                          3 /* form/dialog */
#define ALERT_BACK                         0 /* BOX in tree ALERT_BOX */
#define ALERT_D_ICON                       1 /* CICON in tree ALERT_BOX */ /* max len 0 */
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
#define RSC_VERSION                       13 /* TEXT in tree ABOUT_XAAES */ /* max len 5 */

#define DEF_DESKTOP                        5 /* form/dialog */
#define DESKTOP                            0 /* BOX in tree DEF_DESKTOP */
#define DESKTOP_LOGO                       1 /* CICON in tree DEF_DESKTOP */ /* max len 0 */

#define TASK_MANAGER                       6 /* form/dialog */
#define TM_ICONS                           1 /* IBOX in tree TASK_MANAGER */
#define TM_ICN_HALTED                      2 /* CICON in tree TASK_MANAGER */ /* max len 1 */
#define TM_ICN_LOCK                        3 /* CICON in tree TASK_MANAGER */ /* max len 1 */
#define TM_ICN_MENU                        4 /* ICON in tree TASK_MANAGER */ /* max len 1 */
#define TM_ICN_MESSAGE                     5 /* CICON in tree TASK_MANAGER */ /* max len 1 */
#define TM_ICN_XAAES                       6 /* ICON in tree TASK_MANAGER */ /* max len 1 */
#define TM_PICS                            7 /* IBOX in tree TASK_MANAGER */
#define TM_KILL                            8 /* CICON in tree TASK_MANAGER */ /* max len 1 */
#define TM_TERM                            9 /* CICON in tree TASK_MANAGER */ /* max len 1 */
#define TM_SLEEP                          10 /* CICON in tree TASK_MANAGER */ /* max len 1 */
#define TM_WAKE                           11 /* CICON in tree TASK_MANAGER */ /* max len 1 */
#define XAAES_32                          12 /* CICON in tree TASK_MANAGER */ /* max len 0 */
#define TM_LIST                           13 /* BOX in tree TASK_MANAGER */
#define TM_QUITAPPS                       14 /* BUTTON in tree TASK_MANAGER */
#define TM_REBOOT                         15 /* BUTTON in tree TASK_MANAGER */
#define TM_QUIT                           16 /* BUTTON in tree TASK_MANAGER */
#define TM_HALT                           17 /* BUTTON in tree TASK_MANAGER */
#define TM_OK                             18 /* BUTTON in tree TASK_MANAGER */
#define TM_COLD                           19 /* BUTTON in tree TASK_MANAGER */
#define TM_RESCHG                         20 /* BUTTON in tree TASK_MANAGER */
#define TM_CHART                          21 /* BOXTEXT in tree TASK_MANAGER */ /* max len 0 */
#define TM_CHARTTTL                       22 /* TEXT in tree TASK_MANAGER */ /* max len 11 */
#define TM_CPU                            23 /* BOX in tree TASK_MANAGER */
#define TM_TICK3                          24 /* BOXTEXT in tree TASK_MANAGER */ /* max len 0 */
#define TM_TICK2                          25 /* BOXTEXT in tree TASK_MANAGER */ /* max len 0 */
#define TM_TICK1                          26 /* BOXTEXT in tree TASK_MANAGER */ /* max len 0 */
#define TM_LOAD3                          27 /* BOXTEXT in tree TASK_MANAGER */ /* max len 0 */
#define TM_LOAD2                          28 /* BOXTEXT in tree TASK_MANAGER */ /* max len 0 */
#define TM_LOAD1                          29 /* BOXTEXT in tree TASK_MANAGER */ /* max len 0 */
#define TM_ACTLD                          30 /* BOXTEXT in tree TASK_MANAGER */ /* max len 0 */
#define TM_CPUTTL                         31 /* TEXT in tree TASK_MANAGER */ /* max len 3 */
#define TM_RAM                            32 /* BOX in tree TASK_MANAGER */
#define TM_CORE                           33 /* BOXTEXT in tree TASK_MANAGER */ /* max len 1 */
#define TM_FAST                           34 /* BOXTEXT in tree TASK_MANAGER */ /* max len 1 */
#define TM_MEM                            35 /* BOXTEXT in tree TASK_MANAGER */ /* max len 1 */
#define TM_RAMTTL                         36 /* TEXT in tree TASK_MANAGER */ /* max len 3 */

#define SYS_ERROR                          7 /* form/dialog */
#define SYSALERT_LIST                      1 /* BOX in tree SYS_ERROR */
#define SALERT_ICONS                       2 /* IBOX in tree SYS_ERROR */
#define SALERT_IC1                         3 /* CICON in tree SYS_ERROR */ /* max len 1 */
#define SALERT_IC2                         4 /* CICON in tree SYS_ERROR */ /* max len 1 */
#define SALERT_IC3                         5 /* CICON in tree SYS_ERROR */ /* max len 1 */
#define SALERT_IC4                         6 /* CICON in tree SYS_ERROR */ /* max len 1 */
#define SALERT_CLEAR                       7 /* BUTTON in tree SYS_ERROR */
#define SALERT_OK                          8 /* BUTTON in tree SYS_ERROR */

#define FILE_SELECT                        8 /* form/dialog */
#define FS_FILE                            1 /* FBOXTEXT in tree FILE_SELECT */ /* max len 59 */
#define FS_LIST                            2 /* BOX in tree FILE_SELECT */
#define FS_ICONS                           3 /* IBOX in tree FILE_SELECT */
#define FS_ICN_EXE                         4 /* CICON in tree FILE_SELECT */ /* max len 1 */
#define FS_ICN_DIR                         5 /* CICON in tree FILE_SELECT */ /* max len 1 */
#define FS_ICN_PRG                         6 /* CICON in tree FILE_SELECT */ /* max len 1 */
#define FS_ICN_FILE                        7 /* CICON in tree FILE_SELECT */ /* max len 1 */
#define FS_ICN_SYMLINK                     8 /* CICON in tree FILE_SELECT */ /* max len 1 */
#define FS_CANCEL                          9 /* BUTTON in tree FILE_SELECT */
#define FS_OK                             10 /* BUTTON in tree FILE_SELECT */
#define FS_FNTSIZE                        11 /* IBOX in tree FILE_SELECT */
#define FS_LFNTSIZE                       12 /* BUTTON in tree FILE_SELECT */
#define FS_HFNTSIZE                       14 /* BUTTON in tree FILE_SELECT */

#define KILL_OR_WAIT                       9 /* form/dialog */
#define KORW_WAIT                          3 /* BUTTON in tree KILL_OR_WAIT */
#define KORW_KILL                          4 /* BUTTON in tree KILL_OR_WAIT */
#define KORW_APPNAME                       5 /* TEXT in tree KILL_OR_WAIT */ /* max len 29 */
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
#define FNTS_EDSIZE                       10 /* FBOXTEXT in tree WDLG_FNTS */ /* max len 4 */
#define FNTS_EDRATIO                      11 /* FBOXTEXT in tree WDLG_FNTS */ /* max len 4 */
#define FNTS_CBNAMES                      14 /* BOX in tree WDLG_FNTS */
#define FNTS_CBSTYLES                     15 /* BOX in tree WDLG_FNTS */
#define FNTS_CBSIZES                      16 /* BOX in tree WDLG_FNTS */
#define FNTS_CBRATIO                      17 /* BOX in tree WDLG_FNTS */
#define FNTS_XDISPLAY                     18 /* BUTTON in tree WDLG_FNTS */
#define FNTS_ID                           20 /* TEXT in tree WDLG_FNTS */ /* max len 10 */
#define FNTS_W                            22 /* TEXT in tree WDLG_FNTS */ /* max len 3 */
#define FNTS_H                            24 /* TEXT in tree WDLG_FNTS */ /* max len 3 */
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
#define PDLG_G_FROM                        6 /* FBOXTEXT in tree PDLG_DIALOGS */ /* max len 4 */
#define PDLG_G_TO                          7 /* FBOXTEXT in tree PDLG_DIALOGS */ /* max len 4 */
#define PDLG_G_EVEN                        8 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_G_ODD                         9 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_G_COPIES                     10 /* FBOXTEXT in tree PDLG_DIALOGS */ /* max len 4 */
#define PDLG_PAPER                        16 /* BOX in tree PDLG_DIALOGS */
#define PDLG_P_FORMAT                     17 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_P_TYPE                       18 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_P_INTRAY                     19 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_P_OUTTRAY                    20 /* BUTTON in tree PDLG_DIALOGS */
#define PDLG_P_LETTER                     21 /* CICON in tree PDLG_DIALOGS */ /* max len 1 */
#define PDLG_G_LAND                       22 /* CICON in tree PDLG_DIALOGS */ /* max len 1 */
#define PDLG_P_SCALE                      23 /* FBOXTEXT in tree PDLG_DIALOGS */ /* max len 3 */
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
