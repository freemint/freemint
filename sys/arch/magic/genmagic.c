/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# include <stddef.h>
# include <mint/mintbind.h>

# define __KERNEL__

# include "mint/mint.h"
# include "mint/ktypes.h"
# include "mint/kerinfo.h"
# include "mint/mem.h"
# include "mint/proc.h"
# include "mint/slb.h"

# include "libkern/libkern.h"


/*
 * conventions:
 *
 * B_XXX	offset of XXX in struct basepage
 * C_XXX	offset of XXX in struct context
 * P_XXX	offset of XXX in struct proc
 * SL_XXX	offset of XXX in struct shared_lib
 * SH_XXX	offset of XXX in struct slb_head
 * KER_XXX	offset of XXX in struct kerninfo
 * DMA_XXX	offset of XXX in struct dma
 * TM_XXX	offset of XXX in struct timeout
 */
struct magics
{
	const char name[28];
	long value;
}
magics [] =
{
	{ "B_LOWTPA",		offsetof(struct basepage, p_lowtpa)		},
	{ "B_HITPA",		offsetof(struct basepage, p_hitpa)		},
	{ "B_TBASE",		offsetof(struct basepage, p_tbase)		},
	{ "B_TLEN",		offsetof(struct basepage, p_tlen)		},
	{ "B_DBASE",		offsetof(struct basepage, p_dbase)		},
	{ "B_DLEN",		offsetof(struct basepage, p_dlen)		},
	{ "B_BBASE",		offsetof(struct basepage, p_bbase)		},
	{ "B_BLEN",		offsetof(struct basepage, p_blen)		},
	{ "B_DTA",		offsetof(struct basepage, p_dta)		},
	{ "B_PARENT",		offsetof(struct basepage, p_parent)		},
	{ "B_FLAGS",		offsetof(struct basepage, p_flags)		},
	{ "B_ENV", 		offsetof(struct basepage, p_env)		},
	{ "B_CMDLIN",		offsetof(struct basepage, p_cmdlin)		},

	{ "C_PTRACE",		offsetof(struct context, ptrace)		},
	{ "C_SFMT",		offsetof(struct context, sfmt)			},
	{ "C_INTERNAL",		offsetof(struct context, internal)		},
	{ "C_SR",		offsetof(struct context, sr)			},
	{ "C_PC",		offsetof(struct context, pc)			},
	{ "C_FSTATE",		offsetof(struct context, fstate)		},
	{ "C_FREGS",		offsetof(struct context, fregs)			},
	{ "C_FCTRL",		offsetof(struct context, fctrl)			},
	{ "C_USP",		offsetof(struct context, usp)			},
	{ "C_SSP",		offsetof(struct context, ssp)			},
	{ "C_TERM",		offsetof(struct context, term_vec)		},
	{ "C_D0",		offsetof(struct context, regs)			},
	{ "C_D1",		offsetof(struct context, regs) + 4		},
	{ "C_D2",		offsetof(struct context, regs) + 8		},
	{ "C_A0",		offsetof(struct context, regs) + 32		},
	{ "C_A1",		offsetof(struct context, regs) + 36		},
	{ "C_CRP",		offsetof(struct context, crp)			},
	{ "C_TC",		offsetof(struct context, tc)			},

	{ "P_SYSCTXT",		offsetof(struct proc, ctxt)			},
	{ "P_EUID",		offsetof(struct proc, _euid)	/* XXX */	},
	{ "P_SYSTIME",		offsetof(struct proc, systime)			},
	{ "P_USRTIME",		offsetof(struct proc, usrtime)			},
	{ "P_PTRACER",		offsetof(struct proc, ptracer)			},
	{ "P_PTRACEFLAGS",	offsetof(struct proc, ptraceflags)		},
	{ "P_EXCPC",		offsetof(struct proc, exception_pc)		},
	{ "P_EXCSSP",		offsetof(struct proc, exception_ssp)		},
	{ "P_EXCADDR",		offsetof(struct proc, exception_addr)		},
	{ "P_EXCTBL",		offsetof(struct proc, exception_tbl)		},
	{ "P_EXCMMUSR",		offsetof(struct proc, exception_mmusr)		},
	{ "P_EXCACCESS",	offsetof(struct proc, exception_access)		},
	{ "P_SIGMASK",		offsetof(struct proc, p_sigmask)		},
	{ "P_SIGPENDING",	offsetof(struct proc, sigpending)		},
	{ "P_INDOS",		offsetof(struct proc, in_dos)			},
	{ "P_INVDI",		offsetof(struct proc, in_vdi)			},

	{ "SL_HEAD",		offsetof(struct shared_lib, slb_head)		},
	{ "SL_NAME",		offsetof(struct shared_lib, slb_name)		},

	{ "SH_NAME",		offsetof(struct slb_head, slh_name)		},
	{ "SH_VERSION",		offsetof(struct slb_head, slh_version)		},
	{ "SH_INIT",		offsetof(struct slb_head, slh_slb_init)		},
	{ "SH_EXIT",		offsetof(struct slb_head, slh_slb_exit)		},
	{ "SH_OPEN",		offsetof(struct slb_head, slh_slb_open)		},
	{ "SH_CLOSE",		offsetof(struct slb_head, slh_slb_close)	},
	{ "SH_NO_FUNCS",	offsetof(struct slb_head, slh_no_funcs)		},
	{ "SH_FUNCTIONS",	offsetof(struct slb_head, slh_functions)	},

	{ "KER_MAJ_VERSION",	offsetof(struct kerinfo, maj_version)		},
	{ "KER_MIN_VERSION",	offsetof(struct kerinfo, min_version)		},
	{ "KER_VERSION",	offsetof(struct kerinfo, version)		},
	{ "KER_KMALLOC",	offsetof(struct kerinfo, kmalloc)		},
	{ "KER_KFREE",		offsetof(struct kerinfo, kfree)			},
	{ "KER_NAP",		offsetof(struct kerinfo, nap)			},
	{ "KER_SLEEP",		offsetof(struct kerinfo, sleep)			},
	{ "KER_WAKE",		offsetof(struct kerinfo, wake)			},
	{ "KER_WAKESELECT",	offsetof(struct kerinfo, wakeselect)		},
	{ "KER_DENYSHARE",	offsetof(struct kerinfo, denyshare)		},
	{ "KER_DENYLOCK",	offsetof(struct kerinfo, denylock)		},
	{ "KER_ADDTIMEOUT",	offsetof(struct kerinfo, addtimeout)		},
	{ "KER_CANCELTIMEOUT",	offsetof(struct kerinfo, canceltimeout)		},
	{ "KER_ADDROOTTIMEOUT",	offsetof(struct kerinfo, addroottimeout)	},
	{ "KER_CANCELROOTTIMEOUT", offsetof(struct kerinfo, cancelroottimeout)	},
	{ "KER_IKILL",		offsetof(struct kerinfo, ikill)			},
	{ "KER_IWAKE",		offsetof(struct kerinfo, iwake)			},
	{ "KER_BIO",		offsetof(struct kerinfo, bio)			},
	{ "KER_XTIME",		offsetof(struct kerinfo, xtime)			},
	{ "KER_KILLGROUP",	offsetof(struct kerinfo, killgroup)		},
	{ "KER_DMA",		offsetof(struct kerinfo, dma)			},

	{ "DMA_GET_CHANNEL",	offsetof(struct dma, get_channel)		},
	{ "DMA_FREE_CHANNEL",	offsetof(struct dma, free_channel)		},
	{ "DMA_START",		offsetof(struct dma, dma_start)			},
	{ "DMA_END",		offsetof(struct dma, dma_end)			},
	{ "DMA_BLOCK",		offsetof(struct dma, block)			},
	{ "DMA_DEBLOCK",	offsetof(struct dma, deblock)			},

	{ "TM_NEXT",		offsetof(struct timeout, next)			},
	{ "TM_struct proc",	offsetof(struct timeout, proc)			},
	{ "TM_WHEN",		offsetof(struct timeout, when)			},
	{ "TM_FUNC",		offsetof(struct timeout, func)			},
	{ "TM_FLAGS",		offsetof(struct timeout, flags)			},
	{ "TM_ARG",		offsetof(struct timeout, arg)			},
 
	{ "",			0						}
};

char *outfile = "magic.i";

/* referenced from startup.S */
void small_main(void);

/*
 * be careful with standard library functions here,
 * this is compiled with -mshort just like the kernel,
 * but we cannot use mintlib
 */
static char *mystrcpy(char *dst, const char *src)
{
	while ((*dst++ = *src++) != '\0')
		;
	return dst - 1;
}

static char *myltoa(char *dst, long value)
{
	char *p;
	int neg = 0;
	char tmpbuf[8 * sizeof(long) + 2];
	short i = 0;

	if (value < 0)
	{
		neg = 1;
		value = -value;
	}
	do {
		tmpbuf[i++] = "0123456789"[value % 10];
	} while ((value /= 10) != 0);

	if (neg)
		tmpbuf[i++] = '-';
	p = dst;
	while (--i >= 0)	/* reverse it back  */
		*p++ = tmpbuf[i];
	*p = '\0';

	return p;
}

void
small_main(void)
{
	char buf[128];
	long fd;
	int i;
	char *p;

#if 0
	if (argc != 2)
	{
		(void) Cconws("Usage: genmagic outputfile\r\n");

		Pterm(1);
	}
#endif

	fd = Fopen(outfile, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd < 0)
	{
		(void) Cconws("cannot create outfile\r\n");
		Pterm(1);
	} else
	{
	
		for (i = 0; *magics[i].name; i++)
		{
			p = mystrcpy(buf, "#define ");
			p = mystrcpy(p, magics[i].name);
			*p++ = ' ';
			p = myltoa(p, magics[i].value);
			p = mystrcpy(p, "\n");
			Fwrite(fd, p - buf, buf);
		}
	
		Fclose(fd);
	}

	Pterm0();
}
