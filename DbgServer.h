#include "Server.h"

#ifndef DBG_SERVER_H
#define DBG_SERVER_H

class DbgServer : public Server {
	private:
	protected:
	public:
		DbgServer();
		~DbgServer();

		virtual void sendMsgOverConnection(Connection *c, std::vector<uint8_t> msg);
		virtual void receivedCharOverConnection(Connection *c, uint8_t ch);
};

#endif /*DBG_SERVER_H*/
