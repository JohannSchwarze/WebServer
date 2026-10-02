#include <chrono>
#include <ctime>
#include <time.h>
#include <sys/time.h>

#include "Logger.h"

Logger::Logger() {
	this->write("========== Started logging =========");
}

Logger::~Logger() {
	this->write("========== Stopped logging ==========");
}


std::string Logger::filename = "";

void Logger::write(std::string s) {
	if (Logger::filename.empty()) {
		return;
	}
	this->writelock.lock();
	
	char logtimebuffer[1000];

	time_t logtime= time(NULL);
	struct tm *p = localtime(&logtime);
	strftime(logtimebuffer, 1000, "%a %b %d %Y %T", p);
	std::string logtimestr(logtimebuffer);

	struct timeval tv;
	gettimeofday(&tv, NULL);
	long usec = tv.tv_usec - tv.tv_sec*1000000;

	pthread_t tid = pthread_self();

	std::ofstream logfile;
	logfile.open(this->filename, std::ios::out | std::ios::app);
	logfile << '[' << logtimestr << ":" << tv.tv_usec << "](" << tid << ") " << s << "\n";
	logfile.close();

	this->writelock.unlock();
}

