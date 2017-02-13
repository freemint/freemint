#include <mint/cookie.h>
#include <mint/basepage.h>
#include <mint/mintbind.h>
#include <mint/ssystem.h>
#include <sys/fcntl.h>
#include <string.h>

#include "../../sys/buildinfo/version.h"
#ifndef str
#define str(x) _stringify(x)
#define _stringify(x)  #x
#endif

/* cookie jar definition
*/
typedef struct
{
		long id;
		long value;
} COOKIE;

#define CJAR    (*(struct cookie *)(0x5A0))

#define MINTDIR        MINT_VERS_PATH_STRING
#define DEFAULT        "mint.prg"
#define DEFAULT_68000  "mint000.prg"
#define DEFAULT_68020  "mint020.prg"
#define DEFAULT_68030  "mint030.prg"
#define DEFAULT_68040  "mint040.prg"
#define DEFAULT_68060  "mint060.prg"
#define DEFAULT_MILAN  "mintmil.prg"
#define DEFAULT_ARANYM "mintara.prg"

static long
get_cookie (long id, unsigned long *ret)
{
		void *oldssp;
		COOKIE *cjar;

		oldssp = (void*)(Super(1L) ? NULL : (void*)Super(0L));

		cjar = *(COOKIE**)(0x5A0);

		if(oldssp)
				Super( oldssp );

		while (cjar->id)
		{
				if (cjar->id == id)
				{
						if (ret)
						{
								*ret = cjar->value;
								return 1;
						}
						else
						{
								return cjar->value;
						}
				}

				cjar++;
		}

		return 0;
}


static void
my_strlcpy(char *dst, const char *src, size_t n)
{
		size_t org_n = n;

		if (n && --n)
		{
				do {
						if (!(*dst++ = *src++))
								break;
				}
				while (--n);

		}

		if (org_n && n == 0)
				*dst = '\0';
}

static void
my_strlcat(char *dst, const char *src, size_t n)
{
		if (!n)
				return;

		/* find end */
		while (n-- && *dst)
				dst++;

		/* copy over */
		if (n && --n)
		{
				do {
						if (!(*dst++ = *src++))
								break;
				}
				while (--n);

		}

		/* terminate */
		if (n == 0)
				*dst = '\0';
}

int loader_init(int argc, char **argv, char **env);

int
loader_init(int argc, char **argv, char **env)
{
		unsigned long cpu, mch;
		char path[384];
		char temp[384];
		char *name;
		long fh, r;

		(void)Cconws("FreeMiNT loader starting...\r\n");

		my_strlcpy(path, MINTDIR, sizeof(path));


		/* if the system have a 68000 CPU we use the 68000 compiled
		* kernel by default
		*/
		r = get_cookie(C__CPU, &cpu);
		if (r && cpu < 20)
				name = DEFAULT_68000;
		else
				name = DEFAULT;

		my_strlcpy(temp, path, sizeof(path));
		my_strlcat(temp, "\\", sizeof(temp));
		my_strlcat(temp, name, sizeof(temp));

		fh = Fopen(temp, O_RDONLY);
		if (fh < 0)
		{
				/* default kernel not available, select kernel based on CPU type */
				if(get_cookie(C__CPU, &cpu))
				{
						switch(cpu)
						{
								case 20:        name = DEFAULT_68020;   break;
								case 30:        name = DEFAULT_68030;   break;
								case 40:        name = DEFAULT_68040;   break;
								case 60:        name = DEFAULT_68060;   break;

								default:        break;
						}
				}

				/* special treatment for MILAN and ARANYM */
				if(get_cookie(C__MCH, &mch))
				{
						switch(mch>>16)
						{
								case 4: name = DEFAULT_MILAN;   break;
								case 5: name = DEFAULT_ARANYM;  break;
						}
				}
		}

		Fclose((int)fh);

		/* append kernel name */
		my_strlcat(path, "\\", sizeof(path));
		my_strlcat(path, name, sizeof(path));

		/* check if file exist */
		fh = Fopen(path, O_RDONLY);
		if (fh < 0)
		{
				(void)Cconws("FreeMiNT loader: No such file: \"");
				(void)Cconws(path);
				(void)Cconws("\"\r\n");
				goto error;
		}
		Fclose((int)fh);

		(void)Cconws("Load kernel: ");
		(void)Cconws(path);
		(void)Cconws("\r\n");

		/*
		* Let our DTA point to our parent DTA; the kernel use
		* information from the DTA when processing the auto folder
		*/

		if(_base->p_parent)
				Fsetdta((_DTA*) _base->p_parent->p_dta);

		temp[0] = 0;
		temp[1] = '\0';
		if(Pexec(PE_LOADGO, path, temp, NULL))
		{
				(void)Cconws("FreeMiNT loader: Pexec() failed!\r\n");
				goto error;
		}
		return 0;

error:
		(void)Cconws("press any key to continue ...\r\n");
		Cconin();
		return 1;
}
