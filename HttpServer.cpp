#include <sstream>

#include "HttpServer.h"
#include "HttpMsgModel.h"

HttpConnection::HttpConnection(Server *server, int sockfd) : Connection(server, sockfd) {
	this->messageBuffer = "";
}

HttpConnection::~HttpConnection() {

}

void HttpConnection::sendMsg(std::vector<uint8_t> msg) {
	Connection::sendMsg(msg);
}

void HttpConnection::receivedChar(uint8_t ch) {
	Connection::receivedChar(ch);

	const char tmp[] = {ch, '\0'};
	this->messageBuffer.append(tmp);

	for (int i = 0; i < this->messageBuffer.length(); i++) {
		HttpRequest req = HttpMsgModel::requestFromString(this->messageBuffer.substr(i,this->messageBuffer.length()));
		if (req.parseErrCode == REQERR_NONE) {
			this->receivedHttpRequest(req);
			this->messageBuffer.clear();
			break;
		}
	}
}

void HttpConnection::receivedHttpRequest(HttpRequest req) {
	HttpServer *httpServer = (HttpServer*)this->getServer();
	
	if (req.protocol.compare("HTTP/1.1") != 0) {
		LOG("invalid protocol");
		this->sendHttpResponse_badRequest();
		return;
	}
	if (!httpServer->resourceExists(req.url)) {
		LOG("resource does not exist for url " + req.url);
		this->sendHttpResponse_notFound();
		return;
	}

	if (req.requestMethod.compare("GET") == 0) {
		this->sendHttpResponse_getOK(req.url);
	} else if (req.requestMethod.compare("HEAD") == 0) {
		this->sendHttpResponse_headOK(req.url);
	} else {
		LOG("unsupported verb");
		this->sendHttpResponse_badRequest();
		return;
	}
}

void HttpConnection::sendHttpResponse_notFound() {
	std::stringstream ss;
	ss << "HTTP/1.1 404 not found\r\n";
	ss << "Connection: close\r\n";
	ss << "Server: httpServer/0.1\r\n";
	ss << "Content-Length: 0\r\n";
	ss << "\r\n";

	std::string msg = ss.str();
	this->sendMsg(std::vector<uint8_t>(msg.begin(), msg.end()));

}

void HttpConnection::sendHttpResponse_badRequest() {
	std::stringstream ss;
	ss << "HTTP/1.1 400 bad request\r\n";
	ss << "Connection: close\r\n";
	ss << "Server: httpServer/0.1\r\n";
	ss << "Content-Length: 0\r\n";
	ss << "\r\n";

	std::string msg = ss.str();
	this->sendMsg(std::vector<uint8_t>(msg.begin(), msg.end()));
}

void HttpConnection::sendHttpResponse_getOK(std::string url) {
	HttpServer *httpServer = (HttpServer*)this->getServer();
	std::stringstream ss;
	ss << "HTTP/1.1 200 ok\r\n";
	ss << "Connection: close\r\n";
	ss << "Server: httpServer/0.1\r\n";

	std::vector<uint8_t> resource = httpServer->fetchResource(url);
	ss << "Content-Length: " + std::to_string(resource.size()) + "\r\n";

	ss << "\r\n";
	std::string body(resource.begin(), resource.end());
	ss << body;

	std::string msg = ss.str();
	this->sendMsg(std::vector<uint8_t>(msg.begin(), msg.end()));
}

void HttpConnection::sendHttpResponse_headOK(std::string url) {
	HttpServer *httpServer = (HttpServer*)this->getServer();
	std::stringstream ss;
	ss << "HTTP/1.1 200 ok\r\n";
	ss << "Connection: close\r\n";
	ss << "Server: httpServer/0.1\r\n";

	std::vector<uint8_t> resource = httpServer->fetchResource(url);
	ss << "Content-Length: " + std::to_string(resource.size()) + "\r\n";

	ss << "\r\n";

	std::string msg = ss.str();
	this->sendMsg(std::vector<uint8_t>(msg.begin(), msg.end()));
}










HttpServer::HttpServer(std::string basepath) {
	this->basepath = basepath;
}

HttpServer::~HttpServer() {

}

Connection *HttpServer::instantiateConnection(int sockfd) {
	return new HttpConnection(this, sockfd);
}


void HttpServer::sendMsgOverConnection(Connection *c, std::vector<uint8_t> msg) {
	Server::sendMsgOverConnection(c, msg);
}

void HttpServer::receivedCharOverConnection(Connection *c, uint8_t ch) {
	Server::receivedCharOverConnection(c, ch);
}







bool HttpServer::resourceExists(std::string url) {
	std::string fullpath = this->basepath + url;
	std::ifstream input;
	input.open(fullpath);
	bool exists = false;
	if (input) {
		exists = true;
	}
	input.close();
	return exists;
}

std::vector<uint8_t> HttpServer::fetchResource(std::string url) {
	std::string fullpath = this->basepath + url;
	std::ifstream input;
	input.open(fullpath, std::ios::binary | std::ios::ate);
	
	auto size = input.tellg();
	std::string str(size, '\0');
	input.seekg(0);
	input.read(&str[0], size);
	input.close();

	std::vector<uint8_t> res(str.begin(), str.end());
	return res;
}
