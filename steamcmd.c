#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	printf ("steamcmd started\n");

	char* cwd = getcwd(NULL, 0);
	printf("%s\n", cwd);
	free(cwd);

	for (int i = 0; i < argc; ++i) {
		printf("%s\n", argv[i]);
	}

	sleep(5);

	printf ("steamcmd stopped\n");
	return 0;
}
