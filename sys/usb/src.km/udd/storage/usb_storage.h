struct us_data;
typedef long (*trans_cmnd)(ccb *cb, struct us_data *data);
typedef long (*trans_reset)(struct us_data *data);

/* Galvez: 
 *     For this struct we should take care and cast to long the unsigned 
 *     char elements before calling some of the MACROs defined in usb.h
 *     If we don't do that, when compiling with mshort, some values are 
 *     transformed wrongly during the operations done in those MACROs
 */
struct us_data
{
	struct usb_device *pusb_dev;		/* this usb_device */
	unsigned long	flags;			/* from filter initially */
	unsigned char	ifnum;			/* interface number */
	unsigned char	ep_in;			/* in endpoint */
	unsigned char	ep_out;			/* out ....... */
	unsigned char	ep_int;			/* interrupt . */
	unsigned char	subclass;		/* as in overview */
	unsigned char	protocol;		/* .............. */
	unsigned char	attention_done;		/* force attn on first cmd */
	unsigned short	ip_data;		/* interrupt data */
	long		action;			/* what to do */
	long		ip_wanted;		/* needed */
	long		*irq_handle;		/* for USB int requests */
	unsigned long	irqpipe;		/* pipe for release_irq */
	unsigned char	irqmaxp;		/* max packed for irq Pipe */
	unsigned char	irqinterval;		/* Intervall for IRQ Pipe */
	ccb		*srb;			/* current srb */
	trans_reset	transport_reset;	/* reset routine */
	trans_cmnd	transport;		/* transport routine */
};

#define USB_MAX_STOR_DEV 	10 /* Total max number of LUN units */
