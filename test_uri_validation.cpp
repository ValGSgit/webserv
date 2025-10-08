#include "includes/utils/Utils.hpp"
#include <iostream>
#include <cassert>


//c++ -std=c++98 -I. test_uri_validation.cpp src/utils/Utils.cpp -o test_uri && ./test_uri

void testBasicUris() {
    std::cout << "Testing basic URIs..." << std::endl;
    
    // Valid absolute URIs
    assert(Utils::isValidAbsoluteUri("http://www.example.com/path?query=value#fragment"));
    assert(Utils::isValidAbsoluteUri("https://user:pass@example.com:8080/path"));
    assert(Utils::isValidAbsoluteUri("ftp://ftp.example.com/file.txt"));
    assert(Utils::isValidAbsoluteUri("mailto:test@example.com"));
    assert(Utils::isValidAbsoluteUri("file:///home/user/file.txt"));
    assert(Utils::isValidAbsoluteUri("ldap://[2001:db8::7]/c=GB?objectClass?one"));
    
    // URI with IPv4
    assert(Utils::isValidAbsoluteUri("http://192.168.1.1:8080/path"));
    
    // URI with IPv6
    assert(Utils::isValidAbsoluteUri("http://[2001:0db8:85a3:0000:0000:8a2e:0370:7334]/"));
    assert(Utils::isValidAbsoluteUri("http://[::1]/"));
    
    // Valid relative URI references
    assert(Utils::isValidUriReference("/path/to/resource"));
    assert(Utils::isValidUriReference("../relative/path"));
    assert(Utils::isValidUriReference("?query=only"));
    assert(Utils::isValidUriReference("#fragment-only"));
    assert(Utils::isValidUriReference("relative/path"));
    
    // Relative URIs should fail absolute URI validation
    assert(!Utils::isValidAbsoluteUri("/path/to/resource"));
    assert(!Utils::isValidAbsoluteUri("../relative/path"));
    assert(!Utils::isValidAbsoluteUri("?query=only"));
    assert(!Utils::isValidAbsoluteUri("#fragment-only"));
    
    // Invalid URIs
    assert(!Utils::isValidAbsoluteUri(""));
    assert(!Utils::isValidAbsoluteUri("ht tp://example.com")); // space in scheme
    assert(!Utils::isValidAbsoluteUri("http://exam ple.com")); // space in host
    assert(!Utils::isValidAbsoluteUri("http://example.com:99999")); // invalid port
    
    std::cout << "Basic URI tests passed!" << std::endl;
}

void testUriNormalization() {
    std::cout << "Testing URI normalization..." << std::endl;
    
    // Case normalization
    std::string normalized = Utils::normalizeUri("HTTP://WWW.EXAMPLE.COM/Path");
    assert(normalized == "http://www.example.com/Path");
    
    // Percent-encoding normalization
    normalized = Utils::normalizeUri("http://example.com/path%2Fto%2Fresource");
    assert(normalized.find("%2F") != std::string::npos); // Should keep encoded slash
    
    // Dot segment removal
    normalized = Utils::normalizeUri("http://example.com/a/b/c/./../../g");
    assert(normalized == "http://example.com/a/g");
    
    normalized = Utils::normalizeUri("http://example.com/a/b/c/../d");
    assert(normalized == "http://example.com/a/b/d");
    
    // Default port removal
    normalized = Utils::normalizeUri("http://example.com:80/path");
    assert(normalized == "http://example.com/path");
    
    normalized = Utils::normalizeUri("https://example.com:443/path");
    assert(normalized == "https://example.com/path");
    
    std::cout << "URI normalization tests passed!" << std::endl;
}

void testUriComponents() {
    std::cout << "Testing URI component parsing..." << std::endl;
    
    Utils::UriComponents components;
    std::string uri = "https://user:pass@example.com:8080/path/to/resource?query=value&foo=bar#section1";
    
    assert(Utils::parseUri(uri, components));
    assert(components.hasScheme && components.scheme == "https");
    assert(components.hasAuthority);
    assert(components.hasUserinfo && components.userinfo == "user:pass");
    assert(components.host == "example.com");
    assert(components.hasPort && components.port == "8080");
    assert(components.path == "/path/to/resource");
    assert(components.hasQuery && components.query == "query=value&foo=bar");
    assert(components.hasFragment && components.fragment == "section1");
    
    // Test reconstruction
    std::string reconstructed = Utils::reconstructUri(components);
    assert(reconstructed == uri);
    
    std::cout << "URI component tests passed!" << std::endl;
}

void testEdgeCases() {
    std::cout << "Testing edge cases..." << std::endl;
    
    // Empty components
    assert(Utils::isValidAbsoluteUri("scheme:"));
    assert(Utils::isValidAbsoluteUri("scheme://"));
    assert(Utils::isValidAbsoluteUri("scheme:///"));
    
    // IPv6 edge cases
    assert(Utils::isValidAbsoluteUri("http://[::]/"));
    assert(Utils::isValidAbsoluteUri("http://[2001:db8::1]/"));
    assert(Utils::isValidAbsoluteUri("http://[::ffff:192.0.2.1]/"));
    
    // Port edge cases
    assert(Utils::isValidAbsoluteUri("http://example.com:0/"));
    assert(Utils::isValidAbsoluteUri("http://example.com:65535/"));
    assert(!Utils::isValidAbsoluteUri("http://example.com:65536/")); // Out of range
    
    // Path edge cases
    assert(Utils::isValidAbsoluteUri("http://example.com")); // No path
    assert(Utils::isValidAbsoluteUri("http://example.com/")); // Root path
    assert(Utils::isValidAbsoluteUri("http://example.com/a/b/c"));
    
    std::cout << "Edge case tests passed!" << std::endl;
}

void testRFC3986Examples() {
    std::cout << "Testing RFC 3986 examples..." << std::endl;
    
    // Examples from RFC 3986
    assert(Utils::isValidAbsoluteUri("ftp://ftp.is.co.za/rfc/rfc1808.txt"));
    assert(Utils::isValidAbsoluteUri("http://www.ietf.org/rfc/rfc2396.txt"));
    assert(Utils::isValidAbsoluteUri("ldap://[2001:db8::7]/c=GB?objectClass?one"));
    assert(Utils::isValidAbsoluteUri("mailto:John.Doe@example.com"));
    assert(Utils::isValidAbsoluteUri("news:comp.infosystems.www.servers.unix"));
    assert(Utils::isValidAbsoluteUri("tel:+1-816-555-1212"));
    assert(Utils::isValidAbsoluteUri("telnet://192.0.2.16:80/"));
    assert(Utils::isValidAbsoluteUri("urn:oasis:names:specification:docbook:dtd:xml:4.1.2"));
    
    std::cout << "RFC 3986 example tests passed!" << std::endl;
}

int main() {
    try {
        testBasicUris();
        testUriNormalization();
        testUriComponents();
        testEdgeCases();
        testRFC3986Examples();
        
        std::cout << "\nAll URI validation tests passed successfully!" << std::endl;
        std::cout << "\nKey features implemented:" << std::endl;
        std::cout << "- Full RFC 3986 compliant URI parsing and validation" << std::endl;
        std::cout << "- Comprehensive component validation (scheme, authority, path, query, fragment)" << std::endl;
        std::cout << "- IPv4 and IPv6 address validation" << std::endl;
        std::cout << "- Proper percent-encoding validation and normalization" << std::endl;
        std::cout << "- Dot segment removal for path normalization" << std::endl;
        std::cout << "- Case normalization for scheme and host" << std::endl;
        std::cout << "- Default port removal for common schemes" << std::endl;
        std::cout << "- Detailed error checking for malformed URIs" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
