/* BUBBLEGEM_REQUEST - Nachricht
 * msg[0]   $BABA
 * msg[1]   demonID
 * msg[2]   0
 * msg[3]   winID
 * msg[4]   mX
 * msg[5]   mY
 * msg[6]   KStat
 * msg[7]   0
 */
#define BUBBLEGEM_REQUEST  0xBABA

/* BUBBLEGEM_SHOW - Nachricht:
 * msg[0]   $BABB
 * msg[1]   ap_id
 * msg[2]   0
 * msg[3]   Maus X
 * msg[4]   Maus Y
 * msg[5/6] Pointer auf nullterminierte Zeichenkette im globalen
 *          Speicher
 * msg[7]   Special-Word (=Bitfield)
 */
#define BUBBLEGEM_SHOW  0xBABB


/* BUBBLEGEM_ACK ($BABC)
 *
 * msg[0]   $BABC
 * msg[1]   ap_id
 * msg[2]   0
 * msg[3]   0
 * msg[4]   0
 * msg[5/6] Pointer aus BUBBLEGEM_SHOW
 * msg[7]   0
 */
#define BUBBLEGEM_ACK   0xBABC


/* BUBBLEGEM_ASKFONT ($BABD)
 * msg[0] = $BABD;
 * msg[1] = apID;
 * msg[2] = 0;
 * msg[3] = 0;
 * msg[4] = 0;
 * msg[5] = 0;
 * msg[6] = 0;
 * msg[7] = 0;
 */
#define BUBBLEGEM_ASKFONT  0xbabd;

/* BUBBLEGEM_FONT ($BABE)
 * msg[0] = $BABE;
 * msg[1] = apID;
 * msg[2] = 0;
 * msg[3] = FontID;
 * msg[4] = FontPt;
 * msg[5] = 0;
 * msg[6] = 0;
 * msg[7] = 0;
 */
#define BUBBLEGEM_FONT     0xbabe;


/* BUBBLEGEM_HIDE ($BABF)
 * msg[0] = $BABF;
 * msg[1] = apID;
 * msg[2] = 0;
 * msg[3] = ?;
 * msg[4] = ?;
 * msg[5] = ?;
 * msg[6] = ?;
 * msg[7] = ?;
 */
#define BUBBLEGEM_HIDE     0xBABF;


/*
 * Daten fuer das Special-Word von BUBBLEGEM_SHOW
 */
#define BGS7_USRHIDE	0x0001		/*	fuer modalen Aufruf noetig	*/
#define BGS7_MOUSE 	0x0004		/*	BubbleGEM benutzt die Koordinaten nur 
												zum Zeichnen	*/


/*
 * BGEM Cookie	
 */
typedef struct
{
	long	magic;   /*	'BGEM'	*/
	long	size;    /*	Groesse dieser Struktur, derzeit 18	*/
	int	release; /*	derzeit 6, nie kleiner als 5	*/
	int	active;  /*	<>0, wenn gerade eine Hilfe angezeigt wird;
								0  sonst	*/
	MFORM	*mhelp;	/*	Zeiger auf Hilfe-Mausform	*/
	int	dtimer;	/*	Daemon-Timer; Default 200ms; ab Release 6	*/
}BGEM;


/* BHLP-Cookie:
 * BubbleGEM wertet den Cookie "BHLP" aus. Im oberen Word ist die Dauer 
 * der Mindest-Sichtbarkeit eingetragen, Standard sind 200 Millisekunden.
 * Das untere Word ist eine Bitmap; ist Bit 0 ($0001 = BGC_FONTCHANGED) 
 * gesetzt, so bedeutet dies, dass FONT_CHANGED ausgewertet wird. Ist 
 * BGC_NOWINSTYLE ($0002) (lies: no-win-style, nicht now-in-style) 
 * gesetzt, so wird die Hilfe als Sprechblase dargestellt (d.h. ein 
 * geloeschtes Bit entspricht der Windows-Hilfe!). Ein gesetztes Bit bei 
 * BGC_SENDKEY ($0004) bewirkt, dass nach Schliessen der Hilfe durch einen 
 * Tastendruck AV_SENDKEY an den Aufrufer geschickt wird. BGC_DEMONACTIVE
 * ($0008) bewirkt, dass Daemon eingeschaltet ist.
 */
#define	BGC_FONTCHANGED	0x0001
#define	BGC_NOWINSTYLE		0x0002
#define	BGC_SENDKEY			0x0004
#define	BGC_DEMONACTIVE	0x0004
