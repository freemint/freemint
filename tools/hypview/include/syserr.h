/*
	Bios und Gemdos-Fehlernummern
	(c) 20.02.1998 by Ph.Donze
*/

/*		BIOS-Errors		*/
#define	E_OK		 0			/*	No error	*/
#define	ERROR		-1			/*	Generic error	*/
#define	EDRVNR	-2			/*	Drive not ready	*/
#define	EUNCMD	-3			/*	Unknown command	*/
#define	E_CRC		-4			/*	CRC error	*/
#define	EBADRQ	-5			/*	Bad request	*/
#define	E_SEEK	-6			/*	Seek error	*/
#define	EMEDIA	-7			/*	Unknown media	*/
#define	ESECNF	-8			/*	Sector not found	*/
#define	EPAPER	-9			/*	Out of paper	*/
#define	EWRITF	-10		/*	Write fault	*/
#define	EREADF	-11		/*	Read fault	*/
#define	EGENRL	-12		/*	General error	*/
#define	EWRPRO	-13		/*	Device is write protected	*/
#define	E_CHNG	-14		/*	Media change detected	*/
#define	EUNDEV	-15		/*	Unknown device	*/
#define	EBADSF	-16		/*	Bad sectors on format	*/
#define	EOTHER	-17		/*	Insert other disk	*/
#define	EINSERT	-18		/*	Insert disk (Meta-DOS)	*/
#define	EDVNRSP	-19		/*	Device not responding (Meta-DOS)	*/



/*		GEMDOS-Errors		*/
#define	EINVFN	-32L	/*	Invalid function	*/
#define	EFILNF	-33L	/*	File not found	*/
#define	EPTHNF	-34L	/*	Path not found	*/
#define	ENHNDL	-35L	/*	No more handles	*/
#define	EACCDN	-36L	/*	Access denied	*/
#define	EIHNDL	-37L	/*	Invalid handle	*/
#define	ENSMEM	-39L	/*	Insufficient memory	*/
#define	EIMBA		-40L	/*	Invalid memory block	*/
#define	EDRIVE	-46L	/*	Invalid drive	*/
#define	ENSAME	-48L	/*	Cross device rename / Not the same drive	*/
#define	EXDEV				ENSAME	/*	Not the same Drive	*/
#define	ENMFIL	-49L	/*	No more files	*/
#define	ELOCKED	-58L	/*	Record is already locked	*/
#define	ENSLOCK	-59L	/*	Invalid lock removal	*/
#define	ERANGE	-64L	/*	Range error	*/
#define	ENAMETOOLONG	ERANGE	/*	Range error	*/
#define	EINTRN	-65L	/*	Internal error	*/
#define	EPLFMT	-66L	/*	Invalid program load	*/
#define	ENOEXEC			EPLFMT	/*	Invalid program load format	*/
#define	EGSBF		-67L	/*	Memory block growth	*/
#define	EBREAK	-68L	/* user break (^C)	*/
#define	EXCPT		-69L	/* 68000- exception ("bombs")	*/
#define	EPTHOV	-70L	/* path overflow	*/
#define	ELOOP		-80L	/* too many symlinks in path	*/
