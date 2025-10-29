# Debug Utilities Documentation

This document describes all the debug/display functions available to inspect the state of your WebServ structures at runtime.

## Available Debug Functions

### 1. Utils Class (Static Functions)

#### Display Structure Data

```cpp
#include "includes/utils/Utils.hpp"

// Display a RouteConfig
RouteConfig route;
Utils::printRouteConfig(route, "/api");

// Display a ServerConfig
ServerConfig config;
Utils::printServerConfig(config);

// Display a ServerSocket
ServerSocket socket;
Utils::printServerSocket(socket);

// Display a ClientConnection
ClientConnection client;
Utils::printClientConnection(client);

#ifdef BONUS
// Display SessionData
SessionData session;
Utils::printSessionData(session);
#endif
```

#### Helper Conversion Functions

```cpp
// Convert HttpMethod to string
HttpMethod method = METHOD_GET;
std::string methodStr = Utils::httpMethodToString(method);

// Convert ConnectionState to string
ConnectionState state = STATE_READING_HEADERS;
std::string stateStr = Utils::connectionStateToString(state);
```

### 2. HttpRequest Class

```cpp
#include "includes/http/HttpRequest.hpp"

HttpRequest request;
// ... parse request data ...

// Display all request information
request.print();
```

Output includes:
- HTTP Method
- URI and Query String
- HTTP Version
- All Headers
- Request Parameters
- Cookies (if BONUS enabled)
- Body preview
- Parse status

### 3. HttpResponse Class

```cpp
#include "includes/http/HttpResponse.hpp"

HttpResponse response;
// ... build response ...

// Display all response information
response.print();
```

Output includes:
- HTTP Status Code
- All Headers
- Set-Cookie headers (if BONUS enabled)
- Body size and preview
- Full response string size

### 4. ServerManager Class

```cpp
#include "includes/server/ServerManager.hpp"

ServerManager manager;
// ... initialize manager ...

// Display overall server status
manager.printServerStatus();

// Display all active client connections
manager.printAllClients();

// Display all server sockets
manager.printServerSockets();
```

## Example Usage in Main Loop

Here's how to add debugging to your main run loop:

```cpp
// In main.cpp or wherever you want to debug

#include "includes/webserv.hpp"
#include "includes/server/ServerManager.hpp"
#include "includes/utils/Utils.hpp"
#include <signal.h>

// Global signal handler setup
volatile sig_atomic_t g_debug_signal = 0;

void debugSignalHandler(int signal) {
    if (signal == SIGUSR1) {
        g_debug_signal = 1;
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    // Setup signal handler for debug output
    signal(SIGUSR1, debugSignalHandler);
    
    ServerManager manager;
    
    if (!manager.initialize(argv[1])) {
        std::cerr << "Failed to initialize server" << std::endl;
        return 1;
    }
    
    std::cout << "\n=== Initial Server State ===" << std::endl;
    manager.printServerStatus();
    
    // In your run loop, check for debug signal
    // This allows you to send SIGUSR1 to trigger debug output
    // Example: kill -SIGUSR1 <pid>
    
    manager.run();
    
    return 0;
}
```

## Adding Debug Output to ServerManager::run()

You can modify the run loop to periodically show status:

```cpp
void ServerManager::run() {
    _running = true;
    _last_cleanup = time(NULL);
    time_t last_debug = time(NULL);
    
    std::cout << "\n🚀 Starting WebServ..." << std::endl;
    std::cout << "📊 Send SIGUSR1 for debug info: kill -SIGUSR1 " << getpid() << std::endl;
    
    while (_running) {
        int nfds = epoll_wait(_epoll_fd, _events, MAX_CONNECTIONS, 1000);
        
        // Check for debug signal (if using global g_debug_signal)
        extern volatile sig_atomic_t g_debug_signal;
        if (g_debug_signal) {
            g_debug_signal = 0;
            std::cout << "\n=== DEBUG INFO ===" << std::endl;
            printServerStatus();
            printAllClients();
        }
        
        // Or periodic debug output (every 30 seconds)
        time_t now = time(NULL);
        if (now - last_debug >= 30) {
            last_debug = now;
            std::cout << "\n=== Periodic Status Update ===" << std::endl;
            printServerStatus();
        }
        
        // ... rest of your event handling code ...
    }
}
```

## Debug Output for Specific Clients

```cpp
// In HttpHandler or wherever you process requests

void HttpHandler::handleRead(int fd) {
    // ... your existing code ...
    
    // Debug specific client
    ClientConnection* client = _server_manager->getClient(fd);
    if (client) {
        std::cout << "\n=== Processing Client FD " << fd << " ===" << std::endl;
        Utils::printClientConnection(*client);
    }
    
    // Debug request
    if (_client_requests.find(fd) != _client_requests.end()) {
        std::cout << "\n=== Request for FD " << fd << " ===" << std::endl;
        _client_requests[fd].print();
    }
}

void HttpHandler::handleWrite(int fd) {
    // ... your existing code ...
    
    // Debug response
    if (_client_responses.find(fd) != _client_responses.end()) {
        std::cout << "\n=== Response for FD " << fd << " ===" << std::endl;
        _client_responses[fd].print();
    }
}
```

## Debug Configuration Parsing

```cpp
// In main.cpp after parsing config

ConfigParser parser;
if (parser.parseConfig(argv[1])) {
    const std::vector<ServerConfig>& servers = parser.getServers();
    
    std::cout << "\n=== Parsed Configuration ===" << std::endl;
    std::cout << "Total Servers: " << servers.size() << std::endl;
    
    for (size_t i = 0; i < servers.size(); ++i) {
        std::cout << "\n--- Server " << (i + 1) << " ---" << std::endl;
        Utils::printServerConfig(servers[i]);
    }
}
```

## Environment Variables for Debug Mode

You can add an environment variable to enable/disable debug output:

```cpp
// In main.cpp
bool debug_mode = std::getenv("WEBSERV_DEBUG") != NULL;

if (debug_mode) {
    std::cout << "🐛 Debug mode enabled" << std::endl;
    // Print all structures
    manager.printServerStatus();
    manager.printServerSockets();
}
```

Then run with:
```bash
WEBSERV_DEBUG=1 ./webserv webserv.conf
```

## Integration with Your Existing Code

All these functions are non-intrusive and can be called anywhere in your code without modifying the core logic. Simply include the appropriate headers and call the print functions when needed.

### Quick Reference Table

| Structure/Class     | Function to Call                   | Header Required              |
|--------------------|-------------------------------------|------------------------------|
| RouteConfig        | `Utils::printRouteConfig()`         | `utils/Utils.hpp`            |
| ServerConfig       | `Utils::printServerConfig()`        | `utils/Utils.hpp`            |
| ServerSocket       | `Utils::printServerSocket()`        | `utils/Utils.hpp`            |
| ClientConnection   | `Utils::printClientConnection()`    | `utils/Utils.hpp`            |
| SessionData        | `Utils::printSessionData()`         | `utils/Utils.hpp` + BONUS    |
| HttpRequest        | `request.print()`                   | `http/HttpRequest.hpp`       |
| HttpResponse       | `response.print()`                  | `http/HttpResponse.hpp`      |
| ServerManager      | `manager.printServerStatus()`       | `server/ServerManager.hpp`   |
| ServerManager      | `manager.printAllClients()`         | `server/ServerManager.hpp`   |
| ServerManager      | `manager.printServerSockets()`      | `server/ServerManager.hpp`   |

## Testing the Debug Functions

Compile with debug functions and test:

```bash
make
./webserv webserv.conf

# In another terminal, trigger debug output (if using signal handler):
kill -SIGUSR1 $(pgrep webserv)
```
