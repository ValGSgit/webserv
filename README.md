# Webserv - HTTP Server Implementation

A complete HTTP server implementation in C++98 following the project requirements.

## 📋 Project Overview

This project involves creating a fully functional HTTP server that:
- Handles multiple simultaneous connections using non-blocking I/O
- Supports GET, POST, and DELETE methods
- Serves static websites and handles file uploads
- Executes CGI scripts
- Uses configuration files similar to NGINX
- Remains resilient under stress testing

## 🎯 Implementation Strategy

### Phase 1: Foundation & Research

#### Step 1: Research Phase
- [ ] **Study HTTP/1.0 RFC (RFC 1945)**
  - Focus on request/response format
  - Understand status codes
  - Learn about headers and methods
- [ ] **Analyze NGINX behavior**
  - Test with `telnet localhost 80`
  - Compare headers and responses
  - Study NGINX configuration syntax
- [ ] **Test existing servers**
  - Use `curl -v` for detailed output
  - Test with different browsers
  - Understand connection handling

#### Step 2: Project Architecture Design

##### Overall System Architecture
```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                                WEBSERV ARCHITECTURE                              │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  ┌─────────────┐    ┌──────────────────────────────────────────────────────┐   │
│  │   Config    │───▶│                 Server Manager                        │   │
│  │   Parser    │    │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │   │
│  │             │    │  │   Server    │  │   Server    │  │   Server    │  │   │
│  │ - Parse     │    │  │ Instance 1  │  │ Instance 2  │  │ Instance N  │  │   │
│  │ - Validate  │    │  │ (Port 8080) │  │ (Port 8081) │  │ (Port 80)   │  │   │
│  │ - Load      │    │  └─────────────┘  └─────────────┘  └─────────────┘  │   │
│  └─────────────┘    │                                                      │   │
│                     │  ┌────────────────────────────────────────────────┐  │   │
│                     │  │            Connection Manager                   │  │   │
│                     │  │    ┌─────────────┐      ┌─────────────┐       │  │   │
│                     │  │    │  I/O Event  │      │ Connection  │       │  │   │
│                     │  │    │    Loop     │◄────▶│    Pool     │       │  │   │
│                     │  │    │(poll/epoll) │      │             │       │  │   │
│                     │  │    └─────────────┘      └─────────────┘       │  │   │
│                     │  └────────────────────────────────────────────────┘  │   │
│                     └──────────────────────────────────────────────────────┘   │
│                                           │                                     │
│  ┌─────────────────────────────────────────────────────────────────────────┐  │
│  │                     REQUEST PROCESSING PIPELINE                        │  │
│  ├─────────────────────────────────────────────────────────────────────────┤  │
│  │                                                                         │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │  │
│  │  │   HTTP      │─▶│   Route     │─▶│   Method    │─▶│   Response  │    │  │
│  │  │   Request   │  │   Handler   │  │   Handler   │  │   Builder   │    │  │
│  │  │   Parser    │  │             │  │             │  │             │    │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘    │  │
│  │                                           │                             │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                     │  │
│  │  │    CGI      │  │    File     │  │   Upload    │                     │  │
│  │  │   Handler   │  │   Handler   │  │   Handler   │                     │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                     │  │
│  └─────────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

##### Core Classes and Structures

###### 1. Configuration System
```cpp
// Main configuration container
class ConfigParser {
private:
    std::vector<ServerConfig> servers;
    std::map<std::string, std::string> global_settings;
    
public:
    bool parseConfigFile(const std::string& filepath);
    bool validateConfig() const;
    const std::vector<ServerConfig>& getServers() const;
    void printConfig() const; // Debug helper
};

// Individual server configuration
struct ServerConfig {
    std::vector<int> listen_ports;           // Multiple ports per server
    std::string server_name;                 // Host name
    size_t client_max_body_size;            // Request body limit
    std::map<int, std::string> error_pages;  // Custom error pages
    std::map<std::string, RouteConfig> routes; // URI -> Route mapping
    std::string root_directory;              // Document root
    std::vector<std::string> index_files;    // Default files (index.html)
    
    // Server-level settings
    int connection_timeout;                  // Client timeout
    bool autoindex;                          // Directory listing
    size_t keepalive_timeout;               // Keep-alive duration
};

// Route-specific configuration
struct RouteConfig {
    std::vector<std::string> allowed_methods; // GET, POST, DELETE
    std::string root_directory;              // Override server root
    std::string index_file;                  // Specific index file
    bool directory_listing;                  // Enable/disable autoindex
    std::string upload_directory;            // POST upload location
    std::string cgi_pass;                   // CGI interpreter path
    std::vector<std::string> cgi_extensions; // .php, .py, .pl
    std::string redirect_url;               // HTTP redirect target
    int redirect_code;                      // 301, 302, etc.
    
    // Access control
    std::vector<std::string> allow_ips;     // IP whitelist
    std::vector<std::string> deny_ips;      // IP blacklist
};
```

###### 2. Server Manager (Central Orchestrator)
```cpp
class ServerManager {
private:
    std::vector<ServerInstance*> servers;    // Multiple server instances
    ConnectionManager connection_manager;    // Handles all connections
    ConfigParser config;                     // Configuration data
    EventLoop event_loop;                   // Main I/O event loop
    
    // Server management
    std::map<int, ServerInstance*> port_to_server; // Port mapping
    std::map<int, Connection*> fd_to_connection;   // FD mapping
    
    // Resource management
    size_t max_connections;                  // Connection limit
    size_t current_connections;              // Active connections
    std::queue<int> connection_pool;         // Reusable connection slots
    
public:
    bool initialize(const std::string& config_file);
    bool createServerInstances();           // Step 1: Create servers
    bool setupSocketBindings();             // Step 2: Bind to ports
    bool initializeEventLoop();             // Step 3: Setup I/O multiplexing
    void run();                            // Step 4: Main event loop
    void shutdown();                       // Step 5: Graceful shutdown
    
    // Connection lifecycle
    void handleNewConnection(int server_fd);
    void handleClientData(int client_fd);
    void handleClientWrite(int client_fd);
    void closeConnection(int client_fd);
    
    // Server instance management
    ServerInstance* getServerByPort(int port);
    ServerInstance* getServerByHost(const std::string& host);
    bool addServerInstance(const ServerConfig& config);
    void removeServerInstance(int port);
};
```

###### 3. Server Instance (Individual Server)
```cpp
class ServerInstance {
private:
    ServerConfig config;                     // Server configuration
    int listen_fd;                          // Listening socket FD
    std::vector<Connection*> active_connections; // Client connections
    
    // Request routing
    RouteHandler route_handler;             // Handles route matching
    std::map<std::string, MethodHandler*> method_handlers; // GET, POST, DELETE
    
    // Resource handlers
    FileHandler file_handler;               // Static file serving
    CgiHandler cgi_handler;                 // CGI script execution
    UploadHandler upload_handler;           // File upload processing
    
public:
    explicit ServerInstance(const ServerConfig& config);
    ~ServerInstance();
    
    // Socket operations
    bool setupSocket();                     // Create and bind socket
    bool startListening();                  // Begin accepting connections
    void stopListening();                  // Stop accepting new connections
    
    // Request processing
    HttpResponse processRequest(const HttpRequest& request);
    bool isValidHost(const std::string& host) const;
    const RouteConfig* matchRoute(const std::string& uri) const;
    
    // Getters
    int getListenFd() const { return listen_fd; }
    const ServerConfig& getConfig() const { return config; }
    size_t getActiveConnections() const { return active_connections.size(); }
};
```

###### 4. Connection Management
```cpp
// Individual client connection
class Connection {
private:
    int client_fd;                          // Client socket FD
    ConnectionState state;                  // Reading, writing, complete
    time_t last_activity;                   // For timeout detection
    
    // Request/Response data
    HttpRequest request;                    // Incoming HTTP request
    HttpResponse response;                  // Outgoing HTTP response
    std::string read_buffer;               // Incoming data buffer
    std::string write_buffer;              // Outgoing data buffer
    
    // Transfer state
    size_t bytes_read;                     // Total bytes read
    size_t bytes_written;                  // Total bytes written
    size_t content_length;                 // Expected content length
    bool headers_complete;                 // Request headers parsed
    bool keep_alive;                       // Connection persistence
    
public:
    explicit Connection(int fd);
    ~Connection();
    
    // I/O operations
    bool readData();                       // Non-blocking read
    bool writeData();                      // Non-blocking write
    bool isReadReady() const;             // Ready for reading
    bool isWriteReady() const;            // Ready for writing
    bool isComplete() const;              // Transfer complete
    bool hasTimedOut(int timeout) const;  // Timeout check
    
    // Request/Response handling
    bool parseRequest();                   // Parse HTTP request
    void setResponse(const HttpResponse& resp);
    void reset();                         // Reset for keep-alive
    
    // State management
    ConnectionState getState() const { return state; }
    void setState(ConnectionState new_state) { state = new_state; }
    int getFd() const { return client_fd; }
};

enum ConnectionState {
    CONN_READING_HEADERS,
    CONN_READING_BODY,
    CONN_PROCESSING,
    CONN_WRITING_RESPONSE,
    CONN_KEEP_ALIVE,
    CONN_CLOSING,
    CONN_ERROR
};

// Connection pool manager
class ConnectionManager {
private:
    std::map<int, Connection*> connections; // FD -> Connection mapping
    std::queue<Connection*> available_pool; // Reusable connections
    size_t max_connections;                 // System limit
    int connection_timeout;                 // Timeout in seconds
    
public:
    ConnectionManager(size_t max_conn, int timeout);
    ~ConnectionManager();
    
    // Connection lifecycle
    Connection* createConnection(int client_fd);
    void destroyConnection(int client_fd);
    Connection* getConnection(int client_fd);
    
    // Maintenance
    void cleanupTimedOutConnections();
    size_t getActiveCount() const { return connections.size(); }
    bool isAtCapacity() const { return connections.size() >= max_connections; }
};
```

###### 5. Event Loop System
```cpp
class EventLoop {
private:
    int epoll_fd;                          // epoll/kqueue descriptor
    std::vector<struct epoll_event> events; // Event buffer
    std::map<int, EventHandler*> handlers; // FD -> Handler mapping
    bool running;                          // Loop control flag
    
    // Event types
    static const int READ_EVENT = EPOLLIN;
    static const int WRITE_EVENT = EPOLLOUT;
    static const int ERROR_EVENT = EPOLLERR | EPOLLHUP;
    
public:
    EventLoop();
    ~EventLoop();
    
    // Loop control
    bool initialize();
    void run();                           // Main event loop
    void stop();                          // Stop the loop
    
    // Event registration
    bool addFd(int fd, int events, EventHandler* handler);
    bool modifyFd(int fd, int events);
    bool removeFd(int fd);
    
    // Event processing
    void processEvents(int timeout_ms = 1000);
    void handleEvent(int fd, int events);
};

// Event handler interface
class EventHandler {
public:
    virtual ~EventHandler() {}
    virtual void handleRead(int fd) = 0;
    virtual void handleWrite(int fd) = 0;
    virtual void handleError(int fd) = 0;
};
```

##### Server Manager Integration Steps

###### Step 1: Configuration Loading and Validation
```cpp
bool ServerManager::initialize(const std::string& config_file) {
    // 1. Parse configuration file
    if (!config.parseConfigFile(config_file)) {
        std::cerr << "Failed to parse config file: " << config_file << std::endl;
        return false;
    }
    
    // 2. Validate configuration
    if (!config.validateConfig()) {
        std::cerr << "Invalid configuration detected" << std::endl;
        return false;
    }
    
    // 3. Initialize global settings
    max_connections = config.getGlobalMaxConnections();
    connection_manager.setMaxConnections(max_connections);
    
    return true;
}
```

###### Step 2: Server Instance Creation
```cpp
bool ServerManager::createServerInstances() {
    const std::vector<ServerConfig>& server_configs = config.getServers();
    
    for (size_t i = 0; i < server_configs.size(); ++i) {
        const ServerConfig& server_config = server_configs[i];
        
        // 1. Create server instance
        ServerInstance* server = new ServerInstance(server_config);
        if (!server) {
            std::cerr << "Failed to create server instance" << std::endl;
            return false;
        }
        
        // 2. Setup server socket
        if (!server->setupSocket()) {
            std::cerr << "Failed to setup socket for server" << std::endl;
            delete server;
            return false;
        }
        
        // 3. Register ports mapping
        const std::vector<int>& ports = server_config.listen_ports;
        for (size_t j = 0; j < ports.size(); ++j) {
            port_to_server[ports[j]] = server;
        }
        
        // 4. Add to server list
        servers.push_back(server);
    }
    
    return true;
}
```

###### Step 3: Socket Binding and Listening
```cpp
bool ServerManager::setupSocketBindings() {
    for (size_t i = 0; i < servers.size(); ++i) {
        ServerInstance* server = servers[i];
        
        // 1. Start listening on server socket
        if (!server->startListening()) {
            std::cerr << "Failed to start listening on server" << std::endl;
            return false;
        }
        
        // 2. Register server socket with event loop
        int listen_fd = server->getListenFd();
        if (!event_loop.addFd(listen_fd, EventLoop::READ_EVENT, this)) {
            std::cerr << "Failed to register server socket with event loop" << std::endl;
            return false;
        }
        
        std::cout << "Server listening on ports: ";
        const std::vector<int>& ports = server->getConfig().listen_ports;
        for (size_t j = 0; j < ports.size(); ++j) {
            std::cout << ports[j] << " ";
        }
        std::cout << std::endl;
    }
    
    return true;
}
```

###### Step 4: Event Loop Initialization
```cpp
bool ServerManager::initializeEventLoop() {
    // 1. Initialize event loop system
    if (!event_loop.initialize()) {
        std::cerr << "Failed to initialize event loop" << std::endl;
        return false;
    }
    
    // 2. Set up signal handlers
    signal(SIGINT, ServerManager::signalHandler);
    signal(SIGTERM, ServerManager::signalHandler);
    signal(SIGPIPE, SIG_IGN); // Ignore broken pipe
    
    // 3. Initialize connection manager
    connection_manager.initialize(max_connections);
    
    std::cout << "Event loop initialized successfully" << std::endl;
    return true;
}
```

###### Step 5: Main Event Loop Execution
```cpp
void ServerManager::run() {
    std::cout << "Starting webserv..." << std::endl;
    
    while (running) {
        // 1. Process I/O events
        event_loop.processEvents(1000); // 1 second timeout
        
        // 2. Cleanup timed-out connections
        connection_manager.cleanupTimedOutConnections();
        
        // 3. Perform maintenance tasks
        performMaintenance();
    }
    
    std::cout << "Webserv shutting down..." << std::endl;
}

void ServerManager::performMaintenance() {
    static time_t last_cleanup = time(NULL);
    time_t now = time(NULL);
    
    // Perform cleanup every 30 seconds
    if (now - last_cleanup >= 30) {
        // 1. Log server statistics
        logServerStats();
        
        // 2. Clean up idle connections
        connection_manager.cleanupIdleConnections();
        
        // 3. Update last cleanup time
        last_cleanup = now;
    }
}
```

###### Step 6: Request Processing Pipeline
```cpp
void ServerManager::handleClientData(int client_fd) {
    Connection* conn = connection_manager.getConnection(client_fd);
    if (!conn) return;
    
    // 1. Read incoming data
    if (!conn->readData()) {
        closeConnection(client_fd);
        return;
    }
    
    // 2. Parse HTTP request if complete
    if (conn->getState() == CONN_READING_HEADERS && conn->parseRequest()) {
        conn->setState(CONN_PROCESSING);
        
        // 3. Find appropriate server instance
        const HttpRequest& request = conn->getRequest();
        ServerInstance* server = getServerByHost(request.getHeader("Host"));
        if (!server) {
            server = servers[0]; // Default server
        }
        
        // 4. Process request
        HttpResponse response = server->processRequest(request);
        conn->setResponse(response);
        conn->setState(CONN_WRITING_RESPONSE);
        
        // 5. Register for write events
        event_loop.modifyFd(client_fd, EventLoop::WRITE_EVENT);
    }
}
```

This architecture provides:
- **Scalability**: Multiple server instances, connection pooling
- **Flexibility**: Configurable routing, method handlers
- **Performance**: Non-blocking I/O, efficient event handling
- **Maintainability**: Clear separation of concerns, modular design
- **Robustness**: Error handling, resource management, timeouts

### Phase 2: Core Implementation

#### Step 3: Configuration Parser
- [ ] **Create configuration structures**
  ```cpp
  // Enhanced configuration system with validation
  class ConfigParser {
  private:
      std::vector<ServerConfig> servers;
      std::map<std::string, std::string> global_settings;
      std::string config_file_path;
      
      // Parsing helpers
      bool parseServerBlock(const std::string& block, ServerConfig& config);
      bool parseRouteBlock(const std::string& block, RouteConfig& route);
      bool validateServerConfig(const ServerConfig& config) const;
      std::string extractBlockContent(const std::string& content, const std::string& block_name);
      
  public:
      bool parseConfigFile(const std::string& filepath);
      bool validateConfig() const;
      const std::vector<ServerConfig>& getServers() const;
      size_t getGlobalMaxConnections() const;
      void printConfig() const;
  };
  
  struct ServerConfig {
      // Network settings
      std::vector<int> listen_ports;           // [80, 443, 8080]
      std::string server_name;                 // "example.com"
      std::string bind_address;                // "0.0.0.0" or specific IP
      
      // Request limits
      size_t client_max_body_size;            // 1048576 (1MB)
      int connection_timeout;                  // 60 seconds
      size_t keepalive_timeout;               // 30 seconds
      size_t keepalive_requests;              // 100 requests per connection
      
      // File serving
      std::string root_directory;              // "/var/www/html"
      std::vector<std::string> index_files;    // ["index.html", "index.php"]
      bool autoindex;                          // Directory listing enabled
      
      // Error handling
      std::map<int, std::string> error_pages;  // {404: "/404.html", 500: "/error.html"}
      
      // Route configurations
      std::map<std::string, RouteConfig> routes; // {"/api": route_config, "/uploads": upload_config}
      
      // Security settings
      std::vector<std::string> allowed_ips;    // IP whitelist
      std::vector<std::string> blocked_ips;    // IP blacklist
      bool hide_server_info;                   // Hide server version in headers
  };
  
  struct RouteConfig {
      // HTTP methods
      std::vector<std::string> allowed_methods; // ["GET", "POST", "DELETE"]
      std::vector<std::string> denied_methods;  // ["TRACE", "OPTIONS"]
      
      // File handling
      std::string root_directory;              // Override server root
      std::string index_file;                  // "index.php"
      bool directory_listing;                  // true/false
      std::string alias;                       // URL alias mapping
      
      // Upload settings
      std::string upload_directory;            // "/var/uploads"
      size_t upload_max_size;                 // 10485760 (10MB)
      std::vector<std::string> upload_allowed_types; // ["image/*", "text/plain"]
      
      // CGI configuration
      std::string cgi_pass;                   // "/usr/bin/php-cgi"
      std::vector<std::string> cgi_extensions; // [".php", ".py", ".pl"]
      std::map<std::string, std::string> cgi_env; // Environment variables
      int cgi_timeout;                        // 30 seconds
      
      // Redirection
      std::string redirect_url;               // "https://newsite.com"
      int redirect_code;                      // 301, 302, 307, 308
      bool redirect_permanent;                // true for 301, false for 302
      
      // Access control
      std::vector<std::string> allow_ips;     // Route-specific IP whitelist
      std::vector<std::string> deny_ips;      // Route-specific IP blacklist
      bool require_auth;                      // Basic authentication required
      std::string auth_file;                  // ".htpasswd" file path
      
      // Caching
      int cache_max_age;                      // Cache-Control max-age
      bool cache_enabled;                     // Enable/disable caching
  };
  ```
- [ ] **Implement parser logic**
  - Parse nested server blocks with `{}`
  - Handle directive parsing with parameter validation
  - Support include files and variable substitution
  - Implement configuration inheritance (server -> route)
- [ ] **Create sample configuration files**
  ```nginx
  # Global settings
  worker_connections 1024;
  keepalive_timeout 65;
  
  # Main server
  server {
      listen 80 8080;
      server_name localhost example.com;
      client_max_body_size 1M;
      root /var/www/html;
      index index.html index.php;
      
      error_page 404 /404.html;
      error_page 500 502 503 504 /50x.html;
      
      # Static files
      location / {
          allow_methods GET POST;
          autoindex on;
      }
      
      # File uploads
      location /upload {
          allow_methods POST;
          upload_pass /var/uploads;
          client_max_body_size 10M;
      }
      
      # CGI scripts
      location ~ \.php$ {
          allow_methods GET POST;
          cgi_pass /usr/bin/php-cgi;
          cgi_timeout 30;
      }
      
      # API endpoint
      location /api {
          allow_methods GET POST DELETE;
          proxy_pass http://backend;
      }
  }
  
  # HTTPS server
  server {
      listen 443;
      server_name secure.example.com;
      ssl_certificate /etc/ssl/cert.pem;
      ssl_certificate_key /etc/ssl/key.pem;
      
      location / {
          root /var/www/secure;
          allow_methods GET;
      }
  }
  ```

#### Step 4: Socket Management & Basic Server
- [ ] **Socket initialization and management**
  ```cpp
  class SocketManager {
  private:
      std::map<int, int> port_to_fd;          // Port -> Socket FD mapping
      std::map<int, ServerInstance*> fd_to_server; // FD -> Server mapping
      
  public:
      // Socket creation and binding
      int createListeningSocket(int port, const std::string& bind_addr = "0.0.0.0");
      bool bindSocket(int sockfd, int port, const std::string& addr);
      bool setSocketOptions(int sockfd);       // SO_REUSEADDR, etc.
      bool setNonBlocking(int sockfd);        // fcntl O_NONBLOCK
      
      // Socket management
      bool startListening(int sockfd, int backlog = 128);
      int acceptConnection(int server_fd);     // Accept new client
      void closeSocket(int sockfd);
      
      // Utility functions
      std::string getSocketAddress(int sockfd);
      int getSocketPort(int sockfd);
      bool isSocketValid(int sockfd);
  };
  
  // Enhanced ServerInstance with socket management
  class ServerInstance {
  private:
      ServerConfig config;
      std::vector<int> listen_fds;            // Multiple listening sockets
      SocketManager socket_manager;
      
  public:
      bool setupSocket() {
          const std::vector<int>& ports = config.listen_ports;
          
          for (size_t i = 0; i < ports.size(); ++i) {
              int port = ports[i];
              
              // 1. Create socket
              int sockfd = socket_manager.createListeningSocket(port, config.bind_address);
              if (sockfd < 0) {
                  std::cerr << "Failed to create socket for port " << port << std::endl;
                  return false;
              }
              
              // 2. Set socket options
              if (!socket_manager.setSocketOptions(sockfd)) {
                  std::cerr << "Failed to set socket options for port " << port << std::endl;
                  close(sockfd);
                  return false;
              }
              
              // 3. Set non-blocking mode
              if (!socket_manager.setNonBlocking(sockfd)) {
                  std::cerr << "Failed to set non-blocking mode for port " << port << std::endl;
                  close(sockfd);
                  return false;
              }
              
              // 4. Bind to port
              if (!socket_manager.bindSocket(sockfd, port, config.bind_address)) {
                  std::cerr << "Failed to bind socket to port " << port << std::endl;
                  close(sockfd);
                  return false;
              }
              
              listen_fds.push_back(sockfd);
          }
          
          return true;
      }
      
      bool startListening() {
          for (size_t i = 0; i < listen_fds.size(); ++i) {
              if (!socket_manager.startListening(listen_fds[i])) {
                  std::cerr << "Failed to start listening on socket " << listen_fds[i] << std::endl;
                  return false;
              }
          }
          return true;
      }
  };
  ```
- [ ] **Connection acceptance and management**
  ```cpp
  // Connection acceptance in ServerManager
  void ServerManager::handleNewConnection(int server_fd) {
      // 1. Accept new connection
      struct sockaddr_in client_addr;
      socklen_t client_len = sizeof(client_addr);
      
      int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
      if (client_fd < 0) {
          if (errno != EAGAIN && errno != EWOULDBLOCK) {
              std::cerr << "Accept failed: " << strerror(errno) << std::endl;
          }
          return;
      }
      
      // 2. Check connection limits
      if (connection_manager.isAtCapacity()) {
          std::cerr << "Connection limit reached, rejecting client" << std::endl;
          close(client_fd);
          return;
      }
      
      // 3. Set client socket to non-blocking
      if (!setNonBlocking(client_fd)) {
          std::cerr << "Failed to set client socket non-blocking" << std::endl;
          close(client_fd);
          return;
      }
      
      // 4. Create connection object
      Connection* conn = connection_manager.createConnection(client_fd);
      if (!conn) {
          std::cerr << "Failed to create connection object" << std::endl;
          close(client_fd);
          return;
      }
      
      // 5. Register with event loop for reading
      if (!event_loop.addFd(client_fd, EventLoop::READ_EVENT, this)) {
          std::cerr << "Failed to register client socket with event loop" << std::endl;
          connection_manager.destroyConnection(client_fd);
          return;
      }
      
      // 6. Store client information
      conn->setClientAddress(inet_ntoa(client_addr.sin_addr));
      conn->setClientPort(ntohs(client_addr.sin_port));
      
      std::cout << "New connection from " << conn->getClientAddress() 
                << ":" << conn->getClientPort() << std::endl;
  }
  
  bool setNonBlocking(int sockfd) {
      int flags = fcntl(sockfd, F_GETFL, 0);
      if (flags < 0) {
          std::cerr << "fcntl F_GETFL failed: " << strerror(errno) << std::endl;
          return false;
      }
      
      flags |= O_NONBLOCK;
      if (fcntl(sockfd, F_SETFL, flags) < 0) {
          std::cerr << "fcntl F_SETFL failed: " << strerror(errno) << std::endl;
          return false;
      }
      
      return true;
  }
  ```
- [ ] **Error handling and resource cleanup**
  - Handle `EAGAIN`/`EWOULDBLOCK` properly
  - Implement graceful socket closure
  - Clean up resources on errors
  - Log socket operations for debugging

#### Step 5: I/O Multiplexing Implementation
- [ ] **Choose and implement multiplexing method**
  ```cpp
  // Cross-platform event loop implementation
  #ifdef __linux__
  #include <sys/epoll.h>
  typedef struct epoll_event PlatformEvent;
  #define PLATFORM_READ_EVENT EPOLLIN
  #define PLATFORM_WRITE_EVENT EPOLLOUT
  #define PLATFORM_ERROR_EVENT (EPOLLERR | EPOLLHUP)
  #elif defined(__APPLE__) || defined(__FreeBSD__)
  #include <sys/event.h>
  typedef struct kevent PlatformEvent;
  #define PLATFORM_READ_EVENT EVFILT_READ
  #define PLATFORM_WRITE_EVENT EVFILT_WRITE
  #define PLATFORM_ERROR_EVENT EV_ERROR
  #else
  #include <poll.h>
  typedef struct pollfd PlatformEvent;
  #define PLATFORM_READ_EVENT POLLIN
  #define PLATFORM_WRITE_EVENT POLLOUT
  #define PLATFORM_ERROR_EVENT (POLLERR | POLLHUP)
  #endif
  
  class EventLoop {
  private:
      int platform_fd;                       // epoll_fd, kqueue_fd, or -1 for poll
      std::vector<PlatformEvent> events;     // Event buffer
      std::map<int, EventHandler*> handlers; // FD -> Handler mapping
      std::set<int> monitored_fds;           // Currently monitored file descriptors
      bool running;                          // Loop control
      
      // Platform-specific implementations
      bool initializeEpoll();               // Linux epoll
      bool initializeKqueue();              // macOS/BSD kqueue
      bool initializePoll();                // Fallback poll
      
      // Event processing
      int waitForEvents(int timeout_ms);    // Platform-specific wait
      void processEpollEvents(int num_events);
      void processKqueueEvents(int num_events);
      void processPollEvents(int num_events);
      
  public:
      EventLoop();
      ~EventLoop();
      
      // Initialization
      bool initialize() {
  #ifdef __linux__
          return initializeEpoll();
  #elif defined(__APPLE__) || defined(__FreeBSD__)
          return initializeKqueue();
  #else
          return initializePoll();
  #endif
      }
      
      // Main loop
      void run() {
          running = true;
          std::cout << "Event loop started" << std::endl;
          
          while (running) {
              // 1. Wait for events (1 second timeout)
              int num_events = waitForEvents(1000);
              
              if (num_events < 0) {
                  if (errno == EINTR) continue; // Interrupted by signal
                  std::cerr << "Event wait failed: " << strerror(errno) << std::endl;
                  break;
              }
              
              // 2. Process events
              if (num_events > 0) {
  #ifdef __linux__
                  processEpollEvents(num_events);
  #elif defined(__APPLE__) || defined(__FreeBSD__)
                  processKqueueEvents(num_events);
  #else
                  processPollEvents(num_events);
  #endif
              }
              
              // 3. Handle periodic tasks
              handlePeriodicTasks();
          }
          
          std::cout << "Event loop stopped" << std::endl;
      }
      
      void stop() { running = false; }
      
      // Event registration
      bool addFd(int fd, int events, EventHandler* handler) {
          if (monitored_fds.find(fd) != monitored_fds.end()) {
              std::cerr << "FD " << fd << " already monitored" << std::endl;
              return false;
          }
          
          bool success = false;
  #ifdef __linux__
          success = addEpollFd(fd, events);
  #elif defined(__APPLE__) || defined(__FreeBSD__)
          success = addKqueueFd(fd, events);
  #else
          success = addPollFd(fd, events);
  #endif
          
          if (success) {
              handlers[fd] = handler;
              monitored_fds.insert(fd);
          }
          
          return success;
      }
      
      bool modifyFd(int fd, int events) {
          if (monitored_fds.find(fd) == monitored_fds.end()) {
              std::cerr << "FD " << fd << " not monitored" << std::endl;
              return false;
          }
          
  #ifdef __linux__
          return modifyEpollFd(fd, events);
  #elif defined(__APPLE__) || defined(__FreeBSD__)
          return modifyKqueueFd(fd, events);
  #else
          return modifyPollFd(fd, events);
  #endif
      }
      
      bool removeFd(int fd) {
          if (monitored_fds.find(fd) == monitored_fds.end()) {
              return true; // Already removed
          }
          
          bool success = false;
  #ifdef __linux__
          success = removeEpollFd(fd);
  #elif defined(__APPLE__) || defined(__FreeBSD__)
          success = removeKqueueFd(fd);
  #else
          success = removePollFd(fd);
  #endif
          
          if (success) {
              handlers.erase(fd);
              monitored_fds.erase(fd);
          }
          
          return success;
      }
      
  private:
      // Linux epoll implementation
      bool addEpollFd(int fd, int events) {
          struct epoll_event ev;
          ev.events = events | EPOLLET; // Edge-triggered
          ev.data.fd = fd;
          return epoll_ctl(platform_fd, EPOLL_CTL_ADD, fd, &ev) == 0;
      }
      
      bool modifyEpollFd(int fd, int events) {
          struct epoll_event ev;
          ev.events = events | EPOLLET;
          ev.data.fd = fd;
          return epoll_ctl(platform_fd, EPOLL_CTL_MOD, fd, &ev) == 0;
      }
      
      bool removeEpollFd(int fd) {
          return epoll_ctl(platform_fd, EPOLL_CTL_DEL, fd, NULL) == 0;
      }
  };
  ```
- [ ] **Connection state management with finite state machine**
  ```cpp
  class Connection : public EventHandler {
  private:
      ConnectionState state;
      ConnectionSubState sub_state;
      time_t state_change_time;
      
  public:
      // EventHandler implementation
      void handleRead(int fd) override {
          switch (state) {
              case CONN_READING_HEADERS:
                  handleReadHeaders(fd);
                  break;
              case CONN_READING_BODY:
                  handleReadBody(fd);
                  break;
              default:
                  std::cerr << "Unexpected read event in state " << state << std::endl;
                  break;
          }
      }
      
      void handleWrite(int fd) override {
          switch (state) {
              case CONN_WRITING_RESPONSE:
                  handleWriteResponse(fd);
                  break;
              default:
                  std::cerr << "Unexpected write event in state " << state << std::endl;
                  break;
          }
      }
      
      void handleError(int fd) override {
          std::cerr << "Error on connection " << fd << std::endl;
          setState(CONN_ERROR);
      }
      
  private:
      void handleReadHeaders(int fd) {
          char buffer[4096];
          ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
          
          if (bytes_read > 0) {
              read_buffer.append(buffer, bytes_read);
              
              // Check for complete headers (\r\n\r\n)
              size_t header_end = read_buffer.find("\r\n\r\n");
              if (header_end != std::string::npos) {
                  // Headers complete, parse them
                  std::string headers = read_buffer.substr(0, header_end);
                  if (parseHeaders(headers)) {
                      // Determine if body is expected
                      if (hasRequestBody()) {
                          setState(CONN_READING_BODY);
                          // Process any body data already read
                          std::string body_data = read_buffer.substr(header_end + 4);
                          if (!body_data.empty()) {
                              processBodyData(body_data);
                          }
                      } else {
                          setState(CONN_PROCESSING);
                          processRequest();
                      }
                  } else {
                      setState(CONN_ERROR);
                  }
              }
          } else if (bytes_read == 0) {
              // Client closed connection
              setState(CONN_CLOSING);
          } else {
              // Error occurred
              if (errno != EAGAIN && errno != EWOULDBLOCK) {
                  setState(CONN_ERROR);
              }
          }
      }
  };
  ```
- [ ] **Timeout handling and connection cleanup**
  ```cpp
  class ConnectionManager {
  private:
      std::map<int, Connection*> connections;
      int default_timeout;
      
  public:
      void cleanupTimedOutConnections() {
          time_t now = time(NULL);
          std::vector<int> to_remove;
          
          for (std::map<int, Connection*>::iterator it = connections.begin();
               it != connections.end(); ++it) {
              Connection* conn = it->second;
              
              if (conn->hasTimedOut(default_timeout)) {
                  std::cout << "Connection " << it->first << " timed out" << std::endl;
                  to_remove.push_back(it->first);
              }
          }
          
          // Remove timed-out connections
          for (size_t i = 0; i < to_remove.size(); ++i) {
              destroyConnection(to_remove[i]);
          }
      }
  };
  ```

### Phase 3: HTTP Protocol Implementation

#### Step 6: HTTP Request Parser
- [ ] **Request line parsing**
  - Extract method (GET, POST, DELETE)
  - Parse URI and decode URL encoding
  - Validate HTTP version
- [ ] **Header parsing**
  - Parse all headers into key-value pairs
  - Handle multi-line headers
  - Validate required headers
- [ ] **Body handling**
  - Content-Length based reading
  - Chunked transfer encoding
  - File upload handling (multipart/form-data)
- [ ] **Request validation**
  - Check method validity
  - Validate URI format
  - Enforce body size limits

#### Step 7: HTTP Response Generation
- [ ] **Status code handling**
  - Implement all required status codes
  - Create appropriate response messages
- [ ] **Header generation**
  - Content-Type detection
  - Content-Length calculation
  - Date and Server headers
- [ ] **Response body creation**
  - File serving
  - Directory listing HTML
  - Error page generation

### Phase 4: Core Features

#### Step 8: Static File Serving (GET)
- [ ] **File system operations**
  - Secure path validation (prevent directory traversal)
  - File existence and permission checks
  - MIME type detection based on extensions
- [ ] **Directory handling**
  - Index file serving (index.html, etc.)
  - Directory listing generation
  - Navigation HTML creation
- [ ] **Optimization**
  - Partial content support (Range headers)
  - Caching headers (Last-Modified, ETag)

#### Step 9: File Upload Handling (POST)
- [ ] **Form data parsing**
  - application/x-www-form-urlencoded
  - multipart/form-data
  - Raw body handling
- [ ] **File storage**
  - Secure upload directory handling
  - Filename sanitization
  - Storage space management
- [ ] **Upload validation**
  - File size limits
  - File type restrictions
  - Security checks

#### Step 10: File Deletion (DELETE)
- [ ] **Resource identification**
  - Map URI to file path
  - Security validation
- [ ] **Deletion operations**
  - File deletion
  - Permission checks
  - Error handling

### Phase 5: Advanced Features

#### Step 11: CGI Implementation
- [ ] **CGI detection**
  - File extension matching (.php, .py, etc.)
  - CGI executable location
- [ ] **Environment setup**
  ```cpp
  // Required CGI environment variables
  setenv("REQUEST_METHOD", method.c_str(), 1);
  setenv("QUERY_STRING", query.c_str(), 1);
  setenv("CONTENT_TYPE", content_type.c_str(), 1);
  setenv("CONTENT_LENGTH", content_length.c_str(), 1);
  setenv("SCRIPT_NAME", script_name.c_str(), 1);
  // ... other variables
  ```
- [ ] **Process management**
  - Fork and exec CGI processes
  - Pipe setup for stdin/stdout
  - Process timeout handling
- [ ] **Data exchange**
  - Send request body to CGI
  - Read CGI output
  - Handle chunked CGI responses

#### Step 12: Error Handling & Resilience
- [ ] **Connection management**
  - Handle client disconnections
  - Connection timeout implementation
  - Resource cleanup
- [ ] **Error responses**
  - Custom error pages
  - Appropriate status codes
  - Logging system
- [ ] **Resource limits**
  - Memory usage monitoring
  - Connection limits
  - Request size limits

### Phase 6: Testing & Validation

#### Step 13: Unit Testing
- [ ] **Configuration parser tests**
  - Valid configurations
  - Invalid syntax handling
  - Edge cases
- [ ] **HTTP parser tests**
  - Various request formats
  - Malformed requests
  - Large headers/bodies
- [ ] **Response generation tests**
  - Different content types
  - Error conditions
  - Header correctness

#### Step 14: Integration Testing
- [ ] **Browser compatibility**
  - Chrome, Firefox, Safari testing
  - Different request types
  - File upload/download
- [ ] **Command line testing**
  ```bash
  # Basic GET request
  curl -v http://localhost:8080/
  
  # File upload
  curl -X POST -F "file=@test.txt" http://localhost:8080/upload
  
  # DELETE request
  curl -X DELETE http://localhost:8080/file.txt
  
  # CGI testing
  curl http://localhost:8080/script.php
  ```
- [ ] **Telnet testing**
  ```bash
  telnet localhost 8080
  GET / HTTP/1.1
  Host: localhost
  
  ```

#### Step 15: Stress Testing
- [ ] **Concurrent connections**
  - Use `ab` (Apache Bench) for load testing
  - Test with multiple simultaneous uploads
  - Monitor memory usage
- [ ] **Edge case testing**
  - Very large files
  - Malformed requests
  - Rapid connect/disconnect
- [ ] **Memory leak detection**
  - Use Valgrind or AddressSanitizer
  - Long-running tests
  - Resource monitoring

## 🔧 Development Guidelines

### Code Organization
```
webserv/
├── Makefile
├── README.md
├── config/
│   ├── default.conf              # Basic server configuration
│   ├── advanced.conf             # Multi-server setup
│   ├── cgi.conf                  # CGI-focused configuration
│   └── ssl.conf                  # HTTPS configuration example
├── includes/
│   ├── webserv.hpp               # Main header with common includes
│   ├── config/
│   │   └── ConfigParser.hpp      # Configuration parsing
│   ├── server/
│   │   ├── ServerManager.hpp     # Central server orchestrator
│   │   ├── ServerInstance.hpp    # Individual server instance
│   │   ├── Connection.hpp        # Client connection management
│   │   ├── ConnectionManager.hpp # Connection pool management
│   │   ├── EventLoop.hpp         # I/O event handling
│   │   └── SocketManager.hpp     # Socket operations
│   ├── http/
│   │   ├── HttpRequest.hpp       # HTTP request parsing
│   │   ├── HttpResponse.hpp      # HTTP response building
│   │   ├── HttpHeaders.hpp       # Header management
│   │   └── HttpStatus.hpp        # Status code definitions
│   ├── handlers/
│   │   ├── RequestHandler.hpp    # Base request handler
│   │   ├── RouteHandler.hpp      # Route matching and dispatch
│   │   ├── MethodHandler.hpp     # HTTP method handling
│   │   ├── FileHandler.hpp       # Static file serving
│   │   ├── UploadHandler.hpp     # File upload processing
│   │   └── ErrorHandler.hpp     # Error response generation
│   ├── cgi/
│   │   ├── CgiHandler.hpp        # CGI script execution
│   │   ├── CgiProcess.hpp        # Process management
│   │   └── CgiEnvironment.hpp    # Environment setup
│   └── utils/
│       ├── Utils.hpp             # General utilities
│       ├── Logger.hpp            # Logging system
│       ├── MimeTypes.hpp         # MIME type detection
│       └── Security.hpp          # Security utilities
├── src/
│   ├── main.cpp                  # Application entry point
│   ├── config/
│   │   └── ConfigParser.cpp      # Configuration file parsing
│   ├── server/
│   │   ├── ServerManager.cpp     # Main server orchestration
│   │   ├── ServerInstance.cpp    # Server instance implementation
│   │   ├── Connection.cpp        # Connection handling
│   │   ├── ConnectionManager.cpp # Connection pool management
│   │   ├── EventLoop.cpp         # Event loop implementation
│   │   └── SocketManager.cpp     # Socket management
│   ├── http/
│   │   ├── HttpRequest.cpp       # Request parsing logic
│   │   ├── HttpResponse.cpp      # Response building logic
│   │   ├── HttpHeaders.cpp       # Header manipulation
│   │   └── HttpStatus.cpp        # Status code utilities
│   ├── handlers/
│   │   ├── RequestHandler.cpp    # Base handler implementation
│   │   ├── RouteHandler.cpp      # Route matching logic
│   │   ├── MethodHandler.cpp     # Method-specific handlers
│   │   ├── FileHandler.cpp       # File serving logic
│   │   ├── UploadHandler.cpp     # Upload processing
│   │   └── ErrorHandler.cpp      # Error handling
│   ├── cgi/
│   │   ├── CgiHandler.cpp        # CGI execution logic
│   │   ├── CgiProcess.cpp        # Process management
│   │   └── CgiEnvironment.cpp    # Environment variable setup
│   └── utils/
│       ├── Utils.cpp             # Utility functions
│       ├── Logger.cpp            # Logging implementation
│       ├── MimeTypes.cpp         # MIME type detection
│       └── Security.cpp          # Security functions
├── tests/
│   ├── unit/
│   │   ├── test_config_parser.cpp    # Configuration parsing tests
│   │   ├── test_http_parser.cpp      # HTTP parsing tests
│   │   ├── test_uri_validation.cpp   # URI validation tests
│   │   └── test_utils.cpp            # Utility function tests
│   ├── integration/
│   │   ├── test_server_basic.cpp     # Basic server functionality
│   │   ├── test_file_serving.cpp     # Static file serving
│   │   ├── test_file_upload.cpp      # File upload functionality
│   │   ├── test_cgi_execution.cpp    # CGI script execution
│   │   └── test_error_handling.cpp   # Error scenarios
│   ├── stress/
│   │   ├── concurrent_connections.py # Connection stress testing
│   │   ├── large_file_upload.py      # Large file handling
│   │   ├── memory_leak_test.sh       # Memory leak detection
│   │   └── load_test.sh              # Load testing with ab/siege
│   └── scripts/
│       ├── run_all_tests.sh          # Test runner script
│       ├── setup_test_env.sh         # Test environment setup
│       └── benchmark.sh              # Performance benchmarking
├── www/
│   ├── index.html                # Default landing page
│   ├── error_pages/
│   │   ├── 404.html              # Not Found page
│   │   ├── 403.html              # Forbidden page
│   │   ├── 500.html              # Internal Server Error
│   │   └── 502.html              # Bad Gateway page
│   ├── static/
│   │   ├── css/
│   │   │   └── style.css         # Stylesheet
│   │   ├── js/
│   │   │   └── script.js         # JavaScript
│   │   └── images/
│   │       └── logo.png          # Images
│   ├── cgi-bin/
│   │   ├── test.php              # PHP test script
│   │   ├── info.py               # Python info script
│   │   ├── upload.pl             # Perl upload handler
│   │   └── env.sh                # Environment display script
│   └── uploads/                  # File upload directory
│       └── README.txt            # Upload directory info
├── logs/
│   ├── access.log                # Access log file
│   ├── error.log                 # Error log file
│   └── debug.log                 # Debug information
└── docs/
    ├── API.md                    # API documentation
    ├── CONFIGURATION.md          # Configuration guide
    ├── DEPLOYMENT.md             # Deployment instructions
    ├── TESTING.md                # Testing procedures
    └── TROUBLESHOOTING.md        # Common issues and solutions
```

#### Class Hierarchy and Dependencies
```
ServerManager (Main orchestrator)
├── ConfigParser (Configuration management)
├── EventLoop (I/O event handling)
├── ConnectionManager (Connection pool)
│   └── Connection[] (Individual connections)
├── ServerInstance[] (Multiple server instances)
│   ├── SocketManager (Socket operations)
│   ├── RouteHandler (Route matching)
│   ├── MethodHandler (HTTP methods)
│   │   ├── FileHandler (GET - static files)
│   │   ├── UploadHandler (POST - file uploads)
│   │   └── DeleteHandler (DELETE - file removal)
│   └── CgiHandler (CGI execution)
│       ├── CgiProcess (Process management)
│       └── CgiEnvironment (Environment setup)
├── HttpRequest (Request parsing)
├── HttpResponse (Response building)
└── Utils (Utility functions)
    ├── Logger (Logging system)
    ├── MimeTypes (Content-Type detection)
    └── Security (Security functions)
```

#### Component Interaction Flow
```
1. main() → ServerManager::initialize()
2. ServerManager → ConfigParser::parseConfigFile()
3. ServerManager → createServerInstances()
4. ServerInstance → SocketManager::setupSocket()
5. ServerManager → EventLoop::initialize()
6. EventLoop::run() → ServerManager::handleNewConnection()
7. ServerManager → ConnectionManager::createConnection()
8. Connection::handleRead() → HttpRequest::parse()
9. ServerInstance::processRequest() → RouteHandler::matchRoute()
10. RouteHandler → MethodHandler::handle()
11. MethodHandler → HttpResponse::build()
12. Connection::handleWrite() → send response
```

### Key Implementation Tips

1. **Non-blocking I/O Rules**
   - ALWAYS check poll()/epoll before read/write
   - Handle EAGAIN/EWOULDBLOCK properly
   - Never block the main event loop

2. **Memory Management**
   - Use RAII principles consistently
   - Handle all allocation failures
   - Clean up on client disconnection

3. **Security Considerations**
   - Validate all user input
   - Prevent directory traversal attacks
   - Sanitize file paths and names
   - Limit resource usage

4. **Error Handling**
   - Return appropriate HTTP status codes
   - Log errors for debugging
   - Never crash on invalid input

## 🚀 Testing Commands

### Basic Functionality
```bash
# Start server
./webserv config/default.conf

# Test basic GET
curl -v http://localhost:8080/

# Test file upload
curl -X POST -F "file=@README.md" http://localhost:8080/upload/

# Test file deletion
curl -X DELETE http://localhost:8080/uploads/README.md

# Test CGI (if implemented)
curl http://localhost:8080/cgi-bin/test.php
```

### Stress Testing
```bash
# Apache Bench - 1000 requests, 10 concurrent
ab -n 1000 -c 10 http://localhost:8080/

# Siege testing
siege -c 20 -t 60s http://localhost:8080/

# Memory leak testing
valgrind --leak-check=full ./webserv config/default.conf
```

## 📚 References

- [HTTP/1.0 RFC 1945](https://tools.ietf.org/html/rfc1945)
- [HTTP/1.1 RFC 2616](https://tools.ietf.org/html/rfc2616)
- [CGI Specification RFC 3875](https://tools.ietf.org/html/rfc3875)
- [NGINX Configuration Guide](https://nginx.org/en/docs/beginners_guide.html)

## ✅ Completion Checklist

### Mandatory Features
- [ ] Configuration file parsing
- [ ] Non-blocking I/O with poll/epoll
- [ ] GET method (static files)
- [ ] POST method (file uploads)
- [ ] DELETE method
- [ ] CGI execution
- [ ] Error handling
- [ ] Multiple port support

### Testing
- [ ] Browser compatibility
- [ ] Telnet testing
- [ ] Stress testing
- [ ] Memory leak testing

### Documentation
- [ ] Configuration examples
- [ ] Usage instructions
- [ ] Test procedures

Remember: Start simple, build incrementally, and test thoroughly at each step. The key to success is maintaining non-blocking behavior throughout and handling all edge cases gracefully.
