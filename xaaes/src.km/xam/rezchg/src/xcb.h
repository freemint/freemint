#ifndef _XCB_H
#define _XCB_H

/* Auflsungsstruktur */
/* Lnge der Struktur: 86 Bytes */
struct nova_res
{
	char	name[33];	/* Name der Auflsung */
				/* Fr Nicht-C-Programmierer: */
				/* der Offset der nchsten Variablen */
				/* zum Strukturstart betrgt 34 Bytes */
	short	mode;		/* Auflsungsart (siehe ICB.H) */
	short	bypl;		/* Bytes pro Zeile */
	short	planes;		/* Anzahl Planes */
	unsigned short	colors;	/* Anzahl Farben */
	short	hc_mode;	/* Hardcopy-Modus */
	short	max_x;		/* maximale x- und y-koordinate */
	short	max_y;
	short	real_x;		/* reale max. x- und u-koordinate */
	short	real_y;
				/* folgende Variablen sollten nicht */
				/* beachtet werden */
	short	freq;		/* Frequenz in MHz */
	char	freq2;		/* 2. Frequenz (SIGMA Legend II) */
	char	low_res;	/* halbe Pixelrate */
	char	r_3c2;		/* Register 3c2 */
	char	r_3d4[25];	/* Register 3d4, Index 0 bis $18 */
	char	extended[3];	/* Register 3d4, Index $33 bis $35 */
};
typedef struct nova_res NOVARES;

/* alle Zeiger auf Routinen erwarten ihre Parameter im */
/* Turbo-/Pure-C-Format: WORD-Parameter in ihrer Reihenfolge in */
/* den Registern d0-d2, Zeiger in den Registern a0-a1 */
struct xcb
{
	long	version;
	unsigned char	resolution;
	unsigned char	blnk_time;
	unsigned char	ms_speed;
	char		old_res;
	void		(*p_chres)(NOVARES *res);
	short		mode;
	short		bypl;
	short		planes;
	short		colors;
	short		hc;
	short		max_x;
	short		max_y;
	short		rmn_x;
	short		rmx_x;
	short		rmn_y;
	short		rmx_y;

	short		v_top;
	short		v_bottom;
	short		v_left;
	short		v_right;

	void		(*p_setcol)(short index, unsigned char *colors);
	void		(*p_chng_vrt)(short x, short y);
	void		(*inst_xbios)(short flag);
	void		(*pic_on)(short flag);
	void		(*chng_pos)(void);
	void		(*p_setscr)(void *adr);
	void		*base;
	void		*scr_base;
	short		scrn_cnt;
	unsigned long	scrn_siz;
	void		*reg_base;
	void		(*p_vsync)(void);
	char		name[36];
	unsigned long	mem_size;

	/* This stuff below here is only there from version 1.20!! */
	char		hw_flags[8];	/* Four card-dependant bytes */
	short		cpu;
	void		(*set_gamma)(unsigned char *gamma);
	short		gamma_able;
	short		gamma_rgb[3];
	unsigned char	*mem_reg;
};
typedef struct xcb XCB;

struct nova_data
{
// 	struct xcb *xcb;
	short valid;
	struct nova_res next_res;
};

#endif	/* _ICB_H */
