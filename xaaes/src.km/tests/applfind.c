#include "aestests.h"

int main(void)
{
	char input[10];
	char c;
	_WORD id, id2;
	long code;
	long i;

	appl_init();
	printf("P = pid->ap_id   A = ap_id->pid   N = Name->ap_id  ");
	fflush(stdout);
	do
	{
		c = 0x5f & Cconin();
	} while (c != 'P' && c != 'A' && (c != 'N'));
	code = c == 'P' ? -1 : -2;
	(void) Cconws("\r\n");

	for (;;)
	{
		fflush(stdout);
		(void) Cconws("ID to convert: ");
		i = Fread(0, 9L, input);
		while (i > 0 && (input[i-1] == '\r' || input[i-1] == '\n'))
			i--;
		input[i] = '\0';
		if (i <= 0)
			break;
		if (c == 'N')
		{
			strupr(input);
			while (strlen(input) < 8)
				strcat(input, " ");
			id = appl_find(input);
			printf("Name \"%s\" ===> apid %d          \n", input, id);
		} else
		{
			id = atoi(input);

			i = (code << 16) | id;

			id2 = appl_find((void *) i);
			if (c == 'A')
				printf("apid %d ===> pid %d          \n", id, id2);
			else
				printf("pid %d ===> apid %d          \n", id, id2);
		}
	}
	
	return 0;
}
