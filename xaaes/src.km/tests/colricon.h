/*
 * resource set indices for colricon
 *
 * created by ORCS 2.16
 */

/*
 * Number of Strings:        24
 * Number of Bitblks:        0
 * Number of Iconblks:       0
 * Number of Color Iconblks: 10
 * Number of Color Icons:    14
 * Number of Tedinfos:       3
 * Number of Free Strings:   0
 * Number of Free Images:    0
 * Number of Objects:        19
 * Number of Trees:          1
 * Number of Userblks:       0
 * Number of Images:         0
 * Total file size:          23044
 */

#undef RSC_NAME
#ifndef __ALCYON__
#define RSC_NAME "colricon"
#endif
#undef RSC_ID
#ifdef colricon
#define RSC_ID colricon
#else
#define RSC_ID 0
#endif

#ifndef RSC_STATIC_FILE
# define RSC_STATIC_FILE 0
#endif
#if !RSC_STATIC_FILE
#define NUM_STRINGS 24
#define NUM_FRSTR 0
#define NUM_UD 0
#define NUM_IMAGES 0
#define NUM_BB 0
#define NUM_FRIMG 0
#define NUM_IB 0
#define NUM_CIB 10
#define NUM_TI 3
#define NUM_OBS 19
#define NUM_TREE 1
#endif



#define MAIN                               0 /* form/dialog */
#define HARDDISK                           2 /* CICON in tree MAIN */ /* max len 12 */
#define VDI_PLANES                        13 /* FTEXT in tree MAIN */ /* max len 2 */
#define COOK_VDO                          16 /* FTEXT in tree MAIN */ /* max len 1 */
#define COOK_MCH                          17 /* FTEXT in tree MAIN */ /* max len 1 */




#ifdef __STDC__
#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  else
#    define _WORD short
#  endif
#endif
extern _WORD colricon_rsc_load(_WORD wchar, _WORD hchar);
extern _WORD colricon_rsc_gaddr(_WORD type, _WORD idx, void *gaddr);
extern _WORD colricon_rsc_free(void);
#endif
