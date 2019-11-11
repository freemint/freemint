/* timeout-parameters */
#define BBL_MIN_TO	500L
#define BBL_MAX_TO	1500L
#define BBL_MAX_CNT	2

typedef enum{
	bbl_process_event,
	bbl_send_request,
	bbl_enable_bubble,
	bbl_disable_bubble,
	bbl_disable_and_free,
	bbl_get_status,
	bbl_close_bubble1,
	bbl_close_bubble2,
	bbl_tmp_inact
}BBL_MD;

typedef enum{
	bs_none,
	bs_inactive,
	/* from here it's running */
	bs_closed,
	bs_open,
	bs_tmpinact

}BBL_STATUS;

BBL_STATUS xa_bubble( int lock, BBL_MD md, union msg_buf *msg, short destID );
void bubble_show( const char *str );
void bubble_request( short pid, short whndl, short x, short y );
/* d0 */
#define BBL_EVNT_CLOSE1 0
#define BBL_EVNT_ENABLE 1
void XA_bubble_event(int lock, struct c_event *ce, short cancel);
void display_launched( int lock, char *str );


