/*
 *	/dev/audio for Atari Ste, MegaSte, TT, Falcon running Mint.
 *	(LMC mixer)
 *
 *	Based on audiodev 0.1 by Charles Briscoe-Smith, which is in
 *	turn based on lpdev 0.6 by Thierry Bousch.
 *
 *	10/28/95, Kay Roemer.
 */

# include "global.h"
# include "mint/ioctl.h"

# include "dmasnd.h"
# include "device.h"


static long	lmc_ioctl (short cmd, void *arg);
static long	nomix_ioctl (short cmd, void *arg);

long
lmc_init (struct device *dev)
{
	long snd_cookie;

	if (get_toscookie (COOKIE__SND, &snd_cookie) ||
	    !(snd_cookie & 2)) {
		/*
		 * _SND cookie not set or no Ste compatible
		 * sound hardware
		 */
		return 1;
	}
	MWmask = MW_VAL;
	MWwrite (MW_VOL  | MW_SCALE (100, MW_VOL_VALS));
	MWwrite (MW_LVOL | MW_SCALE (100, MW_BAL_VALS));
	MWwrite (MW_RVOL | MW_SCALE (100, MW_BAL_VALS));
	MWwrite (MW_BASS | MW_SCALE ( 50, MW_AMP_VALS));
	MWwrite (MW_TREB | MW_SCALE ( 50, MW_AMP_VALS));

	dev->mix_ioctl = lmc_ioctl;
	return 0;
}

static long
lmc_ioctl (mode, buf)
	short mode;
	void *buf;
{
	long arg = (long)buf;

	switch (mode) {
	case AIOCSVOLUME:
		if (!MW_CHECK (arg))
			return EINVAL;
		MWwrite (MW_VOL | MW_SCALE (arg, MW_VOL_VALS));
		break;

	case AIOCSLVOLUME:
		if (!MW_CHECK (arg))
			return EINVAL;
		MWwrite (MW_LVOL | MW_SCALE (arg, MW_BAL_VALS));
		break;

	case AIOCSRVOLUME:
		if (!MW_CHECK (arg))
			return EINVAL;
		MWwrite (MW_RVOL | MW_SCALE (arg, MW_BAL_VALS));
		break;

	case AIOCSBASS:
		if (!MW_CHECK (arg))
			return EINVAL;
		MWwrite (MW_BASS | MW_SCALE (arg, MW_AMP_VALS));
		break;

	case AIOCSTREBLE:
		if (!MW_CHECK (arg))
			return EINVAL;
		MWwrite (MW_TREB | MW_SCALE (arg, MW_AMP_VALS));
		break;

	default:
		return ENOSYS;
	}
	return 0;
}

long
nomix_init (struct device *dev)
{
	dev->mix_ioctl = nomix_ioctl;
	return 0;
}

static long
nomix_ioctl (mode, buf)
	short mode;
	void *buf;
{
	return ENOSYS;
}
