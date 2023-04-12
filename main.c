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
	args[0] = "./steamcmd";
	args[1] = "+login";
	args[2] = "anonymous";
	args[3] = "+app_update";
	args[4] = "896660";
	args[5] = "+quit";
	args[6] = NULL;

	return StartProcess(args[0], args, "/home/yaroslav");
}

int StartServer()
{
	char* args[10];
	args[0] = "./server";
	args[1] = "-name";
	args[2] = "Server cool";
	args[3] = "-world";
	args[4] = "brandnewworld";
	args[5] = "-port";
	args[6] = "2456";
	args[7] = "-password";
	args[8] = "gmrules";
	args[9] = NULL;

	return StartProcess(args[0], args, "/home/yaroslav");
}

int Update(int serverPid)
{
	kill(serverPid, SIGINT);
	WaitProcess(serverPid);

	int steamPid = StartSteam();
	WaitProcess(steamPid);
	return StartServer();
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

	if (argc > 1) {
		logFile = argv[1];
		printf("Log: %s.\n", argv[1]);
	}

	Daemonize();
	WritePidfile();
	SetSignalAction();
	InitLog(logFile);

	printf("started Valheim control daemon.\n");

	int serverPid = StartServer();

	while (1) {
		pause();

		if (!Work) {
			break;
		}

		serverPid = Update(serverPid);
	}

	kill(serverPid, SIGINT);
	WaitProcess(serverPid);

	unlink("/tmp/vu_pidfile");

	return 0;
}
