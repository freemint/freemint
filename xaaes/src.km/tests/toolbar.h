/*
 * resource set indices for toolbar
 *
 * created by ORCS 2.18
 */

/*
 * Number of Strings:        315
 * Number of Bitblks:        0
 * Number of Iconblks:       0
 * Number of Color Iconblks: 36
 * Number of Color Icons:    66
 * Number of Tedinfos:       27
 * Number of Free Strings:   0
 * Number of Free Images:    0
 * Number of Objects:        371
 * Number of Trees:          20
 * Number of Userblks:       0
 * Number of Images:         0
 * Total file size:          47442
 */

#ifdef RSC_NAME
#undef RSC_NAME
#endif
#ifndef __ALCYON__
#define RSC_NAME "toolbar"
#endif
#ifdef RSC_ID
#undef RSC_ID
#endif
#ifdef toolbar
#define RSC_ID toolbar
#else
#define RSC_ID 0
#endif

#ifndef RSC_STATIC_FILE
# define RSC_STATIC_FILE 0
#endif
#if !RSC_STATIC_FILE
#define NUM_STRINGS 315
#define NUM_FRSTR 0
#define NUM_UD 0
#define NUM_IMAGES 0
#define NUM_BB 0
#define NUM_FRIMG 0
#define NUM_IB 0
#define NUM_CIB 36
#define NUM_TI 27
#define NUM_OBS 371
#define NUM_TREE 20
#endif



#define MAINMENU   0 /* menu */
#define GTITLE     5 /* TITLE in tree MAINMENU */
#define ETITLE     6 /* TITLE in tree MAINMENU */
#define ABOUTX     9 /* STRING in tree MAINMENU */
#define FOPEN     18 /* STRING in tree MAINMENU */
#define FCLOSE    19 /* STRING in tree MAINMENU */
#define FQUIT     21 /* STRING in tree MAINMENU */
#define STYLE     23 /* STRING in tree MAINMENU */
#define POSITION  24 /* STRING in tree MAINMENU */
#define FONTS1    26 /* STRING in tree MAINMENU */
#define TABOUT    28 /* STRING in tree MAINMENU */
#define TSUB      29 /* STRING in tree MAINMENU */
#define TOOLFLAG  31 /* STRING in tree MAINMENU */
#define SWITCH    32 /* STRING in tree MAINMENU */
#define PHONE     34 /* STRING in tree MAINMENU */
#define PATTERN1  36 /* STRING in tree MAINMENU */
#define COLOR     37 /* STRING in tree MAINMENU */
#define SLISTS    39 /* STRING in tree MAINMENU */

#define FONTTREE   1 /* form/dialog */

#define PTREE      2 /* form/dialog */
#define P1         1 /* BOX in tree PTREE */
#define P2         2 /* BOX in tree PTREE */
#define P3         3 /* BOX in tree PTREE */
#define P4         4 /* BOX in tree PTREE */
#define P5         5 /* BOX in tree PTREE */
#define P6         6 /* BOX in tree PTREE */
#define P7         7 /* BOX in tree PTREE */
#define P8         8 /* BOX in tree PTREE */

#define CTREE      3 /* form/dialog */

#define POSTREE    4 /* form/dialog */

#define STREE      5 /* form/dialog */

#define MTREE      6 /* form/dialog */
#define M1TITLE    1 /* TEXT in tree MTREE */
#define M1BUTTON   2 /* BOXTEXT in tree MTREE */
#define M2TITLE    3 /* TEXT in tree MTREE */
#define M2BUTTON   4 /* BOXTEXT in tree MTREE */
#define M3TITLE    5 /* TEXT in tree MTREE */
#define M3BUTTON   6 /* BOXTEXT in tree MTREE */
#define M4TITLE    7 /* TEXT in tree MTREE */
#define M4BUTTON   8 /* BOXTEXT in tree MTREE */
#define M5TITLE    9 /* TEXT in tree MTREE */
#define M5BUTTON  10 /* BOXTEXT in tree MTREE */
#define M6TITLE   11 /* TEXT in tree MTREE */
#define M6BUTTON  12 /* BOXTEXT in tree MTREE */
#define MOK       13 /* BUTTON in tree MTREE */

#define BAUDRATE   7 /* form/dialog */

#define PARTREE    8 /* form/dialog */

#define BITTREE    9 /* form/dialog */

#define STOPTREE  10 /* form/dialog */

#define PORTTREE  11 /* form/dialog */

#define FLOWTREE  12 /* form/dialog */

#define ATREE     13 /* form/dialog */
#define AEXIT      1 /* BUTTON in tree ATREE */

#define LTREE     14 /* form/dialog */
#define LTITLE     3 /* TEXT in tree LTREE */
#define LBUTTON    4 /* BOXTEXT in tree LTREE */
#define LHOT       5 /* BOXCHAR in tree LTREE */
#define LOK        6 /* BUTTON in tree LTREE */

#define TOOLBOX   15 /* form/dialog */
#define B1B1       1 /* BOX in tree TOOLBOX */
#define B1B2       4 /* BOX in tree TOOLBOX */
#define B1B3       7 /* BOX in tree TOOLBOX */
#define B1B4      10 /* BOX in tree TOOLBOX */
#define B1B5      13 /* BOX in tree TOOLBOX */
#define T1B1      16 /* IBOX in tree TOOLBOX */
#define T1B2      17 /* IBOX in tree TOOLBOX */
#define T1B3      18 /* IBOX in tree TOOLBOX */
#define T1B4      19 /* IBOX in tree TOOLBOX */
#define T1B5      20 /* IBOX in tree TOOLBOX */

#define BLANK     16 /* form/dialog */

#define TOOLBOX2  17 /* form/dialog */
#define T2B1       1 /* BOX in tree TOOLBOX2 */
#define T2B2       4 /* BOX in tree TOOLBOX2 */
#define T2B3       7 /* BOX in tree TOOLBOX2 */
#define T2B4      10 /* BOX in tree TOOLBOX2 */
#define T2B5      13 /* BOX in tree TOOLBOX2 */
#define BASE1     16 /* BOX in tree TOOLBOX2 */
#define BASE2     17 /* BOX in tree TOOLBOX2 */
#define T2I1      19 /* IBOX in tree TOOLBOX2 */
#define T2I2      20 /* IBOX in tree TOOLBOX2 */
#define T2I3      21 /* IBOX in tree TOOLBOX2 */
#define T2I4      22 /* IBOX in tree TOOLBOX2 */
#define T2I5      23 /* IBOX in tree TOOLBOX2 */

#define TOOLBAR   18 /* form/dialog */
#define T3BASE1    1 /* BOX in tree TOOLBAR */
#define T3BASE2   46 /* BOX in tree TOOLBAR */
#define FBUTT1    55 /* BOXTEXT in tree TOOLBAR */
#define FBUTT2    56 /* BOXCHAR in tree TOOLBAR */

#define FONT2     19 /* form/dialog */
#define F2MENU     1 /* BOX in tree FONT2 */




#ifdef __STDC__
#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  else
#    define _WORD short
#  endif
#endif
extern _WORD toolbar_rsc_load(_WORD wchar, _WORD hchar);
extern _WORD toolbar_rsc_gaddr(_WORD type, _WORD idx, void *gaddr);
extern _WORD toolbar_rsc_free(void);
#endif
