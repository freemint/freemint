/* $Id: utf8.c,v 1.1 2010/08/28 12:28:16 tom Exp $ */

#include <vttest.h>

int
conv_to_utf8(unsigned char *target, unsigned source, unsigned limit)
{
#define CH(n) CharOf((source) >> ((n) * 8))
  int rc = 0;

  if (source <= 0x0000007f)
    rc = 1;
  else if (source <= 0x000007ff)
    rc = 2;
  else if (source <= 0x0000ffff)
    rc = 3;
  else if (source <= 0x001fffff)
    rc = 4;
  else if (source <= 0x03ffffff)
    rc = 5;
  else          /* (source <= 0x7fffffff) */
    rc = 6;

  if ((unsigned) rc > limit) {  /* whatever it is, we cannot decode it */
    rc = 0;
  }

  if (target != 0) {
    switch (rc) {
    case 1:
      target[0] = CH(0);
      break;

    case 2:
      target[1] = CharOf(0x80 | (CH(0) & 0x3f));
      target[0] = CharOf(0xc0 | (CH(0) >> 6) | ((CH(1) & 0x07) << 2));
      break;

    case 3:
      target[2] = CharOf(0x80 | (CH(0) & 0x3f));
      target[1] = CharOf(0x80 | (CH(0) >> 6) | ((CH(1) & 0x0f) << 2));
      target[0] = CharOf(0xe0 | ((int) (CH(1) & 0xf0) >> 4));
      break;

    case 4:
      target[3] = CharOf(0x80 | (CH(0) & 0x3f));
      target[2] = CharOf(0x80 | (CH(0) >> 6) | ((CH(1) & 0x0f) << 2));
      target[1] = CharOf(0x80 |
                         ((int) (CH(1) & 0xf0) >> 4) |
                         ((int) (CH(2) & 0x03) << 4));
      target[0] = CharOf(0xf0 | ((int) (CH(2) & 0x1f) >> 2));
      break;

    case 5:
      target[4] = CharOf(0x80 | (CH(0) & 0x3f));
      target[3] = CharOf(0x80 | (CH(0) >> 6) | ((CH(1) & 0x0f) << 2));
      target[2] = CharOf(0x80 |
                         ((int) (CH(1) & 0xf0) >> 4) |
                         ((int) (CH(2) & 0x03) << 4));
      target[1] = CharOf(0x80 | (CH(2) >> 2));
      target[0] = CharOf(0xf8 | (CH(3) & 0x03));
      break;

    case 6:
      target[5] = CharOf(0x80 | (CH(0) & 0x3f));
      target[4] = CharOf(0x80 | (CH(0) >> 6) | ((CH(1) & 0x0f) << 2));
      target[3] = CharOf(0x80 | (CH(1) >> 4) | ((CH(2) & 0x03) << 4));
      target[2] = CharOf(0x80 | (CH(2) >> 2));
      target[1] = CharOf(0x80 | (CH(3) & 0x3f));
      target[0] = CharOf(0xfc | ((int) (CH(3) & 0x40) >> 6));
      break;
    }
  }

  return rc;    /* number of bytes needed in target */
#undef CH
}

int
conv_to_utf32(unsigned *target, const char *source, unsigned limit)
{
#define CH(n) CharOf((*target) >> ((n) * 8))
  int rc = 0;
  int j;
  unsigned mask = 0;

  /*
   * Find the number of bytes we will need from the source.
   */
  if ((*source & 0x80) == 0) {
    rc = 1;
    mask = (unsigned) *source;
  } else if ((*source & 0xe0) == 0xc0) {
    rc = 2;
    mask = (unsigned) (*source & 0x1f);
  } else if ((*source & 0xf0) == 0xe0) {
    rc = 3;
    mask = (unsigned) (*source & 0x0f);
  } else if ((*source & 0xf8) == 0xf0) {
    rc = 4;
    mask = (unsigned) (*source & 0x07);
  } else if ((*source & 0xfc) == 0xf8) {
    rc = 5;
    mask = (unsigned) (*source & 0x03);
  } else if ((*source & 0xfe) == 0xfc) {
    rc = 6;
    mask = (unsigned) (*source & 0x01);
  }

  if ((unsigned) rc > limit) {  /* whatever it is, we cannot decode it */
    rc = 0;
  }

  /*
   * sanity-check.
   */
  if (rc > 1) {
    for (j = 1; j < rc; j++) {
      if ((source[j] & 0xc0) != 0x80)
        break;
    }
    if (j != rc) {
      rc = 0;
    }
  }

  if (target != 0) {
    int shift = 0;
    *target = 0;
    for (j = 1; j < rc; j++) {
      *target |= (unsigned) (source[rc - j] & 0x3f) << shift;
      shift += 6;
    }
    *target |= mask << shift;
  }
  return rc;
#undef CH
}
