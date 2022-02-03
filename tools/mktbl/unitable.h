#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define UNDEF 0xffffu
#define F_REPLACEMENT_CHAR 0x20ffu

extern unsigned char const atari_toupper[][256];
extern unsigned short const atari_to_unicode[][256];

const char *unicode_name(unsigned short unicode);
int unitable_selfcheck(void);

#ifdef WITH_CHECKS
int unifont_check(void);
#endif
