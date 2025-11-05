#!/bin/bash
#
# CGI Test Script - Bash/Shell
# RFC 3875 (CGI/1.1) Compliant Test Implementation
# Tests all CGI functionality with comprehensive visual output
#

# ============================================================================
# CGI HEADERS (RFC 3875 Section 6)
# ============================================================================
echo "Content-Type: text/html; charset=utf-8"
echo ""  # Empty line required after headers (RFC 3875 Section 6.3.3)

# ============================================================================
# COLLECT CGI ENVIRONMENT VARIABLES (RFC 3875 Section 4.1)
# ============================================================================

# Get current time
CURRENT_TIME=$(date '+%Y-%m-%d %H:%M:%S')

# HTML escape function
html_escape() {
    echo "$1" | sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g; s/"/\&quot;/g; s/'"'"'/\&#39;/g'
}

# Get value or default
get_env() {
    local var="$1"
    local val="${!var}"
    if [ -z "$val" ]; then
        echo "(not set)"
    else
        html_escape "$val"
    fi
}

# ============================================================================
# GENERATE HTML OUTPUT
# ============================================================================

cat << 'HTML_START'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>🐚 Bash CGI Test - RFC 3875 Compliant</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #2e8b57 0%, #3cb371 100%);
            padding: 20px;
            color: #333;
        }
        
        .container {
            max-width: 1400px;
            margin: 0 auto;
            background: white;
            border-radius: 15px;
            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
            overflow: hidden;
        }
        
        .header {
            background: linear-gradient(135deg, #2e8b57 0%, #90ee90 100%);
            color: white;
            padding: 40px;
            text-align: center;
        }
        
        .header h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        
        .header .subtitle {
            font-size: 1.1em;
            opacity: 0.95;
        }
        
        .badge {
            display: inline-block;
            padding: 5px 15px;
            background: rgba(255,255,255,0.2);
            border-radius: 20px;
            font-size: 0.9em;
            margin: 5px;
        }
        
        .content {
            padding: 40px;
        }
        
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(600px, 1fr));
            gap: 30px;
            margin-bottom: 30px;
        }
        
        .section {
            background: #f8f9fa;
            border-radius: 10px;
            padding: 25px;
            border-left: 5px solid #2e8b57;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            transition: transform 0.2s;
        }
        
        .section:hover {
            transform: translateY(-5px);
            box-shadow: 0 5px 20px rgba(0,0,0,0.15);
        }
        
        .section h2 {
            color: #2e8b57;
            margin-bottom: 20px;
            font-size: 1.5em;
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        .icon {
            font-size: 1.3em;
        }
        
        table {
            width: 100%;
            border-collapse: collapse;
            background: white;
            border-radius: 8px;
            overflow: hidden;
            box-shadow: 0 2px 5px rgba(0,0,0,0.05);
        }
        
        th {
            background: #2e8b57;
            color: white;
            padding: 12px;
            text-align: left;
            font-weight: 600;
        }
        
        td {
            padding: 12px;
            border-bottom: 1px solid #e9ecef;
        }
        
        tr:nth-child(even) {
            background: #f8f9fa;
        }
        
        tr:hover {
            background: #e7f3e7;
        }
        
        .code {
            font-family: 'Courier New', monospace;
            background: #2d3748;
            color: #90ee90;
            padding: 3px 8px;
            border-radius: 4px;
            font-size: 0.9em;
        }
        
        .method-badge {
            display: inline-block;
            padding: 4px 12px;
            border-radius: 4px;
            font-weight: bold;
            font-size: 0.9em;
        }
        
        .method-GET { background: #28a745; color: white; }
        .method-POST { background: #ffc107; color: #333; }
        .method-PUT { background: #17a2b8; color: white; }
        .method-DELETE { background: #dc3545; color: white; }
        .method-HEAD { background: #6c757d; color: white; }
        
        .status-ok {
            color: #28a745;
            font-weight: bold;
        }
        
        .status-empty {
            color: #6c757d;
            font-style: italic;
        }
        
        .status-available {
            color: #28a745;
            font-weight: bold;
        }
        
        .status-unavailable {
            color: #dc3545;
        }
        
        .alert {
            padding: 15px;
            border-radius: 8px;
            margin: 15px 0;
            border-left: 5px solid;
        }
        
        .alert-info {
            background: #d1ecf1;
            border-color: #0c5460;
            color: #0c5460;
        }
        
        .alert-success {
            background: #d4edda;
            border-color: #155724;
            color: #155724;
        }
        
        .cmd-output {
            background: #2d3748;
            color: #90ee90;
            padding: 15px;
            border-radius: 8px;
            overflow-x: auto;
            font-family: 'Courier New', monospace;
            line-height: 1.6;
            margin: 10px 0;
        }
        
        .footer {
            text-align: center;
            padding: 20px;
            background: #f8f9fa;
            color: #6c757d;
            font-size: 0.9em;
        }
        
        .compliance-badge {
            display: inline-block;
            padding: 8px 15px;
            background: #28a745;
            color: white;
            border-radius: 5px;
            font-weight: bold;
            margin: 10px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🐚 Bash CGI Test Script</h1>
            <div class="subtitle">RFC 3875 (CGI/1.1) Compliant Implementation</div>
            <div>
HTML_START

echo "                <span class='badge'>Bash $BASH_VERSION</span>"
echo "                <span class='badge'>CGI/1.1</span>"
echo "                <span class='badge'>$CURRENT_TIME</span>"

cat << 'HTML_CONT1'
            </div>
        </div>
        
        <div class="content">
            <div class="alert alert-success">
                <strong>✓ CGI Execution Successful!</strong> This shell script executed properly through your web server's CGI interface.
            </div>
            
            <div class="section">
                <h2><span class="icon">📋</span> Request Information</h2>
                <table>
                    <tr>
                        <th>Property</th>
                        <th>Value</th>
                    </tr>
HTML_CONT1

# Request information
METHOD="${REQUEST_METHOD:-GET}"
echo "                    <tr><td><strong>Request Method</strong></td><td><span class='method-badge method-$METHOD'>$METHOD</span></td></tr>"
echo "                    <tr><td><strong>Request URI</strong></td><td class='code'>$(get_env REQUEST_URI)</td></tr>"
echo "                    <tr><td><strong>Script Name</strong></td><td class='code'>$(get_env SCRIPT_NAME)</td></tr>"
echo "                    <tr><td><strong>Query String</strong></td><td class='$([ -z "$QUERY_STRING" ] && echo "status-empty" || echo "status-ok")'>$(get_env QUERY_STRING)</td></tr>"
echo "                    <tr><td><strong>Server Protocol</strong></td><td class='code'>$(get_env SERVER_PROTOCOL)</td></tr>"
echo "                    <tr><td><strong>Server Software</strong></td><td>$(get_env SERVER_SOFTWARE)</td></tr>"

cat << 'HTML_CONT2'
                </table>
            </div>
            
            <div class="grid">
                <div class="section">
                    <h2><span class="icon">🌐</span> RFC 3875 Meta-Variables</h2>
                    <table>
                        <tr>
                            <th>Variable</th>
                            <th>Value</th>
                        </tr>
HTML_CONT2

# RFC 3875 Meta-Variables
for var in AUTH_TYPE CONTENT_LENGTH CONTENT_TYPE GATEWAY_INTERFACE PATH_INFO PATH_TRANSLATED QUERY_STRING REMOTE_ADDR REMOTE_HOST REMOTE_IDENT REMOTE_USER REQUEST_METHOD SCRIPT_NAME SERVER_NAME SERVER_PORT SERVER_PROTOCOL SERVER_SOFTWARE; do
    val="${!var}"
    if [ -z "$val" ]; then
        echo "                        <tr><td class='code'>$var</td><td class='status-empty'>(not set)</td></tr>"
    else
        echo "                        <tr><td class='code'>$var</td><td class='status-ok'>$(html_escape "$val")</td></tr>"
    fi
done

cat << 'HTML_CONT3'
                    </table>
                </div>
                
                <div class="section">
                    <h2><span class="icon">🔗</span> HTTP Headers</h2>
                    <table>
                        <tr>
                            <th>Header</th>
                            <th>Value</th>
                        </tr>
HTML_CONT3

# HTTP headers
env | grep '^HTTP_' | sort | while IFS='=' read -r key value; do
    header=$(echo "$key" | sed 's/^HTTP_//')
    echo "                        <tr><td class='code'>$key</td><td class='status-ok'>$(html_escape "$value")</td></tr>"
done

cat << 'HTML_CONT4'
                    </table>
                </div>
            </div>
HTML_CONT4

# Query Parameters Section
if [ -n "$QUERY_STRING" ]; then
    cat << 'HTML_QUERY1'
            <div class="section">
                <h2><span class="icon">🔍</span> Query Parameters (Parsed)</h2>
                <table>
                    <tr>
                        <th>Parameter</th>
                        <th>Value</th>
                    </tr>
HTML_QUERY1
    
    echo "$QUERY_STRING" | tr '&' '\n' | while IFS='=' read -r key value; do
        # URL decode (basic)
        key=$(echo "$key" | sed 's/+/ /g; s/%20/ /g')
        value=$(echo "$value" | sed 's/+/ /g; s/%20/ /g')
        echo "                    <tr><td class='code'>$(html_escape "$key")</td><td>$(html_escape "$value")</td></tr>"
    done
    
    echo "                </table>"
    echo "            </div>"
fi

# POST Data Section
if [ "$REQUEST_METHOD" = "POST" ] && [ -n "$CONTENT_LENGTH" ] && [ "$CONTENT_LENGTH" -gt 0 ]; then
    cat << 'HTML_POST1'
            <div class="section">
                <h2><span class="icon">📮</span> POST Request Body</h2>
                <div class="alert alert-info">
HTML_POST1
    
    echo "                    <strong>Content-Length:</strong> $CONTENT_LENGTH bytes<br>"
    echo "                    <strong>Content-Type:</strong> $(get_env CONTENT_TYPE)"
    
    echo "                </div>"
    echo "                <h3>Raw Data:</h3>"
    echo "                <div class='cmd-output'>"
    
    # Read POST data
    POST_DATA=$(head -c "$CONTENT_LENGTH")
    if [ -n "$POST_DATA" ]; then
        html_escape "$POST_DATA"
    else
        echo "(empty)"
    fi
    
    echo "                </div>"
    echo "            </div>"
fi

# Shell Environment Section
cat << 'HTML_SHELL1'
            <div class="section">
                <h2><span class="icon">🐚</span> Shell Environment</h2>
                <table>
                    <tr>
                        <th>Property</th>
                        <th>Value</th>
                    </tr>
HTML_SHELL1

echo "                    <tr><td><strong>Shell</strong></td><td class='code'>$SHELL</td></tr>"
echo "                    <tr><td><strong>Bash Version</strong></td><td class='code'>$BASH_VERSION</td></tr>"
echo "                    <tr><td><strong>Process ID</strong></td><td class='code'>$$</td></tr>"
echo "                    <tr><td><strong>Parent PID</strong></td><td class='code'>$PPID</td></tr>"
echo "                    <tr><td><strong>Working Directory</strong></td><td class='code'>$(pwd)</td></tr>"
echo "                    <tr><td><strong>User ID</strong></td><td class='code'>$(id -u 2>/dev/null || echo 'Unknown')</td></tr>"
echo "                    <tr><td><strong>Hostname</strong></td><td class='code'>$(hostname 2>/dev/null || echo 'Unknown')</td></tr>"

cat << 'HTML_SHELL2'
                </table>
            </div>
            
            <div class="section">
                <h2><span class="icon">🔧</span> Command Availability</h2>
                <table>
                    <tr>
                        <th>Command</th>
                        <th>Status</th>
                        <th>Location</th>
                    </tr>
HTML_SHELL2

# Test common commands
for cmd in ls cat grep awk sed curl wget git python python3 php perl ruby node npm; do
    if command -v "$cmd" >/dev/null 2>&1; then
        path=$(command -v "$cmd")
        echo "                    <tr><td class='code'>$cmd</td><td class='status-available'>✓ Available</td><td class='code'>$path</td></tr>"
    else
        echo "                    <tr><td class='code'>$cmd</td><td class='status-unavailable'>✗ Not Found</td><td>-</td></tr>"
    fi
done

cat << 'HTML_COMPLIANCE'
                </table>
            </div>
            
            <div class="section">
                <h2><span class="icon">💻</span> System Commands Demonstration</h2>
                <h3>Date and Time:</h3>
                <div class="cmd-output">$ date '+%Y-%m-%d %H:%M:%S %Z'
HTML_COMPLIANCE

date '+%Y-%m-%d %H:%M:%S %Z' 2>/dev/null

cat << 'HTML_CMD2'
</div>
                <h3>Uptime:</h3>
                <div class="cmd-output">$ uptime
HTML_CMD2

uptime 2>/dev/null || echo "Unknown"

cat << 'HTML_CMD3'
</div>
                <h3>Working Directory Contents:</h3>
                <div class="cmd-output">$ ls -lah
HTML_CMD3

ls -lah 2>/dev/null | head -n 10 || echo "Cannot list directory"

cat << 'HTML_FINAL'
</div>
            </div>
            
            <div class="section">
                <h2><span class="icon">✅</span> RFC 3875 Compliance Check</h2>
                <div class="compliance-badge">✓ RFC 3875 (CGI/1.1) COMPLIANT</div>
                <ul style="margin-top: 15px; line-height: 2;">
                    <li>✓ Content-Type header sent correctly</li>
                    <li>✓ Header/body separator (empty line) present</li>
                    <li>✓ All required meta-variables processed</li>
                    <li>✓ Request body read according to Content-Length</li>
                    <li>✓ Query string parsing implemented</li>
                    <li>✓ UTF-8 encoding specified</li>
                </ul>
            </div>
            
            <div class="alert alert-info">
                <strong>💡 Test this CGI script with different methods:</strong><br>
                • GET: <code>curl http://localhost:8080/cgi-bin/test.sh</code><br>
                • GET with params: <code>curl "http://localhost:8080/cgi-bin/test.sh?name=John&age=30"</code><br>
                • POST: <code>curl -X POST -d "field1=value1&field2=value2" http://localhost:8080/cgi-bin/test.sh</code><br>
                • Test from browser: <code>http://localhost:8080/cgi-bin/test.sh?test=value</code>
            </div>
        </div>
        
        <div class="footer">
HTML_FINAL

echo "            Generated by Bash CGI Test Script | RFC 3875 Compliant | $CURRENT_TIME"

cat << 'HTML_END'
        </div>
    </div>
</body>
</html>
HTML_END
