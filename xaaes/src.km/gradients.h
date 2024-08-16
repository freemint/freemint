#if WITH_GRADIENTS

struct xa_gradient
{
	struct xa_data_hdr *allocs;
	short wmask, hmask;
	short w, h;
	short method;
	short n_steps;
	short steps[8];
	struct rgb_1000 c[16];
};

#define START 0x1111
#define STOP  0x1112
#define GRAD_INIT 0x00ff

enum {

	OTOP_VSLIDE_GRADIENT = GRAD_INIT,
	OTOP_HSLIDE_GRADIENT,
	OTOP_VSLIDER_GRADIENT,
	OTOP_HSLIDER_GRADIENT,
	UTOP_VSLIDE_GRADIENT,
	UTOP_HSLIDE_GRADIENT,
	UTOP_VSLIDER_GRADIENT,
	UTOP_HSLIDER_GRADIENT,
	OTOP_TITLE_GRADIENT,
	UTOP_TITLE_GRADIENT,
	OTOP_INFO_GRADIENT,
	UTOP_INFO_GRADIENT,
	OTOP_GREY_GRADIENT,
	UTOP_GREY_GRADIENT,

	ALERT_OTOP_TITLE_GRADIENT,
	ALERT_UTOP_TITLE_GRADIENT,
	ALERT_UTOP_GREY_GRADIENT,

	SLIST_OTOP_VSLIDE_GRADIENT,
	SLIST_OTOP_HSLIDE_GRADIENT,
	SLIST_OTOP_VSLIDER_GRADIENT,
	SLIST_OTOP_HSLIDER_GRADIENT,
	SLIST_UTOP_VSLIDE_GRADIENT,
	SLIST_UTOP_HSLIDE_GRADIENT,
	SLIST_UTOP_VSLIDER_GRADIENT,
	SLIST_UTOP_HSLIDER_GRADIENT,
	SLIST_OTOP_TITLE_GRADIENT,
	SLIST_UTOP_TITLE_GRADIENT,
	SLIST_OTOP_INFO_GRADIENT,
	SLIST_UTOP_INFO_GRADIENT,
	SLIST_OTOP_GREY_GRADIENT,
	SLIST_UTOP_GREY_GRADIENT,

	MENU_GRADIENT,
	SEL_TITLE_GRADIENT,
	SEL_POPENT_GRADIENT,
	INDBUTT_GRADIENT,
	SEL_INDBUTT_GRADIENT,
	ACTBUTT_GRADIENT,
	POPBKG_GRADIENT,
	BOX_GRADIENT,
	BOX_GRADIENT2,
	TEXT_GRADIENT
};

/* import from win_draw.c */
extern struct xa_gradient otop_vslide_gradient;
extern struct xa_gradient otop_hslide_gradient;
extern struct xa_gradient otop_vslider_gradient;
extern struct xa_gradient otop_hslider_gradient;
extern struct xa_gradient utop_vslide_gradient;
extern struct xa_gradient utop_hslide_gradient;
extern struct xa_gradient utop_vslider_gradient;
extern struct xa_gradient utop_hslider_gradient;
extern struct xa_gradient otop_title_gradient;
extern struct xa_gradient utop_title_gradient;
extern struct xa_gradient otop_info_gradient;
extern struct xa_gradient utop_info_gradient;
extern struct xa_gradient otop_grey_gradient;
extern struct xa_gradient utop_grey_gradient;

extern struct xa_gradient alert_otop_title_gradient;
extern struct xa_gradient alert_utop_title_gradient;
extern struct xa_gradient alert_utop_grey_gradient;
extern struct xa_gradient slist_otop_vslide_gradient;
extern struct xa_gradient slist_otop_hslide_gradient;
extern struct xa_gradient slist_otop_vslider_gradient;
extern struct xa_gradient slist_otop_hslider_gradient;
extern struct xa_gradient slist_utop_vslide_gradient;
extern struct xa_gradient slist_utop_hslide_gradient;
extern struct xa_gradient slist_utop_vslider_gradient;
extern struct xa_gradient slist_utop_hslider_gradient;
extern struct xa_gradient slist_otop_title_gradient;
extern struct xa_gradient slist_utop_title_gradient;
extern struct xa_gradient slist_otop_info_gradient;
extern struct xa_gradient slist_utop_info_gradient;
extern struct xa_gradient slist_otop_grey_gradient;
extern struct xa_gradient slist_utop_grey_gradient;

#define XA_GRADIENT(g) , g

#else

#define XA_GRADIENT(g)

#endif
