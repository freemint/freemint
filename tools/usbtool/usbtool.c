/*
 * USB Accessory for TOS.
 *
 * This deals with hotplug and displays the USB devices.
 *
 * By Alan Hourihane <alanh@fairlite.co.uk>
 *
 * Modified by Claude Labelle based on window handling code by Jacques Delavoix
 * used in his Windform demo.
 */
#define _cdecl
#define __KERNEL__ 1
#include <gem.h>
#include <mint/osbind.h>
#include <string.h>
#define TOSONLY
#include "../../sys/usb/src.km/global.h"
#include "../../sys/usb/src.km/usb.h"
#include "../../sys/usb/src.km/usb_api.h"
#include "usbtool.h"

#define NO_WINDOW   -1      /* no window opened */
#define NO_POSITION -1      /* window has no position yet */

/* USB_MAX_DEVICE is 32 */
char dev_names[USB_MAX_DEVICE][64];
short dev_types[USB_MAX_DEVICE];
short dev_count = 0;
short text_handle = 0;
short h_line;
short xf, yf, wf, hf;
VdiHdl vdi_handle;
short w_char, h_char;
extern short _app;

/* Main window attributes */
#define wind_attr NAME|CLOSER|FULLER|MOVER|SIZER|UPARROW|DNARROW|VSLIDE|LFARROW|RTARROW|HSLIDE|INFO

short line_0;                  /* 1st line of display */
short char_0;                  /* 1st character of display */
short full_flag = FALSE;
short total_height;            /* Total height in number of text lines */
short total_width;             /* Total width in number of characters */

struct usb_module_api *api = 0;
short polling_flag = 0;         /* ON=always poll, OFF=poll only when window is opened */

/* no need to run global constructors/destructors here */
void __main(void);
void __main(void) { }

int
main (void)
{
	short menuID;
	short work_in[11], work_out[57], dummy;
	int i;
	VdiHdl aes_handle;

	if (_app)
		return 0;
	appl_init ();
	menuID = menu_register (gl_apid, "  USB Utility");
	aes_handle = graf_handle(&w_char, &h_char, &dummy, &dummy);

	for (i = 0 ; i < 9 ; work_in[i++] = 1)
		;
	work_in[9] = 0;
	work_in[10] = 2;
	vdi_handle = aes_handle;
	v_opnvwk(work_in, &vdi_handle, work_out);
	vst_height(vdi_handle, h_char >= 16 ? 13 : 6, &dummy, &dummy, &dummy, &h_char);
	vst_alignment(vdi_handle, 0, 5, &dummy, &dummy);

	/* character height + 1, to prevent text lines from touching each other */
	h_line = h_char + 1;

	/* if usbtool.acc is renamed, don't always poll */
	if (appl_find("USBTOOL ")>= 0)
		polling_flag = 1;
	events (menuID);
	return 0;
}

void
events (short menuID)
{
	short event;
	short buff[8];
	short ret;
	unsigned long timer = 1000; /* Wake USB hub events every second. */
	int i;
	long changed;

	xf = NO_POSITION;

	for (;;)
	{
		event = evnt_multi (MU_MESAG | MU_TIMER,
				    0, 0, 0,
				    0, 0, 0, 0, 0,
				    0, 0, 0, 0, 0,
				    buff, timer,
				    &ret, &ret, &ret, &ret, &ret, &ret);

		if (event & MU_MESAG)
		{
			switch(buff[0])
			{
				case AC_OPEN:
					if (buff[4] == menuID)
						open_text();
					break;
				case AC_CLOSE:
					if (buff[3] == menuID)
						text_handle = 0;
					break;
				case WM_REDRAW :
					redraw(buff[3], buff[4], buff[5], buff[6], buff[7]);
					break;
				case WM_TOPPED :
					wind_set(buff[3], WF_TOP, 0, 0, 0, 0);
					break;
				case WM_CLOSED :
					close_text();
					break;
				case WM_FULLED :
					fulled();
					break;
				case WM_ARROWED :
					arrow(buff[4]);
					break;
				case WM_VSLID :
					vslider(buff[3], buff[4], buff[5], buff[6], buff[7]);
					break;
				case WM_HSLID :
					hslider(buff[3], buff[4], buff[5], buff[6], buff[7]);
					break;
				case WM_SIZED :
					sized(buff[3], buff[4], buff[5], buff[6], buff[7]);
					break;
				case WM_MOVED :
					wind_set(buff[3], WF_CURRXYWH, buff[4], buff[5], buff[6], buff[7]);
					xf = buff[4]; yf = buff[5]; /* remember the new coordinates */
					break;
			 }
		}
		else if (event & MU_TIMER)
		{
			if (text_handle > 0 || polling_flag)
			{
				if (!api)
				{
					getcookie (_USB, (long *)&api);
				}
				else
				{
					for (i = 0; i < api->max_hubs; i++)
					{
						struct usb_hub_device *hub = usb_get_hub_index (i);
						if (hub)
						{
							changed = usb_hub_events(hub);
							if (changed & USB_PORT_STAT_C_CONNECTION)
							/* when there is a port connection change reported by usb_hub_events */
							{
								if (text_handle > 0) {
									update_text();
								}
							}
						}
					}
				}
			}
		}
	}
}

void get_device_info(struct usb_device *dev, int dev_index)
{
	unsigned char ifaces = dev->config.no_of_if;
	struct usb_interface *iface;

	while (ifaces--)
	{
		iface = &dev->config.if_desc[ifaces];
		if (iface->desc.bInterfaceClass == USB_CLASS_HUB)
		{
			dev_types[dev_index] = dev->parent?DEV_TYPE_HUB:DEV_TYPE_ROOT_HUB;
		}
		else if (iface->driver)
		{
			dev_types[dev_index] = DEV_TYPE_GENERAL;
			break;
		}
		else
		{
			dev_types[dev_index] = DEV_TYPE_NODRIVER;
		}
	}
	memset (dev_names[dev_index], 0, 64);
	if (strlen(dev->mf) == 0 && strlen(dev->prod) == 0 && dev_index != 0)
	{
		strcat(dev_names[dev_index], "NO NAME");
	}
	else
	{
		strcat (dev_names[dev_index], dev->mf); /* manufacturer */
		if (strlen(dev->mf)) /* For better aligment don't add a blank if mf is empty */
			strcat (dev_names[dev_index], " ");
		strcat (dev_names[dev_index], dev->prod); /* product */
	}
}

int process_children(struct usb_device *dev, int dev_index)
{
	int i;

	for (i = 0; i <= dev->maxchild; i++)
	{
		if (!dev->children[i])
			continue;
		dev_index++;
		get_device_info(dev->children[i], dev_index);
		struct usb_interface *iface = &dev->children[i]->config.if_desc[0L];
		if (iface->desc.bInterfaceClass == USB_CLASS_HUB)
		{
			dev_index = process_children(dev->children[i], dev_index);
		}
	}
	return dev_index;
}

void update_text(void)
{
	int i, dev_index = 0;
	short msg[8];
	static char info_line[64] = "";
	char n_devices[3];

	if (!api)
	{
		/* Get USB cookie */
		getcookie (_USB, (long *)&api);
	}

	if (api)
	{
		for (i = 0; i < api->max_devices; i++)
		{
			struct usb_device *dev = usb_get_dev_index (i);
			if (dev)
			{
				/* See if it's a root hub */
				struct usb_interface *iface = &dev->config.if_desc[0L];
				if (iface->desc.bInterfaceClass == USB_CLASS_HUB && (!dev->parent))
				{
					get_device_info(dev, dev_index);
					dev_index = process_children(dev, dev_index);
					dev_index++;
				}
			}
		}
		dev_count = dev_index;
		if (dev_count == 0)
		{
			dev_types[0] = DEV_TYPE_ERROR;
			memset (dev_names[0], 0, 64);
			strcat (dev_names[0], "No USB interface found.");
			dev_count = 1;
		}
	}
	else
	{
		dev_types[0] = DEV_TYPE_ERROR;
		memset (dev_names[0], 0, 64);
		strcat (dev_names[0], "USB driver not loaded.");
		dev_count = 1;
	}

	/* Update info line */
	strncpy(info_line, " ", 2);
	n_string(n_devices, dev_index);
	strcat(info_line, n_devices);
	strcat(info_line, (dev_index<2)?" device":" devices");
	if (polling_flag == 0)
		strcat(info_line, " (updates when window open)");
	wind_set (text_handle, WF_INFO, (long) info_line >> 16, (long) info_line & 0xffff, 0, 0);

	/* Find maximum height and width */
	total_height = dev_count;
	total_width = 0;
	for (i = 0; i < dev_count; i++)
	{
		if (strlen(dev_names[i]) > total_width)
			total_width = (short) strlen(dev_names[i]);
	}
	sliders();
	/* Redraw */
	msg[0] = WM_REDRAW;
	msg[1] = gl_apid;
	msg[2] = 0;
	msg[3] = text_handle;
	msg[4] = xf;
	msg[5] = yf;
	msg[6] = wf;
	msg[7] = hf;
	appl_write(gl_apid, 16, &msg);
}

void open_text(void)
{
	static char *title = "USB Utility";
	short workW;
	short workH;

	if (text_handle > 0)					/* If the window is already opened */
		wind_set(text_handle, WF_TOP, 0, 0, 0, 0);	/* We bring it on top */
	else
	{
		line_0 = 0;
		char_0 = 0;
		/* Initial position */
		if (xf == NO_POSITION)
		{
			wf = w_char * 64 + 3;
			hf = h_char * 12 + 3;
			wind_get (0, WF_WORKXYWH, &xf, &yf, &workW, &workH);
			if (wf > workW)
				wf = workW;
			xf += (workW - wf) / 2;
			yf += (workH - hf) / 2;
		}

		if ((text_handle = wind_create(wind_attr, xf, yf, wf, hf)) > 0)
		{
			wind_set (text_handle, WF_NAME, (long) title >> 16, (long) title & 0xffff, 0, 0);
			wind_open(text_handle, xf, yf, wf, hf);
			update_text();
		}
	}
}

void close_text(void)
{
	if (text_handle > 0)
	{
		wind_close(text_handle);
		wind_delete(text_handle);
		text_handle = 0;
	}
}

void redraw(short w_handle, short x, short y, short w, short h)
{
	GRECT r, rd;
	short xo, yo, wo, ho, xy[4];
	short i, y_base;

	rd.g_x = x;
	rd.g_y = y;
	rd.g_w = w;
	rd.g_h = h;
	wind_get(w_handle, WF_WORKXYWH, &xo, &yo, &wo, &ho);
	wind_update(BEG_UPDATE);
	wind_get(w_handle, WF_FIRSTXYWH, &r.g_x, &r.g_y, &r.g_w, &r.g_h);
	while (r.g_w && r.g_h)
	{
		if (rc_intersect(&rd, &r))
		{
			v_hide_c(vdi_handle);
			set_clip(1, &r);
			y_base = yo + 1; /* Start display at y + 1, to prevent the first line of text from touching the top of the window */
			xo = xo + 4;	 /* to prevent the first character from touching the side of the window */
			xy[0] = r.g_x;
			xy[1] = r.g_y;
			xy[2] = xy[0]+r.g_w-1;
			xy[3] = xy[1]+r.g_h-1;
			vr_recfl(vdi_handle, xy);
			vswr_mode(vdi_handle, MD_TRANS);
			for (i = line_0; i < dev_count; i++)
			{
				switch(dev_types[i])
				{
					case DEV_TYPE_ROOT_HUB:
						vst_effects(vdi_handle, 1);
						vst_color(vdi_handle, 3);
						break;
					case DEV_TYPE_HUB:
						vst_effects(vdi_handle, 0);
						vst_color(vdi_handle, 3);
						break;
					case DEV_TYPE_GENERAL:
						vst_effects(vdi_handle, 0);
						vst_color(vdi_handle, 1);
						break;
					case DEV_TYPE_NODRIVER:
						vst_effects(vdi_handle, 2);
						vst_color(vdi_handle, 1);
						break;
					case DEV_TYPE_ERROR:
						vst_effects(vdi_handle, 0);
						vst_color(vdi_handle, 2);
						break;
					default:
						break;
				}
				v_gtext(vdi_handle, xo, y_base, dev_names[i] + MIN(char_0, strlen(dev_names[i])));
				y_base += h_line; /* Ready for next line */
			}
			vswr_mode(vdi_handle, MD_REPLACE);
			v_show_c(vdi_handle, 1);
		}
		wind_get(w_handle, WF_NEXTXYWH, &r.g_x, &r.g_y, &r.g_w, &r.g_h);
	}
	wind_update(END_UPDATE);
}

void sized(short w_handle, short x, short y, short w, short h)
{
	short wx, wy, ww, wh;
	short msg[8];
	wind_set(w_handle, WF_CURRXYWH, x, y, w, h);
	wind_get(w_handle, WF_WORKXYWH, &wx, &wy, &ww, &wh);
	/* Save the coordinates */
	wind_get(w_handle, WF_CURRXYWH, &xf, &yf, &wf, &hf);
	hf = hf - (wh - (wh / h_line * h_line)); /* Normalize to display entire line */
	full_flag = FALSE;		/* Cancel full screen flag */
	sliders();
	/* Redraw */
	msg[0] = WM_REDRAW;
	msg[1] = gl_apid;
	msg[2] = 0;
	msg[3] = w_handle;
	msg[4] = xf;
	msg[5] = yf;
	msg[6] = wf;
	msg[7] = hf;
	appl_write(gl_apid, 16, &msg);
}

void fulled()
{
	short wx, wy, ww, wh;
	short msg[8];
	if (full_flag)	/* If already full screen */
	{
		wind_get(text_handle, WF_PREVXYWH, &xf, &yf, &wf, &hf);
		full_flag = FALSE;
	}
	else
	{
		wind_get(text_handle, WF_CURRXYWH, &xf, &yf, &wf, &hf);
		wind_get(text_handle, WF_WORKXYWH, &wx, &wy, &ww, &wh);
		wf = w_char * total_width + wf - ww + 8;
		hf = h_char * MAX(total_height,3) + hf - wh + 8;
		wind_get (0, WF_WORKXYWH, &wx, &wy, &ww, &wh);
		if (hf > wh)
			hf = wh;
		if (wf > ww)
			wf = ww;
		full_flag = TRUE;
	}
	wind_set(text_handle, WF_CURRXYWH, xf, yf, wf, hf);
	sliders();	/* Adjust sizes and positions of sliders */
	/* Redraw */
	msg[0] = WM_REDRAW;
	msg[1] = gl_apid;
	msg[2] = 0;
	msg[3] = text_handle;
	msg[4] = xf;
	msg[5] = yf;
	msg[6] = wf;
	msg[7] = hf;
	appl_write(gl_apid, 16, &msg);
}

void arrow(short buff4)
{
	short wx, wy, ww, wh;
	int action = FALSE, page_height, page_width;
	short msg[8];

	wind_get(text_handle, WF_WORKXYWH, &wx, &wy, &ww, &wh);
	page_height = wh / h_line;	/* Window height in lines of characters */
	page_width = ww / w_char;	/* Window width in characters */
	switch (buff4)
	{
		case WA_UPPAGE :				/* Page Up */
			if (line_0 > 0)
			{
				line_0 = MAX (line_0 - page_height, 0);
				action = TRUE;
			}
			break;
		case WA_DNPAGE :				/* Page Down */
			if ((line_0 + page_height) < total_height)
			{
				line_0 = MIN (line_0 + page_height, total_height - page_height);
				action = TRUE;
			}
			break;
		case WA_LFPAGE :				/* Page Left */
			if (char_0 > 0)
			{
				char_0 = MAX (char_0 - page_width, 0);
				action = TRUE;
			}
			break;
		case WA_RTPAGE :				/* Page Right */
			if ((char_0 + page_width) < total_width)
			{
				char_0 = MIN (char_0 + page_width, total_width - page_width);
				action = TRUE;
			}
			break;
		case WA_UPLINE :				/* Line Up */
			if (line_0 > 0)
			{
				line_0--;
				action = TRUE;
			}
			break;
		case WA_DNLINE :				/* Line Down */
			if ((line_0 + page_height) < total_height)
			{
				line_0++;
				action = TRUE;
			}
			break;
		case WA_LFLINE :				/* Character Left */
			if (char_0 > 0)
			{
				char_0--;
				action = TRUE;
			}
			break;
		case WA_RTLINE :				/* Character Right */
			if ((char_0 + page_width) < total_width)
			{
				char_0++;
				action = TRUE;
			}
			break;
	}
	if (action)
	{
		sliders();
		/* Redraw */
		msg[0] = WM_REDRAW;
		msg[1] = gl_apid;
		msg[2] = 0;
		msg[3] = text_handle;
		msg[4] = wx;
		msg[5] = wy;
		msg[6] = ww;
		msg[7] = wh;
		appl_write(gl_apid, 16, &msg);
	}
}

void vslider(short w_handle, short x, short y, short w, short h)
{
	short slide, wx, wy, ww, wh, page_height, old_line_0;
	short msg[8];
	wind_get(w_handle, WF_WORKXYWH, &wx, &wy, &ww, &wh);
	slide = x;
	page_height = wh / h_line;	/* Height in lines of text */
	old_line_0 = line_0;
	line_0 = (short)((long)slide * (total_height - page_height) / 1000);
	wind_set(w_handle, WF_VSLIDE, slide, 0, 0, 0);
	/* Redraw */
	if (old_line_0 != line_0)
	{
		msg[0] = WM_REDRAW;
		msg[1] = gl_apid;
		msg[2] = 0;
		msg[3] = w_handle;
		msg[4] = wx;
		msg[5] = wy;
		msg[6] = ww;
		msg[7] = wh;
		appl_write(gl_apid, 16, &msg);
	}
}

void hslider(short w_handle, short x, short y, short w, short h)
{
	short slide, wx, wy, ww, wh, page_width, old_char_0;
	short msg[8];
	wind_get(w_handle, WF_WORKXYWH, &wx, &wy, &ww, &wh);
	slide = x;
	page_width = ww / w_char;	/* Width in number of characters */
	old_char_0 = char_0;
	char_0 = (short)((long)slide * (total_width - page_width) / 1000);
	wind_set(w_handle, WF_HSLIDE, slide, 0, 0, 0);
	/* Redraw */
	if (old_char_0 != char_0)
	{
		msg[0] = WM_REDRAW;
		msg[1] = gl_apid;
		msg[2] = 0;
		msg[3] = w_handle;
		msg[4] = wx;
		msg[5] = wy;
		msg[6] = ww;
		msg[7] = wh;
		appl_write(gl_apid, 16, &msg);
	}
}

/* Size and position of sliders V and H : */

void sliders(void)
{
	short wx, wy, ww, wh, page_height, page_width;
	wind_get(text_handle, WF_WORKXYWH, &wx, &wy, &ww, &wh);
	page_height = wh / h_line;
	page_width = ww / w_char;
	/* Size sliders */
	wind_set(text_handle, WF_VSLSIZE, (short) ((1000L * page_height)
						 / (MAX(1, total_height))),  0, 0, 0);
	wind_set(text_handle, WF_HSLSIZE, (short)((1000L * page_width)
						 / (MAX(1, total_width))), 0, 0, 0);
	line_0 = MIN(line_0, total_height - page_height);
	if (line_0 < 0)
		line_0 = 0;

	char_0 = MIN(char_0, total_width - page_width);
	if (char_0 < 0)
		char_0 = 0;

	/* Position sliders */
	wind_set(text_handle, WF_VSLIDE, (short)((1000L * line_0)
						/ (MAX(1, total_height - page_height))), 0, 0, 0);
	wind_set(text_handle, WF_HSLIDE, (short)((1000L * char_0)
						/ (MAX(1, total_width - page_width))), 0, 0, 0);
}

/***********************************************************************/

/*    Clipping :    */

void set_clip(short clip_flag, GRECT *area)
{
	short pxy[4];
	if (area == NULL)
		vs_clip(vdi_handle, clip_flag, pxy);
	else
	{
		pxy[0] = area->g_x;
		pxy[1] = area->g_y;
		pxy[2] = area->g_w + area->g_x - 1;
		pxy[3] = area->g_h + area->g_y - 1;
		vs_clip(vdi_handle, clip_flag, pxy);
	}
}

/* convert number to string, max 99. */
void n_string(char *s, int n)
{
	if (n > 99) {
		s[0] = '\0';
	}
	else
	{
		if (n <= 9)
		{
			s[0] = n + 48;
			s[1] = '\0';
		}
		else
		{
			s[0] = n/10 + 48;
			s[1] = n%10 + 48;
			s[2] = '\0';
		}
	}
}
