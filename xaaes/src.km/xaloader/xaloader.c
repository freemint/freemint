
#include <mint/cookie.h>
#include <mint/mintbind.h>
#include <mint/ssystem.h>
#include <signal.h>
#include <sys/fcntl.h>
#include <string.h>

#define ConsoleWrite (void)Cconws

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
	ConsoleWrite( "XaAES loader: SIGNAL ");
	ConsoleWrite( sign );
	ConsoleWrite( " ignored\r\n");
}

static void d2a( int r, char e[] )
{
	int i = 0;
	if( r < 10 )
		e[i++] = r + '0';
	else
	{
		e[i++] = r / 10 + '0';
		e[i++] = r % 10 + '0';
	}
	e[i++] = '\r';
	e[i++] = '\n';
	e[i] = 0;
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
	ConsoleWrite("XaAES loader starting...\r\n");

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
		ConsoleWrite("XaAES loader: Fopen(\"/kern/sysdir\") failed!\r\n");
		goto error;
	}
	r = Fread((int)fh, sizeof(path), path);
	if (r <= 0)
	{
		ConsoleWrite("XaAES loader: Fread(\"/kern/sysdir\") failed!\r\n");
		goto error;
	}
	if (r >= sizeof(path))
	{
		ConsoleWrite("XaAES loader: buffer for Fread(\"/kern/sysdir\") to small!\r\n");
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

		/* if the system has a 68000 CPU we use the 68000 compiled
		 * module
		 */
		r = Ssystem(S_GETCOOKIE, C__CPU, &cpu);
		if (r == 0 )
		{
			switch( cpu )
			{
			case 30:
				name = "xaaes030.km";
			break;
			case 40:
				name = "xaaes040.km";
			break;
			case 60:
				name = "xaaes060.km";
			break;
			default:
				name = DEFAULT_68000;
			}
		}
		ConsoleWrite("CPU-cookie not found \r\n");
	}
	ConsoleWrite(name);
	ConsoleWrite("\r\n");

	/* change to the XaAES module directory */
	r = Dsetpath(path);
	if (r)
	{
		ConsoleWrite("XaAES loader: No such directory: \"");
		ConsoleWrite(path);
		ConsoleWrite("\"\r\n");
		goto error;
	}

	/* get absolute path to this directory */
	if( !(*path == 'u' && *(path+1) == ':' && *(path+2) == '/') )
	{
		r = Dgetpath(path, 0);
		if (r)
		{
			ConsoleWrite("XaAES loader: Dgetpath() failed???\r\n");
			goto error;
		}
	}

	ConsoleWrite(path);
	ConsoleWrite("'\r\n");
	/* append module name */
	my_strlcat(path, "\\", sizeof(path));
	my_strlcat(path, name, sizeof(path));

	/* check if file exist */
	fh = Fopen(path, O_RDONLY);
	if (fh < 0)
	{
		ConsoleWrite("XaAES loader: No such file: \"");
		ConsoleWrite(path);
		ConsoleWrite("\"\r\n");
		goto error;
	}
	Fclose((int)fh);

	ConsoleWrite("Load kernel module: '");
	ConsoleWrite(path);
	ConsoleWrite("'\r\n");

	fh = Fopen("/dev/km", O_RDONLY);
	if (fh < 0)
	{
		ConsoleWrite("XaAES loader: no /dev/km, please update your kernel!\r\n");
		goto error;
	}
	/* if mint waits for xaloader it should not exit */
	Psignal( SIGINT, ignore );
	Psignal( SIGQUIT, ignore );
	Psignal( SIGSTOP, ignore );
	Psignal( SIGTSTP, ignore );
	Psignal( SIGTERM, ignore );
	//Psignal( SIGKILL, ignore );

	//ConsoleWrite( "XaAES loader: KM_RUN \r\n");
	//Cconin();

	r = Fcntl((int)fh, path, KM_RUN);
	//ConsoleWrite( "XaAES loader: KM_RUN done()\r\n");

	if( r < 0 )
		r = -r;
	if (r)
	{
		char e[6];
		d2a( r, e );
		ConsoleWrite("XaAES loader: Fcntl(KM_RUN) failed!\r\nerror:");
		ConsoleWrite(e);
	}
	if( r != 64 )	// EBADARG
	{
		ConsoleWrite( "XaAES loader: KM_FREE\r\n");
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
				ConsoleWrite( "XaAES loader: KM_FREE failed trying: '");
				ConsoleWrite( p );
				ConsoleWrite( "'..\r\n");
				r = Fcntl((int)fh, p, KM_FREE);
			}
			if( r < 0 )
				r = -r;
			if( r )
			{
				char e[6];
				d2a( r, e );
				ConsoleWrite( "\r\nXaAES loader: KM_FREE failed: ");
				ConsoleWrite( e );
				ConsoleWrite( "press key ...\r\n");
				Cconin();
			}
		}

	}
	//ConsoleWrite( "XaAES loader: Fclose()\r\n");
	Fclose((int)fh);
	//ConsoleWrite( "XaAES loader: return\r\n");
	//Cconin();
	return r;

error:
	ConsoleWrite("press any key to continue ...\r\n");
	Cconin();
	return r;
}
