
#ifndef _tw_config_h_
#define _tw_config_h_


/*
 * Variablen
 */
extern bool	gl_avcycle,
		gl_shortcut,
		gl_allogin;

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
