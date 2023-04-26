#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int Work;

void SignalHandler(int signum)
{
	if (signum == SIGUSR2) {
		Work = 0;
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

int main(int argc, char** argv)
{
	Work = 1;

	char* logFile = NULL;

	if (argc < 4) {
		printf("server server_name world_name password [log_file]\n");
		printf("Server name, world name and password are required.\n");
		return 1;
	}

	char* serverName = argv[1];
	char* worldName = argv[2];
	char* password = argv[3];

	if (argc > 5) {
		printf("server server_name world_name password [log_file]\n");
		printf("Other arguments are not required.\n");
		return 1;
	}

	if (argc == 5) {
		logFile = argv[4];
		printf("Log: %s.\n", argv[4]);
	}

	Daemonize();
	WritePidfile();
	SetSignalAction();
	InitLog(logFile);

	setenv("LD_LIBRARY_PATH", "./linux64:", 1);

	printf("started Valheim control daemon.\n");

	int serverPid = StartServer(serverName, worldName, password);

	while (1) {
		pause();

		if (!Work) {
			break;
		}

		serverPid = Update(serverPid, serverName, worldName, password);
	}

	kill(serverPid, SIGINT);
	WaitProcess(serverPid);

	unlink("/tmp/vu_pidfile");

	return 0;
}
