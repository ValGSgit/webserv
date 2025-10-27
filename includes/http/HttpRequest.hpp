#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include "../webserv.hpp"
#include "HttpStatusCodes.hpp"

class HttpRequest {
    private:
        int _status;
        HttpMethod _method;
        std::string _uri;
        std::string _version;
        std::map<std::string, std::string> _headers;
        std::string _body;
        std::string _query_string;
        std::map<std::string, std::string> _params;
        std::map<std::string, std::string> _cookies;
        bool _headers_complete;
        bool _body_complete;
        size_t _content_length;
        bool _chunked;

        std::vector<std::string> splitIntoLines(const std::string& content);
        void parseRequestLine(const std::string& line);
        void parseHeader(const std::string& line);
        void parseUri(const std::string& uri);
        HttpMethod stringToMethod(const std::string& method_str);
        void parseQueryString(const std::string& query);
        void parseCookies();

    public:
        HttpRequest();
        ~HttpRequest();

        bool parseRequest(const std::string& data);
        void reset();

        // Getters
        HttpMethod getMethod() const;
        const std::string& getUri() const;
        const std::string& getVersion() const;
        const std::map<std::string, std::string>& getHeaders() const;
        const std::string& getBody() const;
        const std::string& getQueryString() const;
        const std::map<std::string, std::string>& getParams() const;
        bool isHeadersComplete() const;
        bool isBodyComplete() const;
        size_t getContentLength() const;
        int getStatus() const;
        bool isChunked() const;

        std::string getHeader(const std::string& key) const;
        std::string methodToString() const;
        void print() const;

        std::string getCookie(const std::string& name) const;
        const std::map<std::string, std::string>& getCookies() const;
};

#endif
