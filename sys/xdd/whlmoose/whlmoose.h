#define MAX_DC_TIME	200
#define SYSTIMER	0x4baL
#define SYSVBI		0x456L
#define SYSNVBI		0x454L

/* Mouse device commands */

#define	MOOSE_INIT_PREFIX	0x4d49	/* 'MI' */
#define	MOOSE_DCLICK_PREFIX	0x4d44	/* 'MD' */
#define MOOSE_BUTTON_PREFIX	0x5842	/* 'XB' */
#define MOOSE_MOVEMENT_PREFIX	0x584d	/* 'XM' */
#define MOOSE_WHEEL_PREFIX	0x5857	/* 'XW' */
#define MOOSE_VECS_PREFIX	0x4d56	/* 'MV' */

struct moose_data
{
	unsigned short l;	/* record length */
	unsigned short ty;	/* button & movement */
	short x;
	short y;
	short state;
	short cstate;
	short clicks;
	short dbg1;
	short dbg2;
};

#define FS_BUFFER_SIZE 32*(sizeof(struct moose_data))

struct mooses_data
{
	short	state;
	short	x;
	short	y;
};

struct moose_init_com
{
	ushort init_prefix;
	void *dum;
};

struct moose_dclick_com
{
	unsigned short dclick_prefix;
	unsigned short dclick_time;
};

typedef short vdi_vec(void *, ...);

struct moose_vecs_com
{
	ushort vecs_prefix;
	vdi_vec *motv;
	vdi_vec *butv;
	vdi_vec *timv;
	vdi_vec *whlv;
};

#define BUT_PAK 	0x42
#define WHL_PAK		0x57

struct mouse_pak
{
	ushort len;
	char ty;
	union
	{
		struct
		{
			uchar state;
			short time;
		}but;
		struct
		{
			uchar wheel;
			short clicks;
		}whl;
		struct
		{
			char state;
		}vbut;
	}t;
	short x;
	short y;
	long dbg;
};

