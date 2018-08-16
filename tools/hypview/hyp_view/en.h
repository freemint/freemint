/*
 * resource set indices for en
 *
 * created by ORCS 2.16
 */

/*
 * Number of Strings:        112
 * Number of Bitblks:        0
 * Number of Iconblks:       0
 * Number of Color Iconblks: 15
 * Number of Color Icons:    29
 * Number of Tedinfos:       14
 * Number of Free Strings:   7
 * Number of Free Images:    0
 * Number of Objects:        91
 * Number of Trees:          7
 * Number of Userblks:       0
 * Number of Images:         0
 * Total file size:          25754
 */

#undef RSC_NAME
#ifndef __ALCYON__
#define RSC_NAME "en"
#endif
#undef RSC_ID
#ifdef en
#define RSC_ID en
#else
#define RSC_ID 0
#endif

#ifndef RSC_STATIC_FILE
# define RSC_STATIC_FILE 0
#endif
#if !RSC_STATIC_FILE
#define NUM_STRINGS 112
#define NUM_FRSTR 7
#define NUM_UD 0
#define NUM_IMAGES 0
#define NUM_BB 0
#define NUM_FRIMG 0
#define NUM_IB 0
#define NUM_CIB 15
#define NUM_TI 14
#define NUM_OBS 91
#define NUM_TREE 7
#endif



#define DIAL_LIBRARY                       0 /* form/dialog */
#define DI_ICON                            1 /* CICON in tree DIAL_LIBRARY */ /* max len 1 */
#define DI_ICONIFY_NAME                    2 /* STRING in tree DIAL_LIBRARY */
#define DI_MEMORY_ERROR                    3 /* STRING in tree DIAL_LIBRARY */
#define DI_WDIALOG_ERROR                   4 /* STRING in tree DIAL_LIBRARY */
#define DI_HELP_ERROR                      5 /* STRING in tree DIAL_LIBRARY */
#define DI_VDI_WKS_ERROR                   6 /* STRING in tree DIAL_LIBRARY */

#define TOOLBAR                            1 /* form/dialog */
#define TO_BACKGRND                        0 /* BOX in tree TOOLBAR */
#define TO_ID                              1 /* STRING in tree TOOLBAR */
#define TO_SEARCHBOX                       2 /* BOX in tree TOOLBAR */
#define TO_SEARCH                          3 /* FTEXT in tree TOOLBAR */ /* max len 25 */
#define TO_STRNOTFOUND                     4 /* TEXT in tree TOOLBAR */ /* max len 10 */
#define TO_BUTTONBOX                       5 /* IBOX in tree TOOLBAR */
#define TO_MENU                            6 /* CICON in tree TOOLBAR */ /* max len 1 */
#define TO_SAVE                            7 /* CICON in tree TOOLBAR */ /* max len 1 */
#define TO_LOAD                            8 /* CICON in tree TOOLBAR */ /* max len 1 */
#define TO_INFO                            9 /* CICON in tree TOOLBAR */ /* max len 0 */
#define TO_HELP                           10 /* CICON in tree TOOLBAR */ /* max len 0 */
#define TO_REFERENCES                     11 /* CICON in tree TOOLBAR */ /* max len 1 */
#define TO_KATALOG                        12 /* CICON in tree TOOLBAR */ /* max len 1 */
#define TO_INDEX                          13 /* CICON in tree TOOLBAR */ /* max len 1 */
#define TO_NEXT                           14 /* CICON in tree TOOLBAR */ /* max len 1 */
#define TO_HOME                           15 /* CICON in tree TOOLBAR */ /* max len 1 */
#define TO_PREVIOUS                       16 /* CICON in tree TOOLBAR */ /* max len 1 */
#define TO_MEMORY                         17 /* CICON in tree TOOLBAR */ /* max len 1 */
#define TO_MOREBACK                       18 /* CICON in tree TOOLBAR */ /* max len 1 */
#define TO_BACK                           19 /* CICON in tree TOOLBAR */ /* max len 1 */

#define CONTEXT                            2 /* form/dialog */
#define CO_BACK                            1 /* STRING in tree CONTEXT */
#define CO_COPY                            2 /* STRING in tree CONTEXT */
#define CO_PASTE                           3 /* STRING in tree CONTEXT */
#define CO_SELECT_ALL                      5 /* STRING in tree CONTEXT */
#define CO_SAVE                            7 /* STRING in tree CONTEXT */
#define CO_SEARCH                          9 /* STRING in tree CONTEXT */
#define CO_SEARCH_AGAIN                   10 /* STRING in tree CONTEXT */
#define CO_DELETE_STACK                   11 /* STRING in tree CONTEXT */
#define CO_PRINT                          12 /* STRING in tree CONTEXT */

#define EMPTYPOPUP                         3 /* form/dialog */
#define EM_BACK                            0 /* BOX in tree EMPTYPOPUP */
#define EM_1                               1 /* STRING in tree EMPTYPOPUP */
#define EM_2                               2 /* STRING in tree EMPTYPOPUP */
#define EM_3                               3 /* STRING in tree EMPTYPOPUP */
#define EM_4                               4 /* STRING in tree EMPTYPOPUP */
#define EM_5                               5 /* STRING in tree EMPTYPOPUP */
#define EM_6                               6 /* STRING in tree EMPTYPOPUP */
#define EM_7                               7 /* STRING in tree EMPTYPOPUP */
#define EM_8                               8 /* STRING in tree EMPTYPOPUP */
#define EM_9                               9 /* STRING in tree EMPTYPOPUP */
#define EM_10                             10 /* STRING in tree EMPTYPOPUP */
#define EM_11                             11 /* STRING in tree EMPTYPOPUP */
#define EM_12                             12 /* STRING in tree EMPTYPOPUP */

#define PROGINFO                           4 /* form/dialog */
#define PROG_OK                            5 /* BUTTON in tree PROGINFO */
#define PROG_NAME                          6 /* STRING in tree PROGINFO */
#define PROG_DATEI                         7 /* STRING in tree PROGINFO */
#define PROG_THEMA                         8 /* STRING in tree PROGINFO */
#define PROG_AUTOR                         9 /* STRING in tree PROGINFO */
#define PROG_VERSION                      10 /* STRING in tree PROGINFO */
#define PROG_DATE                         11 /* STRING in tree PROGINFO */

#define SEARCH_RESULT                      5 /* form/dialog */
#define SR_FSTL_UP                         1 /* BOXCHAR in tree SEARCH_RESULT */
#define SR_FSTL_BACK                       2 /* BOX in tree SEARCH_RESULT */
#define SR_FSTL_WHITE                      3 /* BOX in tree SEARCH_RESULT */
#define SR_FSTL_DOWN                       4 /* BOXCHAR in tree SEARCH_RESULT */
#define SR_BOX                             5 /* IBOX in tree SEARCH_RESULT */
#define SR_FSTL_0                          6 /* TEXT in tree SEARCH_RESULT */ /* max len 70 */
#define SR_FSTL_1                          7 /* TEXT in tree SEARCH_RESULT */ /* max len 70 */
#define SR_FSTL_2                          8 /* TEXT in tree SEARCH_RESULT */ /* max len 70 */
#define SR_FSTL_3                          9 /* TEXT in tree SEARCH_RESULT */ /* max len 70 */
#define SR_FSTL_4                         10 /* TEXT in tree SEARCH_RESULT */ /* max len 70 */
#define SR_FSTL_5                         11 /* TEXT in tree SEARCH_RESULT */ /* max len 70 */
#define SR_FSTL_6                         12 /* TEXT in tree SEARCH_RESULT */ /* max len 70 */
#define SR_FSTL_7                         13 /* TEXT in tree SEARCH_RESULT */ /* max len 70 */
#define SR_FSTL_8                         14 /* TEXT in tree SEARCH_RESULT */ /* max len 70 */
#define SR_FSTL_9                         15 /* TEXT in tree SEARCH_RESULT */ /* max len 70 */
#define SR_ABORT                          16 /* BUTTON in tree SEARCH_RESULT */

#define HYPFIND                            6 /* form/dialog */
#define HYPFIND_STRING                     2 /* FTEXT in tree HYPFIND */ /* max len 30 */
#define HYPFIND_TEXT                       3 /* BUTTON in tree HYPFIND */
#define HYPFIND_PAGES                      4 /* BUTTON in tree HYPFIND */
#define HYPFIND_REF                        5 /* BUTTON in tree HYPFIND */
#define HYPFIND_ABORT                      6 /* BUTTON in tree HYPFIND */
#define HYPFIND_ALL_PAGE                   7 /* BUTTON in tree HYPFIND */
#define HYPFIND_ALL_HYP                    8 /* BUTTON in tree HYPFIND */

#define WARN_FEXIST                        0 /* Alert string */
/* [2][This file exists already.|Do you want to replace it?][Replace|Abort] */

#define WARN_ERASEMARK                     1 /* Alert string */
/* [2][Do you want to remove|%s|from your bookmarks?][  Yes  |  No  ] */

#define ASK_SETMARK                        2 /* Alert string */
/* [2][Do you want to add|%s|to your bookmarks?][  Yes  |  No  ] */

#define ASK_SAVEMARKFILE                   3 /* Alert string */
/* [2][Save bookmarks?][  Yes  |  No  ] */

#define WARN_NORESULT                      4 /* Alert string */
/* [1][HypView: could not find|<%s>][ Abort ] */

#define FSLX_LOAD                          5 /* Free string */
/* Select hypertext to load: */

#define FSLX_SAVE                          6 /* Free string */
/* Save ASCII text as: */




#ifdef __STDC__
#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  else
#    define _WORD short
#  endif
#endif
extern _WORD en_rsc_load(void);
extern _WORD en_rsc_gaddr(_WORD type, _WORD idx, void *gaddr);
extern _WORD en_rsc_free(void);
#endif
