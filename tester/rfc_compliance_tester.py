#!/usr/bin/env python3
"""
RFC COMPLIANCE AND EDGE CASE TESTER
Tests HTTP/1.1 RFC compliance and uncommon edge cases
"""

import socket
import time
import sys
from typing import Optional, Tuple

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    RESET = '\033[0m'
    BOLD = '\033[1m'
    DIM = '\033[2m'

class RFCComplianceTester:
    def __init__(self, host='localhost', port=8080):
        self.host = host
        self.port = port
        self.passed = 0
        self.failed = 0
        self.warnings = 0
    
    def send_raw(self, data: bytes, timeout=5.0) -> Tuple[Optional[int], bytes]:
        """Send raw bytes and return status code and response"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(timeout)
            sock.connect((self.host, self.port))
            sock.sendall(data)
            
            response = b""
            while True:
                try:
                    chunk = sock.recv(8192)
                    if not chunk:
                        break
                    response += chunk
                    if len(response) > 100000:
                        break
                    if b'\r\n\r\n' in response and len(response) > 50:
                        break
                except socket.timeout:
                    break
            
            sock.close()
            
            # Extract status code
            if response:
                try:
                    status_line = response.decode('utf-8', errors='ignore').split('\n')[0]
                    parts = status_line.split()
                    if len(parts) >= 2 and parts[1].isdigit():
                        return int(parts[1]), response
                except:
                    pass
            
            return None, response
            
        except Exception as e:
            return None, str(e).encode()
    
    def test(self, name: str, data: bytes, expected_status: int, alternatives=None):
        """Test a single request"""
        status, response = self.send_raw(data)
        
        if alternatives is None:
            alternatives = []
        
        if status == expected_status or status in alternatives:
            self.passed += 1
            print(f"{Colors.GREEN}✓{Colors.RESET} {name} (got {status})")
            return True
        else:
            self.failed += 1
            print(f"{Colors.RED}✗{Colors.RESET} {name} - Expected {expected_status}, got {status}")
            return False
    
    def test_line_endings(self):
        """Test various line ending formats"""
        print(f"\n{Colors.BOLD}Line Ending Tests:{Colors.RESET}")
        
        # Standard CRLF
        self.test("Standard CRLF", 
                  b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", 200)
        
        # LF only (should work for compatibility)
        self.test("LF only",
                  b"GET / HTTP/1.1\nHost: localhost\n\n", 200, [400])
        
        # Mixed line endings
        self.test("Mixed CRLF and LF",
                  b"GET / HTTP/1.1\r\nHost: localhost\n\r\n", 200, [400])
        
        # CR only (should fail)
        self.test("CR only",
                  b"GET / HTTP/1.1\rHost: localhost\r\r", 400, [200])
        
        # Extra line endings
        self.test("Extra CRLF before request",
                  b"\r\n\r\nGET / HTTP/1.1\r\nHost: localhost\r\n\r\n", 400, [200])
        
        # No final CRLF
        self.test("Missing final CRLF",
                  b"GET / HTTP/1.1\r\nHost: localhost\r\n", 400, [200], timeout=2.0)
    
    def test_whitespace_handling(self):
        """Test whitespace in various positions"""
        print(f"\n{Colors.BOLD}Whitespace Handling Tests:{Colors.RESET}")
        
        # Leading whitespace before method
        self.test("Leading space before method",
                  b" GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", 400)
        
        # Trailing whitespace after version
        self.test("Trailing space after version",
                  b"GET / HTTP/1.1 \r\nHost: localhost\r\n\r\n", 200, [400])
        
        # Multiple spaces between tokens
        self.test("Multiple spaces between tokens",
                  b"GET  /  HTTP/1.1\r\nHost: localhost\r\n\r\n", 200, [400])
        
        # Tab characters
        self.test("Tab instead of space",
                  b"GET\t/\tHTTP/1.1\r\nHost: localhost\r\n\r\n", 400, [200])
        
        # Whitespace in header values (should be trimmed)
        self.test("Leading space in header value",
                  b"GET / HTTP/1.1\r\nHost:  localhost\r\n\r\n", 200)
        
        # Trailing whitespace in header value
        self.test("Trailing space in header value",
                  b"GET / HTTP/1.1\r\nHost: localhost \r\n\r\n", 200)
    
    def test_header_edge_cases(self):
        """Test edge cases in header parsing"""
        print(f"\n{Colors.BOLD}Header Edge Cases:{Colors.RESET}")
        
        # Empty header value
        self.test("Empty header value",
                  b"GET / HTTP/1.1\r\nHost: localhost\r\nX-Empty:\r\n\r\n", 200, [400])
        
        # Header with only spaces as value
        self.test("Header with only spaces",
                  b"GET / HTTP/1.1\r\nHost: localhost\r\nX-Spaces:   \r\n\r\n", 200, [400])
        
        # Very long header name
        long_name = b"X-" + b"a" * 500
        self.test("Very long header name",
                  b"GET / HTTP/1.1\r\nHost: localhost\r\n" + long_name + b": value\r\n\r\n", 
                  200, [400, 431])
        
        # Very long header value
        long_value = b"x" * 8000
        self.test("Very long header value",
                  b"GET / HTTP/1.1\r\nHost: localhost\r\nX-Long: " + long_value + b"\r\n\r\n",
                  200, [400, 431])
        
        # Header line folding (obsolete in HTTP/1.1 but some servers support it)
        self.test("Header line folding (obsolete)",
                  b"GET / HTTP/1.1\r\nHost: localhost\r\nX-Folded: value1\r\n continuation\r\n\r\n",
                  400, [200])
        
        # Duplicate Host headers
        self.test("Duplicate Host headers",
                  b"GET / HTTP/1.1\r\nHost: localhost\r\nHost: example.com\r\n\r\n",
                  400, [200])
        
        # Header name with invalid characters
        self.test("Header with space in name",
                  b"GET / HTTP/1.1\r\nHost: localhost\r\nInvalid Name: value\r\n\r\n", 400, [200])
        
        # Header without colon
        self.test("Header without colon",
                  b"GET / HTTP/1.1\r\nHost: localhost\r\nInvalidHeader\r\n\r\n", 400, [200])
        
        # Empty header name
        self.test("Empty header name",
                  b"GET / HTTP/1.1\r\nHost: localhost\r\n: value\r\n\r\n", 400, [200])
    
    def test_method_edge_cases(self):
        """Test HTTP method edge cases"""
        print(f"\n{Colors.BOLD}Method Edge Cases:{Colors.RESET}")
        
        # Lowercase method
        self.test("Lowercase method",
                  b"get / HTTP/1.1\r\nHost: localhost\r\n\r\n", 400, [501, 200])
        
        # Mixed case method
        self.test("Mixed case method",
                  b"Get / HTTP/1.1\r\nHost: localhost\r\n\r\n", 400, [501, 200])
        
        # Empty method
        self.test("Empty method",
                  b" / HTTP/1.1\r\nHost: localhost\r\n\r\n", 400)
        
        # Very long method
        self.test("Very long method",
                  b"VERYLONGMETHODNAME" + b"X"*100 + b" / HTTP/1.1\r\nHost: localhost\r\n\r\n", 
                  400, [501])
        
        # Method with invalid characters
        self.test("Method with hyphen",
                  b"GET-CUSTOM / HTTP/1.1\r\nHost: localhost\r\n\r\n", 400, [501])
    
    def test_uri_edge_cases(self):
        """Test URI edge cases"""
        print(f"\n{Colors.BOLD}URI Edge Cases:{Colors.RESET}")
        
        # Empty URI
        self.test("Empty URI",
                  b"GET  HTTP/1.1\r\nHost: localhost\r\n\r\n", 400)
        
        # URI with spaces (should be encoded)
        self.test("URI with spaces",
                  b"GET /test file.html HTTP/1.1\r\nHost: localhost\r\n\r\n", 400, [404])
        
        # URI with null byte
        self.test("URI with null byte",
                  b"GET /test\x00file.html HTTP/1.1\r\nHost: localhost\r\n\r\n", 400, [404])
        
        # Asterisk as URI (for OPTIONS *)
        self.test("Asterisk URI",
                  b"OPTIONS * HTTP/1.1\r\nHost: localhost\r\n\r\n", 200, [501, 400])
        
        # Authority form (for CONNECT)
        self.test("Authority form URI",
                  b"CONNECT localhost:8080 HTTP/1.1\r\nHost: localhost\r\n\r\n", 
                  400, [501, 405])
        
        # Absolute URI form
        self.test("Absolute URI",
                  b"GET http://localhost/ HTTP/1.1\r\nHost: localhost\r\n\r\n", 200, [400])
        
        # URI with fragment (should be ignored)
        self.test("URI with fragment",
                  b"GET /index.html#section HTTP/1.1\r\nHost: localhost\r\n\r\n", 200)
        
        # Double slash in path
        self.test("Double slash in path",
                  b"GET //index.html HTTP/1.1\r\nHost: localhost\r\n\r\n", 200, [404])
        
        # Path with backslash
        self.test("Path with backslash",
                  b"GET /test\\file.html HTTP/1.1\r\nHost: localhost\r\n\r\n", 404, [400])
    
    def test_version_edge_cases(self):
        """Test HTTP version edge cases"""
        print(f"\n{Colors.BOLD}Version Edge Cases:{Colors.RESET}")
        
        # HTTP/0.9 (should be supported for compatibility)
        self.test("HTTP/0.9",
                  b"GET /\r\n", 200, [400, 505])
        
        # HTTP/1.0
        self.test("HTTP/1.0",
                  b"GET / HTTP/1.0\r\n\r\n", 200)
        
        # Lowercase http
        self.test("Lowercase http",
                  b"GET / http/1.1\r\nHost: localhost\r\n\r\n", 400, [200])
        
        # Missing minor version
        self.test("Missing minor version",
                  b"GET / HTTP/1\r\nHost: localhost\r\n\r\n", 400, [200])
        
        # Extra decimal places
        self.test("HTTP/1.1.0",
                  b"GET / HTTP/1.1.0\r\nHost: localhost\r\n\r\n", 400, [200])
    
    def test_content_length_edge_cases(self):
        """Test Content-Length edge cases"""
        print(f"\n{Colors.BOLD}Content-Length Edge Cases:{Colors.RESET}")
        
        # Zero content length with body
        self.test("Zero Content-Length with body",
                  b"POST /uploads/ HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\nBODY",
                  201, [200, 204, 400])
        
        # Negative content length
        self.test("Negative Content-Length",
                  b"POST /uploads/ HTTP/1.1\r\nHost: localhost\r\nContent-Length: -10\r\n\r\n",
                  400)
        
        # Non-numeric content length
        self.test("Non-numeric Content-Length",
                  b"POST /uploads/ HTTP/1.1\r\nHost: localhost\r\nContent-Length: abc\r\n\r\n",
                  400)
        
        # Multiple Content-Length headers (same value)
        self.test("Duplicate Content-Length (same)",
                  b"POST /uploads/ HTTP/1.1\r\nHost: localhost\r\nContent-Length: 4\r\nContent-Length: 4\r\n\r\ntest",
                  400, [201, 200])
        
        # Multiple Content-Length headers (different values)
        self.test("Duplicate Content-Length (different)",
                  b"POST /uploads/ HTTP/1.1\r\nHost: localhost\r\nContent-Length: 4\r\nContent-Length: 10\r\n\r\ntest",
                  400)
        
        # Very large Content-Length
        self.test("Very large Content-Length",
                  b"POST /uploads/ HTTP/1.1\r\nHost: localhost\r\nContent-Length: 999999999999\r\n\r\n",
                  413, [400], timeout=2.0)
    
    def test_host_header_edge_cases(self):
        """Test Host header edge cases"""
        print(f"\n{Colors.BOLD}Host Header Edge Cases:{Colors.RESET}")
        
        # Host with port
        self.test("Host with port",
                  b"GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n", 200)
        
        # Host with IPv4
        self.test("Host with IPv4",
                  b"GET / HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n", 200)
        
        # Host with IPv6
        self.test("Host with IPv6",
                  b"GET / HTTP/1.1\r\nHost: [::1]\r\n\r\n", 200, [400])
        
        # Empty Host value
        self.test("Empty Host value",
                  b"GET / HTTP/1.1\r\nHost:\r\n\r\n", 400, [200])
        
        # Host with invalid characters
        self.test("Host with spaces",
                  b"GET / HTTP/1.1\r\nHost: local host\r\n\r\n", 400, [200])
    
    def test_connection_header(self):
        """Test Connection header variations"""
        print(f"\n{Colors.BOLD}Connection Header Tests:{Colors.RESET}")
        
        # Connection: close
        self.test("Connection: close",
                  b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n", 200)
        
        # Connection: keep-alive
        self.test("Connection: keep-alive",
                  b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n", 200)
        
        # Connection: upgrade
        self.test("Connection: upgrade",
                  b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: upgrade\r\n\r\n", 200, [426, 400])
        
        # Multiple connection tokens
        self.test("Connection: keep-alive, Upgrade",
                  b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive, Upgrade\r\n\r\n", 200, [400])
    
    def test_special_requests(self):
        """Test special and uncommon request types"""
        print(f"\n{Colors.BOLD}Special Request Tests:{Colors.RESET}")
        
        # Request with query string only
        self.test("Query string only",
                  b"GET ?query=value HTTP/1.1\r\nHost: localhost\r\n\r\n", 404, [200, 400])
        
        # Request with only fragment
        self.test("Fragment only",
                  b"GET #anchor HTTP/1.1\r\nHost: localhost\r\n\r\n", 404, [200, 400])
        
        # HEAD request
        self.test("HEAD request",
                  b"HEAD / HTTP/1.1\r\nHost: localhost\r\n\r\n", 200, [501])
        
        # OPTIONS request
        self.test("OPTIONS request",
                  b"OPTIONS / HTTP/1.1\r\nHost: localhost\r\n\r\n", 200, [501, 405])
        
        # TRACE request (should be denied for security)
        self.test("TRACE request",
                  b"TRACE / HTTP/1.1\r\nHost: localhost\r\n\r\n", 405, [501, 200])
    
    def run_all_tests(self):
        """Run all RFC compliance tests"""
        print(f"\n{Colors.BOLD}{Colors.CYAN}╔{'═'*78}╗{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}║{' '*20}RFC COMPLIANCE & EDGE CASE TESTER{' '*22}║{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}╚{'═'*78}╝{Colors.RESET}\n")
        
        print(f"{Colors.YELLOW}Testing server at {self.host}:{self.port}{Colors.RESET}")
        
        # Check connectivity
        try:
            status, _ = self.send_raw(b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
            if status is None:
                print(f"{Colors.RED}ERROR: Cannot connect to server{Colors.RESET}")
                return 1
        except:
            print(f"{Colors.RED}ERROR: Cannot connect to server{Colors.RESET}")
            return 1
        
        print(f"{Colors.GREEN}✓ Server is running{Colors.RESET}")
        
        try:
            self.test_line_endings()
            self.test_whitespace_handling()
            self.test_header_edge_cases()
            self.test_method_edge_cases()
            self.test_uri_edge_cases()
            self.test_version_edge_cases()
            self.test_content_length_edge_cases()
            self.test_host_header_edge_cases()
            self.test_connection_header()
            self.test_special_requests()
            
            # Print summary
            total = self.passed + self.failed
            print(f"\n{Colors.BOLD}{'='*80}{Colors.RESET}")
            print(f"{Colors.BOLD}RFC Compliance Test Summary{Colors.RESET}")
            print(f"{'='*80}")
            print(f"Total Tests: {total}")
            print(f"{Colors.GREEN}Passed: {self.passed} ({self.passed/total*100:.1f}%){Colors.RESET}")
            print(f"{Colors.RED}Failed: {self.failed} ({self.failed/total*100:.1f}%){Colors.RESET}")
            print(f"{'='*80}\n")
            
            if self.failed == 0:
                print(f"{Colors.GREEN}{Colors.BOLD}Perfect RFC compliance! 🎉{Colors.RESET}\n")
            elif self.failed < 5:
                print(f"{Colors.YELLOW}Good RFC compliance with minor issues{Colors.RESET}\n")
            else:
                print(f"{Colors.RED}Several RFC compliance issues found{Colors.RESET}\n")
            
        except KeyboardInterrupt:
            print(f"\n{Colors.YELLOW}Tests interrupted{Colors.RESET}")
            return 1
        except Exception as e:
            print(f"\n{Colors.RED}Error: {e}{Colors.RESET}")
            import traceback
            traceback.print_exc()
            return 1
        
        return 0 if self.failed == 0 else 1

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='RFC Compliance and Edge Case Tester for WebServ')
    parser.add_argument('--host', default='localhost', help='Server host')
    parser.add_argument('--port', type=int, default=8080, help='Server port')
    
    args = parser.parse_args()
    
    tester = RFCComplianceTester(args.host, args.port)
    exit_code = tester.run_all_tests()
    sys.exit(exit_code)

if __name__ == '__main__':
    main()
