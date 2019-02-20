#ifndef _BGH_H_
#define _BGH_H_

#define BGH_DIAL	0
#define BGH_ALERT	1
#define BGH_USER	2
#define BGH_MORE	3

void *BGH_load(const char *Name);
void BGH_free(void *bgh_handle);
char *BGH_gethelpstring(void *bgh_handle, int typ, int gruppe, int idx);
void BGH_action(void *bgh_handle, int mx, int my, int typ, int gruppe, int idx);

#endif  /* _BGH_H_ */
