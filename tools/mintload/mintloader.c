#include <mint/arch/nf_ops.h>
#include <mint/basepage.h>
#include <mint/cookie.h>
#include <mint/mintbind.h>
#include <mint/ssystem.h>
#include <string.h>
#include <sys/fcntl.h>

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

#define MINTDIR         "\\mint\\"MINT_VERS_PATH_STRING
#define DEFAULT         "mint.prg"
#define DEFAULT_68000   "mint000.prg"
#define DEFAULT_68000_HATARI "mint000h.prg"
#define DEFAULT_68020   "mint020.prg"
#define DEFAULT_68030   "mint030.prg"
#define DEFAULT_68030_HATARI "mint030h.prg"
#define DEFAULT_68040   "mint040.prg"
#define DEFAULT_68060   "mint060.prg"
#define DEFAULT_MILAN   "mintmil.prg"
#define DEFAULT_ARANYM  "mintara.prg"
#define DEFAULT_FIREBEE "mintv4e.prg"

#ifndef __mcoldfire__
static long
get_cookie (long id, unsigned long *ret)
{
	void *oldssp;
	COOKIE *cjar;

	oldssp = (void*)(Super(1L) ? NULL : (void*)Super(0L));

	cjar = *(COOKIE**)(0x5A0);

	if (oldssp)
		Super (oldssp);

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

/*static*/ const char *
nf_get_name(char *buf, size_t bufsize)
{
	const char *ret = NULL;
	struct nf_ops *nf_ops;

	if ((nf_ops = nf_init()) != NULL)
	{
		long nfid_name = NF_GET_ID(nf_ops, NF_ID_NAME);

		if (nfid_name)
		{
			if (nf_ops->call(nfid_name | 0, (unsigned long)buf, (unsigned long)bufsize) != 0)
			{
				ret = buf;
			}
		}
	}

	return ret;
}

static int
is_hatari(void)
{
	char name[6+1];
	return nf_get_name(name, sizeof(name)) != NULL
		&& name[0] == 'H'
		&& name[1] == 'a'
		&& name[2] == 't'
		&& name[3] == 'a'
		&& name[4] == 'r'
		&& name[5] == 'i';
}
#endif


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
	char path[384];
	char temp[384];
	char *name;
#ifndef __mcoldfire__
	unsigned long cpu;
	unsigned long mch;
	int hatari = is_hatari();
#endif
	long fh;

	(void)Cconws("\033vFreeMiNT loader starting...\r\n");

	my_strlcpy(path, MINTDIR, sizeof(path));
	my_strlcat(path, "\\", sizeof(path));

	my_strlcpy(temp, path, sizeof(path));

#ifdef __mcoldfire__
	name = DEFAULT_FIREBEE;
#else
	if (get_cookie(C__CPU, &cpu))
	{
		switch(cpu)
		{
			case 20:
				name = DEFAULT_68020;
				break;
			case 30:
				name = hatari ? DEFAULT_68030_HATARI : DEFAULT_68030;
				break;
			case 40:
				name = DEFAULT_68040;
				break;
			case 60:
				name = DEFAULT_68060;
				break;

			default:
				name = hatari ? DEFAULT_68000_HATARI : DEFAULT_68000;
				break;
		}
	}
	else
	{
		name = hatari ? DEFAULT_68000_HATARI : DEFAULT_68000;
	}

	/* special treatment for MILAN and ARANYM */
	if (get_cookie(C__MCH, &mch))
	{
		switch(mch>>16)
		{
			case 4:
				name = DEFAULT_MILAN;
				break;
			case 5:
				name = DEFAULT_ARANYM;
				break;
		}
	}
#endif

	my_strlcat(temp, name, sizeof(temp));

	fh = Fopen(temp, O_RDONLY);
	if (fh < 0)
	{
		/* kernel based on CPU type not available, select default kernel */
		name = DEFAULT;
	}
	else
	{
		Fclose((int)fh);
	}

	/* append kernel name */
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

	if (_base->p_parent)
		Fsetdta((_DTA*) _base->p_parent->p_dta);

	temp[0] = 0;
	temp[1] = '\0';
	if (Pexec(PE_LOADGO, path, temp, NULL))
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
