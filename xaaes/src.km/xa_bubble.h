typedef enum{
	bbl_process_event,
	bbl_send_request,
	bbl_enable_bubble,
	bbl_disable_bubble,
	bbl_disable_and_free,
	bbl_get_status,
	bbl_close_bubble1,
	bbl_close_bubble2
}BBL_MD;

typedef enum{
	bs_none,
	bs_inactive,
	/* from here it's running */
	bs_closed,
	bs_open

}BBL_STATUS;

BBL_STATUS xa_bubble( enum locks lock, BBL_MD md, union msg_buf *msg, short destID );
void XA_bubble_event(enum locks lock, struct c_event *ce, bool cancel);

#define BBL_LLEN	32
#define BBL_MAXLLEN	36

