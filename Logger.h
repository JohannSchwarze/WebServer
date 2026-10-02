#include <string>
#include <fstream>
#include <iostream>
#include <thread>
#include <mutex>

#ifndef LOGGER_H
#define LOGGER_H

#define LOG(s) Logger::getInstance().write(s)

class Logger {
	private:
		std::mutex writelock;

		Logger();
		~Logger();
		Logger& operator=(const Logger&)= delete;
		Logger(const Logger&)= delete;
	public:
		static std::string filename;

		static Logger& getInstance() {
			static Logger instance;
			return instance;
		}	

		void write(std::string s);
};

#endif /*LOGGER_H*/
