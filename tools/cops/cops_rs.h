/*
 * resource set indices for cops_rs
 *
 * created by ORCS 2.17
 */

/*
 * Number of Strings:        327
 * Number of Bitblks:        2
 * Number of Iconblks:       0
 * Number of Color Iconblks: 1
 * Number of Color Icons:    1
 * Number of Tedinfos:       5
 * Number of Free Strings:   287
 * Number of Free Images:    0
 * Number of Objects:        40
 * Number of Trees:          6
 * Number of Userblks:       0
 * Number of Images:         2
 * Total file size:          11846
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
#define NUM_STRINGS 327
#define NUM_FRSTR 287
#define NUM_UD 0
#define NUM_IMAGES 2
#define NUM_BB 2
#define NUM_FRIMG 0
#define NUM_IB 0
#define NUM_CIB 1
#define NUM_TI 5
#define NUM_OBS 40
#define NUM_TREE 6
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


#define ABOUT_DIALOG                       0 /* form/dialog */
#define ABOUT_OK                           4 /* BUTTON in tree ABOUT_DIALOG */
#define ABOUT_BY                           5 /* TEXT in tree ABOUT_DIALOG */
#define COPS_ICON                          7 /* CICON in tree ABOUT_DIALOG */

#define INFO_DIALOG                        1 /* form/dialog */
#define CITITLE                            1 /* STRING in tree INFO_DIALOG */
#define INFO_FILENAME                      2 /* STRING in tree INFO_DIALOG */
#define CIFILE                             3 /* STRING in tree INFO_DIALOG */
#define INFO_VERSION                       4 /* STRING in tree INFO_DIALOG */
#define CIVER                              5 /* STRING in tree INFO_DIALOG */
#define INFO_ID                            6 /* STRING in tree INFO_DIALOG */
#define CIID                               7 /* STRING in tree INFO_DIALOG */
#define INFO_RAM                           9 /* BUTTON in tree INFO_DIALOG */
#define INFO_SETONLY                      10 /* BUTTON in tree INFO_DIALOG */
#define INFO_BOOTINIT                     11 /* BUTTON in tree INFO_DIALOG */
#define INFO_AUTOBOOT                     12 /* BUTTON in tree INFO_DIALOG */
#define INFO_OK                           13 /* BUTTON in tree INFO_DIALOG */
#define INFO_CANCEL                       14 /* BUTTON in tree INFO_DIALOG */

#define SET_DIALOG                         2 /* form/dialog */
#define SET_SETTINGS                       1 /* STRING in tree SET_DIALOG */
#define SET_SELPATH                        2 /* BUTTON in tree SET_DIALOG */
#define SET_PATH                           3 /* FTEXT in tree SET_DIALOG */
#define SET_ICONIFY                        4 /* BUTTON in tree SET_DIALOG */
#define SET_DCLICK                         5 /* BUTTON in tree SET_DIALOG */
#define SET_SORTNAME                       6 /* BUTTON in tree SET_DIALOG */
#define SET_TERM                           7 /* BUTTON in tree SET_DIALOG */
#define SET_TERMAFTER                      8 /* FTEXT in tree SET_DIALOG */
#define SET_OK                             9 /* BUTTON in tree SET_DIALOG */
#define SET_CANCEL                        10 /* BUTTON in tree SET_DIALOG */

#define ICONIFIED_DIALOG                   3 /* form/dialog */
#define ICFDLG_ROOT                        0 /* BOX in tree ICONIFIED_DIALOG */
#define ICFDLG_ICON                        1 /* BOX in tree ICONIFIED_DIALOG */

#define EMPTY_TREE                         4 /* form/dialog */

#define ICON_DIALOG                        5 /* form/dialog */
#define INACTIVE_IMG                       1 /* IMAGE in tree ICON_DIALOG */

#define ICNFTITLE_STR                      0 /* Free string */

#define WINDOWTITLE_STR_EN                 1 /* Free string */

#define MENUTITLE_STR_EN                   2 /* Free string */

#define CPXPATH_STR_EN                     3 /* Free string */

#define NOAES_STR_EN                       4 /* Free string */

#define NOWINDOW_ALERT_EN                  5 /* Alert string */

#define SAVE_DFLT_ALERT_EN                 6 /* Alert string */

#define MEM_ERR_ALERT_EN                   7 /* Alert string */

#define FILE_ERR_ALERT_EN                  8 /* Alert string */

#define FNF_ERR_ALERT_EN                   9 /* Alert string */

#define RELOAD_ALERT_EN                   10 /* Alert string */

#define AL_SAVE_HEADER_EN                 11 /* Alert string */

#define AL_NO_SOUND_DMA_EN                12 /* Alert string */

#define CPXLOAD_ALERT_EN                  13 /* Alert string */

#define QUIT_ALERT_EN                     14 /* Alert string */

#define OK_EN                             15 /* Free string */

#define CANCEL_EN                         16 /* Free string */

#define COPS_POPUP_ABOUT_EN               17 /* Free string */

#define COPS_POPUP_DIS1_EN                18 /* Free string */

#define COPS_POPUP_SETTINGS_EN            19 /* Free string */

#define COPS_POPUP_DIS2_EN                20 /* Free string */

#define COPS_POPUP_RELOAD_EN              21 /* Free string */

#define COPS_POPUP_TIDY_UP_EN             22 /* Free string */

#define COPS_POPUP_DIS3_EN                23 /* Free string */

#define COPS_POPUP_SELECT_ALL_EN          24 /* Free string */

#define COPS_POPUP_DIS4_EN                25 /* Free string */

#define COPS_POPUP_HELP_EN                26 /* Free string */

#define CPX_POPUP_OPEN_EN                 27 /* Free string */

#define CPX_POPUP_DIS1_EN                 28 /* Free string */

#define CPX_POPUP_DISABLE_EN              29 /* Free string */

#define CPX_POPUP_ENABLE_EN               30 /* Free string */

#define CPX_POPUP_DIS2_EN                 31 /* Free string */

#define CPX_POPUP_INFO_EN                 32 /* Free string */

#define ABOUT_BY_EN                       33 /* Free string */

#define INFO_FILENAME_EN                  34 /* Free string */

#define INFO_VERSION_EN                   35 /* Free string */

#define INFO_ID_EN                        36 /* Free string */

#define INFO_FRAME_EN                     37 /* Free string */

#define INFO_RAM_EN                       38 /* Free string */

#define INFO_SETONLY_EN                   39 /* Free string */

#define INFO_BOOTINIT_EN                  40 /* Free string */

#define INFO_AUTOBOOT_EN                  41 /* Free string */

#define SET_SETTINGS_EN                   42 /* Free string */

#define SET_SELPATH_EN                    43 /* Free string */

#define SET_ICONIFY_EN                    44 /* Free string */

#define SET_DCLICK_EN                     45 /* Free string */

#define SET_SORTNAME_EN                   46 /* Free string */

#define SET_TERM_EN                       47 /* Free string */

#define SET_TERMAFTER_EN                  48 /* Free string */

#define WINDOWTITLE_STR_DE                49 /* Free string */

#define MENUTITLE_STR_DE                  50 /* Free string */

#define CPXPATH_STR_DE                    51 /* Free string */

#define NOAES_STR_DE                      52 /* Free string */

#define NOWINDOW_ALERT_DE                 53 /* Alert string */

#define SAVE_DFLT_ALERT_DE                54 /* Alert string */

#define MEM_ERR_ALERT_DE                  55 /* Alert string */

#define FILE_ERR_ALERT_DE                 56 /* Alert string */

#define FNF_ERR_ALERT_DE                  57 /* Alert string */

#define RELOAD_ALERT_DE                   58 /* Alert string */

#define AL_SAVE_HEADER_DE                 59 /* Alert string */

#define AL_NO_SOUND_DMA_DE                60 /* Alert string */

#define CPXLOAD_ALERT_DE                  61 /* Alert string */

#define QUIT_ALERT_DE                     62 /* Alert string */

#define CANCEL_DE                         63 /* Free string */

#define COPS_POPUP_ABOUT_DE               64 /* Free string */

#define COPS_POPUP_DIS1_DE                65 /* Free string */

#define COPS_POPUP_SETTINGS_DE            66 /* Free string */

#define COPS_POPUP_DIS2_DE                67 /* Free string */

#define COPS_POPUP_RELOAD_DE              68 /* Free string */

#define COPS_POPUP_TIDY_UP_DE             69 /* Free string */

#define COPS_POPUP_DIS3_DE                70 /* Free string */

#define COPS_POPUP_SELECT_ALL_DE          71 /* Free string */

#define COPS_POPUP_DIS4_DE                72 /* Free string */

#define COPS_POPUP_HELP_DE                73 /* Free string */

#define CPX_POPUP_OPEN_DE                 74 /* Free string */

#define CPX_POPUP_DIS1_DE                 75 /* Free string */

#define CPX_POPUP_DISABLE_DE              76 /* Free string */

#define CPX_POPUP_ENABLE_DE               77 /* Free string */

#define CPX_POPUP_DIS2_DE                 78 /* Free string */

#define CPX_POPUP_INFO_DE                 79 /* Free string */

#define ABOUT_BY_DE                       80 /* Free string */

#define INFO_FILENAME_DE                  81 /* Free string */

#define INFO_VERSION_DE                   82 /* Free string */

#define INFO_ID_DE                        83 /* Free string */

#define INFO_FRAME_DE                     84 /* Free string */

#define INFO_RAM_DE                       85 /* Free string */

#define INFO_SETONLY_DE                   86 /* Free string */

#define INFO_BOOTINIT_DE                  87 /* Free string */

#define INFO_AUTOBOOT_DE                  88 /* Free string */

#define SET_SETTINGS_DE                   89 /* Free string */

#define SET_SELPATH_DE                    90 /* Free string */

#define SET_ICONIFY_DE                    91 /* Free string */

#define SET_DCLICK_DE                     92 /* Free string */

#define SET_SORTNAME_DE                   93 /* Free string */

#define SET_TERM_DE                       94 /* Free string */

#define SET_TERMAFTER_DE                  95 /* Free string */

#define WINDOWTITLE_STR_FR                96 /* Free string */

#define MENUTITLE_STR_FR                  97 /* Free string */

#define CPXPATH_STR_FR                    98 /* Free string */

#define NOAES_STR_FR                      99 /* Free string */

#define NOWINDOW_ALERT_FR                100 /* Alert string */

#define SAVE_DFLT_ALERT_FR               101 /* Alert string */

#define MEM_ERR_ALERT_FR                 102 /* Alert string */

#define FILE_ERR_ALERT_FR                103 /* Alert string */

#define FNF_ERR_ALERT_FR                 104 /* Alert string */

#define RELOAD_ALERT_FR                  105 /* Alert string */

#define AL_SAVE_HEADER_FR                106 /* Alert string */

#define AL_NO_SOUND_DMA_FR               107 /* Alert string */

#define CPXLOAD_ALERT_FR                 108 /* Alert string */

#define QUIT_ALERT_FR                    109 /* Alert string */

#define CANCEL_FR                        110 /* Free string */

#define COPS_POPUP_ABOUT_FR              111 /* Free string */

#define COPS_POPUP_DIS1_FR               112 /* Free string */

#define COPS_POPUP_SETTINGS_FR           113 /* Free string */

#define COPS_POPUP_DIS2_FR               114 /* Free string */

#define COPS_POPUP_RELOAD_FR             115 /* Free string */

#define COPS_POPUP_TIDY_UP_FR            116 /* Free string */

#define COPS_POPUP_DIS3_FR               117 /* Free string */

#define COPS_POPUP_SELECT_ALL_FR         118 /* Free string */

#define COPS_POPUP_DIS4_FR               119 /* Free string */

#define COPS_POPUP_HELP_FR               120 /* Free string */

#define CPX_POPUP_OPEN_FR                121 /* Free string */

#define CPX_POPUP_DIS1_FR                122 /* Free string */

#define CPX_POPUP_DISABLE_FR             123 /* Free string */

#define CPX_POPUP_ENABLE_FR              124 /* Free string */

#define CPX_POPUP_DIS2_FR                125 /* Free string */

#define CPX_POPUP_INFO_FR                126 /* Free string */

#define ABOUT_BY_FR                      127 /* Free string */

#define INFO_FILENAME_FR                 128 /* Free string */

#define INFO_VERSION_FR                  129 /* Free string */

#define INFO_ID_FR                       130 /* Free string */

#define INFO_FRAME_FR                    131 /* Free string */

#define INFO_RAM_FR                      132 /* Free string */

#define INFO_SETONLY_FR                  133 /* Free string */

#define INFO_BOOTINIT_FR                 134 /* Free string */

#define INFO_AUTOBOOT_FR                 135 /* Free string */

#define SET_SETTINGS_FR                  136 /* Free string */

#define SET_SELPATH_FR                   137 /* Free string */

#define SET_ICONIFY_FR                   138 /* Free string */

#define SET_DCLICK_FR                    139 /* Free string */

#define SET_SORTNAME_FR                  140 /* Free string */

#define SET_TERM_FR                      141 /* Free string */

#define SET_TERMAFTER_FR                 142 /* Free string */

#define WINDOWTITLE_STR_ES               143 /* Free string */

#define MENUTITLE_STR_ES                 144 /* Free string */

#define CPXPATH_STR_ES                   145 /* Free string */

#define NOAES_STR_ES                     146 /* Free string */

#define NOWINDOW_ALERT_ES                147 /* Alert string */

#define SAVE_DFLT_ALERT_ES               148 /* Alert string */

#define MEM_ERR_ALERT_ES                 149 /* Alert string */

#define FILE_ERR_ALERT_ES                150 /* Alert string */

#define FNF_ERR_ALERT_ES                 151 /* Alert string */

#define RELOAD_ALERT_ES                  152 /* Alert string */

#define AL_SAVE_HEADER_ES                153 /* Alert string */

#define AL_NO_SOUND_DMA_ES               154 /* Alert string */

#define CPXLOAD_ALERT_ES                 155 /* Alert string */

#define QUIT_ALERT_ES                    156 /* Alert string */

#define OK_ES                            157 /* Free string */

#define CANCEL_ES                        158 /* Free string */

#define COPS_POPUP_ABOUT_ES              159 /* Free string */

#define COPS_POPUP_DIS1_ES               160 /* Free string */

#define COPS_POPUP_SETTINGS_ES           161 /* Free string */

#define COPS_POPUP_DIS2_ES               162 /* Free string */

#define COPS_POPUP_RELOAD_ES             163 /* Free string */

#define COPS_POPUP_TIDY_UP_ES            164 /* Free string */

#define COPS_POPUP_DIS3_ES               165 /* Free string */

#define COPS_POPUP_SELECT_ALL_ES         166 /* Free string */

#define COPS_POPUP_DIS4_ES               167 /* Free string */

#define COPS_POPUP_HELP_ES               168 /* Free string */

#define CPX_POPUP_OPEN_ES                169 /* Free string */

#define CPX_POPUP_DIS1_ES                170 /* Free string */

#define CPX_POPUP_DISABLE_ES             171 /* Free string */

#define CPX_POPUP_ENABLE_ES              172 /* Free string */

#define CPX_POPUP_DIS2_ES                173 /* Free string */

#define CPX_POPUP_INFO_ES                174 /* Free string */

#define ABOUT_BY_ES                      175 /* Free string */

#define INFO_FILENAME_ES                 176 /* Free string */

#define INFO_VERSION_ES                  177 /* Free string */

#define INFO_ID_ES                       178 /* Free string */

#define INFO_FRAME_ES                    179 /* Free string */

#define INFO_RAM_ES                      180 /* Free string */

#define INFO_SETONLY_ES                  181 /* Free string */

#define INFO_BOOTINIT_ES                 182 /* Free string */

#define INFO_AUTOBOOT_ES                 183 /* Free string */

#define SET_SETTINGS_ES                  184 /* Free string */

#define SET_SELPATH_ES                   185 /* Free string */

#define SET_ICONIFY_ES                   186 /* Free string */

#define SET_DCLICK_ES                    187 /* Free string */

#define SET_SORTNAME_ES                  188 /* Free string */

#define SET_TERM_ES                      189 /* Free string */

#define SET_TERMAFTER_ES                 190 /* Free string */

#define WINDOWTITLE_STR_IT               191 /* Free string */

#define MENUTITLE_STR_IT                 192 /* Free string */

#define CPXPATH_STR_IT                   193 /* Free string */

#define NOAES_STR_IT                     194 /* Free string */

#define NOWINDOW_ALERT_IT                195 /* Alert string */

#define SAVE_DFLT_ALERT_IT               196 /* Alert string */

#define MEM_ERR_ALERT_IT                 197 /* Alert string */

#define FILE_ERR_ALERT_IT                198 /* Alert string */

#define FNF_ERR_ALERT_IT                 199 /* Alert string */

#define RELOAD_ALERT_IT                  200 /* Alert string */

#define AL_SAVE_HEADER_IT                201 /* Alert string */

#define AL_NO_SOUND_DMA_IT               202 /* Alert string */

#define CPXLOAD_ALERT_IT                 203 /* Alert string */

#define QUIT_ALERT_IT                    204 /* Alert string */

#define OK_IT                            205 /* Free string */

#define CANCEL_IT                        206 /* Free string */

#define COPS_POPUP_ABOUT_IT              207 /* Free string */

#define COPS_POPUP_DIS1_IT               208 /* Free string */

#define COPS_POPUP_SETTINGS_IT           209 /* Free string */

#define COPS_POPUP_DIS2_IT               210 /* Free string */

#define COPS_POPUP_RELOAD_IT             211 /* Free string */

#define COPS_POPUP_TIDY_UP_IT            212 /* Free string */

#define COPS_POPUP_DIS3_IT               213 /* Free string */

#define COPS_POPUP_SELECT_ALL_IT         214 /* Free string */

#define COPS_POPUP_DIS4_IT               215 /* Free string */

#define COPS_POPUP_HELP_IT               216 /* Free string */

#define CPX_POPUP_OPEN_IT                217 /* Free string */

#define CPX_POPUP_DIS1_IT                218 /* Free string */

#define CPX_POPUP_DISABLE_IT             219 /* Free string */

#define CPX_POPUP_ENABLE_IT              220 /* Free string */

#define CPX_POPUP_DIS2_IT                221 /* Free string */

#define CPX_POPUP_INFO_IT                222 /* Free string */

#define ABOUT_BY_IT                      223 /* Free string */

#define INFO_FILENAME_IT                 224 /* Free string */

#define INFO_VERSION_IT                  225 /* Free string */

#define INFO_ID_IT                       226 /* Free string */

#define INFO_FRAME_IT                    227 /* Free string */

#define INFO_RAM_IT                      228 /* Free string */

#define INFO_SETONLY_IT                  229 /* Free string */

#define INFO_BOOTINIT_IT                 230 /* Free string */

#define INFO_AUTOBOOT_IT                 231 /* Free string */

#define SET_SETTINGS_IT                  232 /* Free string */

#define SET_SELPATH_IT                   233 /* Free string */

#define SET_ICONIFY_IT                   234 /* Free string */

#define SET_DCLICK_IT                    235 /* Free string */

#define SET_SORTNAME_IT                  236 /* Free string */

#define SET_TERM_IT                      237 /* Free string */

#define SET_TERMAFTER_IT                 238 /* Free string */

#define WINDOWTITLE_STR_SV               239 /* Free string */

#define MENUTITLE_STR_SV                 240 /* Free string */

#define CPXPATH_STR_SV                   241 /* Free string */

#define NOAES_STR_SV                     242 /* Free string */

#define NOWINDOW_ALERT_SV                243 /* Alert string */

#define SAVE_DFLT_ALERT_SV               244 /* Alert string */

#define MEM_ERR_ALERT_SV                 245 /* Alert string */

#define FILE_ERR_ALERT_SV                246 /* Alert string */

#define FNF_ERR_ALERT_SV                 247 /* Alert string */

#define RELOAD_ALERT_SV                  248 /* Alert string */

#define AL_SAVE_HEADER_SV                249 /* Alert string */

#define AL_NO_SOUND_DMA_SV               250 /* Alert string */

#define CPXLOAD_ALERT_SV                 251 /* Alert string */

#define QUIT_ALERT_SV                    252 /* Alert string */

#define OK_SV                            253 /* Free string */

#define CANCEL_SV                        254 /* Free string */

#define COPS_POPUP_ABOUT_SV              255 /* Free string */

#define COPS_POPUP_DIS1_SV               256 /* Free string */

#define COPS_POPUP_SETTINGS_SV           257 /* Free string */

#define COPS_POPUP_DIS2_SV               258 /* Free string */

#define COPS_POPUP_RELOAD_SV             259 /* Free string */

#define COPS_POPUP_TIDY_UP_SV            260 /* Free string */

#define COPS_POPUP_DIS3_SV               261 /* Free string */

#define COPS_POPUP_SELECT_ALL_SV         262 /* Free string */

#define COPS_POPUP_DIS4_SV               263 /* Free string */

#define COPS_POPUP_HELP_SV               264 /* Free string */

#define CPX_POPUP_OPEN_SV                265 /* Free string */

#define CPX_POPUP_DIS1_SV                266 /* Free string */

#define CPX_POPUP_DISABLE_SV             267 /* Free string */

#define CPX_POPUP_ENABLE_SV              268 /* Free string */

#define CPX_POPUP_DIS2_SV                269 /* Free string */

#define CPX_POPUP_INFO_SV                270 /* Free string */

#define ABOUT_BY_SV                      271 /* Free string */

#define INFO_FILENAME_SV                 272 /* Free string */

#define INFO_VERSION_SV                  273 /* Free string */

#define INFO_ID_SV                       274 /* Free string */

#define INFO_FRAME_SV                    275 /* Free string */

#define INFO_RAM_SV                      276 /* Free string */

#define INFO_SETONLY_SV                  277 /* Free string */

#define INFO_BOOTINIT_SV                 278 /* Free string */

#define INFO_AUTOBOOT_SV                 279 /* Free string */

#define SET_SETTINGS_SV                  280 /* Free string */

#define SET_SELPATH_SV                   281 /* Free string */

#define SET_ICONIFY_SV                   282 /* Free string */

#define SET_DCLICK_SV                    283 /* Free string */

#define SET_SORTNAME_SV                  284 /* Free string */

#define SET_TERM_SV                      285 /* Free string */

#define SET_TERMAFTER_SV                 286 /* Free string */




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
