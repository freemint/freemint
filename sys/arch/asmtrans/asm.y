
%{
#define YYSTYPE char *

#include "asmtrans.h"
%}

%token WORD
%token WHITESP
%token EOLN
%token STRING
%token DEFINECMD
%token INCLUDECMD
%token IFDEFCMD
%token IFNDEFCMD
%token ELSECMD
%token ENDIFCMD

/* Grammar follows */
%%
input:
        | input line
;

line:	EOLN		{ emit($1); }
		/* note that emit() will automatically free the memory,
		   and will also check the hidecnt variable */
	| label EOLN	{ emit($1); emit($2); }
	| opline EOLN	{ emit($1); emit($2); }
	| label opline EOLN	{ emit($1); emit($2);
			emit($3); }
	| INCLUDECMD WHITESP STRING EOLN { if (!hidecnt) do_include($3); free($3); }
	| INCLUDECMD WHITESP WORD EOLN { if (!hidecnt) do_include($3); free($3); }
	| DEFINECMD WHITESP WORD WHITESP STRING EOLN {
		if (!hidecnt) do_define($3, $5); free($3); free($5); }
	| DEFINECMD WHITESP WORD WHITESP operand EOLN {
		if (!hidecnt) do_define($3, $5); free($3); free($5); }
	| IFDEFCMD WHITESP WORD EOLN { do_ifdef($3); free($3); }
	| IFNDEFCMD WHITESP WORD EOLN { do_ifndef($3); free($3); }
	| ELSECMD EOLN { do_else(); }
	| ENDIFCMD EOLN { do_endif(); } 
;

opline:	WHITESP opcode		{ $$ = do_ops("", $2, "", ""); free($2); }
	| WHITESP opcode WHITESP ops
			{ $$ = do_ops("", $2, $3, $4);
			 free($2); free($3); free($4); }
	| WORD WHITESP opcode	{ $$ = do_ops($1, $3, "", ""); free($1); free($3); }
	| WORD WHITESP opcode WHITESP ops
			{ $$ = do_ops($1, $3, $4, $5);
			 free($1); free($3); free($4); free($5);}
;

ops: operand { $$ = $1; }
	| operand ',' ops { $$ = concat3($1, ",", $3);
				free($1); free($3); }
;

opcode: WORD	{ $$ = wordlookup($1); free($1); }
;

label: WORD ':' { $$ = concat($1, ":"); free($1); }

operand: basic	{$$ = $1; }
	| '#' basic {$$ = immediate($2); free($2); }
	| '(' basic ')' {$$ = indirect($2); free($2); }
	| '(' basic ')' '+' {$$ = postinc($2); free($2); }
	| '-' '(' basic ')' {$$ = predec($3); free($3); }
	| basic '(' basic ')' {$$ = indexed($1, $3); free($1); free($3); }
	| '(' basic ')' WORD {$$ = sizedop($2, $4); free($2); free($4); }
	| basic '(' basic ',' basic ')' {$$ = twoindex($1, $3, $5);
				free($1); free($3); free($5); }
	| basic '{' basic ':' basic '}' {$$ = bitfield($1, $3, $5);
			free($1); free($3); free($5); }
	| '(' '[' basic ',' basic ']' ',' basic ',' basic ')'
	   { $$=postindex($3,$5,$8,$10); 
	     free($3); free($5); free($8); free($10); }		
	| '(' '[' basic ',' basic ',' basic ']' ',' basic ')'
	   { $$=preindex($3,$5,$7,$10); 
	     free($3); free($5); free($7); free($10); }		
	| '(' '[' basic ']' ')'
	   { $$=postindex0($3); 
	     free($3); }
	| '(' '[' basic ']' ','  basic ')'
	   { $$=postindex1($3,$6); 
	     free($3); free($6); }		     		     
;

basic: basexpr { $$ = $1; }
	| basexpr op basic { $$ = concat3($1, $2, $3); free($1); free($2); free($3); }
	| '-' basic { $$ = concat("-", $2); free($2); }

basexpr: WORD {$$ = wordlookup($1); free($1); }
	| '$' WORD {$$ = hexop($2); free($2);}
;

op: 	'+' { $$ = strdup("+"); }
	| '-' { $$ = strdup("-"); }
	| '*' { $$ = strdup("*"); }
	| '/' { $$ = strdup("/"); }
;
%%
#include <setjmp.h>

jmp_buf start;

#ifdef __MINT__
long _stksize = 64 * 1024;
#endif

static void
usage(void)
{
	fprintf(stderr, "Usage: asmtrans [-gas][-asm][-o outfile] infile\n");
	exit(2);
}

int errors = 0;

void
do_include(char *file)
{
	jmp_buf save;
	FILE *oldin, *f;

	f = fopen(file, "rt");
	if (!f) {
		perror(file);
		return;
	}
	bcopy(start, save, sizeof(jmp_buf));
	oldin = infile;
	infile = f;
	setjmp(start);
	yyparse();
	fclose(f);
	infile = oldin;
	bcopy(save, start, sizeof(jmp_buf));
	longjmp(start,1);
}

/* set up initial definitions based on syntax type */

void
do_initial_defs(void)
{
	if (syntax == GAS) {
		do_define("mmusr", "psr");
		do_define("fpiar", "fpi");
		do_define("XREF", ".globl");
		do_define("XDEF", ".globl");
		do_define("TEXT", ".text");
		do_define("DATA", ".data");
	/* gas doesn't have a .bss directive */
		do_define("BSS", ".data");
		do_define("END", "| END");
		do_define("dc.l", ".long");
		do_define("dc.w", ".word");
		do_define("dc.b", ".byte");
	} else if (syntax == ASM) {
		do_define("TEXT", "SECTION TEXT,CODE");
		do_define("DATA", "SECTION DATA,DATA");
		do_define("BSS", "SECTION BSS,BSS");
	}
}

int
main (int argc, char **argv)
{
	argv++;
	outfile = stdout;

	while (*argv) {
		if (!strcmp(*argv, "-o")) {
			FILE *f;
			
			argv++;
			if (*argv == 0) {
				fprintf(stderr, "missing argument to -o\n");
				usage();
			}
			f = fopen(*argv, "wt");
			if (!f)
				perror(*argv);
			else
				outfile = f;
			argv++;
		} else if (!strcmp(*argv, "-gas")) {
			argv++;
			syntax = GAS;
		} else if (!strcmp(*argv, "-asm")) {
			argv++;
			syntax = ASM;
		} else if (!strcmp(*argv, "-purec")) {
			argv++;
			syntax = PUREC;
		} else if (!strncmp(*argv, "-D", 2)) {
			char *word, *defn;
			word = *argv+2;
			defn = index(word,'=');
			if (defn)
				*defn++ = '\0';
			else
				defn = "1";
			if (*word) do_define(word,defn);
			argv++;
		} else if (!strcmp(*argv, "--")) {
			argv++;
			break;
		} else {
			if (**argv == '-') {
				fprintf(stderr, "unknown option: %s\n",
					*argv);
				usage();
			}
			break;
		}
	}

	do_initial_defs();

	if (*argv == 0) {
		setjmp(start);
		infile = stdin;
		yyparse();
	} else {
	    while(*argv) {
		FILE *f;
		if (!(f = fopen(*argv, "rt")))
			perror(*argv);
		else {
			infile = f;
			setjmp(start);
			yyparse();
			fclose(f);
		}
		argv++;
	    }
	}

	if (ifstkptr != 0) {
		fputs("%ifdef without matching %endif\n", stderr);
		errors++;
	}
	return errors;
}

void
yyerror (char *s)  /* Called by yyparse on error */
             
{
	errors++;
	printf("%s\n", s);
	longjmp(start, 1);
}
