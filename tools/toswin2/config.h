
#ifndef tw_config_h
# define tw_config_h

#ifndef tw_global_h
# include "global.h"
#endif
#ifndef tw_window_h
# include "window.h"
#endif

typedef struct _wincfg WINCFG;
struct _wincfg
{
	WINCFG	*next;
	char	progname[80];
	char	arg[50];
	char	title[80];
	int	kind;
	int	font_id, font_pts;
	int	col, row, scroll;
	int	xpos, ypos;
	int	width, height;		/* 'offene' Gr”že */
	bool	iconified;
	bool	wrap;
	bool	autoclose;
	int	vt_mode;
	int	fg_color;
	int	bg_color;
	int	vdi_colors;
	unsigned long	fg_effects;
	unsigned long	bg_effects;
	int	char_tab;
};

/*
 * Variablen
 */
extern bool	gl_shortcut;
extern bool	gl_allogin;

extern bool	gl_con_auto,
		gl_con_output,
		gl_con_log;
extern char	gl_con_logname[];

extern WINCFG	*gl_wincfg;


/*
 * Funktionen
 */
void 	config_init	(void);
void	config_term	(void);


/* Datei */
bool 	config_load	(void);
void 	config_save	(void);


/* Dialog */
void 	winconfig_open	(void);
void	conconfig_open	(void);

void	update_font	(WINDOW *w, int id, int pts);


/*
 * Liefert die CFG zu dem Programm.
 */
WINCFG *get_wincfg	(const char *prog);


#endif
