<div align="center">

# 🌐 WebServ

### *A High-Performance HTTP/1.1 Server in C++98*

[![C++98](https://img.shields.io/badge/C++-98-00599C?style=for-the-badge&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/98)
[![Linux](https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)](https://www.linux.org/)
[![epoll](https://img.shields.io/badge/epoll-I%2FO-green?style=for-the-badge)](https://man7.org/linux/man-pages/man7/epoll.7.html)
[![License](https://img.shields.io/badge/License-MIT-blue?style=for-the-badge)](LICENSE)

*Built from scratch with non-blocking I/O, epoll multiplexing, and full CGI support*

[Features](#-features) • [Quick Start](#-quick-start) • [Configuration](#️-configuration) • [Testing](#-testing) • [Architecture](#-architecture)

---

<img src="https://raw.githubusercontent.com/tandpfun/skill-icons/main/icons/CPP.svg" width="48"> <img src="https://raw.githubusercontent.com/tandpfun/skill-icons/main/icons/Linux-Dark.svg" width="48"> <img src="https://raw.githubusercontent.com/tandpfun/skill-icons/main/icons/Python-Dark.svg" width="48"> <img src="https://raw.githubusercontent.com/tandpfun/skill-icons/main/icons/PHP-Dark.svg" width="48">

</div>

---

## 📖 Table of Contents

- [✨ Features](#-features)
- [🚀 Quick Start](#-quick-start)
- [⚙️ Configuration](#️-configuration)
- [🧪 Testing](#-testing)
- [🏗️ Architecture](#️-architecture)
- [📚 API Reference](#-api-reference)
- [🎯 Project Goals](#-project-goals)
- [📸 Screenshots](#-screenshots)
- [🤝 Contributing](#-contributing)

---

## ✨ Features

<div align="center">

| 🔥 Core Features | 🎯 Advanced Features | 🛡️ Reliability |
|-----------------|---------------------|----------------|
| **HTTP/1.1 Protocol** | **CGI Execution** | **Non-blocking I/O** |
| GET, POST, DELETE | Python, PHP, Perl, Ruby, Shell | Event-driven epoll |
| **Multiple Ports** | **File Upload** | **Connection Pooling** |
| Simultaneous servers | Multipart form data | Efficient resource usage |
| **Static Files** | **Redirections** | **Error Handling** |
| HTML, CSS, JS, images | 301, 302 redirects | Custom error pages |
| **Directory Listing** | **Virtual Hosts** | **Graceful Shutdown** |
| Autoindex on/off | Server name routing | SIGINT/SIGTERM handling |

</div>

### 🌟 Highlights

- ⚡ **Blazing Fast**: Built with Linux `epoll` for maximum I/O efficiency
- 🔧 **NGINX-inspired**: Configuration syntax similar to industry standards
- 🎨 **Modern Web**: Serves HTML5, CSS3, JavaScript with proper MIME types
- 🐍 **CGI Ready**: Execute Python, PHP, Perl, Ruby, and Shell scripts
- 📁 **File Management**: Upload, download, and delete files via HTTP
- 🛠️ **Developer Friendly**: Extensive test suite with 60+ automated tests
- 🔒 **Production Ready**: Handles concurrent connections with timeout management
- 📊 **Battle Tested**: Stress-tested with siege for high-load scenarios

---

### Showcase
![flow](https://github.com/user-attachments/assets/e049376a-4fbe-44e7-afd7-938dfab32fcb)


## 🚀 Quick Start

### Prerequisites

```bash
# Required
✓ Linux OS (uses epoll)
✓ C++ compiler with C++98 support (g++/clang++)
✓ GNU Make

# Optional (for testing)
✓ Python 3.x
✓ Siege (for stress testing)
✓ Valgrind (for memory leak detection)
```

### Installation

```bash
# Clone the repository
git clone https://github.com/ValGSgit/webserv.git
cd webserv

# Build the server
make

# Run with default configuration
./webserv config/default.conf
```

<div align="center">

**🎉 Server is now running on http://localhost:8080 🎉**

</div>

### Your First Requests

```bash
# Test static file serving
curl http://localhost:8080/

# Test CGI script
curl http://localhost:8080/cgi-bin/test.py

# Test with query parameters
curl "http://localhost:8080/cgi-bin/test.py?name=WebServ&status=awesome"

# Upload a file
curl -X POST -F "file=@myfile.txt" http://localhost:8080/upload

# Delete a file
curl -X DELETE http://localhost:8080/uploads/myfile.txt

# Test directory listing
curl http://localhost:8080/browse/
```

### Using a Web Browser

Simply open your browser and navigate to:
- 🏠 **Home**: http://localhost:8080/
- 🎨 **Demo**: http://localhost:8080/demo.html
- 🐍 **CGI Demo**: http://localhost:8080/cgi-demo.html
- 📤 **Upload**: http://localhost:8080/upload.html
- 📂 **Browse Files**: http://localhost:8080/browse/

---

## ⚙️ Configuration

WebServ uses an **NGINX-style configuration syntax** that's both powerful and easy to understand.

### 📝 Basic Example

```nginx
server {
    listen 8080;                        # Port to listen on
    server_name localhost;               # Server name
    root ./www;                          # Document root
    index index.html;                    # Default index file
    client_max_body_size 10485760;      # Max upload size (10MB)
    
    # Custom error pages
    error_page 404 /error_pages/404.html;
    error_page 500 /error_pages/500.html;
    
    # Root location
    location / {
        allow_methods GET POST DELETE;   # Allowed HTTP methods
        autoindex on;                    # Enable directory listing
    }
    
    # File upload endpoint
    location /upload {
        allow_methods POST DELETE;
        upload_pass ./www/uploads;       # Upload directory
        client_max_body_size 52428800;  # 50MB for uploads
    }
    
    # CGI scripts
    location /cgi-bin {
        allow_methods GET POST;
        root ./www/cgi-bin;
        cgi_extension .py .php .pl .rb .sh;  # Supported extensions
    }
    
    # HTTP redirections
    location /redirect {
        return 301 https://github.com/ValGSgit/webserv;
    }
}
```

### 🔧 Configuration Directives

#### Server Block

| Directive | Description | Example |
|-----------|-------------|---------|
| `listen` | Port number to listen on | `listen 8080;` |
| `server_name` | Server hostname | `server_name localhost;` |
| `root` | Document root directory | `root ./www;` |
| `index` | Default index file | `index index.html;` |
| `autoindex` | Enable directory listing | `autoindex on;` |
| `client_max_body_size` | Max request body size (bytes) | `client_max_body_size 10485760;` |
| `error_page` | Custom error page | `error_page 404 /404.html;` |

#### Location Block

| Directive | Description | Example |
|-----------|-------------|---------|
| `allow_methods` | Allowed HTTP methods | `allow_methods GET POST DELETE;` |
| `root` | Location-specific root | `root ./www/api;` |
| `upload_pass` | File upload directory | `upload_pass ./uploads;` |
| `cgi_extension` | CGI script extensions | `cgi_extension .py .php;` |
| `return` | HTTP redirection | `return 301 /new-page;` |
| `autoindex` | Directory listing for location | `autoindex off;` |

### 🌐 Multiple Servers Example

```nginx
# Main production server
server {
    listen 8080;
    server_name localhost webserv.local;
    root ./www;
    client_max_body_size 52428800;  # 50MB
    
    location / {
        allow_methods GET POST DELETE;
        autoindex off;
    }
    
    location /cgi-bin {
        allow_methods GET POST;
        cgi_extension .py .php .pl .rb .sh;
    }
}

# Development server
server {
    listen 8081;
    server_name dev.local;
    root ./www-dev;
    autoindex on;  # Enable for all locations
    
    location / {
        allow_methods GET POST;
    }
}

# API-only server
server {
    listen 8082;
    server_name api.local;
    root ./www/api;
    
    location / {
        allow_methods GET POST DELETE;
    }
}
```

### 📋 Sample Configurations

The project includes several pre-configured examples:

- **`config/default.conf`** - Basic configuration for general use
- **`config/simple.conf`** - Minimal setup for testing
- **`config/complex.conf`** - Advanced multi-server setup
- **`webserv.conf`** - Full-featured production configuration

---

## 🧪 Testing

WebServ comes with a comprehensive test suite covering functionality, CGI, stress testing, and more.

### 🎯 Quick Test

```bash
# Terminal 1: Start the server
./webserv config/default.conf

# Terminal 2: Run all tests
cd tests/scripts
./run_all_tests.sh
```

### 🔬 Test Suites

#### 1️⃣ Functional Tests (45+ tests)

```bash
cd tests/scripts
./test_webserv.py

# Test categories:
✓ Static file serving
✓ HTTP methods (GET, POST, DELETE)
✓ Error handling (400, 403, 404, 405, 500)
✓ File uploads
✓ Directory listing
✓ Configuration parsing
✓ Multiple ports
```

#### 2️⃣ CGI Tests (12 tests)

```bash
./test_cgi.py

# Test coverage:
✓ CGI environment variables
✓ Query string handling
✓ POST data processing
✓ Multiple script types (.py, .php, .pl, .rb, .sh)
✓ CGI timeouts
✓ Special characters
```

#### 3️⃣ Stress Tests (6 test suites)

```bash
./stress_test.py

# Load testing:
✓ Concurrent connections (100+)
✓ Rapid requests (1000+ req/s)
✓ Large file uploads
✓ Sustained load (30+ seconds)
✓ Memory leak detection
✓ Connection stability
```

#### 4️⃣ Manual Tests

```bash
./manual_tests.sh

# Interactive tests with curl:
✓ Basic GET requests
✓ POST with form data
✓ File operations
✓ Custom headers
✓ Edge cases
```

#### 5️⃣ Siege Benchmarking

```bash
cd tests/siege
./test_siege.sh

# Performance metrics:
✓ Transaction rate
✓ Response time
✓ Throughput
✓ Concurrency handling
✓ Availability percentage
```

### 📊 Test Results

Expected performance metrics:

| Metric | Target | Description |
|--------|--------|-------------|
| **Functional Tests** | 95%+ pass | Core HTTP functionality |
| **CGI Tests** | 90%+ pass | Script execution |
| **Stress Tests** | 80%+ pass | High load scenarios |
| **Response Time** | <50ms | Average response time |
| **Throughput** | 1000+ req/s | Requests per second |
| **Memory Leaks** | 0 | Valgrind clean |

### 🐛 Memory Leak Testing

```bash
# Build and run with valgrind
make re
valgrind --leak-check=full --show-leak-kinds=all \
         --track-fds=yes ./webserv config/default.conf

# In another terminal, run tests
cd tests/scripts
./test_webserv.py

# Stop server with Ctrl+C and check valgrind output
```

---

## 🏗️ Architecture

### System Design

WebServ is built on a modern, event-driven architecture using Linux's `epoll` for efficient I/O multiplexing.

```
┌─────────────────────────────────────────────────────────────┐
│                         Client                              │
│                    (Web Browser/curl)                       │
└────────────────────────┬────────────────────────────────────┘
                         │ HTTP Request
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                      WebServ Server                         │
│  ┌───────────────────────────────────────────────────────┐  │
│  │                  ServerManager                        │  │
│  │  • Manages epoll event loop                          │  │
│  │  • Handles multiple server sockets (ports)           │  │
│  │  • Tracks client connections                         │  │
│  │  • Performs timeout cleanup                          │  │
│  └──────────────┬────────────────────────────────────────┘  │
│                 │                                            │
│  ┌──────────────▼────────────────────────────────────────┐  │
│  │                  HttpHandler                          │  │
│  │  • Parses HTTP requests                              │  │
│  │  • Routes to appropriate handler                     │  │
│  │  • Generates HTTP responses                          │  │
│  │  • Manages read/write operations                     │  │
│  └──┬───────────────────┬─────────────────┬─────────────┘  │
│     │                   │                 │                 │
│     ▼                   ▼                 ▼                 │
│  ┌──────┐          ┌────────┐       ┌─────────┐           │
│  │Static│          │  CGI   │       │  Upload │           │
│  │Files │          │Handler │       │ Handler │           │
│  └──────┘          └────────┘       └─────────┘           │
│                         │                                   │
│                         ▼                                   │
│              ┌─────────────────────┐                       │
│              │  Python/PHP/Perl    │                       │
│              │  Script Execution   │                       │
│              └─────────────────────┘                       │
└─────────────────────────────────────────────────────────────┘
                         │ HTTP Response
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                         Client                              │
└─────────────────────────────────────────────────────────────┘
```

### Core Components

#### 1. **ServerManager** (`includes/server/ServerManager.hpp`)
- Creates and manages multiple server sockets (one per port)
- Initializes epoll for event-driven I/O
- Maintains client connection pool
- Performs periodic timeout cleanup
- Handles graceful shutdown

#### 2. **HttpHandler** (`includes/http/HttpHandler.hpp`)
- Accepts new client connections
- Reads incoming HTTP requests
- Delegates to appropriate handlers (static, CGI, upload)
- Writes HTTP responses back to clients
- Manages connection lifecycle

#### 3. **ConfigParser** (`includes/config/ConfigParser.hpp`)
- Parses NGINX-style configuration files
- Validates syntax and directives
- Builds server configuration structures
- Supports multiple server blocks and locations

#### 4. **HttpRequest** (`includes/http/HttpRequest.hpp`)
- Parses HTTP request line, headers, and body
- Handles chunked transfer encoding
- Extracts query parameters
- Validates HTTP method and version

#### 5. **HttpResponse** (`includes/http/HttpResponse.hpp`)
- Generates HTTP response headers
- Sets appropriate status codes
- Handles MIME types
- Serves error pages

#### 6. **CgiHandler** (`includes/cgi/CgiHandler.hpp`)
- Executes CGI scripts (Python, PHP, Perl, Ruby, Shell)
- Sets up CGI environment variables
- Captures script output
- Handles timeouts and errors

### Request Flow

```
1. Client connects to server socket (port 8080)
   └─> ServerManager accepts connection
       └─> Adds client socket to epoll (EPOLLIN)

2. Client sends HTTP request
   └─> epoll_wait() detects EPOLLIN event
       └─> HttpHandler::handleRead()
           └─> HttpRequest::parseRequest()

3. Request processing
   └─> HttpHandler::processRequest()
       ├─> Static file? → Serve from filesystem
       ├─> CGI script?  → CgiHandler::executeCgi()
       └─> Upload?      → Save to upload directory

4. Response generation
   └─> HttpResponse::getResponseString()
       └─> Modify epoll to EPOLLOUT

5. Client ready to receive
   └─> epoll_wait() detects EPOLLOUT event
       └─> HttpHandler::handleWrite()
           └─> Send response data

6. Connection handling
   └─> Keep-Alive? → Return to step 2
       └─> Close?    → Clean up and close socket
```

### Key Technologies

- **epoll**: Linux's scalable I/O event notification mechanism
- **Non-blocking sockets**: All socket operations are non-blocking
- **Edge-triggered mode**: Efficient event notification (EPOLLET)
- **CGI 1.1**: Common Gateway Interface for script execution
- **HTTP/1.1**: Persistent connections, chunked transfer

### File Structure

```
webserv/
├── src/
│   ├── main.cpp                    # Entry point
│   ├── server/
│   │   └── ServerManager.cpp       # Main server loop
│   ├── http/
│   │   ├── HttpHandler.cpp         # Request/response handling
│   │   ├── HttpRequest.cpp         # HTTP parsing
│   │   ├── HttpResponse.cpp        # Response generation
│   │   └── HttpTemplates.cpp       # Error page templates
│   ├── config/
│   │   └── ConfigParser.cpp        # Configuration parsing
│   ├── cgi/
│   │   └── CgiHandler.cpp          # CGI execution
│   └── utils/
│       └── Utils.cpp                # Helper functions
├── includes/
│   ├── webserv.hpp                 # Main header
│   └── [component headers]         # Interface definitions
├── config/
│   └── [*.conf]                    # Configuration files
├── www/
│   ├── index.html                  # Web root
│   ├── cgi-bin/                    # CGI scripts
│   ├── uploads/                    # Upload directory
│   └── error_pages/                # Error pages
└── tests/
    ├── scripts/                    # Test suites
    └── siege/                      # Stress testing
```

---

## 📚 API Reference

### HTTP Methods

#### GET
```bash
# Static file
GET /index.html HTTP/1.1
Host: localhost:8080

# CGI with query string
GET /cgi-bin/test.py?name=John&age=30 HTTP/1.1
Host: localhost:8080

# Directory listing
GET /browse/ HTTP/1.1
Host: localhost:8080
```

#### POST
```bash
# Form data
POST /cgi-bin/test.py HTTP/1.1
Host: localhost:8080
Content-Type: application/x-www-form-urlencoded
Content-Length: 23

name=John&status=active

# File upload
POST /upload HTTP/1.1
Host: localhost:8080
Content-Type: multipart/form-data; boundary=----WebKitFormBoundary
Content-Length: 1234

------WebKitFormBoundary
Content-Disposition: form-data; name="file"; filename="test.txt"

[file contents]
------WebKitFormBoundary--
```

#### DELETE
```bash
# Delete a file
DELETE /uploads/test.txt HTTP/1.1
Host: localhost:8080
```

### Response Status Codes

| Code | Status | Description |
|------|--------|-------------|
| **200** | OK | Request successful |
| **201** | Created | File uploaded successfully |
| **204** | No Content | File deleted successfully |
| **301** | Moved Permanently | Resource has been redirected |
| **400** | Bad Request | Malformed request |
| **403** | Forbidden | Access denied |
| **404** | Not Found | Resource not found |
| **405** | Method Not Allowed | HTTP method not allowed for this resource |
| **413** | Payload Too Large | Request body exceeds max_body_size |
| **500** | Internal Server Error | Server error or CGI failure |
| **505** | HTTP Version Not Supported | Only HTTP/1.1 is supported |

### CGI Environment Variables

WebServ sets the following CGI environment variables:

| Variable | Description | Example |
|----------|-------------|---------|
| `REQUEST_METHOD` | HTTP method | `GET`, `POST` |
| `REQUEST_URI` | Full request URI | `/cgi-bin/test.py?name=John` |
| `QUERY_STRING` | Query parameters | `name=John&age=30` |
| `CONTENT_TYPE` | Request content type | `application/x-www-form-urlencoded` |
| `CONTENT_LENGTH` | Request body length | `123` |
| `SERVER_NAME` | Server hostname | `localhost` |
| `SERVER_PORT` | Server port | `8080` |
| `SERVER_PROTOCOL` | HTTP version | `HTTP/1.1` |
| `GATEWAY_INTERFACE` | CGI version | `CGI/1.1` |
| `SCRIPT_NAME` | Script path | `/cgi-bin/test.py` |
| `SCRIPT_FILENAME` | Full script path | `/var/www/cgi-bin/test.py` |

---

## 🎯 Project Goals

This project was built as part of the **42 School curriculum** to understand:

✅ **Low-level networking** - Socket programming, TCP/IP
✅ **I/O multiplexing** - epoll, non-blocking I/O
✅ **HTTP protocol** - Request/response cycle, headers, methods
✅ **CGI specification** - Environment variables, process execution
✅ **C++98 standard** - STL containers, memory management
✅ **Server architecture** - Event loops, connection pooling
✅ **Configuration parsing** - Tokenization, validation
✅ **Error handling** - Graceful failure, custom error pages

### 📜 Subject Requirements

All mandatory requirements have been implemented:

- ✅ Non-blocking I/O operations
- ✅ Single poll/select/epoll/kqueue for all I/O
- ✅ HTTP methods: GET, POST, DELETE
- ✅ Static file serving
- ✅ CGI execution
- ✅ File upload handling
- ✅ Multiple port listening
- ✅ Configuration file parsing
- ✅ Custom error pages
- ✅ Client max body size
- ✅ Location-based routing
- ✅ HTTP redirections
- ✅ Directory listing
- ✅ Default file handling

---

## 📸 Screenshots

### 🏠 Home Page
The main landing page showcasing the server capabilities.

### 🐍 CGI Demo
Interactive CGI script execution with environment variable display.

### 📤 File Upload
User-friendly file upload interface with drag-and-drop support.

### 📂 Directory Browser
Clean directory listing with file sizes and modification dates.

### ❌ Error Pages
Custom styled error pages for 404, 500, and other HTTP errors.

---

## 🤝 Contributing

While this is primarily an educational project, contributions are welcome!

### How to Contribute

1. **Fork the repository**
   ```bash
   git clone https://github.com/ValGSgit/webserv.git
   cd webserv
   ```

2. **Create a feature branch**
   ```bash
   git checkout -b feature/amazing-feature
   ```

3. **Make your changes**
   - Follow C++98 standard
   - Maintain coding style consistency
   - Add tests for new features

4. **Test thoroughly**
   ```bash
   make re
   ./run_all_tests.sh
   ```

5. **Commit and push**
   ```bash
   git commit -m "Add amazing feature"
   git push origin feature/amazing-feature
   ```

6. **Open a Pull Request**

### Development Guidelines

- 📝 **Code Style**: Follow existing conventions
- 🧪 **Testing**: Write tests for new features
- 📚 **Documentation**: Update README for significant changes
- 🐛 **Bug Reports**: Use GitHub issues with detailed descriptions
- ⚠️ **C++98**: No C++11/14/17/20 features

---

## 📄 License

This project is licensed under the MIT License. See [LICENSE](LICENSE) file for details.

---

## 👨‍💻 Authors

**ValGS**
- GitHub: [@ValGSgit](https://github.com/ValGSgit)
- Project: [webserv](https://github.com/ValGSgit/webserv)

**Ka Hou**
- Github: [@fankahou](https://github.com/fankahou)

---

## 🙏 Acknowledgments

- **42 School** for the project subject and guidance
- **NGINX** for configuration syntax inspiration
- **RFC 7230-7235** for HTTP/1.1 specification
- **RFC 3875** for CGI 1.1 specification
- The open-source community for tools and libraries

---

## 📚 Resources

__Networking__
- [Create a simple HTTP server in c](https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa)
- [(Video) Create a simple web server in c](https://www.youtube.com/watch?v=esXw4bdaZkc&ab_channel=JacobSorber)
- [(Video) explaining select()](https://www.youtube.com/watch?v=Y6pFtgRdUts&ab_channel=JacobSorber)
- [IBM - Nonblocking I/O and select()](https://www.ibm.com/support/knowledgecenter/ssw_ibm_i_72/rzab6/xnonblock.htm)
- [All about sockets blocking](http://dwise1.net/pgm/sockets/blocking.html)
- [TCP Socket Programming: HTTP](https://w3.cs.jmu.edu/kirkpams/OpenCSF/Books/csf/html/TCPSockets.html)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)

__HTTP__
- [MDN - HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP)
- [An Overview of the HTTP as Coverd in RFCs](https://www.inspirisys.com/HTTP_Protocol_as_covered_in_RFCs-An_Overview.pdf)
- [How the web works: HTTP and CGI explained](https://www.garshol.priv.no/download/text/http-tut.html)
- [MIME](https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types)
- [HTTP Status Codes](https://umbraco.com/knowledge-base/http-status-codes/)

__RFC__
- [How to Read an RFC](https://www.tutorialspoint.com/cplusplus/cpp_web_programming.htm)
- [RFC 9110 - HTTP Semantics ](https://www.rfc-editor.org/info/rfc9110)
- [RFC 9112 - HTTP/1.1 ](https://www.rfc-editor.org/info/rfc9112) 
- [RFC 2068 - ABNF](https://www.cs.columbia.edu/sip/syntax/rfc2068.html) 
- [RFC 3986 -  (URI) Generic Syntax](https://www.ietf.org/rfc/rfc3986)
- [RFC 6265 - HTTP State Management Mechanism (Cookies)](https://www.rfc-editor.org/rfc/rfc6265)
- [RFC 3875 - CGI](https://datatracker.ietf.org/doc/html/rfc3875)

__CGI__
- [Python web Programming](https://www.tutorialspoint.com/python/python_cgi_programming.htm)
- [CPP web Programming](https://www.tutorialspoint.com/cplusplus/cpp_web_programming.htm)
- [(Video) Creating a file upload page](https://www.youtube.com/watch?v=_j5spdsJdV8&t=562s)

__StackOverFlow__
- [What HTTP response headers are required](https://stackoverflow.com/questions/4726515/what-http-response-headers-are-required)
- [Why do we cast sockaddr_in to sockaddr when calling bind()](https://stackoverflow.com/questions/21099041/why-do-we-cast-sockaddr-in-to-sockaddr-when-calling-bind)
- [Is an entity body allowed for an HTTP DELETE request?](https://stackoverflow.com/questions/299628/is-an-entity-body-allowed-for-an-http-delete-request)
- [Sending images over http to browser in C](https://stackoverflow.com/questions/28631767/sending-images-over-http-to-browser-in-c)
- [Handling whitespaces in http headers](https://stackoverflow.com/questions/31773667/handling-whitespaces-in-http-headers)

__Tools__
- [Postman](https://www.postman.com/downloads/) : Send custom requests to the server
- [PuTTY](https://www.putty.org/) : Send raw data to the server (Windows Only)
    - [Video: How to use](https://www.youtube.com/watch?v=ptJYNY7UbQU&ab_channel=GeekThis)
- [Wireshark]() : Capture request/response traffic
- [Sige](https://www.linode.com/docs/guides/load-testing-with-siege/) : Load testing 

__Other__
- [URL Encoding](https://www.urlencoder.io/learn/#:~:text=A%20URL%20is%20composed%20from,%22%20%2C%20%22~%22%20)
- [Nginx](https://nginx.org/en/)
- [Nginx Source Code](https://github.com/nginx/nginx)

---

<div align="center">

### ⭐ If you found this project helpful, please consider giving it a star! ⭐

**Made with ❤️ and lots of ☕**

</div>

