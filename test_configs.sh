#!/bin/bash

echo "========================================"
echo "WebServ Configuration Parser Test Suite"
echo "========================================"
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counter
TESTS_RUN=0
TESTS_PASSED=0

test_config() {
    local config_file="$1"
    local description="$2"
    local should_pass="$3"  # "pass" or "fail"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    
    echo -e "${BLUE}Test $TESTS_RUN:${NC} $description"
    echo -e "Config file: ${YELLOW}$config_file${NC}"
    
    if [ ! -f "$config_file" ]; then
        echo -e "${RED}✗ Config file not found${NC}"
        echo
        return
    fi
    
    # Run the parser test
    ./webserv "$config_file" > /dev/null 2>&1
    local exit_code=$?
    
    if [ "$should_pass" = "pass" ]; then
        if [ $exit_code -eq 0 ]; then
            echo -e "${GREEN}✓ PASSED${NC} - Configuration parsed successfully"
            TESTS_PASSED=$((TESTS_PASSED + 1))
        else
            echo -e "${RED}✗ FAILED${NC} - Configuration should have passed but failed"
        fi
    else
        if [ $exit_code -ne 0 ]; then
            echo -e "${GREEN}✓ PASSED${NC} - Configuration correctly failed as expected"
            TESTS_PASSED=$((TESTS_PASSED + 1))
        else
            echo -e "${RED}✗ FAILED${NC} - Configuration should have failed but passed"
        fi
    fi
    echo
}

echo "Building webserv..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1

if [ ! -f "./webserv" ]; then
    echo -e "${RED}Error: Failed to build webserv${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Build successful${NC}"
echo

# Test existing configurations
test_config "config/simple.conf" "Simple configuration with basic server block" "pass"
test_config "config/default.conf" "Default configuration with multiple locations" "pass"

# Test new configurations
test_config "config/minimal.conf" "Minimal configuration" "pass"
test_config "config/advanced.conf" "Advanced multi-server configuration" "pass"
test_config "config/edge-cases.conf" "Edge cases and missing fields" "pass"
test_config "config/load-balance.conf" "Load balancing simulation" "pass"
test_config "config/complex.conf" "Complex nested locations and special cases" "pass"

# Test non-existent file
test_config "config/nonexistent.conf" "Non-existent configuration file" "fail"

# Create a malformed config for testing
cat > config/malformed.conf << 'EOF'
server {
    listen 8080
    server_name test
    # Missing closing brace and semicolons
    location / {
        allow_methods GET
        # Missing closing brace
EOF

test_config "config/malformed.conf" "Malformed configuration (syntax errors)" "fail"

# Test empty file
touch config/empty.conf
test_config "config/empty.conf" "Empty configuration file" "fail"

# Clean up test files
rm -f config/malformed.conf config/empty.conf

echo "========================================"
echo "Test Results Summary"
echo "========================================"
echo -e "Tests run: ${BLUE}$TESTS_RUN${NC}"
echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests failed: ${RED}$((TESTS_RUN - TESTS_PASSED))${NC}"

if [ $TESTS_PASSED -eq $TESTS_RUN ]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
else
    echo -e "${YELLOW}⚠ Some tests failed${NC}"
fi

echo
echo "========================================"
echo "CGI Scripts Test"
echo "========================================"

echo "Testing CGI script permissions and syntax..."

# Check if scripts are executable
for script in www/cgi-bin/*.py www/cgi-bin/*.php www/cgi-bin/*.pl www/cgi-bin/*.rb www/cgi-bin/*.sh; do
    if [ -f "$script" ]; then
        if [ -x "$script" ]; then
            echo -e "${GREEN}✓${NC} $script is executable"
        else
            echo -e "${RED}✗${NC} $script is not executable"
        fi
    fi
done

echo
echo "========================================"
echo "Configuration Analysis"
echo "========================================"

echo "Analyzing configuration files for complexity..."

for config in config/*.conf; do
    if [ -f "$config" ]; then
        servers=$(grep -c "^server {" "$config" 2>/dev/null || echo "0")
        locations=$(grep -c "location " "$config" 2>/dev/null || echo "0")
        echo -e "${BLUE}$config${NC}: $servers server(s), $locations location(s)"
    fi
done

echo
echo "Test suite completed!"
