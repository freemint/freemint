#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, const char **argv)
{
	FILE *f;
	const char *fname = "fpsp.bin";
	unsigned char buf[32];
	unsigned int i1, i2, i3, i4;
	unsigned int pos;
	
	if (argc > 1)
		fname = argv[1];
	f = fopen(fname, "rb");
	if (f == NULL)
	{
		perror(fname);
		return 1;
	}
	memset(buf, 0, sizeof(buf));
	if (fread(buf, 1, 32, f) > 0)
	{
		if (buf[0] != 0x40 || buf[1] != 0x02 || buf[2] != 0x01 || buf[3] != 0x07)
			fseek(f, 0, SEEK_SET);
		if (buf[0] == 0x60 && buf[1] == 0x1a)
			fseek(f, 28, SEEK_SET);
	}
	pos = 0;
	printf("\t.text\n");
	for (;;)
	{
		memset(buf, 0, sizeof(buf));
		if (fread(buf, 1, 16, f) <= 0)
			break;
		i1 = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | (buf[3] << 0);
		i2 = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | (buf[7] << 0);
		i3 = (buf[8] << 24) | (buf[9] << 16) | (buf[10] << 8) | (buf[11] << 0);
		i4 = (buf[12] << 24) | (buf[13] << 16) | (buf[14] << 8) | (buf[15] << 0);
		printf("\t.dc.l\t0x%08x,0x%08x,0x%08x,0x%08x /* %08X */\n", i1, i2, i3, i4, pos);
		pos += 16;
	}
	fclose(f);
	return 0;
}
