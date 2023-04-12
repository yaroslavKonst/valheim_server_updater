#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
	int pidfileFd = open("/tmp/vu_pidfile", O_RDONLY);

	if (pidfileFd < 0) {
		printf("Pidfile not found.\n");
		printf("Server may not be started.\n");
		return 2;
	}

	int pid;

	if (read(pidfileFd, &pid, sizeof(int)) != sizeof(int)) {
		printf("Pidfile is not valid.\n");
		printf("Delete it if server is not running.\n");
		return 3;
	}

	if (argc != 2) {
		printf("Specify 'stop' or 'update' argument.\n");
		return 1;
	}


	if (!strcmp(argv[1], "stop")) {
		kill(pid, SIGUSR2);
	}

	if (!strcmp(argv[1], "update")) {
		kill(pid, SIGUSR1);
	}

	return 0;
}
