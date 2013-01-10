/*
 * tw-call.app
 *
 * TW-Call ist ein Zusatz fr TosWin 2.
 * Es wird beim AES angemeldet und bernimmt das Starten von TOS-Programmen.
 * Wird es ohne Parameter gestartet, lst es bei TosWin2 die Funktion 'Loginshell'
 * aus, ansonsten wird das bergeben Programm getstartet.
 * Luft TosWin2 noch nicht, wird es gestartet.
 *
 * 15.04.97		Programm angelegt (Loginshell).
 * 19.04.97		TalkTW integriert.
 * 02.05.97		\n als Arg-Trenner und argc, envc eingefhrt.
 * 20.07.97		Name (toswin*2*) gendert.
 * 24.08.97		Option '-l' eingefhrt.
 *					Ohne Parms wird nur noch TW2 gestartet.
 * 25.11.98		Ab N.AES 1.20 ist shel_write() case-sensetiv, also kommt der
 *					Start-Pfad nicht mehr in Groschrift und mu ge-lowert werden!
 * 11.04.99		Es wurde send_vastart() aus der CF-Lib benutzt, obwohl die Lib
 *					gar nicht initialisiert wurde. Fhrte zu Problemem unter MP.
 *					tw-call benutzt nun kein VA_START mehr, um eine neue Shell
 *					zu starten. Stattdessen wurde eine neue Nachricht definiert.
 * 23.05.99		Zustzlich zum Men-Namen, meldet tw-call den TOS-Namen
 *					auch mit menu_register(-1) an, d.h. beim appl_find(TOS-Name)
 *					wird tw-call gefunden.
 * 20.08.00		Check fr N.AES < 1.20 (case-sensetiver shel_write()) korrigiert.
 *
 */

#include <ctype.h>
#include <string.h>

#include <mint/basepage.h>
#include <mint/mintbind.h>
#include <sys/ioctl.h>

#include "../global.h"
#include "../share.h"


int	tw_id;
bool	quit = FALSE,
		dbg = FALSE;
char	menu_str[25] = "  ";
char	menu_str2[25] = "";
int	naes = 0;

static void copy_env(char *s, char *d)
{
	do
	{
		while ((*d++ = *s++) != '\0')
			;
	}
	while (*s);
	*d = '\0';
}

static void send_msg(int what)
{
	short	msg[8];

	msg[0] = what;
	msg[1] = gl_apid;
	msg[2] = 0;
	appl_write(tw_id, (int)sizeof(msg), msg);
}


static bool write_blk(int argc, char *argv[])
{
	int			fd, i;
	SHAREDATA	*blk;
	char			shm_name[256];
	bool			ok = FALSE;

	sprintf(shm_name, SHARENAME, gl_apid);

	if (dbg)
		printf("tw-call: SHM-Name: %s\n", shm_name);

	blk = (SHAREDATA *) Malloc(sizeof(SHAREDATA));
	if (blk != NULL)
	{
		fd = (int)Fcreate(shm_name, 0);
		if (fd >= 0)
		{
			Fcntl(fd, blk, SHMSETBLK);

			strcpy(blk->name, argv[1]);			/* Programmname */

			/* altes N.AES schickt Namen von Minix in GROSS! */
			if (((naes > 0) && (naes < 0x120)) || fs_case_sens(blk->name) == NO_CASE)
				str_tolower(blk->name);
			else
				blk->name[0] = tolower(blk->name[0]);	/* nur Laufwerk klein */

			get_path(blk->pfad, 0);					/* akt. Verzeichnis */

			strcpy(blk->arg, "");					/* Argumente */
			if (argc > 2)
			{
				strcpy(blk->arg, argv[2]);
				for (i = 3; i < argc; i++)
				{
					strcat(blk->arg, "\n");		/* LF als Trenner, damit ' ' nicht geklammert werden mssen! */
					strcat(blk->arg, argv[i]);
				}
			}

			copy_env(_base->p_env, blk->env);	/* Environment */

			if (dbg)
			{
				printf("tw-call: argv[1]   : %s\r\n", argv[1]);
				printf("tw-call: blk->name : %s\r\n", blk->name);
				printf("tw-call: blk->pfad : %s\r\n", blk->pfad);
				printf("tw-call: blk->arg  : %s\r\n", blk->arg);
			}

			Fclose(fd);
			ok = TRUE;
		}
		else
			form_alert(1, "[3][TW-Call:|Kann SHM-Datei nicht anlegen!][Abbruch]");
	}
	else
		form_alert(1, "[3][TW-Call:|Kein SharedMem nicht anfordern!][Abbruch]");

	return ok;
}


static int find_tw(void)
{
	int	id;
	char	progname[256], str[256], *p = NULL;

	id = appl_find("TOSWIN2 ");
	if (id < 0)
	{
		shel_envrn(&p, "TOSRUN=");
		if (p == NULL)
			p = getenv("TOSRUN");
		if (p != NULL)
		{
			strcpy(str, p);
			p = strrchr(str, '\\');
			*(p+1)= '\0';
			strcat(str, "toswin2.app");
		}
		else
		{
			shel_read(progname, str);
			if (strlen(progname) > 0)
			{
				strcpy(str, progname);
				p = strrchr(str, '\\');
				*(p+1) = '\0';
				strcat(str, "toswin2.app");
			}
		}
		id = shel_write(1, 1, 1, str, "");
		if (id > 0)
			evnt_timer(500L);
		else
			form_alert(1, "[3][TW-Call:|Kann TosWin2.app nicht finden!][Abbruch]");
	}
	return id;
}

int main(int argc, char *argv[])
{
	short ret = 0, smsg[8];
	short msg[8];
	char str[256];
	long l;

	dbg = (getenv("DEBUG") != NULL);

	if (dbg)
	{
		int   i;
		for (i = 0; i < argc; i++)
		printf("tw-call: %d. Argument: %s\n", i, argv[i]);
	}

	appl_init();
	(void)Pdomain(1);

	if (getcookie("nAES", &l))
		naes = *(unsigned short *)l;

	tw_id = find_tw();						/* TW2 suchen bzw. starten */
	if (tw_id > 0)
	{
		if (argc > 1 && strcmp(argv[1], "-l") == 0)	/* Neue Shell */
			send_msg(TWSHELL);
		else if (argc > 1)					/* TOS-Programm */
		{
			int	win;

			split_filename(argv[1], NULL, str);
			str_tolower(str);
			strcat(menu_str, str);
			menu_register(gl_apid, menu_str);

			split_filename(argv[1], NULL, menu_str2);
			menu_register(-1, menu_str2);
			
			/*
			 * Dummy-Fenster, damit wir mitbekommen, wenn TW-Call ber das
			 * Programm-Men aktiviert wird.
			*/
			win = wind_create(0, 0, 0, 0, 0);
			wind_open(win, 0, 0, 0, 0);

			send_msg(TWSTART);
			while (!quit)
			{
				evnt_mesag(msg);
				switch(msg[0])
				{
					case TWOK:
						if (write_blk(argc, argv))
							send_msg(TWWRITE);
						else
						{
							ret = -1;
							quit = TRUE;
						}
						break;

					case TWREAD:
						break;

					case TWERR:
						form_alert(1, "[3][TW-Call:|TosWin2 meldet Fehler!][Abbruch]");
						ret = -1;
						quit = TRUE;
						break;

					case CH_EXIT :
						/*
						 * TosWin2 schickt ein CH_EXIT mit unserer ID und ExitCode des
						 * TOS-Programmes !
						 */
						if (msg[3] == gl_apid)
						{
							ret = msg[4];
							quit = TRUE;
						}
						else if (msg[3] == tw_id)		/* TosWin2 wurde beendet */
						{
							ret = 0;
							quit = TRUE;
						}
						break;

					case AP_TERM :
						quit = TRUE;
						break;

					/* Aktionen auf Dummy-Window */
					case WM_ONTOP :
						wind_set(win, WF_BOTTOM, 0,0,0,0);
						break;
					case WM_TOPPED :
						smsg[0] = TWTOP;
						smsg[1] = gl_apid;
						appl_write(tw_id, (int)sizeof(smsg), smsg);
						break;

					default:
						if (dbg)
							printf("tw-call: unsupported message %d from %d\n", msg[0], msg[1]);
						break;
				}
			} /* while */
			wind_close(win);
			wind_delete(win);
		}
	}
	else
		form_alert(1, "[3][TW-Call:|TosWin2 luft nicht!][Abbruch]");

	if (dbg)
		printf("tw-call: leaving tw-call(%d) with exit_code = %d.\n", gl_apid, (int) ret);

	appl_exit();
	exit(ret);

	/* never reached */
	return 0;
}
