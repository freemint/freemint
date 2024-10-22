/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for FreeMiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Main AES trap handler routine
 * -----------------------------
 * This module include the main AES trap 2 handler, called from the
 * FreeMiNT kernel trap wrapper.
 *
 */

#include "handler.h"
#include "xa_global.h"

#include "xaaes.h"

#include "c_window.h"
#include "k_main.h"
#include "k_mouse.h"
#include "taskman.h"
#include "mint/signal.h"


/*
 * initialize trap handler table
 *
 * lockscreen flag for AES functions that are writing the screen
 * and are supposed to be locking
 */

#define DBG_CALLS 0

#include "xa_appl.h"
#include "xa_form.h"
#include "xa_fsel.h"
#include "xa_evnt.h"
#include "xa_graf.h"
#include "xa_wind.h"
#include "xa_objc.h"
#include "xa_rsrc.h"
#include "xa_menu.h"
#include "xa_shel.h"
#include "xa_scrp.h"

#include "xa_fnts.h"
#include "xa_fslx.h"
#include "xa_lbox.h"
#include "xa_wdlg.h"
#include "xa_pdlg.h"

struct xa_ftab
{
	AES_function *f;	/* function pointer */
	int flags;
#define DO_LOCKSCREEN	0x1	/* if set syscall is enclosed with lock/unclock_screen */
#define NOCLIENT	0x2	/* if set syscall is callable without appl_init() */
#if DBG_CALLS || GENERATE_DIAGS
	const char *descr;
#define DESCR(x) x
#else
#define DESCR(x)
#endif
};

/* The main AES kernel command jump table */
static struct xa_ftab aes_tab[220] =
{
	/*   0 */ { NULL,                      0,        DESCR(NULL)              },
	/*   1 */ { NULL,                      0,        DESCR(NULL)              },
	/*   2 */ { NULL,                      0,        DESCR(NULL)              },
	/*   3 */ { NULL,                      0,        DESCR(NULL)              },
	/*   4 */ { NULL,                      0,        DESCR(NULL)              },
	/*   5 */ { NULL,                      0,        DESCR(NULL)              },
	/*   6 */ { NULL,                      0,        DESCR(NULL)              },
	/*   7 */ { NULL,                      0,        DESCR(NULL)              },
	/*   8 */ { NULL,                      0,        DESCR(NULL)              },
	/*   9 */ { NULL,                      0,        DESCR(NULL)              },

	/*  10 */ { XA_appl_init,              NOCLIENT, DESCR("appl_init")       },
	/*  11 */ { NULL,                      0,        DESCR("appl_read")       }, /* unimplemented */
	/*  12 */ { XA_appl_write,             0,        DESCR("appl_write")      },
	/*  13 */ { XA_appl_find,              NOCLIENT, DESCR("appl_find")       },
	/*  14 */ { NULL,                      0,        DESCR("appl_tplay")      }, /* unimplemented */
	/*  15 */ { NULL,                      0,        DESCR("appl_trecord")    }, /* unimplemented */
	/*  16 */ { NULL,                      0,        DESCR(NULL)              },
	/*  17 */ { XA_appl_yield,             0,        DESCR("appl_yield")      },
	/*  18 */ { XA_appl_search,            0,        DESCR("appl_search")     },
	/*  19 */ { XA_appl_exit,              0,        DESCR("appl_exit")       },

	/*  20 */ { XA_evnt_keybd,             0,        DESCR("evnt_keybd")      },
	/*  21 */ { XA_evnt_button,            0,        DESCR("evnt_button")     },
	/*  22 */ { XA_evnt_mouse,             0,        DESCR("evnt_mouse")      },
	/*  23 */ { XA_evnt_mesag,             0,        DESCR("evnt_mesag")      },
	/*  24 */ { XA_evnt_timer,             0,        DESCR("evnt_timer")      },
	/*  25 */ { XA_evnt_multi,             0,        DESCR("evnt_multi")      },
	/*  26 */ { XA_evnt_dclick,            0,        DESCR("evnt_dclick")     },
	/*  27 */ { NULL,                      0,        DESCR(NULL)              },
	/*  28 */ { NULL,                      0,        DESCR(NULL)              },
	/*  29 */ { NULL,                      0,        DESCR(NULL)              },

	/*  30 */ { XA_menu_bar,               0,        DESCR("menu_bar")        },
	/*  31 */ { XA_menu_icheck,            0,        DESCR("menu_icheck")     },
	/*  32 */ { XA_menu_ienable,           0,        DESCR("menu_ienable")    },
	/*  33 */ { XA_menu_tnormal,           0,        DESCR("menu_tnormal")    },
	/*  34 */ { XA_menu_text,              0,        DESCR("menu_text")       },
	/*  35 */ { XA_menu_register,          0,        DESCR("menu_register")   },
	/*  36 */ { XA_menu_popup,             0,        DESCR("menu_popup")      },
	/*  37 */ { XA_menu_attach,            0,        DESCR("menu_attach")     },
	/*  38 */ { XA_menu_istart,            0,        DESCR("menu_istart")     },
	/*  39 */ { XA_menu_settings,          0,        DESCR("menu_settings")   },

	/*  40 */ { XA_objc_add,               0,        DESCR("objc_add")        },
	/*  41 */ { XA_objc_delete,            0,        DESCR("objc_delete")     },
	/*  42 */ { XA_objc_draw,              0,        DESCR("objc_draw")       },
	/*  43 */ { XA_objc_find,              0,        DESCR("objc_find")       },
	/*  44 */ { XA_objc_offset,            0,        DESCR("objc_offset")     },
	/*  45 */ { XA_objc_order,             0,        DESCR("objc_order")      },
	/*  46 */ { XA_objc_edit,              0,        DESCR("objc_edit")       },
	/*  47 */ { XA_objc_change,            0,        DESCR("objc_change")     },
	/*  48 */ { XA_objc_sysvar,            0,        DESCR("objc_sysvar")     },
	/*  49 */ { XA_objc_find,              0,        DESCR("objc_xfind")      },

	/*  50 */ { XA_form_do,                0,        DESCR("form_do")         },
	/*  51 */ { XA_form_dial,              0,        DESCR("form_dial")       },
	/*  52 */ { XA_form_alert,             0,        DESCR("form_alert")      },
	/*  53 */ { XA_form_error,             0,        DESCR("form_error")      },
	/*  54 */ { XA_form_center,            0,        DESCR("xa_form_center")     },
	/*  55 */ { XA_form_keybd,             0,        DESCR("form_keybd")      },
	/*  56 */ { XA_form_button,            0,        DESCR("Form_Button")     },
	/*  57 */ { NULL,                      0,        DESCR(NULL)              },
	/*  58 */ { NULL,                      0,        DESCR(NULL)              },
	/*  59 */ { NULL,                      0,        DESCR(NULL)              },

	/*  60 */ { XA_objc_wdraw,             0,        DESCR("objc_wdraw")      }, /* MagiC 5.10 extension */
	/*  61 */ { XA_objc_wchange,           0,        DESCR("objc_wchange")    }, /* MagiC 5.10 extension */
	/*  62 */ { XA_graf_wwatchbox,         0,        DESCR("graf_wwatchbox")  }, /* MagiC 5.10 extension */
	/*  63 */ { XA_form_wbutton,           0,        DESCR("form_wbutton")    }, /* MagiC 5.10 extension */
	/*  64 */ { XA_form_wkeybd,            0,        DESCR("form_wkeybd")     }, /* MagiC 5.10 extension */
	/*  65 */ { XA_objc_wedit,             0,        DESCR("objc_wedit")      }, /* MagiC 5.10 extension */
	/*  66 */ { NULL,                      0,        DESCR(NULL)              },
	/*  67 */ { NULL,                      0,        DESCR(NULL)              },
	/*  68 */ { NULL,                      0,        DESCR(NULL)              },
	/*  69 */ { NULL,                      0,        DESCR("graf_multirubber")}, /* N.AES extension */

	/*  70 */ { XA_graf_rubberbox,         0,        DESCR("graf_rubberbox")  },
	/*  71 */ { XA_graf_dragbox,           0,        DESCR("graf_dragbox")    },
	/*  72 */ { NULL,                      0,        DESCR("graf_movebox")    }, /* unimplemented */
	/*  73 */ { NULL,                      0,        DESCR("graf_growbox")    }, /* unimplemented */
	/*  74 */ { NULL,                      0,        DESCR("graf_shrinkbox")  }, /* unimplemented */
	/*  75 */ { XA_graf_watchbox,          0,        DESCR("graf_watchbox")   },
	/*  76 */ { XA_graf_slidebox,          0,        DESCR("graf_slidebox")   },
	/*  77 */ { XA_graf_handle,            NOCLIENT, DESCR("graf_hand")       },
	/*  78 */ { XA_xa_graf_mouse,          NOCLIENT, DESCR("graf_mouse")      },
	/*  79 */ { XA_graf_mkstate,           NOCLIENT, DESCR("graf_mkstate")    },

	/*  80 */ { XA_scrp_read,              0,        DESCR("scrp_read")       },
	/*  81 */ { XA_scrp_write,             0,        DESCR("scrp_write")      },
	/*  82 */ { NULL,                      0,        DESCR("scrp_clear")      }, /* unimplemented */
	/*  83 */ { NULL,                      0,        DESCR(NULL)              },
	/*  84 */ { NULL,                      0,        DESCR(NULL)              },
	/*  85 */ { NULL,                      0,        DESCR(NULL)              },
	/*  86 */ { NULL,                      0,        DESCR(NULL)              },
	/*  87 */ { NULL,                      0,        DESCR(NULL)              },
	/*  88 */ { NULL,                      0,        DESCR(NULL)              },
	/*  89 */ { NULL,                      0,        DESCR(NULL)              },

	/*  90 */ { XA_fsel_input,             0,        DESCR("fsel_input")      },
	/*  91 */ { XA_fsel_exinput,           0,        DESCR("fsel_exinput")    },
	/*  92 */ { NULL,                      0,        DESCR(NULL)              },
	/*  93 */ { NULL,                      0,        DESCR(NULL)              },
	/*  94 */ { NULL,                      0,        DESCR(NULL)              },
	/*  95 */ { NULL,                      0,        DESCR(NULL)              },
	/*  96 */ { NULL,                      0,        DESCR(NULL)              },
	/*  97 */ { NULL,                      0,        DESCR(NULL)              },
	/*  98 */ { NULL,                      0,        DESCR(NULL)              },
	/*  99 */ { NULL,                      0,        DESCR(NULL)              },

	/* 100 */ { XA_wind_create,            0,        DESCR("wind_create")     },
	/* 101 */ { XA_wind_open,              0,        DESCR("wind_open")       },
	/* 102 */ { XA_wind_close,             0,        DESCR("wind_close")      },
	/* 103 */ { XA_wind_delete,            0,        DESCR("wind_delete")     },
	/* 104 */ { XA_wind_get,               0,        DESCR("wind_get")        },
	/* 105 */ { XA_wind_set,               0,        DESCR("wind_set")        },
	/* 106 */ { XA_wind_find,              0,        DESCR("wind_find")       },
	/* 107 */ { XA_wind_update,            NOCLIENT, DESCR("wind_update")     },
	/* 108 */ { XA_wind_calc,              0,        DESCR("wind_calc")       },
	/* 109 */ { XA_wind_new,               0,        DESCR("wind_new")        },

	/* 110 */ { XA_rsrc_load,              0,        DESCR("rsrc_load")       },
	/* 111 */ { XA_rsrc_free,              NOCLIENT, DESCR("rsrc_free")       },
	/* 112 */ { XA_rsrc_gaddr,             0,        DESCR("rsrc_gaddr")      },
	/* 113 */ { NULL,                      0,        DESCR("rsrc_saddr")      }, /* unimplemented */
	/* 114 */ { XA_rsrc_obfix,             0,        DESCR("rsrc_obfix")      },
	/* 115 */ { XA_rsrc_rcfix,             0,        DESCR("rsrc_rcfix")      },
	/* 116 */ { NULL,                      0,        DESCR(NULL)              },
	/* 117 */ { NULL,                      0,        DESCR(NULL)              },
	/* 118 */ { NULL,                      0,        DESCR(NULL)              },
	/* 119 */ { NULL,                      0,        DESCR(NULL)              },

	/* 120 */ { XA_shel_read,              NOCLIENT, DESCR("shel_read")       },
	/* 121 */ { XA_shel_write,             0,        DESCR("shel_write")      },
	/* 122 */ { NULL,                      0,        DESCR("shel_get")        }, /* not supported */
	/* 123 */ { NULL,                      0,        DESCR("shel_put")        }, /* not supported */
	/* 124 */ { XA_shel_find,              0,        DESCR("shel_find")       },
	/* 125 */ { XA_shel_envrn,             NOCLIENT, DESCR("shel_envrn")      },
	/* 126 */ { NULL,                      0,        DESCR("shel_rdef")       }, /* unimplemented */
	/* 127 */ { NULL,                      0,        DESCR("shel_wdef")       }, /* unimplemented */
	/* 128 */ { NULL,                      0,        DESCR("shel_help")       }, /* unimplemented */
	/* 129 */ { XA_appl_control,           0,        DESCR("appl_control")    },

	/* 130 */ { XA_appl_getinfo,           NOCLIENT, DESCR("appl_getinfo")    },
	/* 131 */ { NULL,                      0,        DESCR("appl_getcicon")   }, /* MyAES only */
	/* 132 */ { NULL,                      0,        DESCR(NULL)              },
	/* 133 */ { NULL,                      0,        DESCR(NULL)              },
	/* 134 */ { NULL,                      0,        DESCR(NULL)              },
	/* 135 */ { XA_form_popup,             0,        DESCR("form_popup")      },
	/* 136 */ { NULL,                      0,        DESCR("form_xerr")       }, /* unimplemented */
	/* 137 */ { XA_appl_options,           0,        DESCR("appl_options")    },
	/* 138 */ { NULL,                      0,        DESCR(NULL)              },
	/* 139 */ { NULL,                      0,        DESCR(NULL)              },
#if WITH_OBJC_DATA
	/* 140 */ { XA_objc_data,              0,        DESCR("objc_data")       },
#else
	/* 140 */ { NULL,                      0,        DESCR(NULL)              },
#endif
	/* 141 */ { NULL,                      0,        DESCR(NULL)              },
	/* 142 */ { NULL,                      0,        DESCR(NULL)              },
	/* 143 */ { NULL,                      0,        DESCR(NULL)              },
	/* 144 */ { NULL,                      0,        DESCR(NULL)              },
	/* 145 */ { NULL,                      0,        DESCR(NULL)              },
	/* 146 */ { NULL,                      0,        DESCR(NULL)              },
	/* 147 */ { NULL,                      0,        DESCR(NULL)              },
	/* 148 */ { NULL,                      0,        DESCR(NULL)              },
	/* 149 */ { NULL,                      0,        DESCR(NULL)              },

	/* 150 */ { NULL,                      0,        DESCR(NULL)              },
	/* 151 */ { NULL,                      0,        DESCR(NULL)              },
	/* 152 */ { NULL,                      0,        DESCR(NULL)              },
	/* 153 */ { NULL,                      0,        DESCR(NULL)              },
	/* 154 */ { NULL,                      0,        DESCR(NULL)              },
	/* 155 */ { NULL,                      0,        DESCR(NULL)              },
	/* 156 */ { NULL,                      0,        DESCR(NULL)              },
	/* 157 */ { NULL,                      0,        DESCR(NULL)              },
	/* 158 */ { NULL,                      0,        DESCR(NULL)              },
	/* 159 */ { NULL,                      0,        DESCR(NULL)              },

	/* 160 */ { NULL,                      0,        DESCR("wdlg_create")     }, /* MagiC extension */
	/* 161 */ { NULL,                      0,        DESCR("wdlg_open")       }, /* MagiC extension */
	/* 162 */ { NULL,                      0,        DESCR("wdlg_close")      }, /* MagiC extension */
	/* 163 */ { NULL,                      0,        DESCR("wdlg_delete")     }, /* MagiC extension */
	/* 164 */ { NULL,                      0,        DESCR("wdlg_get")        }, /* MagiC extension */
	/* 165 */ { NULL,                      0,        DESCR("wdlg_set")        }, /* MagiC extension */
	/* 166 */ { NULL,                      0,        DESCR("wdlg_event")      }, /* MagiC extension */
	/* 167 */ { NULL,                      0,        DESCR("wdlg_redraw")     }, /* MagiC extension */
	/* 168 */ { NULL,                      0,        DESCR(NULL)              },
	/* 169 */ { NULL,                      0,        DESCR(NULL)              },

	/* 170 */ { NULL,                      0,        DESCR("lbox_create")     }, /* MagiC extension */
	/* 171 */ { NULL,                      0,        DESCR("lbox_update")     }, /* MagiC extension */
	/* 172 */ { NULL,                      0,        DESCR("lbox_do")         }, /* MagiC extension */
	/* 173 */ { NULL,                      0,        DESCR("lbox_delete")     }, /* MagiC extension */
	/* 174 */ { NULL,                      0,        DESCR("lbox_get")        }, /* MagiC extension */
	/* 175 */ { NULL,                      0,        DESCR("lbox_set")        }, /* MagiC extension */
	/* 176 */ { NULL,                      0,        DESCR(NULL)              },
	/* 177 */ { NULL,                      0,        DESCR(NULL)              },
	/* 178 */ { NULL,                      0,        DESCR(NULL)              },
	/* 179 */ { NULL,                      0,        DESCR(NULL)              },

	/* 180 */ { NULL,                      0,        DESCR("fnts_create")     }, /* MagiC extension */
	/* 181 */ { NULL,                      0,        DESCR("fnts_delete")     }, /* MagiC extension */
	/* 182 */ { NULL,                      0,        DESCR("fnts_open")       }, /* MagiC extension */
	/* 183 */ { NULL,                      0,        DESCR("fnts_close")      }, /* MagiC extension */
	/* 184 */ { NULL,                      0,        DESCR("fnts_get")        }, /* MagiC extension */
	/* 185 */ { NULL,                      0,        DESCR("fnts_set")        }, /* MagiC extension */
	/* 186 */ { NULL,                      0,        DESCR("fnts_evnt")       }, /* MagiC extension */
	/* 187 */ { NULL,                      0,        DESCR("fnts_do")         }, /* MagiC extension */
	/* 188 */ { NULL,                      0,        DESCR(NULL)              },
	/* 189 */ { NULL,                      0,        DESCR(NULL)              },

	/* 190 */ { NULL,                      0,        DESCR("fslx_open")       }, /* MagiC extension */
	/* 191 */ { NULL,                      0,        DESCR("fslx_close")      }, /* MagiC extension */
	/* 192 */ { NULL,                      0,        DESCR("fslx_getnxtfile") }, /* MagiC extension */
	/* 193 */ { NULL,                      0,        DESCR("fslx_evnt")       }, /* MagiC extension */
	/* 194 */ { NULL,                      0,        DESCR("fslx_do")         }, /* MagiC extension */
	/* 195 */ { NULL,                      0,        DESCR("fslx_set_flags")  }, /* MagiC extension */
	/* 196 */ { NULL,                      0,        DESCR(NULL)              },
	/* 197 */ { NULL,                      0,        DESCR(NULL)              },
	/* 198 */ { NULL,                      0,        DESCR(NULL)              },
	/* 199 */ { NULL,                      0,        DESCR(NULL)              },

	/* 200 */ { NULL,                      0,        DESCR("pdlg_create")     }, /* MagiC extension */
	/* 201 */ { NULL,                      0,        DESCR("pdlg_delete")     }, /* MagiC extension */
	/* 202 */ { NULL,                      0,        DESCR("pdlg_open")       }, /* MagiC extension */
	/* 203 */ { NULL,                      0,        DESCR("pdlg_close")      }, /* MagiC extension */
	/* 204 */ { NULL,                      0,        DESCR("pdlg_get")        }, /* MagiC extension */
	/* 205 */ { NULL,                      0,        DESCR("pdlg_set")        }, /* MagiC extension */
	/* 206 */ { NULL,                      0,        DESCR("pdlg_evnt")       }, /* MagiC extension */
	/* 207 */ { NULL,                      0,        DESCR("pdlg_do")         }, /* MagiC extension */
	/* 208 */ { NULL,                      0,        DESCR(NULL)              },
	/* 209 */ { NULL,                      0,        DESCR(NULL)              },

	/* 210 */ { NULL,                      0,        DESCR("edit_create")     }, /* MagiC extension */
	/* 211 */ { NULL,                      0,        DESCR("edit_open")       }, /* MagiC extension */
	/* 212 */ { NULL,                      0,        DESCR("edit_close")      }, /* MagiC extension */
	/* 213 */ { NULL,                      0,        DESCR("edit_delete")     }, /* MagiC extension */
	/* 214 */ { NULL,                      0,        DESCR("edit_cursor")     }, /* MagiC extension */
	/* 215 */ { NULL,                      0,        DESCR("edit_evnt")       }, /* MagiC extension */
	/* 216 */ { NULL,                      0,        DESCR("edit_get")        }, /* MagiC extension */
	/* 217 */ { NULL,                      0,        DESCR("edit_set")        }, /* MagiC extension */
	/* 218 */ { NULL,                      0,        DESCR(NULL)              },
	/* 219 */ { NULL,                      0,        DESCR(NULL)              },
};
#define aes_tab_size	(sizeof(aes_tab) / sizeof(aes_tab[0]))

#define XA_RSRC_LOAD	110
#define XA_SHELL_FIND	124

void
setup_handler_table(void)
{
	/*
	 * auto lock/unlock
	 */

	aes_tab[ 30].flags |= DO_LOCKSCREEN; /* XA_MENU_BAR */
	aes_tab[ 31].flags |= DO_LOCKSCREEN; /* XA_MENU_ICHECK */
	aes_tab[ 32].flags |= DO_LOCKSCREEN; /* XA_MENU_IENABLE */
	aes_tab[ 33].flags |= DO_LOCKSCREEN; /* XA_MENU_TNORMAL */
	//aes_tab[ 36].flags |= DO_LOCKSCREEN; /* XA_MENU_POPUP */

	//aes_tab[ 50].flags |= DO_LOCKSCREEN; /* XA_FORM_DO */
	aes_tab[ 51].flags |= DO_LOCKSCREEN; /* XA_FORM_DIAL */
	//aes_tab[ 52].flags |= DO_LOCKSCREEN; /* XA_FORM_ALERT */
	//aes_tab[ 53].flags |= DO_LOCKSCREEN; /* XA_FORM_ERROR */

	aes_tab[ 70].flags |= DO_LOCKSCREEN; /* XA_GRAF_RUBBERBOX */
	aes_tab[ 71].flags |= DO_LOCKSCREEN; /* XA_GRAF_DRAGBOX */
	aes_tab[ 75].flags |= DO_LOCKSCREEN; /* XA_GRAF_WATCHBOX */
	aes_tab[ 76].flags |= DO_LOCKSCREEN; /* XA_GRAF_SLIDEBOX */

	//aes_tab[ 90].flags |= DO_LOCKSCREEN; /* XA_FSEL_INPUT */
	//aes_tab[ 91].flags |= DO_LOCKSCREEN; /* XA_FSEL_EXINPUT */

	aes_tab[101].flags |= DO_LOCKSCREEN; /* XA_WIND_OPEN */
	aes_tab[102].flags |= DO_LOCKSCREEN; /* XA_WIND_CLOSE */
	aes_tab[105].flags |= DO_LOCKSCREEN; /* XA_WIND_SET */
	aes_tab[109].flags |= DO_LOCKSCREEN; /* XA_WIND_NEW */

	//aes_tab[135].flags |= DO_LOCKSCREEN; /* XA_FORM_POPUP */


#if WDIALOG_WDLG
	/*
	 * optional wdlg_? class functions
	 */
	aes_tab[160].f		=  XA_wdlg_create;

	aes_tab[161].f		=  XA_wdlg_open;
	aes_tab[161].flags 	|= DO_LOCKSCREEN;

	aes_tab[162].f		=  XA_wdlg_close;
	aes_tab[162].flags	|= DO_LOCKSCREEN;

	aes_tab[163].f		=  XA_wdlg_delete;

	aes_tab[164].f		=  XA_wdlg_get;

	aes_tab[165].f		=  XA_wdlg_set;
	aes_tab[165].flags	|= DO_LOCKSCREEN;

	aes_tab[166].f		=  XA_wdlg_event;

	aes_tab[167].f		=  XA_wdlg_redraw;
	aes_tab[167].flags	|= DO_LOCKSCREEN;
#endif

#if WDIALOG_LBOX
	/*
	 * optional lbox_? class functions
	 */
	aes_tab[170].f = XA_lbox_create;
	aes_tab[171].f = XA_lbox_update;
	aes_tab[172].f = XA_lbox_do;
	aes_tab[173].f = XA_lbox_delete;
	aes_tab[174].f = XA_lbox_get;
	aes_tab[175].f = XA_lbox_set;
#endif

#if WDIALOG_FNTS
	/*
	 * optional fnts_? class functions
	 */
	aes_tab[180].f = XA_fnts_create;
	aes_tab[181].f = XA_fnts_delete;
	aes_tab[182].f = XA_fnts_open;
	aes_tab[183].f = XA_fnts_close;
	aes_tab[184].f = XA_fnts_get;
	aes_tab[185].f = XA_fnts_set;
	aes_tab[186].f = XA_fnts_evnt;
	aes_tab[187].f = XA_fnts_do;
#endif

#if WDIALOG_FSLX
	/*
	 * optional fslx_? class functions
	 */
	aes_tab[190].f = XA_fslx_open;
	aes_tab[191].f = XA_fslx_close;
	aes_tab[192].f = XA_fslx_getnxtfile;
	aes_tab[193].f = XA_fslx_evnt;
	aes_tab[194].f = XA_fslx_do;
	aes_tab[195].f = XA_fslx_set_flags;
#endif

#if WDIALOG_PDLG
	/*
	 * optional pdlg_? class functions
	 */
	aes_tab[200].f = XA_pdlg_create;
	aes_tab[201].f = XA_pdlg_delete;
	aes_tab[202].f = XA_pdlg_open;
	aes_tab[203].f = XA_pdlg_close;
	aes_tab[204].f = XA_pdlg_get;
	aes_tab[205].f = XA_pdlg_set;
	aes_tab[206].f = XA_pdlg_evnt;
	aes_tab[207].f = XA_pdlg_do;
#endif

#if WDIALOG_EDIT
	/*
	 * optional edit_? class functions
	 */
	aes_tab[210].f = XA_edit_create;
	aes_tab[211].f = XA_edit_open;
	aes_tab[212].f = XA_edit_close;
	aes_tab[213].f = XA_edit_delete;
	aes_tab[214].f = XA_edit_cursor;
	aes_tab[215].f = XA_edit_evnt;
	aes_tab[216].f = XA_edit_get;
	aes_tab[217].f = XA_edit_set;
#endif

#if GENERATE_DIAGS
	/*
	 * fill unused numbers with description
	 */
	{
		int i;

		for (i = 0; i < aes_tab_size; i++)
			if (!aes_tab[i].descr)
				aes_tab[i].descr = "unknown";
	}
#endif
}

/* the main AES trap handler */
long _cdecl
XA_handler(void *_pb)
{
	register AESPB *pb = _pb;
	struct xa_client *client;
	short cmd;

	if (!pb)
	{
		DIAGS(("XaAES: No AES Parameter Block (pid %ld)\n", p_getpid()));

#if 0
		/* inform user what's going on */
		ALERT(("XaAES: No AES Parameter Block, killing it", p_getpid()));

		exit_proc(0, get_curproc(), 0);

		raise(SIGKILL);
#else
		/* inform user what's going on */
		ALERT((xa_strings(AL_NOPB)/*"XaAES: No AES Parameter Block, returning"*/, p_getpid()));
#endif
		return 0;
	}
#if GENERATE_DIAGS
	DIAGS((" -- pb=%lx, control=%lx, intin=%lx, intout=%lx, addrin=%lx, addrout=%lx",
		(unsigned long)pb, (unsigned long)pb->control, (unsigned long)pb->intin, (unsigned long)pb->intout, (unsigned long)pb->addrin, (unsigned long)pb->addrout));
#endif

	cmd = pb->control[0];

	if ((cmd >= 0) && (cmd < aes_tab_size))
	{
		AES_function *cmd_routine;
		unsigned long cmd_rtn;
		struct proc *p = get_curproc();

		/* XXX	- ozk:
		 * I dont know how I like this! However, we test force-init/attaching client structure
		 * to a process calling the AES wihtout prior appl_init() call.
		 */
		if ((!(client = lookup_extension(NULL, XAAES_MAGIC)) ) && cmd != 10)	/* 10:XA_appl_init */
		{
#if INSERT_APPL_INIT
			if (!(aes_tab[cmd].flags & NOCLIENT) && !(p->modeflags & M_XA_CLIENT_EXIT ) )
			{
				if( cmd == 19 && (p->p_flag & (P_FLAG_SLB | (P_FLAG_SLB << 8))) )
				{
#ifdef BOOTLOG
					struct proc *pp = pid2proc( p->ppid );
					BLOG((0,"slb %s (called by %s) did appl_exit (ignored)!", p->name, pp ? pp->name : ""));
#endif
					return 0;
				}
				client = init_client(0, false);
				if (client)
				{
					add_to_tasklist(client);
					client->forced_init_client = true;
				}
				BLOG((0,"non-AES-process '%s' (%d:(%s)) did AES-call %d (%sfixed),globl_ptr=%lx,status=%lx,modeflags=%x",
					p->name,
					p->pid,
					C.SingleTaskPid == p->pid ? "SingleTask" : "",
					cmd,
					client ? "": "not ",
					(unsigned long)client->globl_ptr,
					client->status,
					p->modeflags ));
				if( !client )
				{
					ALERT(("XaAES: non-AES process issued AES system call %i, ignored", cmd));
					return 1;
				}
			}
#endif
		}
		/*
		 * If process has not called appl_init() yet, it has restricted
		 * access to AES functions
		 */
		if (!client && !(aes_tab[cmd].flags & NOCLIENT))
		{
			pb->intout[0] = 0;

			DIAGS(("XaAES: AES trap (cmd %i) for non AES process (pid %ld, pb 0x%lx)\n",
				cmd, p_getpid(), (unsigned long)pb));

			/* inform user what's going on */
			if( !(p->p_flag & (P_FLAG_SLB | (P_FLAG_SLB << 8))) )
			{
				ALERT((xa_strings(AL_NOAESPR)/*"XaAES: non-AES process issued AES system call %i, killing it"*/, cmd));
				exit_proc(0, get_curproc(), 0);
				raise(SIGKILL);
			}
			return 0;
		}
		/*
		 * default paths are kept per process by MiNT ??
		 * so we need to get them here when we run under the process id.
		 */
		if (client)
		{
			client->usr_evnt = 0;

			if (   cmd == XA_RSRC_LOAD
			    || cmd == XA_SHELL_FIND)
			{
				client->xdrive = d_getdrv();
				d_getpath(client->xpath, 0);
			}

			while (client->irdrw_msg)
			{
				exec_iredraw_queue(0, client);
			}
		}
#if GENERATE_DIAGS
		if (client)
		{
			DIAG((D_trap, client, "AES trap[enter=%d]: %s[%d] made by %s",
				client->enter, aes_tab[cmd].descr, cmd, client->name));
		}
		else
		{
			DIAG((D_trap, NULL, "AES trap: %s[%d] made by non AES process (pid %ld)",
				aes_tab[cmd].descr, cmd, p_getpid()));
		}
#endif
#if DBG_CALLS
		/* 25:XA_evnt_multi */
		BLOGif(cmd!=25,(0, "%s[%d:%x] made by %lx:%s(%s:%d),status=%lx",	aes_tab[cmd].descr, cmd, aes_tab[cmd].flags, client, client ? client->name : "", get_curproc()->name, get_curproc()->pid, client->status));
#endif
		cmd_routine = aes_tab[cmd].f;

		/* if opcode is implemented, call it */
		if (cmd_routine)
		{

			/* The root of all locking under client pid.
			 *
			 * how about this? It means that these
			 * semaphores are not needed and are effectively skipped.
			 */
			int lock = LOCK_WINLIST|LOCK_ENVSTR|LOCK_PENDING;
#if GENERATE_DIAGS
			if (client)
				client->enter++;
#endif
			if (aes_tab[cmd].flags & DO_LOCKSCREEN)
				lock_screen(p, false);

			/* callout the AES function */

			/* Ozk:
			 * ATTENTION: DO _NOT_ actually access the client structure after calling
			 * the AES function, because it may be released by the function, appl_exit()
			 * for example! It can be used to check if process _was_ an AES application
			 * until the lookup_extension below.
			 */
			cmd_rtn = (*cmd_routine)(lock, client, pb);

			if (aes_tab[cmd].flags & DO_LOCKSCREEN)
				unlock_screen(p);

			/* execute delayed delete_window */
			if (S.deleted_windows.first)
				do_delayed_delete_window(lock);

#if GENERATE_DIAGS
			if (client)
			{
				DIAG((D_trap, client, " %s[%d] returned %d for %s",
					aes_tab[cmd].descr, cmd, pb->intout[0], p->name));
			}
			else
			{
				DIAG((D_trap, client, " %s[%d] returned %d for non AES process (pid %ld)",
					aes_tab[cmd].descr, cmd, pb->intout[0], p_getpid()));
			}
#endif
#if DBG_CALLS
			BLOGif(cmd!=25,(0, "%s[%d] returned %ld for %s", aes_tab[cmd].descr, cmd, cmd_rtn, p->name));
#endif
			/* Ozk:
			 * Now we check if circumstances under which we check if process started/ended
			 * being a AES client
			 */
			if (!client || cmd == 10 || cmd == 19)	/* XA_appl_init || XA_appl_exit */
			{
				client = lookup_extension(NULL, XAAES_MAGIC);
#if GENERATE_DIAGS
				if (client)
					client->enter++;
#endif
			}
			/* Ozk:
			* From now on we can rely on 'client' again...
			*/
			switch (cmd_rtn)
			{
				case XAC_DONE:
					break;

				/* block indefinitly */
				case XAC_BLOCK:
					if (client)
					{
						DIAG((D_trap, client, "XA_Hand: Block client %s", client->name));
						(*client->block)(client);
						DIAG((D_trap, client, "XA_Hand: Unblocked %s", client->name));
					}
					break;
				default:
					BLOG((0,"Xa_handler:%s:unknown cmd_rtn:%ld", client ? client->name : "-", cmd_rtn));
			}
#if GENERATE_DIAGS
			if (client)
			{
				if (mouse_locked() || update_locked())
				{
					if (mouse_locked() == client->p)
					{
						DIAG((D_kern, client, "Leaving AES with mouselock %s", client->name));
					}
					if (update_locked() == client->p)
					{
						DIAG((D_kern, client, "Leaving AES with screenlock %s", client->name));
					}
				}
				else
					DIAG((D_kern, client, "Leaving AES %s", client->name));

				client->enter--;
			}
			else
				DIAG((D_kern, NULL, "Leaving AES non AES process (pid %ld)", p_getpid()));

#endif
			return 0;
		}
		else
		{
			DIAGS(("Unimplemented AES trap: %s[%d]", aes_tab[cmd].descr, cmd));
		}
		if (client)
		{
			while (client->irdrw_msg)
			{
				exec_iredraw_queue(0, client);
			}
		}
	}
	else
	{
		DIAGS(("Unimplemented AES trap: %d", cmd));
	}
	/* error exit */
	pb->intout[0] = 0;
	return 0;
}
