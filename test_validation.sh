#!/bin/bash

echo "========================================"
echo "WebServ Configuration Validation Test"
echo "========================================"
echo

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Build first
echo "Building webserv..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1

if [ ! -f "./webserv" ]; then
    echo -e "${RED}Error: Failed to build webserv${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Build successful${NC}"
echo

test_config_simple() {
    local config_file="$1"
    local description="$2"
    
    echo -e "${BLUE}Testing:${NC} $description"
    echo -e "File: ${YELLOW}$config_file${NC}"
    
    if [ ! -f "$config_file" ]; then
        echo -e "${RED}✗ Config file not found${NC}"
        echo
        return
    fi
    
    # Test with timeout to prevent hanging
    timeout 3 ./webserv "$config_file" > /tmp/webserv_test.log 2>&1
    local exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✓ Configuration parsed successfully${NC}"
        # Show some stats
        local servers=$(grep "Found .* server" /tmp/webserv_test.log | tail -1)
        if [ ! -z "$servers" ]; then
            echo -e "  $servers"
        fi
    elif [ $exit_code -eq 124 ]; then
        echo -e "${YELLOW}⚠ Timeout (possible infinite loop)${NC}"
    else
        echo -e "${RED}✗ Configuration parsing failed${NC}"
        echo -e "  Exit code: $exit_code"
    fi
    echo
}

# Test all configurations
echo "Testing valid configurations:"
echo "-----------------------------"
test_config_simple "config/simple.conf" "Simple configuration"
test_config_simple "config/default.conf" "Default configuration"
test_config_simple "config/minimal.conf" "Minimal configuration"
test_config_simple "config/advanced.conf" "Advanced multi-server configuration"
test_config_simple "config/edge-cases.conf" "Edge cases and missing fields"
test_config_simple "config/load-balance.conf" "Load balancing simulation"
test_config_simple "config/complex.conf" "Complex nested locations"

echo "Testing invalid configurations:"
echo "------------------------------"
test_config_simple "config/invalid-braces.conf" "Invalid braces (should fail gracefully)"
test_config_simple "config/invalid-location.conf" "Invalid location block"

# Test component functionality
echo "========================================"
echo "Component Tests"
echo "========================================"

echo "Testing component functionality..."
./test_components > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ All component tests passed${NC}"
else
    echo -e "${RED}✗ Some component tests failed${NC}"
fi

echo
echo "========================================"
echo "CGI Scripts Validation"
echo "========================================"

echo "Available CGI scripts:"
for script in www/cgi-bin/*; do
    if [ -f "$script" ] && [ -x "$script" ]; then
        echo -e "${GREEN}✓${NC} $(basename "$script")"
    elif [ -f "$script" ]; then
        echo -e "${YELLOW}⚠${NC} $(basename "$script") (not executable)"
    fi
done

echo
echo "========================================"
echo "Configuration Statistics"
echo "========================================"

echo "Configuration file analysis:"
for config in config/*.conf; do
    if [ -f "$config" ]; then
        servers=$(grep -c "^server {" "$config" 2>/dev/null || echo "0")
        locations=$(grep -c "location " "$config" 2>/dev/null || echo "0")
        lines=$(wc -l < "$config" 2>/dev/null || echo "0")
        echo -e "${BLUE}$(basename "$config")${NC}: $servers server(s), $locations location(s), $lines lines"
    fi
done

echo
echo "Test validation completed!"

# Cleanup
rm -f /tmp/webserv_test.log
