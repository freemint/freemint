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

BBL_STATUS xa_bubble( enum locks lock, BBL_MD md, union msg_buf *msg, short destID );
void bubble_show( char *str );
void bubble_request( short pid, short whndl, short x, short y );
void XA_bubble_event(enum locks lock, struct c_event *ce, bool cancel);
void display_launched( enum locks lock, char *str );


