#include "../includes/webserv.hpp"
#include "../includes/config/ConfigParser.hpp"
#include "../includes/http/HttpRequest.hpp"
#include "../includes/http/HttpResponse.hpp"
#include "../includes/http/HttpHandler.hpp"
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
    ConfigParser _config;
    
public:
    TestServer() : _server_fd(-1), _epoll_fd(-1) {}
    
    ~TestServer() {
        cleanup();
    }
    
    bool initialize(int port, std::string input) {
        std::cout << "Initializing test server on port " << port << std::endl;
        
        // Load configuration
        if (!_config.parseConfig(input)) {
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
        
        //NEW instance
        HttpHandler handler(_server_fd, _epoll_fd);

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
                    // New connection NEW
                    handler.acceptConnection();
                } else {
                    // Handle client NEW
                    if (_events[i].events & EPOLLIN) {
                        handler.handleRead(fd);
                    }
                    if (_events[i].events & EPOLLOUT) {
                        handler.handleWrite(fd);
                    }
                    if (_events[i].events & (EPOLLHUP | EPOLLERR)) {
                        handler.closeConnection(fd);
                    }
                }
            }
        }
    }
    
private:

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
    (void)argc;
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGPIPE, SIG_IGN);

    std::stringstream other;
    other << argv[2];
    std::string adv = "config/advanced.conf";

    TestServer server;
    if (argv[2] && !server.initialize(port, argv[2])) {
        std::cerr << "Failed to initialize server" << std::endl;
        return 1;
    } else {
        if (!server.initialize(port, adv))
            return std::cerr<<"not advanced enough yet" << std::endl && -1;

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