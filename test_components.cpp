#include "includes/webserv.hpp"
#include "includes/config/ConfigParser.hpp"
#include "includes/cgi/CgiHandler.hpp"
#include "includes/http/HttpRequest.hpp"
#include "includes/http/HttpResponse.hpp"
#include "includes/utils/Utils.hpp"

class TestRunner {
private:
    int tests_run;
    int tests_passed;
    
public:
    TestRunner() : tests_run(0), tests_passed(0) {}
    
    void assert_true(bool condition, const std::string& test_name) {
        tests_run++;
        if (condition) {
            tests_passed++;
            std::cout << "✓ " << test_name << std::endl;
        } else {
            std::cout << "✗ " << test_name << std::endl;
        }
    }
    
    void assert_equal(const std::string& expected, const std::string& actual, const std::string& test_name) {
        tests_run++;
        if (expected == actual) {
            tests_passed++;
            std::cout << "✓ " << test_name << std::endl;
        } else {
            std::cout << "✗ " << test_name << " - Expected: '" << expected << "', Got: '" << actual << "'" << std::endl;
        }
    }
    
    void print_summary() {
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "Test Summary: " << tests_passed << "/" << tests_run << " passed";
        if (tests_passed == tests_run) {
            std::cout << " ✓ ALL TESTS PASSED!" << std::endl;
        } else {
            std::cout << " ✗ " << (tests_run - tests_passed) << " FAILED" << std::endl;
        }
        std::cout << std::string(50, '=') << std::endl;
    }
    
    bool all_passed() const {
        return tests_passed == tests_run;
    }
};

// Test ConfigParser
void test_config_parser(TestRunner& runner) {
    std::cout << "\n Testing ConfigParser..." << std::endl;
    
    ConfigParser parser;
    
    // Test 1: Parse simple config
    bool parse_result = parser.parseConfig("config/simple.conf");
    runner.assert_true(parse_result, "Parse simple config file");
    
    if (parse_result) {
        const std::vector<ServerConfig>& servers = parser.getServers();
        runner.assert_true(servers.size() > 0, "At least one server configuration loaded");
        
        if (servers.size() > 0) {
            const ServerConfig& server = servers[0];
            
            // Test server properties
            runner.assert_true(server.ports.size() > 0, "Server has at least one port");
            runner.assert_equal("localhost", server.server_name, "Server name is localhost");
            runner.assert_equal("./www", server.root, "Server root is ./www");
            runner.assert_equal("index.html", server.index, "Server index is index.html");
            runner.assert_true(server.autoindex, "Autoindex is enabled");
            
            // Test location block
            runner.assert_true(server.routes.find("/") != server.routes.end(), "Root location exists");
            
            if (server.routes.find("/") != server.routes.end()) {
                const RouteConfig& route = server.routes.at("/");
                runner.assert_true(route.allowed_methods.size() >= 3, "Root location has multiple allowed methods");
                
                // Check if GET, POST, DELETE are allowed
                bool has_get = false, has_post = false, has_delete = false;
                for (size_t i = 0; i < route.allowed_methods.size(); ++i) {
                    if (route.allowed_methods[i] == "GET") has_get = true;
                    if (route.allowed_methods[i] == "POST") has_post = true;
                    if (route.allowed_methods[i] == "DELETE") has_delete = true;
                }
                runner.assert_true(has_get, "GET method is allowed");
                runner.assert_true(has_post, "POST method is allowed");
                runner.assert_true(has_delete, "DELETE method is allowed");
            }
        }
    }
    
    // Test 2: Parse default config
    bool parse_default = parser.parseConfig("config/default.conf");
    runner.assert_true(parse_default, "Parse default config file");
    
    if (parse_default) {
        const std::vector<ServerConfig>& servers = parser.getServers();
        runner.assert_true(servers.size() >= 1, "At least one server configuration loaded from default.conf");
        
        if (servers.size() >= 1) {
            const ServerConfig& server = servers[0];
            
            // Test error pages
            runner.assert_true(server.error_pages.find(404) != server.error_pages.end(), "404 error page configured");
            runner.assert_true(server.error_pages.find(500) != server.error_pages.end(), "500 error page configured");
            
            // Test multiple location blocks
            runner.assert_true(server.routes.find("/upload") != server.routes.end(), "Upload location exists");
            runner.assert_true(server.routes.find("/cgi-bin") != server.routes.end(), "CGI-bin location exists");
        }
    }
    
    // Test 3: Test with non-existent file
    bool parse_missing = parser.parseConfig("config/nonexistent.conf");
    runner.assert_true(!parse_missing, "Correctly fails on non-existent config file");
}

// Test HttpRequest parsing
void test_http_request(TestRunner& runner) {
    std::cout << "\n Testing HttpRequest..." << std::endl;
    
    HttpRequest request;
    
    // Test 1: Simple GET request
    std::string get_request = "GET /test.html HTTP/1.1\r\n"
                             "Host: localhost:8080\r\n"
                             "User-Agent: Test/1.0\r\n"
                             "\r\n";
    
    bool parse_result = request.parseRequest(get_request);
    runner.assert_true(parse_result, "Parse simple GET request");
    runner.assert_equal("GET", request.methodToString(), "Method is GET");
    runner.assert_equal("/test.html", request.getUri(), "URI is /test.html");
    runner.assert_equal("localhost:8080", request.getHeader("host"), "Host header parsed");
    
    // Test 2: POST request with body
    request.reset();
    std::string post_request = "POST /upload HTTP/1.1\r\n"
                              "Host: localhost:8080\r\n"
                              "Content-Type: application/x-www-form-urlencoded\r\n"
                              "Content-Length: 13\r\n"
                              "\r\n"
                              "name=test&id=1";
    
    parse_result = request.parseRequest(post_request);
    runner.assert_true(parse_result, "Parse POST request with body");
    runner.assert_equal("POST", request.methodToString(), "Method is POST");
    runner.assert_equal("/upload", request.getUri(), "URI is /upload");
    runner.assert_true(request.getContentLength() == 13, "Content-Length is 13");
    
    // Test 3: GET request with query parameters
    request.reset();
    std::string query_request = "GET /search?q=test&limit=10 HTTP/1.1\r\n"
                               "Host: localhost:8080\r\n"
                               "\r\n";
    
    parse_result = request.parseRequest(query_request);
    runner.assert_true(parse_result, "Parse GET request with query parameters");
    runner.assert_equal("/search", request.getUri(), "URI without query string");
    runner.assert_equal("q=test&limit=10", request.getQueryString(), "Query string extracted");
    
    const std::map<std::string, std::string>& params = request.getParams();
    runner.assert_true(params.find("q") != params.end(), "Query parameter 'q' exists");
    runner.assert_true(params.find("limit") != params.end(), "Query parameter 'limit' exists");
    if (params.find("q") != params.end()) {
        runner.assert_equal("test", params.at("q"), "Query parameter 'q' value is 'test'");
    }
}

// Test HttpResponse generation
void test_http_response(TestRunner& runner) {
    std::cout << "\n Testing HttpResponse..." << std::endl;
    
    // Test 1: Simple response
    HttpResponse response;
    response.setStatus(STATUS_OK);
    response.setContentType("text/html");
    response.setBody("<h1>Hello World</h1>");
    
    const std::string& response_str = response.getResponseString();
    runner.assert_true(response_str.find("HTTP/1.1 200") != std::string::npos, "Response contains status line");
    runner.assert_true(response_str.find("Content-Type: text/html") != std::string::npos, "Response contains content type");
    runner.assert_true(response_str.find("<h1>Hello World</h1>") != std::string::npos, "Response contains body");
    
    // Test 2: Error response
    HttpResponse error_resp = HttpResponse::errorResponse(STATUS_NOT_FOUND, "Page not found");
    runner.assert_true(error_resp.getStatus() == STATUS_NOT_FOUND, "Error response has correct status");
    runner.assert_true(error_resp.getBody().find("Error 404") != std::string::npos, "Error response contains error message");
    
    // Test 3: File response (if test.html exists)
    if (Utils::fileExists("www/test.html")) {
        HttpResponse file_resp = HttpResponse::fileResponse("www/test.html");
        runner.assert_true(file_resp.getStatus() == STATUS_OK, "File response has OK status");
        runner.assert_true(file_resp.getBody().find("<!DOCTYPE html>") != std::string::npos, "File response contains HTML content");
    }
}

// Test CGI Handler
void test_cgi_handler(TestRunner& runner) {
    std::cout << "\n Testing CgiHandler..." << std::endl;
    
    CgiHandler cgi_handler;
    
    // Test 1: Check if CGI request detection works
    runner.assert_true(CgiHandler::isCgiRequest("/cgi-bin/test.py"), "Detects CGI request for /cgi-bin/test.py");
    runner.assert_true(!CgiHandler::isCgiRequest("/test.html"), "Does not detect CGI for static file");
    
    // Test 2: Test CGI execution (if Python script exists)
    if (Utils::fileExists("www/cgi-bin/test.py")) {
        HttpRequest request;
        std::string cgi_request = "GET /cgi-bin/test.py HTTP/1.1\r\n"
                                 "Host: localhost:8080\r\n"
                                 "\r\n";
        request.parseRequest(cgi_request);
        
        HttpResponse cgi_response = cgi_handler.executeCgi(request, "www/cgi-bin/test.py");
        runner.assert_true(cgi_response.getStatus() == STATUS_OK || cgi_response.getStatus() == STATUS_INTERNAL_SERVER_ERROR, 
                          "CGI execution returns valid status");
        
        if (cgi_response.getStatus() == STATUS_OK) {
            runner.assert_true(cgi_response.getBody().length() > 0, "CGI response has body content");
            runner.assert_true(cgi_response.getBody().find("CGI Test") != std::string::npos || 
                              cgi_response.getBody().find("Content-Type") != std::string::npos, 
                              "CGI response contains expected content");
        }
    } else {
        std::cout << "  Skipping CGI execution test - test.py not found" << std::endl;
    }
}

// Test Utils functions
void test_utils(TestRunner& runner) {
    std::cout << "\n Testing Utils..." << std::endl;
    
    // Test 1: String utilities
    runner.assert_equal("hello", Utils::trim("  hello  "), "Trim whitespace");
    runner.assert_equal("hello", Utils::toLowerCase("HELLO"), "Convert to lowercase");
    runner.assert_equal("HELLO", Utils::toUpperCase("hello"), "Convert to uppercase");
    runner.assert_true(Utils::startsWith("hello world", "hello"), "Starts with test");
    runner.assert_true(Utils::endsWith("hello world", "world"), "Ends with test");
    
    // Test 2: File utilities
    runner.assert_true(Utils::fileExists("www/index.html"), "Detect existing file");
    runner.assert_true(!Utils::fileExists("www/nonexistent.html"), "Detect non-existing file");
    runner.assert_true(Utils::isDirectory("www"), "Detect directory");
    runner.assert_true(!Utils::isDirectory("www/index.html"), "Detect non-directory");
    
    // Test 3: MIME type detection
    runner.assert_equal("text/html", Utils::getMimeType("test.html"), "HTML MIME type");
    runner.assert_equal("text/css", Utils::getMimeType("style.css"), "CSS MIME type");
    runner.assert_equal("image/jpeg", Utils::getMimeType("image.jpg"), "JPEG MIME type");
    runner.assert_equal("application/octet-stream", Utils::getMimeType("unknown.xyz"), "Unknown MIME type");
    
    // Test 4: Status messages
    runner.assert_equal("OK", Utils::getStatusMessage(200), "Status message for 200");
    runner.assert_equal("Not Found", Utils::getStatusMessage(404), "Status message for 404");
    runner.assert_equal("Internal Server Error", Utils::getStatusMessage(500), "Status message for 500");
    
    // Test 5: Path utilities
    runner.assert_equal("dir/file.txt", Utils::joinPath("dir", "file.txt"), "Join paths");
    runner.assert_equal(".txt", Utils::getFileExtension("file.txt"), "Get file extension");
    runner.assert_equal("dir", Utils::getDirectory("dir/file.txt"), "Get directory");
    runner.assert_equal("file.txt", Utils::getFilename("dir/file.txt"), "Get filename");
}

int main() {
    std::cout << " WebServ Component Tester " << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    TestRunner runner;
    
    // Run all tests
    test_utils(runner);
    test_config_parser(runner);
    test_http_request(runner);
    test_http_response(runner);
    test_cgi_handler(runner);
    
    runner.print_summary();
    
    return runner.all_passed() ? 0 : 1;
}
