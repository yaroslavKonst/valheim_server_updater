#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "global_values.h"
#include "cmd_parse.h"

int VerifyCommand(char* command)
{
	if (command == NULL) {
		printf("Specify 'stop' or 'update' command via '-command'.\n");
		return 0;
	}

	int correctCommand =
		!strcmp(command, "stop") ||
		!strcmp(command, "update");

	if (!correctCommand) {
		printf("Specify 'stop' or 'update' command.\n");
		return 0;
	}

	return 1;
}

int Local(char* command)
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

	if (!strcmp(command, "stop")) {
		kill(pid, SIGUSR2);
	} else if (!strcmp(command, "update")) {
		kill(pid, SIGUSR1);
	} else {
		printf("Specify 'stop' or 'update' command.\n");
		return 1;
	}

	return 0;
}

int Remote(char* remote, char* command)
{
	int sockFd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sockFd == -1) {
		printf("Failed to create socket.\n");
		printf("%s.\n", strerror(errno));
		return 1;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(DGRAM_PORT_NUMBER);
	if (!inet_aton(remote, &addr.sin_addr)) {
		printf("Specified address %s is invalid.", remote);
		close(sockFd);
		return 1;
	}

	int buf;

	if (!strcmp(command, "stop")) {
		buf = SHUTDOWN_COMMAND;
	} else if (!strcmp(command, "update")) {
		buf = UPDATE_COMMAND;
	} else {
		printf("Specify 'stop' or 'update' command.\n");
		close(sockFd);
		return 1;
	}

	int stat = sendto(
		sockFd,
		&buf,
		sizeof(buf),
		0,
		(struct sockaddr*)&addr,
		sizeof(addr));

	if (stat != sizeof(buf)) {
		printf("Failed to send command.\n");
		printf("%s.\n", strerror(errno));
		close(sockFd);
		return 1;
	}

	close(sockFd);
	return 0;
}

int main(int argc, char** argv)
{
	struct CmdData cmd = ParseCmd(argc, argv);

	char* remote = GetCmdValue(cmd, "remote");
	char* command = GetCmdValue(cmd, "command");

	if (!VerifyCommand(command)) {
		return 1;
	}

	int retVal;

	if (remote == NULL) {
		retVal = Local(command);
	} else {
		retVal = Remote(remote, command);
	}

	FreeCmd(cmd);

	return retVal;
}
