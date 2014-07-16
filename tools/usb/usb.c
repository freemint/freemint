/*
 * USB Accessory for TOS.
 *
 * This deals with hotplug and displays the USB devices.
 *
 * By Alan Hourihane <alanh@fairlite.co.uk>
 */
#define _cdecl
#define __KERNEL__ 1
#include <gem.h>
#include <mint/osbind.h>
#define TOSONLY
#include "../../sys/usb/src.km/global.h"
#include "../../sys/usb/src.km/usb.h"
#include "../../sys/usb/src.km/usb_api.h"

#define NO_WINDOW	-1      /* no window opened */
#define	NO_POSITION	-1      /* window has no position yet */

typedef struct window
{
    short id;
    short x;
    short y;
    short w;
    short h;
} Window;

OBJECT tree[] = {
    {-1, 1, 6, G_BOX, OF_NONE, OS_NORMAL, {0x000000L}, 0, 0, 6, 6},
    {2, -1, -1, G_STRING, OF_NONE, OS_NORMAL, {0L}, 0, 0, 6, 1},
    {3, -1, -1, G_STRING, OF_NONE, OS_NORMAL, {1L}, 0, 1, 6, 1},
    {4, -1, -1, G_STRING, OF_NONE, OS_NORMAL, {2L}, 0, 2, 6, 1},
    {5, -1, -1, G_STRING, OF_NONE, OS_NORMAL, {3L}, 0, 3, 6, 1},
    {6, -1, -1, G_STRING, OF_NONE, OS_NORMAL, {4L}, 0, 4, 6, 1},
    {0, -1, -1, G_STRING, OF_LASTOB, OS_NORMAL, {5L}, 0, 5, 6, 1}
};

struct usb_module_api *api = 0;
static int first_time = 1;

void events (short menuID);
void openWindow (Window * wp);
void closeWindow (Window * wp);
void update (Window * wp);
void init (void);

void
init (void)
{
    short menuID;

    appl_init ();
    menuID = menu_register (gl_apid, "  USB Utility");
    events (menuID);
}

void
events (short menuID)
{
    Window wind;
    short event;
    short msgbuf[8];
    short ret;
    unsigned long timer = 1000; /* Wake USB hub events every second. */

    wind.id = NO_WINDOW;
    wind.x = NO_POSITION;

    for (;;)
    {
        event = evnt_multi (MU_MESAG | MU_TIMER,
                            0, 0, 0,
                            0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0,
                            msgbuf, timer,
                            &ret, &ret, &ret, &ret, &ret, &ret);

        if (event & MU_MESAG)
        {
            switch (msgbuf[0])
            {
            case AC_OPEN:
                if (msgbuf[4] == menuID)
                {
                    if (wind.id == NO_WINDOW)
                    {
                        openWindow (&wind);
                    }
                    else
                    {
                        wind_set (wind.id, WF_TOP, 0, 0, 0, 0);
                    }
                }
                break;
            case AC_CLOSE:
                if (msgbuf[3] == menuID)
                    wind.id = NO_WINDOW;
                break;
            case WM_CLOSED:
                if (msgbuf[3] == wind.id)
                    closeWindow (&wind);
                break;
            case WM_MOVED:
                wind_set (wind.id, WF_CURRXYWH, msgbuf[4], msgbuf[5],
                          msgbuf[6], msgbuf[7]);
                wind.x = msgbuf[4];
                wind.y = msgbuf[5];
                wind.w = msgbuf[6];
                wind.h = msgbuf[7];
                break;
            case WM_NEWTOP:
            case WM_TOPPED:
                if (msgbuf[3] == wind.id)
                    wind_set (wind.id, WF_TOP, 0, 0, 0, 0);
                break;
            }
        }

        if (event & MU_TIMER)
        {
            if (!api)
            {
                api = get_usb_cookie ();
            }
            else
            {
                long i;

                if (first_time)
                {
                    openWindow (&wind);
                }

                for (i = 0; i < api->max_hubs; i++)
                {
                    struct usb_hub_device *hub = usb_get_hub_index (i);
                    if (hub)
                    {
                        if (usb_hub_events (hub))
                        {
                            if (wind.id != NO_WINDOW)
                            {
                                update (&wind);
                            }
                        }
                    }
                }

                if (first_time)
                {
                    first_time = 0;
                    closeWindow (&wind);
                }
            }
        }
    }
}

void
update (wp)
     Window *wp;
{
    int i, j = 0;
    static char *title = "USB Utility";
    static char usbname[6][64];

    wind_set (wp->id, WF_NAME, (long) title >> 16, (long) title & 0xffff, 0,
              0);

    /* currently only the first 6 USB devices displayed. */
    for (i = 0; i < 6; i++)
    {
        memset (usbname[i], 0, 64);
        if (first_time)
        {
            strcat (usbname[i], "Initializing....");
        }
    }

    if (api)
    {
        for (i = 0; i < api->max_devices; i++)
        {
            struct usb_device *dev = usb_get_dev_index (i);
            if (dev && dev->mf && dev->prod)
            {
                strcat (usbname[j], dev->mf);
                strcat (usbname[j], " ");
                strcat (usbname[j], dev->prod);
                j++;
            }
        }
    }

    tree[0].ob_x = wp->x;
    tree[0].ob_y = wp->y;
    tree[0].ob_width = wp->w;
    tree[0].ob_height = wp->h;

    tree[1].ob_spec.free_string = usbname[0];
    tree[1].ob_x = 8;
    tree[1].ob_y = 24;
    tree[1].ob_width = 7;
    tree[1].ob_height = 8;

    tree[2].ob_spec.free_string = usbname[1];
    tree[2].ob_x = 8;
    tree[2].ob_y = 24 + (16 * 1);
    tree[2].ob_width = 7;
    tree[2].ob_height = 8;

    tree[3].ob_spec.free_string = usbname[2];
    tree[3].ob_x = 8;
    tree[3].ob_y = 24 + (16 * 2);
    tree[3].ob_width = 7;
    tree[3].ob_height = 8;

    tree[4].ob_spec.free_string = usbname[3];
    tree[4].ob_x = 8;
    tree[4].ob_y = 24 + (16 * 3);
    tree[4].ob_width = 7;
    tree[4].ob_height = 8;

    tree[5].ob_spec.free_string = usbname[4];
    tree[5].ob_x = 8;
    tree[5].ob_y = 24 + (16 * 4);
    tree[5].ob_width = 7;
    tree[5].ob_height = 8;

    tree[6].ob_spec.free_string = usbname[5];
    tree[6].ob_x = 8;
    tree[6].ob_y = 24 + (16 * 5);
    tree[6].ob_width = 7;
    tree[6].ob_height = 8;

    form_dial (FMD_START, wp->x, wp->y, wp->w, wp->h, wp->x, wp->y, wp->w,
               wp->h);
    objc_draw (tree, 0, MAX_DEPTH, wp->x, wp->y, wp->w, wp->h);
    form_dial (FMD_FINISH, wp->x, wp->y, wp->w, wp->h, wp->x, wp->y, wp->w,
               wp->h);
}

void
openWindow (wp)
     Window *wp;
{
    short workW;
    short workH;
    short ret;

    if (wp->id == NO_WINDOW)
    {
        if (wp->x == NO_POSITION)
        {
            graf_handle (&wp->w, &ret, &ret, &wp->h);
            wp->w *= 64 + 3;
            wp->h *= 10;
            wind_get (0, WF_WORKXYWH, &wp->x, &wp->y, &workW, &workH);
            wp->x += (workW - wp->w) / 2;
            wp->y += (workH - wp->h) / 2;
        }
        wp->id =
            wind_create (VSLIDE | NAME | CLOSER | MOVER, wp->x, wp->y, wp->w,
                         wp->h);
        wind_open (wp->id, wp->x, wp->y, wp->w, wp->h);
        update (wp);
    }
}

void
closeWindow (wp)
     Window *wp;
{
    if (wp->id != NO_WINDOW)
    {
        wind_close (wp->id);
        wind_delete (wp->id);
        wp->id = NO_WINDOW;
    }
}
