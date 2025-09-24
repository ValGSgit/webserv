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
```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   Config    │───▶│   Server    │───▶│   Request   │
│   Parser    │    │   Manager   │    │   Handler   │
└─────────────┘    └─────────────┘    └─────────────┘
                           │
                    ┌─────────────┐
                    │ Connection  │
                    │  Manager    │
                    │ (poll/epoll)│
                    └─────────────┘
```

### Phase 2: Core Implementation

#### Step 3: Configuration Parser
- [ ] **Create configuration structures**
  ```cpp
  struct ServerConfig {
      std::vector<int> ports;
      std::map<std::string, RouteConfig> routes;
      size_t max_body_size;
      std::map<int, std::string> error_pages;
      std::string server_name;
  };
  
  struct RouteConfig {
      std::vector<std::string> allowed_methods;
      std::string root_directory;
      std::string index_file;
      bool directory_listing;
      std::string upload_path;
      std::string cgi_extension;
      std::string redirect_url;
  };
  ```
- [ ] **Implement parser logic**
  - Parse server blocks
  - Handle nested configurations
  - Validate configuration values
- [ ] **Create sample configuration files**
  - Basic server setup
  - Multiple virtual hosts
  - CGI configurations

#### Step 4: Socket Management & Basic Server
- [ ] **Socket initialization**
  - Create listening sockets for multiple ports
  - Set sockets to non-blocking mode using `fcntl()`
  - Handle socket binding and listening
- [ ] **Basic connection acceptance**
  - Accept new connections
  - Set client sockets to non-blocking
  - Store connection information

#### Step 5: I/O Multiplexing Implementation
- [ ] **Choose multiplexing method**
  - Linux: `epoll()` (recommended)
  - macOS/BSD: `kqueue()`
  - Fallback: `poll()` or `select()`
- [ ] **Implement event loop**
  ```cpp
  class EventLoop {
      void run();
      void handleRead(int fd);
      void handleWrite(int fd);
      void handleError(int fd);
      void addConnection(int fd);
      void removeConnection(int fd);
  };
  ```
- [ ] **Connection state management**
  - Track reading/writing states
  - Handle partial operations
  - Implement timeouts

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
├── config/
│   ├── default.conf
│   └── example.conf
├── src/
│   ├── config/
│   │   ├── ConfigParser.cpp
│   │   └── ConfigParser.hpp
│   ├── http/
│   │   ├── HttpRequest.cpp
│   │   ├── HttpRequest.hpp
│   │   ├── HttpResponse.cpp
│   │   └── HttpResponse.hpp
│   ├── server/
│   │   ├── Server.cpp
│   │   ├── Server.hpp
│   │   ├── Connection.cpp
│   │   └── Connection.hpp
│   ├── cgi/
│   │   ├── CgiHandler.cpp
│   │   └── CgiHandler.hpp
│   ├── utils/
│   │   ├── Utils.cpp
│   │   └── Utils.hpp
│   └── main.cpp
├── tests/
│   ├── unit_tests.cpp
│   ├── integration_tests.py
│   └── stress_test.sh
└── www/
    ├── index.html
    ├── error_pages/
    └── uploads/
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
