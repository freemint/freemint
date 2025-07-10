#ifndef __mint_struct_kbdvbase_h
#define __mint_struct_kbdvbase_h

struct kbdvbase
{
	long	midivec;
	long	vkbderr;
	long	vmiderr;
	long	statvec;
	long	mousevec;
	long	clockvec;
	long	joyvec;
	long	midisys;
	long	ikbdsys;
	short	drvstat;	/* Non-zero if a packet is currently transmitted. */
};
typedef struct kbdvbase KBDVEC;

#endif
