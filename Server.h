#include <vector>
#include <map>
#include <queue>
#include <deque>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>

#include "Logger.h"

#ifndef SERVER_H
#define SERVER_H


const int MAX_BACKLOG = 5;

void *thread_acceptLoop(void *server_ptr);
void *thread_faultyCleanupLoop(void *server_ptr);
void *thread_readLoop(void *conn_ptr);



class Server;

class Connection {
	private:
		Server *server;

		int sockfd;
		std::shared_ptr<std::thread> readLoopThread;
		bool isReading;
		bool shouldStopReading;


		std::string strSockfd;
	public:
		Connection(Server *server, int sockfd);
		virtual ~Connection();

		float secReadTimeout;

		void startReading();
		virtual void didStartReading();
		void stopReading();
		virtual void didStopReading();

		virtual void sendMsg(std::vector<uint8_t> msg);
		virtual void receivedChar(uint8_t ch);

		Server *getServer();
		int getSockfd();
		bool getIsReading();
		bool getShouldStopReading();
};

class Server {
	private:
		int mainSocket;
		int portno;
		bool isRunning;

		std::shared_ptr<std::thread> acceptLoopThread;
		bool isAccepting;
		bool shouldStopAccepting;

		std::shared_ptr<std::thread> faultyCleanupLoopThread;
		std::condition_variable needsConnectionCleanup;
		std::mutex mtx_needsConnectionCleanup;
		bool isCleaning;
		bool shouldStopCleaning;
	
		std::map<int, Connection*> connections;
		std::deque<Connection *> faultyConnections;
		std::mutex mtx_connections;
		std::mutex mtx_faultyConnections;

		void addConnection(Connection *connection);
		void removeConnection(int sockfd);
		void pushFaultyConnection(Connection *connection);


		void logConnections();
		void logFaultyConnections();
	protected:
	public:
		float secAcceptTimeout;
		float secCleanupTimeout;

		Server();
		virtual ~Server();
		int startServer(int portno);
		int stopServer();

		void startAccepting();
		virtual void didStartAccepting();
		void stopAccepting();
		virtual void didStopAccepting();
		virtual Connection *instantiateConnection(int sockfd);
		virtual void didAcceptConnection(Connection *c);

		void startFaultyCleanup();
		virtual void didStartFaultyCleanup();
		void stopFaultyCleanup();
		virtual void didStopFaultyCleanup();
		virtual void connectionBecameFaulty(Connection *c);

		virtual void sendMsgOverConnection(Connection *c, std::vector<uint8_t> msg);
		virtual void receivedCharOverConnection(Connection *c, uint8_t ch);

		friend void *thread_acceptLoop(void* server_ptr);	
		friend void *thread_faultyCleanupLoop(void* server_ptr);
		
		int getMainSocketFD();
		int getNumberOfConnections();
		int getNumberOfFaultyConnections();
		bool getIsRunning();
		bool getIsAccepting();
		bool getShouldStopAccepting();
		bool getIsCleaning();
		bool getShouldStopCleaning();
};

#endif /*SERVER_H*/
