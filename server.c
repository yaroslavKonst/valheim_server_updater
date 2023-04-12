#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv)
{
	printf("server started\n");

	char* cwd = getcwd(NULL, 0);
	printf("%s\n", cwd);
	free(cwd);
	
	for (int i = 0; i < argc; ++i) {
		printf("%s\n", argv[i]);
	}

	while (1) {
		printf("server running\n");
		sleep(5);
	}

	printf ("server stopped\n");
	return 0;
}
