#include <string.h>
#include <stdio.h>

#define LINSIZ 128

#ifdef __STDC__
#define P_(x) x
#else
#define P_(x) ()
#define const
#endif

/* are we compiling on an Atari? */
#if defined(atarist) || defined(ATARI)
#define NATIVEATARI 1
#endif

#if defined(__STDC__) && !defined(NO_STDLIB)
# include <stdlib.h>
#else
#define size_t int
extern char *malloc();
#endif

#define GAS 0
#define ASM 1
#define PUREC 2
extern int syntax;

/* this variable should always be 0
 * at the end of translation */
extern int ifstkptr;

char *concat P_((char *, char *));
char *concat3 P_((char *, char *, char *));
char *concat4 P_((char *, char *, char *, char *));
char *concat5 P_((char *,char *, char *, char *, char *));
char *concat6 P_((char *,char *,char *, char *, char *, char *));
char *concat8 P_((char *,char *,char *, char *, char *, char *, char *, char *));
char *concat9 P_((char *,char *,char *, char *, char *, char *, char *, char *, char *));
void do_define P_((char *, char *));
void do_ifdef P_((char *));
void do_ifndef P_((char *));
void do_else P_((void));
void do_endif P_((void));
void do_include P_((char *));
void do_initial_defs P_((void));
void yyerror P_((char *));

char *wordlookup P_((char *));
char *changesiz P_((char *));
char *changesiz2 P_((char *));
char *fixupword P_((char *));
void emit P_((char *));

char *immediate P_((char *));
char *indirect P_((char *));
char *postinc P_((char *));
char *predec P_((char *));
char *indexed P_((char *, char *));
char *sizedop P_((char *, char *));
char *twoindex P_((char *, char *, char *));
char *postindex P_((char *, char *, char *, char *));
char *postindex0 P_((char *));
char *postindex1 P_((char *, char *));
char *preindex P_((char *, char *, char *, char *));
char *bitfield P_((char *, char *, char *));
char *do_ops P_((char *, char *, char *, char *));
char *hexop P_((char *));

extern FILE *infile, *outfile;
extern int hidecnt;

int yylex (void);
