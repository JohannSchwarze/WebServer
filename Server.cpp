#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <strings.h>
#include <arpa/inet.h>
#include <string>
#include <sstream>
#include <sys/select.h>
#include <sys/time.h>
#include <chrono>

#include "Server.h"

Connection::Connection(Server *server, int sockfd) {
	this->server = server;
	this->sockfd = sockfd;
	this->strSockfd = std::to_string(sockfd);
	this->secReadTimeout = 1.0f;
	this->isReading = false;
	this->shouldStopReading = false;
	this->readLoopThread = nullptr;
}

Connection::~Connection() {
	this->stopReading();
	close(this->sockfd);
}



void Connection::startReading() {
	if (this->isReading) {
		return;
	}
	this->readLoopThread = std::make_shared<std::thread>(std::thread(thread_readLoop, this));
}

void Connection::didStartReading() {
	this->isReading = true;
	LOG("c" + this->strSockfd + ": Started reading");
}

void Connection::stopReading() {
	if (!this->isReading) {
		return;
	}
	this->shouldStopReading = true;
	this->readLoopThread->join();
}

void Connection::didStopReading() {
	this->shouldStopReading = false;
	this->isReading = false;

	LOG("c" + this->strSockfd + ": Stopped reading");
}



void Connection::sendMsg(std::vector<uint8_t> msg) {
	//this->server->log.write("c" + this->strSockfd + ": Sending msg");
	int n = write(this->sockfd, msg.data(), msg.size());
	if (n < 0) {
		LOG("c" + this->strSockfd + ": Error writing to socket");
	}
}

void Connection::receivedChar(uint8_t ch) {
	//this->server->log.write("c" + this->strSockfd + ": Received char");
	this->server->receivedCharOverConnection(this, ch);
}


Server *Connection::getServer() {
	return this->server;
}

int Connection::getSockfd() {
	return this->sockfd;
}

bool Connection::getIsReading() {
	return this->isReading;
}

bool Connection::getShouldStopReading() {
	return this->shouldStopReading;
}



void *thread_readLoop(void *conn_ptr) {
	Connection *c = (Connection*)conn_ptr;
	Server *server = c->getServer();
	std::string strSockfd = std::to_string(c->getSockfd());

	c->didStartReading();
	while (!c->getShouldStopReading()) {
		int sockfd = c->getSockfd();
		
		fd_set sockfdset;
		FD_ZERO(&sockfdset);
		FD_SET(sockfd, &sockfdset);

		int intpart = (int)c->secReadTimeout;
		int decpart = ((float)intpart - c->secReadTimeout) * 1000000.0f;
		struct timeval timeout;
		timeout.tv_sec = intpart;
		timeout.tv_usec = decpart;
		int sRetVal = select(sockfd+1, &sockfdset, NULL, NULL, &timeout);
		if (sRetVal < 0) {
			LOG("RL - c" + strSockfd + ": Error on select");
			server->connectionBecameFaulty(c);
			continue;
		} else if (sRetVal == 0) {
			//timeout expired
			continue;
		}

		//select woke up without timeout expiring, data can be read
		uint8_t buffer[1];
		buffer[0] = 0;
		int n = read(sockfd, buffer, 1);
		if (n < 0) {
			LOG("RL - c" + strSockfd + ": Error reading from socket");
			server->connectionBecameFaulty(c);
			continue;
		} else if (n == 0) {
			//connection shut down
			LOG("RL - c" + strSockfd + ": Connection shut down");
			server->connectionBecameFaulty(c);
			continue;
		}
		c->receivedChar(buffer[0]);
	}
	LOG("RL - c" + strSockfd + ": terminating...");
	c->didStopReading();
}









Server::Server() {
	this->mainSocket = 0;
       	this->portno = 0;	
	this->isRunning = false;

	this->acceptLoopThread = nullptr;
	this->isAccepting = false;
	this->shouldStopAccepting = false;

	this->faultyCleanupLoopThread = nullptr;
	this->isCleaning = false;
	this->shouldStopCleaning = false;

	this->secAcceptTimeout = 1.0f;
	this->secCleanupTimeout = 1.0f;
}

Server::~Server() {
	this->stopServer();
}

int Server::startServer(int portno) {
	if (this->isRunning) {
		return -1;
	}
	struct sockaddr_in serv_addr;
	std::string strPortno = std::to_string(portno);

	LOG("S: Attempting to start server on port \"" + strPortno + "\"");
	this->mainSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (this->mainSocket < 0) {
		LOG("S: Error opening socket on port \"" + strPortno + "\"");
		return -1;	
	}

	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	int err = bind(this->mainSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (err < 0) {
		LOG("S: Error binding socket on port \"" + strPortno + "\"");
		return -1;
	}
	listen(this->mainSocket, MAX_BACKLOG);

	LOG("S: Started server on port \"" + strPortno + "\"");
	
	this->startFaultyCleanup();
	this->startAccepting();

	this->isRunning = true;
	this->portno = portno;
	return 0;
}

int Server::stopServer() {
	if (!this->isRunning) {
		return -1;
	}
	LOG("S: Attempting to stop server on port \"" + std::to_string(this->portno) + "\"");
	this->stopAccepting();
	this->stopFaultyCleanup();

	this->mtx_connections.lock();
	for (auto it = this->connections.begin(); it != this->connections.end(); it++) {
		Connection *c = (Connection*)it->second;
		c->stopReading();
	}
	for (auto it = this->connections.begin(); it != this->connections.end(); it++) {
		Connection *c = (Connection*)it->second;
		delete c;
	}
	this->mtx_connections.unlock();

	close(this->mainSocket);
	LOG("S: Stopped server on port \"" + std::to_string(this->portno) + "\"");
	this->isRunning = false;
	return 0;

}





void Server::startAccepting() {
	if (this->isAccepting) {
		return;
	}
	this->acceptLoopThread = std::make_shared<std::thread>(std::thread(thread_acceptLoop, this));
}

void Server::didStartAccepting() {
	this->isAccepting = true;

	LOG("S: Started accepting connections");
}

void Server::stopAccepting() {
	if (!this->isAccepting) {
		return;
	}
	this->shouldStopAccepting = true;
	this->acceptLoopThread->join();
}

void Server::didStopAccepting() {
	this->isAccepting = false;
	this->shouldStopAccepting = false;

	LOG("S: Stopped accepting connections");
}

Connection *Server::instantiateConnection(int sockfd) {
	return new Connection(this, sockfd);
}

void Server::didAcceptConnection(Connection *c) {
	this->addConnection(c);
	c->startReading();

	std::string strSockfd = std::to_string(c->getSockfd());
	LOG("S: Accepted connection c" + strSockfd);
}





void Server::startFaultyCleanup() {
	if (this->isCleaning) {
		return;
	}
	this->faultyCleanupLoopThread = std::make_shared<std::thread>(std::thread(thread_faultyCleanupLoop, this));
}

void Server::didStartFaultyCleanup() {
	this->isCleaning = true;
	LOG("S: Started cleaning up faulty connections");
}

void Server::stopFaultyCleanup() {
	if (!this->isCleaning) {
		return;
	}
	this->shouldStopCleaning = true;
	this->faultyCleanupLoopThread->join();
}

void Server::didStopFaultyCleanup() {
	this->isCleaning = false;
	this->shouldStopCleaning = false;
	LOG("S: Stopped cleaning up faulty connections");
}

void Server::connectionBecameFaulty(Connection *c) {
	int sockfd = c->getSockfd();
	this->pushFaultyConnection(c);
	LOG("S: Connection c" + std::to_string(sockfd) + " became faulty");

	this->mtx_needsConnectionCleanup.lock();
	this->needsConnectionCleanup.notify_all();
	this->mtx_needsConnectionCleanup.unlock();
}




void Server::sendMsgOverConnection(Connection *c, std::vector<uint8_t> msg) {
	c->sendMsg(msg);
}

void Server::receivedCharOverConnection(Connection *c, uint8_t ch) {
	
}





void Server::addConnection(Connection *connection) {
	int sockfd = connection->getSockfd();
	this->mtx_connections.lock();

	bool found = false;
	for (auto it = this->connections.begin(); it != this->connections.end(); it++) {
		if (sockfd == it->first) {
			found = true;
			break;
		}
	}
	if (found) {
		this->mtx_connections.unlock();
		return;
	}

	this->connections[sockfd] = connection;
	this->mtx_connections.unlock();
}

void Server::removeConnection(int sockfd) {
	this->mtx_connections.lock();
	if (this->connections.count(sockfd) <= 0) {
		return;
	}
	this->connections.erase(sockfd);
	this->mtx_connections.unlock();
}

void Server::pushFaultyConnection(Connection *connection) {
	this->mtx_faultyConnections.lock();

	bool found = false;
	for (auto it = this->faultyConnections.begin(); it != this->faultyConnections.end(); it++) {
		if (connection == *it) {
			found = true;
			break;
		}
	}
	if (found) {
		this->mtx_faultyConnections.unlock();
		return;
	}
	this->faultyConnections.push_back(connection);
	this->mtx_faultyConnections.unlock();
}




int Server::getMainSocketFD() {
	return this->mainSocket;
}

int Server::getNumberOfConnections() {
	this->mtx_connections.lock();
	int n = this->connections.size();
	this->mtx_connections.unlock();
	return n;
}

int Server::getNumberOfFaultyConnections() {
	this->mtx_faultyConnections.lock();
	int n = this->faultyConnections.size();
	this->mtx_faultyConnections.unlock();
	return n;
}

bool Server::getIsRunning() {
	return this->isRunning;
}

bool Server::getIsAccepting() {
	return this->isAccepting;
}

bool Server::getShouldStopAccepting() {
	return this->shouldStopAccepting;
}

bool Server::getIsCleaning() {
	return this->isCleaning;
}

bool Server::getShouldStopCleaning() {
	return this->shouldStopCleaning;
}




void Server::logConnections() {
	this->mtx_connections.lock();
	std::stringstream ss;
	ss << "{\n";
	for (auto it = this->connections.begin(); it != this->connections.end(); it++) {
		ss << it->first << " - " << it->second << "\n";
	}
	ss << "}";
	LOG(ss.str());
	this->mtx_connections.unlock();
}

void Server::logFaultyConnections() {
	this->mtx_faultyConnections.lock();
	std::stringstream ss;
	ss << "{\n";
	for (auto it = this->faultyConnections.begin(); it != this->faultyConnections.end(); it++) {
		ss << *it << "\n";
	}
	ss << "}";
	LOG(ss.str());
	this->mtx_faultyConnections.unlock();
}






void *thread_acceptLoop(void *server_ptr) {
	Server *server = (Server*)server_ptr;
	server->didStartAccepting();
	while(!server->getShouldStopAccepting()) {
		int sockfd = server->getMainSocketFD();

		fd_set sockfdset;
		FD_ZERO(&sockfdset);
		FD_SET(sockfd, &sockfdset);

		struct timeval timeout;
		int intval = (int)server->secAcceptTimeout;
		int decval = (server->secAcceptTimeout - (float)intval) * 1000000.0f;
		timeout.tv_sec = intval;
		timeout.tv_usec = decval;

		int sRetVal = select(sockfd+1, &sockfdset, NULL, NULL, &timeout);
		if (sRetVal < 0) {
			LOG("AL: Error on select() on mainSocketFD");
			continue;
		} else if (sRetVal == 0) {
			//timeout
			continue;
		}

		struct sockaddr_in cli_addr;
		socklen_t clilen = sizeof(cli_addr);
		int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
		if (newsockfd < 0) {
			LOG("AL: Error on accept");
			continue;
		}

		Connection *c = server->instantiateConnection(newsockfd);
		server->didAcceptConnection(c);
	}
	LOG("AL: terminating...");
	server->didStopAccepting();
}


void *thread_faultyCleanupLoop(void *server_ptr) {
	Server *server = (Server*)server_ptr;
	server->didStartFaultyCleanup();
	while (!server->getShouldStopCleaning()) {

		std::chrono::nanoseconds duration((int)server->secCleanupTimeout * 1000000000);
		std::unique_lock<std::mutex> lk(server->mtx_needsConnectionCleanup);
		std::cv_status retval = server->needsConnectionCleanup.wait_for(lk, duration);
		lk.unlock();

		if (retval != std::cv_status::timeout) {
			//signal received
			LOG("FCL: received cleanup needed signal");
			while (server->getNumberOfFaultyConnections() > 0) {
				server->mtx_faultyConnections.lock();
				Connection *c = server->faultyConnections.front();
				server->mtx_faultyConnections.unlock();
				LOG("FCL: cleaning up connection c" + std::to_string(c->getSockfd()));
				int sockfd = c->getSockfd();
				server->removeConnection(c->getSockfd());
				LOG("FCL: removed connection c" + std::to_string(c->getSockfd()) + " from list");
				delete c;

				LOG("FCL: removing faulty connection c" + std::to_string(c->getSockfd()) + " from faulty list");	
				server->mtx_faultyConnections.lock();
				server->faultyConnections.pop_front();
				server->mtx_faultyConnections.unlock();
				LOG("FCL: removed faulty connection c" + std::to_string(c->getSockfd()) + " from faulty list");	

				LOG("FCL: removing connection c" + std::to_string(c->getSockfd()) + " from list");

				LOG("FCL: Did clean up connection c" + std::to_string(sockfd));
			}
			LOG("FCL: cleanup queue empty");
		} else {
			//timeout
		}

		LOG("FCL: checking should stop cleaning");
	}
	LOG("FCL: terminating...");
	server->didStopFaultyCleanup();
}
