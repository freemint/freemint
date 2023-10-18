struct dev_info
{
	struct usb_device *dev;			/* Data Interface Class device */
	unsigned char ep_out;			/* endpoint out */
	unsigned char ep_in;			/* endpoint in */
};

/*    Prototypes : */

void find_data_interface(void);
void send_data(void);
void receive_data(void);
