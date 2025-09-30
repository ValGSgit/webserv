#ifndef UTILS_HPP
#define UTILS_HPP

#include "../webserv.hpp"

class Utils {
    public:
        // String utilities
        static std::string trim(const std::string& str);
        static std::vector<std::string> split(const std::string& str, char delimiter);
        static std::string toLowerCase(const std::string& str);
        static std::string toUpperCase(const std::string& str);
        static bool startsWith(const std::string& str, const std::string& prefix);
        static bool endsWith(const std::string& str, const std::string& suffix);
        static std::string urlDecode(const std::string& url);
        static std::string urlEncode(const std::string& str);

        // File utilities
        static bool fileExists(const std::string& filepath);
        static bool isDirectory(const std::string& path);
        static bool isReadable(const std::string& filepath);
        static bool isWritable(const std::string& filepath);
        static size_t getFileSize(const std::string& filepath);
        static std::string readFile(const std::string& filepath);
        static bool writeFile(const std::string& filepath, const std::string& content);
        static std::string getFileExtension(const std::string& filepath);
        static std::vector<std::string> listDirectory(const std::string& dirpath);

        // Path utilities
        static std::string joinPath(const std::string& path1, const std::string& path2);
        static std::string normalizePath(const std::string& path);
        static std::string getDirectory(const std::string& filepath);
        static std::string getFilename(const std::string& filepath);
        static bool isPathSecure(const std::string& path, const std::string& root);

        // HTTP utilities
        static std::string getMimeType(const std::string& filepath);
        static std::string getStatusMessage(int status_code);
        static std::string formatFileSize(size_t size);

        // Network utilities
        static void setNonBlocking(int fd);
        static std::string getClientIP(int client_fd);
        
        // Conversion utilities
        static std::string toString(int value);
        static std::string toString(size_t value);
        static int toInt(const std::string& str);
        static size_t toSizeT(const std::string& str);

        // Validation utilities
        static bool isValidHttpMethod(const std::string& method);
        static bool isValidUri(const std::string& uri);
        static bool isValidPort(int port);
};

#endif
