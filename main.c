#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "global_values.h"
#include "cmd_parse.h"

int G_Work;
int G_Update;

void SignalHandler(int signum)
{
	if (signum == SIGUSR2) {
		G_Work = 0;
	} else {
		G_Update = 1;
	}
}

int StartProcess(char* exe, char** args, char* workDir)
{
	printf("Starting %s.\n", exe);

	int pid = fork();

	if (pid == 0) {
		if (workDir) {
			chdir(workDir);
			printf("Set working directory: %s.\n", workDir);
		}

		execvp(exe, args);
		printf("Start unsuccessful: %s.\n", strerror(errno));
		exit(10);
	}

	return pid;
}

int WaitProcess(int pid)
{
	int status;
	waitpid(pid, &status, 0);

	if (WIFEXITED(status)) {
		return WEXITSTATUS(status);
	} else if (WIFSIGNALED(status)) {
		return WTERMSIG(status);
	} else {
		return -1;
	}
}

int StartSteam()
{
	char* args[10];
	args[0] = "steamcmd";
	args[1] = "+login";
	args[2] = "anonymous";
	args[3] = "+app_update";
	args[4] = "896660";
	args[5] = "+quit";
	args[6] = NULL;

	char* home = getenv("HOME");

	return StartProcess(args[0], args, home);
}

int StartServer(
	char* serverName,
	char* worldName,
	char* password)
{
	char* args[10];
	args[0] = "./valheim_server.x86_64";
	args[1] = "-name";
	args[2] = serverName;
	args[3] = "-port";
	args[4] = "2456";
	args[5] = "-world";
	args[6] = worldName;
	args[7] = "-password";
	args[8] = password;
	args[9] = NULL;

	char* home = getenv("HOME");
	char* pathToServer = "/Steam/steamapps/common/Valheim dedicated server";

	char* dir = malloc(strlen(home) + strlen(pathToServer) + 1);
	strcpy(dir, home);
	strcat(dir, pathToServer);

	int pid = StartProcess(args[0], args, dir);

	free(dir);
	return pid;
}

int Update(
	int serverPid,
	char* serverName,
	char* worldName,
	char* password)
{
	kill(serverPid, SIGINT);
	WaitProcess(serverPid);

	int steamPid = StartSteam();
	WaitProcess(steamPid);

	return StartServer(serverName, worldName, password);
}

void SetSignalAction()
{
	struct sigaction sigact;
	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = SignalHandler;

	sigaction(SIGUSR1, &sigact, NULL);
	sigaction(SIGUSR2, &sigact, NULL);
}

void Daemonize()
{
	int pidFileFd = open("/tmp/vu_pidfile", O_RDONLY);

	if (pidFileFd >= 0) {
		printf("Daemon is already running.\n");
		exit(10);
	}

	if (fork()) {
		exit(0);
	}

	if (setsid() < 0) {
		exit(1);
	}

	int nullFd = open("/dev/null", O_RDONLY);
	dup2(nullFd, 0);
	close(nullFd);
	chdir("/");
}

void WritePidfile()
{
	int selfPid = getpid();

	int pidFileFd = open(
		"/tmp/vu_pidfile",
		O_WRONLY | O_CREAT | O_TRUNC | O_EXCL,
		0444);

	if (pidFileFd < 0) {
		exit(10);
	}

	write(pidFileFd, &selfPid, sizeof(int));
	close(pidFileFd);
}

void InitLog(char* logFile)
{
	int logFd = -1;

	if (logFile) {
		logFd = open(logFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	}

	if (logFd < 0) {
		logFd = open("/dev/null", O_WRONLY);
	}

	dup2(logFd, 1);
	dup2(logFd, 2);
	close(logFd);
}

int InitSocket()
{
	int sockFd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sockFd == -1) {
		printf("Failed to create socket.\n");
		printf("%s.\n", strerror(errno));
		return -1;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(DGRAM_PORT_NUMBER);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int stat = bind(sockFd, (struct sockaddr*)&addr, sizeof(addr));

	if (stat == -1) {
		printf("Failed to bind socket.\n");
		printf("%s.\n", strerror(errno));
		close(sockFd);
		return -1;
	}

	return sockFd;
}

int main(int argc, char** argv)
{
	G_Work = 1;
	G_Update = 0;

	struct CmdData cmd = ParseCmd(argc, argv);
	PrintCmd(cmd);

	char* serverName = GetCmdValue(cmd, "server");
	char* worldName = GetCmdValue(cmd, "world");
	char* password = GetCmdValue(cmd, "password");
	char* logFile = GetCmdValue(cmd, "log");

	int argsCorrect = 1;

	if (serverName == NULL) {
		printf("Server name is not specified. Use '-server' key.\n");
		argsCorrect = 0;
	}

	if (worldName == NULL) {
		printf("World name is not specified. Use '-world' key.\n");
		argsCorrect = 0;
	}

	if (password == NULL) {
		printf("Password is not specified. Use '-password' key.\n");
		argsCorrect = 0;
	}

	if (logFile == NULL) {
		printf("Log file is not specified. Log will not be written.\n");
		printf("Use '-log' key to set log file.\n");
	}

	if (!argsCorrect) {
		return 1;
	}

	Daemonize();
	WritePidfile();
	SetSignalAction();
	InitLog(logFile);

	setenv("LD_LIBRARY_PATH", "./linux64:", 1);

	printf("started Valheim control daemon.\n");

	int sockFd = InitSocket();

	int serverPid = StartServer(serverName, worldName, password);

	while (1) {
		if (sockFd != -1) {
			int buf;
			int msglen = recv(sockFd, &buf, sizeof(buf), 0);

			if (msglen == sizeof(buf)) {
				if (buf == UPDATE_COMMAND) {
					G_Update = 1;
				} else if (buf == SHUTDOWN_COMMAND) {
					G_Work = 0;
				}
			}
		} else {
			pause();
		}

		if (!G_Work) {
			break;
		}

		if (G_Update) {
			serverPid = Update(
				serverPid,
				serverName,
				worldName,
				password);

			G_Update = 0;
		}
	}

	kill(serverPid, SIGINT);
	WaitProcess(serverPid);

	close(sockFd);

	unlink("/tmp/vu_pidfile");

	FreeCmd(cmd);

	return 0;
}
