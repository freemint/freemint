/* ANSI color stuff.
 * Copyright (C) 2001, Guido Flohr <guido@freemint.de>,
 * all rights reserved.
 */

#include <gem.h>

#include "ansicol.h"

const int ansi2vdi[8] = { 1, 2, 3, 6, 4, 7, 5, 0 };
const int vdi2ansi[8] = { 7, 0, 1, 2, 4, 6, 3, 5 };

struct rgb {
	short int red, green, blue;
};

struct renderer {
	int normal;
	int bright;
	int hbright;
	unsigned long bright_effects;
	unsigned long hbright_effects;
} renderer[8];

char* tw52_env = "tw52";
char* tw100_env = "tw100";
char* colorterm_env = "1";

static struct rgb ansi_colors[8 * 3] = {
		{    0,    0,	 0 },	/* Bright black.  */
		{    0,    0,	 0 },	/* Black.  */
		{  408,  408,  408 },	/* Half-bright (dimmed) black.  */
		{ 1000,   94,	94 },	/* Bright red.	*/
		{  698,   94,	94 },	/* Red.  */
		{  408,   94,	94 },	/* Half-bright (dimmed) red.  */
		{   94, 1000,	94 },	/* Bright green.  */
		{   94,  698,	94 },	/* Green.  */
		{   94,  408,	94 },	/* Half-bright (dimmed) green.	*/
#if 1
		/* This is the Linux console "yellow".	*/
		{ 1000, 1000,	94 },	/* Bright yellow.  */
		{  698,  408,	94 },	/* Yellow.  */
		{  408,  329,	94 },	/* Half-bright (dimmed) yellow.  */
#else
		/* This is what it would be when applying the
		 * rules for the other colors.
		 */
		{ 1000, 1000,	94 },	/* Bright yellow.  */
		{  698,  698,	94 },	/* Yellow.  */
		{  408,  408,	94 },	/* Half-bright (dimmed) yellow.  */
#endif
		{   94,   94, 1000 },	/* Bright blue.  */
		{   94,   94,  698 },	/* Blue.  */
		{   94,   94,  408 },	/* Half-bright (dimmed) blue.  */
		{ 1000,   94, 1000 },	/* Bright magenta.  */
		{  698,   94,  698 },	/* Magenta.  */
		{  408,   94,  408 },	/* Half-bright (dimmed) magenta.  */
		{   94, 1000, 1000 },	/* Bright cyan.  */
		{   94,  698,  698 },	/* Cyan.  */
		{   94,  408,  408 },	/* Half-bright (dimmed) cyan.  */
		{ 1000, 1000, 1000 },	/* Bright white.  */
		{  698,  698,  698 },	/* White.  */
		{  408,  408,  408 },	/* Half-bright (dimmed) white.	*/
};

static int ncolors;

static unsigned long int color_diff (const short int color1[3],
				     const short int color2[3]);

void
init_ansi_colors (const short* work_out)
{
	int i;
	struct color_diffs {
		unsigned long diff;
		int index;
	} diffs[8 * 3];

	/* First inquire all colors from vdi.  */
	ncolors = work_out[13];

	for (i = 0; i < 8 * 3; diffs[i].index = 0,
			       diffs[i++].diff = 0xffffffff);

	if (ncolors > 256)
		ncolors = 256;

	if (ncolors < 8) {
		tw52_env = "tw52-m";
		tw100_env = "tw100-m";
		colorterm_env = "0";
	}

	for (i = 0; i < ncolors; ++i) {
		struct rgb rgb;
		int ansi_color;

		vq_color (vdi_handle, i, 1, (short*) &rgb);

		for (ansi_color = 0; ansi_color < 8 * 3; ++ansi_color) {
			unsigned long diff =
				color_diff ((short*) &rgb,
					    (short*)
					    &ansi_colors[ansi_color]);

			if (diff < diffs[ansi_color].diff) {
				diffs[ansi_color].index = i;
				diffs[ansi_color].diff = diff;
			}
		}
	}

	/* Now fill up our renderer.  */
	for (i = 0; i < 8 && i < ncolors; ++i) {
		renderer[i].bright = diffs[3 * i].index;
		renderer[i].normal = diffs[3 * i + 1].index;
		if (renderer[i].bright == renderer[i].normal)
			renderer[i].bright_effects = CE_BOLD;
		else
			renderer[i].bright_effects = 0;
		renderer[i].hbright = diffs[3 * i + 2].index;
		if (renderer[i].hbright == renderer[i].normal)
			renderer[i].hbright_effects = CE_LIGHT;
		else
			renderer[i].hbright_effects = 0;
	}

	for (i = 0; i < 8; ++i) {
		struct rgb rgb;
		int normal, bright, hbright;

		normal = renderer[i].normal;
		bright = renderer[i].bright;
		hbright = renderer[i].hbright;

		debug ("ANSI color #%d:\n", i);
		vq_color (vdi_handle, renderer[i].bright, 1, (short*) &rgb);
		debug ("  Bright:      (%04d|%04d|%04d), effects: 0x%08x\n",
		       rgb.red, rgb.green, rgb.blue, renderer[i].bright_effects);
		vq_color (vdi_handle, renderer[i].normal, 1, (short*) &rgb);
		debug ("  Normal:      (%04d|%04d|%04d), effects: 0x%08x\n",
		       rgb.red, rgb.green, rgb.blue, 0);
		vq_color (vdi_handle, renderer[i].hbright, 1, (short*) &rgb);
		debug ("  Half-bright: (%04d|%04d|%04d), effects: 0x%08x\n",
		       rgb.red, rgb.green, rgb.blue, renderer[i].hbright_effects);
	}
}

void
set_ansi_fg_color (TEXTWIN* tw, int color)
{
	color -= 48;

	if (color == 9) {
		tw->curr_cattr = (tw->curr_cattr & ~CFGCOL) |
			(tw->cfg->fg_color << 4);
	} else if (color >= 0 && color < 8) {
		tw->curr_cattr = (tw->curr_cattr & ~CFGCOL) |
			(color << 4);
	} else if (color == 'M') {
		tw->curr_cattr = (tw->curr_cattr & ~CE_ANSI_EFFECTS) |
			CE_BOLD;
	} else if (color == 'N') {
		tw->curr_cattr = (tw->curr_cattr & ~CE_ANSI_EFFECTS) |
			CE_LIGHT;
	}
}

void
set_ansi_bg_color (TEXTWIN* tw, int color)
{
	color -= 48;

	if (color == 9 || (color >= 0 && color < 8))
		tw->curr_cattr = (tw->curr_cattr & ~CBGCOL) | color;
}

/* Calculate the difference between two colors.  */
static unsigned long int
color_diff (color1, color2)
	const short int color1[3];
	const short int color2[3];
{
	unsigned long int diff = 0;
	unsigned long int r1 = color1[0];
	unsigned long int r2 = color2[0];
	unsigned long int g1 = color1[1];
	unsigned long int g2 = color2[1];
	unsigned long int b1 = color1[2];
	unsigned long int b2 = color2[2];
	unsigned long int max1 = 0;
	unsigned long int max2 = 0;
	unsigned long int min1 = 1000;
	unsigned long int min2 = 1000;
	unsigned long int s1, s2;
	int i;

	diff += (r2 - r1) * (r2 - r1);
	diff += (g2 - g1) * (g2 - g1);
	diff += (b2 - b1) * (b2 - b1);

	for (i = 0; i < 3; i++) {
		if (color1[i] > max1)
			max1 = color1[i];
		if (color2[i] > max2)
			max2 = color2[i];
		if (color1[i] < min1)
			min1 = color1[i];
		if (color2[i] < min2)
			min2 = color2[i];
	}

	s1 = max1 - min1;
	s2 = max2 - min2;

	diff += (s2 - s1) * (s2 - s1);

	return diff;
}

void
use_ansi_colors (TEXTWIN* tw, unsigned long flag,
		int* fgcolor, int* bgcolor,
		int* texteffects)
{
	*bgcolor = flag & CBGCOL;
	*fgcolor = (flag & CFGCOL) >> 4;
	*texteffects = flag & CEFFECTS;

	if (!tw->vdi_colors) {
		*texteffects &= ~CE_ANSI_EFFECTS;

		if (*fgcolor == 9) {
			*fgcolor = tw->cfg->fg_color;
			if (tw->cfg->fg_effects & CE_BOLD)
				flag |= CE_BOLD;
			else if (tw->cfg->fg_effects & CE_LIGHT)
				flag |= CE_LIGHT;
		}

		if (*fgcolor >= 0 && *fgcolor <= 7) {
			*texteffects &= ~CE_ANSI_EFFECTS;

			if (flag & CE_BOLD) {
				*texteffects |=
					renderer[*fgcolor].bright_effects;
				*fgcolor = renderer[*fgcolor].bright;
			} else if (flag & CE_LIGHT) {
				*texteffects |=
					renderer[*fgcolor].hbright_effects;
				*fgcolor = renderer[*fgcolor].hbright;
			} else {
				*fgcolor = renderer[*fgcolor].normal;
			}
		} else {
			*fgcolor &= 0xf;
		}

		if (*bgcolor >= 0 && *bgcolor <= 7) {
			*bgcolor = renderer[*bgcolor].normal;
		} else if (*bgcolor == 9) {
			*bgcolor = tw->cfg->bg_color & 0x7;
			if (tw->cfg->bg_effects & CE_BOLD)
				*bgcolor = renderer[*bgcolor].bright;
			else if (tw->cfg->bg_effects & CE_LIGHT)
				*bgcolor = renderer[*bgcolor].hbright;
			else
				*bgcolor = renderer[*bgcolor].normal;
		} else {
			*bgcolor &= 0xf;
		}
	}

	if (flag & (CINVERSE|CSELECTED)) {
		*fgcolor ^= *bgcolor;
		*bgcolor ^= *fgcolor;
		*fgcolor ^= *bgcolor;
	}

	*texteffects >>= 8;
}

#if 0
int
get_ansi_color (color)
	int color;
{
	if (color < 0 || color > 7)
		return 0;
	else
		return renderer[color].normal;
}
#endif
