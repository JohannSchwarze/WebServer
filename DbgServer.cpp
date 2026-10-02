#include <iostream>

#include "DbgServer.h"

DbgServer::DbgServer() {

}
DbgServer::~DbgServer() {

}

void DbgServer::sendMsgOverConnection(Connection *c, std::vector<uint8_t> msg) {
	Server::sendMsgOverConnection(c, msg);
}

void DbgServer::receivedCharOverConnection(Connection *c, uint8_t ch) {
	Server::receivedCharOverConnection(c, ch);

	const char c_str[] = {ch, '\0'};
	std::string reply(c_str);

	this->sendMsgOverConnection(c, std::vector<uint8_t>(reply.begin(), reply.end()));
}
