#ifndef _tw_textwin_h_
#define _tw_textwin_h_

extern void		char2pixel(TEXTWIN *t, int col, int row, int *xp, int *yp);
extern void 	pixel2char(TEXTWIN *t, int x, int y, int *colp, int *rowp);
extern void 	mark_clean(TEXTWIN *t);
extern void 	refresh_textwin(TEXTWIN *t, int force);

extern TEXTWIN *create_textwin(char *title, WINCFG *cfg);
extern void 	destroy_textwin(TEXTWIN *t);

extern void 	resize_textwin(TEXTWIN *t, int cols, int rows, int scrollback);
extern void		textwin_setfont(TEXTWIN *t, int font, int points);
extern void		reconfig_textwin(TEXTWIN *t, WINCFG *cfg);

extern void 	sendstr(TEXTWIN *t, char *s);
extern void		write_text(TEXTWIN *t, char *b, long len);

extern TEXTWIN *get_textwin(int handle, int talkID);

extern void		textwin_term(void);

#endif
