/*
 * XaAES - XaAES Ain't the AES
 *
 * A multitasking AES replacement for MiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _vdi_h
#define _vdi_h

/* normal graphics drawing modes */
#define MD_REPLACE		1
#define MD_TRANS		2
#define MD_XOR			3
#define MD_ERASE		4

/* bit blt rules */
#define ALL_WHITE		0
#define S_AND_D			1
#define S_AND_NOTD		2
#define S_ONLY			3
#define NOTS_AND_D		4
#define D_ONLY			5
#define S_XOR_D			6
#define S_OR_D			7
#define NOT_SORD		8
#define NOT_SXORD		9
#define D_INVERT		10
#define NOT_D			10
#define S_OR_NOTD		11
#define NOT_S			12
#define NOTS_OR_D		13
#define NOT_SANDD		14
#define ALL_BLACK		15

/* v_bez modes */
#define BEZ_BEZIER		0x01
#define BEZ_POLYLINE		0x00
#define BEZ_NODRAW		0x02

/* v_bit_image modes */
#define IMAGE_LEFT		0
#define IMAGE_CENTER		1
#define IMAGE_RIGHT		2
#define IMAGE_TOP 		0
#define IMAGE_BOTTOM		2

/* v_justified modes */
#define NOJUSTIFY		0
#define JUSTIFY			1

/* vq_color modes */
#define COLOR_REQUESTED		0
#define COLOR_ACTUAL		1

/* return values for vq_vgdos() inquiry */
#define GDOS_NONE		(-2L)		/* no GDOS installed */
#define GDOS_FSM		0x5F46534DL	/* '_FSM' */
#define GDOS_FNT		0x5F464E54L	/* '_FNT' */

/* vqin_mode & vsin_mode modes */
#define LOCATOR			1
#define VALUATOR		2
#define CHOICE			3
#define STRING			4

/* vqt_cachesize modes */
#define CACHE_CHAR		0
#define CACHE_MISC		1

/* vqt_devinfo return values */
#define DEV_MISSING		0
#define DEV_INSTALLED		1

/* vqt_name return values */
#define BITMAP_FONT		0

/* vsf_interior modes */
#define FIS_HOLLOW		0
#define FIS_SOLID		1
#define FIS_PATTERN		2
#define FIS_HATCH		3
#define FIS_USER		4

/* vsf_perimeter modes */
#define PERIMETER_OFF		0
#define PERIMETER_ON		1

/* vsl_ends modes */
#define SQUARE			0
#define ARROWED			1
#define ROUND			2

/* vsl_type modes */
#define SOLID			1
#define LDASHED			2
#define DOTTED			3
#define DASHDOT			4
#define DASH			5
#define DASHDOTDOT		6
#define USERLINE		7

/* vsm_type modes */
#define MRKR_DOT		1
#define MRKR_PLUS 		2
#define MRKR_ASTERISK		3
#define MRKR_BOX		4
#define MRKR_CROSS		5
#define MRKR_DIAMOND		6

/* vst_alignment modes */
#define TA_LEFT         	0 /* horizontal */
#define TA_CENTER       	1
#define TA_RIGHT        	2
#define TA_BASE         	0 /* vertical */
#define TA_HALF         	1
#define TA_ASCENT       	2
#define TA_BOTTOM       	3
#define TA_DESCENT      	4
#define TA_TOP          	5

/* vst_charmap modes */
#define MAP_BITSTREAM   	0
#define MAP_ATARI       	1
#define MAP_UNICODE     	2 /* for vst_map_mode, NVDI 4 */

/* vst_effects modes */
#define TXT_NORMAL		0x0000
#define TXT_THICKENED		0x0001
#define TXT_LIGHT		0x0002
#define TXT_SKEWED		0x0004
#define TXT_UNDERLINED		0x0008
#define TXT_OUTLINED		0x0010
#define TXT_SHADOWED		0x0020

/* vst_error modes */
#define APP_ERROR		0
#define SCREEN_ERROR		1

/* vst_error return values */
#define NO_ERROR		0
#define CHAR_NOT_FOUND		1
#define FILE_READERR 		8
#define FILE_OPENERR 		9
#define BAD_FORMAT		10
#define CACHE_FULL		11
#define MISC_ERROR		(-1)

/* vst_kern tmodes */
#define TRACK_NONE		0
#define TRACK_NORMAL 		1
#define TRACK_TIGHT		2
#define TRACK_VERYTIGHT		3

/* vst_kern pmodes */
#define PAIR_OFF		0
#define PAIR_ON			1

/* vst_scratch modes */
#define SCRATCH_BOTH		0
#define SCRATCH_BITMAP		1
#define SCRATCH_NONE		2

/* v_updwk return values */
#define SLM_OK			0x00
#define SLM_ERROR		0x02
#define SLM_NOTONER		0x03
#define SLM_NOPAPER		0x04

#ifndef __PXY
# define __PXY
typedef struct point_coord
{
	short p_x;
	short p_y;
} PXY;
#endif

/* VDI Memory Form Definition Block */
typedef struct memory_form
{
	void	*fd_addr;
	short 	fd_w;		/* Form Width in Pixels */
	short 	fd_h; 		/* Form Height in Pixels */
	short 	fd_wdwidth;	/* Form Width in shorts(fd_w/sizeof(short) */
	short 	fd_stand;	/* Form format 0= device spec 1=standard */
	short 	fd_nplanes;	/* Number of memory planes */
	short 	fd_r1;		/* Reserved */
	short 	fd_r2;		/* Reserved */
	short 	fd_r3;		/* Reserved */
} MFDB;

/* RGB intesities in promille */
typedef struct rgb_1000
{ short  red;    /* Red-Intensity in range [0..1000] */
  short  green;  /* Green-Intensity in range [0..1000] */
  short  blue;   /* Blue-Intensity in range [0..1000] */
} RGB1000;


/*******************************************************************************
 * The VDI bindings
 */

typedef short VdiHdl;   /* for better readability */

/*
 * attribute functions 
 */
void  vs_color  (VdiHdl , short color_idx, short rgb[]);
short vswr_mode (VdiHdl , short mode);

short vsf_color     (VdiHdl , short color_idx);
short vsf_interior  (VdiHdl , short style);
short vsf_perimeter (VdiHdl , short vis);
short vsf_style     (VdiHdl , short style);
void  vsf_udpat     (VdiHdl , short pat[], short planes);

short vsl_color (VdiHdl , short color_idx);
void  vsl_ends  (VdiHdl , short begstyle, short endstyle);
short vsl_type  (VdiHdl , short style);
void  vsl_udsty (VdiHdl , short pat);
short vsl_width (VdiHdl , short width);

short vsm_color  (VdiHdl , short color_idx);
short vsm_height (VdiHdl , short height);
short vsm_type   (VdiHdl , short symbol);

void  vst_alignment (VdiHdl , short hin, short vin, short *hout, short *vout);
short vst_color     (VdiHdl , short color_idx);
short vst_effects   (VdiHdl , short effects);
void  vst_error     (VdiHdl , short mode, short *errorvar);
short vst_font      (VdiHdl , short font);
void  vst_height    (VdiHdl , short height, short *charw, short *charh,
                                            short *cellw, short *cellh);
short vst_point     (VdiHdl , short point, short *charw, short *charh,
                                           short *cellw, short *cellh);
short vst_rotation  (VdiHdl , short ang);
void  vst_scratch   (VdiHdl , short mode);

/*
 * control functions
 */
void  v_clrwk          (VdiHdl );
void  v_clsvwk         (VdiHdl );
void  v_clswk          (VdiHdl );
short v_flushcache     (VdiHdl );
short v_loadcache      (VdiHdl , const char *filename, short mode);
void  v_opnvwk         (short work_in[], VdiHdl *, short work_out[]);
void  v_opnwk          (short work_in[], VdiHdl *, short work_out[]);
short v_savecache      (VdiHdl , const char *filename);
void  v_set_app_buff   (VdiHdl , void *buf_p, short size);
void  v_updwk          (VdiHdl );
void  vs_clip          (VdiHdl , short clip_flag, short pxy[]);
void  vs_clip_pxy      (VdiHdl , PXY pxy[]);
void  vs_clip_off      (VdiHdl );
short vst_load_fonts   (VdiHdl , short /* select */);
void  vst_unload_fonts (VdiHdl , short /* select */);

/*
 * escape functions
 */
void  v_bit_image       (VdiHdl , const char *filename, short aspect,
                                  short x_scale, short y_scale,
                                  short h_align, short v_align, short *pxy);
void  v_clear_disp_list (VdiHdl );
short v_copies          (VdiHdl , short count);
void  v_dspcur          (VdiHdl , short x, short y);
void  v_form_adv        (VdiHdl );
void  v_hardcopy        (VdiHdl );
short v_orient          (VdiHdl , short orientation);
void  v_output_window   (VdiHdl , short *pxy);
short v_page_size       (VdiHdl , short page_id);
void  v_rmcur           (VdiHdl );
void  v_trays           (VdiHdl , short input, short output,
                                  short *set_input, short *set_output);
short vq_calibrate      (VdiHdl , short *flag);
short vq_page_name      (VdiHdl , short page_id, char *page_name,
                                  long *page_width, long *page_height);
void  vq_scan           (VdiHdl , short *g_slice, short *g_page,
                                  short *a_slice, short *a_page, short *div_fac);
short vq_tabstatus      (VdiHdl );
void  vq_tray_names     (VdiHdl , char *input_name, char *output_name,
                                  short *input, short *output);
short vs_calibrate      (VdiHdl , short flag, short *rgb);
short vs_palette        (VdiHdl , short palette);

void vq_tdimensions (VdiHdl , short *xdimension, short *ydimension);
void vt_alignment   (VdiHdl , short dx, short dy);
void vt_axis        (VdiHdl , short xres, short yres, short *xset, short *yset);
void vt_origin      (VdiHdl , short xorigin, short yorigin);
void vt_resolution  (VdiHdl , short xres, short yres, short *xset, short *yset);

void v_meta_extents (VdiHdl , short min_x, short min_y,
                              short max_x, short max_y);
void v_write_meta   (VdiHdl , short numvdi_intin, short *avdi_intin,
                              short num_ptsin, short *a_ptsin);
void vm_coords      (VdiHdl , short llx, short lly, short urx, short ury);
void vm_filename    (VdiHdl , const char *filename);
void vm_pagesize    (VdiHdl , short pgwidth, short pgheight);

void vsc_expose (VdiHdl , short state);
void vsp_film   (VdiHdl , short color_idx, short lightness);

void v_escape2000 (VdiHdl , short times);

void v_alpha_text  (VdiHdl , const char *str);
void v_curdown     (VdiHdl );
void v_curhome     (VdiHdl );
void v_curleft     (VdiHdl );
void v_curright    (VdiHdl );
void v_curtext     (VdiHdl , const char *str);
void v_curup       (VdiHdl );
void v_eeol        (VdiHdl );
void v_eeos        (VdiHdl );
void v_enter_cur   (VdiHdl );
void v_exit_cur    (VdiHdl );
void v_rvoff       (VdiHdl );
void v_rvon        (VdiHdl );
void vq_chcells    (VdiHdl , short *n_rows, short *n_cols);
void vq_curaddress (VdiHdl , short *cur_row, short *cur_col);
void vs_curaddress (VdiHdl , short row, short col);
 

/*
 * inquiry functions
 */
void  vq_cellarray   (VdiHdl , short pxy[], short row_len, short nrows,
                               short *el_used, short *rows_used,
                               short *status, short color[]);
short vq_color       (VdiHdl , short color_idx, short flag, short rgb[]);
void  vq_extnd       (VdiHdl , short flag, short work_out[]);
void  vqf_attributes (VdiHdl , short atrib[]);
void  vqin_mode      (VdiHdl , short dev, short *mode);
void  vql_attributes (VdiHdl , short atrib[]);
void  vqm_attributes (VdiHdl , short atrib[]);
void  vqt_attributes (VdiHdl , short atrib[]);
void  vqt_cachesize  (VdiHdl , short which_cache, long *size);
void  vqt_extent     (VdiHdl , const char *str, short extent[]);
void  vqt_extent16   (VdiHdl , const short *wstr, short extent[]);
void  vqt_extent16n  (VdiHdl , const short *wstr, short num, short extent[]);
void  vqt_fontinfo   (VdiHdl , short *minade, short *maxade, short distances[],
                               short *maxwidth, short effects[]);
void  vqt_get_table  (VdiHdl , short **map);
short vqt_name       (VdiHdl , short element, char *name);
short vqt_width      (VdiHdl , short chr, short *cw,
                               short *ldelta, short *rdelta);

short vq_gdos  (void);
long  vq_vgdos (void);


/*
 * input function
 */
void  v_hide_c     (VdiHdl );
void  v_show_c     (VdiHdl , short reset);
void  vex_butv     (VdiHdl , void *pusrcode, void **psavcode);
void  vex_curv     (VdiHdl , void *pusrcode, void **psavcode);
void  vex_motv     (VdiHdl , void *pusrcode, void **psavcode);
void  vex_timv     (VdiHdl , void *time_addr,
                             void **otime_addr, short *time_conv);
void  vq_key_s     (VdiHdl , short *state);
void  vq_mouse     (VdiHdl , short *pstatus, short *x, short *y);
void  vrq_choice   (VdiHdl , short cin, short *cout);
void  vrq_locator  (VdiHdl , short x, short y,
                             short *xout, short *yout, short *term);
void  vrq_string   (VdiHdl , short len, short echo, short echoxy[], char *str);
void  vrq_valuator (VdiHdl , short in, short *out, short *term);
void  vsc_form     (VdiHdl , struct mfstr *form);
short vsin_mode    (VdiHdl , short dev, short mode);
short vsm_choice   (VdiHdl , short *choice);
short vsm_locator  (VdiHdl , short x, short y,
                             short *xout, short *yout, short *term);
short vsm_string   (VdiHdl , short len, short echo, short echoxy[], char *str);
void  vsm_valuator (VdiHdl , short in, short *out, short *term, short *status);


/*
 * output functions
 */
void v_arc         (VdiHdl , short x, short y,
                             short radius, short begang, short endang);
void v_bar         (VdiHdl , short pxy[]);
void v_cellarray   (VdiHdl , short pxy[], short row_length, short elements,
                             short nrows, short write_mode, short colarray[]);
void v_circle      (VdiHdl , short x, short y, short radius);
void v_contourfill (VdiHdl , short x, short y, short color_idx);
void v_ellarc      (VdiHdl , short x, short y, short xrad, short yrad,
                             short begang, short endang);
void v_ellipse     (VdiHdl , short x, short y, short xrad, short yrad);
void v_ellpie      (VdiHdl , short x, short y, short xrad, short yrad,
                             short begang, short endang);
void v_fillarea    (VdiHdl , short count, short pxy[]);
void v_gtext       (VdiHdl , short x, short y, const char *str);
void v_gtext16     (VdiHdl , short x, short y, const short *wstr);
void v_gtext16n    (VdiHdl , PXY pos, const short *wstr, short num);
void v_justified   (VdiHdl , short x, short y, const char *str,
                             short len, short word_space, short char_space);
void v_pieslice    (VdiHdl , short x, short y,
                             short radius, short begang, short endang);
void v_pline       (VdiHdl , short count, short pxy[]);
void v_pmarker     (VdiHdl , short count, short pxy[]);
void v_rbox        (VdiHdl , short pxy[]);
void v_rfbox       (VdiHdl , short pxy[]);
void vr_recfl      (VdiHdl , short pxy[]);

/*
 * raster functions
 */
void v_get_pixel (VdiHdl , short x, short y, short *pel, short *color_idx);
void vr_trnfm    (VdiHdl , MFDB *src, MFDB *dst);
void vro_cpyfm   (VdiHdl , short mode, short pxy[], MFDB *src, MFDB *dst);
void vrt_cpyfm   (VdiHdl , short mode, short pxy[], MFDB *src, MFDB *dst,
                           short color[]);


/*
 * Some usefull extensions.
 */
void  vdi_array2str (const short *src, char  *des, short len);
short vdi_str2array (const char  *src, short *des);
short vdi_wstrlen   (const short *wstr);

/*
 * vdi trap interface
 */

/* Array sizes in vdi control block */
#define VDI_CNTRLMAX     15
#define VDI_INTINMAX   1024
#define VDI_INTOUTMAX   256
#define VDI_PTSINMAX    256
#define VDI_PTSOUTMAX   256

typedef struct
{
 short       *control;
 const short *intin;
 const short *ptsin;
 short       *intout;
 short       *ptsout;
} VDIPB;

void vdi (VDIPB *pb);

#endif /* _vdi_h */
