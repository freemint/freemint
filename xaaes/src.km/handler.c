/*
 * $Id$
 *
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

#include "c_window.h"
#include "k_main.h"
#include "k_mouse.h"

#include "mint/signal.h"


/*
 * initialize trap handler table
 * 
 * lockscreen flag for AES functions that are writing the screen
 * and are supposed to be locking
 */

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

struct xa_ftab
{
	AES_function *f;	/* function pointer */
	bool lockscreen;	/* if true syscall is enclosed with lock/unclock_screen */
#if GENERATE_DIAGS
	const char *descr;
#define DESCR(x) x
#else
#define DESCR(x)
#endif
};

/* The main AES kernel command jump table */
static struct xa_ftab aes_tab[220] =
{
	/*   0 */ { NULL,                      false, DESCR(NULL)              },
	/*   1 */ { NULL,                      false, DESCR(NULL)              },
	/*   2 */ { NULL,                      false, DESCR(NULL)              },
	/*   3 */ { NULL,                      false, DESCR(NULL)              },
	/*   4 */ { NULL,                      false, DESCR(NULL)              },
	/*   5 */ { NULL,                      false, DESCR(NULL)              },
	/*   6 */ { NULL,                      false, DESCR(NULL)              },
	/*   7 */ { NULL,                      false, DESCR(NULL)              },
	/*   8 */ { NULL,                      false, DESCR(NULL)              },
	/*   9 */ { NULL,                      false, DESCR(NULL)              },

	/*  10 */ { XA_appl_init,              false, DESCR("appl_init")       },
	/*  11 */ { NULL,                      false, DESCR("appl_read")       }, // unimplemented
	/*  12 */ { XA_appl_write,             false, DESCR("appl_write")      },
	/*  13 */ { XA_appl_find,              false, DESCR("appl_find")       },
	/*  14 */ { NULL,                      false, DESCR("appl_tplay")      }, // unimplemented
	/*  15 */ { NULL,                      false, DESCR("appl_trecord")    }, // unimplemented
	/*  16 */ { NULL,                      false, DESCR(NULL)              },
	/*  17 */ { XA_appl_yield,             false, DESCR("appl_yield")      },
	/*  18 */ { XA_appl_search,            false, DESCR("appl_search")     },
	/*  19 */ { XA_appl_exit,              false, DESCR("appl_exit")       },

	/*  20 */ { XA_evnt_keybd,             false, DESCR("evnt_keybd")      },
	/*  21 */ { XA_evnt_button,            false, DESCR("evnt_button")     },
	/*  22 */ { XA_evnt_mouse,             false, DESCR("evnt_mouse")      },
	/*  23 */ { XA_evnt_mesag,             false, DESCR("evnt_mesag")      },
	/*  24 */ { XA_evnt_timer,             false, DESCR("evnt_timer")      },
	/*  25 */ { XA_evnt_multi,             false, DESCR("evnt_multi")      },
	/*  26 */ { NULL,                      false, DESCR("evnt_dclick")     },
	/*  27 */ { NULL,                      false, DESCR(NULL)              },
	/*  28 */ { NULL,                      false, DESCR(NULL)              },
	/*  29 */ { NULL,                      false, DESCR(NULL)              },

	/*  30 */ { XA_menu_bar,               false, DESCR("menu_bar")        },
	/*  31 */ { XA_menu_icheck,            false, DESCR("menu_icheck")     },
	/*  32 */ { XA_menu_ienable,           false, DESCR("menu_ienable")    },
	/*  33 */ { XA_menu_tnormal,           false, DESCR("menu_tnormal")    },
	/*  34 */ { XA_menu_text,              false, DESCR("menu_text")       },
	/*  35 */ { XA_menu_register,          false, DESCR("menu_register")   },
	/*  36 */ { XA_menu_popup,             false, DESCR("menu_popup")      },
	/*  37 */ { XA_menu_attach,            false, DESCR("menu_attach")     },
	/*  38 */ { XA_menu_istart,            false, DESCR("menu_istart")     },
	/*  39 */ { XA_menu_settings,          false, DESCR("menu_settings")   },

	/*  40 */ { XA_objc_add,               false, DESCR("objc_add")        },
	/*  41 */ { XA_objc_delete,            false, DESCR("objc_delete")     },
	/*  42 */ { XA_objc_draw,              false, DESCR("objc_draw")       },
	/*  43 */ { XA_objc_find,              false, DESCR("objc_find")       },
	/*  44 */ { XA_objc_offset,            false, DESCR("objc_offset")     },
	/*  45 */ { XA_objc_order,             false, DESCR("objc_order")      },
	/*  46 */ { XA_objc_edit,              false, DESCR("objc_edit")       },
	/*  47 */ { XA_objc_change,            false, DESCR("objc_change")     },
	/*  48 */ { XA_objc_sysvar,            false, DESCR("objc_sysvar")     },
	/*  49 */ { NULL,                      false, DESCR(NULL)              },

	/*  50 */ { XA_form_do,                false, DESCR("form_do")         },
	/*  51 */ { XA_form_dial,              false, DESCR("form_dial")       },
	/*  52 */ { XA_form_alert,             false, DESCR("form_alert")      },
	/*  53 */ { XA_form_error,             false, DESCR("form_error")      },
	/*  54 */ { XA_form_center,            false, DESCR("form_center")     },
	/*  55 */ { XA_form_keybd,             false, DESCR("form_keybd")      },
	/*  56 */ { XA_form_button,            false, DESCR("form_button")     },
	/*  57 */ { NULL,                      false, DESCR(NULL)              },
	/*  58 */ { NULL,                      false, DESCR(NULL)              },
	/*  59 */ { NULL,                      false, DESCR(NULL)              },

	/*  60 */ { NULL,                      false, DESCR("objc_wdraw")      }, // MagiC 5.10 extension
	/*  61 */ { NULL,                      false, DESCR("objc_wchange")    }, // MagiC 5.10 extension
	/*  62 */ { NULL,                      false, DESCR("graf_wwatchbox")  }, // MagiC 5.10 extension
	/*  63 */ { NULL,                      false, DESCR("objc_wbutton")    }, // MagiC 5.10 extension
	/*  64 */ { NULL,                      false, DESCR("objc_wkeybd")     }, // MagiC 5.10 extension
	/*  65 */ { NULL,                      false, DESCR("objc_wedit")      }, // MagiC 5.10 extension
	/*  66 */ { NULL,                      false, DESCR(NULL)              },
	/*  67 */ { NULL,                      false, DESCR(NULL)              },
	/*  68 */ { NULL,                      false, DESCR(NULL)              },
	/*  69 */ { NULL,                      false, DESCR(NULL)              },

	/*  70 */ { XA_graf_rubberbox,         false, DESCR("graf_rubberbox")  },
	/*  71 */ { XA_graf_dragbox,           false, DESCR("graf_dragbox")    },
	/*  72 */ { NULL,                      false, DESCR("graf_movebox")    }, // unimplemented
	/*  73 */ { NULL,                      false, DESCR("graf_growbox")    }, // unimplemented
	/*  74 */ { NULL,                      false, DESCR("graf_shrinkbox")  }, // unimplemented
	/*  75 */ { XA_graf_watchbox,          false, DESCR("graf_watchbox")   },
	/*  76 */ { XA_graf_slidebox,          false, DESCR("graf_slidebox")   },
	/*  77 */ { XA_graf_handle,            false, DESCR("graf_hand")       },
	/*  78 */ { XA_graf_mouse,             false, DESCR("graf_mouse")      },
	/*  79 */ { XA_graf_mkstate,           false, DESCR("graf_mkstate")    },

	/*  80 */ { XA_scrp_read,              false, DESCR("scrp_read")       },
	/*  81 */ { XA_scrp_write,             false, DESCR("scrp_write")      },
	/*  82 */ { NULL,                      false, DESCR(NULL)              },
	/*  83 */ { NULL,                      false, DESCR(NULL)              },
	/*  84 */ { NULL,                      false, DESCR(NULL)              },
	/*  85 */ { NULL,                      false, DESCR(NULL)              },
	/*  86 */ { NULL,                      false, DESCR(NULL)              },
	/*  87 */ { NULL,                      false, DESCR(NULL)              },
	/*  88 */ { NULL,                      false, DESCR(NULL)              },
	/*  89 */ { NULL,                      false, DESCR(NULL)              },

	/*  90 */ { XA_fsel_input,             false, DESCR("fsel_input")      },
	/*  91 */ { XA_fsel_exinput,           false, DESCR("fsel_exinput")    },
	/*  92 */ { NULL,                      false, DESCR(NULL)              },
	/*  93 */ { NULL,                      false, DESCR(NULL)              },
	/*  94 */ { NULL,                      false, DESCR(NULL)              },
	/*  95 */ { NULL,                      false, DESCR(NULL)              },
	/*  96 */ { NULL,                      false, DESCR(NULL)              },
	/*  97 */ { NULL,                      false, DESCR(NULL)              },
	/*  98 */ { NULL,                      false, DESCR(NULL)              },
	/*  99 */ { NULL,                      false, DESCR(NULL)              },

	/* 100 */ { XA_wind_create,            false, DESCR("wind_create")     },
	/* 101 */ { XA_wind_open,              false, DESCR("wind_open")       },
	/* 102 */ { XA_wind_close,             false, DESCR("wind_close")      },
	/* 103 */ { XA_wind_delete,            false, DESCR("wind_delete")     },
	/* 104 */ { XA_wind_get,               false, DESCR("wind_get")        },
	/* 105 */ { XA_wind_set,               false, DESCR("wind_set")        },
	/* 106 */ { XA_wind_find,              false, DESCR("wind_find")       },
	/* 107 */ { XA_wind_update,            false, DESCR("wind_update")     },
	/* 108 */ { XA_wind_calc,              false, DESCR("wind_calc")       },
	/* 109 */ { XA_wind_new,               false, DESCR("wind_new")        },

	/* 110 */ { XA_rsrc_load,              false, DESCR("rsrc_load")       },
	/* 111 */ { XA_rsrc_free,              false, DESCR("rsrc_free")       },
	/* 112 */ { XA_rsrc_gaddr,             false, DESCR("rsrc_gaddr")      },
	/* 113 */ { NULL,                      false, DESCR("rsrc_saddr")      }, // unimplemented
	/* 114 */ { XA_rsrc_obfix,             false, DESCR("rsrc_obfix")      },
	/* 115 */ { XA_rsrc_rcfix,             false, DESCR("rsrc_rcfix")      },
	/* 116 */ { NULL,                      false, DESCR(NULL)              },
	/* 117 */ { NULL,                      false, DESCR(NULL)              },
	/* 118 */ { NULL,                      false, DESCR(NULL)              },
	/* 119 */ { NULL,                      false, DESCR(NULL)              },

	/* 120 */ { XA_shell_read,             false, DESCR("shell_read")      },
	/* 121 */ { XA_shell_write,            false, DESCR("shell_write")     },
	/* 122 */ { NULL,                      false, DESCR("shell_get")       }, // unimplemented
	/* 123 */ { NULL,                      false, DESCR("shell_put")       }, // unimplemented
	/* 124 */ { XA_shell_find,             false, DESCR("shell_find")      },
	/* 125 */ { XA_shell_envrn,            false, DESCR("shell_envrn")     },
	/* 126 */ { NULL,                      false, DESCR(NULL)              },
	/* 127 */ { NULL,                      false, DESCR(NULL)              },
	/* 128 */ { NULL,                      false, DESCR(NULL)              },
	/* 129 */ { XA_appl_control,           false, DESCR("appl_control")    },

	/* 130 */ { XA_appl_getinfo,           false, DESCR("appl_getinfo")    },
	/* 131 */ { NULL,                      false, DESCR(NULL)              },
	/* 132 */ { NULL,                      false, DESCR(NULL)              },
	/* 133 */ { NULL,                      false, DESCR(NULL)              },
	/* 134 */ { NULL,                      false, DESCR(NULL)              },
	/* 135 */ { XA_form_popup,             false, DESCR("form_popup")      },
	/* 136 */ { NULL,                      false, DESCR("form_err")        }, // unimplemented
	/* 137 */ { NULL,                      false, DESCR(NULL)              },
	/* 138 */ { NULL,                      false, DESCR(NULL)              },
	/* 139 */ { NULL,                      false, DESCR(NULL)              },

	/* 140 */ { NULL,                      false, DESCR(NULL)              },
	/* 141 */ { NULL,                      false, DESCR(NULL)              },
	/* 142 */ { NULL,                      false, DESCR(NULL)              },
	/* 143 */ { NULL,                      false, DESCR(NULL)              },
	/* 144 */ { NULL,                      false, DESCR(NULL)              },
	/* 145 */ { NULL,                      false, DESCR(NULL)              },
	/* 146 */ { NULL,                      false, DESCR(NULL)              },
	/* 147 */ { NULL,                      false, DESCR(NULL)              },
	/* 148 */ { NULL,                      false, DESCR(NULL)              },
	/* 149 */ { NULL,                      false, DESCR(NULL)              },

	/* 150 */ { NULL,                      false, DESCR(NULL)              },
	/* 151 */ { NULL,                      false, DESCR(NULL)              },
	/* 152 */ { NULL,                      false, DESCR(NULL)              },
	/* 153 */ { NULL,                      false, DESCR(NULL)              },
	/* 154 */ { NULL,                      false, DESCR(NULL)              },
	/* 155 */ { NULL,                      false, DESCR(NULL)              },
	/* 156 */ { NULL,                      false, DESCR(NULL)              },
	/* 157 */ { NULL,                      false, DESCR(NULL)              },
	/* 158 */ { NULL,                      false, DESCR(NULL)              },
	/* 159 */ { NULL,                      false, DESCR(NULL)              },

	/* 160 */ { NULL,                      false, DESCR("wdlg_create")     }, // MagiC extension
	/* 161 */ { NULL,                      false, DESCR("wdlg_open")       }, // MagiC extension
	/* 162 */ { NULL,                      false, DESCR("wdlg_close")      }, // MagiC extension
	/* 163 */ { NULL,                      false, DESCR("wdlg_delete")     }, // MagiC extension
	/* 164 */ { NULL,                      false, DESCR("wdlg_get")        }, // MagiC extension
	/* 165 */ { NULL,                      false, DESCR("wdlg_set")        }, // MagiC extension
	/* 166 */ { NULL,                      false, DESCR("wdlg_event")      }, // MagiC extension
	/* 167 */ { NULL,                      false, DESCR("wdlg_redraw")     }, // MagiC extension
	/* 168 */ { NULL,                      false, DESCR(NULL)              },
	/* 169 */ { NULL,                      false, DESCR(NULL)              },

	/* 170 */ { NULL,                      false, DESCR("lbox_create")     }, // MagiC extension
	/* 171 */ { NULL,                      false, DESCR("lbox_update")     }, // MagiC extension
	/* 172 */ { NULL,                      false, DESCR("lbox_do")         }, // MagiC extension
	/* 173 */ { NULL,                      false, DESCR("lbox_delete")     }, // MagiC extension
	/* 174 */ { NULL,                      false, DESCR("lbox_get")        }, // MagiC extension
	/* 175 */ { NULL,                      false, DESCR("lbox_set")        }, // MagiC extension
	/* 176 */ { NULL,                      false, DESCR(NULL)              },
	/* 177 */ { NULL,                      false, DESCR(NULL)              },
	/* 178 */ { NULL,                      false, DESCR(NULL)              },
	/* 179 */ { NULL,                      false, DESCR(NULL)              },

	/* 180 */ { NULL,                      false, DESCR("fnts_create")     }, // MagiC extension
	/* 181 */ { NULL,                      false, DESCR("fnts_delete")     }, // MagiC extension
	/* 182 */ { NULL,                      false, DESCR("fnts_open")       }, // MagiC extension
	/* 183 */ { NULL,                      false, DESCR("fnts_close")      }, // MagiC extension
	/* 184 */ { NULL,                      false, DESCR("fnts_get")        }, // MagiC extension
	/* 185 */ { NULL,                      false, DESCR("fnts_set")        }, // MagiC extension
	/* 186 */ { NULL,                      false, DESCR("fnts_evnt")       }, // MagiC extension
	/* 187 */ { NULL,                      false, DESCR("fnts_do")         }, // MagiC extension
	/* 188 */ { NULL,                      false, DESCR(NULL)              },
	/* 189 */ { NULL,                      false, DESCR(NULL)              },

	/* 190 */ { NULL,                      false, DESCR("fslx_open")       }, // MagiC extension
	/* 191 */ { NULL,                      false, DESCR("fslx_close")      }, // MagiC extension
	/* 192 */ { NULL,                      false, DESCR("fslx_getnxtfile") }, // MagiC extension
	/* 193 */ { NULL,                      false, DESCR("fslx_evnt")       }, // MagiC extension
	/* 194 */ { NULL,                      false, DESCR("fslx_do")         }, // MagiC extension
	/* 195 */ { NULL,                      false, DESCR("fslx_set_flags")  }, // MagiC extension
	/* 196 */ { NULL,                      false, DESCR(NULL)              },
	/* 197 */ { NULL,                      false, DESCR(NULL)              },
	/* 198 */ { NULL,                      false, DESCR(NULL)              },
	/* 199 */ { NULL,                      false, DESCR(NULL)              },

	/* 200 */ { NULL,                      false, DESCR("pdlg_create")     }, // MagiC extension
	/* 201 */ { NULL,                      false, DESCR("pdlg_delete")     }, // MagiC extension
	/* 202 */ { NULL,                      false, DESCR("pdlg_open")       }, // MagiC extension
	/* 203 */ { NULL,                      false, DESCR("pdlg_close")      }, // MagiC extension
	/* 204 */ { NULL,                      false, DESCR("pdlg_get")        }, // MagiC extension
	/* 205 */ { NULL,                      false, DESCR("pdlg_set")        }, // MagiC extension
	/* 206 */ { NULL,                      false, DESCR("pdlg_evnt")       }, // MagiC extension
	/* 207 */ { NULL,                      false, DESCR("pdlg_do")         }, // MagiC extension
	/* 208 */ { NULL,                      false, DESCR(NULL)              },
	/* 209 */ { NULL,                      false, DESCR(NULL)              },

	/* 210 */ { NULL,                      false, DESCR("edit_create")     }, // MagiC extension
	/* 211 */ { NULL,                      false, DESCR("edit_open")       }, // MagiC extension
	/* 212 */ { NULL,                      false, DESCR("edit_close")      }, // MagiC extension
	/* 213 */ { NULL,                      false, DESCR("edit_delete")     }, // MagiC extension
	/* 214 */ { NULL,                      false, DESCR("edit_cursor")     }, // MagiC extension
	/* 215 */ { NULL,                      false, DESCR("edit_evnt")       }, // MagiC extension
	/* 216 */ { NULL,                      false, DESCR("edit_get")        }, // MagiC extension
	/* 217 */ { NULL,                      false, DESCR("edit_set")        }, // MagiC extension
	/* 218 */ { NULL,                      false, DESCR(NULL)              },
	/* 219 */ { NULL,                      false, DESCR(NULL)              },
};
#define aes_tab_size	(sizeof(aes_tab) / sizeof(aes_tab[0]))

#define XA_APPL_INIT	 10
#define XA_RSRC_LOAD	110
#define XA_SHELL_FIND	124

void
setup_handler_table(void)
{
	/*
	 * auto lock/unlock
	 */

	aes_tab[ 30].lockscreen = true; /* XA_MENU_BAR */
	aes_tab[ 31].lockscreen = true; /* XA_MENU_ICHECK */
	aes_tab[ 32].lockscreen = true; /* XA_MENU_IENABLE */
	aes_tab[ 33].lockscreen = true; /* XA_MENU_TNORMAL */
	aes_tab[ 36].lockscreen = true; /* XA_MENU_POPUP */

	aes_tab[ 50].lockscreen = true; /* XA_FORM_DO */
	aes_tab[ 51].lockscreen = true; /* XA_FORM_DIAL */
	aes_tab[ 52].lockscreen = true; /* XA_FORM_ALERT */
	aes_tab[ 53].lockscreen = true; /* XA_FORM_ERROR */

	aes_tab[ 90].lockscreen = true; /* XA_FSEL_INPUT */
	aes_tab[ 91].lockscreen = true; /* XA_FSEL_EXINPUT */

	aes_tab[101].lockscreen = true; /* XA_WIND_OPEN */
	aes_tab[102].lockscreen = true; /* XA_WIND_CLOSE */
	aes_tab[105].lockscreen = true; /* XA_WIND_SET */
	aes_tab[109].lockscreen = true; /* XA_WIND_NEW */

	aes_tab[135].lockscreen = true; /* XA_FORM_POPUP */


#ifndef WDIALOG_WDLG
#define WDIALOG_WDLG 0
#endif
#if WDIALOG_WDLG
	/*
	 * optional wdlg_? class functions
	 */
	aes_tab[160].f = XA_wdlg_create;
	aes_tab[161].f = XA_wdlg_open;
	aes_tab[162].f = XA_wdlg_close;
	aes_tab[163].f = XA_wdlg_delete;
	aes_tab[164].f = XA_wdlg_get;
	aes_tab[165].f = XA_wdlg_set;
	aes_tab[166].f = XA_wdlg_event;
	aes_tab[167].f = XA_wdlg_redraw;
#endif

#ifndef WDIALOG_LBOX
#define WDIALOG_LBOX 0
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

#ifndef WDIALOG_FNTS
#define WDIALOG_FNTS 0
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

#ifndef WDIALOG_FSLX
#define WDIALOG_FSLX 0
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

#ifndef WDIALOG_PDLG
#define WDIALOG_PDLG 0
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

#ifndef WDIALOG_EDIT
#define WDIALOG_EDIT 0
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

/* timeout callback */
static void
wakeme_timeout(struct proc *p, struct xa_client *client)
{
	client->timeout = NULL;
	wake(IO_Q, (long)client);
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
		DIAGS(("XaAES: No AES Parameter Block (pid %d)\n", p_getpid()));
		raise(SIGSYS);

		return 0;
	}

	cmd = pb->control[0];

	client = lookup_extension(NULL, XAAES_MAGIC);
	if (!client && cmd != XA_APPL_INIT)
	{
		pb->intout[0] = 0;

		DIAGS(("XaAES: AES trap (cmd %i) for non AES process (pid %ld, pb 0x%lx)\n", cmd, p_getpid(), pb));
		raise(SIGILL);

		return 0;
	}

	/* default paths are kept per process by MiNT ??
	 * so we need to get them here when we run under the process id.
	 */
	if (client)
	{
		client->usr_evnt = 0;

#if 0
		if ((!strcmp("  Thing Desktop", client->name)) || (!strcmp("  PORTHOS ", client->name)))
		{
			display("%s opcod %d - %d, %d, %d, %d, %d, %d, %d, %d, %d, %d", client->name, cmd,
			pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3], pb->intin[4],
			pb->intin[5], pb->intin[6], pb->intin[7], pb->intin[8], pb->intin[9] );

			if (cmd == XA_SHELL_WRITE)
				display("\n SW '%s','%s'", pb->addrin[0], pb->addrin[1]);
		}
#endif

		if (   cmd == XA_RSRC_LOAD
		    || cmd == XA_SHELL_FIND)
		{
			client->xdrive = d_getdrv();
			d_getpath(client->xpath, 0);
		}
	}

	/* better check this first */
	if ((cmd >= 0) && (cmd < aes_tab_size))
	{
		AES_function *cmd_routine;
		unsigned long cmd_rtn;

#if 0
		if ((cfg.fsel_cookie || cfg.no_xa_fsel)
		    && (   cmd == XA_FSEL_INPUT
		        || cmd == XA_FSEL_EXINPUT))
		{
			DIAG((D_fsel, client, "Redirected fsel call"));

			/* This causes call via the old vector
			 * see p_handlr.s
			 */
			return -1;
		}
#endif

		DIAG((D_trap, client, "AES trap: %s[%d] made by %s",
			aes_tab[cmd].descr, cmd, client->name));

		cmd_routine = aes_tab[cmd].f;

		/* if opcode is implemented, call it */
		if (cmd_routine)
		{
			/* The root of all locking under client pid.
			 * 
			 * how about this? It means that these
			 * semaphores are not needed and are effectively skipped.
			 */
			enum locks lock = winlist|envstr|pending;

			// XXX ???
			// Ozk: Me thinks this is not necessary... but I'm not sure.
			//vq_mouse(C.vh, &button.b, &button.x, &button.y);
			//vq_key_s(C.vh, &button.ks);

			if (aes_tab[cmd].lockscreen)
				lock_screen(client, 0, NULL, 2);

			/* callout the AES function */
			cmd_rtn = (*cmd_routine)(lock, client, pb);

			if (aes_tab[cmd].lockscreen)
				unlock_screen(client, 2);

			/* execute delayed delete_window */
			if (S.deleted_windows.first)
				do_delayed_delete_window(lock);

			DIAG((D_trap, client, " %s[%d] retuned %ld for %s",
				aes_tab[cmd].descr, cmd, cmd_rtn, client->name));

			switch (cmd_rtn)
			{
				case XAC_DONE:
					break;

				/* block indefinitly */
				case XAC_BLOCK:
				{
					if (!client)
						client = lookup_extension(NULL, XAAES_MAGIC);
					if (client)
					{
						DIAG((D_trap, client, "XA_Hand: Block client %s", client->name));
						Block(client, 1);
						DIAG((D_trap, client, "XA_Hand: Unblocked %s", client->name));
						break;
					}
				}

				/* block with timeout */
				case XAC_TIMER:
				{
					if (!client)
						client = lookup_extension(NULL, XAAES_MAGIC);

					if (client)
					{
						if (client->timer_val)
						{
							client->timeout = addtimeout(client->timer_val,
										     wakeme_timeout);

							if (client->timeout)
								client->timeout->arg = (long)client;
						}

						Block(client, 1);
						
						if (client->timeout)
						{
							canceltimeout(client->timeout);
							client->timeout = NULL;
						}
						else
						{
							/* timeout */

							if (client->waiting_for & XAWAIT_MULTI)
							{
								short *o;

								/* fill out mouse data */

								o = client->waiting_pb->intout;
								if (!(o[0] & MU_BUTTON))
								{
									check_mouse(client, o+3, o+1, o+2);
									vq_key_s(C.vh, o+4);
								}

								o[0] = MU_TIMER;
								o[5] = 0;
								o[6] = 0;
							}
							else
								/* evnt_timer() always returns 1 */
								client->waiting_pb->intout[0] = 1;

							cancel_evnt_multi(client,3);
							DIAG((D_kern, client, "[20]Unblocked timer for %s",
								c_owner(client)));
						}
					}
				}
			}
#if GENERATE_DIAGS
			if (client)
			{
				if (mouse_locked() || update_locked())
				{
					if (mouse_locked() == client)
						DIAG((D_kern, client, "Leaving AES with mouselock %s", client->name));
					if (update_locked() == client)
						DIAG((D_kern, client, "Leaving AES with screenlock %s", client->name));
				}
				else
					DIAG((D_kern, client, "Leaving AES %s", client->name));
			}
#endif
			return 0;
		}
		else
		{
			DIAGS(("Unimplemented AES trap: %s[%d]", aes_tab[cmd].descr, cmd));
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
