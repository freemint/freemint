/*
 * stripex.c 
 * strip symbol table, GNU binutils aexec header, and ELF headers from TOS executables.
 * Needed e.g. for SLB shared library files or CPX modules to get the header back
 * to the start of the text segment.
 *
 * BUGS:
 * - relocation table is not checked for validity. If it is corrupted,
 *   this program might crash and/or create corrupted output files.
 *   Should rarely be a problem, since the kernel does not check the table either.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef O_BINARY
#  ifdef _O_BINARY
#    define O_BINARY _O_BINARY
#  else
#    define O_BINARY 0
#  endif
#endif 

#define NEWBUFSIZ	16384L

static unsigned char mybuf[NEWBUFSIZ];
static int verbose;
static int force;

static char tmpname[1024];

#if defined(__atarist__) && defined(__GNUC__)
long _stksize = 30000;
#endif

struct aexec
{
	short a_magic;						/* magic number */
	unsigned long a_text;				/* size of text segment */
	unsigned long a_data;				/* size of initialized data */
	unsigned long a_bss;				/* size of uninitialized data */
	unsigned long a_syms;				/* size of symbol table */
	unsigned long a_res1;				/* see below */
	unsigned long a_AZero2;				/* always zero */
	unsigned short a_isreloc;			/* is reloc info present */
};

#define	CMAGIC	0x601A					/* contiguous text */

/*
 * magic values in a_res1 field
 */
#define EXEC_FMT_TOS  0 /* original TOS */
#define EXEC_FMT_MINT 0x4d694e54UL /* binutils a.out format */
#define EXEC_FMT_ELF  0x454c4600UL /* new binutils PRG+ELF format */


#define SIZEOF_SHORT 2
#define SIZEOF_LONG  4

/*
 * sizeof(struct aexec)
 */
#define SIZEOF_AEXEC (SIZEOF_SHORT + (6 * SIZEOF_LONG) + SIZEOF_SHORT)
/*
 * extra header added by a.out-mintprg format
 */
#define SIZEOF_BINUTILS_AEXEC 228


static char *f_basename(char *name)
{
	char *p1, *p2;

	p1 = strrchr(name, '/');
	p2 = strrchr(name, '\\');
	if (p1 == NULL || p2 > p1)
		p1 = p2;
	if (p1 == NULL)
		p1 = name;
	else
		p1++;
	return p1;
}


static unsigned short read_beword(const unsigned char *p)
{
	return (p[0] << 8) | p[1];
}


static unsigned long read_belong(const unsigned char *p)
{
	return ((unsigned long)read_beword(p) << 16) | read_beword(p + 2);
}


static void write_beword(unsigned char *p, unsigned short v)
{
	p[0] = (v >> 8) & 0xff;
	p[1] = (v     ) & 0xff;
}


static void write_belong(unsigned char *p, unsigned long v)
{
	p[0] = (unsigned char)((v >> 24) & 0xff);
	p[1] = (unsigned char)((v >> 16) & 0xff);
	p[2] = (unsigned char)((v >>  8) & 0xff);
	p[3] = (unsigned char)((v      ) & 0xff);
}


/*
 * read header -- return -1 on error
 */
static int read_head(int fd, struct aexec *a)
{
	unsigned char buf[SIZEOF_AEXEC];
	
	if (read(fd, buf, SIZEOF_AEXEC) != SIZEOF_AEXEC)
		return -1;
	a->a_magic = read_beword(buf);
	a->a_text = read_belong(buf + 2);
	a->a_data = read_belong(buf + 6);
	a->a_bss = read_belong(buf + 10);
	a->a_syms = read_belong(buf + 14);
	a->a_res1 = read_belong(buf + 18);
	a->a_AZero2 = read_belong(buf + 22);
	a->a_isreloc = read_beword(buf + 26);

	return SIZEOF_AEXEC;
}


static int write_head(int fd, const struct aexec *a)
{
	unsigned char buf[SIZEOF_AEXEC];
	
	write_beword(buf, a->a_magic);
	write_belong(buf + 2, a->a_text);
	write_belong(buf + 6, a->a_data);
	write_belong(buf + 10, a->a_bss);
	write_belong(buf + 14, a->a_syms);
	write_belong(buf + 18, a->a_res1);
	write_belong(buf + 22, a->a_AZero2);
	write_beword(buf + 26, a->a_isreloc);

	if (write(fd, buf, SIZEOF_AEXEC) != SIZEOF_AEXEC)
		return -1;
	return SIZEOF_AEXEC;
}


/*
 * copy from, to in NEWBUFSIZ chunks upto bytes or EOF whichever occurs first
 * returns # of bytes copied
 */
static long copy(int from, int to, long bytes)
{
	long todo;
	long done = 0;
	long remaining = bytes;
	long actual;

	while (done != bytes)
	{
		todo = (remaining > NEWBUFSIZ) ? NEWBUFSIZ : remaining;
		if ((actual = read(from, mybuf, todo)) != todo)
		{
			if (actual < 0)
			{
				fprintf(stderr, "Error Reading\n");
				return -done;
			}
		}
		if (write(to, mybuf, actual) != actual)
		{
			fprintf(stderr, "Error Writing\n");
			return -done;
		}
		done += actual;
		if (actual != todo)				/* eof reached */
			return done;
		remaining -= actual;
	}
	return done;
}


static int relocate(const char *name, int fd, long reloc_pos, long bytes_to_delete)
{
	if (lseek(fd, reloc_pos, SEEK_SET) != reloc_pos)
	{
		perror(name);
		return 0;
	}
	if (read(fd, mybuf, SIZEOF_LONG) != SIZEOF_LONG)
	{
		perror(name);
		return 0;
	}
	write_belong(mybuf, read_belong(mybuf) - bytes_to_delete);
	if (lseek(fd, reloc_pos, SEEK_SET) != reloc_pos)
	{
		perror(name);
		return 0;
	}
	if (write(fd, mybuf, SIZEOF_LONG) != SIZEOF_LONG)
	{
		perror(name);
		return 0;
	}
	return 1;
}


/* copy TOS relocation table from `from` to `to`. Copy bytes until NUL byte or
   first 4 bytes if == 0.
   returns length of relocation table or -1 in case of an error */

static long copy_relocs(const char *name, int from, int to, long bytes_to_delete, long image_size)
{
	long res = 0;
	long bytes;
	long rbytes = 0;
	long first_relo;
	long reloc_pos;
	long limit;
	
	res = read(from, mybuf, SIZEOF_LONG);
	if (res != 0 && res != SIZEOF_LONG)
	{
		fprintf(stderr, "Error reading\n");
		return -1;
	}

	if (res == 0)
	{
		/* I think empty relocation tables are allowed,
		   but could cause trouble with certain programs */
		fprintf(stderr, "Warning: %s: No relocation table\n", name);
		return 0;
	}
	first_relo = read_belong(mybuf);
	if (first_relo != 0)
	{
		/*
		 * The initially designed elf format had some relocations in the header
		 * that we are about to remove.
		 * Supporting that would require to rebuild the relocation table.
		 * The current format does not have such relocations anymore,
		 * but better check for it.
		 */
		if (first_relo <= bytes_to_delete)
		{
			fprintf(stderr, "%s: first relocation at 0x%08lx is in area to be deleted\n", name, first_relo);
			fprintf(stderr, "update your binutils\n");
			return -1;
		}
		first_relo -= bytes_to_delete;
		write_belong(mybuf, first_relo);
	}
	
	if (write(to, mybuf, res) != res)
	{
		fprintf(stderr, "%s: Error writing\n", name);
		return -1;
	}

	rbytes = SIZEOF_LONG;
	if (first_relo == 0)
		return rbytes;						/* This is a clean version of an empty
										   relocation table                   */

	reloc_pos = SIZEOF_AEXEC + first_relo;
	if (relocate(name, to, reloc_pos, bytes_to_delete) == 0)
		return -1;
	limit = SIZEOF_AEXEC + image_size;

	for (;;)
	{
		lseek(to, 0, SEEK_END);
		if ((bytes = read(from, mybuf, 1)) < 0)
		{
			fprintf(stderr, "%s: Error reading\n", name);
			return -1;
		}
		if (bytes == 0)
		{
			fprintf(stderr, "Warning: %s: Unterminated relocation table\n", name);
			return rbytes;
		}
		if (write(to, mybuf, bytes) != bytes)
		{
			fprintf(stderr, "%s: Error writing\n", name);
			return -1;
		}
		rbytes += bytes;
		if (mybuf[0] == 0)
			break;
		if (mybuf[0] == 1)
		{
			reloc_pos += 254;
		} else
		{
			reloc_pos += mybuf[0];
			/*
			 * check that we don't try to write beyond the new image size.
			 * This too, could happen with early version of new elf binutils
			 */
			if (reloc_pos >= limit)
			{
				fprintf(stderr, "%s: relocation at 0x%08lx is outside image\n", name, reloc_pos);
				fprintf(stderr, "update your binutils\n");
				return -1;
			}
			if (relocate(name, to, reloc_pos, bytes_to_delete) == 0)
				return -1;
		}
	}
	return rbytes;
}


static int strip(const char *name)
{
	int fd;
	int tfd;
	long count, rbytes, sbytes;
	struct aexec ahead;
	unsigned char buf[13 * SIZEOF_LONG];
	unsigned long magic1, magic2;
	long bytes_to_delete;

	if ((fd = open(name, O_RDONLY | O_BINARY, 0755)) < 0)
	{
		perror(name);
		return 1;
	}
	if ((tfd = open(tmpname, O_RDWR | O_BINARY | O_TRUNC | O_CREAT, 0755)) < 0)
	{
		perror(tmpname);
		close(fd);
		return 1;
	}
	
	/*
	 * read g_jump_entry and first 8 longs of exec header
	 */
	if (read_head(fd, &ahead) < 0 ||
		(int)read(fd, buf, 8 * SIZEOF_LONG) != 8 * SIZEOF_LONG)
	{
		perror(name);
		close(tfd);
		close(fd);
		return 1;
	}
	if (ahead.a_magic != CMAGIC)
	{
		fprintf(stderr, "%s: Bad Magic number\n", name);
		close(tfd);
		close(fd);
		return 1;
	}
	if (ahead.a_res1 == EXEC_FMT_MINT)		/* 'MiNT' extended exec header magic */
	{
		long e_entry;

		bytes_to_delete = SIZEOF_BINUTILS_AEXEC;

		/*
		 * read trampoline code
		 */
		magic1 = read_belong(buf);
		magic2 = read_belong(buf + SIZEOF_LONG);
		if (!((magic1 == 0x283a001aUL && magic2 == 0x4efb48faUL) ||	/* Original binutils */
			  (magic1 == 0x203a001aUL && magic2 == 0x4efb08faUL)))		/* binutils >= 2.18-mint-20080209 */
		{
			fprintf(stderr, "%s: no aexec header\n", name);
			close(tfd);
			close(fd);
			return 1;
		}
#if 0
		magic1 = read_belong(buf + 2 * SIZEOF_LONG);
		printf("e_info: %08lx\n", magic1);
		magic1 = read_belong(buf + 3 * SIZEOF_LONG);
		printf("e_text: %08lx\n", magic1);
		magic1 = read_belong(buf + 4 * SIZEOF_LONG);
		printf("e_data: %08lx\n", magic1);
		magic1 = read_belong(buf + 5 * SIZEOF_LONG);
		printf("e_bss: %08lx\n", magic1);
		magic1 = read_belong(buf + 6 * SIZEOF_LONG);
		printf("e_syms: %08lx\n", magic1);
#endif
		e_entry = read_belong(buf + 7 * SIZEOF_LONG);
#if 0
		printf("e_entry: %08lx\n", e_entry);
#endif
		if (e_entry != bytes_to_delete)
		{
			fprintf(stderr, "%s: warning: entry %08lx not at start of text segment\n", name, e_entry);
			if (!force)
			{
				fprintf(stderr, "use -f to override\n");
				close(tfd);
				close(fd);
				return 1;
			}
		}
	} else if ((ahead.a_res1 & 0xffffff00UL) == EXEC_FMT_ELF && (ahead.a_res1 & 0xff) >= 40)
	{
		long elf_offset;
		long e_phoff;
		long e_entry;
		long p_offset;
		unsigned short e_phnum;
		
		/*
		 * offset of ELF header is determined by last byte of a_res1 (normally 40)
		 */
		elf_offset = (ahead.a_res1 & 0xff);
		/*
		 * read trampoline code
		 */
		magic1 = read_belong(buf);
		magic2 = read_belong(buf + SIZEOF_LONG);
		if (magic1 != (0x203a0000UL | (elf_offset + 24 - 30)) || magic2 != 0x4efb08faUL)		/* binutils >= 2.41-mintelf */
		{
			fprintf(stderr, "%s: no exec header\n", name);
			close(tfd);
			close(fd);
			return 1;
		}
#if 0
		magic1 = read_belong(buf + 2 * SIZEOF_LONG);
		printf("_stksize: @%08lx\n", magic1);
#endif
		if (lseek(fd, elf_offset, SEEK_SET) < 0 ||
			(int)read(fd, buf, 13 * SIZEOF_LONG) != 13 * SIZEOF_LONG)
		{
			perror(name);
			close(tfd);
			close(fd);
			return 1;
		}
		/*
		 * We will find a complete ELF header there,
		 * but only care here about e_magic & e_entry
		 */
		magic1 = read_belong(buf);
		e_phnum = read_beword(buf + 44);
		if (magic1 != 0x7f454c46UL || e_phnum == 0)
		{
			fprintf(stderr, "%s: no ELF header\n", name);
			close(tfd);
			close(fd);
			return 1;
		}
		e_entry = read_belong(buf + 6 * SIZEOF_LONG);
		e_phoff = read_belong(buf + 7 * SIZEOF_LONG);
		/*
		 * read first program header
		 */
		if (lseek(fd, e_phoff, SEEK_SET) < 0 ||
			(int)read(fd, buf, 8 * SIZEOF_LONG) != 8 * SIZEOF_LONG)
		{
			perror(name);
			close(tfd);
			close(fd);
			return 1;
		}
		/*
		 * get start of text segment
		 */
		p_offset = read_belong(buf + SIZEOF_LONG);
		bytes_to_delete = e_phoff + e_phnum * 32 - SIZEOF_AEXEC;
		if (p_offset != SIZEOF_AEXEC || e_entry != bytes_to_delete)
		{
			fprintf(stderr, "%s: warning: entry %08lx not at start of text segment %08lx\n", name, e_entry, bytes_to_delete);
			if (!force)
			{
				fprintf(stderr, "use -f to override\n");
				close(tfd);
				close(fd);
				return 1;
			}
		}
	} else if (ahead.a_res1 == EXEC_FMT_TOS)
	{
		if (ahead.a_syms == 0)
		{
			fprintf(stderr, "%s: already stripped\n", name);
			close(tfd);
			close(fd);
			return 0;
		}
		bytes_to_delete = 0;
	} else
	{
		fprintf(stderr, "%s: unsupported file format\n", name);
		close(tfd);
		close(fd);
		return 1;
	}

	sbytes = ahead.a_syms;
	if (verbose)
	{
		printf("%s: text=0x%lx, data=0x%lx, syms=0x%lx\n", name, ahead.a_text, ahead.a_data, ahead.a_syms);
	}

	ahead.a_syms = 0;
	ahead.a_text -= bytes_to_delete;
	ahead.a_res1 = 0;
	if (write_head(tfd, &ahead) < 0)
	{
		perror(name);
		close(tfd);
		close(fd);
		return 1;
	}
	if (lseek(fd, bytes_to_delete + SIZEOF_AEXEC, SEEK_SET) < 0)
	{
		perror(name);
		close(tfd);
		close(fd);
		return 1;
	}
	if (verbose)
	{
		printf("%s: skipped 0x%lx bytes extra header\n", name, bytes_to_delete);
	}

	count = ahead.a_text + ahead.a_data;
	if (copy(fd, tfd, count) != count)
	{
		close(tfd);
		close(fd);
		return 1;
	}
	if (verbose)
	{
		printf("%s: copied 0x%lx bytes text+data\n", name, count);
	}
	if (lseek(fd, sbytes, SEEK_CUR) < 0)
	{
		perror(name);
		close(tfd);
		close(fd);
		return 1;
	}
	if (sbytes != 0 && verbose)
	{
		printf("%s: skipped 0x%lx bytes symbols\n", name, sbytes);
	}
	rbytes = 0;
	if (ahead.a_isreloc == 0)
	{
		if ((rbytes = copy_relocs(name, fd, tfd, bytes_to_delete, count)) < 0)
		{
			close(tfd);
			close(fd);
			return 1;
		}
		if (verbose)
		{
			printf("%s: copied 0x%lx bytes relocation table\n", name, rbytes);
		}
	}
	if (verbose)
	{
		unsigned long pos, size;
		
		pos = lseek(fd, 0, SEEK_CUR);
		lseek(fd, 0, SEEK_END);
		size = lseek(fd, 0, SEEK_CUR);
		lseek(fd, pos, SEEK_SET);
		if (size > pos)
		{
			/* could be pure-c debug information, after end of relocation table */
			printf("%s: skipped 0x%lx bytes trailer\n", name, size - pos);
		}
	}

	close(tfd);
	close(fd);
	if (rename(tmpname, name) == 0)
		return 0;						/* try to rename it */
	if ((fd = open(name, O_WRONLY | O_BINARY | O_TRUNC | O_CREAT, 0755)) < 0)
	{
		perror(name);
		return 1;
	}
	if ((tfd = open(tmpname, O_RDONLY | O_BINARY, 0755)) < 0)
	{
		perror(tmpname);
		close(fd);
		return 1;
	}

	count = SIZEOF_AEXEC + ahead.a_text + ahead.a_data + rbytes;
	if (copy(tfd, fd, count) != count)
	{
		close(tfd);
		close(fd);
		return 1;
	}
	close(tfd);
	close(fd);
	return 0;
}


static void usage(const char *s)
{
	fprintf(stderr, "%s", s);
	fprintf(stderr, "Usage: stripex [-f] files ...\n");
	fprintf(stderr, "strip GNU-binutils aexec/elf header from executables\n");
}


int main(int argc, char **argv)
{
	int status = 0;

	verbose = 0;
	
	/* process arguments */
	while (argv++, --argc)
	{
		if ('-' != **argv)
			break;
		(*argv)++;
		switch (**argv)
		{
		case 'v':
			verbose = 1;
			break;
		case 'f':
			force = 1;
			break;
		default:
			usage("");
			return 1;
		}
	}

	if (argc < 1)
	{
		usage("");
		return 1;
	}
	
	do
	{
		const char *filename = *argv++;
		char *base;
		int fd;

		strcpy(tmpname, filename);
		base = f_basename(tmpname);
		strcpy(base, "STXXXXXX");
		fd = mkstemp(tmpname);
		if (fd < 0)
		{
			perror(tmpname);
			status |= 1;
		} else
		{
			close(fd);
			status |= strip(filename);
			unlink(tmpname);
		}
	} while (--argc > 0);

	return status;
}
