#include "aestests.h"

int main(int argc, char *argv[])
{
	OBJECT *tree;

	if (0 > appl_init())
		return -1;
	if (argc != 2)
	{
		form_alert(1, "[3][Syntax: RSC_LOAD pathname][Cancel]");
		return 1;
	}
	if (!rsrc_load(argv[1]))
	{
		form_alert(1, "[3][RSC-File not found][Cancel]");
		return 1;
	}
	if (!rsrc_gaddr(R_TREE, 0, &tree))
	{
		form_alert(1, "[3][rsrc_gaddr reports error][Cancel]");
		return 3;
	}
	objc_draw(tree, 0, 8, 0, 0, 640, 400);

	appl_exit();
	return 0;
}
