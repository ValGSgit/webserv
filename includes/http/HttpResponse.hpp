#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "../webserv.hpp"

class HttpRequest;

class HttpResponse {
    private:
        HttpStatus _status;
        std::map<std::string, std::string> _headers;
        std::string _body;
        std::string _response_string;
        bool _headers_sent;

        std::string statusToString(HttpStatus status);
        std::string getCurrentTime();
        void buildResponseString();

    public:
        HttpResponse();
        ~HttpResponse();

        void setStatus(HttpStatus status);
        void setHeader(const std::string& key, const std::string& value);
        void setBody(const std::string& body);
        void appendBody(const std::string& data);
        
        HttpStatus getStatus() const;
        const std::string& getBody() const;
        const std::string& getResponseString();

        void reset();
        void setDefaultHeaders();
        void setContentType(const std::string& content_type);
        void setContentLength(size_t length);
        
        // Static response builders
        static HttpResponse errorResponse(HttpStatus status, const std::string& message = "");
        static HttpResponse fileResponse(const std::string& filepath);
        static HttpResponse directoryListingResponse(const std::string& path, const std::string& uri);
        static HttpResponse redirectResponse(const std::string& location);
};

#endif
