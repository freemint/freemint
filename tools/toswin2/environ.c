
#include <string.h>
#include <support.h>

#include <cflib.h>

#include "global.h"
#include "environ.h"


static char TERMprefix[] = "TERM";
static char LINESprefix[] = "LINES";
static char COLSprefix[] = "COLUMNS";
static char HOMEprefix[] = "HOME";
static char SHELLprefix[] = "SHELL";
static char ARGVprefix[] = "ARGV";

/*
 * Aktuelles AES-Environment auslesen.
*/
static char *get_aesenv(long extra_bytes)
{
	long	len;
	char	*buf = NULL;
	
	len = shel_write(8, 0, 0, NULL, NULL);
	if (len > 0)
	{
		buf = malloc(len + extra_bytes);
		shel_write(8, 2, len, buf, NULL);
	}
	else
	{
		/* FIXME: copy env from basepge into buf! */
		buf = malloc(extra_bytes);
	}
	return buf;
}

/*
 * Sucht eine Variable in <env>.
*/
static char *get_var(char *env, char *var)
{
	char	*v, *s, *r;
	
	s = env;
	while (*s)
	{
		for (r = s, v = var; *s && *s++ == *v++; )
		{
			if (*s == '=' && (*v == '=' || *v == '\0'))
				return r;
		}
		while (*s++)
			;
	}
	return NULL;
}

/*
 * Kopiert von <s> nach <d>.
*/
static void copy_env(char *s, char *d)
{
	do {
		while ((*d++ = *s++) != '\0')
			;
	}
	while (*s);
	*d = '\0';
}

/*
 * Tr„gt <var> mit <value> in <env> ein. Ist <var> schon vorhanden, wird
 * der alte Eintrag gel”scht.
*/
static void put_env(char *env, char *var, char *value)
{
	char	*d, *s;
	char	new[80] = "";
	
	d = s = get_var(env, var);
	if (d != NULL)						/* Variable gefunden -> l”schen */
	{
		while (*s++)
			;
		if (*s)
			copy_env(s, d);
		else
			d[0] = d[1] = '\0';
	}
	
	d = env;
	while (*d)
		while(*d++)
			;
	if (strlen(var) > 0)
	{
		strcpy(new, var);
		strcat(new, "=");
	}
	strcat(new, value);
	s = new;
	while ((*d++ = *s++) != '\0')
		;
	*d = '\0';
}


#if 0

static void dump_env(char *env)
{
	char	*p;
	
	p = env;
	while (p[0] || p[1]) 
	{
		if (p[0] == '\0')
			printf("\n");
		else
			printf("%c", p[0]);
		p++;
	}
	printf("\n");
}

#endif

/* --------------------------------------------------------------------------- */
char *normal_env(int cols, int rows, int term)
{
	long	extra;
	char	*new, str[5];
	
	extra = strlen(TERMprefix) + 5 + 2 + 	/* 5 Zeichen plus '=' und '\0' */
			  strlen(LINESprefix) + 3 + 2 + 	/* 3 Stellen plus '=' und '\0' */
			  strlen(COLSprefix) + 3 + 2; 	/* 3 Stellen plus '=' und '\0' */

	/* Kopie vom AES */
	new = get_aesenv(extra);

	/* Eigene Vars eintragen */
	if (term == MODE_VT100)
		put_env(new, TERMprefix, "tw100");
	else
		put_env(new, TERMprefix, "tw52");

	_ltoa(rows, str, 10);
	put_env(new, LINESprefix, str);

	_ltoa(cols, str, 10);
	put_env(new, COLSprefix, str);
/*
dump_env(new);
*/	
	return new;
}

/* --------------------------------------------------------------------------- */
char *shell_env(int cols, int rows, int term, char *homedir, char *shell, bool login)
{
	long	extra;
	char	*new, str[5];
	
	extra = strlen(TERMprefix) + 5 + 2 + 	/* 5 Zeichen plus '=' und '\0' */
			  strlen(LINESprefix) + 3 + 2 + 	/* 3 Stellen plus '=' und '\0' */
			  strlen(COLSprefix) + 3 + 2 + 	/* 3 Stellen plus '=' und '\0' */
			  strlen(HOMEprefix) + strlen(homedir) + 2 +	/* plus '=' und '\0' */
			  strlen(SHELLprefix) + strlen(shell) + 2;	/* plus '=' und '\0' */
	if (login)
		extra += strlen(ARGVprefix) + 2 + 2;	/* '=' und '\0' und '-' und '\0' */

	/* Kopie vom AES */
	new = get_aesenv(extra);

	/* Eigene Vars eintragen */
	if (term == MODE_VT100)
		put_env(new, TERMprefix, "tw100");
	else
		put_env(new, TERMprefix, "tw52");

	_ltoa(rows, str, 10);
	put_env(new, LINESprefix, str);

	_ltoa(cols, str, 10);
	put_env(new, COLSprefix, str);

	put_env(new, HOMEprefix, homedir);
	put_env(new, SHELLprefix, shell);

	put_env(new, ARGVprefix, "");
	if (login)
		put_env(new, "", "-");
	else
		put_env(new, "", shell);
/*
dump_env(new);
*/	
	return new;
}

/* --------------------------------------------------------------------------- */
char *share_env(int cols, int rows, int term, char *prgname, char *arg, char *env)
{
	long	extra, len = 0;
	int	j, k;
	char	*new, str[256], *p;

	extra = strlen(TERMprefix) + 5 + 2 + 	/* 5 Zeichen plus '=' und '\0' */
			  strlen(LINESprefix) + 3 + 2 + 	/* 3 Stellen plus '=' und '\0' */
			  strlen(COLSprefix) + 3 + 2 + 	/* 3 Stellen plus '=' und '\0' */
			  strlen(ARGVprefix) + 2 +			/* '=' und '\0' */
			  strlen(prgname) + 1 +				/* argv[0] und '\0' */
			  strlen(arg);							/* die '\n' werden zu '\0'! */

	p = env;
	if (p != NULL && *p)
	{
		while (*p)
		{
			while (*p++)
				;
		}
		len = p - env;
	}

	new = malloc(len + extra);
	copy_env(env, new);
	
	/* Eigene Vars eintragen */
	if (term == MODE_VT100)
		put_env(new, TERMprefix, "tw100");
	else
		put_env(new, TERMprefix, "tw52");

	_ltoa(rows, str, 10);
	put_env(new, LINESprefix, str);

	_ltoa(cols, str, 10);
	put_env(new, COLSprefix, str);

	len = strlen(arg);
	if (len > 124)
	{
		put_env(new, ARGVprefix, "");
		put_env(new, "", prgname);

		k = 0;
		while (k < len)
		{
			j = 0;
			while ((k < len) && (arg[k] != '\n'))
			{
				str[j] = arg[k];
				k++;
				j++;
			}
			str[j] = '\0';
			put_env(new, "", str);
			k++;
		}
		/* arg wurde in ARGV bernommen */	
		arg[0] = '\0';
	}
	else
	{
		for (k = 0; k < len; k++)
			if (arg[k] == '\n')
				arg[k] = ' ';
	}

/*
dump_env(new);
*/
	return new;
}
