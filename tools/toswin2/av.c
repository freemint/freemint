
#undef DEBUG_AV

#include <osbind.h>

#include "av.h"
#include "toswin2.h"
#include "proc.h"
#include "drag.h"

int		av_shell_id = -1;		/* ID des Desktops */
unsigned short	av_shell_status = 0;		/* Welche AV_* kann Desktop */

bool	gl_avcycle = FALSE;

static char	*global_str = NULL;
static short	msgbuff[8];

static bool send_msg(void)
{
	int	ret;

	msgbuff[1] = gl_apid;
	msgbuff[2] = 0;
	ret = appl_write(av_shell_id, sizeof(msgbuff), msgbuff);
	return (ret > 0);
}

static void send_avstarted(int id, int i1, int i2)
{
	short	msg[8];
	
	msg[0] = AV_STARTED;
	msg[1] = gl_apid;
	msg[2] = 0;
	msg[3] = i1;
	msg[4] = i2;
#ifdef DEBUG_AV
debug("AV_STARTED (%d)\n", id);
#endif
	appl_write(id, (int)sizeof(msg), msg);	/* muž nicht av_shell_id sein!! */
}

static void send_avprot(void)
{
	if (av_shell_id >= 0)
	{
		memset(msgbuff, 0, (int)sizeof(msgbuff));
		msgbuff[0] = AV_PROTOKOLL;
		msgbuff[3] = (2/*|16*/);		/* VA_START, Quoting */
		strcpy(global_str, TW2NAME);
		ol2ts((long)global_str, &msgbuff[6], &msgbuff[7]);
#ifdef DEBUG_AV
debug("AV_PROTOKOLL (%d)\n", av_shell_id);
#endif
		send_msg();
	}
}

static void send_avexit(void)
{
	if ((av_shell_id >= 0) && (av_shell_status & 1024))
	{
		memset(msgbuff, 0, sizeof(msgbuff));
		msgbuff[0] = AV_EXIT;
		msgbuff[3] = gl_apid;
#ifdef DEBUG_AV
debug("AV_EXIT\n");
#endif
		send_msg();
	}
}

bool send_avkey(short ks, short kr)
{
	bool	b = FALSE;

	if ((av_shell_id >= 0) && (av_shell_status & 1))
	{
		memset(msgbuff, 0, sizeof(msgbuff));
		msgbuff[0] = AV_SENDKEY;
		msgbuff[1] = gl_apid;
		msgbuff[3] = ks;
		msgbuff[4] = kr;
#ifdef DEBUG_AV
debug("AV_SENDKEY (%d,%d)\n", ks, kr);
#endif
		b = send_msg();
	}
	return b;
}

void send_avwinopen(short handle)
{
	if ((av_shell_id >= 0) && gl_avcycle)
	{
		memset(msgbuff, 0, (int)sizeof(msgbuff));
		msgbuff[0] = AV_ACCWINDOPEN;
		msgbuff[3] = handle;
#ifdef DEBUG_AV
debug("AV_ACCWINDOPEN (%d)\n", handle);
#endif
		send_msg();
	}
}

void send_avwinclose(short handle)
{
	if ((av_shell_id >= 0) && gl_avcycle)
	{
		memset(msgbuff, 0, (int)sizeof(msgbuff));
		msgbuff[0] = AV_ACCWINDCLOSED;
		msgbuff[3] = handle;
#ifdef DEBUG_AV
debug("AV_ACCWINDCLOSED (%d)\n", handle);
#endif
		send_msg();
	}
}

void handle_av(short *msg)
{
	char *p;
	
	switch (msg[0])
	{
		case VA_START :
			p = (char *)ts2ol(msg[3], msg[4]);
			if (p != NULL)
			{
#ifdef DEBUG_AV
debug("VA_START %s\n", p);
#endif
				if (strcmp(p, "-l") == 0)
					new_shell();
			}
			send_avstarted(msg[1], msg[3], msg[4]);
			break;

		case VA_PROTOSTATUS :
#ifdef DEBUG_AV
{
	unsigned short m;
	m = (unsigned short)msg[3];
debug("VA_PROTSTATUS %u\n", m);
}
#endif
			av_shell_status = msg[3];
			if (gl_avcycle && !(av_shell_status & 64))
				gl_avcycle = FALSE;			/* glob. Fensterwechsel abschalten */
			break;

		case VA_DRAGACCWIND :				/* bei D&D mit glob. Fensterwechsel */
			p = (char *)ts2ol(msg[6], msg[7]);
			if (p != NULL)
			{
#ifdef DEBUG_AV
debug("VA_DRAGACCWIND %s\n", p);
#endif
				handle_avdd(msg[3], p);
			}
			break;

		default:
#ifdef DEBUG_AV
debug("Unbekannte AV-Meldung: %d, %X\n", msg[0], msg[0]);
#endif
			break;
	}
}

void call_stguide(char *data, bool with_hyp)
{
	int	stg_id;

	stg_id = appl_find("ST-GUIDE");
	if (stg_id > 0)
	{
		if (with_hyp)
		{
			char	str[80] = "*:\\toswin2.hyp ";

			strcat(str, data);
			send_vastart(stg_id, str);
		}
		else
			send_vastart(stg_id, data);
	}
	else
		alert(1, 0, NOSTGUIDE);
}


void send_to(char *appname, char *str)
{
	int	id;
	
	id = appl_find(appname);
	if (id >= 0)
		send_vastart(id, str);
}



bool av_init(void)
{
	int	i;
	char	name[9], *p;

	p = getenv("AVSERVER");
	if (p != NULL)
	{
		strncpy(name, p, 8);
		name[8] = '\0';
		for (i = strlen(name); i < 8; i++)
			strcat(name, " ");
		i = appl_find(name);
		if (i >= 0)
		{
			av_shell_id = i;
			global_str = malloc_global(80);
			send_avprot();
		}
	}
	return (av_shell_id >= 0);
}

void av_term(void)
{
	send_avexit();
	if (global_str != NULL)
		Mfree(global_str);
}
