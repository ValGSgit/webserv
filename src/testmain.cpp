#include "../includes/webserv.hpp"
#include "../includes/config/ConfigParser.hpp"
#include "../includes/http/HttpRequest.hpp"
#include "../includes/http/HttpResponse.hpp"
#include "../includes/cgi/CgiHandler.hpp"
#include "../includes/utils/Utils.hpp"
#include <sys/epoll.h>

volatile sig_atomic_t g_running = 1;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutting down server..." << std::endl;
        g_running = 0;
    }
}

class TestServer {
private:
    int _server_fd;
    int _epoll_fd;
    struct epoll_event _events[MAX_CONNECTIONS];
    std::map<int, std::string> _client_buffers;
    std::map<int, HttpRequest> _client_requests;
    std::map<int, HttpResponse> _client_responses;
    std::map<int, std::string> _response_buffers;
    std::map<int, size_t> _response_offsets;
    ConfigParser _config;
    
public:
    TestServer() : _server_fd(-1), _epoll_fd(-1) {}
    
    ~TestServer() {
        cleanup();
    }
    
    bool initialize(int port) {
        std::cout << "Initializing test server on port " << port << std::endl;
        
        // Load configuration
        if (!_config.parseConfig("config/default.conf")) {
            std::cerr << "Failed to parse configuration" << std::endl;
            return false;
        }
        std::cout << "✓ Configuration loaded" << std::endl;
        
        // Create socket
        _server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (_server_fd == -1) {
            perror("socket");
            return false;
        }
        
        // Set socket options
        int opt = 1;
        if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt");
            return false;
        }
        
        // Make socket non-blocking
        Utils::setNonBlocking(_server_fd);
        
        // Bind socket
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(_server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind");
            return false;
        }
        
        // Listen
        if (listen(_server_fd, SOMAXCONN) < 0) {
            perror("listen");
            return false;
        }
        
        // Create epoll instance
        _epoll_fd = epoll_create1(0);
        if (_epoll_fd == -1) {
            perror("epoll_create1");
            return false;
        }
        
        // Add server socket to epoll
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = _server_fd;
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _server_fd, &ev) == -1) {
            perror("epoll_ctl: server_fd");
            return false;
        }
        
        std::cout << "✓ Server initialized and listening on port " << port << std::endl;
        return true;
    }
    
    void run() {
        std::cout << "Starting test server main loop..." << std::endl;
        std::cout << "Test with: curl http://localhost:8080/" << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        
        while (g_running) {
            int nfds = epoll_wait(_epoll_fd, _events, MAX_CONNECTIONS, 1000);
            if (nfds == -1) {
                if (errno == EINTR) continue;
                perror("epoll_wait");
                break;
            }
            
            for (int i = 0; i < nfds; i++) {
                int fd = _events[i].data.fd;
                
                if (fd == _server_fd) {
                    // New connection
                    acceptConnection();
                } else {
                    // Handle client
                    if (_events[i].events & EPOLLIN) {
                        handleRead(fd);
                    }
                    if (_events[i].events & EPOLLOUT) {
                        handleWrite(fd);
                    }
                    if (_events[i].events & (EPOLLHUP | EPOLLERR)) {
                        closeConnection(fd);
                    }
                }
            }
        }
    }
    
private:
    void acceptConnection() {
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
    
    void handleRead(int client_fd) {
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
        
        if (bytes_read == 0) {
            std::cout << "Client disconnected (fd: " << client_fd << ")" << std::endl;
            closeConnection(client_fd);
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("read");
            closeConnection(client_fd);
        }
    }
    
    void processRequest(int client_fd) {
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
    
    HttpResponse handleUpload(const HttpRequest& request) {
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
    
    HttpResponse handleJsonApi(const HttpRequest& request) {
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
    
    void handleWrite(int client_fd) {
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
    
    void closeConnection(int client_fd) {
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        close(client_fd);
        
        _client_buffers.erase(client_fd);
        _client_requests.erase(client_fd);
        _client_responses.erase(client_fd);
        _response_buffers.erase(client_fd);
        _response_offsets.erase(client_fd);
        
        std::cout << "Connection closed (fd: " << client_fd << ")" << std::endl;
    }
    
    void cleanup() {
        if (_epoll_fd != -1) {
            close(_epoll_fd);
            _epoll_fd = -1;
        }
        if (_server_fd != -1) {
            close(_server_fd);
            _server_fd = -1;
        }
    }
};

int main(int argc, char **argv) {
    std::cout << "=== WebServ Test Server with Epoll ===" << std::endl;
    
    int port = 8080;
    if (argc > 1) {
        port = Utils::toInt(argv[1]);
    }
    
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGPIPE, SIG_IGN);
    
    TestServer server;
    if (!server.initialize(port)) {
        std::cerr << "Failed to initialize server" << std::endl;
        return 1;
    }
    
    std::cout << "\n=== Test Endpoints ===" << std::endl;
    std::cout << "GET  http://localhost:" << port << "/           - Home page" << std::endl;
    std::cout << "GET  http://localhost:" << port << "/test        - Test page" << std::endl;
    std::cout << "GET  http://localhost:" << port << "/api/test    - JSON API" << std::endl;
    std::cout << "GET  http://localhost:" << port << "/redirect    - Redirect test" << std::endl;
    std::cout << "POST http://localhost:" << port << "/upload      - File upload" << std::endl;
    std::cout << "GET  http://localhost:" << port << "/cgi-bin/test.py - CGI test" << std::endl;
    std::cout << "GET  http://localhost:" << port << "/nonexistent - 404 test" << std::endl;
    std::cout << "\n=== Example curl commands ===" << std::endl;
    std::cout << "curl http://localhost:" << port << "/" << std::endl;
    std::cout << "curl -X POST -d 'test data' http://localhost:" << port << "/upload" << std::endl;
    std::cout << "curl -H 'Content-Type: application/json' http://localhost:" << port << "/api/test" << std::endl;
    std::cout << "========================" << std::endl;
    
    server.run();
    
    std::cout << "Server shutdown complete" << std::endl;
    return 0;
}