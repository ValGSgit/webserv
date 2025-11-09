#ifndef HTTP_STATUS_CODES_HPP
#define HTTP_STATUS_CODES_HPP

// RFC 7230 Section 3.2.5 - Field Limits
// These limits help prevent denial-of-service attacks
// https://medium.com/@hexadecimalapp/431-request-header-fields-too-large-51131c64b0d1
#define MAX_URI 2000
#define MAX_HEADER_SIZE 8000
#define MAX_FIELD_SIZE 4000 // that means also for cookies
// #define MAX_FILE_SIZE 10000000 // 10 MB

// RFC 7231 Section 6 - Response Status Codes
// Status codes are grouped into 5 classes:
// 1xx: Informational - Request received, continuing process
// 2xx: Success - The action was successfully received, understood, and accepted
// 3xx: Redirection - Further action must be taken in order to complete the request
// 4xx: Client Error - The request contains bad syntax or cannot be fulfilled
// 5xx: Server Error - The server failed to fulfill an apparently valid request

// 1xx Informational responses (RFC 7231 Section 6.2)
#define HTTP_CONTINUE 100                           // "Continue"
#define HTTP_SWITCHING_PROTOCOLS 101                // "Switching Protocols"
#define HTTP_PROCESSING 102                         // "Processing"
#define HTTP_EARLY_HINTS 103                        // "Early Hints"

// 2xx Success (RFC 7231 Section 6.3)
#define HTTP_OK 200                                 // "OK"
#define HTTP_CREATED 201                            // "Created"
#define HTTP_ACCEPTED 202                           // "Accepted"
#define HTTP_NON_AUTHORITATIVE_INFORMATION 203      // "Non-Authoritative Information"
#define HTTP_NO_CONTENT 204                         // "No Content"
#define HTTP_RESET_CONTENT 205                      // "Reset Content"
#define HTTP_PARTIAL_CONTENT 206                    // "Partial Content"
#define HTTP_MULTI_STATUS 207                       // "Multi-Status"
#define HTTP_ALREADY_REPORTED 208                   // "Already Reported"
#define HTTP_IM_USED 226                            // "IM Used"

// 3xx Redirection (RFC 7231 Section 6.4)
#define HTTP_MULTIPLE_CHOICES 300                   // "Multiple Choices"
#define HTTP_MOVED_PERMANENTLY 301                  // "Moved Permanently"
#define HTTP_FOUND 302                              // "Found"
#define HTTP_SEE_OTHER 303                          // "See Other"
#define HTTP_NOT_MODIFIED 304                       // "Not Modified"
#define HTTP_USE_PROXY 305                          // "Use Proxy" (Deprecated)
#define HTTP_TEMPORARY_REDIRECT 307                 // "Temporary Redirect"
#define HTTP_PERMANENT_REDIRECT 308                 // "Permanent Redirect"

// 4xx Client Error (RFC 7231 Section 6.5)
#define HTTP_BAD_REQUEST 400                        // "Bad Request"
#define HTTP_UNAUTHORIZED 401                       // "Unauthorized"
#define HTTP_PAYMENT_REQUIRED 402                   // "Payment Required"
#define HTTP_FORBIDDEN 403                          // "Forbidden"
#define HTTP_NOT_FOUND 404                          // "Not Found"
#define HTTP_METHOD_NOT_ALLOWED 405                 // "Method Not Allowed"
#define HTTP_NOT_ACCEPTABLE 406                     // "Not Acceptable"
#define HTTP_PROXY_AUTHENTICATION_REQUIRED 407      // "Proxy Authentication Required"
#define HTTP_REQUEST_TIMEOUT 408                    // "Request Timeout"
#define HTTP_CONFLICT 409                           // "Conflict"
#define HTTP_GONE 410                               // "Gone"
#define HTTP_LENGTH_REQUIRED 411                    // "Length Required"
#define HTTP_PRECONDITION_FAILED 412                // "Precondition Failed"
#define HTTP_PAYLOAD_TOO_LARGE 413                  // "Payload Too Large"
#define HTTP_URI_TOO_LONG 414                       // "URI Too Long"
#define HTTP_UNSUPPORTED_MEDIA_TYPE 415             // "Unsupported Media Type"
#define HTTP_RANGE_NOT_SATISFIABLE 416              // "Range Not Satisfiable"
#define HTTP_EXPECTATION_FAILED 417                 // "Expectation Failed"
#define HTTP_IM_A_TEAPOT 418                        // "I'm a teapot"
#define HTTP_MISDIRECTED_REQUEST 421                // "Misdirected Request"
#define HTTP_UNPROCESSABLE_ENTITY 422               // "Unprocessable Entity"
#define HTTP_LOCKED 423                             // "Locked"
#define HTTP_FAILED_DEPENDENCY 424                  // "Failed Dependency"
#define HTTP_TOO_EARLY 425                          // "Too Early"
#define HTTP_UPGRADE_REQUIRED 426                   // "Upgrade Required"
#define HTTP_PRECONDITION_REQUIRED 428              // "Precondition Required"
#define HTTP_TOO_MANY_REQUESTS 429                  // "Too Many Requests"
#define HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE 431    // "Request Header Fields Too Large"
#define HTTP_UNAVAILABLE_FOR_LEGAL_REASONS 451      // "Unavailable For Legal Reasons"

// 5xx Server Error (RFC 7231 Section 6.6)
#define HTTP_INTERNAL_SERVER_ERROR 500              // "Internal Server Error"
#define HTTP_NOT_IMPLEMENTED 501                    // "Not Implemented"
#define HTTP_BAD_GATEWAY 502                        // "Bad Gateway"
#define HTTP_SERVICE_UNAVAILABLE 503                // "Service Unavailable"
#define HTTP_GATEWAY_TIMEOUT 504                    // "Gateway Timeout"
#define HTTP_HTTP_VERSION_NOT_SUPPORTED 505         // "HTTP Version Not Supported"
#define HTTP_VARIANT_ALSO_NEGOTIATES 506            // "Variant Also Negotiates"
#define HTTP_INSUFFICIENT_STORAGE 507               // "Insufficient Storage"
#define HTTP_LOOP_DETECTED 508                      // "Loop Detected"
#define HTTP_NOT_EXTENDED 510                       // "Not Extended"
#define HTTP_NETWORK_AUTHENTICATION_REQUIRED 511    // "Network Authentication Required"

#endif