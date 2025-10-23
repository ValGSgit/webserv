#!/bin/bash

# Siege Testing Script for WebServ
# This script runs various siege benchmarks on your HTTP server

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SERVER_HOST="localhost"
SERVER_PORT="8080"
BASE_URL="http://${SERVER_HOST}:${SERVER_PORT}"
URLS_FILE="siege_urls.txt"

# Print colored output
print_header() {
    echo -e "\n${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${GREEN}$1${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"
}

print_info() {
    echo -e "${YELLOW}ℹ ${NC}$1"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

# Check if server is running
check_server() {
    print_info "Checking if server is running on ${BASE_URL}..."
    
    if curl -s --max-time 2 "${BASE_URL}/" > /dev/null 2>&1; then
        print_success "Server is running!"
        return 0
    else
        print_error "Server is not responding on ${BASE_URL}"
        print_info "Please start your server with: ./webserv config/default.conf"
        exit 1
    fi
}

# Test 1: Quick benchmark (10 seconds, 10 concurrent users)
test_quick_benchmark() {
    print_header "Test 1: Quick Benchmark (10s, 10 users)"
    print_info "Testing basic performance..."
    
    siege -c 10 -t 10S "${BASE_URL}/" 2>&1 | grep -E "(Transactions|Availability|Elapsed time|Response time|Transaction rate|Throughput|Concurrency|Successful|Failed)"
}

# Test 2: Concurrent users stress test
test_concurrent_users() {
    print_header "Test 2: Concurrent Users (25 users, 100 requests)"
    print_info "Testing with 25 concurrent users..."
    
    siege -c 25 -r 4 "${BASE_URL}/" 2>&1 | grep -E "(Transactions|Availability|Elapsed time|Response time|Transaction rate|Throughput|Concurrency|Successful|Failed)"
}

# Test 3: URL list testing
test_url_list() {
    print_header "Test 3: Multiple URLs (from ${URLS_FILE})"
    
    if [ ! -f "${URLS_FILE}" ]; then
        print_error "URL file not found: ${URLS_FILE}"
        return 1
    fi
    
    print_info "Testing multiple endpoints..."
    siege -c 10 -r 5 -f "${URLS_FILE}" 2>&1 | grep -E "(Transactions|Availability|Elapsed time|Response time|Transaction rate|Throughput|Concurrency|Successful|Failed)"
}

# Test 4: Sustained load test
test_sustained_load() {
    print_header "Test 4: Sustained Load (30s, 20 users)"
    print_info "Testing sustained performance..."
    
    siege -c 20 -t 30S "${BASE_URL}/" 2>&1 | grep -E "(Transactions|Availability|Elapsed time|Response time|Transaction rate|Throughput|Concurrency|Successful|Failed)"
}

# Test 5: Heavy load stress test
test_heavy_load() {
    print_header "Test 5: Heavy Load (50 users, 50 requests each)"
    print_info "Testing under heavy load..."
    
    siege -c 50 -r 50 "${BASE_URL}/" 2>&1 | grep -E "(Transactions|Availability|Elapsed time|Response time|Transaction rate|Throughput|Concurrency|Successful|Failed)"
}

# Test 6: Static file performance
test_static_files() {
    print_header "Test 6: Static File Performance"
    print_info "Testing static file serving..."
    
    siege -c 15 -t 15S "${BASE_URL}/index.html" 2>&1 | grep -E "(Transactions|Availability|Elapsed time|Response time|Transaction rate|Throughput|Concurrency|Successful|Failed)"
}

# Test 7: CGI performance (if available)
test_cgi_performance() {
    print_header "Test 7: CGI Script Performance"
    print_info "Testing CGI execution..."
    
    # Check if CGI is available
    if curl -s --max-time 2 "${BASE_URL}/cgi-bin/test.py" > /dev/null 2>&1; then
        siege -c 5 -r 10 "${BASE_URL}/cgi-bin/test.py" 2>&1 | grep -E "(Transactions|Availability|Elapsed time|Response time|Transaction rate|Throughput|Concurrency|Successful|Failed)"
    else
        print_info "CGI endpoint not available, skipping..."
    fi
}

# Test 8: Internet simulation (random delays)
test_internet_simulation() {
    print_header "Test 8: Internet Simulation (delays enabled)"
    print_info "Simulating real-world internet conditions..."
    
    siege -c 10 -t 15S --delay=0.5 "${BASE_URL}/" 2>&1 | grep -E "(Transactions|Availability|Elapsed time|Response time|Transaction rate|Throughput|Concurrency|Successful|Failed)"
}

# Custom test function
run_custom_test() {
    print_header "Custom Siege Test"
    
    if [ $# -eq 0 ]; then
        print_error "No custom parameters provided"
        return 1
    fi
    
    print_info "Running: siege $@"
    siege "$@"
}

# Show usage
show_usage() {
    cat << EOF
${GREEN}WebServ Siege Testing Script${NC}

Usage: $0 [option]

Options:
    all             Run all tests (default)
    quick           Quick benchmark (10s)
    concurrent      Concurrent users test
    urls            Test multiple URLs
    sustained       Sustained load test (30s)
    heavy           Heavy load test (50 users)
    static          Static file performance
    cgi             CGI script performance
    internet        Internet simulation with delays
    custom          Run custom siege command (pass additional args)
    help            Show this help message

Examples:
    $0                          # Run all tests
    $0 quick                    # Run quick benchmark only
    $0 sustained                # Run sustained load test
    $0 custom -c 100 -r 10      # Custom: 100 users, 10 requests each

Siege Options Reference:
    -c NUM      Number of concurrent users
    -r NUM      Number of repetitions per user
    -t TIME     Time to run (e.g., 30S, 1M, 1H)
    -f FILE     URL file to test
    -d NUM      Delay between requests (seconds)
    -b          Benchmark mode (no delays)
    -v          Verbose output
    -i          Internet mode (random URLs from file)

EOF
}

# Main execution
main() {
    echo -e "${GREEN}"
    cat << "EOF"
╔═══════════════════════════════════════╗
║     WebServ Siege Benchmark Tool     ║
║         HTTP Server Testing          ║
╚═══════════════════════════════════════╝
EOF
    echo -e "${NC}"
    
    # Check if server is running first
    check_server
    
    # Determine which test to run
    case "${1:-all}" in
        all)
            test_quick_benchmark
            sleep 2
            test_concurrent_users
            sleep 2
            test_static_files
            sleep 2
            test_url_list
            sleep 2
            test_sustained_load
            sleep 2
            test_cgi_performance
            sleep 2
            test_internet_simulation
            print_header "All Tests Complete!"
            ;;
        quick)
            test_quick_benchmark
            ;;
        concurrent)
            test_concurrent_users
            ;;
        urls)
            test_url_list
            ;;
        sustained)
            test_sustained_load
            ;;
        heavy)
            test_heavy_load
            ;;
        static)
            test_static_files
            ;;
        cgi)
            test_cgi_performance
            ;;
        internet)
            test_internet_simulation
            ;;
        custom)
            shift
            run_custom_test "$@"
            ;;
        help|--help|-h)
            show_usage
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
    
    print_success "Testing complete!"
}

# Run main function
main "$@"
