
#ifndef tw_share_h
# define tw_share_h 1


#define	SHARENAME	"u:\\shm\\tw-call.%d"

/* AES-Message Nummern zu Komunikation */
#define TWSTART	0x0CF1		/* TW-Call -> TosWin:  empfangsbereit? */
#define TWOK		0x0CF2		/* TosWin  -> TW-Call: ja */
#define TWWRITE	0x0CF3		/* TW-Call -> TosWin:  Daten geschrieben */
#define TWREAD		0x0CF4		/* TosWin  -> TW-Call: Daten gelesen */
#define TWERR		0x0CF5		/* TosWin  -> TW-Call: Fehler! */
#define TWTOP		0x0CF6		/* TW-Call -> TosWin:  Fenster toppen */
#define TWSHELL	0x0CF7		/* TW-Call -> TosWin:  Neue Shell */

typedef struct 
{
	char	name[256];		/* Programm Name inkl. Pfad */
	char	pfad[256];		/* akt. Verzeichnis beim Start */
	char	arg[4096];		/* Argument */
	char	env[4096];		/* Environment */
} SHAREDATA;
	

void	handle_share	(int id);
void	handle_exit	(int id);


#endif
