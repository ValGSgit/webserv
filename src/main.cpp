#include "../includes/webserv.hpp"
#include "../includes/config/ConfigParser.hpp"
#include "../includes/http/HttpRequest.hpp"
#include "../includes/http/HttpResponse.hpp"
#include "../includes/cgi/CgiHandler.hpp"
#include "../includes/utils/Utils.hpp"
//#include "../includes/server/EventLoop.hpp"
//#include "../includes/server/Server.hpp"

volatile sig_atomic_t g_running = 1;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutting down server..." << std::endl;
        g_running = 0;
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGPIPE, SIG_IGN);

    // Parse configuration
    std::cout << "Parsing configuration file: " << argv[1] << std::endl;
    ConfigParser parser;
    
    if (!parser.parseConfig(argv[1])) {
        std::cerr << "Error: Failed to parse configuration file" << std::endl;
        return 1;
    }
    
    std::cout << "✓ Configuration file parsed successfully" << std::endl;

    // Validate configuration
    const std::vector<ServerConfig>& servers = parser.getServers();
    if (servers.empty()) {
        std::cerr << "Error: No server configurations found" << std::endl;
        return 1;
    }
    
    std::cout << "✓ Found " << servers.size() << " server configuration(s)" << std::endl;
    
    // Detailed configuration validation
    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfig& server = servers[i];
        
        std::cout << "\n--- Server " << (i + 1) << " Configuration ---" << std::endl;
        std::cout << "Server name: " << server.server_name << std::endl;
        
        // Check ports
        if (server.ports.empty()) {
            std::cerr << "Warning: Server has no listening ports configured" << std::endl;
        } else {
            std::cout << "Listening on port(s): ";
            for (size_t j = 0; j < server.ports.size(); ++j) {
                std::cout << server.ports[j];
                if (j < server.ports.size() - 1) std::cout << ", ";
            }
            std::cout << std::endl;
        }
        
        // Check document root
        std::cout << "Document root: " << server.root << std::endl;
        if (!Utils::isDirectory(server.root)) {
            std::cerr << "Warning: Document root '" << server.root << "' is not accessible" << std::endl;
        } else {
            std::cout << "✓ Document root is accessible" << std::endl;
        }
        
        // Check index file
        std::cout << "Index file: " << server.index << std::endl;
        std::string index_path = Utils::joinPath(server.root, server.index);
        if (!Utils::fileExists(index_path)) {
            std::cerr << "Warning: Index file '" << index_path << "' not found" << std::endl;
        } else {
            std::cout << "✓ Index file exists" << std::endl;
        }
        
        // Check autoindex setting
        std::cout << "Directory listing: " << (server.autoindex ? "enabled" : "disabled") << std::endl;
        
        // Check max body size
        std::cout << "Max body size: " << server.max_body_size << " bytes" << std::endl;
        
        // Check error pages
        if (!server.error_pages.empty()) {
            std::cout << "Error pages configured:" << std::endl;
            for (std::map<int, std::string>::const_iterator it = server.error_pages.begin();
                 it != server.error_pages.end(); ++it) {
                std::cout << "  " << it->first << " -> " << it->second << std::endl;
                
                // Check if error page file exists
                std::string error_page_path = Utils::joinPath(server.root, it->second);
                if (!Utils::fileExists(error_page_path)) {
                    std::cerr << "Warning: Error page '" << error_page_path << "' not found" << std::endl;
                } else {
                    std::cout << "    ✓ Error page file exists" << std::endl;
                }
            }
        }
        
        // Check location blocks
        if (!server.routes.empty()) {
            std::cout << "Location blocks configured:" << std::endl;
            for (std::map<std::string, RouteConfig>::const_iterator it = server.routes.begin();
                 it != server.routes.end(); ++it) {
                const std::string& location = it->first;
                const RouteConfig& route = it->second;
                
                std::cout << "  " << location << ":" << std::endl;
                
                // Check allowed methods
                if (!route.allowed_methods.empty()) {
                    std::cout << "    Methods: ";
                    for (size_t j = 0; j < route.allowed_methods.size(); ++j) {
                        std::cout << route.allowed_methods[j];
                        if (j < route.allowed_methods.size() - 1) std::cout << ", ";
                    }
                    std::cout << std::endl;
                }
                
                // Check root directory for this location
                if (!route.root_directory.empty()) {
                    std::cout << "    Root: " << route.root_directory << std::endl;
                    if (!Utils::isDirectory(route.root_directory)) {
                        std::cerr << "    Warning: Location root '" << route.root_directory << "' not accessible" << std::endl;
                    }
                }
                
                // Check upload path
                if (!route.upload_path.empty()) {
                    std::cout << "    Upload path: " << route.upload_path << std::endl;
                    if (!Utils::isDirectory(route.upload_path)) {
                        std::cerr << "    Warning: Upload path '" << route.upload_path << "' not accessible" << std::endl;
                    } else if (!Utils::isWritable(route.upload_path)) {
                        std::cerr << "    Warning: Upload path '" << route.upload_path << "' not writable" << std::endl;
                    } else {
                        std::cout << "    ✓ Upload directory is writable" << std::endl;
                    }
                }
                
                // Check CGI extensions
                if (!route.cgi_extension.empty()) {
                    std::cout << "    CGI extensions: " << route.cgi_extension << std::endl;
                }
                
                // Check redirects
                if (!route.redirect_url.empty()) {
                    std::cout << "    Redirect to: " << route.redirect_url << std::endl;
                }
                
                std::cout << "    Directory listing: " << (route.directory_listing ? "enabled" : "disabled") << std::endl;
                std::cout << "    Max body size: " << route.max_body_size << " bytes" << std::endl;
            }
        }
    }
    
    std::cout << "Testing config parser memory management..." << std::endl;
    
    for (int i = 0; i < 10; ++i) {
        ConfigParser* test_parser = new ConfigParser();
        test_parser->parseConfig(argv[1]);
        delete test_parser;
    }
    std::cout << "✓ Created and destroyed 10 ConfigParser instances" << std::endl;
    
    // Test HTTP components
    std::cout << "Testing HTTP component memory management..." << std::endl;
    for (int i = 0; i < 5; ++i) {
        HttpRequest* request = new HttpRequest();
        HttpResponse* response = new HttpResponse();
        
        // Test parsing a simple request
        std::string test_request = "GET /test HTTP/1.1\r\nHost: localhost\r\n\r\n";
        request->parseRequest(test_request);
        
        // Test generating a simple response
        response->setStatus(STATUS_OK);
        response->setBody("<html><body>Test</body></html>");
        response->getResponseString();
        
        delete request;
        delete response;
    }
    std::cout << "✓ Created and destroyed 5 HTTP request/response pairs" << std::endl;
    
    // Test CGI handler
    std::cout << "Testing CGI handler memory management..." << std::endl;
    for (int i = 0; i < 3; ++i) {
        CgiHandler* cgi = new CgiHandler();
        delete cgi;
    }
    std::cout << "✓ Created and destroyed 3 CGI handlers" << std::endl;
    
    std::cout << "\n--- Validation Complete ---" << std::endl;
    std::cout << "✓ All components initialized successfully" << std::endl;
    std::cout << "✓ Configuration is valid" << std::endl;
    std::cout << "✓ Memory management tests completed" << std::endl;
    std::cout << "\nNote: Run with valgrind to detect any memory leaks:" << std::endl;
    std::cout << "  valgrind --leak-check=full ./webserv " << argv[1] << std::endl;
    
    std::cout << "\nServer would start here with " << servers.size() << " server(s)" << std::endl;
    std::cout << "Server shutdown complete" << std::endl;
    return 0;
}
