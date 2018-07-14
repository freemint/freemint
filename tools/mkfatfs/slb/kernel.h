/* Prototypes for kernel.slb functions */

# ifndef _kernel_slb_h
# define _kernel_slb_h

# include <mint/basepage.h>
# include <mint/slb.h>

/*
 * The "nwords" argument should actually only be a "short".
 * MagiC will expect it that way, with the actual arguments
 * following.
 * However, a "short" in the actual function definition
 * will be treated as promoted to int.
 * So we pass a long instead, with the upper half
 * set to 1 + nwords to account for the extra space.
 * This also has the benefit of keeping the stack longword aligned.
 */
#undef SLB_NWORDS
#define SLB_NWORDS(_nargs) ((((long)(_nargs) * 2 + 1l) << 16) | (long)(_nargs))
#undef SLB_NARGS
#define SLB_NARGS(_nargs) SLB_NWORDS(_nargs)

/*
 * this function must be provided by the application
 * using kernel32.slb
 */
extern SLB *get_kern_p(void);

#ifndef _CDECL
#  define _CDECL
#endif

static inline void
dos_pterm0(void)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long))k->exec;

	(*exec)(k->handle, 0L, SLB_NARGS(0));
}

static inline long
dos_fopen(const char *name, short mode)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *, long) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *, long))k->exec;

	return (*exec)(k->handle, 0x003dL, SLB_NARGS(2), name, (long)mode);
}

static inline long
dos_fread(short handle, long size, void *buf)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, long, long, void *) = (long _CDECL (*)(SLB_HANDLE, long, long, long, long, void *))k->exec;

	return (*exec)(k->handle, 0x003fL, SLB_NARGS(3), (long)handle, size, buf);
}

static inline long
dos_fclose(short handle)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long, long))k->exec;

	return (*exec)(k->handle, 0x003eL, SLB_NARGS(1), (long)handle);
}

static inline long
dos_pkill(short pid, short sig)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long, long, long))k->exec;

	return (*exec)(k->handle, 0x0111L, SLB_NARGS(2), (long)pid, (long)sig);
}

static inline const char *
dos_serror(long error)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long, long))k->exec;

	return (const char *)(*exec)(k->handle, 0x0200L, SLB_NARGS(1), error);
}

static inline long
dos_fsize(const char *fname)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *))k->exec;

	return (*exec)(k->handle, 0x0201L, SLB_NARGS(1), fname);
}

static inline long
dos_fexists(const char *fname)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *))k->exec;

	return (*exec)(k->handle, 0x0202L, SLB_NARGS(1), fname);
}

static inline long
dos_fsearch(const char *name, char *fullname, const char *pathvar)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *, char *, const char *) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *, char *, const char *))k->exec;

	return (*exec)(k->handle, 0x0203L, SLB_NARGS(3), name, fullname, pathvar);
}

static inline BASEPAGE *
dos_pbase(void)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long))k->exec;

	return (BASEPAGE *)(*exec)(k->handle, 0x0204L, SLB_NARGS(0));
}

static inline long
dos_fload(const char *fname, char **buf, long *size, short *mode)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *, char **, long *, short *) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *, char **, long *, short *))k->exec;

	return (*exec)(k->handle, 0x0205L, SLB_NARGS(4), fname, buf, size, mode);
}

static inline long
dos_fsave(const char *fname, const void *buf, long size, short mode)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *, const void *, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *, const void *, long, long))k->exec;

	return (*exec)(k->handle, 0x0206L, SLB_NARGS(4), fname, buf, size, mode);
}

static inline long
dos_finfdir(char *buf, long blen)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, char *, long) = (long _CDECL (*)(SLB_HANDLE, long, long, char *, long))k->exec;

	return (*exec)(k->handle, 0x0207L, SLB_NARGS(2), buf, blen);
}

static inline char *
dos_getenv(const char *var)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *))k->exec;

	return (char *)(*exec)(k->handle, 0x0208L, SLB_NARGS(1), var);
}

static inline void
dos_setenv(const char *var, const char *value)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *, const char *) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *, const char *))k->exec;

	(*exec)(k->handle, 0x0209L, SLB_NARGS(2), var, value);
}

static inline void
dos_delenv(const char *var)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *))k->exec;

	(*exec)(k->handle, 0x020aL, SLB_NARGS(1), var);
}

static inline long
dos_floadbuf(const char *fname, char *buf, long size, short *mode)
{
	SLB *k = get_kern_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *, char *, long, short *) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *, char *, long, short *))k->exec;

	return (*exec)(k->handle, 0x020bL, SLB_NARGS(4), fname, buf, size, mode);
}

#undef SLB_NWORDS
#undef SLB_NARGS

# endif

/* EOF */
