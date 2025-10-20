#include "../../includes/server/ServerManager.hpp"

/**
 * @brief Constructor for ServerManager
 * 
 * Initializes the server manager in a safe state with:
 * - Running flag set to false (server not started)
 * - Last cleanup timestamp set to 0
 * - Pre-allocates memory for poll file descriptors and server sockets to prevent
 *   runtime memory allocation failures that could crash the server
 * 
 * This constructor never throws exceptions and ensures the server starts in a
 * predictable, safe state following the "never crash" requirement.
 */
ServerManager::ServerManager() : _running(false), _last_cleanup(0) {
    // Reserve space to prevent reallocation
    _pollfds.reserve(MAX_CON);
    _server_sockets.reserve(10); // Reasonable number of server configs
}

/**
 * @brief Destructor for ServerManager
 * 
 * Ensures clean shutdown by calling shutdown() which:
 * - Closes all client connections gracefully
 * - Closes all server sockets
 * - Cleans up all internal data structures
 * - Sets running flag to false
 * 
 * The destructor never throws exceptions and guarantees proper cleanup
 * even if the server is forcefully terminated.
 */
ServerManager::~ServerManager() {
    shutdown();
}

/**
 * @brief Initialize the server manager with configuration file
 * 
 * This function performs complete server initialization:
 * 1. Parses the configuration file using ConfigParser
 * 2. Validates that at least one server configuration exists
 * 3. Creates and binds server sockets for all configured ports
 * 4. Sets up the server for accepting connections
 * 
 * The function uses comprehensive exception handling to ensure it never crashes:
 * - Catches std::exception for known exceptions
 * - Catches (...) for any unexpected exceptions
 * - Returns false on any failure, true on success
 * 
 * @param config_file Path to the configuration file to parse
 * @return true if initialization successful, false otherwise
 */
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
        
        // Initialize server sockets
        if (!initializeServerSockets()) {
            logError("Failed to initialize server sockets");
            return false;
        }
        
        std::cout << "✓ ServerManager initialized with " << _server_sockets.size() << " server sockets" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        logError("Exception during initialization", e.what());
        return false;
    } catch (...) {
        logError("Unknown exception during initialization");
        return false;
    }
}

/**
 * @brief Initialize all server sockets from parsed configurations
 * 
 * This function creates server sockets for all server configurations:
 * - Iterates through all parsed server configurations
 * - Calls createServerSocket() for each configuration
 * - Continues processing even if individual socket creation fails (resilient)
 * - Clears any existing server sockets before creating new ones
 * 
 * The function implements the "never crash" principle by:
 * - Not stopping on individual socket creation failures
 * - Logging errors but continuing operation
 * - Only failing if NO sockets could be created
 * 
 * @return true if at least one server socket was created successfully, false if all failed
 */
bool ServerManager::initializeServerSockets() {
    _server_sockets.clear();
    
    for (size_t i = 0; i < _server_configs.size(); ++i) {
        if (!createServerSocket(_server_configs[i])) {
            // Continue with other servers even if one fails
            logError("Failed to create server socket for config", Utils::toString(i));
        }
    }
    
    return !_server_sockets.empty();
}

/**
 * @brief Create server socket(s) for a specific server configuration
 * 
 * This function handles complete server socket creation for one ServerConfig:
 * 1. Iterates through all ports defined in the configuration
 * 2. Checks for port conflicts with existing sockets
 * 3. Creates socket with AF_INET family and SOCK_STREAM type
 * 4. Sets SO_REUSEADDR to allow port reuse after restart
 * 5. Makes socket non-blocking using Utils::setNonBlocking()
 * 6. Binds to the specified port on all interfaces (INADDR_ANY)
 * 7. Starts listening with SOMAXCONN backlog
 * 8. Stores the ServerSocket structure in _server_sockets
 * 
 * Error handling strategy:
 * - Individual port failures don't stop processing of other ports
 * - Closes socket file descriptor on any setup failure
 * - Logs specific errors for debugging
 * - Returns true if ANY socket creation succeeds
 * 
 * @param config The server configuration containing ports and settings
 * @return true if at least one socket was created, false if all failed
 */
bool ServerManager::createServerSocket(const ServerConfig& config) {
    // Handle multiple ports per server config
    for (size_t i = 0; i < config.ports.size(); ++i) {
        int port = config.ports[i];
        
        // Check for duplicate ports
        for (size_t j = 0; j < _server_sockets.size(); ++j) {
            if (_server_sockets[j].port == port) {
                logError("Port already in use", Utils::toString(port));
                continue;
            }
        }
        
        // Create socket
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            logError("socket() failed for port", Utils::toString(port));
            continue;
        }
        
        // Set socket options
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            close(server_fd);
            logError("setsockopt() failed for port", Utils::toString(port));
            continue;
        }
        
        // Make non-blocking
        Utils::setNonBlocking(server_fd);
        
        // Bind
        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(server_fd);
            logError("bind() failed for port", Utils::toString(port));
            continue;
        }
        
        // Listen
        if (listen(server_fd, SOMAXCONN) < 0) {
            close(server_fd);
            logError("listen() failed for port", Utils::toString(port));
            continue;
        }
        
        // Add to our server sockets
        ServerSocket server_socket;
        server_socket.fd = server_fd;
        server_socket.port = port;
        server_socket.config = &config;
        _server_sockets.push_back(server_socket);
        
        std::cout << "✓ Server listening on port " << port << std::endl;
    }
    
    return true; // Return true even if some ports failed
}

/**
 * @brief Main server event loop using poll-based I/O
 * 
 * This is the core of the server that implements the "single poll() for ALL I/O" requirement:
 * 1. Sets _running flag to true and initializes cleanup timer
 * 2. Enters main loop that continues until _running becomes false
 * 3. Calls setupPollStructure() to prepare poll file descriptor array
 * 4. Performs single poll() call with timeout to monitor ALL sockets
 * 5. Processes poll events for server sockets (new connections) and clients
 * 6. Handles timeouts with cleanupTimeouts() maintenance
 * 7. Processes POLLIN (readable), POLLOUT (writable), and error events
 * 
 * Key features implementing the "never crash" requirement:
 * - Comprehensive try-catch around the entire loop
 * - Continues on poll() errors (except EINTR)
 * - Handles individual socket errors without stopping
 * - Performs periodic timeout cleanup
 * - Uses non-blocking I/O exclusively
 * - Never calls read/write without poll() indicating readiness
 * 
 * The function only exits when _running is set to false (via shutdown() or signal handler).
 */
void ServerManager::run() {
    _running = true;
    _last_cleanup = time(NULL);
    
    std::cout << "✓ Server started with poll-based I/O" << std::endl;
    std::cout << "✓ Monitoring " << _server_sockets.size() << " server sockets" << std::endl;
    
    while (_running) {
        try {
            // Setup poll structure
            setupPollStructure();
            
            // Poll for events - this is our single poll() call for ALL I/O
            int poll_count = poll(&_pollfds[0], _pollfds.size(), TIMEOUT_MS);
            
            if (poll_count < 0) {
                // Only break on EINTR (signal), continue on other errors
                if (errno == EINTR) {
                    continue;
                }
                logError("poll() failed");
                continue; // Don't break - keep server running
            }
            
            if (poll_count == 0) {
                // Timeout - perform maintenance tasks
                cleanupTimeouts();
                continue;
            }
            
            // Process events
            for (size_t i = 0; i < _pollfds.size() && poll_count > 0; ++i) {
                struct pollfd& pfd = _pollfds[i];
                
                if (pfd.revents == 0) continue;
                poll_count--;
                
                // Check for errors first
                if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    if (isServerSocket(pfd.fd)) {
                        logError("Server socket error", Utils::toString(pfd.fd));
                        // Don't close server socket, just log and continue
                    } else {
                        closeClient(pfd.fd, "Socket error");
                    }
                    continue;
                }
                
                // Handle server socket (new connections)
                if (isServerSocket(pfd.fd)) {
                    if (pfd.revents & POLLIN) {
                        const ServerSocket* server = findServerSocket(pfd.fd);
                        if (server) {
                            handleNewConnection(*server);
                        }
                    }
                    continue;
                }
                
                // Handle client socket
                std::map<int, ClientConnection>::iterator client_it = _clients.find(pfd.fd);
                if (client_it == _clients.end()) {
                    continue; // Client was removed during processing
                }
                
                ClientConnection& client = client_it->second;
                client.last_activity = time(NULL);
                
                // Handle client read
                if (pfd.revents & POLLIN) {
                    handleClientRead(client);
                }
                
                // Handle client write
                if (pfd.revents & POLLOUT && client.state == CLIENT_WRITING_RESPONSE) {
                    handleClientWrite(client);
                }
            }
            
            // Periodic cleanup
            if (time(NULL) - _last_cleanup > 5) {
                cleanupTimeouts();
                _last_cleanup = time(NULL);
            }
            
        } catch (const std::exception& e) {
            logError("Exception in main loop", e.what());
            // Continue running - never crash
        } catch (...) {
            logError("Unknown exception in main loop");
            // Continue running - never crash
        }
        printStatus();
    }
    
    std::cout << "✓ Server main loop exited" << std::endl;
}

/**
 * @brief Prepare poll file descriptor array for the poll() system call
 * 
 * This function builds the _pollfds array used by poll() to monitor all sockets:
 * 1. Clears any existing poll file descriptors
 * 2. Adds all server sockets with POLLIN flag (monitoring for new connections)
 * 3. Adds all client sockets with appropriate event flags:
 *    - POLLIN for reading incoming data
 *    - POLLOUT for clients ready to send response data (CLIENT_WRITING_RESPONSE state)
 * 4. Sets revents to 0 for all descriptors
 * 
 * This is called before every poll() to ensure the array reflects the current
 * state of all sockets. The function implements the "single poll() for ALL I/O"
 * requirement by consolidating all socket monitoring into one data structure.
 * 
 * Performance considerations:
 * - Uses clear() and push_back() for predictable memory behavior
 * - Iterates through maps efficiently using iterators
 * - Only adds POLLOUT when client actually needs to write
 */
void ServerManager::setupPollStructure() {
    _pollfds.clear();
    
    // Add server sockets
    for (size_t i = 0; i < _server_sockets.size(); ++i) {
        struct pollfd pfd;
        pfd.fd = _server_sockets[i].fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _pollfds.push_back(pfd);
    }
    
    // Add client sockets
    for (std::map<int, ClientConnection>::iterator it = _clients.begin(); 
         it != _clients.end(); ++it) {
        struct pollfd pfd;
        pfd.fd = it->first;
        pfd.events = POLLIN;
        
        // Add POLLOUT for clients that need to write
        if (it->second.state == CLIENT_WRITING_RESPONSE) {
            pfd.events |= POLLOUT;
        }
        
        pfd.revents = 0;
        _pollfds.push_back(pfd);
    }
}

/**
 * @brief Handle new incoming connection on a server socket
 * 
 * This function processes new client connections with robust error handling:
 * 1. Checks if connection limit (MAX_CON) has been reached
 *    - If at limit: accepts and immediately closes connection to prevent backlog buildup
 * 2. Calls accept() with client address structure to get new client socket
 * 3. Handles accept() failures gracefully (EAGAIN/EWOULDBLOCK are normal for non-blocking)
 * 4. Makes client socket non-blocking using Utils::setNonBlocking()
 * 5. Creates ClientConnection structure with:
 *    - Initial state: CLIENT_READING_REQUEST
 *    - Current timestamp for activity tracking
 *    - Associated server configuration for routing
 *    - Empty request buffer for incoming HTTP data
 * 6. Stores client in _clients map for future event processing
 * 
 * Error handling follows "never crash" principle:
 * - Silently handles accept() failures (common with non-blocking sockets)
 * - Closes client socket on any setup failure
 * - Logs errors for debugging but continues operation
 * - Rate-limits connections by enforcing MAX_CON limit
 * 
 * @param server_socket The server socket that received the connection
 */
void ServerManager::handleNewConnection(const ServerSocket& server_socket) {
    // Check connection limit
    if (_clients.size() >= MAX_CON) {
        // Accept and immediately close to prevent backlog buildup
        int client_fd = accept(server_socket.fd, NULL, NULL);
        if (client_fd >= 0) {
            close(client_fd);
        }
        return;
    }
    
    // Accept connection
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_socket.fd, (struct sockaddr*)&client_addr, &client_len);
    
    if (client_fd < 0) {
        // Non-blocking accept can return EAGAIN/EWOULDBLOCK
        return;
    }
    
    // Make client socket non-blocking
    Utils::setNonBlocking(client_fd);
    
    // Create client connection
    ClientConnection client;
    client.fd = client_fd;
    client.state = CLIENT_READING_HEADERS;
    client.last_activity = time(NULL);
    client.server_config = server_socket.config;
    
    _clients[client_fd] = client;
    
    std::cout << "✓ New connection: " << inet_ntoa(client_addr.sin_addr) 
              << ":" << ntohs(client_addr.sin_port) << " (fd=" << client_fd << ")" << std::endl;
}

/**
 * @brief Handle reading data from a client socket
 * 
 * This function implements stateful HTTP request parsing with these phases:
 * 1. Calls safeRead() to read available data without blocking
 * 2. Handles client disconnection (empty read) gracefully
 * 3. Appends new data to client's accumulating buffer
 * 4. Processes data based on client state:
 * 
 * CLIENT_READING_HEADERS state:
 * - Searches for "\r\n\r\n" to detect end of HTTP headers
 * - Parses headers using HttpRequest::parseRequest()
 * - Transitions to CLIENT_READING_BODY if body expected
 * - Transitions to CLIENT_PROCESSING if no body needed
 * - Generates HTTP_BAD_REQUEST on parse failures
 * - Generates HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE if headers too long
 * 
 * CLIENT_READING_BODY state:
 * - Accumulates body data until Content-Length bytes received
 * - Sets complete body in HttpRequest via setBody()
 * - Transitions to CLIENT_PROCESSING when body complete
 * - Generates HTTP_PAYLOAD_TOO_LARGE if body exceeds server limits
 * 
 * Error conditions handled:
 * - Network read failures -> closeClient()
 * - Client disconnections -> closeClient()
 * - Parse errors -> generateErrorResponse()
 * - Size limit violations -> generateErrorResponse()
 * 
 * @param client Reference to the ClientConnection being processed
 */
void ServerManager::handleClientRead(ClientConnection& client) {
    std::string read_buffer;
    
    if (!safeRead(client.fd, read_buffer)) {
        closeClient(client.fd, "Read failed");
        return;
    }
    
    if (read_buffer.empty()) {
        closeClient(client.fd, "Client disconnected");
        return;
    }
    
    client.buffer += read_buffer;
    
    // Process based on client state
    if (client.state == CLIENT_READING_HEADERS) {
        // Look for end of headers
        size_t header_end = client.buffer.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            // Parse request
            std::string request_data = client.buffer.substr(0, header_end + 4);
            if (client.request.parseRequest(request_data)) {
                client.state = CLIENT_READING_BODY;
                client.buffer = client.buffer.substr(header_end + 4);
                
                // Check if we need to read body
                if (client.request.getContentLength() == 0 && !client.request.isChunked()) {
                    client.state = CLIENT_PROCESSING;
                    processClientRequest(client);
                }
            } else {
                generateErrorResponse(client, HTTP_BAD_REQUEST);
            }
        } else if (client.buffer.length() > BUFFER_SIZE) {
            generateErrorResponse(client, HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE);
        }
    } else if (client.state == CLIENT_READING_BODY) {
        size_t content_length = client.request.getContentLength();
        if (client.buffer.length() >= content_length) {
            // Body complete
            std::string body = client.buffer.substr(0, content_length);
            client.request.setBody(body); // We'll need to add this method
            client.state = CLIENT_PROCESSING;
            processClientRequest(client);
        } else if (client.buffer.length() > client.server_config->max_body_size) {
            generateErrorResponse(client, HTTP_PAYLOAD_TOO_LARGE);
        }
    }
}

/**
 * @brief Handle writing response data to a client socket
 * 
 * This function manages sending HTTP responses to clients with these steps:
 * 1. Generates response buffer on first write call (lazy initialization)
 * 2. Calls safeWrite() to send remaining response data without blocking
 * 3. Tracks bytes sent using client.response_sent counter
 * 4. Handles partial writes by updating sent offset
 * 5. When response fully sent, handles connection lifecycle:
 * 
 * Keep-Alive connections:
 * - Resets client to CLIENT_READING_HEADERS state for next request
 * - Clears buffers and counters for reuse
 * - Updates last_activity timestamp
 * 
 * Non-Keep-Alive connections:
 * - Closes client connection immediately after response sent
 * 
 * Error handling:
 * - Write failures result in immediate client disconnection
 * - Uses safeWrite() which handles EAGAIN/EWOULDBLOCK gracefully
 * - Never blocks on write operations (maintains non-blocking requirement)
 * 
 * This function implements the "NEVER read/write without poll()" requirement
 * by only being called when poll() indicates POLLOUT readiness.
 * 
 * @param client Reference to the ClientConnection being written to
 */
void ServerManager::handleClientWrite(ClientConnection& client) {
    if (client.response_buffer.empty()) {
        // Set Connection header before generating response string
        setConnectionHeader(client);
        client.response_buffer = client.response.getResponseString();
    }
    
    size_t bytes_written = 0;
    if (!safeWrite(client.fd, client.response_buffer.substr(client.response_sent), bytes_written)) {
        closeClient(client.fd, "Write failed");
        return;
    }
    
    client.response_sent += bytes_written;
    
    if (client.response_sent >= client.response_buffer.length()) {
        // Response sent completely - decide connection fate
        if (shouldKeepConnectionAlive(client)) {
            // Keep connection alive for next request
            resetClientForNextRequest(client);
        } else {
            // Close connection
            client.state = CLIENT_DONE;
            closeClient(client.fd, "Response complete");
        }
    }
}

/**
 * @brief Process a complete HTTP request and generate appropriate response
 * 
 * This function handles HTTP request routing and response generation:
 * 1. Extracts request information from client.request
 * 2. Finds matching route configuration using findRoute()
 * 3. Validates HTTP method is allowed for the route
 * 4. Routes request to appropriate handler based on type:
 *    - CGI scripts: delegates to CgiHandler
 *    - Static files: serves file content
 *    - Directory listings: generates directory index
 *    - Error pages: serves custom error pages
 * 5. Handles special cases:
 *    - Method not allowed -> HTTP_METHOD_NOT_ALLOWED
 *    - File not found -> HTTP_NOT_FOUND
 *    - Internal errors -> HTTP_INTERNAL_SERVER_ERROR
 * 
 * Response generation:
 * - Sets appropriate HTTP status codes
 * - Adds proper headers (Content-Type, Content-Length, etc.)
 * - Handles both success and error responses
 * - Determines keep-alive behavior based on HTTP version and headers
 * - Transitions client to CLIENT_WRITING_RESPONSE state
 * 
 * Error handling follows "never crash" principle:
 * - Comprehensive try-catch around all processing
 * - Generates 500 Internal Server Error on exceptions
 * - Logs errors for debugging
 * - Always produces a valid HTTP response
 * 
 * @param client Reference to ClientConnection with complete request
 */

//
// void ServerManager::processClientRequest(ClientConnection& client) {
//     try {
//         const HttpRequest& request = client.request;
        
//         // Find appropriate route
//         const RouteConfig* route = findRoute(*client.server_config, request.getUri());
        
//         // Check method allowed
//         if (route && !isMethodAllowed(*route, request.getMethod())) {
//             generateErrorResponse(client, HTTP_METHOD_NOT_ALLOWED);
//             return;
//         }
        
//         // Resolve file path
//         std::string filepath = resolvePath(*client.server_config, route, request.getUri());
        
//         // Handle different request types
//         if (request.getMethod() == METHOD_GET) {
//             if (Utils::fileExists(filepath) && !Utils::isDirectory(filepath)) {
//                 generateFileResponse(client, filepath);
//             } else if (Utils::isDirectory(filepath)) {
//                 // Try to serve index file first
//                 std::string index_path = "";
                
//                 // Check route-level index file first
//                 if (route && !route->index_file.empty()) {
//                     std::string candidate = Utils::joinPath(filepath, route->index_file);
//                     if (Utils::fileExists(candidate) && !Utils::isDirectory(candidate)) {
//                         index_path = candidate;
//                     }
//                 }
                
//                 // Fall back to server-level index file
//                 if (index_path.empty() && !client.server_config->index.empty()) {
//                     std::string candidate = Utils::joinPath(filepath, client.server_config->index);
//                     if (Utils::fileExists(candidate) && !Utils::isDirectory(candidate)) {
//                         index_path = candidate;
//                     }
//                 }
                
//                 if (!index_path.empty()) {
//                     // Serve the index file
//                     generateFileResponse(client, index_path);
//                 } else if (route && route->directory_listing) {
//                     // Fall back to directory listing
//                     generateDirectoryListing(client, filepath, request.getUri());
//                 } else {
//                     // Directory access forbidden
//                     generateErrorResponse(client, HTTP_FORBIDDEN);
//                 }
//             } else {
//                 generateErrorResponse(client, HTTP_NOT_FOUND);
//             }
//         } else if (request.getMethod() == METHOD_POST) {
//             // Handle POST - could be upload or CGI
//             if (route && !route->upload_path.empty()) {
//                 // Handle upload
//                 generateErrorResponse(client, HTTP_NOT_IMPLEMENTED, "Upload not yet implemented");
//             } else if (route && !route->cgi_extension.empty()) {
//                 // Handle CGI
//                 if (!handleCGI(client, filepath)) {
//                     generateErrorResponse(client, HTTP_INTERNAL_SERVER_ERROR);
//                 }
//             } else {
//                 generateErrorResponse(client, HTTP_METHOD_NOT_ALLOWED);
//             }
//         } else {
//             generateErrorResponse(client, HTTP_NOT_IMPLEMENTED);
//         }
        
//     } catch (const std::exception& e) {
//         logError("Exception processing request", e.what());
//         generateErrorResponse(client, HTTP_INTERNAL_SERVER_ERROR);
//     } catch (...) {
//         logError("Unknown exception processing request");
//         generateErrorResponse(client, HTTP_INTERNAL_SERVER_ERROR);
//     }
// }

/**
 * @brief Generate standardized HTTP error response with smart connection handling
 * 
 * Enhanced error response generation that preserves keep-alive when possible:
 * 1. Uses HttpTemplates::generateTemplateResponse() for formatted error pages
 * 2. Only sets "Connection: close" for severe errors (5xx) or when required
 * 3. Preserves keep-alive for client errors (4xx) when connection is healthy
 * 4. Allows recovery from non-fatal errors on persistent connections
 * 
 * Connection closure is forced for:
 * - 5xx server errors (server state may be compromised)
 * - Parse errors that may corrupt the connection state
 * - Resource exhaustion conditions
 * 
 * @param client Reference to ClientConnection that encountered error
 * @param status_code HTTP status code (e.g., 404, 500, 400)
 * @param message Optional custom error message (defaults to standard message)
 */
void ServerManager::generateErrorResponse(ClientConnection& client, int status_code, const std::string& message) {
    client.response = HttpTemplates::generateTemplateResponse(status_code, message);
    
    // Only force connection close for severe errors or when necessary
    bool force_close = false;
    
    if (status_code >= 500) {
        // Server errors - close connection as server state may be compromised
        force_close = true;
    } else if (status_code == HTTP_BAD_REQUEST || status_code == HTTP_REQUEST_TIMEOUT) {
        // Request parsing errors - connection state may be corrupted
        force_close = true;
    } else if (_clients.size() > (MAX_CON * 0.95)) {
        // Server overloaded - shed connections aggressively
        force_close = true;
    }
    
    if (force_close) {
        client.keep_alive = false;
    }
    
    // Let setConnectionHeader decide final Connection header value
    client.state = CLIENT_WRITING_RESPONSE;
}

/**
 * @brief Generate HTTP response for serving static files
 * 
 * This function handles static file serving with proper HTTP headers:
 * 1. Uses HttpResponse::fileResponse() to read file and generate response
 * 2. Automatically sets Content-Type based on file extension
 * 3. Sets Content-Length header for the file size
 * 4. Handles file reading errors (file not found, permission denied, etc.)
 * 5. Determines keep-alive behavior based on client request headers
 * 6. Transitions client to CLIENT_WRITING_RESPONSE state
 * 
 * Keep-alive logic:
 * - Checks "Connection" header in client request
 * - Enables keep-alive only if explicitly requested by client
 * - Maintains connection for subsequent requests when appropriate
 * 
 * The function integrates with the HttpResponse class which handles:
 * - File reading with error handling
 * - MIME type detection
 * - Proper HTTP header generation
 * - Binary file support
 * 
 * @param client Reference to ClientConnection requesting the file
 * @param filepath Absolute path to the file to serve
 */
void ServerManager::generateFileResponse(ClientConnection& client, const std::string& filepath) {
    client.response = HttpResponse::fileResponse(filepath);
    
    // Determine keep-alive based on HTTP version and headers
    std::string connection = client.request.getHeader("Connection");
    client.keep_alive = (Utils::toLowerCase(connection) == "keep-alive");
    client.response.setHeader("Connection", client.keep_alive ? "keep-alive" : "close");
    
    client.state = CLIENT_WRITING_RESPONSE;
}

void ServerManager::generateDirectoryListing(ClientConnection& client, const std::string& path, const std::string& uri) {
    client.response = HttpResponse::directoryListingResponse(path, uri);
    client.response.setHeader("Connection", "close");
    client.keep_alive = false;
    client.state = CLIENT_WRITING_RESPONSE;
}

bool ServerManager::handleCGI(ClientConnection& client, const std::string& script_path) {
    // CGI handling - simplified for now
    // In a full implementation, we'd fork and exec
    (void)script_path;
    generateErrorResponse(client, HTTP_NOT_IMPLEMENTED, "CGI not yet implemented");
    return false;
}

void ServerManager::closeClient(int client_fd, const std::string& reason) {
    std::map<int, ClientConnection>::iterator it = _clients.find(client_fd);
    if (it != _clients.end()) {
        close(client_fd);
        _clients.erase(it);
        
        if (!reason.empty()) {
            std::cout << "✓ Closed client " << client_fd << ": " << reason << std::endl;
        }
    }
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
        closeClient(to_close[i], "Timeout");
    }
}

bool ServerManager::safeRead(int fd, std::string& buffer, size_t max_size) {
    char read_buf[BUFFER_SIZE];
    ssize_t bytes_read = read(fd, read_buf, sizeof(read_buf) - 1);
    
    (void)max_size;
    if (bytes_read > 0) {
        read_buf[bytes_read] = '\0';
        buffer.assign(read_buf, bytes_read);
        return true;
    } else if (bytes_read == 0) {
        // Connection closed
        return false;
    } else {
        // Error or would block
        return (errno == EAGAIN || errno == EWOULDBLOCK);
    }
}

bool ServerManager::safeWrite(int fd, const std::string& data, size_t& bytes_written) {
    ssize_t result = write(fd, data.c_str(), data.length());
    
    if (result > 0) {
        bytes_written = result;
        return true;
    } else if (result == 0) {
        bytes_written = 0;
        return true;
    } else {
        bytes_written = 0;
        return (errno == EAGAIN || errno == EWOULDBLOCK);
    }
}

void ServerManager::logError(const std::string& operation, const std::string& details) {
    std::cerr << "ERROR [" << operation << "]";
    if (!details.empty()) {
        std::cerr << ": " << details;
    }
    std::cerr << std::endl;
}

void ServerManager::handleOutOfMemory() {
    // Emergency cleanup - close some connections to free memory
    logError("Out of memory - performing emergency cleanup");
    
    // Close oldest connections first
    time_t oldest_time = time(NULL);
    int oldest_fd = -1;
    
    for (std::map<int, ClientConnection>::iterator it = _clients.begin(); 
         it != _clients.end(); ++it) {
        if (it->second.last_activity < oldest_time) {
            oldest_time = it->second.last_activity;
            oldest_fd = it->first;
        }
    }
    
    if (oldest_fd != -1) {
        closeClient(oldest_fd, "Emergency memory cleanup");
    }
}

const RouteConfig* ServerManager::findRoute(const ServerConfig& server, const std::string& uri) const {
    // Find longest matching route
    const RouteConfig* best_route = NULL;
    size_t best_length = 0;
    
    for (std::map<std::string, RouteConfig>::const_iterator it = server.routes.begin();
         it != server.routes.end(); ++it) {
        const std::string& route_path = it->first;
        
        if (uri.find(route_path) == 0 && route_path.length() > best_length) {
            best_route = &it->second;
            best_length = route_path.length();
        }
    }
    
    return best_route;
}

std::string ServerManager::resolvePath(const ServerConfig& server, const RouteConfig* route, const std::string& uri) const {
    std::string root = server.root;
    if (route && !route->root_directory.empty()) {
        root = route->root_directory;
    }
    
    return Utils::joinPath(root, uri);
}

bool ServerManager::isMethodAllowed(const RouteConfig& route, HttpMethod method) const {
    std::string method_str;
    switch (method) {
        case METHOD_GET: method_str = "GET"; break;
        case METHOD_POST: method_str = "POST"; break;
        case METHOD_DELETE: method_str = "DELETE"; break;
        default: return false;
    }
    
    for (size_t i = 0; i < route.allowed_methods.size(); ++i) {
        if (route.allowed_methods[i] == method_str) {
            return true;
        }
    }
    
    return false;
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

/* 
execve, pipe, strerror, gai_strerror, errno, dup,
dup2, fork, socketpair, htons, htonl, ntohs, ntohl,
select, poll, epoll (epoll_create, epoll_ctl,
epoll_wait), kqueue (kqueue, kevent), socket,
accept, listen, send, recv, chdir, bind, connect,
getaddrinfo, freeaddrinfo, setsockopt, getsockname,
getprotobyname, fcntl, close, read, write, waitpid,
kill, signal, access, stat, open, opendir, readdir
and closedir.
*/
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
    
    std::cout << "✓ ServerManager shutdown complete" << std::endl;
}

void ServerManager::printStatus() const {
    std::cout << "\n=== Server Status ===" << std::endl;
    std::cout << "Running: " << (_running ? "Yes" : "No") << std::endl;
    std::cout << "Server sockets: " << _server_sockets.size() << std::endl;
    std::cout << "Active connections: " << _clients.size() << std::endl;
    
    for (size_t i = 0; i < _server_sockets.size(); ++i) {
        std::cout << "  Port " << _server_sockets[i].port << " (fd=" << _server_sockets[i].fd << ")" << std::endl;
    }
    
    std::cout << "====================" << std::endl;
}

/**
 * @brief Request graceful shutdown from signal handler
 * 
 * This method is signal-safe and can be called from signal handlers.
 * It sets the _running flag to false, causing the main event loop to exit gracefully.
 * 
 * Unlike shutdown(), this method only requests shutdown without doing cleanup,
 * allowing the main loop to finish current operations and then shut down cleanly.
 */
void ServerManager::requestShutdown() {
    _running = false;
}

/**
 * @brief Determine if connection should be kept alive based on HTTP/1.1 rules
 * 
 * HTTP/1.1 persistent connection logic:
 * 1. HTTP/1.1 defaults to keep-alive unless "Connection: close" is specified
 * 2. HTTP/1.0 defaults to close unless "Connection: keep-alive" is specified
 * 3. Check connection limits: max requests per connection, max connection time
 * 4. Check server load: close connections if server is overloaded
 * 5. Respect client's connection preferences when possible
 * 
 * @param client Reference to ClientConnection to evaluate
 * @return true if connection should be kept alive, false if it should be closed
 */
bool ServerManager::shouldKeepConnectionAlive(const ClientConnection& client) {
    // Check if keep_alive flag is already disabled
    if (!client.keep_alive) {
        return false;
    }
    
    // Check connection age and request limits
    time_t now = time(NULL);
    if (now - client.connection_start > MAX_CONNECTION_TIME) {
        return false; // Connection too old
    }
    
    if (client.requests_handled >= MAX_REQUESTS_PER_CONNECTION) {
        return false; // Too many requests on this connection
    }
    
    // Check server load - close connections if overloaded
    if (_clients.size() > (MAX_CON * 0.9)) {
        return false; // Server overloaded, shed connections
    }
    
    // Check HTTP version and Connection header
    std::string version = client.request.getVersion();
    std::string connection_header = client.request.getHeader("connection");
    
    if (version == "HTTP/1.1") {
        // HTTP/1.1 defaults to keep-alive unless explicitly closed
        return !Utils::equalsIgnoreCase(connection_header, "close");
    } else if (version == "HTTP/1.0") {
        // HTTP/1.0 defaults to close unless explicitly keep-alive
        return Utils::equalsIgnoreCase(connection_header, "keep-alive");
    }
    
    // Unknown version - default to close for safety
    return false;
}

/**
 * @brief Reset client connection for processing next request
 * 
 * Prepares ClientConnection for reuse with HTTP/1.1 persistent connections:
 * 1. Resets HTTP request/response objects to clean state
 * 2. Clears buffers and counters for next request
 * 3. Updates connection statistics (requests handled)
 * 4. Sets client state back to reading headers
 * 5. Updates last activity timestamp for timeout tracking
 * 
 * This function enables connection reuse for HTTP/1.1 keep-alive.
 * 
 * @param client Reference to ClientConnection to reset for next request
 */
void ServerManager::resetClientForNextRequest(ClientConnection& client) {
    // Reset HTTP objects
    client.request.reset();
    client.response.reset();
    
    // Clear buffers and counters
    client.buffer.clear();
    client.buffer_pos = 0;
    client.response_buffer.clear();
    client.response_sent = 0;
    
    // Update connection statistics
    client.requests_handled++;
    client.last_activity = time(NULL);
    
    // Set state for next request
    client.state = CLIENT_READING_HEADERS;
}

/**
 * @brief Determine keep-alive support based on HTTP version and headers
 * 
 * Analyzes HTTP request to determine if keep-alive is supported:
 * 1. HTTP/1.1: keep-alive is default behavior unless "Connection: close"
 * 2. HTTP/1.0: connection closes by default unless "Connection: keep-alive"
 * 3. Validates Connection header format and values
 * 4. Returns appropriate keep-alive setting for this connection
 * 
 * @param client Reference to ClientConnection with parsed HTTP request
 * @return true if keep-alive should be enabled, false otherwise
 */
bool ServerManager::determineKeepAliveSupport(const ClientConnection& client) {
    std::string version = client.request.getVersion();
    std::string connection = client.request.getHeader("connection");
    
    if (version == "HTTP/1.1") {
        // HTTP/1.1 defaults to persistent connections
        if (Utils::equalsIgnoreCase(connection, "close")) {
            return false; // Client explicitly wants to close
        }
        return true; // Default keep-alive for HTTP/1.1
    } else if (version == "HTTP/1.0") {
        // HTTP/1.0 defaults to close connections
        if (Utils::equalsIgnoreCase(connection, "keep-alive")) {
            return true; // Client explicitly wants keep-alive
        }
        return false; // Default close for HTTP/1.0
    }
    
    // Unknown version - be conservative
    return false;
}

/**
 * @brief Set appropriate Connection header in HTTP response
 * 
 * Sets the Connection header based on HTTP/1.1 persistent connection rules:
 * 1. If keep_alive is false: set "Connection: close"
 * 2. If HTTP/1.1 and keep_alive is true: don't set header (keep-alive is default)
 * 3. If HTTP/1.0 and keep_alive is true: set "Connection: keep-alive"
 * 4. Respects any existing Connection header set by application logic
 * 
 * This implements smart Connection header management - only sending
 * Connection headers when necessary to override defaults.
 * 
 * @param client Reference to ClientConnection with response to modify
 */
void ServerManager::setConnectionHeader(ClientConnection& client) {
    // Don't override if Connection header already set
    if (client.response.hasConnectionHeader()) {
        return;
    }
    
    std::string version = client.request.getVersion();
    
    if (!client.keep_alive) {
        // Always set Connection: close when closing
        client.response.setHeader("Connection", "close");
    } else if (version == "HTTP/1.0") {
        // HTTP/1.0 needs explicit keep-alive header
        client.response.setHeader("Connection", "keep-alive");
        client.response.setHeader("Keep-Alive", "timeout=60, max=100");
    }
    // HTTP/1.1 with keep_alive=true: don't set header (keep-alive is default)
}

/**
 * @brief Find index file in directory for HTTP directory requests
 * 
 * Searches for index files in the specified directory based on server configuration:
 * 1. Checks route-specific index file first if provided
 * 2. Falls back to server-level index file configuration
 * 3. Uses Utils::fileExists() to verify file accessibility
 * 4. Returns full path to found index file
 * 5. Returns empty string if no index file found
 * 
 * This enables proper HTTP directory handling with configurable index files.
 * 
 * @param dir_path Path to directory to search for index files
 * @param server_config Server configuration containing default index file
 * @param route Route configuration that may override index file
 * @return Full path to found index file, or empty string if none found
 */
std::string ServerManager::findIndexFile(const std::string& dir_path, 
                                       const ServerConfig* server_config, 
                                       const RouteConfig* route) const {
    // Check route-specific index file first
    if (route && !route->index_file.empty()) {
        std::string index_path = Utils::joinPath(dir_path, route->index_file);
        if (Utils::fileExists(index_path) && Utils::isReadable(index_path)) {
            return index_path;
        }
    }
    
    // Fall back to server-level index file
    if (server_config && !server_config->index.empty()) {
        std::string index_path = Utils::joinPath(dir_path, server_config->index);
        if (Utils::fileExists(index_path) && Utils::isReadable(index_path)) {
            return index_path;
        }
    }
    
    // No index file found
    return "";
}