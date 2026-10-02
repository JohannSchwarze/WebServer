#include "Server.h"
#include "HttpMsgModel.h"

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

class HttpConnection : public Connection {
	private:
		std::string messageBuffer;

		void receivedHttpRequest(HttpRequest req);
		void sendHttpResponse_notFound();
		void sendHttpResponse_badRequest();
		void sendHttpResponse_getOK(std::string url);
		void sendHttpResponse_headOK(std::string url);

	protected:
	public:
		HttpConnection(Server *server, int sockfd);
		virtual ~HttpConnection();

		virtual void sendMsg(std::vector<uint8_t> msg);
		virtual void receivedChar(uint8_t ch);
};

class HttpServer : public Server {
	private:
		std::string basepath;
	protected:
	public:
		HttpServer(std::string basepath);
		virtual ~HttpServer();
		virtual Connection *instantiateConnection(int sockfd);

		virtual void sendMsgOverConnection(Connection *c, std::vector<uint8_t> msg);
		virtual void receivedCharOverConnection(Connection *c, uint8_t ch);

		bool resourceExists(std::string url);
		std::vector<uint8_t> fetchResource(std::string url);
};

#endif /*HTTP_SERVER_H*/
