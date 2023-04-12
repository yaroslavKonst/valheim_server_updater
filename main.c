#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

int work;

void signal_handler(int signum)
{
	if (signum == SIGUSR2) {
		work = 0;
	}
}

int start_process(char* exe, char** args, char* work_dir)
{
	int pid = fork();

	if (pid == 0) {
		if (work_dir) {
			chdir(work_dir);
		}

		execvp(exe, args);
		exit(10);
	}

	return pid;
}

int wait_process(int pid)
{
	int status;
	waitpid(pid, &status, 0);

	if (WIFEXITED(status)) {
		return WEXITSTATUS(status);
	} else {
		return -1;
	}
}

int start_steam()
{
	char* args[10];
	args[0] = "./steamcmd";
	args[1] = "+login";
	args[2] = "anonymous";
	args[3] = "steamcmd";
	args[4] = "steamcmd";
	args[5] = "steamcmd";
	args[6] = "steamcmd";
	args[7] = "steamcmd";
	args[8] = NULL;

	return start_process(args[0], args, "~");
}

int start_server()
{
	char* args[10];
	args[0] = "./server";
	args[1] = "world";
	args[2] = "qq";
	args[3] = "steamcmd";
	args[4] = "steamcmd";
	args[5] = "steamcmd";
	args[6] = "steamcmd";
	args[7] = "steamcmd";
	args[8] = NULL;

	return start_process(args[0], args, "~");
}

int update(int server_pid)
{
	kill(server_pid, SIGINT);

	wait_process(server_pid);

	int steam_pid = start_steam();
	wait_process(steam_pid);
	return start_server();
}

void set_signal_action()
{
	struct sigaction sigact;
	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = signal_handler;

	sigaction(SIGUSR1, &sigact, NULL);
	sigaction(SIGUSR2, &sigact, NULL);
}

void daemonize()
{
	if (fork()) {
		exit(0);
	}

	if (setsid() < 0) {
		exit(1);
	}

	int null_fd = open("/dev/null", O_RDONLY);
	dup2(null_fd, 0);
	close(null_fd);
}

void write_pidfile()
{
	int self_pid = getpid();

	int pid_file_fd = open("pidfile", O_WRONLY | O_CREAT | O_TRUNC, 0444);
	write(pid_file_fd, &self_pid, sizeof(int));
	close(pid_file_fd);
}

void init_log()
{
	int log_fd = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	dup2(log_fd, 1);
	dup2(log_fd, 2);
	close(log_fd);
}

int main()
{
	work = 1;

	daemonize();
	init_log();
	write_pidfile();
	set_signal_action();

	int server_pid = start_server();

	while (1) {
		pause();

		if (!work) {
			break;
		}

		server_pid = update(server_pid);
	}

	kill(server_pid, SIGINT);
	wait_process(server_pid);

	unlink("pidfile");

	return 0;
}
