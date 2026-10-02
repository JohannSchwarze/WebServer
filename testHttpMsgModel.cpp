#include <iostream>

#include "HttpMsgModel.h"

int main() {
	std::string req_msg = "GET /index.html HTTP/1.1\r\nHost: www.example.com\r\n\r\n";
	HttpRequest req = HttpMsgModel::requestFromString(req_msg);
	
	std::cout << req_msg << "\n";
	std::cout << to_string(req.parseErrCode) << "\n";
	std::cout << req.requestMethod << "\n";
	std::cout << req.url << "\n";
	std::cout << req.protocol << "\n";
	for (auto it = req.headers.begin(); it != req.headers.end(); it++) {
		std::cout << it->first << " = " << it->second << "\n";
	}
	std::cout << req.body << "\n";


	std::string res_msg = "HTTP/1.1 404 not found\r\n\r\nThis is the body";
	HttpResponse res = HttpMsgModel::responseFromString(res_msg);
	std::cout << res_msg << "\n";
	std::cout << to_string(res.parseErrCode) << "\n";
       	std::cout << res.protocol << "\n";
	std::cout << res.errCode << "\n";
	std::cout << res.reason << "\n";
	for (auto it = res.headers.begin(); it != res.headers.end(); it++) {
		std::cout << it->first << " = " << it->second << "\n";
	}
	std::cout << res.body << "\n";
	

	return 0;
}
