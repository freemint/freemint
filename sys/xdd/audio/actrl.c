/*
 * /dev/audio configuration utility.
 *
 * (w) 1995, Kay Roemer.
 *
 *	9802, John Blakeley - added support for F030 which can handle up to
 *						4 pairs: had to change mono/stereo into chans.
 *						Also have changed audios.h so it's no longer
 *						compatible with previous versions.
 */

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
# include <unistd.h>
# include <fcntl.h>
# include <ioctl.h>

 
char *loptarg;

int
lgetopt (int argc, char *argv[], const char *opts)
{
	static char sp = 1;
	const char *op, *lop;
	int i, olen;

	if (sp >= argc || !strcmp ("--", argv[sp])) {
		sp = 1;
		return EOF;
	}
	for (i = 1, op = opts; *op; ++i, op = lop+1) {
		lop = strchr (op, ':');
		if (!lop) {
			lop = op + strlen (op);
		}
		if (op == lop) {
			fprintf (stderr, "invalid opt string (program error)\n");
			sp = 1;
			return 0;
		}
		if (lop[-1] == '=') {
			/*
			 * expects an argument
			 */
			olen = (lop - op) - 1;
			if (!strncmp (argv[sp], op, olen)) {
				if (sp+1 >= argc) {
					fprintf (stderr, "missing argument for"
						" option %s\n", argv[sp]);
					sp = 1;
					return 0;
				}
				loptarg = argv[sp+1];
				sp += 2;
				return i;
			}
		} else {
			/*
			 * no arg
			 */
			olen = lop - op;
			if (!strncmp (argv[sp], op, olen)) {
				++sp;
				return i;
			}
		}
	}
	fprintf (stderr, "invalid option: %s\n", argv[sp]);
	sp = 1;
	return 0;
}

void
usage (void)
{
	fprintf (stderr,
"usage: actrl [options ...]\n" \
"valid options are:\n" \
"\t chan\t -- set number of channels\n" \
"\t vol\t -- set volume: 0 (silent) ... 100 (loud)\n" \
"\t left\t -- set volume of left channel: 0 (silent) ... 100 (loud)\n" \
"\t right\t -- set volume of right channel: 0 (silent) ... 100 (loud)\n" \
"\t bass\t -- set bass amplification: 0 (low) ... 100 (high)\n" \
"\t treble\t -- set treble amplification: 0 (low) ... 100 (high)\n" \
"\t speed\t -- set play rate (in Hz)\n" \
"\t format\t -- set sample format: u8 (unsigned), s8 (signed), ulaw (u-law)\n" \
"\t       \t                       u16 (unsigned), s16 (signed)\n" \
"\t device\t -- set audio device name (default /dev/audio)\n" \
"\t reset\t -- reset audio device\n");

	exit (1);
}

#define OPTSTRING \
	"chan=:" \
	"vol=:left=:right=:bass=:treble=:" \
	"speed=:format=:" \
	"device=:" \
	"reset"

#define NOTSET 0x87654321

static long chan = NOTSET;
static long vol = NOTSET;
static long left = NOTSET;
static long right = NOTSET;
static long bass = NOTSET;
static long treble = NOTSET;
static long speed = NOTSET;
static char *format = NULL;
static char *device = "/dev/audio";
static long reset = 0;

int
main (int argc, char *argv[])
{
int c, r, fdevice, ret = 0;

	while ((c = lgetopt (argc, argv, OPTSTRING)) != EOF) {
		switch (c) {
		case 0: /* error */
			usage ();
			break;

		case 1: /* chan */
			chan = atol (loptarg);
			break;

		case 2: /* vol */
			vol = atol (loptarg);
			break;

		case 3: /* left */
			left = atol (loptarg);
			break;

		case 4: /* right */
			right = atol (loptarg);
			break;

		case 5: /* bass */
			bass = atol (loptarg);
			break;

		case 6: /* treble */
			treble = atol (loptarg);
			break;

		case 7: /* speed */
			speed = atol (loptarg);
			break;

		case 8: /* format */
			format = loptarg;
			break;

		case 9: /* device */
			device = loptarg;
			break;

		case 10: /* reset */
			reset = 1;
			break;
		}
	}
	fdevice = open (device, O_RDONLY);
	if (fdevice < 0) {
		fprintf (stderr, "cannot open audio device %s: %s.\n",
			device, strerror (errno));
		return 1;
	}
	if (reset) {
		r = ioctl (fdevice, AIOCRESET, (void *)reset);
		if (r < 0) {
			fprintf (stderr, "cannot reset audio device: %s\n",
				strerror (errno));
			ret = 1;
		}
	}
	if (chan != NOTSET) {
		r = ioctl (fdevice, AIOCSCHAN, (void *)chan);
		if (r < 0) {
			fprintf (stderr, "cannot change channels to %ld: %s\n",
				chan, strerror (errno));
			ret = 1;
		}
	}
	if (vol != NOTSET) {
		r = ioctl (fdevice, AIOCSVOLUME, (void *)vol);
		if (r < 0) {
			fprintf (stderr, "cannot set volume to %ld: %s\n",
				vol, strerror (errno));
			ret = 1;
		}
	}
	if (left != NOTSET) {
		r = ioctl (fdevice, AIOCSLVOLUME, (void *)left);
		if (r < 0) {
			fprintf (stderr, "cannot set left volume to %ld: %s\n",
				left, strerror (errno));
			ret = 1;
		}
	}
	if (right != NOTSET) {
		r = ioctl (fdevice, AIOCSRVOLUME, (void *)right);
		if (r < 0) {
			fprintf (stderr, "cannot set right volume to %ld: %s\n",
				left, strerror (errno));
			ret = 1;
		}
	}
	if (bass != NOTSET) {
		r = ioctl (fdevice, AIOCSBASS, (void *)bass);
		if (r < 0) {
			fprintf (stderr, "cannot set bass amp. to %ld: %s\n",
				bass, strerror (errno));
			ret = 1;
		}
	}
	if (treble != NOTSET) {
		r = ioctl (fdevice, AIOCSTREBLE, (void *)treble);
		if (r < 0) {
			fprintf (stderr, "cannot set treble amp. to %ld: %s\n",
				treble, strerror (errno));
			ret = 1;
		}
	}
	if (speed != NOTSET) {
		r = ioctl (fdevice, AIOCSSPEED, (void *)speed);
		if (r < 0) {
			fprintf (stderr, "cannot set speed  to %ldHz: %s\n",
				speed, strerror (errno));
			ret = 1;
		} else {
			long nspeed;
			r = ioctl (fdevice, AIOCGSPEED, &nspeed);
			if (r == 0 && nspeed != speed)
				fprintf (stderr, "speed set to %ldHz\n",
					nspeed);
		}
	}
	if (format) {
		long fmt = 0;
		if (!strcasecmp (format, "u8"))
			fmt = AFMT_U8;
		else if (!strcasecmp (format, "s8"))
			fmt = AFMT_S8;
		else if (!strcasecmp (format, "ulaw"))
			fmt = AFMT_ULAW;
		else if (!strcasecmp (format, "u16"))
			fmt = AFMT_U16;
		else if (!strcasecmp (format, "s16"))
			fmt = AFMT_S16;
		else {
			fprintf (stderr, "unknown sample format: %s\n",
				format);
			ret = 1;
		}
		if (fmt) {
			r = ioctl (fdevice, AIOCSFMT, (void *)fmt);
			if (r < 0) {
				fprintf (stderr, "cannot set sample format "
					"to %s\n",
					format);
				ret = 1;
			}
		}
	}
	return ret;
}
