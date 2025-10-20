#include "../../includes/config/ConfigParser.hpp"
#include "../../includes/utils/Utils.hpp"

ConfigParser::ConfigParser() {}

ConfigParser::~ConfigParser() {}

bool ConfigParser::parseConfig(const std::string& config_file) {
    _config_file = config_file;
    _servers.clear(); // Clear any existing servers
    
    // Read entire file using allowed functions
    std::string content = Utils::readFile(config_file);
    if (content.empty()) {
        return false;
    }

    std::vector<std::string> lines = splitIntoLines(content);
    
    // First pass: Check for basic syntax issues
    if (!validateBasicSyntax(lines)) {
        return false;
    }
    
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line = trim(lines[i]);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (line == "server {" || (line.find("server") == 0 && line.find("{") != std::string::npos)) {
            ServerConfig server;
            size_t new_index = parseServerBlock(lines, i + 1, server);
            
            // Always advance index, even if parsing failed
            if (new_index > i + 1) {
                i = new_index - 1; // -1 because loop will increment
            } else {
                // Server block parsing failed completely
                return false;
            }
            
            // Only add server if it has minimum requirements
            if (!server.ports.empty()) {
                _servers.push_back(server);
            }
        } else if (line.find("server") == 0) {
            // Server directive without proper opening brace
            return false;
        }
    }

    return !_servers.empty();
}

size_t ConfigParser::parseServerBlock(const std::vector<std::string>& lines, size_t start_index, ServerConfig& server) {
    int brace_count = 1;
    size_t i = start_index;
    size_t max_iterations = lines.size() * 2; // Safety limit
    size_t iterations = 0;

    while (i < lines.size() && brace_count > 0 && iterations < max_iterations) {
        iterations++;
        std::string line = trim(lines[i]);
        if (line.empty() || line[0] == '#') {
            i++;
            continue;
        }

        if (line == "}") {
            brace_count--;
            i++;
            continue;
        }
        
        if (line.find("location") != std::string::npos && line.find("{") != std::string::npos) {
            size_t start = line.find_first_of(" \t");
            size_t end = line.find_last_of(" \t{");
            if (start != std::string::npos && end != std::string::npos && end > start) {
                std::string location = trim(line.substr(start, end - start));
                RouteConfig route;
                size_t new_i = parseLocationBlock(lines, i + 1, route);
                if (new_i > i + 1) {
                    i = new_i;
                } else {
                    // Location parsing failed - this is an error
                    return start_index; // Return original index to indicate failure
                }
                server.routes[location] = route;
                continue;
            }
        }

        std::vector<std::string> tokens = split(line, ' ');
        if (tokens.size() < 2) {
            i++;
            continue;
        }

        std::string directive = tokens[0];
        std::string value = tokens[1];
        
        // Check if directive line ends with semicolon (basic syntax validation)
        bool has_semicolon = false;
        if (!line.empty() && line[line.length() - 1] == ';') {
            has_semicolon = true;
        }
        
        // Some directives require semicolons - be strict about this (case insensitive)
        if (Utils::equalsIgnoreCase(directive, "listen") || Utils::equalsIgnoreCase(directive, "server_name") || 
            Utils::equalsIgnoreCase(directive, "root") || Utils::equalsIgnoreCase(directive, "index") || 
            Utils::equalsIgnoreCase(directive, "autoindex") || Utils::equalsIgnoreCase(directive, "client_max_body_size")) {
            if (!has_semicolon) {
                // Strict validation - return failure
                return start_index;
            }
        }
        
        // Remove semicolon if present
        if (!value.empty() && value[value.length() - 1] == ';') {
            value = value.substr(0, value.length() - 1);
        }

        if (Utils::equalsIgnoreCase(directive, "listen")) {
            int port = Utils::toInt(value);
            if (Utils::isValidPort(port)) {
                server.ports.push_back(port);
            }
        }
        else if (Utils::equalsIgnoreCase(directive, "server_name")) {
            server.server_name = value;
        }
        else if (Utils::equalsIgnoreCase(directive, "root")) {
            server.root = value;
        }
        else if (Utils::equalsIgnoreCase(directive, "index")) {
            server.index = value;
        }
        else if (Utils::equalsIgnoreCase(directive, "autoindex")) {
            server.autoindex = (value == "on");
        }
        else if (Utils::equalsIgnoreCase(directive, "client_max_body_size")) {
            server.max_body_size = Utils::toSizeT(value);
        }
        else if (Utils::equalsIgnoreCase(directive, "error_page")) {
            if (tokens.size() >= 3) {
                int status = Utils::toInt(value);
                std::string page = tokens[2];
                if (!page.empty() && page[page.length() - 1] == ';') {
                    page = page.substr(0, page.length() - 1);
                }
                server.error_pages[status] = page;
            }
        }
        i++;
    }
    
    // Check if braces are balanced
    if (brace_count != 0) {
        return start_index; // Return original index to indicate incomplete server block
    }

    // Set default port if none specified
    if (server.ports.empty()) {
        server.ports.push_back(8080);
    }
    
    // Set default server name if none specified
    if (server.server_name.empty()) {
        server.server_name = "localhost";
    }
    
    // Ensure at least one route exists
    if (server.routes.empty()) {
        RouteConfig default_route;
        default_route.allowed_methods.push_back("GET");
        default_route.root_directory = server.root;
        default_route.directory_listing = server.autoindex;
        server.routes["/"] = default_route;
    }
    
    return i;
}

size_t ConfigParser::parseLocationBlock(const std::vector<std::string>& lines, size_t start_index, RouteConfig& route) {
    int brace_count = 1;
    size_t i = start_index;
    size_t max_iterations = lines.size(); // Safety limit
    size_t iterations = 0;

    while (i < lines.size() && brace_count > 0 && iterations < max_iterations) {
        iterations++;
        std::string line = trim(lines[i]);
        if (line.empty() || line[0] == '#') {
            i++;
            continue;
        }

        if (line.find("{") != std::string::npos) {
            brace_count++;
            i++;
            continue;
        }
        if (line == "}") {
            brace_count--;
            i++;
            continue;
        }

        std::vector<std::string> tokens = split(line, ' ');
        if (tokens.size() < 2) {
            i++;
            continue;
        }

        std::string directive = tokens[0];
        std::string value = tokens[1];
        
        // Remove semicolon if present
        if (!value.empty() && value[value.length() - 1] == ';') {
            value = value.substr(0, value.length() - 1);
        }

        if (Utils::equalsIgnoreCase(directive, "allow_methods")) {
            for (size_t j = 1; j < tokens.size(); ++j) {
                std::string method = tokens[j];
                if (!method.empty() && method[method.length() - 1] == ';') {
                    method = method.substr(0, method.length() - 1);
                }
                if (isValidMethod(method)) {
                    route.allowed_methods.push_back(method);
                }
            }
        }
        else if (Utils::equalsIgnoreCase(directive, "root")) {
            route.root_directory = value;
        }
        else if (Utils::equalsIgnoreCase(directive, "index")) {
            route.index_file = value;
        }
        else if (Utils::equalsIgnoreCase(directive, "autoindex")) {
            route.directory_listing = (value == "on");
        }
        else if (Utils::equalsIgnoreCase(directive, "upload_pass")) {
            route.upload_path = value;
        }
        else if (Utils::equalsIgnoreCase(directive, "cgi_extension")) {
            route.cgi_extension = value;
        }
        else if (Utils::equalsIgnoreCase(directive, "return")) {
            if (tokens.size() >= 3) {
                route.redirect_url = tokens[2];
                if (!route.redirect_url.empty() && route.redirect_url[route.redirect_url.length() - 1] == ';') {
                    route.redirect_url = route.redirect_url.substr(0, route.redirect_url.length() - 1);
                }
            }
        }
        else if (Utils::equalsIgnoreCase(directive, "client_max_body_size")) {
            route.max_body_size = Utils::toSizeT(value);
        }
        i++;
    }
    
    // Check if braces are balanced
    if (brace_count != 0) {
        return start_index; // Return original index to indicate incomplete location block
    }
    
    // Set default methods if none specified
    if (route.allowed_methods.empty()) {
        route.allowed_methods.push_back("GET");
    }
    
    return i;
}

bool ConfigParser::validateBasicSyntax(const std::vector<std::string>& lines) {
    int brace_count = 0;
    bool in_server_block = false;
    
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line = trim(lines[i]);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Count braces
        for (size_t j = 0; j < line.length(); ++j) {
            if (line[j] == '{') {
                brace_count++;
                if (line.find("server") != std::string::npos) {
                    in_server_block = true;
                }
            } else if (line[j] == '}') {
                brace_count--;
                if (brace_count < 0) {
                    return false; // More closing braces than opening
                }
            }
        }
        
        // Check for server directive without opening brace
        if (line.find("server") == 0 && line.find("{") == std::string::npos && !in_server_block) {
            return false;
        }
    }
    
    // Check final brace balance
    return brace_count == 0;
}

const std::vector<ServerConfig>& ConfigParser::getServers() const {
    return _servers;
}

std::string ConfigParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> ConfigParser::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == delimiter) {
            if (!token.empty()) {
                token = trim(token);
                if (!token.empty()) {
                    tokens.push_back(token);
                }
                token.clear();
            }
        } else {
            token += str[i];
        }
    }
    
    // Add last token if not empty
    if (!token.empty()) {
        token = trim(token);
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

std::vector<std::string> ConfigParser::splitIntoLines(const std::string& content) {
    std::vector<std::string> lines;
    std::string line;
    
    for (size_t i = 0; i < content.length(); ++i) {
        if (content[i] == '\n') {
            lines.push_back(line);
            line.clear();
        } else if (content[i] != '\r') {  // Skip carriage return
            line += content[i];
        }
    }
    
    // Add last line if it doesn't end with newline
    if (!line.empty()) {
        lines.push_back(line);
    }
    
    return lines;
}

bool ConfigParser::isValidMethod(const std::string& method) {
    return (method == "GET" || method == "POST" || method == "DELETE");
}

void ConfigParser::printConfig() const {
    for (size_t i = 0; i < _servers.size(); ++i) {
        const ServerConfig& server = _servers[i];
        std::cout << "Server " << i + 1 << ":" << std::endl;
        std::cout << "  Server name: " << server.server_name << std::endl;
        std::cout << "  Ports: ";
        for (size_t j = 0; j < server.ports.size(); ++j) {
            std::cout << server.ports[j];
            if (j < server.ports.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        std::cout << "  Root: " << server.root << std::endl;
        std::cout << "  Index: " << server.index << std::endl;
        std::cout << "  Max body size: " << server.max_body_size << std::endl;
        std::cout << "  Routes: " << server.routes.size() << std::endl;
    }
}
