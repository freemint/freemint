/*
 * Filename:     gs_conf.c
 * Project:      GlueSTiK
 * 
 * Note:         Please send suggestions, patches or bug reports to
 *               the MiNT mailing list <freemint-discuss@lists.sourceforge.net>
 * 
 * Copying:      Copyright 1999 Frank Naumann <fnaumann@freemint.de>
 * 
 * Portions copyright 1997, 1998, 1999 Scott Bigham <dsb@cs.duke.edu>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# include <string.h>
# include <ctype.h>
# ifdef __PUREC__
# include <tos.h>
# ifdef __TOS /* using original header file */
# define __XATTR
# define st_size size
# endif
# else
# include <osbind.h>
# include <mintbind.h>
# include <sys/stat.h>
# endif

# include "../socklib/fxattr.h"

# include "gs_conf.h"


# define SYSVAR_bootdev	(*((short *) 0x446UL))


typedef struct
{
	void *mem;
	size_t size;
	int n_entries;
	int n_slots;
} GrowBuf;

typedef struct
{
	int name_off;
	int val_off;
} Var;


static GrowBuf gvars =
{
	0,
	sizeof (Var),
	0,
	0
};

static GrowBuf gdata =
{
	0, sizeof (char),
	0,
	0
};

static char *data = 0;
static Var *vars = 0;

char cfg_filename[160] = "A:\\STIK_CFG\\DEFAULT.CFG";
static char stikdir_dat[] = "A:\\STIK_DIR.DAT";


/* growbuf() -- request space for |new_slots| new entries in the
 * GrowBuf |G|, reallocating the buffer if necessary
 */
static int
growbuf (GrowBuf *buf, int new_slots)
{
	if (!buf->mem)
	{
		buf->mem = malloc (buf->size * new_slots);
		if (!buf->mem)
			return 0;
		
		buf->n_slots = new_slots;
		buf->n_entries = 0;
		
		return 1;
	}
	
	if (buf->n_entries + new_slots <= buf->n_slots)
		return 1;
	
	{
		int n = buf->n_slots;
		void *v;
		
		while (n < buf->n_entries + new_slots)
			n *= 2;
		
		v = malloc (buf->size * n);
		if (!v)
			return 0;
		
		memcpy (v, buf->mem, buf->size * buf->n_entries);
		free (buf->mem);
		
		buf->mem = v;
		buf->n_slots = n;
		
		return 1;
	}
}

# define GROWBUF(buf, slots, var) \
	(growbuf (&(buf), slots) ? (((var) = (buf).mem), 1) : 0)

/* set_var() -- add variable |name| with value |value| (or change value
 * if variable is already present).  Returns 1 on success, 0 on failure.
 */
static int
set_var (const char *name, const char *value)
{
	int i;
	
	if (!GROWBUF (gvars, 1, vars) ||
		!GROWBUF (gdata, (int)(strlen (name) + strlen (value) + 2), data))
	{
		return 0;
	}
	
	for (i = 0; i < gvars.n_entries; i++)
	{
		if (!stricmp (name, data + vars[i].name_off))
		{
			vars[i].val_off = gdata.n_entries;
			strcpy (data + gdata.n_entries, value);
			gdata.n_entries += (int)strlen (value) + 1;
			
			return 1;
		}
	}
	
	i = gvars.n_entries++;
	vars[i].name_off = gdata.n_entries;
	strcpy (data + gdata.n_entries, name);
	gdata.n_entries += (int)strlen (name) + 1;
	vars[i].val_off = gdata.n_entries;
	strcpy (data + gdata.n_entries, value);
	gdata.n_entries += (int)strlen (value) + 1;
	
	return 1;
}

/* kill_tws() -- strip trailing whitespace from string */
static void
kill_tws (char *s)
{
	register char *t = s + strlen (s);
	
	while (t > s && isspace (t[-1]))
		t--;
	
	*t = '\0';
}


static long
get_bootdrive (void)
{
	return SYSVAR_bootdev;
}

static int
find_config_file (void)
{
	register long l;
	register int n, fd, boot;
	register char *s;
	
	boot = (int)Supexec (get_bootdrive);
	if (boot < 0 || boot > 31)
	{
		(void) Cconws ("Failed to determine boot device!\r\n");
		return 1;
	}
	
	cfg_filename [0] += boot;
	stikdir_dat [0] += boot;
	
	if ((l = Fopen (stikdir_dat, 0)) < 0)
		return 1;
	
	fd = (int)l;
	if ((n = (int)Fread (fd, sizeof (cfg_filename), cfg_filename)) < 0)
	{
		Fclose (fd);
		(void) Cconws ("Error reading STIK_DIR.DAT; using default path\r\n");
		return 1;
	}
	
	if ((int)n >= (int)sizeof (cfg_filename))
		n = (int)sizeof (cfg_filename) - 1;
	
	if ((s = strchr (cfg_filename, '\n')) == NULL)
		s = cfg_filename + n;
	
	if (s [-1] == '\r')
		s--;
	
	*s = 0;
	
	/* Incompatibility:  The STiK docs imply that you can change the
	 * config file name via STIK_DIR.DAT, but then say to include only the
	 * new path.	I'm gonna handle it this way:  if the line read from
	 * STIK_DIR.DAT has a trailing '/' or '\', assume it's a path and
	 * append "DEFAULT.CFG"; otherwise, assume it's the whole file name
	 * and return it untouched.
	 */
	if (s [-1] == '\\' || s [-1] == '/')
		strcpy (s, "DEFAULT.CFG");
	
	return 1;
}

int
load_config_file (void)
{
	XATTR xattr;
	char *file_data;
	ulong filesize;
	long l;
	int n, fd;
	register char *s, *t;
	
	if (!GROWBUF (gvars, 100, vars) || !GROWBUF (gdata, 5000, data))
	{
		(void) Cconws ("Cannot get memory for STiK vars\r\n");
		return 0;
	}
	
	if (!find_config_file ())
	{
		/* Okay, so I'm paranoid... ;)
		 */
		(void) Cconws ("Unable to find config file\r\n");
		return 0;
	}
	
	if (Fxattr (0, cfg_filename, &xattr))
	{
		(void) Cconws ("Config file ");
		(void) Cconws (cfg_filename);
		(void) Cconws (" does not exist; using built-in defaults...\r\n");
		
		if (!set_var ("ALLOCMEM", "100000"))
			return 0;
		if (!set_var ("GLUESTIK_TRACE", "0"))
			return 0;
		
		return 1;
	}
	
	filesize = xattr.st_size;
	(void) Cconws ("Reading config file ");
	(void) Cconws (cfg_filename);
	(void) Cconws ("...\r\n");
	
	file_data = malloc (filesize + 2);
	if (!file_data)
	{
		(void) Cconws ("Cannot get memory to read config file\r\n");
		return 0;
	}
	file_data [filesize] = file_data [filesize + 1] = '\0';
	
	if ((l = Fopen (cfg_filename, 0)) < 0)
	{
		(void) Cconws ("Error opening config file ");
		(void) Cconws (cfg_filename);
		(void) Cconws ("\r\n");
		return 0;
	}
	fd = (int)l;
	
	s = file_data;
	while ((n = (int)Fread (fd, filesize, s)) > 0)
		s += n;
	
	if (n < 0)
	{
		(void) Cconws ("Error reading config file ");
		(void) Cconws (cfg_filename);
		(void) Cconws ("\r\n");
		free (file_data);
		return 0;
	}
	
	Fclose (fd);
	*s = 0;
	
	/* Split off all lines */
	for (s = file_data; s < file_data + filesize; s++)
	{
		if (*s == '\r' || *s == '\n')
			*s = 0;
	}
	
	s = file_data;
	while (s < file_data + filesize)
	{
		/* skip blanks
		 */
		while (*s && isspace (*s))
			s++;
		
		if (!*s)
		{
			s++;
			continue;
		}
		
		/* skip comments
		 */
		if (*s == '#')
		{
			while (*s) s++;
			continue;
		}

		/* If no '=', variable is "true"
		 */
		t = strchr(s, '=');
		if (!t)
		{
			kill_tws (s);
			t = "1";
			goto setit;
		}
		
		*t++ = 0;
		kill_tws (s);
		while (*t && isspace (*t))
			t++;
		
		/* Empty value means variable is "false"
		 */
		if (!*t)
		{
			t = "0";
			goto setit;
		}
		
		kill_tws (t);
setit:
		if (set_var (s, t))
		{
			while (*s)
				s++;
			continue;
		}
		
		(void) Cconws ("Unable to set variable \"");
		(void) Cconws (s);
		(void) Cconws ("\"\r\n");
		free (file_data);
		return 0;
	}
	
	free (file_data);
	return 1;
}

const char *
gs_getvstr (const char *var)
{
	register int n;
	
	if (!stricmp (var, "CDVALID"))
	{
		DEBUG (("Special case: getvstr(\"CDVALID\") returns \"0\""));
		return "0";
	}
	
	for (n = 0; n < gvars.n_entries; n++)
	{
		if (stricmp (var, data + vars [n].name_off) == 0)
		{
			DEBUG (("getvstr(\"%s\") returns \"%s\"",
				var, data + vars [n].val_off));
			
			return data + vars [n].val_off;
		}
	}
	
	DEBUG (("getvstr(\"%s\") returns \"0\"", var));
	return "0";
}

long
gs_setvstr (const char *vs, const char *value)
{
	int n;
	
	n = set_var (vs, value);
	DEBUG (("setvstr(\"%s\", \"%s\") returns %d", vs, value, n));
	
	return n;
}


void
cleanup_config (void)
{
	if (gvars.mem)
		free (gvars.mem);
	
	if (gdata.mem)
		free (gdata.mem);
}
