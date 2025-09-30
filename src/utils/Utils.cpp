#include "../../includes/utils/Utils.hpp"

std::string Utils::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> Utils::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == delimiter) {
            token = trim(token);
            if (!token.empty()) {
                tokens.push_back(token);
            }
            token.clear();
        } else {
            token += str[i];
        }
    }
    
    token = trim(token);
    if (!token.empty()) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string Utils::toLowerCase(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.length(); ++i) {
        if (result[i] >= 'A' && result[i] <= 'Z') {
            result[i] = result[i] + ('a' - 'A');
        }
    }
    return result;
}

std::string Utils::toUpperCase(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.length(); ++i) {
        if (result[i] >= 'a' && result[i] <= 'z') {
            result[i] = result[i] - ('a' - 'A');
        }
    }
    return result;
}

bool Utils::startsWith(const std::string& str, const std::string& prefix) {
    if (prefix.length() > str.length()) return false;
    return str.substr(0, prefix.length()) == prefix;
}

bool Utils::endsWith(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) return false;
    return str.substr(str.length() - suffix.length()) == suffix;
}

std::string Utils::urlDecode(const std::string& url) {
    std::string result;
    for (size_t i = 0; i < url.length(); ++i) {
        if (url[i] == '%' && i + 2 < url.length()) {
            std::string hex = url.substr(i + 1, 2);
            // Manual hex to int conversion
            int value = 0;
            for (size_t j = 0; j < hex.length(); ++j) {
                char c = hex[j];
                value *= 16;
                if (c >= '0' && c <= '9') {
                    value += c - '0';
                } else if (c >= 'A' && c <= 'F') {
                    value += c - 'A' + 10;
                } else if (c >= 'a' && c <= 'f') {
                    value += c - 'a' + 10;
                }
            }
            result += static_cast<char>(value);
            i += 2;
        } else if (url[i] == '+') {
            result += ' ';
        } else {
            result += url[i];
        }
    }
    return result;
}

std::string Utils::urlEncode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        char c = str[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            result += c;
        } else {
            result += '%';
            result += "0123456789ABCDEF"[c >> 4];
            result += "0123456789ABCDEF"[c & 15];
        }
    }
    return result;
}

bool Utils::fileExists(const std::string& filepath) {
    struct stat st;
    return stat(filepath.c_str(), &st) == 0;
}

bool Utils::isDirectory(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

bool Utils::isReadable(const std::string& filepath) {
    return access(filepath.c_str(), R_OK) == 0;
}

bool Utils::isWritable(const std::string& filepath) {
    return access(filepath.c_str(), W_OK) == 0;
}

size_t Utils::getFileSize(const std::string& filepath) {
    struct stat st;
    if (stat(filepath.c_str(), &st) != 0) return 0;
    return st.st_size;
}

std::string Utils::readFile(const std::string& filepath) {
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd == -1) return "";
    
    std::string content;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        content.append(buffer, bytes_read);
    }
    
    close(fd);
    return content;
}

bool Utils::writeFile(const std::string& filepath, const std::string& content) {
    int fd = open(filepath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) return false;
    
    ssize_t bytes_written = write(fd, content.c_str(), content.length());
    close(fd);
    
    return bytes_written == static_cast<ssize_t>(content.length());
}

std::string Utils::getFileExtension(const std::string& filepath) {
    size_t dot_pos = filepath.find_last_of('.');
    if (dot_pos == std::string::npos) return "";
    return toLowerCase(filepath.substr(dot_pos));
}

std::vector<std::string> Utils::listDirectory(const std::string& dirpath) {
    std::vector<std::string> files;
    DIR* dir = opendir(dirpath.c_str());
    if (!dir) return files;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name != "." && name != "..") {
            files.push_back(name);
        }
    }
    closedir(dir);
    
    // Manual sort (bubble sort for C++98 compatibility)
    for (size_t i = 0; i < files.size(); ++i) {
        for (size_t j = 0; j < files.size() - 1 - i; ++j) {
            if (files[j] > files[j + 1]) {
                std::string temp = files[j];
                files[j] = files[j + 1];
                files[j + 1] = temp;
            }
        }
    }
    return files;
}

std::string Utils::joinPath(const std::string& path1, const std::string& path2) {
    if (path1.empty()) return path2;
    if (path2.empty()) return path1;
    
    std::string result = path1;
    if (result[result.length() - 1] != '/' && path2[0] != '/') {
        result += "/";
    }
    result += path2;
    return result;
}

std::string Utils::normalizePath(const std::string& path) {
    std::vector<std::string> components = split(path, '/');
    std::vector<std::string> normalized;
    
    for (size_t i = 0; i < components.size(); ++i) {
        const std::string& component = components[i];
        if (component == "." || component.empty()) {
            continue;
        } else if (component == "..") {
            if (!normalized.empty() && normalized.back() != "..") {
                normalized.pop_back();
            } else if (path[0] != '/') {
                normalized.push_back("..");
            }
        } else {
            normalized.push_back(component);
        }
    }
    
    std::string result;
    if (path[0] == '/') result = "/";
    
    for (size_t i = 0; i < normalized.size(); ++i) {
        if (i > 0) result += "/";
        result += normalized[i];
    }
    
    return result.empty() ? "/" : result;
}

std::string Utils::getDirectory(const std::string& filepath) {
    size_t slash_pos = filepath.find_last_of('/');
    if (slash_pos == std::string::npos) return ".";
    return filepath.substr(0, slash_pos);
}

std::string Utils::getFilename(const std::string& filepath) {
    size_t slash_pos = filepath.find_last_of('/');
    if (slash_pos == std::string::npos) return filepath;
    return filepath.substr(slash_pos + 1);
}

bool Utils::isPathSecure(const std::string& path, const std::string& root) {
    std::string normalized_path = normalizePath(path);
    std::string normalized_root = normalizePath(root);
    return startsWith(normalized_path, normalized_root);
}

std::string Utils::getMimeType(const std::string& filepath) {
    std::string ext = getFileExtension(filepath);
    
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".xml") return "application/xml";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".txt") return "text/plain";
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".zip") return "application/zip";
    
    return "application/octet-stream";
}

std::string Utils::getStatusMessage(int status_code) {
    switch (status_code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default: return "Unknown";
    }
}
std::string Utils::formatFileSize(size_t size) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double dsize = static_cast<double>(size);
    
    while (dsize >= 1024 && unit < 3) {
        dsize /= 1024;
        unit++;
    }
    
    std::string result;
    int intPart = static_cast<int>(dsize);
    int fracPart = static_cast<int>((dsize - intPart) * 10);
    
    result = toString(intPart);
    if (fracPart > 0) {
        result += ".";
        result += toString(fracPart);
    }
    result += " ";
    result += units[unit];
    
    return result;
}

void Utils::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

std::string Utils::getClientIP(int client_fd) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    if (getpeername(client_fd, (struct sockaddr*)&addr, &addr_len) == -1) {
        return "unknown";
    }
    return std::string(inet_ntoa(addr.sin_addr));
}

std::string Utils::toString(int value) {
    if (value == 0) return "0";
    
    std::string result;
    bool negative = value < 0;
    if (negative) value = -value;
    
    while (value > 0) {
        result = static_cast<char>('0' + (value % 10)) + result;
        value /= 10;
    }
    
    if (negative) result = "-" + result;
    return result;
}

std::string Utils::toString(size_t value) {
    if (value == 0) return "0";
    
    std::string result;
    while (value > 0) {
        result = static_cast<char>('0' + (value % 10)) + result;
        value /= 10;
    }
    return result;
}

int Utils::toInt(const std::string& str) {
    if (str.empty()) return 0;
    
    int result = 0;
    bool negative = false;
    size_t i = 0;
    
    if (str[0] == '-') {
        negative = true;
        i = 1;
    } else if (str[0] == '+') {
        i = 1;
    }
    
    for (; i < str.length() && str[i] >= '0' && str[i] <= '9'; ++i) {
        result = result * 10 + (str[i] - '0');
    }
    
    return negative ? -result : result;
}

size_t Utils::toSizeT(const std::string& str) {
    if (str.empty()) return 0;
    
    size_t result = 0;
    for (size_t i = 0; i < str.length() && str[i] >= '0' && str[i] <= '9'; ++i) {
        result = result * 10 + (str[i] - '0');
    }
    
    return result;
}

bool Utils::isValidHttpMethod(const std::string& method) {
    return (method == "GET" || method == "POST" || method == "DELETE" ||
            method == "PUT" || method == "HEAD" || method == "OPTIONS");
}

bool Utils::isValidUri(const std::string& uri) {
    if (uri.empty() || uri[0] != '/') return false;
    // Additional URI validation can be added here
    return true;
}

bool Utils::isValidPort(int port) {
    return port > 0 && port <= 65535;
}
