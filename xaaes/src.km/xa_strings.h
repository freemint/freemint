/*
 * Output-strings
 */

enum
{
 /* window-titles */
	RS_ABOUT,
	RS_TM,
	RS_SYS,
	RS_APPLST,
	/* system-window*/
	RS_ALERTS,
	RS_ENV,
	RS_SYSTEM,
	RS_VIDEO,
	RS_MEMPROT,
	RS_OFF,
	RS_ON,

	/* alert-text */
	ASK_QUITALL_ALERT,
	ASK_SHUTDOWN_ALERT,
	SNAP_ERR1,
	SNAP_ERR2,
	SNAP_ERR3,
	SDALERT,
	RS_LAUNCH,
	AL_SDMASTER,
	AL_KBD,
	AL_TERMAES,
	AL_KILLAES,
	AL_NOPB,
	AL_NOAESPR,
	AL_VALOBTREE,
	AL_ATTACH,
	AL_MEM,
	AL_PDLG,
	AL_STMD,
	AL_NOGLOBPTR,
	AL_SHELLRUNS,
	AL_NOSHELL,

	MNU_CLIENTS,

	XA_HELP_FILE
};

enum TR_STRINGS
{
	ABOUT_LINES,
	WCTXT_MAIN_TXT,
	XA_STRINGS
};


extern char *xa_strings[];
extern char **trans_strings[];

