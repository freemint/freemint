#ifndef __mint_struct_iorec_h
#define __mint_struct_iorec_h

/* structure used to hold i/o buffers */
typedef struct io_rec
{
	char *bufaddr;
	short buflen;
	volatile short head;
	volatile short tail;
	short low_water;
	short hi_water;
	
} IOREC_T;

#endif
