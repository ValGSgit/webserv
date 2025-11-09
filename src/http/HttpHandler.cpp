#include "../../includes/http/HttpHandler.hpp"
#include "../../includes/server/ServerManager.hpp"
#include <fstream>
#include <sys/stat.h>

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
                // RFC 7231: Reached root, no route found
                // Allow safe methods (GET, HEAD, OPTIONS) by default
                // OPTIONS should always be allowed (RFC 7231 Section 4.3.7)
                return (method == "GET" || method == "HEAD" || method == "OPTIONS");
            }
            
            // Remove last path segment and try again
            size_t last_slash = route_path.find_last_of('/');
            if (last_slash == 0) {
                route_path = "/";  // Parent is root
            } else if (last_slash != std::string::npos) {
                route_path = route_path.substr(0, last_slash);
            } else {
                // RFC 7231: No slash found (shouldn't happen with valid URIs)
                // Allow safe methods by default
                return (method == "GET" || method == "HEAD" || method == "OPTIONS");
            }
        }
    }
}

// Find the most specific matching route for a URI
const RouteConfig* HttpHandler::findMatchingRoute(const std::string& uri, const ServerConfig& config) {
    std::string route_path = uri;
    
    // Keep going up the directory tree until we find a matching route
    while (true) {
        std::map<std::string, RouteConfig>::const_iterator it = config.routes.find(route_path);
        if (it != config.routes.end()) {
            return &(it->second);  // Found matching route
        }
        
        // Route not found, try parent directory
        if (route_path == "/") {
            return NULL;  // No matching route found
        }
        
        // Remove last path segment and try again
        size_t last_slash = route_path.find_last_of('/');
        if (last_slash == 0) {
            route_path = "/";  // Parent is root
        } else if (last_slash != std::string::npos) {
            route_path = route_path.substr(0, last_slash);
        } else {
            return NULL;  // No slash found
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

    // ========== DEBUG: Log incoming request ==========
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              INCOMING REQUEST (FD: " << client_fd << ", Port: " << server_port << ")              ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════════╣\n";
    std::cout << "  Method:  " << request.methodToString() << "\n";
    std::cout << "  URI:     " << request.getUri() << "\n";
    std::cout << "  Version: " << request.getVersion() << "\n";
    std::cout << "  Headers:\n";
    const std::map<std::string, std::string>& headers = request.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        std::cout << "    " << it->first << ": " << it->second << "\n";
    }
    if (!request.getBody().empty()) {
        std::cout << "  Body length: " << request.getBody().length() << " bytes\n";
        if (request.getBody().length() < 200) {
            std::cout << "  Body: " << request.getBody() << "\n";
        }
    }
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    
    std::string uri = request.getUri();
    
    // RFC 7230 Section 3.3.2 - Payload size validation
    // check max body size
    if (request.getContentLength() > config->max_body_size)
        response = HttpResponse::errorResponse(HTTP_PAYLOAD_TOO_LARGE);
    // Check for client request errors
    else if (request.getStatus()) {
        response = HttpResponse::errorResponse(request.getStatus());
    }
    // Check for configured redirects in routes FIRST
    else {
        const RouteConfig* route = findMatchingRoute(uri, *config);
        if (route && !route->redirect_url.empty() && route->redirect_code > 0) {
            // This route has a redirect configured
            std::string redirect_location = route->redirect_url;
            
            // If redirect URL is relative and doesn't start with http/https, make it absolute
            if (redirect_location.find("http://") != 0 && redirect_location.find("https://") != 0) {
                // For relative URLs, you might want to construct full URL
                // For now, we'll just use the redirect_url as-is (works for relative redirects)
            }
            
            response = HttpResponse::redirectResponse(redirect_location, route->redirect_code);
        }
        // RFC 7231 Section 4 - Method Definitions
        // RFC 7231 Section 6.5.5 - 405 Method Not Allowed
        // Check if method is allowed
        else if (!methodAllowed(uri, request.methodToString(), *config)) {
            response = HttpResponse::errorResponse(HTTP_METHOD_NOT_ALLOWED);
        }
#ifdef BONUS
        // Session API endpoints (MUST be checked BEFORE generic POST handler!)
        else if (uri == "/api/session/login" && request.getMethod() == METHOD_POST) {
            SessionManager* sm = _server_manager->getSessionManager();

            // Parse username from request body
            std::string username = "demo_user"; // default
            std::string body = request.getBody();
            
            // Simple JSON parsing to extract username
            size_t username_pos = body.find("\"username\"");
            if (username_pos != std::string::npos) {
                size_t colon_pos = body.find(":", username_pos);
                if (colon_pos != std::string::npos) {
                    size_t quote_start = body.find("\"", colon_pos);
                    size_t quote_end = body.find("\"", quote_start + 1);
                    if (quote_start != std::string::npos && quote_end != std::string::npos) {
                        username = body.substr(quote_start + 1, quote_end - quote_start - 1);
                        // Sanitize username (remove special chars, limit length)
                        if (username.length() > 30) username = Utils::sanitizeFilename(username);
                    }
                }
            }

            // CHECK: Does this username already have an active session? (OPTION 3)
            std::string existing_session_id = sm->getSessionByUsername(username);
            
            if (!existing_session_id.empty()) {
                // User is already logged in - REJECT the new login
                response.setStatus(409); // HTTP 409 Conflict
                response.setContentType("application/json");
                response.setBody("{\"success\": false, \"message\": \"User '" + username + "' is already logged in. Please logout first.\"}");
            } else {
                // No active session - proceed with login
                std::string session_id = sm->createSession();

                // Store data in the session
                SessionData* session = sm->getSession(session_id);
                if (session) {
                    session->data["username"] = username;
                    session->data["authenticated"] = "true";
                    session->data["role"] = "user";
                    
                    // Register username with this session
                    sm->registerUsername(session_id, username);
                }

                response.setStatus(HTTP_OK);
                response.setContentType("application/json");
                response.setCookie("SESSIONID", session_id, 3600);  // 1 hour expiry
                response.setBody("{\"success\": true, \"message\": \"Login successful\", \"session_id\": \"" + session_id + "\", \"username\": \"" + username + "\"}");
            }
        }
        else if (uri == "/api/session/profile" && request.getMethod() == METHOD_GET) {
            // Get session data (protected endpoint)
            std::string session_id = request.getCookie("SESSIONID");

            if (session_id.empty()) {
                response.setStatus(HTTP_UNAUTHORIZED);
                response.setContentType("application/json");
                response.setBody("{\"success\": false, \"message\": \"Not authenticated\"}");
            } else {
                SessionManager* sm = _server_manager->getSessionManager();
                SessionData* session = sm->getSession(session_id);

                if (!session || session->data["authenticated"] != "true") {
                    response.setStatus(HTTP_UNAUTHORIZED);
                    response.setContentType("application/json");
                    response.setBody("{\"success\": false, \"message\": \"Invalid or expired session\"}");
                } else {
                    // Return user profile
                    std::string username = session->data["username"];
                    std::string role = session->data["role"];
                    response.setStatus(HTTP_OK);
                    response.setContentType("application/json");
                    response.setBody("{\"success\": true, \"username\": \"" + username + "\", \"role\": \"" + role + "\"}");
                }
            }
        }
        else if (uri == "/api/session/logout" && request.getMethod() == METHOD_POST) {
            // Destroy session
            std::string session_id = request.getCookie("SESSIONID");

            if (!session_id.empty()) {
                SessionManager* sm = _server_manager->getSessionManager();
                sm->destroySession(session_id);
            }

            response.setStatus(HTTP_OK);
            response.setContentType("application/json");
            response.clearCookie("SESSIONID");
            response.setBody("{\"success\": true, \"message\": \"Logged out successfully\"}");
        }
        else if (uri == "/api/session/info" && request.getMethod() == METHOD_GET) {
            // Get session manager stats
            SessionManager* sm = _server_manager->getSessionManager();
            size_t active_sessions = sm->getActiveSessionCount();

            response.setStatus(HTTP_OK);
            response.setContentType("application/json");
            response.setBody("{\"active_sessions\": " + Utils::toString(active_sessions) + "}");
        }
#endif
        // RFC 7231 Section 4.3.7 - OPTIONS method
        // Returns the communication options available for the target resource
        else if (request.getMethod() == METHOD_OPTIONS) {
            std::cout << "[OPTIONS] Handling OPTIONS request for URI: " << uri << std::endl;
            response = handleOptions(uri, *config);
            std::cout << "[OPTIONS] Response created with status: " << response.getStatus() << std::endl;
        }
        // API test endpoint
        else if (uri == "/api/test") {
            response = handleJsonApi(request);
        }
        // CGI handling
        else if (uri.find("/cgi-bin/") == 0) {
            CgiHandler cgi;
            std::string script_path = config->root + uri;
            response = cgi.executeCgi(request, script_path);
        }
        // Root index
        else if (uri == "/") {
            std::string index_path = config->root + "/" + config->index;
            response = HttpResponse::fileResponse(index_path);
        }
        // RFC 7231 Section 4.3.3 - POST method for resource creation/modification
        // Generic POST handler for uploads (after all API endpoints!)
        else if (request.getMethod() == METHOD_POST) {
            response = handleUpload(request, *config, client_fd);
        }
        // RFC 7231 Section 4.3.4 - PUT method for resource creation/replacement
        // PUT creates or completely replaces the resource at the target URI
        else if (request.getMethod() == METHOD_PUT) {
            std::cout << "[PUT] Handling PUT request for URI: " << uri << std::endl;
            response = handlePut(request, *config, client_fd);
            std::cout << "[PUT] Response created with status: " << response.getStatus() << std::endl;
        }
        // RFC 7231 Section 4.3.5 - DELETE method for resource removal
        // Generic DELETE handler
        else if (request.getMethod() == METHOD_DELETE) {
            response = handleDelete(request, *config, client_fd);
        }
        // RFC 7231 Section 4.3.2 - HEAD method
        // Identical to GET but must not return a message body
        else if (request.getMethod() == METHOD_HEAD) {
            std::cout << "[HEAD] Handling HEAD request for URI: " << uri << std::endl;
            // Process as GET, then remove body
            std::string filepath;
            if (uri == "/") {
                filepath = config->root + "/" + config->index;
            } else if (uri.find("/static/") == 0) {
                filepath = config->root + uri;
            } else {
                filepath = config->root + uri;
            }
            
            std::cout << "[HEAD] Filepath: " << filepath << std::endl;
            
            if (Utils::fileExists(filepath)) {
                if (Utils::isDirectory(filepath)) {
                    std::cout << "[HEAD] Directory detected, generating listing\n";
                    response = HttpResponse::directoryListingResponse(filepath, uri);
                } else {
                    std::cout << "[HEAD] File detected, generating file response\n";
                    response = HttpResponse::fileResponse(filepath);
                }
            } else {
                std::cout << "[HEAD] File not found: " << filepath << std::endl;
                response = HttpResponse::errorResponse(HTTP_NOT_FOUND);
            }
            
            std::cout << "[HEAD] Response status before removeBody(): " << response.getStatus() << std::endl;
            std::cout << "[HEAD] Body length before removeBody(): " << response.getBody().length() << " bytes\n";
            
            // Remove body but keep all headers (including Content-Length)
            response.removeBody();
            
            std::cout << "[HEAD] Body length after removeBody(): " << response.getBody().length() << " bytes\n";
        }
        // Static files
        else if (uri.find("/static/") == 0) {
            std::string filepath = config->root + uri;
            response = HttpResponse::fileResponse(filepath);
        } else {
            // RFC 7231 Section 4.3.1 - GET method for resource retrieval
            // Try to serve as file or directory
            std::string filepath = config->root + uri;
            if (Utils::fileExists(filepath)) {
                if (Utils::isDirectory(filepath)) {
                    response = HttpResponse::directoryListingResponse(filepath, uri);
                } else {
                    response = HttpResponse::fileResponse(filepath);
                }
            } else {
                // RFC 7231 Section 6.5.4 - 404 Not Found
                response = HttpResponse::errorResponse(HTTP_NOT_FOUND);
            }
        }
    }
    
    // Prepare response for sending
    _response_buffers[client_fd] = response.getResponseString();
    _response_offsets[client_fd] = 0;
    
    // ========== DEBUG: Log outgoing response ==========
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              OUTGOING RESPONSE (FD: " << client_fd << ")                     ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════════╣\n";
    std::cout << "  Status: " << response.getStatus() << "\n";
    std::cout << "  Body Length: " << response.getBody().length() << " bytes\n";
    std::cout << "  Response Preview (first 500 chars):\n";
    std::string resp_str = _response_buffers[client_fd];
    if (resp_str.length() > 500) {
        std::cout << resp_str.substr(0, 500) << "...\n";
    } else {
        std::cout << resp_str << "\n";
    }
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    // ========== END DEBUG ==========
    
    // RFC 7230 Section 6 - Connection Management
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

// RFC 7231 Section 4.3.5 - DELETE Method
// The DELETE method requests that the origin server remove the association
// between the target resource and its current functionality
HttpResponse HttpHandler::handleDelete(const HttpRequest& request, const ServerConfig& config, int client_fd)
{
    (void)config;
    (void)client_fd;
    std::string file_path = config.root + request.getUri();
    int fd = open(file_path.c_str(), O_WRONLY, 0644);
    if (fd == -1)
        // RFC 7231 Section 6.5.4 - 404 Not Found
        return HttpResponse::errorResponse(HTTP_NOT_FOUND, "File not found!"); // why is 204 not working???
    close(fd);
    std::remove(file_path.c_str());
    // RFC 7231 Section 6.3.1 - 200 OK or 202 Accepted or 204 No Content
    // or 202 HTTP_ACCEPTED?
    return HttpResponse::messageResponse(HTTP_OK, "File deleted!");
}

// RFC 7231 Section 4.3.4 - PUT Method
// The PUT method requests that the state of the target resource be created
// or replaced with the state defined by the representation enclosed in the request
HttpResponse HttpHandler::handlePut(const HttpRequest& request, const ServerConfig& config, int client_fd) {
    (void)client_fd;
    
    std::string uri = request.getUri();
    std::string file_path = config.root + uri;
    
    std::cout << "[PUT] Target file path: " << file_path << std::endl;
    std::cout << "[PUT] Request body length: " << request.getBody().length() << " bytes\n";
    
    // RFC 7231: Check if resource already exists (determines 200 vs 201 response)
    bool resource_exists = Utils::fileExists(file_path);
    
    // Prevent writing to directories
    if (Utils::fileExists(file_path) && Utils::isDirectory(file_path)) {
        std::cout << "[PUT] ERROR: Cannot PUT to a directory\n";
        return HttpResponse::errorResponse(HTTP_CONFLICT, "Cannot replace a directory");
    }
    
    // Create parent directories if they don't exist
    size_t last_slash = file_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        std::string dir_path = file_path.substr(0, last_slash);
        if (!Utils::fileExists(dir_path)) {
            std::cout << "[PUT] Creating parent directory: " << dir_path << std::endl;
            // Create directory recursively (simplified - you may need mkdir -p logic)
            mkdir(dir_path.c_str(), 0755);
        }
    }
    
    // Write the body to the file (create or replace)
    std::ofstream outfile(file_path.c_str(), std::ios::binary | std::ios::trunc);
    if (!outfile.is_open()) {
        std::cout << "[PUT] ERROR: Failed to open file for writing\n";
        return HttpResponse::errorResponse(HTTP_INTERNAL_SERVER_ERROR, "Failed to write file");
    }
    
    outfile.write(request.getBody().c_str(), request.getBody().length());
    outfile.close();
    
    std::cout << "[PUT] File written successfully\n";
    
    // RFC 7231 Section 4.3.4: 
    // - 201 Created if a new resource was created
    // - 200 OK or 204 No Content if an existing resource was modified
    if (resource_exists) {
        std::cout << "[PUT] Resource replaced (200 OK)\n";
        return HttpResponse::messageResponse(HTTP_OK, "Resource updated successfully");
    } else {
        std::cout << "[PUT] Resource created (201 Created)\n";
        HttpResponse response;
        response.setStatus(HTTP_CREATED); // 201
        response.setContentType("text/plain");
        response.setBody("Resource created successfully");
        // RFC 7231: Should include Location header with the URI of the created resource
        response.setHeader("Location", uri);
        return response;
    }
}

// RFC 7231 Section 4.3.7 - OPTIONS Method
// The OPTIONS method requests information about the communication options
// available for the target resource
HttpResponse HttpHandler::handleOptions(const std::string& uri, const ServerConfig& config) {
    std::cout << "[OPTIONS] handleOptions() called for URI: " << uri << std::endl;
    
    // Find the matching route to determine allowed methods
    const RouteConfig* route = findMatchingRoute(uri, config);
    
    std::vector<std::string> allowed_methods;
    
    if (route && !route->allowed_methods.empty()) {
        std::cout << "[OPTIONS] Route found with configured allowed methods\n";
        // Use configured allowed methods for this route
        allowed_methods = route->allowed_methods;
    } else {
        std::cout << "[OPTIONS] No route or no configured methods, using defaults\n";
        // Default allowed methods - GET and HEAD are always safe
        allowed_methods.push_back("GET");
        allowed_methods.push_back("HEAD");
        allowed_methods.push_back("OPTIONS");
        
        // Check if resource exists to determine other methods
        std::string filepath = config.root + uri;
        if (Utils::fileExists(filepath)) {
            // For existing files, also allow POST (upload/overwrite) and DELETE
            if (!Utils::isDirectory(filepath)) {
                allowed_methods.push_back("DELETE");
            }
        }
        // POST is typically allowed for upload endpoints
        if (uri.find("/upload") != std::string::npos || uri == "/") {
            allowed_methods.push_back("POST");
        }
    }
    
    // Always include OPTIONS itself
    bool has_options = false;
    for (size_t i = 0; i < allowed_methods.size(); ++i) {
        if (allowed_methods[i] == "OPTIONS") {
            has_options = true;
            break;
        }
    }
    if (!has_options) {
        allowed_methods.push_back("OPTIONS");
    }
    
    std::cout << "[OPTIONS] Allowed methods for " << uri << ": ";
    for (size_t i = 0; i < allowed_methods.size(); ++i) {
        std::cout << allowed_methods[i];
        if (i < allowed_methods.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    return HttpResponse::optionsResponse(allowed_methods);
}


// RFC 7231 Section 4.3.3 - POST Method
// The POST method requests that the target resource process the representation
// enclosed in the request according to the resource's own specific semantics
HttpResponse HttpHandler::handleUpload(const HttpRequest& request, const ServerConfig& config, int client_fd) {
    std::cout << "Handling file upload" << std::endl;

    const std::string& body = request.getBody();
    // default upload path
    std::string upload_dir = config.root + "/uploads/";
    // Find upload path from config
    try
    {
        const RouteConfig &route = config.routes.at("/upload");
        if (route.upload_path != "")
            upload_dir = route.upload_path + "/";
    }
    catch(const std::exception& e){}
    // use temp string to find things
    std::string temp = _raw_buffer;
    // Simple upload handling - save to uploads directory
    std::size_t needle = temp.find("filename=");
    std::string filename;
    if (needle != std::string::npos)
        filename = temp.substr(needle + 10, temp.find_first_of("\"", needle + 10) - needle - 10);
    else
        filename = "uploaded_file_" + Utils::toString(static_cast<int>(time(NULL)));
    
    // SECURITY FIX: Sanitize filename to prevent path traversal
    filename = Utils::sanitizeFilename(filename);
    
    // SECURITY FIX: Check if filename has safe path (no .., /, etc.)
    if (!Utils::isSafePath(filename)) {
        return HttpResponse::errorResponse(HTTP_FORBIDDEN, "Invalid filename");
    }
    
    // SECURITY FIX: Validate file extension
    if (!Utils::isAllowedUploadExtension(filename)) {
        return HttpResponse::errorResponse(HTTP_FORBIDDEN, "File type not allowed");
    }
    
    std::string filepath = upload_dir + filename;
    // if file already exist, add suffix
    if (Utils::fileExists(filepath))
        filepath = filepath + "_copy_" + Utils::toString(static_cast<int>(time(NULL)));
    // find boundary
    needle = temp.find("boundary=");
    std::string boundary;
    std::string boundary_end;
    // if boundary is found
    if (needle != std::string::npos)
    {
        boundary = "\r\n--" + temp.substr(needle + 9, temp.find_first_of("\r\n", needle + 9) - needle - 9);
        boundary_end = boundary + "--";
        needle = temp.find(boundary);
        _raw_bytes_read -= needle;
        _raw_buffer = &_raw_buffer[needle];
        temp =_raw_buffer;
    }
    needle = temp.find("\r\n\r\n");
    // needle is the starting index of the file
    needle += 4;
    _raw_bytes_read -= needle;
    // save also file info
    if (boundary != "")
        _file_info = temp.substr(0, needle);
    size_t total_bytes_read = 0;
    // if the end boundary is already in the raw_buffer
    if (boundary != "" && check_boundary(&_raw_buffer[needle], boundary_end.c_str(), _raw_bytes_read))
            _raw_bytes_read = check_boundary(&_raw_buffer[needle], boundary_end.c_str(), _raw_bytes_read);
    // chrome send the body later, dont write the raw_buffer
    if (body == "")
        _raw_bytes_read = 0;
    if (Utils::writeFile(filepath, &_raw_buffer[needle], _raw_bytes_read)) {
        total_bytes_read += _raw_bytes_read;
        char buffer[BUFFER_SIZE];
        int bytes_read = 0;
        // if there are things left in fd, continue to read the body and write to file
        while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE)) > 0) {
            //get rid of the end boundary
            if (boundary != "" && check_boundary(buffer, boundary_end.c_str(), bytes_read))
                bytes_read = check_boundary(buffer, boundary_end.c_str(), bytes_read);
            if (!Utils::writeFile(filepath, buffer, bytes_read))
                return HttpResponse::errorResponse(HTTP_INTERNAL_SERVER_ERROR, "Failed to save file");
            total_bytes_read += bytes_read;
            
            // open it to handle individual file size limit
/*             if (total_bytes_read > MAX_FILE_SIZE) // 10MB default
            {
                // delete the file!
                std::remove(filepath.c_str());
                return HttpResponse::errorResponse(HTTP_PAYLOAD_TOO_LARGE, "File is too big");
            } */
        }
        if (total_bytes_read == 0)
        {
            std::remove(filepath.c_str());
            return HttpResponse::errorResponse(HTTP_BAD_REQUEST, "No file data");
        }

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
