#include "../../includes/http/HttpRequest.hpp"
#include "../../includes/utils/Utils.hpp"

HttpRequest::HttpRequest() 
    : _status(0), _method(METHOD_UNKNOWN), _version("HTTP/1.1"), _headers_complete(false), 
      _body_complete(false), _content_length(0), _chunked(false) {
}

HttpRequest::~HttpRequest() {}

bool HttpRequest::parseRequest(const std::string& data) {
    // Manual parsing without stringstream/getline
    std::vector<std::string> lines = splitIntoLines(data);
    bool first_line = true;
    //size_t body_start = 0;
    size_t header_size = 0;
    
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        
        if (header_size > MAX_HEADER_SIZE || line.size() > MAX_FIELD_SIZE)
            _status = HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE;
        //if (_status) // save some resources
        //    return true;

        // why \r? it's already skipped in splitIntoLines()
        if (line.empty() || line == "\r") {
            _headers_complete = true;
            //body_start = i + 1;
            break;
        }
        
        //parsing the first line
        if (first_line) {
            parseRequestLine(line);
            first_line = false;
        } else {
            parseHeader(line);
        }
        header_size += line.size();
    }

#ifdef BONUS
    // Parse cookies after headers are complete
    if (_headers_complete) {
        parseCookies();
    }
#endif

    // Read body if present
    if (_headers_complete && _content_length > 0) {
        std::size_t needle = data.find("\r\n\r\n");
        if (needle != std::string::npos)
            _body += data.substr(needle + 4);
/*         for (size_t i = body_start; i < lines.size(); ++i) {
            if (i > body_start) _body += "\n";
            _body += lines[i];
        } */
        _body_complete = (_body.length() >= _content_length);
    }
    return _headers_complete;
}

//parsing first line
void HttpRequest::parseRequestLine(const std::string& line) {
    // Manual parsing without istringstream
    std::vector<std::string> tokens = Utils::split(line, ' ');
    if (tokens.size() >= 3) {
        _method = stringToMethod(tokens[0]);
        _uri = tokens[1];
        _version = tokens[2];
        
        // SECURITY FIX: Validate HTTP version
        if (!Utils::isValidHttpVersion(_version)) {
            _status = HTTP_BAD_REQUEST;
            return;
        }
        
        if (_uri.size() > MAX_URI) // need to include domain name?
            _status = HTTP_URI_TOO_LONG;
        else if (_method == METHOD_UNKNOWN)
            _status = HTTP_NOT_IMPLEMENTED;
        else
            parseUri(_uri);
    }
    // to avoid throwing error too early if the header is only partly sent, eg. (printf "GE" ; sleep 30 ; printf "T / HTTP/1.0\r\n\r\n") | nc localhost 8080
/*     else
        _status = HTTP_BAD_REQUEST; */
}

void HttpRequest::parseHeader(const std::string& line) {
    size_t colon_pos = line.find(':');
    if (colon_pos != std::string::npos) {
        std::string key = Utils::trim(line.substr(0, colon_pos));
        std::string value = Utils::trim(line.substr(colon_pos + 1));
        
        // SECURITY FIX: Check for injection in header values
        if (Utils::containsLF(value) || Utils::containsLF(key)) {
            _status = HTTP_BAD_REQUEST;
            return;
        }
        
        std::string key_lower = Utils::toLowerCase(key);
        
        // SECURITY FIX: Detect duplicate Content-Length headers
        if (key_lower == "content-length" || key_lower == "transfer-encoding" || key_lower == "host") {
            if (_headers.find(key_lower) != _headers.end()) {
                // Duplicate critical header
                _status = HTTP_BAD_REQUEST;
                return;
            }
        }
        
        _headers[key_lower] = value;
        
        if (key_lower == "content-length") {
            _content_length = Utils::toSizeT(value);
        }
    }
}

void HttpRequest::parseUri(const std::string& uri) {
    size_t query_pos = uri.find('?');
    if (query_pos != std::string::npos) {
        _query_string = uri.substr(query_pos + 1);
        _uri = uri.substr(0, query_pos);
        parseQueryString(_query_string);
    }
}

HttpMethod HttpRequest::stringToMethod(const std::string& method_str) {
    if (method_str == "GET") return METHOD_GET;
    if (method_str == "POST") return METHOD_POST;
    if (method_str == "PUT") return METHOD_PUT;
    if (method_str == "DELETE") return METHOD_DELETE;
    return METHOD_UNKNOWN;
}

void HttpRequest::parseQueryString(const std::string& query) {
    std::vector<std::string> pairs = Utils::split(query, '&');
    for (size_t i = 0; i < pairs.size(); ++i) {
        size_t eq_pos = pairs[i].find('=');
        if (eq_pos != std::string::npos) {
            std::string key = Utils::urlDecode(pairs[i].substr(0, eq_pos));
            std::string value = Utils::urlDecode(pairs[i].substr(eq_pos + 1));
            _params[key] = value;
        }
    }
}

#ifdef BONUS
/**
 * Parses the Cookie header and stores cookies in the _cookies map
 * Cookie format: "name1=value1; name2=value2; name3=value3"
 */
void HttpRequest::parseCookies() {
    std::string cookie_header = getHeader("Cookie");
    if (cookie_header.empty()) {
        return;
    }

    // Split by semicolon to get individual cookies
    std::vector<std::string> pairs = Utils::split(cookie_header, ';');
    for (size_t i = 0; i < pairs.size(); ++i) {
        std::string pair = Utils::trim(pairs[i]);
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string name = Utils::trim(pair.substr(0, eq_pos));
            std::string value = Utils::trim(pair.substr(eq_pos + 1));
            _cookies[name] = value;
        }
    }
}
#endif

void HttpRequest::reset() {
    _method = METHOD_UNKNOWN;
    _uri.clear();
    _version = "HTTP/1.1";
    _headers.clear();
    _body.clear();
    _query_string.clear();
    _params.clear();
#ifdef BONUS
    _cookies.clear();
#endif
    _headers_complete = false;
    _body_complete = false;
    _content_length = 0;
    _chunked = false;
}

// Getters
HttpMethod HttpRequest::getMethod() const { return _method; }
const std::string& HttpRequest::getUri() const { return _uri; }
const std::string& HttpRequest::getVersion() const { return _version; }
const std::map<std::string, std::string>& HttpRequest::getHeaders() const { return _headers; }
const std::string& HttpRequest::getBody() const { return _body; }
const std::string& HttpRequest::getQueryString() const { return _query_string; }
const std::map<std::string, std::string>& HttpRequest::getParams() const { return _params; }
bool HttpRequest::isHeadersComplete() const { return _headers_complete; }
bool HttpRequest::isBodyComplete() const { return _body_complete; }
size_t HttpRequest::getContentLength() const { return _content_length; }
int HttpRequest::getStatus() const{ return _status; }
bool HttpRequest::isChunked() const { return _chunked; }

std::string HttpRequest::getHeader(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = _headers.find(Utils::toLowerCase(key));
    return (it != _headers.end()) ? it->second : "";
}

std::string HttpRequest::methodToString() const {
    switch (_method) {
        case METHOD_GET: return "GET";
        case METHOD_POST: return "POST";
        case METHOD_PUT: return "PUT";
        case METHOD_DELETE: return "DELETE";
        default: return "UNKNOWN";
    }
}

void HttpRequest::print() const {
    std::cout << "Method: " << methodToString() << std::endl;
    std::cout << "URI: " << _uri << std::endl;
    std::cout << "Version: " << _version << std::endl;
    std::cout << "Query: " << _query_string << std::endl;
    std::cout << "Headers:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); 
         it != _headers.end(); ++it) {
        std::cout << "  " << it->first << ": " << it->second << std::endl;
    }
    std::cout << "Body length: " << _body.length() << std::endl;
}

#ifdef BONUS
/**
 * Gets a specific cookie value by name
 *
 * @param name The cookie name to look up
 * @return The cookie value or empty string if not found
 */
std::string HttpRequest::getCookie(const std::string& name) const {
    std::map<std::string, std::string>::const_iterator it = _cookies.find(name);
    return (it != _cookies.end()) ? it->second : "";
}

/**
 * Gets all cookies from the request
 *
 * @return Reference to the cookies map
 */
const std::map<std::string, std::string>& HttpRequest::getCookies() const {
    return _cookies;
}
#endif

std::vector<std::string> HttpRequest::splitIntoLines(const std::string& content) {
    std::vector<std::string> lines;
    std::string line;
    
    //The HTTP and MIME specs specify that header lines must end with \r\n
    //https://stackoverflow.com/questions/6324167/do-browsers-send-r-n-or-n-or-does-it-depend-on-the-browser
    for (size_t i = 0; i < content.length(); ++i) {
        if (content[i] == '\n') {
            lines.push_back(line);
            line.clear();
        } else if (content[i] != '\r') {  // Skip carriage return
            line += content[i];
        }
    }
    
    // Add last line if it doesn't end with newline
    if (!line.empty()) {
        lines.push_back(line);
    }
    
    return lines;
}
