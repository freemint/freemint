
#include <mint/mintbind.h>
#include <sys/fcntl.h>
#include <string.h>

/* KM_RUN */
#include "../../../sys/mint/ioctl.h"

#define DEFAULT "xaaes.km"

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
		Cconws("XaAES loader: Fopen(\"/kern/sysdir\") failed!\r\n");
		goto error;
	}
	r = Fread((int)fh, sizeof(path), path);
	if (r <= 0)
	{
		Cconws("XaAES loader: Fread(\"/kern/sysdir\") failed!\r\n");
		goto error;
	}
	if (r >= sizeof(path))
	{
		Cconws("XaAES loader: buffer for Fread(\"/kern/sysdir\") to small!\r\n");
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
		name = DEFAULT;

	/* change to the XaAES module directory */
	r = Dsetpath(path);
	if (r)
	{
		Cconws("XaAES loader: No such directory: \"");
		Cconws(path);
		Cconws("\"\r\n");
		goto error;
	}

	/* get absolute path to this directory */
	r = Dgetpath(path, 0);
	if (r)
	{
		Cconws("XaAES loader: Dgetpath() failed???\r\n");
		goto error;
	}

	/* append module name */
	my_strlcat(path, "\\", sizeof(path));
	my_strlcat(path, name, sizeof(path));

	/* check if file exist */
	fh = Fopen(path, O_RDONLY);
	if (fh < 0)
	{
		Cconws("XaAES loader: No such file: \"");
		Cconws(path);
		Cconws("\"\r\n");
		goto error;
	}
	Fclose((int)fh);

	Cconws("Load kernel module: ");
	Cconws(path);
	Cconws("\r\n");

	fh = Fopen("/dev/km", O_RDONLY);
	if (fh < 0)
	{
		Cconws("XaAES loader: no /dev/km, please update your kernel!\r\n");
		goto error;
	}

	r = Fcntl((int)fh, path, KM_RUN);
	if (r)
	{
		Cconws("XaAES loader: Fcntl(KM_RUN) failed!\r\n");
		goto error;
	}

	Fclose((int)fh);
	return 0;

error:
	Cconws("press any key to continue ...\r\n");
	Cconin();
	return 1;
}
