
#include <mint/cookie.h>
#include <mint/mintbind.h>
#include <mint/ssystem.h>
#include <sys/fcntl.h>
#include <string.h>

/* KM_RUN */
#include "../../../sys/mint/ioctl.h"

#define DEFAULT        "xaaes.km"
#define DEFAULT_68000  "xaaes000.km"

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

/*
 * - without an argument try to load xaaes.km from sysdir
 * - with argument:
 *   - without a path separator try to load this from sysdir
 *   - with path separator go to this dir and load the module
 *     from there
 */

int loader_init(int argc, char **argv, char **env);

int
loader_init(int argc, char **argv, char **env)
{
	char path[384];
	char *name;
	long fh, r;

	/*
	 *  Go into MiNT domain
	 */
	(void)Pdomain(1);

	(void)Cconws("XaAES loader starting...\r\n");

	/*
	 * first make sure we are on FreeMiNT's '/'
	 */
	Dsetdrv('u' - 'a');
	Dsetpath("/");

	/*
	 * now lookup FreeMiNT's sysdir
	 */
	fh = Fopen("/kern/sysdir", O_RDONLY);
	if (fh < 0)
	{
		(void)Cconws("XaAES loader: Fopen(\"/kern/sysdir\") failed!\r\n");
		goto error;
	}
	r = Fread((int)fh, sizeof(path), path);
	if (r <= 0)
	{
		(void)Cconws("XaAES loader: Fread(\"/kern/sysdir\") failed!\r\n");
		goto error;
	}
	if (r >= sizeof(path))
	{
		(void)Cconws("XaAES loader: buffer for Fread(\"/kern/sysdir\") to small!\r\n");
		goto error;
	}
	Fclose((int)fh);

	/* null terminate */
	path[r] = '\0';

	/* append XaAES subdir */
	my_strlcat(path, "xaaes\\", sizeof(path));

	if (argc > 1)
	{
		char *s = argv[1];
		char c;

		name = NULL;

		do {
			c = *s++;
			if (c == '\\' || c == '/')
				name = s - 1;
		}
		while (c);

		if (name)
		{
			*name++ = '\0';

			my_strlcpy(path, argv[1], sizeof(path));
			my_strlcat(path, "/", sizeof(path));
		}
		else
			name = argv[1];
	}
	else
	{
		long cpu;

		name = DEFAULT;

		/* if the system have a 68000 CPU we use the 68000 compiled
		 * module
		 */
		r = Ssystem(S_GETCOOKIE, C__CPU, &cpu);
		if (r == 0 && cpu < 20)
			name = DEFAULT_68000;
	}

	/* change to the XaAES module directory */
	r = Dsetpath(path);
	if (r)
	{
		(void)Cconws("XaAES loader: No such directory: \"");
		(void)Cconws(path);
		(void)Cconws("\"\r\n");
		goto error;
	}

	/* get absolute path to this directory */
	r = Dgetpath(path, 0);
	if (r)
	{
		(void)Cconws("XaAES loader: Dgetpath() failed???\r\n");
		goto error;
	}

	/* append module name */
	my_strlcat(path, "\\", sizeof(path));
	my_strlcat(path, name, sizeof(path));

	/* check if file exist */
	fh = Fopen(path, O_RDONLY);
	if (fh < 0)
	{
		(void)Cconws("XaAES loader: No such file: \"");
		(void)Cconws(path);
		(void)Cconws("\"\r\n");
		goto error;
	}
	Fclose((int)fh);

	(void)Cconws("Load kernel module: ");
	(void)Cconws(path);
	(void)Cconws("\r\n");

	fh = Fopen("/dev/km", O_RDONLY);
	if (fh < 0)
	{
		(void)Cconws("XaAES loader: no /dev/km, please update your kernel!\r\n");
		goto error;
	}

	r = Fcntl((int)fh, path, KM_RUN);
	if (r)
	{
		(void)Cconws("XaAES loader: Fcntl(KM_RUN) failed!\r\n");
		goto error;
	}

	Fclose((int)fh);
	return 0;

error:
	(void)Cconws("press any key to continue ...\r\n");
	Cconin();
	return 1;
}
