
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <mint/mintbind.h>
#include <mint/ssystem.h>

#define DEFAULT "/c/xaaes.km/xaaes.km"

int
main(int argc, char *argv[])
{
	char buf[1024];
	char *path;
	long r;

	if (argc > 1)
	{
		path = getcwd(buf, sizeof(buf));
		if (!path)
		{
			printf("getcwd failed\n");
			return 0;
		}
		strcat(path, "/");
		strcat(path, argv[1]);
	}
	else
		path = DEFAULT;

	printf("load \"%s\"\n", path);
	r = Ssystem(2000, path, 0);
	printf("Ssystem -> %li\n", r);

	pause();
	return 0;
}
