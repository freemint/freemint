/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# include <stdio.h>
# include <stdlib.h>
# include <stddef.h>

# include "mint/mint.h"
# include "mint/ktypes.h"
# include "mint/kerinfo.h"
# include "mint/mem.h"
# include "mint/proc.h"
# include "mint/slb.h"


/*
 * conventions:
 * 
 * C_XXX	offset of XXX in struct CONTEXT
 * P_XXX	offset of XXX in struct PROC
 * SL_XXX	offset of XXX in struct SHARED_LIB
 * SH_XXX	offset of XXX in struct SLB_HEAD
 * KER_XXX	offset of XXX in struct kerninfo
 * DMA_XXX	offset of XXX in struct DMA
 */
struct magics
{
	const char name[28];
	long value;
}
magics [] =
{
	{ "C_PTRACE",		offsetof (CONTEXT, ptrace)		},
	{ "C_SFMT",		offsetof (CONTEXT, sfmt)		},
	{ "C_INTERNAL",		offsetof (CONTEXT, internal)		},
	{ "C_SR",		offsetof (CONTEXT, sr)			},
	{ "C_PC",		offsetof (CONTEXT, pc)			},
	{ "C_FSTATE",		offsetof (CONTEXT, fstate)		},
	{ "C_FREGS",		offsetof (CONTEXT, fregs)		},
	{ "C_FCTRL",		offsetof (CONTEXT, fctrl)		},
	{ "C_USP",		offsetof (CONTEXT, usp)			},
	{ "C_SSP",		offsetof (CONTEXT, ssp)			},
	{ "C_TERM",		offsetof (CONTEXT, term_vec)		},
	{ "C_D0",		offsetof (CONTEXT, regs)		},
	{ "C_D1",		offsetof (CONTEXT, regs) + 4		},
	{ "C_D2",		offsetof (CONTEXT, regs) + 8		},
	{ "C_A0",		offsetof (CONTEXT, regs) + 32		},
	{ "C_A1",		offsetof (CONTEXT, regs) + 36		},
	{ "C_CRP",		offsetof (CONTEXT, crp)			},
	{ "C_TC",		offsetof (CONTEXT, tc)			},
	
	{ "P_CTXT0",		offsetof (PROC, ctxt)			},
	{ "P_EUID",		offsetof (PROC, _euid)	/* XXX */	},
	{ "P_SYSTIME",		offsetof (PROC, systime)		},
	{ "P_USRTIME",		offsetof (PROC, usrtime)		},
	{ "P_PTRACER",		offsetof (PROC, ptracer)		},
	{ "P_SYSCTXT",		offsetof (PROC, ctxt)			},
	{ "P_EXCPC",		offsetof (PROC, exception_pc)		},
	{ "P_EXCSSP",		offsetof (PROC, exception_ssp)		},
	{ "P_EXCADDR",		offsetof (PROC, exception_addr)		},
	{ "P_EXCTBL",		offsetof (PROC, exception_tbl)		},
	{ "P_EXCMMUSR",		offsetof (PROC, exception_mmusr)	},
	{ "P_EXCACCESS",	offsetof (PROC, exception_access)	},
	{ "P_SIGMASK",		offsetof (PROC, p_sigmask)		},
	{ "P_SIGPENDING",	offsetof (PROC, sigpending)		},
	{ "P_INDOS",		offsetof (PROC, in_dos)			},
	{ "P_BASE",		offsetof (PROC, base)			},
	
	{ "SL_HEAD",		offsetof (SHARED_LIB, slb_head)		},
	{ "SL_NAME",		offsetof (SHARED_LIB, slb_name)		},
	
	{ "SH_NAME",		offsetof (SLB_HEAD, slh_name)		},
	{ "SH_VERSION",		offsetof (SLB_HEAD, slh_version)	},
	{ "SH_OPEN",		offsetof (SLB_HEAD, slh_slb_open)	},
	{ "SH_CLOSE",		offsetof (SLB_HEAD, slh_slb_close)	},
	{ "SH_NO_FUNCS",	offsetof (SLB_HEAD, slh_no_funcs)	},
	{ "SH_FUNCTIONS",	offsetof (SLB_HEAD, slh_functions)	},
	
	{ "KER_MAJ_VERSION",	offsetof (struct kerinfo, maj_version)	},
	{ "KER_MIN_VERSION",	offsetof (struct kerinfo, min_version)	},
	{ "KER_VERSION",	offsetof (struct kerinfo, version)	},
	{ "KER_KMALLOC",	offsetof (struct kerinfo, kmalloc)	},
	{ "KER_KFREE",		offsetof (struct kerinfo, kfree)	},
	{ "KER_NAP",		offsetof (struct kerinfo, nap)		},
	{ "KER_SLEEP",		offsetof (struct kerinfo, sleep)	},
	{ "KER_WAKE",		offsetof (struct kerinfo, wake)		},
	{ "KER_WAKESELECT",	offsetof (struct kerinfo, wakeselect)	},
	{ "KER_DENYSHARE",	offsetof (struct kerinfo, denyshare)	},
	{ "KER_DENYLOCK",	offsetof (struct kerinfo, denylock)	},
	{ "KER_ADDTIMEOUT",	offsetof (struct kerinfo, addtimeout)	},
	{ "KER_CANCELTIMEOUT",	offsetof (struct kerinfo, canceltimeout)		},
	{ "KER_ADDROOTTIMEOUT",	offsetof (struct kerinfo, addroottimeout)		},
	{ "KER_CANCELROOTTIMEOUT",		offsetof (struct kerinfo, cancelroottimeout)		},
	{ "KER_IKILL",		offsetof (struct kerinfo, ikill)	},
	{ "KER_IWAKE",		offsetof (struct kerinfo, iwake)	},
	{ "KER_BIO",		offsetof (struct kerinfo, bio)		},
	{ "KER_XTIME",		offsetof (struct kerinfo, xtime)	},
	{ "KER_KILLGROUP",	offsetof (struct kerinfo, killgroup)	},
	{ "KER_DMA",		offsetof (struct kerinfo, dma)		},
	
	{ "DMA_GET_CHANNEL",	offsetof (DMA, get_channel)		},
	{ "DMA_FREE_CHANNEL",	offsetof (DMA, free_channel)		},
	{ "DMA_START",		offsetof (DMA, dma_start)		},
	{ "DMA_END",		offsetof (DMA, dma_end)			},
	{ "DMA_BLOCK",		offsetof (DMA, block)			},
	{ "DMA_DEBLOCK",	offsetof (DMA, deblock)			},
	
	{ "",			0					}
};

static void usage (void);

static void
usage (void)
{
	fprintf (stderr, "Usage: genmagic outputfile\n");
	fflush (stderr);
	exit (2);
}

int
main (int argc, char **argv)
{
	FILE *f;
	int i;
	
	if (argc != 2)
		usage ();
	
	f = fopen (argv [1], "w");
	if (!f)
	{
		perror (argv [1]);
		exit (1);
	}
	
	for (i = 0; *magics[i].name; i++)
	{
		fprintf (f, "%%define %s %ld\n", magics[i].name, magics[i].value);
	}
	
	fclose (f);
	return 0;
}
