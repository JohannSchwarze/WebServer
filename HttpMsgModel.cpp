#include <sstream>

#include "HttpMsgModel.h"

std::string to_string(HttpRequestParseError err) {
	if (err == REQERR_NONE) {
		return "REQERR_NONE";
	} else if (err == REQERR_NO_REQLINE) {
		return "REQERR_NO_REQLINE";
	} else if (err == REQERR_NO_REQVERB) {
		return "REQERR_NO_REQVERB";
	} else if (err == REQERR_NO_URL) {
		return "REQERR_NO_URL";
	} else if (err == REQERR_NO_EMPTYLINE) {
		return "REQERR_NO_EMPTYLINE";
	} else if (err == REQERR_NO_HEADERNAME) {
		return "REQERR_NO_HEADERNAME";
	} else if (err == REQERR_NO_HEADERVAL) {
		return "REQERR_NO_HEADERVAL";
	}
	return "";
}

std::string to_string(HttpResponseParseError err) {
	if (err == RESERR_NONE) {
		return "RESERR_NONE";
	} else if (err == RESERR_NO_RESLINE) {
		return "RESERR_NO_RESLINE";
	} else if (err == RESERR_NO_PROTOCOL) {
		return "RESERR_NO_PROTOCOL";
	} else if (err == RESERR_NO_ERRCODE) {
		return "RESERR_NO_ERRCODE";
	} else if (err == RESERR_NO_EMPTYLINE) {
		return "RESERR_NO_EMPTYLINE";
	} else if (err == RESERR_NO_HEADERNAME) {
		return "RESERR_NO_HEADERNAME";
	} else if (err == RESERR_NO_HEADERVAL) {
		return "RESERR_NO_HEADERVAL";
	}
	return "";
}
















HttpRequest HttpMsgModel::requestFromString(std::string str) {
	HttpRequest req;

	//request line
	std::string::size_type n = str.find("\r\n");
	if (n == std::string::npos) {
		req.parseErrCode = REQERR_NO_REQLINE;
		return req;
	}
	std::string reqline = str.substr(0, n);
	str.erase(0, n+2);

	//verb
	n = reqline.find(" ");
	if (n == std::string::npos) {
		req.parseErrCode = REQERR_NO_REQVERB;
		return req;
	}
	std::string verb = reqline.substr(0, n);
	reqline.erase(0, n+1);
	req.requestMethod = verb;

	//url
	n = reqline.find(" ");
	if (n == std::string::npos) {
		req.parseErrCode = REQERR_NO_URL;
		return req;
	}
	std::string url = reqline.substr(0,n);
	reqline.erase(0,n+1);
	req.url = url;

	//protocol
	std::string protocol = reqline;
	req.protocol = protocol;

	//headers
	n = str.find("\r\n");
	if (n == std::string::npos) {
		req.parseErrCode = REQERR_NO_EMPTYLINE;
		return req;
	}
	std::string headerline = str.substr(0, n);
	str.erase(0,n+2);
	while(headerline.length() != 0) {
		//name
		n = headerline.find(":");
		if (n == std::string::npos) {
			req.parseErrCode = REQERR_NO_HEADERNAME;
			return req;
		}
		std::string fieldname = headerline.substr(0, n);
		headerline.erase(0,n+1);

		//value
		std::string value = headerline;
		if (value.front() == ' ') {
			value.erase(0,1);
		}
		if (value.back() == ' ') {
			value.erase(value.end()-1, value.end());
		}
		if (value.length() == 0) {
			req.parseErrCode = REQERR_NO_HEADERVAL;
			return req;
		}
		req.headers[fieldname] = value;

		//next headerline
		n = str.find("\r\n");
		if (n == std::string::npos) {
			req.parseErrCode = REQERR_NO_EMPTYLINE;
			return req;
		}
		headerline = str.substr(0,n);
		str.erase(0,n+2);
	}

	//body
	req.body = str;

	req.parseErrCode = REQERR_NONE;
	return req;
}

HttpResponse HttpMsgModel::responseFromString(std::string str) {
	HttpResponse res;

	//resline
	std::string::size_type n = str.find("\r\n");
	if (n == std::string::npos) {
		res.parseErrCode = RESERR_NO_RESLINE;
		return res;
	}
	std::string resline = str.substr(0,n);
	str.erase(0,n+2);

	//protocol
	n = resline.find(" ");
	if (n == std::string::npos) {
		res.parseErrCode = RESERR_NO_PROTOCOL;
		return res;
	}
	std::string protocol = resline.substr(0,n);
	resline.erase(0,n+1);
	res.protocol = protocol;

	//status code
	n = resline.find(" ");
	if (n == std::string::npos) {
		res.parseErrCode = RESERR_NO_ERRCODE;
		return res;
	}
	std::string errCode = resline.substr(0,n);
	resline.erase(0,n+1);
	res.errCode = errCode;

	//reason phrase
	std::string reason = resline;
	res.reason = reason;

	//headers
	n = str.find("\r\n");
	if (n == std::string::npos) {
		res.parseErrCode = RESERR_NO_EMPTYLINE;
		return res;
	}
	std::string headerline = str.substr(0, n);
	str.erase(0,n+2);
	while(headerline.length() != 0) {
		//name
		n = headerline.find(":");
		if (n == std::string::npos) {
			res.parseErrCode = RESERR_NO_HEADERNAME;
			return res;
		}
		std::string fieldname = headerline.substr(0, n);
		headerline.erase(0,n+1);

		//value
		std::string value = headerline;
		if (value.front() == ' ') {
			value.erase(0,1);
		}
		if (value.back() == ' ') {
			value.erase(value.end()-1, value.end());
		}
		if (value.length() == 0) {
			res.parseErrCode = RESERR_NO_HEADERVAL;
			return res;
		}
		res.headers[fieldname] = value;

		//next headerline
		n = str.find("\r\n");
		if (n == std::string::npos) {
			res.parseErrCode = RESERR_NO_EMPTYLINE;
			return res;
		}
		headerline = str.substr(0,n);
		str.erase(0,n+2);
	}

	//body
	res.body = str;

	res.parseErrCode = RESERR_NONE;
	return res;
}
