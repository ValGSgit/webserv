#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "../webserv.hpp"
#include "HttpStatusCodes.hpp"
#include <ctime>

class HttpRequest;

class HttpResponse {
    private:
        int _status;
        std::map<std::string, std::string> _headers;
        std::string _body;
        std::string _response_string;
        bool _headers_sent;

        std::string statusToString(int status);
        std::string getCurrentTime();
        void buildResponseString();

    public:
        HttpResponse();
        ~HttpResponse();

        void setStatus(int status);
        void setHeader(const std::string& key, const std::string& value);
        void setBody(const std::string& body);
        void appendBody(const std::string& data);
        
        int getStatus() const;
        const std::string& getBody() const;
        const std::string& getResponseString();

        void reset();
        void setDefaultHeaders();
        void setContentType(const std::string& content_type);
        void setContentLength(size_t length);
        
        // Static response builders
        static HttpResponse errorResponse(int status, const std::string& message = "");
        static HttpResponse messageResponse(int status, const std::string& title = "", const std::string& message = "");
        static HttpResponse fileResponse(const std::string& filepath);
        static HttpResponse directoryListingResponse(const std::string& path, const std::string& uri);
        static HttpResponse redirectResponse(const std::string& location, int status_code = HTTP_MOVED_PERMANENTLY);
};

#endif
