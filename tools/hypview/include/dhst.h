typedef struct
{
	char *appname;		/*	The name of the application (e.g. "Texel")	*/
	char *apppath;		/*	The absolute path of the  application (e.g. "c:\program\texel.app")	*/
	char *docname;		/*	The name of the document (e.g. "balmer.txl")	*/
	char *docpath;		/*	The absolute path of the document (e.B. "d:\data\balmer.txl")	*/
}DHSTINFO;

/*
	The application then sends the DHST-Server the following message:
		msg[0]			DHST_ADD
		msg[1]			ap_id
		msg[2]			0
		msg[3]+msg[4]	Pointer to the DHSTINFO-structure
		msg[5]			0
		msg[6]			0
		msg[7]			0
*/
#define	DHST_ADD	0xdadd

/*
	As response you'll get:
		msg[0]			DHST_ACK
		msg[1]			ap_id
		msg[2]			0
		msg[3]+msg[4]	Pointer to DHSTINFO-structure
							(As passed with DHST_ADD)
		msg[5]			0
		msg[6]			0
		msg[7]			0=error(s), else: OK
*/
#define	DHST_ACK	0xdade
