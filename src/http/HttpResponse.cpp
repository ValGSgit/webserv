#include "../../includes/http/HttpResponse.hpp"
#include "../../includes/utils/Utils.hpp"

HttpResponse::HttpResponse() : _status(HTTP_OK), _headers_sent(false) {
    setDefaultHeaders();
}

HttpResponse::~HttpResponse() {}

void HttpResponse::setStatus(int status) {
    _status = status;
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    _headers[key] = value;
}

void HttpResponse::setBody(const std::string& body) {
    _body = body;
    setContentLength(_body.length());
}

void HttpResponse::appendBody(const std::string& data) {
    _body += data;
    setContentLength(_body.length());
}

int HttpResponse::getStatus() const {
    return _status;
}

const std::string& HttpResponse::getBody() const {
    return _body;
}

const std::string& HttpResponse::getResponseString() {
    if (_response_string.empty()) {
        buildResponseString();
    }
    return _response_string;
}

void HttpResponse::reset() {
    _status = HTTP_OK;
    _headers.clear();
    _body.clear();
    _response_string.clear();
    _headers_sent = false;
    setDefaultHeaders();
}

void HttpResponse::setDefaultHeaders() {
    _headers["Server"] = "WebServ/1.0"; // 1.1?
    std::time_t time = std::time(NULL); // or just implement a member function for future use?
    char buffer[30];
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S", std::localtime(&time)); // GMT + 2 for Vienna time?
    _headers["Date"] = buffer;
    _headers["Connection"] = "close";
}

void HttpResponse::setContentType(const std::string& content_type) {
    _headers["Content-Type"] = content_type;
}

void HttpResponse::setContentLength(size_t length) {
    _headers["Content-Length"] = Utils::toString(length);
}

std::string HttpResponse::statusToString(int status) {
    return Utils::toString((int)status) + " " + Utils::getStatusMessage((int)status);
}

void HttpResponse::buildResponseString() {
    _response_string = "HTTP/1.1 " + statusToString(_status) + "\r\n";
    
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end(); ++it) {
        _response_string += it->first + ": " + it->second + "\r\n";
    }
    
    _response_string += "\r\n" + _body;
}

HttpResponse HttpResponse::messageResponse(int status, const std::string& title, const std::string& message) {
    HttpResponse response;
    response.setStatus(status);
    response.setContentType("text/html");
    
    std::string body = "<!DOCTYPE html><html><head><title>" + title + "</title></head><body>";
    body += "<h1>" + title + "</h1>";
    if (!message.empty()) {
        body += "<p>" + message + "</p>";
    }
    body += "</body></html>";
    
    response.setBody(body);
    return response;
}

HttpResponse HttpResponse::errorResponse(int status, const std::string& message) {
    HttpResponse response;
    response.setStatus(status);
    response.setContentType("text/html");
    
    std::string body = "<!DOCTYPE html><html><head><title>Error " + 
                      Utils::toString((int)status) + "</title></head><body>";
    body += "<h1>Error " + Utils::toString((int)status) + "</h1>";
    if (!message.empty()) {
        body += "<p>" + message + "</p>";
    }
    body += "</body></html>";
    
    response.setBody(body);
    return response;
}

HttpResponse HttpResponse::fileResponse(const std::string& filepath) {
    HttpResponse response;
    
    if (!Utils::fileExists(filepath) || !Utils::isReadable(filepath)) {
        return errorResponse(HTTP_NOT_FOUND);
    }
    std::string content = Utils::readFile(filepath);
    std::string mime_type = Utils::getMimeType(filepath);
    
    response.setContentType(mime_type);
    response.setBody(content);
    
    return response;
}

HttpResponse HttpResponse::directoryListingResponse(const std::string& path, const std::string& uri) {
    HttpResponse response;
    
    if (!Utils::isDirectory(path)) {
        return errorResponse(HTTP_NOT_FOUND);
    }
    
    std::vector<std::string> files = Utils::listDirectory(path);
    
    std::string body = "<!DOCTYPE html><html><head><title>Directory listing for " + uri + "</title>";
    body += "<style>body{font-family:Arial,sans-serif;margin:40px;}h1{color:#333;}ul{list-style:none;}</style>";
    body += "</head><body><h1>Directory listing for " + uri + "</h1><ul>";
    
    if (uri != "/") {
        body += "<li><a href=\"../\">../</a></li>";
    }
    
    for (size_t i = 0; i < files.size(); ++i) {
        std::string file_path = Utils::joinPath(path, files[i]);
        std::string display_name = files[i];
        if (Utils::isDirectory(file_path)) {
            display_name += "/";
        }
        body += "<li><a href=\"" + display_name + "\">" + display_name + "</a></li>";
    }
    
    body += "</ul></body></html>";
    
    response.setContentType("text/html");
    response.setBody(body);
    
    return response;
}

HttpResponse HttpResponse::redirectResponse(const std::string& location, int status_code) {
    HttpResponse response;
    response.setStatus(status_code);
    response.setHeader("Location", location);
    response.setBody("");
    return response;
}
