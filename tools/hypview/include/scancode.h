/*
 *		Eine Sammlung wichtiger Scancodes fuer Ataris
 *		Philipp Donz 11.08.2000
 */
#ifndef __SCANCODES__
#define __SCANCODES__	1

/*	Bits zu Kennzeichnung der Zusatztasten	*/
#define	KbNUM			0x10		/*	Zahlenblock	*/
#define	KbALT			0x08
#define	KbCTRL			0x04
#define	KbSHIFT			0x03
#define	KbLSHIFT		0x02
#define	KbRSHIFT		0x01

/*	selten vorhandene Tasten	*/
#define	KbISO				0x37
#define	KbPAGEUP			0x49
#define	KbPAGEDOWN		0x51
#define	KbEND				0x4F

/*	Funktionstasten	*/
#define	KbF1				0x3b
#define	KbF2				0x3c
#define	KbF3				0x3d
#define	KbF4				0x3e
#define	KbF5				0x3f
#define	KbF6				0x40
#define	KbF7				0x41
#define	KbF8				0x42
#define	KbF9				0x43
#define	KbF10				0x44
/*	Shift+Funktionstasten	*/
#define	KbF11				0x54
#define	KbF12				0x55
#define	KbF13				0x56
#define	KbF14				0x57
#define	KbF15				0x58
#define	KbF16				0x59
#define	KbF17				0x5a
#define	KbF18				0x5b
#define	KbF19				0x5c
#define	KbF20				0x5d
/*	Cursor-Bereich	*/
#define	KbUNDO			0x61
#define	KbHELP			0x62
#define	KbINSERT			0x52
#define	KbHOME			0x47
#define	KbUP				0x48
#define	KbDOWN			0x50
#define	KbLEFT			0x4b
#define	KbRIGHT			0x4d
/*	Alternate-numerische Tasten	*/
#define	KbAlt1			0x78
#define	KbAlt2			0x79
#define	KbAlt3			0x7a
#define	KbAlt4			0x7b
#define	KbAlt5			0x7c
#define	KbAlt6			0x7d
#define	KbAlt7			0x7e
#define	KbAlt8			0x7f
#define	KbAlt9			0x80
#define	KbAlt0			0x81

#endif