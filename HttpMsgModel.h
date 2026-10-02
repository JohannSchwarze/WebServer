#include <string>
#include <map>

#ifndef HTTP_MSG_MODEL_H
#define HTTP_MSG_MODEL_H

typedef enum HttpRequestParseError {
	REQERR_NONE,
	REQERR_NO_REQLINE,
	REQERR_NO_REQVERB,
	REQERR_NO_URL,
	REQERR_NO_EMPTYLINE,
	REQERR_NO_HEADERNAME,
	REQERR_NO_HEADERVAL
}HttpRequestParseError;
std::string to_string(HttpRequestParseError err);

typedef enum HttpResponseParseError {
	RESERR_NONE,
	RESERR_NO_RESLINE,
	RESERR_NO_PROTOCOL,
	RESERR_NO_ERRCODE,
	RESERR_NO_EMPTYLINE,
	RESERR_NO_HEADERNAME,
	RESERR_NO_HEADERVAL
}HttpResponseParseError;
std::string to_string(HttpResponseParseError err);

class HttpRequest {
	private:
	protected:
	public:
		HttpRequestParseError parseErrCode;
		
		std::string requestMethod;
		std::string url;
		std::string protocol;
		std::map<std::string, std::string> headers;
		std::string body;
};

class HttpResponse {
	private:
	protected:
	public:
		HttpResponseParseError parseErrCode;
		
		std::string protocol;
		std::string errCode;
		std::string reason;
		std::map<std::string, std::string> headers;
		std::string body;
};

class HttpMsgModel {
	private:
	protected:
	public:
		static HttpRequest requestFromString(std::string str);
		static HttpResponse responseFromString(std::string str);
};

#endif /*HTTP_MSG_MODEL_H*/
