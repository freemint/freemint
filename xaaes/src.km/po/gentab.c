#include <stdio.h>
#include <stdlib.h>

typedef unsigned short h_unichar_t;

#include "cp_atari.h"
#include "cp_1250.h"
#include "cp_1251.h"
#include "cp_1252.h"

static void gentab(const unsigned char (*const table[256])[256])
{
	int i;
	unsigned char atari;
	unsigned char cp;
	h_unichar_t uni;
	
	for (i = 0x80; i <= 0xff; i++)
	{
		atari = i;
		uni = atari_to_unicode[atari];
		cp = (*table[uni >> 8])[uni & 0xff];
		if (cp == 0xff && atari != 0x98)
			continue;
		if (atari == cp)
			continue;
		printf("/%c/%c/g\n", atari, cp);
	}
}


int main(int argc, char **argv)
{
	int cp;
	if (argc != 2)
		return 1;
	cp = strtol(argv[1], NULL, 0);
	switch (cp)
	{
		case 1250: gentab(utf16_to_cp1250); break;
		case 1251: gentab(utf16_to_cp1251); break;
		case 1252: gentab(utf16_to_cp1252); break;
		default: return 1;
	}
	return 0;
}
