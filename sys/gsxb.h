/*
 * $Id$
 *
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */



struct gsxb_device
{
	short	ver_minor;
	short	ver_major;
	short	dev_id;
	short	mix_id;
	short	dac_id;
	short	adc_id;
	short	dsp_id;

	long _cdecl (*mixio)(short dev_id, short mix_id, short cmd, long arg);
	long _cdecl (*dacio)(short dev_id, short dac_id, short cmd, long arg);
	long _cdecl (*dspio)(short dev_id, short dsp_id, short cmd, long arg);
};
