#ifndef _tw_config_h_
#define _tw_config_h_

/*
 * Variablen
*/
extern bool		gl_avcycle,
					gl_shortcut,
					gl_allogin;

extern bool		gl_con_auto,
					gl_con_output,
					gl_con_log;
extern char		gl_con_logname[];

extern WINCFG	*gl_wincfg;


/*
 * Funktionen
*/
extern void 	config_init		(void);
extern void		config_term		(void);


/* Datei */
extern bool 	config_load		(void);
extern void 	config_save		(void);


/* Dialog */
extern void 	winconfig_open	(void);
extern void		conconfig_open	(void);

extern void		update_font		(WINDOW *w, int id, int pts);

extern WINCFG 	*get_wincfg(const char *prog);
/*
 * Liefert die CFG zu dem Programm.
 */

#endif
