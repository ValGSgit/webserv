#include "../../includes/http/HttpResponse.hpp"
#include "../../includes/utils/Utils.hpp"

HttpResponse::HttpResponse() : _status(HTTP_OK), _headers_sent(false) {
    setDefaultHeaders();
}

HttpResponse::~HttpResponse() {}

void HttpResponse::setStatus(int status) {
    _status = status;
}

// RFC 7230 Section 3.2 - Header Fields
void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    _headers[key] = value;
}

// RFC 7230 Section 3.3.2 - Content-Length
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
#ifdef BONUS
    _set_cookie_headers.clear();
#endif
    _body.clear();
    _response_string.clear();
    _headers_sent = false;
    setDefaultHeaders();
}

// RFC 7230 Section 3.2 - Header Fields
// RFC 7231 Section 7.1.1.2 - Date header field
void HttpResponse::setDefaultHeaders() {
    // RFC 7231 Section 7.4.2 - Server header (optional but common)
    // SECURITY: Don't expose version information
    _headers["Server"] = "WebServ";
    
    // RFC 7231 Section 7.1.1.2 - Date header SHOULD be sent in all responses
    // Date format per RFC 7231 Section 7.1.1.1 uses IMF-fixdate format
    std::time_t time = std::time(NULL);
    char buffer[30];
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S", std::localtime(&time));
    _headers["Date"] = buffer;
    
    // RFC 7230 Section 6.1 - Connection header (we use close, not persistent)
    _headers["Connection"] = "close";
    
    // SECURITY: Added security headers for browser protection
    // These are not required by HTTP/1.1 but recommended for security
    _headers["X-Content-Type-Options"] = "nosniff";
    _headers["X-Frame-Options"] = "DENY";
    _headers["X-XSS-Protection"] = "1; mode=block";
    _headers["Referrer-Policy"] = "no-referrer";
}

// RFC 7231 Section 3.1.1.5 - Content-Type
void HttpResponse::setContentType(const std::string& content_type) {
    _headers["Content-Type"] = content_type;
}

// RFC 7230 Section 3.3.2 - Content-Length
// The Content-Length header field indicates the size of the message body
void HttpResponse::setContentLength(size_t length) {
    _headers["Content-Length"] = Utils::toString(length);
}

#ifdef BONUS
/**
 * Sets a cookie in the HTTP response
 * Creates a Set-Cookie header with the specified parameters
 *
 * @param name Cookie name
 * @param value Cookie value
 * @param max_age Expiration time in seconds (-1 for session cookie)
 * @param path Cookie path (default: "/")
 * @param http_only If true, cookie is not accessible via JavaScript
 * @param secure If true, cookie only sent over HTTPS
 */
void HttpResponse::setCookie(const std::string& name, const std::string& value,
                              int max_age, const std::string& path,
                              bool http_only, bool secure) {
    std::string cookie = name + "=" + value;
    cookie += "; Path=" + path;

    if (max_age >= 0) {
        cookie += "; Max-Age=" + Utils::toString(max_age);
    }
    if (http_only) {
        cookie += "; HttpOnly";
    }
    if (secure) {
        cookie += "; Secure";
    }

    _set_cookie_headers.push_back(cookie);
}

/**
 * Clears a cookie by setting it with Max-Age=0
 * This tells the browser to delete the cookie
 *
 * @param name Cookie name to clear
 */
void HttpResponse::clearCookie(const std::string& name) {
    setCookie(name, "", 0);
}
#endif

std::string HttpResponse::statusToString(int status) const {
    return Utils::toString((int)status) + " " + Utils::getStatusMessage((int)status);
}

// RFC 7230 Section 3.1.2 - Status Line Format
// status-line = HTTP-version SP status-code SP reason-phrase CRLF
// RFC 7230 Section 3.2 - Header Fields Format
// RFC 7230 Section 3 - Message body follows headers after CRLF CRLF
void HttpResponse::buildResponseString() {
    // RFC 7230 Section 3.1.2 - Status line
    _response_string = "HTTP/1.1 " + statusToString(_status) + "\r\n";

    // RFC 7230 Section 3.2 - Header fields
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end(); ++it) {
        _response_string += it->first + ": " + it->second + "\r\n";
    }

#ifdef BONUS
    // RFC 6265 - HTTP State Management Mechanism (Cookies)
    // Multiple Set-Cookie headers are allowed
    for (size_t i = 0; i < _set_cookie_headers.size(); ++i) {
        _response_string += "Set-Cookie: " + _set_cookie_headers[i] + "\r\n";
    }
#endif

    // RFC 7230 Section 3 - Empty line separates headers from body
    _response_string += "\r\n" + _body;
}

// RFC 7231 Section 6 - Response Status Codes
// Creates a response with a specific status and message
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

// RFC 7231 Section 6 - Response Status Codes
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

// RFC 7231 Section 6.4 - Redirection 3xx Status Codes
// RFC 7231 Section 7.1.2 - Location header field
HttpResponse HttpResponse::redirectResponse(const std::string& location, int status_code) {
    HttpResponse response;
    response.setStatus(status_code);
    response.setHeader("Location", location);
    response.setBody("");
    return response;
}

// RFC 7231 Section 4.3.7 - OPTIONS Method
// Returns a response indicating the allowed methods for a resource
HttpResponse HttpResponse::optionsResponse(const std::vector<std::string>& allowed_methods) {
    HttpResponse response;
    response.setStatus(HTTP_OK);
    
    // RFC 7231 Section 7.4.1 - Allow header field
    // The Allow header field lists the set of methods advertised as supported by the target resource
    std::string allow_header;
    for (size_t i = 0; i < allowed_methods.size(); ++i) {
        if (i > 0) allow_header += ", ";
        allow_header += allowed_methods[i];
    }
    response.setHeader("Allow", allow_header);
    
    // OPTIONS responses typically have no body, but Content-Length: 0 is good practice
    response.setBody("");
    
    return response;
}

// RFC 7231 Section 4.3.2 - HEAD Method
// The HEAD method is identical to GET except that the server MUST NOT
// send a message body in the response
// IMPORTANT: Content-Length header should reflect what GET would have returned
void HttpResponse::removeBody() {
    // Keep Content-Length as it was set (reflecting the body size)
    // Just clear the body content
    _body.clear();
    // Note: We do NOT update Content-Length here - it should remain as it was
}

void HttpResponse::print() const {
    std::cout << "╔══════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║            HTTP RESPONSE                             ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════╝" << std::endl;
    
    std::cout << "Status: " << _status << " - " << statusToString(_status) << std::endl;
    std::cout << "Headers Sent: " << (_headers_sent ? "yes" : "no") << std::endl;
    
    std::cout << "\nHeaders (" << _headers.size() << "):" << std::endl;
    if (_headers.empty()) {
        std::cout << "  (no headers)" << std::endl;
    } else {
        for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); 
             it != _headers.end(); ++it) {
            std::cout << "  " << it->first << ": " << it->second << std::endl;
        }
    }
    
#ifdef BONUS
    if (!_set_cookie_headers.empty()) {
        std::cout << "\nSet-Cookie Headers (" << _set_cookie_headers.size() << "):" << std::endl;
        for (size_t i = 0; i < _set_cookie_headers.size(); ++i) {
            std::cout << "  " << _set_cookie_headers[i] << std::endl;
        }
    }
#endif
    
    std::cout << "\nBody Size: " << _body.size() << " bytes" << std::endl;
    if (!_body.empty() && _body.size() <= 500) {
        std::cout << "Body Preview:" << std::endl;
        std::cout << "---" << std::endl;
        std::cout << _body.substr(0, 500);
        std::cout << std::endl << "---" << std::endl;
    } else if (!_body.empty()) {
        std::cout << "Body Preview (first 500 chars):" << std::endl;
        std::cout << "---" << std::endl;
        std::cout << _body.substr(0, 500) << "...";
        std::cout << std::endl << "---" << std::endl;
    }
    
    if (!_response_string.empty()) {
        std::cout << "\nFull Response Size: " << _response_string.size() << " bytes" << std::endl;
    }
    std::cout << std::endl;
}
