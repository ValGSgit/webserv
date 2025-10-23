#include "../../includes/http/HttpHandler.hpp"
#include "../../includes/server/ServerManager.hpp"

HttpHandler::~HttpHandler() {}

HttpHandler::HttpHandler(HttpHandler const &other) : _server_manager(other._server_manager), _epoll_fd(other._epoll_fd) {
    *this = other;
}

HttpHandler &HttpHandler::operator=(HttpHandler const &other) {
    if (this == &other)
        return (*this);
    return (*this);
}

HttpHandler::HttpHandler(ServerManager* manager, int epoll_fd) 
    : _server_manager(manager), _epoll_fd(epoll_fd) {}

const ServerConfig* HttpHandler::findServerForClient(int client_fd) {
    ClientConnection* client = _server_manager->getClient(client_fd);
    if (!client) {
        return NULL;
    }
    return _server_manager->findServerConfig(client->server_port);
}

// bool HttpHandler::methodAllowed(const std::string& uri, const std::string& method, const ServerConfig& config) {
//     try {
//         std::string temp = uri.substr(0, uri.find_last_of('/'));
//         //really like this?
//         if (method == "DELETE")
//         {
//             if (temp == "")
//                 temp += "/";
//             const RouteConfig& route = config.routes.at(temp);
//             for (size_t i = 0; i < route.allowed_methods.size(); ++i) {
//                 if (method == route.allowed_methods[i])
//                     return true;
//             }
//         }
//         else
//         {
//             const RouteConfig& route = config.routes.at(uri);
//             for (size_t i = 0; i < route.allowed_methods.size(); ++i) {
//                 if (method == route.allowed_methods[i])
//                     return true;
//             }
//         }
//     } catch(const std::exception& e) {
//         // Route not found - allow GET by default
//         if (method == "GET")
//             return true;
//     }
//     return false;
// }

bool HttpHandler::methodAllowed(const std::string& uri, const std::string& method, const ServerConfig& config) {
    // Try to find the most specific matching route
    std::string route_path = uri;
    
    // Keep going up the directory tree until we find a matching route
    while (true) {
        try {
            const RouteConfig& route = config.routes.at(route_path); //at not allowed
            
            // Found a matching route! Check if method is allowed
            for (size_t i = 0; i < route.allowed_methods.size(); ++i) {
                if (method == route.allowed_methods[i]) {
                    return true;  // Method is allowed
                }
            }
            
            // Route found but method not allowed
            return false;
            
        } catch (const std::exception& e) {
            // Route not found, try parent directory
            if (route_path == "/") {
                // Reached root, no route found
                // Allow GET by default for backwards compatibility
                return (method == "GET");
            }
            
            // Remove last path segment and try again
            size_t last_slash = route_path.find_last_of('/');
            if (last_slash == 0) {
                route_path = "/";  // Parent is root
            } else if (last_slash != std::string::npos) {
                route_path = route_path.substr(0, last_slash);
            } else {
                // No slash found (shouldn't happen with valid URIs)
                return (method == "GET");
            }
        }
    }
}
    
void HttpHandler::processRequest(int client_fd, int server_port) {
    HttpRequest& request = _client_requests[client_fd];
    HttpResponse& response = _client_responses[client_fd];
    
    const ServerConfig* config = _server_manager->findServerConfig(server_port);
    if (!config) {
        response = HttpResponse::errorResponse(HTTP_INTERNAL_SERVER_ERROR);
        _response_buffers[client_fd] = response.getResponseString();
        _response_offsets[client_fd] = 0;
        return;
    }
        
    std::cout << "Processing " << request.methodToString() << " " << request.getUri() << std::endl;
    
    std::string uri = request.getUri();
    // Check for client request errors
    if (request.getStatus()) {
        response = HttpResponse::errorResponse(request.getStatus());
    }
    // Check if method is allowed
    else if (!methodAllowed(uri, request.methodToString(), *config)) {
        response = HttpResponse::errorResponse(HTTP_METHOD_NOT_ALLOWED);}
    // upload
    else if (request.getMethod() == METHOD_POST) {
        response = handleUpload(request, *config, client_fd);}
    // delete
    else if (request.getMethod() == METHOD_DELETE) {
        response = handleDelete(request, *config, client_fd);}
    // Route handling
    else if (uri == "/") {
        std::string index_path = config->root + "/" + config->index;
        response = HttpResponse::fileResponse(index_path);
    } else if (uri.find("/cgi-bin/") == 0) {
        CgiHandler cgi;
        std::string script_path = config->root + uri;
        response = cgi.executeCgi(request, script_path);
    } else if (uri == "/api/test") {
        response = handleJsonApi(request);
    } else if (uri == "/redirect") {
        response = HttpResponse::redirectResponse("http://localhost:" + 
                                                  Utils::toString(server_port) + "/");
    } else if (uri.find("/static/") == 0) {
        std::string filepath = config->root + uri;
        response = HttpResponse::fileResponse(filepath);
    } else {
        // Try to serve as file or directory
        std::string filepath = config->root + uri;
        if (Utils::fileExists(filepath)) {
            if (Utils::isDirectory(filepath)) {
                response = HttpResponse::directoryListingResponse(filepath, uri);
            } else {
                response = HttpResponse::fileResponse(filepath);
            }
        } else {
            response = HttpResponse::errorResponse(HTTP_NOT_FOUND);
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

// return the index of boundary in the buffer if found
// what if the boundary lays between buffer?
static int check_boundary(char *buffer, const char *boundary, size_t len)
{
    size_t	i;
	size_t	j;
    size_t len_boundary = 0;

    while (boundary[len_boundary])
        len_boundary++;
    if (!*boundary)
        return (0);
    i = 0;
	while (i < len)
	{
		j = 0;
		if (buffer[i] == boundary[j])
		{
			while (buffer[i + j] == boundary[j] && boundary[j] != '\0')
			{
				j++;
				if (j == len_boundary && i + j <= len)
					return (i);
			}
		}
		i++;
	}
	return (0);
}

HttpResponse HttpHandler::handleDelete(const HttpRequest& request, const ServerConfig& config, int client_fd)
{
    (void)config;
    (void)client_fd;
    std::string file_path = config.root + request.getUri();
    int fd = open(file_path.c_str(), O_WRONLY, 0644);
    if (fd == -1)
        return HttpResponse::errorResponse(HTTP_BAD_REQUEST, "File not found!"); // why is 204 not working???
    close(fd);
    std::remove(file_path.c_str());
    // or 202 HTTP_ACCEPTED?
    return HttpResponse::messageResponse(HTTP_OK, "File deleted!");
}

HttpResponse HttpHandler::handleUpload(const HttpRequest& request, const ServerConfig& config, int client_fd) {
    std::cout << "Handling file upload" << std::endl;

    const std::string& body = request.getBody();
    // default upload path
    std::string upload_dir = config.root + "/uploads/";
    // Find upload path from config
    try
    {
        const RouteConfig &route = config.routes.at("/upload"); //at not allowed
        //std::cout << "route.upload_path = " << route.upload_path << std::endl;
        if (route.upload_path != "")
            upload_dir = route.upload_path + "/";
    }
    catch(const std::exception& e)
    {
        //std::cerr << "route not found!" << '\n';
    }
    
    // Simple upload handling - save to uploads directory
    std::string filename = "uploaded_file_" + Utils::toString(static_cast<int>(time(NULL)));
    std::string filepath = upload_dir + filename;
    
    // skip headers
    std::size_t needle = body.find("\r\n");
    // find boundary
    std::string boundary = "\r\n" + body.substr(0, needle);;
    std::string temp =_raw_buffer;
    needle = temp.find(boundary);
    _raw_bytes_read -= needle;
    _raw_buffer = &_raw_buffer[needle];
    temp =_raw_buffer;
    needle = temp.find("\r\n\r\n");
    // save file info
    _file_info = temp.substr(0, needle);
    //std::cout << _file_info;
    // needle is the staring index of the file
    needle += 4;
    _raw_bytes_read -= needle;
    // save also file info?
    size_t total_bytes_read = 0;
    if (Utils::writeFile(filepath, &_raw_buffer[needle], _raw_bytes_read)) {
        total_bytes_read += _raw_bytes_read;
        char buffer[BUFFER_SIZE];
        int bytes_read = 0;
        // if there are things left in fd, continue to read the body and write to file
        while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE)) > 0) {
            //std::cout << "bytes_read = " << bytes_read << std::endl;
            if (check_boundary(buffer, boundary.c_str(), bytes_read))
                bytes_read = check_boundary(buffer, boundary.c_str(), bytes_read);
            if (!Utils::writeFile(filepath, buffer, bytes_read))
                return HttpResponse::errorResponse(HTTP_INTERNAL_SERVER_ERROR, "Failed to save file");
            total_bytes_read += bytes_read;
            
            if (total_bytes_read > config.max_body_size)
            {
                // delete the file!
                std::remove(filepath.c_str());
                return HttpResponse::errorResponse(HTTP_PAYLOAD_TOO_LARGE, "File is too big");
            }
        }
        if (total_bytes_read == 0)
        {
            std::remove(filepath.c_str());
            return HttpResponse::errorResponse(HTTP_BAD_REQUEST, "No file data");
        }
        //std::cout << "total_bytes_read = " << total_bytes_read << std::endl;

        return HttpResponse::messageResponse(HTTP_CREATED, "Upload Successful", "File uploaded successfully!");//response;
    } else {
        return HttpResponse::errorResponse(HTTP_INTERNAL_SERVER_ERROR, "Failed to save file");
    }
}
    
HttpResponse HttpHandler::handleJsonApi(const HttpRequest& request) {
    std::cout << "Handling JSON API request" << std::endl;
    
    HttpResponse response;
    response.setStatus(HTTP_OK);
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
        _raw_buffer = buffer;
        _raw_bytes_read = bytes_read;
        // Try to parse request
        if (_client_requests[client_fd].parseRequest(_client_buffers[client_fd])) {
            ClientConnection* client = _server_manager->getClient(client_fd);
            if (client) {
                processRequest(client_fd, client->server_port);
            } else {
                closeConnection(client_fd);
            }
            return;
        }
    }
        
    if (bytes_read == 0) {
        std::cout << "Client disconnected (fd: " << client_fd << ")" << std::endl;
        closeConnection(client_fd);
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("read");
        closeConnection(client_fd);
    }
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


void HttpHandler::acceptConnection(int server_fd, int server_port) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
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

    // Initialize client tracking data
    _client_buffers[client_fd] = "";
    _client_requests[client_fd] = HttpRequest();
    _client_responses[client_fd] = HttpResponse();
    _response_buffers[client_fd] = "";
    _response_offsets[client_fd] = 0;
    
    // Register client in ServerManager
    ClientConnection* client = _server_manager->getClient(client_fd);
    if (client) {
        client->fd = client_fd;
        client->server_port = server_port;
        client->state = STATE_READING_HEADERS;
        client->last_activity = time(NULL);
    }
        
    std::cout << "✓ New connection accepted (fd: " << client_fd << ", port: " << server_port << ")" << std::endl;
}

void HttpHandler::closeConnection(int client_fd) {
    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);

    // Clean up local tracking data
    _client_buffers.erase(client_fd);
    _client_requests.erase(client_fd);
    _client_responses.erase(client_fd);
    _response_buffers.erase(client_fd);
    _response_offsets.erase(client_fd);

    // Clean up in ServerManager
    _server_manager->closeClient(client_fd);

    std::cout << "Connection closed (fd: " << client_fd << ")" << std::endl;
}
