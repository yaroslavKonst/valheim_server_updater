#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
	int pidfile_fd = open("/tmp/vu_pidfile", O_RDONLY);

	if (pidfile_fd < 0) {
		printf("pidfile is not found.\n");
		return 2;
	}

	int pid;

	if (read(pidfile_fd, &pid, sizeof(int)) != sizeof(int)) {
		printf("pidfile is not valid.\n");
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
