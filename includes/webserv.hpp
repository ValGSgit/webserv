#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <csignal>

// System includes
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <dirent.h>
#include <errno.h>

// Constants
#define BUFFER_SIZE 8192
#define MAX_CONNECTIONS 1024
#define CONNECTION_TIMEOUT 60
#define CGI_TIMEOUT 30

// HTTP Methods
enum HttpMethod {
    METHOD_GET,
    METHOD_POST,
    METHOD_DELETE,
    METHOD_UNKNOWN
};

// HTTP Status Codes
enum HttpStatus {
    STATUS_OK = 200,
    STATUS_CREATED = 201,
    STATUS_NO_CONTENT = 204,
    STATUS_MOVED_PERMANENTLY = 301,
    STATUS_FOUND = 302,
    STATUS_BAD_REQUEST = 400,
    STATUS_FORBIDDEN = 403,
    STATUS_NOT_FOUND = 404,
    STATUS_METHOD_NOT_ALLOWED = 405,
    STATUS_REQUEST_TIMEOUT = 408,
    STATUS_PAYLOAD_TOO_LARGE = 413,
    STATUS_INTERNAL_SERVER_ERROR = 500,
    STATUS_NOT_IMPLEMENTED = 501,
    STATUS_BAD_GATEWAY = 502,
    STATUS_SERVICE_UNAVAILABLE = 503
};

// Connection States
enum ConnectionState {
    STATE_READING_HEADERS,
    STATE_READING_BODY,
    STATE_PROCESSING,
    STATE_WRITING_RESPONSE,
    STATE_DONE,
    STATE_ERROR
};

#endif
