# include <stdio.h>
# include <mintbind.h>

typedef struct
{
    void *unshift;	/* pointer to unshifted keys */
    void *shift;	/* pointer to shifted keys */
    void *caps;		/* pointer to capslock keys */
    void *alt;
    void *altshift;
    void *altcaps;
} KEYTAB;

static long
get_akp(void)
{
	long *cj = Setexc(0x5a0L>>2, -1L);

	while(cj[0])
	{
		if (cj[0] == 0x5f414b50L)
			return cj[1];
		cj += 2;
	}

	return -1L;
}

static void
n_write(FILE *fd, unsigned char num)
{
	if (num >= 0x20 && num <= 0x7e)
		if (num == '\'' || num == '\\')
			fprintf(fd, "'\\%c',", num);
		else
			fprintf(fd, "'%c' ,", num);
	else
		fprintf(fd, "0x%02hx,", num);
}

int
main()
{
	KEYTAB *key;
	FILE *fd;
	short i, j;
	long akp;
	unsigned char *aux;
	char *filename, *tabname;

	akp = get_akp();

	if (akp < 0)
	{
		fprintf(stdout, "Keyboard config not determinable\n\n");

		return -1;
	}

	akp &= 0x000000ffL;

	switch(akp)
	{
		case	0:
			filename = "usa.kbd";
			tabname = "usa_kbd";
			break;
		case	1:
			filename = "german.kbd";
			tabname = "german_kbd";
			break;
		case	2:
			filename = "french.kbd";
			tabname = "french_kbd";
			break;
		case	3:
			filename = "british.kbd";
			tabname = "british_kbd";
			break;
		case	4:
			filename = "spanish.kbd";
			tabname = "spanish_kbd";
			break;
		case	5:
			filename = "italian.kbd";
			tabname = "italian_kbd";
			break;
		case	7:
			filename = "sw_french.kbd";
			tabname = "sw_french_kbd";
			break;
		case	8:
			filename = "sw_german.kbd";
			tabname = "sw_german_kbd";
			break;
		default:
			filename = "english.kbd";
			tabname = "english_kbd";
			break;
	}

	key = (KEYTAB *)Keytbl(-1L, -1L, -1L);

	fd = fopen(filename, "w");
	if (!fd)
		return -1;
	
	fprintf(fd, "/* Keyboard table for TOS 4.04 */\n\n/* Unshifted */\n\n");
	fprintf(fd, "const char %s[] =\n{\n", tabname);
	
	aux = key->unshift;

	for (i = 0; i < 16; i++)
	{
		fprintf(fd, "\t");
		for (j = 0; j < 8; j++)
			n_write(fd, aux[i*8+j]);
		fprintf(fd, "\n");
	}

	fprintf(fd, "\n/* Shifted */\n\n");
	
	aux = key->shift;

	for (i = 0; i < 16; i++)
	{
		fprintf(fd, "\t");
		for (j = 0; j < 8; j++)
			n_write(fd, aux[i*8+j]);
		fprintf(fd, "\n");
	}

	fprintf(fd, "\n/* Caps */\n\n");
	
	aux = key->caps;

	for (i = 0; i < 16; i++)
	{
		fprintf(fd, "\t");
		for (j = 0; j < 8; j++)
			n_write(fd, aux[i*8+j]);
		fprintf(fd, "\n");
	}

	fprintf(fd, "\n/* Alternate */\n");

	aux = key->alt;
	i = 0;
	while(aux[i])
	{
		if (!i)
			fprintf(fd, "\n\t");
		n_write(fd, aux[i++]);
		if (i > 7)
		{
			i = 0;
			aux += 8;
		}
	}	
	fprintf(fd, "0x00,\n");

	fprintf(fd, "\n/* Alternate shifted */\n");

	aux = key->altshift;
	i = 0;
	while(aux[i])
	{
		if (!i)
			fprintf(fd, "\n\t");
		n_write(fd, aux[i++]);
		if (i > 7)
		{
			i = 0;
			aux += 8;
		}
	}	
	fprintf(fd, "0x00,\n");

	fprintf(fd, "\n/* Alternate Caps */\n");

	aux = key->altcaps;
	i = 0;
	while(aux[i])
	{
		if (!i)
			fprintf(fd, "\n\t");
		n_write(fd, aux[i++]);
		if (i > 7)
		{
			i = 0;
			aux += 8;
		}
	}	
	fprintf(fd, "0x00\n};\n\n/* EOF */\n");

	fclose(fd);

	return 0;
}

/* EOF */
