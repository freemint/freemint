/*
 * resource set indices for xaaeswdg
 *
 * created by ORCS 2.17
 */

/*
 * Number of Strings:        11
 * Number of Bitblks:        0
 * Number of Iconblks:       1
 * Number of Color Iconblks: 9
 * Number of Color Icons:    18
 * Number of Tedinfos:       0
 * Number of Free Strings:   0
 * Number of Free Images:    0
 * Number of Objects:        12
 * Number of Trees:          1
 * Number of Userblks:       0
 * Number of Images:         2
 * Total file size:          7996
 */

#undef RSC_NAME
#ifndef __ALCYON__
#define RSC_NAME "xaaeswdg"
#endif
#undef RSC_ID
#ifdef xaaeswdg
#define RSC_ID xaaeswdg
#else
#define RSC_ID 0
#endif

#ifndef RSC_STATIC_FILE
# define RSC_STATIC_FILE 0
#endif
#if !RSC_STATIC_FILE
#define NUM_STRINGS 11
#define NUM_FRSTR 0
#define NUM_UD 0
#define NUM_IMAGES 2
#define NUM_BB 0
#define NUM_FRIMG 0
#define NUM_IB 1
#define NUM_CIB 9
#define NUM_TI 0
#define NUM_OBS 12
#define NUM_TREE 1
#endif



#define WIDGETS            0 /* form/dialog */
#define WIDG_CLOSER        1 /* CICON in tree WIDGETS */
#define WIDG_LOGO          2 /* ICON in tree WIDGETS */
#define WIDG_HIDE          3 /* CICON in tree WIDGETS */
#define WIDG_ICONIFY       4 /* CICON in tree WIDGETS */
#define WIDG_FULL          5 /* CICON in tree WIDGETS */
#define WIDG_UP            6 /* CICON in tree WIDGETS */
#define WIDG_DOWN          7 /* CICON in tree WIDGETS */
#define WIDG_LEFT          8 /* CICON in tree WIDGETS */
#define WIDG_RIGHT         9 /* CICON in tree WIDGETS */
#define WIDG_SIZE         10 /* CICON in tree WIDGETS */
#define WIDG_CFG          11 /* STRING in tree WIDGETS */




#ifdef __STDC__
#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  else
#    define _WORD short
#  endif
#endif
extern _WORD xaaeswdg_rsc_load(_WORD wchar, _WORD hchar);
extern _WORD xaaeswdg_rsc_gaddr(_WORD type, _WORD idx, void *gaddr);
extern _WORD xaaeswdg_rsc_free(void);
#endif
