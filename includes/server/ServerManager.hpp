#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include "../webserv.hpp"
#include "../config/ConfigParser.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../http/HttpTemplates.hpp"
#include "../cgi/CgiHandler.hpp"
#include "../utils/Utils.hpp"
#include <poll.h>
#include <sys/wait.h>

// Connection states for managing client lifecycle
enum ClientState {
    CLIENT_READING_HEADERS,
    CLIENT_READING_BODY,
    CLIENT_PROCESSING,
    CLIENT_WRITING_RESPONSE,
    CLIENT_ERROR,
    CLIENT_DONE
};

// Client connection data structure
struct ClientConnection {
    int fd;
    ClientState state;
    time_t last_activity;
    time_t connection_start;          // Track connection start time
    size_t requests_handled;          // Count requests on this connection
    std::string buffer;
    size_t buffer_pos;
    HttpRequest request;
    HttpResponse response;
    std::string response_buffer;
    size_t response_sent;
    bool keep_alive;
    const ServerConfig* server_config;
    
    ClientConnection() : fd(-1), state(CLIENT_READING_HEADERS), last_activity(0), 
                        connection_start(0), requests_handled(0), buffer_pos(0), 
                        response_sent(0), keep_alive(false), server_config(NULL) {}
};

// Server socket structure
struct ServerSocket {
    int fd;
    int port;
    const ServerConfig* config;
    
    ServerSocket() : fd(-1), port(0), config(NULL) {}
};
 
class ServerManager {
private:
    // Configuration
    std::vector<ServerConfig> _server_configs;
    std::vector<ServerSocket> _server_sockets;
    
    // Poll structures
    std::vector<struct pollfd> _pollfds;
    std::map<int, ClientConnection> _clients;
    
    // State management
    bool _running;
    time_t _last_cleanup;
    
    // Constants
    private:
    // Enhanced timeout constants for keep-alive support
    static const int TIMEOUT_MS = 1000;              // 1 second poll timeout
    static const int CLIENT_TIMEOUT = 30;            // 30 seconds for request completion
    static const int KEEPALIVE_TIMEOUT = 60;         // 60 seconds keep-alive timeout
    static const int MAX_CONNECTION_TIME = 300;      // 5 minutes maximum connection time
    static const int MAX_REQUESTS_PER_CONNECTION = 100; // Max requests per persistent connection
    static const int CLEANUP_INTERVAL = 5;           // Run cleanup every 5 seconds
    static const int MAX_CON = 1024;
    
    // Private methods - all designed to never crash
    bool initializeServerSockets();
    bool createServerSocket(const ServerConfig& config);
    const ServerSocket* findServerSocket(int fd) const;
    bool isServerSocket(int fd) const;
    void handleNewConnection(const ServerSocket& server_socket);
    void handleClientRead(ClientConnection& client);
    void handleClientWrite(ClientConnection& client);
    void processClientRequest(ClientConnection& client);
    void closeClient(int client_fd, const std::string& reason = "");
    void cleanupTimeouts();
    void setupPollStructure();
    const ServerConfig* findServerConfig(int port, const std::string& host = "") const;
    
    // Error handling - these methods never throw or crash
    bool safeRead(int fd, std::string& buffer, size_t max_size = BUFFER_SIZE);
    bool safeWrite(int fd, const std::string& data, size_t& bytes_written);
    void logError(const std::string& operation, const std::string& details = "");
    void handleOutOfMemory();
    
    // HTTP processing
    void generateErrorResponse(ClientConnection& client, int status_code, const std::string& message = "");
    void generateFileResponse(ClientConnection& client, const std::string& filepath);
    void generateDirectoryListing(ClientConnection& client, const std::string& path, const std::string& uri);
    bool handleCGI(ClientConnection& client, const std::string& script_path);
    
    // Keep-alive support methods
    bool shouldKeepConnectionAlive(const ClientConnection& client);
    void resetClientForNextRequest(ClientConnection& client);
    bool determineKeepAliveSupport(const ClientConnection& client);
    void setConnectionHeader(ClientConnection& client);
    std::string findIndexFile(const std::string& dir_path, 
                             const ServerConfig* server_config, 
                             const RouteConfig* route) const;
    
    // Request routing
    const RouteConfig* findRoute(const ServerConfig& server, const std::string& uri) const;
    std::string resolvePath(const ServerConfig& server, const RouteConfig* route, const std::string& uri) const;
    bool isMethodAllowed(const RouteConfig& route, HttpMethod method) const;
    
public:
    ServerManager();
    ~ServerManager();
    
    // Main interface - these methods handle all errors gracefully
    bool initialize(const std::string& config_file);
    void run();
    void shutdown();
    void requestShutdown(); // Signal safe shutdown request
    
    // Status and debugging
    size_t getConnectionCount() const { return _clients.size(); }
    bool isRunning() const { return _running; }
    void printStatus() const;
};

#endif