
#ifndef _tw_textwin_h_
#define _tw_textwin_h_


void	char2pixel (TEXTWIN *t, short col, short row, short *xp, short *yp);
void 	pixel2char (TEXTWIN *t, short x, short y, short *colp, short *rowp);
void 	mark_clean (TEXTWIN *t);
void 	refresh_textwin (TEXTWIN *t, short force);

TEXTWIN *create_textwin (char *title, WINCFG *cfg);
void 	destroy_textwin (TEXTWIN *t);

void 	resize_textwin (TEXTWIN *t, short cols, short rows, short scrollback);
void	soft_resize_textwin (TEXTWIN *t, short cols, short rows,
			     short scrollback);
void	textwin_setfont (TEXTWIN *t, short font, short points);
void	reconfig_textwin (TEXTWIN *t, WINCFG *cfg);

void 	sendstr (TEXTWIN *t, char *s);
void	write_text (TEXTWIN *t, char *b, long len);

TEXTWIN *get_textwin (short handle, short talkID);

void	textwin_term (void);


#endif
