/*
 * resource set indices for tbarmenu
 *
 * created by ORCS 2.18
 */

/*
 * Number of Strings:        19
 * Number of Bitblks:        0
 * Number of Iconblks:       0
 * Number of Color Iconblks: 0
 * Number of Color Icons:    0
 * Number of Tedinfos:       3
 * Number of Free Strings:   0
 * Number of Free Images:    0
 * Number of Objects:        17
 * Number of Trees:          4
 * Number of Userblks:       0
 * Number of Images:         0
 * Total file size:          672
 */

#ifdef RSC_NAME
#undef RSC_NAME
#endif
#ifndef __ALCYON__
#define RSC_NAME "tbarmenu"
#endif
#ifdef RSC_ID
#undef RSC_ID
#endif
#ifdef tbarmenu
#define RSC_ID tbarmenu
#else
#define RSC_ID 0
#endif

#ifndef RSC_STATIC_FILE
# define RSC_STATIC_FILE 0
#endif
#if !RSC_STATIC_FILE
#define NUM_STRINGS 19
#define NUM_FRSTR 0
#define NUM_UD 0
#define NUM_IMAGES 0
#define NUM_BB 0
#define NUM_FRIMG 0
#define NUM_IB 0
#define NUM_CIB 0
#define NUM_TI 3
#define NUM_OBS 17
#define NUM_TREE 4
#endif



#define TBAR               0 /* form/dialog */
#define DESK               1 /* TEXT in tree TBAR */ /* max len 6 */
#define FILE               2 /* TEXT in tree TBAR */ /* max len 6 */
#define TEXT               3 /* TEXT in tree TBAR */ /* max len 6 */

#define DESKPOPUP          1 /* form/dialog */

#define FILEPOPUP          2 /* form/dialog */

#define TEXTPOPUP          3 /* form/dialog */




#ifdef __STDC__
#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  else
#    define _WORD short
#  endif
#endif
extern _WORD tbarmenu_rsc_load(_WORD wchar, _WORD hchar);
extern _WORD tbarmenu_rsc_gaddr(_WORD type, _WORD idx, void *gaddr);
extern _WORD tbarmenu_rsc_free(void);
#endif
