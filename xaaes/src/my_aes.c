
#include "global.h"
#include "my_aes.h"
#include "xa_types.h"

short my_global_aes[16];

/*
 * Returns the object number of this object's parent or -1 if it is the root
 */
int
get_parent(OBJECT *t, int object)
{
	if (object)
	{
		int last;

		do {
			last = object;
			object = t[object].ob_next;
		}
		while(t[object].ob_tail != last);

		return object;
	}

	return -1;
}
