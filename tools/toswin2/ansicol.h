/* Declarations for ANSI color stuff.
 * Copyright (C) 2001, Guido Flohr <guido@freemint.de>,
 * all rights reserved.
 */
 
#ifndef tw_ansicol_h
# define tw_ansicol_h 1

#define ANSI_BLACK	0
#define ANSI_RED	1
#define ANSI_GREEN	2
#define ANSI_YELLOW	3
#define ANSI_BLUE	4
#define ANSI_MAGENTA	5
#define ANSI_CYAN	6
#define ANSI_WHITE	7
#define ANSI_DEFAULT	9

#define ANSI_DEFAULT_FGCOLOR ANSI_WHITE
#define ANSI_DEFAULT_BGCOLOR ANSI_BLACK

extern const int ansi2vdi[8];
extern const int vdi2ansi[8];

extern char* tw52_env;
extern char* tw100_env;
extern char* colorterm_env;

void init_ansi_colors (const short* __work_out);
void set_ansi_fg_color  (TEXTWIN* __textwin, int __color);
void set_ansi_bg_color  (TEXTWIN* __textwin, int __color);
void use_ansi_colors    (TEXTWIN* __textwin, unsigned long __flag, 
			 int* __fgcolor, int* __bgcolor,
			 int* __texteffects);
int get_ansi_color (int __color);

#endif
