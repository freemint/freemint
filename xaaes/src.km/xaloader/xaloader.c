
#include <mint/cookie.h>
#include <mint/mintbind.h>
#include <mint/ssystem.h>
#include <signal.h>
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

static void ignore(long sig)
{
	char sign[4];
	if( sig > 29 )
	{
		sign[0] = '3';
		sig -= 30;
	}
	else if( sig > 19 )
	{
		sign[0] = '2';
		sig -= 20;
	}
	else if( sig > 9 )
	{
		sign[0] = '1';
		sig -= 10;
	}
	else
		sign[0] = ' ';
	sign[1] = sig + '0';
	sign[2] = 0;
	Cconws( "XaAES loader: SIGNAL ");
	Cconws( sign );
	Cconws( " ignored\r\n");
}

int
loader_init(int argc, char **argv, char **env)
{
	char path[384];
	char *name;
	long fh, r = 1;

	/*
	 *  Go into MiNT domain
	 */
	(void)Pdomain(1);

	Cconws("XaAES loader starting...\r\n");

	/*
	 * first make sure we are on FreeMiNT's '/'
	 */
	Dsetdrv('u' - 'a');
	Dsetpath("/");

	/*
	 * now lookup FreeMiNT's sysdir
	 */
	fh = Fopen("u:/kern/sysdir", O_RDONLY);
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
	/* if mint waits for xaloader it should not exit */
	Psignal( SIGINT, ignore );
	Psignal( SIGQUIT, ignore );
	Psignal( SIGSTOP, ignore );
	Psignal( SIGTSTP, ignore );
	Psignal( SIGTERM, ignore );
	//Psignal( SIGKILL, ignore );

	//Cconws( "XaAES loader: KM_RUN \r\n");
	//Cconin();
	r = Fcntl((int)fh, path, KM_RUN);
	//Cconws( "XaAES loader: KM_RUN done()\r\n");

	if (r)
	{
		Cconws("XaAES loader: Fcntl(KM_RUN) failed!\r\n");
	}

	Cconws( "XaAES loader: KM_FREE\r\n");
	//Cconin();

	r = Fcntl((int)fh, path, KM_FREE);
	if( r )
	{
		char *p;
		for( p = path; *p; p++ );
		for( --p; p > path && !(*p == '/' || *p == '\\'); p-- );
		if( p > path )
		{
			p++;
			Cconws( "XaAES loader: KM_FREE failed trying: '");
			Cconws( p );
			Cconws( "'..\r\n");
			r = Fcntl((int)fh, p, KM_FREE);
		}
		if( r )
		{
			char e[2];
			if( r < 0 )
				r = -r;
			e[0] = r + '0';
			e[1] = 0;
			Cconws( "\r\nXaAES loader: KM_FREE failed: ");
			Cconws( e );
			Cconws( "\r\n");
			//Cconin();
		}
	}
	//Cconws( "XaAES loader: Fclose()\r\n");
	Fclose((int)fh);
	//Cconws( "XaAES loader: return\r\n");
	//Cconin();
	return r;

error:
	Cconws("press any key to continue ...\r\n");
	Cconin();
	return r;
}
