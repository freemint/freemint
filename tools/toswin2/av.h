
#ifndef _tw2_av_h_
#define _tw2_av_h_

/*
 * Communication with AV server.
 */
#define AV_PROTOKOLL		0x4700
#define VA_PROTOSTATUS		0x4701
#define AV_SENDKEY		0x4710
#define VA_START		0x4711
#define AV_ACCWINDOPEN		0x4724
#define VA_DRAGACCWIND		0x4725
#define AV_ACCWINDCLOSED	0x4726
#define AV_EXIT			0x4736
#define AV_STARTED		0x4738


/*
 * Send key to AV server.
 */
bool	send_avkey	(short ks, short kr);

/*
 * Send opened or cloased window to AV server.
 */
void	send_avwinopen	(short handle);
void	send_avwinclose	(short handle);

/*
 * Send <data> ton ST-Guide. 
 * If <with_hyp> ST-Guide try to find <data> in toswin2.hyp (online help).
 */
void	call_stguide	(char *data, bool with_hyp);

/*
 * Send <str> via VA_START to <appname>
 */
void	send_to		(char *appname, char *str);

/*
 * Handle AV massages.
 */
void	handle_av	(short *msg);

bool	av_init		(void);
void	av_term		(void);


#endif
