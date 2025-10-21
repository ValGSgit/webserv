#include "../../includes/server/ServerManager.hpp"
#include "../../includes/utils/Utils.hpp"

ServerManager::ServerManager() : _epoll_fd(-1), _running(false), _last_cleanup(0), _http_handler(NULL) {
    _server_sockets.reserve(10);
}

ServerManager::~ServerManager() {
    shutdown();
}

bool ServerManager::initialize(const std::string& config_file) {
    try {
        // Parse configuration
        ConfigParser parser;
        if (!parser.parseConfig(config_file)) {
            logError("Configuration parsing failed", config_file);
            return false;
        }
        
        _server_configs = parser.getServers();
        if (_server_configs.empty()) {
            logError("No server configurations found");
            return false;
        }
        
        // Setup epoll
        if (!setupEpoll()) {
            logError("Failed to setup epoll");
            return false;
        }
        
        // Initialize server sockets
        if (!initializeServerSockets()) {
            logError("Failed to initialize server sockets");
            return false;
        }
        
        // Create HTTP handler
        _http_handler = new HttpHandler(this, _epoll_fd);
        
        std::cout << "✓ ServerManager initialized with " << _server_sockets.size() << " server socket(s)" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        logError("Exception during initialization", e.what());
        return false;
    } catch (...) {
        logError("Unknown exception during initialization");
        return false;
    }
}

bool ServerManager::setupEpoll() {
    _epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1) {
        perror("epoll_create1");
        return false;
    }
    return true;
}

bool ServerManager::initializeServerSockets() {
    _server_sockets.clear();
    
    for (size_t i = 0; i < _server_configs.size(); ++i) {
        if (!createServerSocket(_server_configs[i])) {
            logError("Failed to create server socket for config", Utils::toString(i));
        }
    }
    
    return !_server_sockets.empty();
}

bool ServerManager::createServerSocket(const ServerConfig& config) {
    bool success = false;
    
    for (size_t i = 0; i < config.ports.size(); ++i) {
        int port = config.ports[i];
        
        // Check for duplicate ports
        bool duplicate = false;
        for (size_t j = 0; j < _server_sockets.size(); ++j) {
            if (_server_sockets[j].port == port) {
                logError("Port already in use", Utils::toString(port));
                duplicate = true;
                break;
            }
        }
        if (duplicate) continue;
        
        // Create socket
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
            perror("socket");
            continue;
        }
        
        // Set socket options
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
            perror("setsockopt");
            close(server_fd);
            continue;
        }
        
        // Make non-blocking
        Utils::setNonBlocking(server_fd);
        
        // Bind
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            perror("bind");
            close(server_fd);
            continue;
        }
        
        // Listen
        if (listen(server_fd, SOMAXCONN) == -1) {
            perror("listen");
            close(server_fd);
            continue;
        }
        
        // Add to epoll
        addToEpoll(server_fd, EPOLLIN);
        
        // Store server socket
        ServerSocket ss;
        ss.fd = server_fd;
        ss.port = port;
        ss.config = &config;
        _server_sockets.push_back(ss);
        
        std::cout << "✓ Server socket created on port " << port << std::endl;
        success = true;
    }
    
    return success;
}

void ServerManager::addToEpoll(int fd, uint32_t events) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.fd = fd;
    
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl: add");
    }
}

void ServerManager::modifyEpoll(int fd, uint32_t events) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.fd = fd;
    
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
        perror("epoll_ctl: mod");
    }
}

void ServerManager::removeFromEpoll(int fd) {
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        perror("epoll_ctl: del");
    }
}

void ServerManager::run() {
    if (!_http_handler) {
        logError("HTTP handler not initialized");
        return;
    }
    
    _running = true;
    _last_cleanup = time(NULL);
    
    std::cout << "\n🚀 Starting WebServ with epoll-based I/O..." << std::endl;
    std::cout << "📡 Press Ctrl+C to stop server" << std::endl;
    
    while (_running) {
        int nfds = epoll_wait(_epoll_fd, _events, MAX_CONNECTIONS, 1000);
        
        if (nfds == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }
        
        // Process events
        for (int i = 0; i < nfds; i++) {
            int fd = _events[i].data.fd;
            
            // Check if it's a server socket
            if (isServerSocket(fd)) {
                const ServerSocket* ss = findServerSocket(fd);
                if (ss) {
                    _http_handler->acceptConnection(ss->fd, ss->port);
                }
            } else {
                // Client socket
                if (_events[i].events & EPOLLIN) {
                    _http_handler->handleRead(fd);
                }
                if (_events[i].events & EPOLLOUT) {
                    _http_handler->handleWrite(fd);
                }
                if (_events[i].events & (EPOLLHUP | EPOLLERR)) {
                    _http_handler->closeConnection(fd);
                }
            }
        }
        
        // Periodic cleanup
        time_t now = time(NULL);
        if (now - _last_cleanup >= CLEANUP_INTERVAL) {
            cleanupTimeouts();
            _last_cleanup = now;
        }
    }
    
    std::cout << "\n🛑 Server shutting down..." << std::endl;
}

void ServerManager::cleanupTimeouts() {
    time_t now = time(NULL);
    std::vector<int> to_close;
    
    for (std::map<int, ClientConnection>::iterator it = _clients.begin(); 
         it != _clients.end(); ++it) {
        if (now - it->second.last_activity > CLIENT_TIMEOUT) {
            to_close.push_back(it->first);
        }
    }
    
    for (size_t i = 0; i < to_close.size(); ++i) {
        std::cout << "⏱ Timeout: closing client fd " << to_close[i] << std::endl;
        if (_http_handler) {
            _http_handler->closeConnection(to_close[i]);
        }
    }
}

bool ServerManager::isServerSocket(int fd) const {
    for (size_t i = 0; i < _server_sockets.size(); ++i) {
        if (_server_sockets[i].fd == fd) {
            return true;
        }
    }
    return false;
}

const ServerSocket* ServerManager::findServerSocket(int fd) const {
    for (size_t i = 0; i < _server_sockets.size(); ++i) {
        if (_server_sockets[i].fd == fd) {
            return &_server_sockets[i];
        }
    }
    return NULL;
}

const ServerConfig* ServerManager::findServerConfig(int port) const {
    for (size_t i = 0; i < _server_configs.size(); ++i) {
        for (size_t j = 0; j < _server_configs[i].ports.size(); ++j) {
            if (_server_configs[i].ports[j] == port) {
                return &_server_configs[i];
            }
        }
    }
    return NULL;
}

void ServerManager::closeClient(int client_fd) {
    _clients.erase(client_fd);
}

ClientConnection* ServerManager::getClient(int client_fd) {
    if (_clients.find(client_fd) == _clients.end()) {
        // Create new client entry
        _clients[client_fd] = ClientConnection();
        _clients[client_fd].fd = client_fd;
    }
    return &_clients[client_fd];
}

void ServerManager::shutdown() {
    _running = false;
    
    // Close all client connections
    for (std::map<int, ClientConnection>::iterator it = _clients.begin(); 
         it != _clients.end(); ++it) {
        close(it->first);
    }
    _clients.clear();
    
    // Close all server sockets
    for (size_t i = 0; i < _server_sockets.size(); ++i) {
        close(_server_sockets[i].fd);
    }
    _server_sockets.clear();
    
    // Close epoll
    if (_epoll_fd != -1) {
        close(_epoll_fd);
        _epoll_fd = -1;
    }
    
    // Delete HTTP handler
    if (_http_handler) {
        delete _http_handler;
        _http_handler = NULL;
    }
    
    std::cout << "✓ Server shutdown complete" << std::endl;
}

void ServerManager::requestShutdown() {
    _running = false;
}

void ServerManager::logError(const std::string& operation, const std::string& details) {
    std::cerr << "❌ Error in " << operation;
    if (!details.empty()) {
        std::cerr << ": " << details;
    }
    std::cerr << std::endl;
}
