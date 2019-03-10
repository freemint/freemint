/*
 *  usb_printer.h
 *
 *
 *  Created by Claude Labelle on 18-11-29.
 */

#ifndef usb_printer_h
#define usb_printer_h
/* #include "mint/mint.h" */
#define PRINTER_NO_ERROR		0L
#define PRINTER_PAPER_EMPTY		1L
#define TYPE_UNKNOWN 	0
#define TYPE_TEXT		1
#define TYPE_PJL		2
#define TYPE_PCL		3
#define TYPE_PDF		4
#define TYPE_JPG		5

#define PJL_CODE 		"\x1B%-12345X"
#define PDF_CODE 		"%PDF-"
#define PDF_CODE_EOF	"%%EOF"
#define JPG_CODE		"\xFF\xD8\xFF"
#define JPG_CODE_EOF	"\xFF\xD9"

void printer_spool (char *filename);
long printer_reset(void);
long printer_status(void);

/* for future API, work in progress */
/*
struct printer_module_api
{
	void _cdecl (*printer_spool) (char *filename);
	void _cdecl (*printer_reset) (void);
};

#define api_printer_spool (*printer_api->printer_spool)
#define api_printer_reset (*printer_api->printer_reset)
*/
#endif /* usb_printer_h */
