/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

# ifdef DEBUG_INFO
# define NALLOC_STAT
# endif

# ifdef NALLOC_STAT
typedef struct test {
	long int size;
	long int count;
	long success;
} TEST;

static TEST st_alloc [1001]; static long int st_alloc_count = 0;
static TEST st_free [1001]; static long int st_free_count = 0;
static TEST st_get [1001]; static long int st_get_count = 0;

# define FASTFN static inline

static void st		(long int size, TEST *st_, long int *st_count);
FASTFN void st_a	(long int size);
FASTFN void st_f	(long int size);
FASTFN void st_g	(long int size);
FASTFN void st_g_succes	(long int size);

static void
st (long int size, TEST *st_, long int *st_count)
{
	long i;
	for (i = *st_count; i > 0; i--)
	{
		if (st_[i].size == size)
		{
			st_[i].count++;
			return;
		}
	}
	if (*st_count < 1000)
	{
		(*st_count)++;
		st_[*st_count].size = size;
		st_[*st_count].count = 1;
	}
}
FASTFN void
st_a (long int size)
{
	st (size, st_alloc, &st_alloc_count);
}
FASTFN void
st_f (long int size)
{
	st (size, st_free, &st_free_count);
}
FASTFN void
st_g (long int size)
{
	st (size, st_get, &st_get_count);
}
FASTFN void
st_g_succes (long int size)
{
	long i;
	for (i = st_get_count; i > 0; i--)
	{
		if (st_get[i].size == size)
		{
			st_get[i].success++;
			return;
		}
	}
}
# endif

/*
 * General-purpose memory allocator, on the MWC arena model, with
 * this added feature:
 *
 * All blocks are coalesced when they're freed.  If this results in
 * an arena with only one block, and that free, it's returned to the
 * OS.
 *
 * The functions here have the same names and bindings as the MWC
 * memory manager, which is the same as the UNIX names and bindings.
 *
 * MiNT version: used for kmalloc to manage kernel memory in small hunks,
 * rather than using a page at a time.
 */

# include "nalloc2.h"

# include "memory.h"


# if 0
# define NALLOC_DEBUG(c) TRACE(("nalloc: %c",c));
# else
# define NALLOC_DEBUG(c) /* nothing */
# endif

# define NKMAGIC 0x19870425L

/*
 * block header: every memory block has one.
 * A block is either allocated or on the free
 * list of the arena.  There is no allocated list: it's not necessary.
 * The next pointer is only valid for free blocks: for allocated blocks
 * maybe it should hold a verification value..?
 *
 * Zero-length blocks are possible and free; they hold space which might 
 * get coalesced usefully later on.
 */

struct block
{
    struct block *b_next;   /* NULL for last guy; next alloc or next free */
    long b_size;
};

/*
 * arena header: every arena has one.  Each arena is always completely
 * filled with blocks; the first starts right after this header.
 */

struct arena
{
    struct arena *a_next;
    struct block *a_ffirst;
    long a_size;
};

/*
 * Arena linked-list pointer, and block size.
 */

static struct arena *a_first = (struct arena *) NULL;
static long int used_mem = 0;

void
nalloc_arena_add (void *start, long int len)
{
	struct arena *a;
	struct block *b;
	
	for (a = a_first; a && a->a_next; a = a->a_next)
		continue;
	
	if (a)
		a->a_next = (struct arena *) start;
	else
		a_first = (struct arena *) start;
	
	a = start;
	a->a_next = NULL;
	a->a_ffirst = b = (struct block *)(a+1);
	a->a_size = len - sizeof (*a);
	b->b_next = NULL;
	b->b_size = (len - sizeof (*a) - sizeof (*b));
	used_mem += len;
}


void *
nalloc (long int size)
{
	struct arena *a;
	struct block *b, *mb, **q;
	long temp;
	
# ifdef NALLOC_STAT
	long get_temp = size - 8;
	st_g (get_temp);
# endif
	
	NALLOC_DEBUG ('A');
	
	/* force even-sized alloc's */
	size = (size + 1) & ~1;
	
	for (a = a_first; a; a = a->a_next)
	{
		for (b = *(q = &a->a_ffirst); b; b = *(q = &b->b_next))
		{
			/* if big enough, use it */
			if (b->b_size >= size)
			{
				
				/* got one */
				mb = b;
				
				/* cut the free block into an allocated part & a free part */
				temp = mb->b_size - size - sizeof(struct block);
				if (temp >= 0)
				{
					/* large enough to cut */
					NALLOC_DEBUG ('c');
					b = (struct block *)(((char *)(b+1)) + size);
					b->b_size = temp;
					b->b_next = mb->b_next;
					*q = b;
					mb->b_size = size;
# ifdef NALLOC_STAT
					st_a (size);
					st_g_succes (get_temp);
# endif
				}
				else
				{
					/* not big enough to cut: unlink this from free list */
					NALLOC_DEBUG ('w');
					*q = mb->b_next;
# ifdef NALLOC_STAT
					st_a (mb->b_size);
					st_g_succes (get_temp);
# endif
				}
				
				mb->b_next = (struct block *) NKMAGIC;
				return (void *)(mb + 1);
			}
		}
	}
	
	/* no block available: get a new arena */
# if 1
	return NULL;	/* MiNT: fail. */
# else
	if (!minarena)
	{
		minarena = Malloc(-1L) / 20;
		if (minarena > MAXARENA) minarena = MAXARENA;
	}
	
	if (size < minarena)
	{
		NALLOC_DEBUG ('m');
		temp = minarena;
	}
	else
	{
		NALLOC_DEBUG ('s');
		temp = size;
	}
	
	a = (struct arena *) Malloc (temp + 
				sizeof (struct arena) +
				sizeof (struct block));
	
	/* if Malloc failed return failure */
	if (a == NULL)
	{
		NALLOC_DEBUG ('x');
		return NULL;
	}
	
	a->a_size = temp + sizeof(struct block);
	a->a_next = a_first;
	a_first = a;
	mb = (struct block *)(a+1);
	mb->b_next = NULL;
	mb->b_size = size;
	
	if (temp > (size + sizeof (struct block)))
	{
		NALLOC_DEBUG ('c');
		b = a->a_ffirst = ((char *)(mb + 1)) + size;
		b->b_next = NULL;
		b->b_size = temp - size - sizeof (struct block);
	}
	else
	{
		a->a_ffirst = NULL;
	}
	
	return (void *)(++mb);
# endif
}

void
nfree (void *start)
{
	struct arena *a, **qa;
	struct block *b;
	struct block *pb;
	struct block *fb = (struct block *)start;
	
	NALLOC_DEBUG ('F');
	
	/* set fb (and b) to header start */
	b = --fb;
	
	if (fb->b_next != (struct block *) NKMAGIC)
	{
		FATAL ("nfree: block %lx not allocated by nalloc!", fb);
	}
	
	/* the arena this block lives in */
	for (a = *(qa = &a_first); a; a = *(qa = &a->a_next))
	{
		if ((ulong) b >= (ulong) (a+1)
			&& (ulong) b < (((ulong) (a+1)) + a->a_size))
		{
			goto found;
		}
	}
	
	FATAL ("nfree: block %lx not in any arena!", fb);
	
found:
	/* Found it! */
	/* a is this block's arena */
	
# ifdef NALLOC_STAT
	st_f (fb->b_size - 8);
# endif
	
	/* set pb to the previous free block in this arena, b to next */
	for (pb = NULL, b = a->a_ffirst; b && (b < fb); pb = b, b = b->b_next)
		;
	
	fb->b_next = b;
	
	/* Coalesce backwards: if any prev ... */
	if (pb)
	{
		/* if it's adjacent ... */
		if ((((ulong) (pb + 1)) + pb->b_size) == (ulong) fb)
		{
			NALLOC_DEBUG('b');
			pb->b_size += sizeof(struct block) + fb->b_size;
			fb = pb;
		}
		else
		{
			/* ... else not adjacent, but there is a prev free block */
			/* so set its next ptr to fb */
			pb->b_next = fb;
		}
	}
	else
	{
		/* ... else no prev free block: set arena's free list ptr to fb */
		a->a_ffirst = fb;
	}
	
	/* Coalesce forwards: b holds start of free block AFTER fb, if any */
	if (b && (((ulong) (fb + 1)) + fb->b_size) == (ulong) b)
	{
		NALLOC_DEBUG ('f');
		fb->b_size += sizeof (struct block) + b->b_size;
		fb->b_next = b->b_next;
	}

	/* if, after coalescing, this arena is entirely free, Mfree it! */
	if ((struct arena *) a->a_ffirst == a + 1
		&& (a->a_ffirst->b_size + sizeof (struct block)) == a->a_size
		&& a != a_first)
	{
		NALLOC_DEBUG ('!');
		*qa = a->a_next;
		used_mem -= a->a_size - sizeof (*a);
# if 1
		/* MiNT -- give back so it can be used by users */
		kfree (a);
# else
		(void) Mfree (a);
# endif
	}
}

# ifndef NALLOC_STAT
void
NALLOC_DUMP (void)
{
	struct arena *a;
	struct block *b;
	
	FORCE ("nalloc used_mem = %ld", used_mem);
	
	for (a = a_first; a; a = a->a_next)
	{
		FORCE ("Arena at %lx size %lx: free list:", a, a->a_size);
		for (b = a->a_ffirst; b; b = b->b_next)
		{
			FORCE ("    %8lx size %8lx", b, b->b_size);
		}
	}
}

# else

# include "dosfile.h"
# include "util.h"

void
NALLOC_DUMP (void)
{
	static char line [SPRINTF_MAX];
	long int i, len;
	FILEPTR *f;

	FORCE ("nalloc used_mem = %ld", used_mem);

	f = do_open("u:\\ram\\nalloc.txt", (O_RDWR|O_CREAT|O_TRUNC), 0, (XATTR *)0);
	if (f)
	{
		ksprintf (line, "block size:\tcount:\r\n");
		len = strlen (line);
		(*f->dev->write)(f, line, len);
		for (i = 0; i <= st_alloc_count; i++)
		{
			ksprintf (line, "nalloc\t%ld\t%ld\r\n", st_alloc[i].size, st_alloc[i].count);
			len = strlen (line);
			(*f->dev->write)(f, line, len);
		}
		ksprintf (line, "block size:\tcount:\r\n");
		len = strlen (line);
		(*f->dev->write)(f, line, len);
		for (i = 0; i <= st_free_count; i++)
		{
			ksprintf (line, "nfree\t%ld\t%ld\r\n", st_free[i].size, st_free[i].count);
			len = strlen (line);
			(*f->dev->write)(f, line, len);
		}
		do_close (f);
	}
	f = do_open("u:\\ram\\array_init.c", (O_RDWR|O_CREAT|O_TRUNC), 0, (XATTR *)0);
	if (f)
	{
		ksprintf (line, "/* request:\tcount:\tready:\tdifference: */\r\n");
		len = strlen (line);
		(*f->dev->write)(f, line, len);
		ksprintf (line, "\r\nvoid array_init (void) {\r\n");
		len = strlen (line);
		(*f->dev->write)(f, line, len);
		for (i = 0; i <= st_get_count; i++)
		{
			ksprintf (line, "req_array [%ld].req_alloc = %ld;\r\nreq_array [%ld].req_free = %ld; /* %ld */\r\n", st_get[i].size, st_get[i].count, st_get[i].size, st_get[i].success, st_get[i].count - st_get[i].success);
			len = strlen (line);
			(*f->dev->write)(f, line, len);
		}
		ksprintf (line, "\r\n}\r\n");
		len = strlen (line);
		(*f->dev->write)(f, line, len);
		do_close (f);
	}
}
# endif
