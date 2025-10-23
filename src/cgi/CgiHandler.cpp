#include "../../includes/cgi/CgiHandler.hpp"
#include "../../includes/http/HttpRequest.hpp"
#include "../../includes/http/HttpResponse.hpp"
#include "../../includes/http/HttpStatusCodes.hpp"
#include "../../includes/utils/Utils.hpp"

CgiHandler::CgiHandler() : _timeout(CGI_TIMEOUT) {}

CgiHandler::~CgiHandler() {}

HttpResponse CgiHandler::executeCgi(const HttpRequest& request, const std::string& script_path) {
    if (!Utils::fileExists(script_path) || !Utils::isReadable(script_path)) {
        return HttpResponse::errorResponse(HTTP_NOT_FOUND);
    }
    
    // Find CGI executable
    std::string extension = Utils::getFileExtension(script_path);
    std::string cgi_executable = findCgiExecutable(extension);
    
    if (cgi_executable.empty()) {
        return HttpResponse::errorResponse(HTTP_NOT_IMPLEMENTED);
    }
    
    // Create pipes for communication
    int stdin_pipe[2], stdout_pipe[2];
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
        perror("pipe");
        return HttpResponse::errorResponse(HTTP_INTERNAL_SERVER_ERROR);
    }
    
    // Fork process
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        return HttpResponse::errorResponse(HTTP_INTERNAL_SERVER_ERROR);
    }
    
    if (pid == 0) {
        // Child process
        close(stdin_pipe[1]);   // Close write end of stdin pipe
        close(stdout_pipe[0]);  // Close read end of stdout pipe
        
        // Redirect stdin and stdout
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
        
        // Setup environment
        setupEnvironment(request, script_path);
        
        // Execute CGI script
        const char* args[] = {cgi_executable.c_str(), script_path.c_str(), NULL};
        execve(cgi_executable.c_str(), const_cast<char* const*>(args), NULL);
        
        // If we get here, exec failed
        perror("execve");
        exit(1);
    }
    
    // Parent process
    close(stdin_pipe[0]);   // Close read end of stdin pipe
    close(stdout_pipe[1]);  // Close write end of stdout pipe
    
    // Write request body to CGI stdin
    const std::string& body = request.getBody();
    if (!body.empty()) {
        write(stdin_pipe[1], body.c_str(), body.length());
    }
    close(stdin_pipe[1]);
    
    // Read CGI output
    std::string output;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    // Set timeout for reading
    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = _timeout;
    timeout.tv_usec = 0;
    
    while (true) {
        FD_ZERO(&readfds);
        FD_SET(stdout_pipe[0], &readfds);
        
        int select_result = select(stdout_pipe[0] + 1, &readfds, NULL, NULL, &timeout);
        
        if (select_result == -1) {
            perror("select");
            break;
        } else if (select_result == 0) {
            // Timeout
            kill(pid, SIGTERM);
            break;
        }
        
        bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) break;
        
        buffer[bytes_read] = '\0';
        output += std::string(buffer, bytes_read);
    }
    
    close(stdout_pipe[0]);
    
    // Wait for child process
    int status;
    waitpid(pid, &status, 0);
    
    if (output.empty()) {
        return HttpResponse::errorResponse(HTTP_INTERNAL_SERVER_ERROR);
    }
    
    return parseCgiOutput(output);
}

void CgiHandler::setupEnvironment(const HttpRequest& request, const std::string& script_path) {
    // Clear environment
    _env.clear();
    
    // CGI environment variables
    _env["REQUEST_METHOD"] = request.methodToString();
    _env["REQUEST_URI"] = request.getUri();
    _env["QUERY_STRING"] = request.getQueryString();
    _env["SERVER_NAME"] = "localhost";
    _env["SERVER_PORT"] = "8080";
    _env["SERVER_PROTOCOL"] = "HTTP/1.1";
    _env["SERVER_SOFTWARE"] = "WebServ/1.0";
    _env["GATEWAY_INTERFACE"] = "CGI/1.1";
    _env["SCRIPT_NAME"] = request.getUri();
    _env["SCRIPT_FILENAME"] = script_path;
    _env["PATH_INFO"] = "";
    _env["PATH_TRANSLATED"] = "";
    _env["DOCUMENT_ROOT"] = "./www";
    
    // Content related
    std::string content_type = request.getHeader("Content-Type");
    std::string content_length = request.getHeader("Content-Length");
    
    if (!content_type.empty()) {
        _env["CONTENT_TYPE"] = content_type;
    }
    if (!content_length.empty()) {
        _env["CONTENT_LENGTH"] = content_length;
    } else {
        _env["CONTENT_LENGTH"] = Utils::toString(request.getBody().length());
    }
    
    // HTTP headers (convert to HTTP_HEADER_NAME format)
    const std::map<std::string, std::string>& headers = request.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        std::string env_name = "HTTP_" + Utils::toUpperCase(it->first);
        // Replace hyphens with underscores
        for (size_t i = 0; i < env_name.length(); ++i) {
            if (env_name[i] == '-') {
                env_name[i] = '_';
            }
        }
        _env[env_name] = it->second;
    }
    
    // NOTE: setenv is not in allowed functions list
    // Environment variables would need to be passed differently to CGI
    // For now, we'll comment this out and would need to implement
    // a different approach for CGI environment variable passing
    
    // Set environment variables (DISABLED - setenv not allowed)
    // for (std::map<std::string, std::string>::const_iterator it = _env.begin();
    //      it != _env.end(); ++it) {
    //     setenv(it->first.c_str(), it->second.c_str(), 1);
    // }
}

std::string CgiHandler::findCgiExecutable(const std::string& extension) {
    if (extension == ".php") {
        if (Utils::fileExists("/usr/bin/php")) return "/usr/bin/php";
        if (Utils::fileExists("/usr/local/bin/php")) return "/usr/local/bin/php";
    } else if (extension == ".py") {
        if (Utils::fileExists("/usr/bin/python3")) return "/usr/bin/python3";
        if (Utils::fileExists("/usr/bin/python")) return "/usr/bin/python";
        if (Utils::fileExists("/usr/local/bin/python3")) return "/usr/local/bin/python3";
    } else if (extension == ".pl") {
        if (Utils::fileExists("/usr/bin/perl")) return "/usr/bin/perl";
        if (Utils::fileExists("/usr/local/bin/perl")) return "/usr/local/bin/perl";
    } else if (extension == ".rb") {
        if (Utils::fileExists("/usr/bin/ruby")) return "/usr/bin/ruby";
        if (Utils::fileExists("/usr/local/bin/ruby")) return "/usr/local/bin/ruby";
    }
    
    return "";
}

HttpResponse CgiHandler::parseCgiOutput(const std::string& output) {
    HttpResponse response;
    
    // Find the separator between headers and body
    size_t header_end = output.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        header_end = output.find("\n\n");
        if (header_end == std::string::npos) {
            // No headers, treat everything as body
            response.setStatus(HTTP_OK);
            response.setContentType("text/html");
            response.setBody(output);
            return response;
        }
    }
    
    // Parse headers
    std::string headers = output.substr(0, header_end);
    std::string body = output.substr(header_end + (output.find("\r\n\r\n") != std::string::npos ? 4 : 2));
    
    std::vector<std::string> header_lines = Utils::split(headers, '\n');
    bool status_set = false;
    
    for (size_t i = 0; i < header_lines.size(); ++i) {
        std::string line = Utils::trim(header_lines[i]);
        if (line.empty()) continue;
        
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = Utils::trim(line.substr(0, colon_pos));
            std::string value = Utils::trim(line.substr(colon_pos + 1));
            
            if (Utils::toLowerCase(key) == "status") {
                int status_code = Utils::toInt(value);
                if (status_code > 0) {
                    response.setStatus(status_code);
                    status_set = true;
                }
            } else {
                response.setHeader(key, value);
            }
        }
    }
    
    if (!status_set) {
        response.setStatus(HTTP_OK);
    }
    
    response.setBody(body);
    return response;
}

void CgiHandler::setTimeout(int seconds) {
    _timeout = seconds;
}

bool isCgiRequest(const std::string& uri, const RouteConfig& route) {
    // Check if URI matches any CGI extension
    for (size_t i = 0; i < route.cgi_extensions.size(); ++i) {
        const std::string& ext = route.cgi_extensions[i];
        if (uri.length() >= ext.length()) {
            if (uri.substr(uri.length() - ext.length()) == ext) {
                return true;
            }
        }
    }
    return false;
}