#include "../../includes/http/HttpHandler.hpp"

HttpHandler::HttpHandler(){}
HttpHandler::~HttpHandler(){}
HttpHandler::HttpHandler(HttpHandler const &other){*this = other;}
HttpHandler &HttpHandler::operator=(HttpHandler const &other){
    if (this == &other)
        return (*this);
    return (*this);
}

HttpHandler::HttpHandler(int server_fd, int epoll_fd) : _server_fd(server_fd), _epoll_fd(epoll_fd)
{}
    
void HttpHandler::processRequest(int client_fd) {
    HttpRequest& request = _client_requests[client_fd];
    HttpResponse& response = _client_responses[client_fd];
        
    std::cout << "Processing " << request.methodToString() << " " << request.getUri() << std::endl;
    
        // Route the request
    std::string uri = request.getUri();
    
    if (uri == "/") {
        // Serve index.html
        response = HttpResponse::fileResponse("www/index.html");
    } else if (uri == "/test") {
        // Serve test.html
        response = HttpResponse::fileResponse("www/test.html");
    } else if (uri.find("/cgi-bin/") == 0) {
        // Handle CGI request
        CgiHandler cgi;
        std::string script_path = "www" + uri;
        response = cgi.executeCgi(request, script_path);
    } else if (uri == "/upload" && request.getMethod() == METHOD_POST) {
        // Handle file upload
        response = handleUpload(request);
    } else if (uri == "/api/test") {
        // Test JSON API endpoint
        response = handleJsonApi(request);
    } else if (uri == "/redirect") {
        // Test redirect
        response = HttpResponse::redirectResponse("http://localhost:8080/");
    } else if (uri.find("/static/") == 0) {
        // Serve static files
        std::string filepath = "www" + uri;
        response = HttpResponse::fileResponse(filepath);
    } else {
        // Try to serve as file or directory
        std::string filepath = "www" + uri;
        if (Utils::fileExists(filepath)) {
            if (Utils::isDirectory(filepath)) {
                response = HttpResponse::directoryListingResponse(filepath, uri);
            } else {
                response = HttpResponse::fileResponse(filepath);
            }
        } else {
            response = HttpResponse::errorResponse(STATUS_NOT_FOUND);
        }
    }
    
    // Prepare response for sending
    _response_buffers[client_fd] = response.getResponseString();
    _response_offsets[client_fd] = 0;
    
    // Switch to write mode
    struct epoll_event ev;
    ev.events = EPOLLOUT | EPOLLET;
    ev.data.fd = client_fd;
    epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
}
    
HttpResponse HttpHandler::handleUpload(const HttpRequest& request) {
    std::cout << "Handling file upload" << std::endl;
    
    const std::string& body = request.getBody();
    if (body.empty()) {
        return HttpResponse::errorResponse(STATUS_BAD_REQUEST, "No file data");
    }
    
    // Simple upload handling - save to uploads directory
    std::string filename = "uploaded_file_" + Utils::toString(42); // Static filename instead of time-based
    std::string filepath = "www/uploads/" + filename;

    if (Utils::writeFile(filepath, body)) {
        HttpResponse response;
        response.setStatus(STATUS_CREATED);
        response.setContentType("application/json");
        response.setBody("{\"message\":\"File uploaded successfully\",\"filename\":\"" + filename + "\"}");
        return response;
    } else {
        return HttpResponse::errorResponse(STATUS_INTERNAL_SERVER_ERROR, "Failed to save file");
    }
}
    
HttpResponse HttpHandler::handleJsonApi(const HttpRequest& request) {
    std::cout << "Handling JSON API request" << std::endl;
    
    HttpResponse response;
    response.setStatus(STATUS_OK);
    response.setContentType("application/json");

    std::string json_body = "{\n";
    json_body += "  \"message\": \"Test API endpoint\",\n";
    json_body += "  \"method\": \"" + request.methodToString() + "\",\n";
    json_body += "  \"uri\": \"" + request.getUri() + "\",\n";
    //json_body += "  \"timestamp\": \"" + need to implement a get time + "\",\n";
    json_body += "  \"headers\": {\n";
    
    const std::map<std::string, std::string>& headers = request.getHeaders();
    bool first = true;
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
            it != headers.end(); ++it) {
        if (!first) json_body += ",\n";
        json_body += "    \"" + it->first + "\": \"" + it->second + "\"";
        first = false;
    }
        
    json_body += "\n  }\n}";
    response.setBody(json_body);
        
    return response;
}

void HttpHandler::handleRead(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        _client_buffers[client_fd] += buffer;
            
        // Try to parse request
        if (_client_requests[client_fd].parseRequest(_client_buffers[client_fd])) {
            processRequest(client_fd);
                return;
        }
    }
        
/*     if (bytes_read == 0) {
        std::cout << "Client disconnected (fd: " << client_fd << ")" << std::endl;
        closeConnection(client_fd);
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("read");
        closeConnection(client_fd);
    } */
}

void HttpHandler::handleWrite(int client_fd) {
    std::string& buffer = _response_buffers[client_fd];
    size_t& offset = _response_offsets[client_fd];
        
    if (offset >= buffer.length()) {
        closeConnection(client_fd);
        return;
    }
        
    ssize_t bytes_sent = write(client_fd, buffer.c_str() + offset, buffer.length() - offset);
    if (bytes_sent > 0) {
        offset += bytes_sent;
        if (offset >= buffer.length()) {
            std::cout << "✓ Response sent successfully to fd " << client_fd << std::endl;
            closeConnection(client_fd);
        }
    } else if (bytes_sent == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("write");
        closeConnection(client_fd);
    }
}


void HttpHandler::acceptConnection() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(_server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("accept");
        }
        return;
    }
    Utils::setNonBlocking(client_fd);
        
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = client_fd;
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
        perror("epoll_ctl: client_fd");
        close(client_fd);
        return;
    }

    _client_buffers[client_fd] = "";
    _client_requests[client_fd] = HttpRequest();
    _client_responses[client_fd] = HttpResponse();
    _response_buffers[client_fd] = "";
    _response_offsets[client_fd] = 0;
        
    std::cout << "✓ New connection accepted (fd: " << client_fd << ")" << std::endl;
}

void HttpHandler::closeConnection(int client_fd) {
    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);

    _client_buffers.erase(client_fd);
    _client_requests.erase(client_fd);
    _client_responses.erase(client_fd);
    _response_buffers.erase(client_fd);
    _response_offsets.erase(client_fd);

    std::cout << "Connection closed (fd: " << client_fd << ")" << std::endl;
}