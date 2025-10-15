#!/bin/bash

echo "Content-Type: text/html"
echo ""

cat << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>Bash CGI Test</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f0f0f0; }
        .container { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #2e8b57; }
        .section { background: #f5f5f5; padding: 15px; margin: 15px 0; border-radius: 5px; border-left: 4px solid #2e8b57; }
        table { width: 100%; border-collapse: collapse; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #2e8b57; color: white; }
        .code { font-family: monospace; background: #f1f1f1; padding: 2px 4px; border-radius: 3px; }
        pre { background: #f8f8f8; padding: 10px; border-radius: 3px; overflow-x: auto; }
        .cmd-output { background: #2d3748; color: #e2e8f0; padding: 10px; border-radius: 3px; font-family: monospace; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🐚 Bash CGI Test Script</h1>
        
        <div class="section">
            <h2>Shell Information</h2>
            <table>
                <tr><th>Property</th><th>Value</th></tr>
EOF

echo "<tr><td>Shell</td><td>$SHELL</td></tr>"
echo "<tr><td>Bash Version</td><td>$BASH_VERSION</td></tr>"
echo "<tr><td>Request Method</td><td>${REQUEST_METHOD:-Unknown}</td></tr>"
echo "<tr><td>Request URI</td><td>${REQUEST_URI:-Unknown}</td></tr>"
echo "<tr><td>Query String</td><td>${QUERY_STRING:-(empty)}</td></tr>"
echo "<tr><td>Server Time</td><td>$(date)</td></tr>"

cat << 'EOF'
            </table>
        </div>
        
        <div class="section">
            <h2>Environment Variables</h2>
            <table>
                <tr><th>Variable</th><th>Value</th></tr>
EOF

# Display important CGI environment variables
for var in GATEWAY_INTERFACE SERVER_NAME SERVER_PORT SERVER_PROTOCOL DOCUMENT_ROOT SCRIPT_NAME SCRIPT_FILENAME CONTENT_TYPE CONTENT_LENGTH; do
    value="${!var:-Not set}"
    echo "<tr><td class='code'>$var</td><td>$(echo "$value" | sed 's/</\&lt;/g; s/>/\&gt;/g')</td></tr>"
done

cat << 'EOF'
            </table>
        </div>
        
        <div class="section">
            <h2>HTTP Headers</h2>
            <table>
                <tr><th>Header</th><th>Value</th></tr>
EOF

# Display HTTP headers
env | grep '^HTTP_' | sort | while IFS='=' read -r key value; do
    header=$(echo "$key" | sed 's/^HTTP_//' | tr '_' '-')
    echo "<tr><td class='code'>$header</td><td>$(echo "$value" | sed 's/</\&lt;/g; s/>/\&gt;/g')</td></tr>"
done

cat << 'EOF'
            </table>
        </div>
EOF

# Handle POST data
if [ "$REQUEST_METHOD" = "POST" ]; then
    cat << 'EOF'
        <div class="section">
            <h2>POST Data</h2>
            <pre>
EOF
    # Read POST data from stdin
    if [ -n "$CONTENT_LENGTH" ] && [ "$CONTENT_LENGTH" -gt 0 ]; then
        head -c "$CONTENT_LENGTH" | sed 's/</\&lt;/g; s/>/\&gt;/g'
    fi
    echo "</pre>"
    echo "</div>"
fi

cat << 'EOF'
        
        <div class="section">
            <h2>System Information</h2>
            <table>
                <tr><th>Property</th><th>Value</th></tr>
EOF

echo "<tr><td>Hostname</td><td>$(hostname 2>/dev/null || echo 'Unknown')</td></tr>"
echo "<tr><td>Uptime</td><td>$(uptime 2>/dev/null || echo 'Unknown')</td></tr>"
echo "<tr><td>Process ID</td><td>$$</td></tr>"
echo "<tr><td>User ID</td><td>$(id -u 2>/dev/null || echo 'Unknown')</td></tr>"
echo "<tr><td>Working Directory</td><td>$(pwd)</td></tr>"
echo "<tr><td>Shell Options</td><td>$-</td></tr>"

cat << 'EOF'
            </table>
        </div>
        
        <div class="section">
            <h2>Command Availability Test</h2>
            <table>
                <tr><th>Command</th><th>Status</th><th>Version/Path</th></tr>
EOF

# Test common commands
for cmd in ls cat grep awk sed curl wget git python python3 php perl ruby node; do
    if command -v "$cmd" >/dev/null 2>&1; then
        path=$(command -v "$cmd")
        version=$("$cmd" --version 2>/dev/null | head -n1 || echo "Available at $path")
        echo "<tr><td class='code'>$cmd</td><td style='color: green;'>Available</td><td>$(echo "$version" | sed 's/</\&lt;/g; s/>/\&gt;/g')</td></tr>"
    else
        echo "<tr><td class='code'>$cmd</td><td style='color: red;'>Not Available</td><td>-</td></tr>"
    fi
done

cat << 'EOF'
            </table>
        </div>
        
        <div class="section">
            <h2>URL Parameters</h2>
EOF

if [ -n "$QUERY_STRING" ]; then
    echo "<table><tr><th>Parameter</th><th>Value</th></tr>"
    # Parse query string (simple implementation)
    echo "$QUERY_STRING" | tr '&' '\n' | while IFS='=' read -r key value; do
        # URL decode (basic)
        key=$(echo "$key" | sed 's/+/ /g; s/%20/ /g')
        value=$(echo "$value" | sed 's/+/ /g; s/%20/ /g; s/</\&lt;/g; s/>/\&gt;/g')
        echo "<tr><td class='code'>$key</td><td>$value</td></tr>"
    done
    echo "</table>"
else
    echo "<p>No parameters received. Try adding ?name=value to the URL.</p>"
fi

cat << 'EOF'
        </div>
        
        <div class="section">
            <h2>Simple Shell Calculation</h2>
            <div class="cmd-output">
                $ echo "Scale=2; 22/7" | bc -l
EOF

# Simple calculation (if bc is available)
if command -v bc >/dev/null 2>&1; then
    echo "Scale=2; 22/7" | bc -l 2>/dev/null || echo "3.14"
else
    echo "3.14 (bc not available)"
fi

echo "                $ date '+%Y-%m-%d %H:%M:%S'"
date '+%Y-%m-%d %H:%M:%S' 2>/dev/null || echo "$(date)"

cat << 'EOF'
            </div>
        </div>
    </div>
</body>
</html>
EOF
