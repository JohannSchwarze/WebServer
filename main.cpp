#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <getopt.h>
#include <errno.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "HttpServer.h"

std::mutex mtx_running;
std::condition_variable cond_running;

void daemonize() {
	pid_t pid = 0;
	int fd;

	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}
	if (setsid() < 0) {
		exit(EXIT_FAILURE);
	}
	signal(SIGCHLD, SIG_IGN);

	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	umask(0);
	chdir("/");

	for (fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--) {
		close(fd);
	}
	stdin = fopen("/dev/null", "r");
	stdout = fopen("/dev/null", "w+");
	stderr = fopen("/dev/null", "w+");
}

void handle_signal(int sig) {
	if (sig == SIGINT) {
		mtx_running.lock();
		cond_running.notify_all();
		mtx_running.unlock();
		signal(SIGINT, SIG_DFL);
	}	
}

int main() {
	Logger::filename = "/home/jjs/prj/socketHttp/httpServer/log.txt";
	daemonize();
	signal(SIGINT, handle_signal);
	
	std::unique_lock<std::mutex> lk(mtx_running);
	HttpServer *server = new HttpServer("/home/jjs/prj/socketHttp/httpServer/");
	server->startServer(12345);
	cond_running.wait(lk);
	server->stopServer();
	delete server;
	lk.unlock();

	return 0;
}
