#ifndef HTTP_HANDLER_HPP
#define HTTP_HANDLER_HPP

#include "../webserv.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "../cgi/CgiHandler.hpp"
#include <sys/epoll.h>
#include "../utils/Utils.hpp"
#include "../config/ConfigParser.hpp"

class HttpHandler {
    private:
        HttpHandler(HttpHandler const &other);
        HttpHandler &operator=(HttpHandler const &other);

        int _server_fd;
        int _epoll_fd;
        ConfigParser &_config; // use ref?
        std::map<int, std::string> _client_buffers;
        std::map<int, HttpRequest> _client_requests;
        std::map<int, HttpResponse> _client_responses;
        std::map<int, std::string> _response_buffers;
        std::map<int, size_t> _response_offsets;

        void processRequest(int client_fd);
        HttpResponse handleUpload(const HttpRequest& request);
        HttpResponse handleJsonApi(const HttpRequest& request);
        const ServerConfig &find_server(int port);
        bool method_allowed(std::string &uri, std::string method);
    public:
        HttpHandler(int server_fd, int epoll_fd, ConfigParser &config); // use ref?
        ~HttpHandler();
        void handleRead(int fd);
        void handleWrite(int fd);
        void acceptConnection();
        void closeConnection(int client_fd);

};

#endif
