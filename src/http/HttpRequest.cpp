#include "../../includes/http/HttpRequest.hpp"
#include "../../includes/utils/Utils.hpp"

HttpRequest::HttpRequest() 
    : _status(0), _method(METHOD_UNKNOWN), _version("HTTP/1.1"), _headers_complete(false), 
      _body_complete(false), _in_body(false), _content_length(0), _chunked(false) {
}

HttpRequest::~HttpRequest() {}

bool HttpRequest::parseRequest(const std::string& data, char *buffer, ssize_t bytes_read) {
    // Manual parsing without stringstream/getline
    //std::cout << data << std::endl;
    std::vector<std::string> lines = splitIntoLines(data);
    bool first_line = true;
    //size_t body_start = 0;
    size_t header_size = 0;
    size_t header_count = 0;
    //std::cout << "####################------------------------------" << std::endl;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (_headers_complete)
            break ;
        const std::string& line = lines[i];
        //std::cout << line << std::endl;
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
            header_count++;
            if (header_count > MAX_HEADERS) {
                _status = HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE;
                return true;
            }
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

    // HTTP/1.1 requires Host header and it must not be empty
    if (_headers_complete && _version == "HTTP/1.1") {
        std::string host = getHeader("Host");
        if (host.empty()) {
            _status = HTTP_BAD_REQUEST;
            return true;
        }
    }
    
    // SECURITY FIX: If headers are complete but request line was malformed (empty URI), set bad request
    if (_headers_complete && _uri.empty()) {
        _status = HTTP_BAD_REQUEST;
        return true;
    }

    // for chunked request
    if (_headers_complete && _chunked) {
        std::size_t needle = data.find("\r\n\r\n");
        int i = 0;
        if (!_in_body && needle != std::string::npos)
        {
            needle += 4;
            i = needle;
        }
        _in_body = true;
        bool data = false;
        std::string info;
        long line_size = 0;
        long info_dec = 0;
        while (i < bytes_read)
        {
            //std::cout << buffer[i];
            if (buffer[i] == '\r' && buffer[i + 1] == '\n')
            {
                if (!data)
                {
                    data = true;
                    //std::cout << "info = " << info << std::endl;

                    // atoi_base
                    info_dec = std::strtol(info.c_str(), NULL ,16);
                    //info_dec = atoi_base(info);
                    if (info == "0")
                        break ;
                    // reset info
                    info = "";
                }
                else
                {
                    data = false;
                    //std::cout << "info_dec = " << info_dec << std::endl;
                    //std::cout << "line_size = " << line_size << std::endl;
                    // wrong info
                    if (info_dec != line_size)
                    {
                        _status = HTTP_BAD_REQUEST;
                        _body_complete = true;
                        return true;
                    }
                    line_size = 0;
                }
                i += 2;
                continue ;
            }
            if (data)
            {
                _body.push_back(buffer[i]);
                line_size++;
            }
            else
                info += buffer[i];
            i++;
        }
        //std::cout << info << " = info\n";
        if (info == "0")
        {
            //std::cout << "body is complete\n";
            _body_complete = true;
            return true ;
        }
    }

    // Read body if present
    if (_headers_complete && _content_length > 0) {
        std::size_t needle = data.find("\r\n\r\n");
        int i = 0;
        if (!_in_body && needle != std::string::npos)
        {
            needle += 4;
            i = needle;
        }
        _in_body = true;
        //std::cout << "body----------------\n" << std::endl;
        while (i < bytes_read)
        {
            //std::cout << buffer[i];
            _body.push_back(buffer[i]);
            i++;
        }
        //std::cout << _body.size() << " = _body.size()\n";
        //std::cout << _content_length << " = _content_length\n";
        _body_complete = (_body.size() >= _content_length);
    }
    
    // POST with Content-Length: 0 is valid (empty body)
    if (_headers_complete && _method == METHOD_POST && _content_length == 0 && !_chunked) {
        // if content-length not found
        if (_headers.find("content-length") == _headers.end()) {
            _status = HTTP_LENGTH_REQUIRED;
        }
        _body_complete = true;
    }
    
    // If headers are complete but body is expected and not yet complete, wait for more data
    // Only return true when the FULL request is ready to be processed
    if (_headers_complete && !_chunked) {
        // If we expect a body (Content-Length > 0) but haven't received it all yet, return false
        if (_content_length > 0 && !_body_complete) {
            return false;  // Keep reading, request not complete yet
        }
        // Headers complete and either no body expected or body is complete
        return true;
    }
    
    return false;  // Headers not complete yet
}

//parsing first line
void HttpRequest::parseRequestLine(const std::string& line) {
    // Manual parsing without istringstream
    //std::cout << line << std::endl;
    std::vector<std::string> tokens = Utils::split(line, ' ');
    if (tokens.size() == 3) {
        // these are case-sensitive
        _method = stringToMethod(tokens[0]);
        _uri = tokens[1];
        _version = tokens[2];
        
        // SECURITY FIX: Validate HTTP version
        if (!Utils::isValidHttpVersion(_version)) {
            _status = HTTP_HTTP_VERSION_NOT_SUPPORTED;  // 505 instead of 400
            return;
        }
        
        if (_uri.size() > MAX_URI) // need to include domain name?
            _status = HTTP_URI_TOO_LONG;
        else if (_method == METHOD_UNKNOWN)
            _status = HTTP_NOT_IMPLEMENTED;
        else
            parseUri(_uri);
        // check only two space in between
        int i = 0;
        int space = 0;
        while (line[i])
        {
            if(line[i] == ' ')
                space++;
            i++;
        }
        if(space != 2)
        {
            _status = HTTP_BAD_REQUEST;
            return ;
        }
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
        
        // For most headers we lowercase the value, but NOT for Cookie header (values are case-sensitive)
        if (key_lower == "cookie") {
            _headers[key_lower] = value;  // Keep original case for cookie values
        } else {
            _headers[key_lower] = Utils::toLowerCase(value);
        }
        
        if (key_lower == "content-length") {
            _content_length = Utils::toSizeT(value);
            // can't co-exist
            if (_headers.find("transfer-encoding") != _headers.end())
                _status = HTTP_BAD_REQUEST;
            return;
        }
        else if (key_lower == "transfer-encoding") {
            // if we don't support chunked encoding, return 411 Length Required
            //_status = HTTP_LENGTH_REQUIRED;
            if (_headers.find("content-length") != _headers.end())
                _status = HTTP_BAD_REQUEST;
            // only chunked supported, no compress, deflate, gzip, etc
            if (_headers[key_lower] == "chunked")
                _chunked = true;
            else
                _status = HTTP_BAD_REQUEST;
            return;
        }
        // expectation is not supported
        if (key_lower == "expect")
        {
            _status = HTTP_EXPECTATION_FAILED;
            return ;
        }
    }
}

void HttpRequest::parseUri(const std::string& uri) {
    // Remove fragment first (fragments are client-side only, never sent to server in valid HTTP)
    // But some clients might send them, so we handle them
    std::string uri_without_fragment = uri;
    size_t fragment_pos = uri.find('#');
    if (fragment_pos != std::string::npos) {
        uri_without_fragment = uri.substr(0, fragment_pos);
    }
    
    // Now parse query string
    size_t query_pos = uri_without_fragment.find('?');
    if (query_pos != std::string::npos) {
        _query_string = uri_without_fragment.substr(query_pos + 1);
        _uri = uri_without_fragment.substr(0, query_pos);
        parseQueryString(_query_string);
    } else {
        _uri = uri_without_fragment;
    }
}

HttpMethod HttpRequest::stringToMethod(const std::string& method_str) {
    if (method_str == "GET") return METHOD_GET;
    if (method_str == "POST") return METHOD_POST;
    if (method_str == "PUT") return METHOD_PUT;
    if (method_str == "HEAD") return METHOD_HEAD;
    if (method_str == "DELETE") return METHOD_DELETE;
    if (method_str == "OPTIONS") return METHOD_OPTIONS;
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
const std::vector<char>& HttpRequest::getBody() const { return _body; }
//const std::string& HttpRequest::getBody() const { return _body; }
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
        case METHOD_HEAD: return "HEAD";
        case METHOD_DELETE: return "DELETE";
        case METHOD_OPTIONS: return "OPTIONS";
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
    std::cout << "Body length: " << _body.size() << std::endl;
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
