/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 */
#define GSXB_SEMA_GET		1
#define GSXB_SEMA_REL		0
#define GSXB_SEMA_QRY		-1

#define PCM_IOCTL_SBUFF		1
#define PCM_IOCTL_GUBFF		2
#define PCM_IOCTL_SSRATE	3
#define PCM_IOCTL_GSRATE	4
#define PCM_IOCTL_SSFORMAT	5
#define PCM_IOCTL_GSFORMAT	6
#define PCM_IOCTL_SVOICES	7
#define PCM_IOCTL_GVOICES	8
#define PCM_IOCTL_START		9
#define PCM_IOCTL_STOP		10
#define PCM_IOCTL_SFLAGS	11
#define PCM_IOCTL_GFLAGS	12
#define PCM_IOCTL_GFMTMAP	13
#define PCM_IOCTL_SCFORMAT	14
#define PCM_IOCTL_GCFORMAT	15
#define PCM_IOCTL_GDUPLEX	16
#define PCM_IOCTL_IEOFINTR	17
#define PCM_IOCTL_UEOFINTR	18
#define PCM_IOCTL_SEMAPHORE	19

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
