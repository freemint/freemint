#define YYSTYPE char *
#include "asmtrans.h"
#include "asmtab.h"

#define QUOTESYM '"'
#define CMDSYM '%'

FILE *infile, *outfile;
extern YYSTYPE yylval;

#define MAXNEST 10
int hidecnt = 0;
int ifstack[MAXNEST], ifstkptr;

void emit(char *s)
{
	if (hidecnt == 0)
		fputs(s, outfile);
	free(s);
}

char *concat(char *s1, char *s2)
{
	size_t siz = strlen(s1) + strlen(s2) + 1;
	char *r;

	r = malloc(siz);
	if (!r) return 0;

	strcpy(r, s1);
	strcat(r, s2);
	return r;
}

char *concat3(char *s1, char *s2, char *s3)
{
	size_t siz = strlen(s1) + strlen(s2) + strlen(s3) + 1;
	char *r;

	r = malloc(siz);
	if (!r) return 0;

	strcpy(r, s1);
	strcat(r, s2);
	strcat(r, s3);
	return r;
}

char *concat4(char *s1, char *s2, char *s3, char *s4)
{
	size_t siz = strlen(s1) + strlen(s2) + strlen(s3) + strlen(s4) + 1;
	char *r;

	r = malloc(siz);
	if (!r) return 0;

	strcpy(r, s1);
	strcat(r, s2);
	strcat(r, s3);
	strcat(r, s4);
	return r;
}

char *concat5(char *s1, char *s2, char *s3, char *s4, char *s5)
{
	size_t siz = strlen(s1) + strlen(s2) + strlen(s3)
			+ strlen(s4) + strlen(s5) + 1;
	char *r;

	r = malloc(siz);
	if (!r) return 0;

	strcpy(r, s1);
	strcat(r, s2);
	strcat(r, s3);
	strcat(r, s4);
	strcat(r, s5);
	return r;
}

char *concat6(char *s1, char *s2, char *s3, char *s4, char *s5, char *s6)
{
	size_t siz = strlen(s1) + strlen(s2) + strlen(s3)
			+ strlen(s4) + strlen(s5) + strlen(s6) + 1;
	char *r;

	r = malloc(siz);
	if (!r) return 0;

	strcpy(r, s1);
	strcat(r, s2);
	strcat(r, s3);
	strcat(r, s4);
	strcat(r, s5);
	strcat(r, s6);
	return r;
}

char *concat8(char *s1, char *s2, char *s3, char *s4, char *s5, char *s6, char *s7, char *s8)
{
	size_t siz = strlen(s1) + strlen(s2) + strlen(s3)
			+ strlen(s4) + strlen(s5) + strlen(s6) 
			+ strlen(s7) + strlen(s8) + 1;
	char *r;

	r = malloc(siz);
	if (!r) return 0;

	strcpy(r, s1);
	strcat(r, s2);
	strcat(r, s3);
	strcat(r, s4);
	strcat(r, s5);
	strcat(r, s6);
	strcat(r, s7);
	strcat(r, s8);
	return r;
}

char *concat9(char *s1, char *s2, char *s3, char *s4, char *s5, char *s6, char *s7, char *s8, char *s9)
{
	size_t siz = strlen(s1) + strlen(s2) + strlen(s3)
			+ strlen(s4) + strlen(s5) + strlen(s6) 
			+ strlen(s7) + strlen(s8) + strlen(s9) + 1;
	char *r;

	r = malloc(siz);
	if (!r) return 0;

	strcpy(r, s1);
	strcat(r, s2);
	strcat(r, s3);
	strcat(r, s4);
	strcat(r, s5);
	strcat(r, s6);
	strcat(r, s7);
	strcat(r, s8);
	strcat(r, s9);
	return r;
}


static int is_word_sym(int c)
{
	if (c == '.' || (c >= '0' && c <= '9')) return 1;
	if (c >= 'a' && c <= 'z') return 1;
	if (c >= 'A' && c <= 'Z') return 1;
	if (c == '_') return 1;
	return 0;
}

typedef struct wordtab {
	char *word;
	char *defn;
	struct wordtab *next;
} WORDTAB;

WORDTAB *globaltab;

void
do_define(char *word, char *defn)
{
	WORDTAB *w;

	w = malloc(sizeof(WORDTAB));
	if (!w) return;
	w->word = strdup(word);
	w->defn = strdup(defn);
	w->next = globaltab;
	globaltab = w;
}

/*
 * if we were actually using this program for
 * large things, we would use a hash table
 * to speed up the table lookup; but for the
 * uses we have, there aren't likely to be
 * many defines
 */

char *
wordlookup(char *which)
{
	WORDTAB *w;

	for (w = globaltab; w; w = w->next) {
		if (!strcmp(which, w->word)) {
			return strdup(w->defn);
		}
	}
	return strdup(which);
}

void
do_ifdef(char *which)
{
	int output = 0;
	WORDTAB *w;

	for (w = globaltab; w; w = w->next) {
		if (!strcmp(which, w->word)) {
			output = 1;
			break;
		}
	}
	if (ifstkptr < MAXNEST) {
		ifstack[ifstkptr++] = output;
		if (!output)
			hidecnt++;
	} else {
		ifstkptr++;
		yyerror("too many levels of %ifdef");
	}
}

void
do_ifndef(char *which)
{
	int output = 1;
	WORDTAB *w;

	for (w = globaltab; w; w = w->next) {
		if (!strcmp(which, w->word)) {
			output = 0;
			break;
		}
	}
	if (ifstkptr < MAXNEST) {
		ifstack[ifstkptr++] = output;
		if (!output)
			hidecnt++;
	} else {
		ifstkptr++;
		yyerror("too many levels of %ifdef");
	}
}

void
do_else(void)
{
	int output;

	if (ifstkptr == 0) {
		yyerror("%else without %ifdef");
	} else {
		if (ifstkptr > MAXNEST) return;
	/* if we were outputting, stop */
	/* otherwise, start again */
		output = !ifstack[ifstkptr-1];
		if (output)
			hidecnt--;
		else
			hidecnt++;
		ifstack[ifstkptr-1] = output;
	}
}

void
do_endif(void)
{
	int output;

	if (ifstkptr == 0) {
		yyerror("%endif without %ifdef");
	} else {
		ifstkptr--;
		if (ifstkptr >= MAXNEST) return;

		output = ifstack[ifstkptr];
		if (!output)
			hidecnt--;
	}
}

/* this is a terrible hack to remove the leading
 * '_' from labels...
 */

char *
fixupword(char *w)
{
	if (*w == '_' && syntax == PUREC)
		return strdup(w+1);
	else
		return strdup(w);
}


static char footext[1024];

int
yylex(void)
{
	int c;
	char *to = footext;
	int cmdword;
	static int saweoln = 1;

	c = getc(infile);

	if (c < 0) {
doeof:
		saweoln = 1;
		return 0;
	}
	if (c == ';') {
docomment:
		if (syntax == GAS)
			c = '|';
		*to++ = c;
		do {
			c = getc(infile);
			if (c < 0) return 0;
			if (c != '\r')
				*to++ = c;
		} while (c != '\n');
		*to = 0;
		yylval = strdup(footext);
		saweoln = 1;
		return EOLN;
	}
	if (c == '\n') {
doeoln:
		*to++ = c;
		*to = 0;
		yylval = strdup(footext);
		saweoln = 1;
		return EOLN;
	}
	if (c == CMDSYM && saweoln) {
		cmdword = 1;
		c = getc(infile);
	} else {
		cmdword = 0;
	}

	if (c == ' ' || c == '\t' || c == '\r') {
		do {
			if (c == '\r')
				c = ' ';
			*to++ = c;
			c = getc(infile);
		} while (c == ' ' || c == '\t');
		if (c == '\n') goto doeoln;
		if (c == ';') goto docomment;
		if (!cmdword) {
			ungetc(c, infile);
			*to = 0;
			yylval = strdup(footext);
			return WHITESP;
		} else {
			to = footext;
		}
	}

	saweoln = 0;

	if (c == QUOTESYM) {
		for(;;) {
			c = getc(infile);
			if (c < 0) goto doeof;
			if (c == QUOTESYM) break;
			*to++ = c;
		}
		*to = 0;
		yylval = strdup(footext);
		return STRING;
	}
	if (is_word_sym(c)) {
		do {
			*to++ = c;
			c = getc(infile);
		} while (is_word_sym(c));
		ungetc(c, infile);
		*to = 0;
		if (cmdword) {
			yylval = footext;
			if (!strcmp(footext, "define")) {
				return DEFINECMD;
			} else if (!strcmp(footext, "include")) {
				return INCLUDECMD;
			} else if (!strcmp(footext, "ifdef")) {
				return IFDEFCMD;
			} else if (!strcmp(footext, "ifndef")) {
				return IFNDEFCMD;
			} else if (!strcmp(footext, "else")) {
				return ELSECMD;
			} else if (!strcmp(footext, "endif")) {
				return ENDIFCMD;
			} else {
				fprintf(stderr, "Unknown command: %s\n", footext);
			}
		}
		yylval = fixupword(footext);
		return WORD;
	}

	*to++ = c;
	*to = 0;
	yylval = footext;
	return c;
}
