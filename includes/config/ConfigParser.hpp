#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "../webserv.hpp"

struct RouteConfig {
    std::vector<std::string> allowed_methods;
    std::string root_directory;
    std::string index_file;
    bool directory_listing;
    std::string upload_path;
    std::string cgi_extension;
    std::string redirect_url;
    size_t max_body_size;

    RouteConfig() : directory_listing(false), max_body_size(1048576) {}
};

struct ServerConfig {
    std::vector<int> ports;
    std::string server_name;
    std::map<std::string, RouteConfig> routes;
    size_t max_body_size;
    std::map<int, std::string> error_pages;
    std::string root;
    std::string index;
    bool autoindex;

    ServerConfig() : max_body_size(1048576), root("./www"), index("index.html"), autoindex(false) {}
};

class ConfigParser {
    private:
        std::vector<ServerConfig> _servers;
        std::string _config_file;

        std::vector<std::string> splitIntoLines(const std::string& content);
        size_t parseServerBlock(const std::vector<std::string>& lines, size_t start_index, ServerConfig& server);
        size_t parseLocationBlock(const std::vector<std::string>& lines, size_t start_index, RouteConfig& route);
        std::string trim(const std::string& str);
        std::vector<std::string> split(const std::string& str, char delimiter);
        bool isValidMethod(const std::string& method);
        bool validateBasicSyntax(const std::vector<std::string>& lines);

    public:
        ConfigParser();
        ~ConfigParser();

        bool parseConfig(const std::string& config_file);
        const std::vector<ServerConfig>& getServers() const;
        void printConfig() const;
};

#endif
