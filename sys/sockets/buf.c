/*
 *	This file implements the net memory menager. Basically it is
 *	done using a so called `buddy system'. It allows allocating
 *	different sized memory chunks with minimal overhead.
 *
 *	NOTE: debug output at splhigh hangs the system !!!
 *
 *	01/12/93, kay roemer.
 */

# include "buf.h"

# include "mint/asm.h"
# include "mint/file.h"
# include "net.h"


# define BUF_BLOCK_SIZE		(1024 * 32L)
# define BUF_NSPLIT		7
# define BUF_MAGIC		0x73ec5a13ul


# define GC_TIMEOUT		60000	/* garbage collect every minute */

# define BUF_EMPTY(b)		(pool[b]._nfree == &pool[b])
# define BUF_SIZE(b)		(BUF_BLOCK_SIZE >> (b))


static void	gc		(long);
static void	addmem		(long);
static short	buf_add_block	(void);
static short	buf_free_block	(void);

static long failed_allocs = 0;
static long mem_used = 0;
static BUF pool[BUF_NSPLIT+1];
static TIMEOUT *tmout = NULL;


static void
gc (long proc)
{
	long mem = mem_used;
	
	while (buf_free_block ())
		;
	
	if (mem_used < mem)
	{
		DEBUG (("NET: failed allocs: %ld", failed_allocs));
		DEBUG (("NET: mem used: %ldk before, %ldk after garbage coll.",
			mem/1024, mem_used/1024));
	}
	
	addroottimeout (GC_TIMEOUT, gc, 0);
}

static void
addmem (long proc)
{
	tmout = 0;
	buf_add_block ();
}

static short
buf_add_block (void)
{
	BUF *new;
	ushort sr;
	
	new = kmalloc (BUF_BLOCK_SIZE);
	if (!new)
		return 1;
	
	new->buflen = BUF_BLOCK_SIZE;
	new->links  = 0;
	new->_n     = 
	new->_p	    = NULL;
	
	sr = splhigh();
	new->_nfree = pool[0]._nfree;
	new->_pfree = &pool[0];
	new->_nfree->_pfree = new;
	new->_pfree->_nfree = new;
	mem_used += new->buflen;
	spl (sr);
	
	if (tmout)
	{
		cancelroottimeout (tmout);
		tmout = 0;
	}
	
	return 0;
}

static short
buf_free_block (void)
{
	BUF *buf;
	ushort sr;
	
	sr = splhigh();
	buf = pool[0]._nfree;
	if (buf == &pool[0] || buf->_nfree == &pool[0])
	{
		spl (sr);
		return 0;
	}
	mem_used -= buf->buflen;
	buf->_nfree->_pfree = buf->_pfree;
	buf->_pfree->_nfree = buf->_nfree;
	spl (sr);
	
	kfree (buf);
	
	return 1;
}

long
buf_init (void)
{
	int i;
	
	for (i = 0; i <= BUF_NSPLIT; ++i)
	{
		pool[i].buflen = 0;
		
		pool[i].next = NULL;
		pool[i].prev = NULL;
		
		pool[i].links = 1000;
		
		pool[i]._n = NULL;
		pool[i]._p = NULL;
		pool[i]._nfree = &pool[i];
		pool[i]._pfree = &pool[i];
	}
	
	if (buf_add_block ())
	{
		DEBUG (("buf_init: Cannot alloc buffer pool (%ldk)",
			BUF_BLOCK_SIZE/1024l));
		
		return -1;
	}
	
	addroottimeout (GC_TIMEOUT, gc, 0);
	return 0;
}

# if 0
static short
buf_check (void)
{
	BUF *b;
	short freelist[BUF_NSPLIT+1];
	int i;
	
	for (i = 0; i <= BUF_NSPLIT; ++i)
	{
		freelist[i] = 0;
		for (b = pool[i]._nfree; b != &pool[i]; b = b->_nfree)
			++freelist[i];
	}
	
	DEBUG (("BUF_CHECK: { %d, %d, %d, %d, %d, %d, %d, %d, %d }",
		freelist[0], freelist[1], freelist[2], freelist[3],
		freelist[4], freelist[5], freelist[6], freelist[7],
		freelist[8]));
	
	return 0;
}
# endif

# ifdef BUF_DEBUG
static void
sanity_check (BUF *buf)
{
	int correct = 1;
	
	
	if (buf->dend < buf->dstart)
		correct = 0;
	
	
	if (buf->dstart < buf->data)
		correct = 0;
	
	if (buf->dstart > ((char *) buf + buf->buflen))
		correct = 0;
	
	
	if (buf->dend < buf->data)
		correct = 0;
	
	if (buf->dend > ((char *) buf + buf->buflen))
		correct = 0;
	
	
	if (buf->buflen > BUF_BLOCK_SIZE)
		correct = 0;
	
	
	if (!correct)
		ALERT (("sanity check -> invalid buf"));
}
# define SANITY_CHECK(b) sanity_check(b)
# else
# define SANITY_CHECK(b)
# endif

BUF *
buf_reserve (BUF *buf, long reserve, short mode)
{
	BUF *nbuf;
	ulong nspace, ospace, used;
	
	reserve = (reserve + 1) & ~1;
	
	switch (mode)
	{
		case BUF_RESERVE_START:
		{
			nspace = (long) buf + buf->buflen - (long) buf->dstart + reserve;
			ospace = buf->buflen - sizeof (BUF);
			if (nspace <= ospace)
				return buf;
			
			DEBUG (("buf_reserve: allocating new buf"));
			
			used = (long) buf->dend - (long) buf->dstart;
			nbuf = buf_alloc (nspace, reserve, BUF_NORMAL);
			if (!nbuf)
				return 0;
			
			memcpy (nbuf->dstart, buf->dstart, used);
			nbuf->dend = nbuf->dstart + used;
			nbuf->info = buf->info;
			buf_deref (buf, BUF_NORMAL);
			
			return nbuf;
		}
		case BUF_RESERVE_END:
		{
			nspace = (ulong) buf->dend - (ulong) buf->data + reserve;
			ospace = buf->buflen - sizeof (BUF);
			if (nspace <= ospace)
				return buf;
			
			DEBUG (("buf_reserve: allocating new buf"));
			SANITY_CHECK(buf);
			
			used = (ulong) buf->dend - (ulong) buf->dstart;
			nbuf = buf_alloc (nspace, (ulong) buf->dstart - (ulong) buf->data, BUF_NORMAL);
			if (!nbuf)
				return 0;
			
			SANITY_CHECK(nbuf);
			
			memcpy (nbuf->dstart, buf->dstart, used);
			nbuf->dend = nbuf->dstart + used;
			nbuf->info = buf->info;
			buf_deref (buf, BUF_NORMAL);
			
			return nbuf;
		}
	}
	
	FATAL ("buf_reserve: invalid mode");
	
	/* NOTREACHED */
	return 0;
}

BUF *
buf_alloc (ulong size, ulong reserve, short mode)
{
	short index, i;
	BUF *newbuf;
	ushort sr;
	
	reserve = (reserve + 1) & ~1;
	
	/*
	 * Add 2 'cuz reserve may be rounded up, so size is at least one byte
	 * more than needed
	 */
	size = (size + sizeof (BUF) + 2) & ~1;
	if (size < BUF_SIZE (BUF_NSPLIT))
		size = BUF_SIZE (BUF_NSPLIT);
	
	for (index = BUF_NSPLIT; index >= 0; index--)
		if (size <= BUF_SIZE (index))
			break;
	
	if (index < 0)
	{
		/*
		 * requested block to big
		 */
		failed_allocs++;
		return NULL;
	}
	
try_again:
	sr = splhigh();
	for (i = index; i >= 0 && BUF_EMPTY (i); i--)
		;
	
	if (i < 0)
	{
		if (mode == BUF_ATOMIC)
		{
			if (!tmout)
				tmout = addroottimeout (0, addmem, 1);
			failed_allocs++;
			spl (sr);
			return 0;
		}
		spl (sr);
		
		if (buf_add_block ())
		{
			/*
			 * out of kernel memory
			 */
			failed_allocs++;
			return NULL;
		}
		
		goto try_again;
	}
	
	newbuf = pool[i]._nfree;
	newbuf->_nfree->_pfree = newbuf->_pfree;
	newbuf->_pfree->_nfree = newbuf->_nfree;
	
	if (newbuf->buflen - size >= BUF_SIZE (BUF_NSPLIT))
	{
		BUF *nxtbuf;
		
		nxtbuf = (BUF *)((long) newbuf + size);
		if (newbuf->_n)
			newbuf->_n->_p = nxtbuf;
		
		nxtbuf->buflen = newbuf->buflen - size;
		nxtbuf->links = 0;
		nxtbuf->_n = newbuf->_n;
		nxtbuf->_p = newbuf;
		
		newbuf->buflen = size;
		newbuf->_n = nxtbuf;
		
		while (nxtbuf->buflen < BUF_SIZE (i))
			i++;
		
		if (i > BUF_NSPLIT)
		{
			spl (sr);
			FATAL("%i > BUF_NSPLIT, buflen = %lu", i, nxtbuf->buflen);
		}
		
		nxtbuf->_nfree = pool[i]._nfree;
		nxtbuf->_pfree = &pool[i];
		nxtbuf->_nfree->_pfree = nxtbuf;
		nxtbuf->_pfree->_nfree = nxtbuf;
	}
	
	newbuf->links = 1;
	newbuf->_nfree = NULL;
	newbuf->_pfree = NULL;
	
	spl (sr);
	
	newbuf->dstart = newbuf->data + reserve;
	newbuf->dend = newbuf->dstart;
	
	DEBUG (("newbuf->buflen = %lu", newbuf->buflen));
	if (newbuf->buflen > BUF_BLOCK_SIZE)
		ALERT (("newbuf->buflen = %lu", newbuf->buflen));
	
	return newbuf;
}

static void
_buf_free (BUF *buf, ushort sr)
{
	BUF *b;
	short i;
	
	b = buf->_p;
	if (b && !b->links)
	{
		b->_nfree->_pfree = b->_pfree;
		b->_pfree->_nfree = b->_nfree;
		
		if (buf->_n)
			buf->_n->_p = b;
		
		b->_n = buf->_n;
		b->buflen += buf->buflen;
		buf = b;
	}
	
	b = buf->_n;
	if (b && !b->links)
	{
		b->_nfree->_pfree = b->_pfree;
		b->_pfree->_nfree = b->_nfree;
		
		if (b->_n)
			b->_n->_p = buf;
		
		buf->_n = b->_n;
		buf->buflen += b->buflen;
	}
	
	for (i = 0; i <= BUF_NSPLIT; i++)
		if (buf->buflen >= BUF_SIZE (i))
			break;
	
	if (buf->buflen > BUF_BLOCK_SIZE || i > BUF_NSPLIT)
	{
		spl (sr);
		FATAL ("buf_free: invalid buf size: %ld (%i)", buf->buflen, i);
	}
	
	buf->links = 0;
	buf->_nfree = pool[i]._nfree;
	buf->_pfree = &pool[i];
	buf->_nfree->_pfree = buf;
	buf->_pfree->_nfree = buf;
	
	spl (sr);
}

void
buf_free (BUF *buf, short mode)
{
	FORCE ("Warning, buf_free called directly, update your xif!");
	
	_buf_free(buf, splhigh());
}

void
buf_deref (BUF *buf, short mode)
{
	ushort sr = splhigh();
	
	buf->links--;
	
	if (buf->links < 0)
	{
		spl(sr);
		FATAL("buf_deref: links < 0 (%i)", buf->links);
	}
	
	if (buf->links) spl(sr);
	else _buf_free (buf, sr);
}

BUF *
buf_clone (BUF *buf, short mode)
{
	BUF *nbuf;
	long len;
	
	len = buf->dstart - buf->data;
	nbuf = buf_alloc (buf->buflen - sizeof (BUF), len, mode);
	if (!nbuf)
		return 0;
	
	len = buf->dend - buf->dstart;
	memcpy (nbuf->dstart, buf->dstart, len);
	nbuf->dend += len;
	nbuf->info = buf->info;
	
	return nbuf;
}
