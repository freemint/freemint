/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000-2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

%{

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "syscalldefs.h"

/* from scanner */
int yylex (void);
extern int yylinecount;
extern char *yytext;

extern int errors;

#define OUT_OF_MEM(x) \
	if (!x) \
	{ yyerror("out of memory"); YYERROR; }


/* local things */

struct systab *gemdos = NULL;
struct systab *bios = NULL;
struct systab *xbios = NULL;

struct systab *systab = NULL;


/* exported stuff */

struct systab *gemdos_table(void) { return gemdos; }
struct systab *bios_table(void) { return bios; }
struct systab *xbios_table(void) { return xbios; }


/* prototypes */

static void yyerror(char *s);
static void insert_string(char *dst, const char *src);

static int resize_tab(struct systab *tab, int newsize);
static int add_tab(struct systab *tab, int nr, const char *name, struct arg *r, struct arg *p, int status);

static void add_arg(struct arg **head, struct arg *t);
static struct arg *make_arg(int type, const char *s);


/* defaults */

#define INITIAL_GEMDOS	100
#define INITIAL_BIOS	100
#define INITIAL_XBIOS	100

%}

%union {
	/* terminals */
	char	ident[STRMAX];
	long	value;

	/* nonterminals */
	struct systab *tab;
	struct arg *list;
}

%token	<ident>	_IDENT_GEMDOS
%token	<ident>	_IDENT_BIOS
%token	<ident>	_IDENT_XBIOS
%token	<ident>	_IDENT_MAX

%token	<ident>	_IDENT_VOID
%token	<ident>	_IDENT_CONST
%token	<ident>	_IDENT_STRUCT
%token	<ident>	_IDENT_UNION

%token	<ident>	_IDENT_CHAR
%token	<ident>	_IDENT_SHORT
%token	<ident>	_IDENT_LONG
%token	<ident>	_IDENT_UNSIGNED
%token	<ident>	_IDENT_UCHAR
%token	<ident>	_IDENT_USHORT
%token	<ident>	_IDENT_ULONG

%token	<ident>	_IDENT_UNDEFINED
%token	<ident>	_IDENT_UNSUPPORTED
%token	<ident>	_IDENT_UNIMPLEMENTED
%token	<ident>	_IDENT_PASSTHROUGH

%token	<ident>	Identifier
%token	<value>	Integer


%type	<tab>	gemdos
%type	<tab>	bios
%type	<tab>	xbios

%type	<list>	return_type
%type	<list>	parameter_list
%type	<list>	simple_parameter_list
%type	<list>	simple_parameter
%type	<list>	simple_type
%type	<list>	type
%type	<value>	status


%start syscalls

%%

syscalls
:	gemdos bios xbios
	{
		gemdos = $1;
		bios = $2;
		xbios = $3;
	}
;

gemdos
:	{
		systab = malloc(sizeof(*systab));
		OUT_OF_MEM(systab);

		bzero(systab, sizeof(*systab));

		systab->size = INITIAL_GEMDOS;
		systab->table = malloc(systab->size * sizeof(*(systab->table)));
		OUT_OF_MEM(systab->table);

		bzero(systab->table, systab->size * sizeof(*(systab->table)));
	}
	'[' _IDENT_GEMDOS ']' definition_list
	{
		$$ = systab;
	}
;

bios
:	{
		systab = malloc(sizeof(*systab));
		OUT_OF_MEM(systab);

		bzero(systab, sizeof(*systab));

		systab->size = INITIAL_BIOS;
		systab->table = malloc(systab->size * sizeof(*(systab->table)));
		OUT_OF_MEM(systab->table);

		bzero(systab->table, systab->size * sizeof(*(systab->table)));
	}
	'[' _IDENT_BIOS ']' definition_list
	{
		$$ = systab;
	}
;

xbios
:	{
		systab = malloc(sizeof(*systab));
		OUT_OF_MEM(systab);

		bzero(systab, sizeof(*systab));

		systab->size = INITIAL_XBIOS;
		systab->table = malloc(systab->size * sizeof(*(systab->table)));
		OUT_OF_MEM(systab->table);

		bzero(systab->table, systab->size * sizeof(*(systab->table)));
	}
	'[' _IDENT_XBIOS ']' definition_list
	{
		$$ = systab;
	}
;

definition_list
:	definition
	{ }
|	definition_list definition
	{ }
;

definition
:	Integer Identifier '(' parameter_list ')' status
	{
		if (systab->max && $1 >= systab->max)
		{ yyerror("entry greater than MAX"); YYERROR; }

		if (add_tab(systab, $1, $2, NULL, $4, $6))
		{ yyerror("out of memory"); YYERROR; }
	}
|	Integer return_type Identifier '(' parameter_list ')' status
	{
		if (systab->max && $1 >= systab->max)
		{ yyerror("entry greater than MAX"); YYERROR; }

		if (add_tab(systab, $1, $3, $2, $5, $7))
		{ yyerror("out of memory"); YYERROR; }
	}
|	Integer _IDENT_UNDEFINED
	{
		if (systab->max && $1 >= systab->max)
		{ yyerror("entry greater than MAX"); YYERROR; }

		if (add_tab(systab, $1, $2, NULL, NULL, SYSCALL_UNDEFINED))
		{ yyerror("out of memory"); YYERROR; }
	}
|	Integer _IDENT_PASSTHROUGH
	{
		if (systab->max && $1 >= systab->max)
		{ yyerror("entry greater than MAX"); YYERROR; }

		if (add_tab(systab, $1, $2, NULL, NULL, SYSCALL_PASSTHROUGH))
		{ yyerror("out of memory"); YYERROR; }
	}
|	Integer _IDENT_MAX
	{
		if (systab->max)
		{ yyerror("MAX already defined"); YYERROR; }

		systab->max = $1;

		if (systab->maxused && systab->maxused >= systab->max)
		{ yyerror("there are entries greater than MAX"); YYERROR; }

		if (!resize_tab(systab, systab->max))
		{ yyerror("out of memory"); YYERROR; }
	}
;

return_type
:
	type
	{
		$$ = $1;
	}
|	type '*'
	{
		struct arg *l = $1;

		l->flags |= FLAG_POINTER;

		$$ = l;
	}
;

parameter_list
:	_IDENT_VOID
	{
		$$ = NULL;
	}
|	simple_parameter_list
	{
		$$ = $1;
	}
;

simple_parameter_list
:	simple_parameter
	{
		struct arg *l = $1;

		$$ = l;
	}
|	simple_parameter_list ',' simple_parameter
	{
		struct arg *head = $1;
		struct arg *l = $3;

		add_arg(&(head->next), l);

		$$ = head;
	}
;

simple_parameter
:	simple_type Identifier
	{
		struct arg *l = $1;

		strcpy(l->name, $2);

		$$ = l;
	}
|	simple_type '*' Identifier
	{
		struct arg *l = $1;

		strcpy(l->name, $3);
		l->flags |= FLAG_POINTER;

		$$ = l;
	}
|	simple_type '*' '*' Identifier
	{
		struct arg *l = $1;

		strcpy(l->name, $4);
		l->flags |= FLAG_POINTER;
		l->flags |= FLAG_POINTER2;

		$$ = l;
	}
|	simple_type Identifier '[' ']'
	{
		struct arg *l = $1;

		strcpy(l->name, $2);
		l->flags |= FLAG_POINTER;

		$$ = l;
	}
|	simple_type Identifier '[' Integer ']'
	{
		struct arg *l = $1;

		strcpy(l->name, $2);
		l->flags |= FLAG_ARRAY;
		l->ar_size = $4;

		$$ = l;
	}
;

simple_type
:	type
	{
		struct arg *l = $1;

		$$ = l;
	}
|	_IDENT_CONST type
	{
		struct arg *l = $2;

		l->flags |= FLAG_CONST;
		// insert_string(l->types, $1);

		$$ = l;
	}
|	_IDENT_STRUCT type
	{
		struct arg *l = $2;

		l->flags |= FLAG_STRUCT;
		insert_string(l->types, $1);

		$$ = l;
	}
|	_IDENT_CONST _IDENT_STRUCT type
	{
		struct arg *l = $3;

		l->flags |= FLAG_CONST;
		l->flags |= FLAG_STRUCT;
		insert_string(l->types, $2);
		// insert_string(l->types, $1);

		$$ = l;
	}
|	_IDENT_UNION type
	{
		struct arg *l = $2;

		l->flags |= FLAG_UNION;
		insert_string(l->types, $1);

		$$ = l;
	}
;

type
:	_IDENT_CHAR
	{
		struct arg *l;

		l = make_arg(TYPE_CHAR, $1);
		OUT_OF_MEM(l);

		$$ = l;
	}
|	_IDENT_SHORT
	{
		struct arg *l;

		l = make_arg(TYPE_SHORT, $1);
		OUT_OF_MEM(l);

		$$ = l;
	}
|	_IDENT_LONG
	{
		struct arg *l;

		l = make_arg(TYPE_LONG, $1);
		OUT_OF_MEM(l);

		$$ = l;
	}
|	_IDENT_UNSIGNED
	{
		struct arg *l;

		l = make_arg(TYPE_UNSIGNED, $1);
		OUT_OF_MEM(l);

		$$ = l;
	}
|	_IDENT_UCHAR
	{
		struct arg *l;

		l = make_arg(TYPE_UCHAR, $1);
		OUT_OF_MEM(l);

		$$ = l;
	}
|	_IDENT_USHORT
	{
		struct arg *l;

		l = make_arg(TYPE_USHORT, $1);
		OUT_OF_MEM(l);

		$$ = l;
	}
|	_IDENT_ULONG
	{
		struct arg *l;

		l = make_arg(TYPE_ULONG, $1);
		OUT_OF_MEM(l);

		$$ = l;
	}
|	_IDENT_VOID
	{
		struct arg *l;

		l = make_arg(TYPE_VOID, $1);
		OUT_OF_MEM(l);

		$$ = l;
	}
|	Identifier
	{
		struct arg *l;

		l = make_arg(TYPE_IDENT, $1);
		OUT_OF_MEM(l);

		$$ = l;
	}
;

status
:	/* regular */
	{
		$$ = SYSCALL_REGULAR;
	}
|	_IDENT_UNSUPPORTED
	{
		$$ = SYSCALL_UNSUPPORTED;
	}
|	_IDENT_UNIMPLEMENTED
	{
		$$ = SYSCALL_UNIMPLEMENTED;
	}
|	_IDENT_PASSTHROUGH
	{
		$$ = SYSCALL_PASSTHROUGH;
	}
;

%%

void
yyerror(char *s)
{
	errors++;
	printf("line %i: %s", yylinecount, s);

	if (strstr(s, "parse error"))
		printf(" near token \"%s\"", yytext);

	printf("\n");
}

static void
insert_string(char *dst, const char *src)
{
	char save[STRMAX];
	int l;

	strcpy(save, dst);
	strcpy(dst, src);

	l = strlen(dst);
	dst[l++] = ' ';
	strcpy(dst+l, save);
}

static int
resize_tab(struct systab *tab, int newsize)
{
	if (newsize > tab->size)
	{
		struct syscall **newtable = malloc(newsize * sizeof(*newtable));

		if (newtable)
		{
			bzero(newtable, newsize * sizeof(*newtable));
			memcpy(newtable, tab->table, tab->size * sizeof(*(tab->table)));

			free(tab->table);
			tab->table = newtable;
			tab->size = newsize;

			return 1;
		}
	}
	else
	{
		tab->size = newsize;

		return 1;
	}

	return 0;
}

static int
add_tab(struct systab *tab, int nr, const char *name, struct arg *r, struct arg *p, int status)
{
	struct syscall *call = NULL;

	if (tab->size <= nr)
		if (!resize_tab(tab, nr + 100))
			return 1;

	if (name)
	{
		call = malloc(sizeof(*call));
		if (!call)
			return 1;

		bzero(call, sizeof(*call));

		strcpy(call->name, name);
		call->ret = r;
		call->args = p;
		call->status = status;
	}

	tab->table[nr] = call;

	if (tab->maxused < nr)
		tab->maxused = nr;

	return 0;
}

static void
add_arg(struct arg **head, struct arg *t)
{
	while (*head)
		head = &((*head)->next);

	t->next = NULL;
	*head = t;
}

static struct arg *
make_arg(int type, const char *s)
{
	struct arg *l = malloc(sizeof(*l));

	if (l)
	{
		bzero(l, sizeof(*l));

		if (s) strcpy(l->types, s);
		l->type = type;
	}

	return l;
}
