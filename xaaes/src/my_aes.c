
#include "global.h"
#include "my_aes.h"
#include "xa_types.h"


/* Array sizes in aes control block */
#define AES_CTRLMAX		5
#define AES_GLOBMAX		16
#define AES_INTINMAX 		16
#define AES_INTOUTMAX		16
#define AES_ADDRINMAX		16
#define AES_ADDROUTMAX		16

struct aespb
{
	short       *control;
	short	    *global;
	const short *intin;
	short       *intout;
	const long  *addrin;
	long        *addrout;
};

#if defined(__GNUC_INLINE__) && (__GNUC__ > 2 || __GNUC_MINOR__ > 5)

static inline void
_aes_trap(struct aespb *aespb)
{
	__asm__ volatile ("
		move.l	%0, d1;    | &aespb
		move.w	#200,d0;
		trap	#2;
		"
		:
		: "a"(aespb)
		: "d0", "d1"
	);
}

#else

/** perform AES trap */
extern void aes (struct aespb *pb);

#define _aes_trap(aespb) aes(aespb)

#endif

#define AES_TRAP(aespb, cntrl_0, cntrl_1, cntrl_2, cntrl_3, cntrl_4) \
	my_aes_control[0] = cntrl_0; \
	my_aes_control[1] = cntrl_1; \
	my_aes_control[2] = cntrl_2; \
	my_aes_control[3] = cntrl_3; \
	my_aes_control[4] = cntrl_4; \
	_aes_trap(&aespb)

/*
 * global aes variables, initialized by appl_init
 */
short my_gl_apid;
short my_gl_ap_version;

/*
 * global aes binding params
 */
static short	my_aes_control [AES_CTRLMAX];
static short	my_aes_global  [AES_GLOBMAX];
static short	my_aes_intin   [AES_INTINMAX];
static short	my_aes_intout  [AES_INTOUTMAX];
static long	my_aes_addrin  [AES_ADDRINMAX];
static long	my_aes_addrout [AES_ADDROUTMAX];

static struct aespb my_aes_params =
{
	&my_aes_control[0],
	&my_aes_global[0],
	&my_aes_intin[0],
	&my_aes_intout[0],
	&my_aes_addrin[0],
	&my_aes_addrout[0]
};

short
my_appl_init(void)
{
	/* clear all binding arrays
	 * other binding arrays are synonyms for the stuff
	 */
	bzero(&my_aes_control[0], AES_CTRLMAX * sizeof(short));
	bzero(&my_aes_intin[0], AES_INTINMAX * sizeof(short));
	bzero(&my_aes_intout[0], AES_INTOUTMAX * sizeof(short));
	bzero(&my_aes_addrin[0], AES_ADDRINMAX * sizeof(short));
	bzero(&my_aes_addrout[0], AES_ADDROUTMAX * sizeof(short));
	bzero(&my_aes_global[0], AES_GLOBMAX * sizeof(short));

	AES_TRAP(my_aes_params, 10, 0,1,0,0);

	my_gl_ap_version = my_aes_global[0];
	my_gl_apid       = my_aes_intout[0];

	return my_aes_intout[0];
}

short
my_appl_exit(void)
{
	AES_TRAP(my_aes_params, 19, 0,1,0,0);

	return my_aes_intout[0];
}

short
my_graf_handle(short *Wchar, short *Hchar, short *Wbox, short *Hbox)
{
	AES_TRAP(my_aes_params, 77, 0,5,0,0);

	*Wchar = my_aes_intout[1];
	*Hchar = my_aes_intout[2];
	*Wbox  = my_aes_intout[3];
	*Hbox  = my_aes_intout[4];

	return my_aes_intout[0];
}


/*
 * Returns the object number of this object's parent or -1 if it is the root
 */
int
get_parent(OBJECT *t, int object)
{
	if (object)
	{
		int last;

		do {
			last = object;
			object = t[object].ob_next;
		}
		while(t[object].ob_tail != last);

		return object;
	}

	return -1;
}
