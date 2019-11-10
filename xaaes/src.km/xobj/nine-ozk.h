/*
 * resource set indices for nine-ozk
 *
 * created by ORCS 2.16
 */

/*
 * Number of Strings:        5
 * Number of Bitblks:        0
 * Number of Iconblks:       0
 * Number of Color Iconblks: 4
 * Number of Color Icons:    4
 * Number of Tedinfos:       0
 * Number of Free Strings:   0
 * Number of Free Images:    0
 * Number of Objects:        6
 * Number of Trees:          1
 * Number of Userblks:       0
 * Number of Images:         0
 * Total file size:          3466
 */

#undef RSC_NAME
#ifndef __ALCYON__
#define RSC_NAME "nine-ozk"
#endif
#undef RSC_ID
#ifdef nine-ozk
#define RSC_ID nine-ozk
#else
#define RSC_ID 0
#endif

#ifndef RSC_STATIC_FILE
# define RSC_STATIC_FILE 0
#endif
#if !RSC_STATIC_FILE
#define NUM_STRINGS 5
#define NUM_FRSTR 0
#define NUM_UD 0
#define NUM_IMAGES 0
#define NUM_BB 0
#define NUM_FRIMG 0
#define NUM_IB 0
#define NUM_CIB 4
#define NUM_TI 0
#define NUM_OBS 6
#define NUM_TREE 1
#endif



#define EXT_AESOBJ         0 /* form/dialog */
#define XOBJ_R_DSEL        1 /* CICON in tree EXT_AESOBJ */ /* max len 1 */
#define XOBJ_R_SEL         2 /* CICON in tree EXT_AESOBJ */ /* max len 1 */
#define XOBJ_B_DSEL        3 /* CICON in tree EXT_AESOBJ */ /* max len 1 */
#define XOBJ_B_SEL         4 /* CICON in tree EXT_AESOBJ */ /* max len 1 */
#define EXTOBJ_NAME        5 /* STRING in tree EXT_AESOBJ */




#ifdef __STDC__
#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  else
#    define _WORD short
#  endif
#endif
extern _WORD nine-ozk_rsc_load(_WORD wchar, _WORD hchar);
extern _WORD nine-ozk_rsc_gaddr(_WORD type, _WORD idx, void *gaddr);
extern _WORD nine-ozk_rsc_free(void);
#endif
