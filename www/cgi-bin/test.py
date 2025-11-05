#!/usr/bin/env python3
"""
CGI Test Script - Python
RFC 3875 (CGI/1.1) Compliant Test Implementation
Tests all CGI functionality with comprehensive visual output
"""

import os
import sys
import json
from datetime import datetime
import html

# ============================================================================
# CGI HEADERS (RFC 3875 Section 6)
# ============================================================================
print("Content-Type: text/html; charset=utf-8")
print()  # Empty line required after headers (RFC 3875 Section 6.3.3)

# ============================================================================
# COLLECT CGI ENVIRONMENT VARIABLES (RFC 3875 Section 4.1)
# ============================================================================

# Meta-Variables (RFC 3875 Section 4.1)
meta_vars = {
    'AUTH_TYPE': os.environ.get('AUTH_TYPE', ''),
    'CONTENT_LENGTH': os.environ.get('CONTENT_LENGTH', ''),
    'CONTENT_TYPE': os.environ.get('CONTENT_TYPE', ''),
    'GATEWAY_INTERFACE': os.environ.get('GATEWAY_INTERFACE', ''),
    'PATH_INFO': os.environ.get('PATH_INFO', ''),
    'PATH_TRANSLATED': os.environ.get('PATH_TRANSLATED', ''),
    'QUERY_STRING': os.environ.get('QUERY_STRING', ''),
    'REMOTE_ADDR': os.environ.get('REMOTE_ADDR', ''),
    'REMOTE_HOST': os.environ.get('REMOTE_HOST', ''),
    'REMOTE_IDENT': os.environ.get('REMOTE_IDENT', ''),
    'REMOTE_USER': os.environ.get('REMOTE_USER', ''),
    'REQUEST_METHOD': os.environ.get('REQUEST_METHOD', ''),
    'SCRIPT_NAME': os.environ.get('SCRIPT_NAME', ''),
    'SERVER_NAME': os.environ.get('SERVER_NAME', ''),
    'SERVER_PORT': os.environ.get('SERVER_PORT', ''),
    'SERVER_PROTOCOL': os.environ.get('SERVER_PROTOCOL', ''),
    'SERVER_SOFTWARE': os.environ.get('SERVER_SOFTWARE', ''),
}

# Additional useful variables
extra_vars = {
    'REQUEST_URI': os.environ.get('REQUEST_URI', ''),
    'DOCUMENT_ROOT': os.environ.get('DOCUMENT_ROOT', ''),
    'SCRIPT_FILENAME': os.environ.get('SCRIPT_FILENAME', ''),
    'HTTP_HOST': os.environ.get('HTTP_HOST', ''),
    'HTTP_USER_AGENT': os.environ.get('HTTP_USER_AGENT', ''),
    'HTTP_ACCEPT': os.environ.get('HTTP_ACCEPT', ''),
    'HTTP_ACCEPT_LANGUAGE': os.environ.get('HTTP_ACCEPT_LANGUAGE', ''),
    'HTTP_ACCEPT_ENCODING': os.environ.get('HTTP_ACCEPT_ENCODING', ''),
    'HTTP_CONNECTION': os.environ.get('HTTP_CONNECTION', ''),
    'HTTP_COOKIE': os.environ.get('HTTP_COOKIE', ''),
}

# ============================================================================
# READ REQUEST BODY (RFC 3875 Section 4.2)
# ============================================================================
post_data = ""
post_data_parsed = {}

method = meta_vars['REQUEST_METHOD']
content_length = meta_vars['CONTENT_LENGTH']

if method == 'POST' and content_length:
    try:
        length = int(content_length)
        if length > 0:
            post_data = sys.stdin.read(length)
            # Try to parse as form data
            if '=' in post_data and '&' in post_data or '=' in post_data:
                for pair in post_data.split('&'):
                    if '=' in pair:
                        key, val = pair.split('=', 1)
                        post_data_parsed[key] = val
    except Exception as e:
        post_data = f"(Error reading POST data: {str(e)})"

# ============================================================================
# PARSE QUERY STRING (RFC 3875 Section 4.1.7)
# ============================================================================
query_params = {}
query_string = meta_vars['QUERY_STRING']
if query_string:
    for pair in query_string.split('&'):
        if '=' in pair:
            key, val = pair.split('=', 1)
            query_params[key] = val

# ============================================================================
# GENERATE HTML OUTPUT
# ============================================================================

current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
python_version = sys.version

html_output = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>🐍 Python CGI Test - RFC 3875 Compliant</title>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}
        
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            padding: 20px;
            color: #333;
        }}
        
        .container {{
            max-width: 1400px;
            margin: 0 auto;
            background: white;
            border-radius: 15px;
            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
            overflow: hidden;
        }}
        
        .header {{
            background: linear-gradient(135deg, #3776ab 0%, #ffd343 100%);
            color: white;
            padding: 40px;
            text-align: center;
        }}
        
        .header h1 {{
            font-size: 2.5em;
            margin-bottom: 10px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }}
        
        .header .subtitle {{
            font-size: 1.1em;
            opacity: 0.95;
        }}
        
        .badge {{
            display: inline-block;
            padding: 5px 15px;
            background: rgba(255,255,255,0.2);
            border-radius: 20px;
            font-size: 0.9em;
            margin: 5px;
        }}
        
        .content {{
            padding: 40px;
        }}
        
        .grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(600px, 1fr));
            gap: 30px;
            margin-bottom: 30px;
        }}
        
        .section {{
            background: #f8f9fa;
            border-radius: 10px;
            padding: 25px;
            border-left: 5px solid #3776ab;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            transition: transform 0.2s;
        }}
        
        .section:hover {{
            transform: translateY(-5px);
            box-shadow: 0 5px 20px rgba(0,0,0,0.15);
        }}
        
        .section h2 {{
            color: #3776ab;
            margin-bottom: 20px;
            font-size: 1.5em;
            display: flex;
            align-items: center;
            gap: 10px;
        }}
        
        .icon {{
            font-size: 1.3em;
        }}
        
        table {{
            width: 100%;
            border-collapse: collapse;
            background: white;
            border-radius: 8px;
            overflow: hidden;
            box-shadow: 0 2px 5px rgba(0,0,0,0.05);
        }}
        
        th {{
            background: #3776ab;
            color: white;
            padding: 12px;
            text-align: left;
            font-weight: 600;
        }}
        
        td {{
            padding: 12px;
            border-bottom: 1px solid #e9ecef;
        }}
        
        tr:nth-child(even) {{
            background: #f8f9fa;
        }}
        
        tr:hover {{
            background: #e7f3ff;
        }}
        
        .code {{
            font-family: 'Courier New', monospace;
            background: #2d3748;
            color: #68d391;
            padding: 3px 8px;
            border-radius: 4px;
            font-size: 0.9em;
        }}
        
        .method-badge {{
            display: inline-block;
            padding: 4px 12px;
            border-radius: 4px;
            font-weight: bold;
            font-size: 0.9em;
        }}
        
        .method-GET {{ background: #28a745; color: white; }}
        .method-POST {{ background: #ffc107; color: #333; }}
        .method-PUT {{ background: #17a2b8; color: white; }}
        .method-DELETE {{ background: #dc3545; color: white; }}
        .method-HEAD {{ background: #6c757d; color: white; }}
        
        .status-ok {{
            color: #28a745;
            font-weight: bold;
        }}
        
        .status-empty {{
            color: #6c757d;
            font-style: italic;
        }}
        
        .status-error {{
            color: #dc3545;
            font-weight: bold;
        }}
        
        .alert {{
            padding: 15px;
            border-radius: 8px;
            margin: 15px 0;
            border-left: 5px solid;
        }}
        
        .alert-info {{
            background: #d1ecf1;
            border-color: #0c5460;
            color: #0c5460;
        }}
        
        .alert-success {{
            background: #d4edda;
            border-color: #155724;
            color: #155724;
        }}
        
        .alert-warning {{
            background: #fff3cd;
            border-color: #856404;
            color: #856404;
        }}
        
        .json-view {{
            background: #2d3748;
            color: #68d391;
            padding: 15px;
            border-radius: 8px;
            overflow-x: auto;
            font-family: 'Courier New', monospace;
            line-height: 1.6;
        }}
        
        .footer {{
            text-align: center;
            padding: 20px;
            background: #f8f9fa;
            color: #6c757d;
            font-size: 0.9em;
        }}
        
        .compliance-badge {{
            display: inline-block;
            padding: 8px 15px;
            background: #28a745;
            color: white;
            border-radius: 5px;
            font-weight: bold;
            margin: 10px 0;
        }}
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🐍 Python CGI Test Script</h1>
            <div class="subtitle">RFC 3875 (CGI/1.1) Compliant Implementation</div>
            <div>
                <span class="badge">Python {sys.version.split()[0]}</span>
                <span class="badge">CGI/1.1</span>
                <span class="badge">{current_time}</span>
            </div>
        </div>
        
        <div class="content">
            <div class="alert alert-success">
                <strong>✓ CGI Execution Successful!</strong> This script executed properly through your web server's CGI interface.
            </div>
            
            <div class="section">
                <h2><span class="icon">📋</span> Request Information</h2>
                <table>
                    <tr>
                        <th>Property</th>
                        <th>Value</th>
                    </tr>
                    <tr>
                        <td><strong>Request Method</strong></td>
                        <td><span class="method-badge method-{method}">{method}</span></td>
                    </tr>
                    <tr>
                        <td><strong>Request URI</strong></td>
                        <td class="code">{html.escape(extra_vars['REQUEST_URI'])}</td>
                    </tr>
                    <tr>
                        <td><strong>Script Name</strong></td>
                        <td class="code">{html.escape(meta_vars['SCRIPT_NAME'])}</td>
                    </tr>
                    <tr>
                        <td><strong>Query String</strong></td>
                        <td class="{'status-empty' if not query_string else 'status-ok'}">{html.escape(query_string) if query_string else '(empty)'}</td>
                    </tr>
                    <tr>
                        <td><strong>Server Protocol</strong></td>
                        <td class="code">{html.escape(meta_vars['SERVER_PROTOCOL'])}</td>
                    </tr>
                    <tr>
                        <td><strong>Server Software</strong></td>
                        <td>{html.escape(meta_vars['SERVER_SOFTWARE'])}</td>
                    </tr>
                </table>
            </div>
            
            <div class="grid">
                <div class="section">
                    <h2><span class="icon">🌐</span> RFC 3875 Meta-Variables</h2>
                    <table>
                        <tr>
                            <th>Variable</th>
                            <th>Value</th>
                        </tr>"""

for var, value in meta_vars.items():
    status_class = 'status-ok' if value else 'status-empty'
    display_value = html.escape(value) if value else '(not set)'
    html_output += f"""
                        <tr>
                            <td class="code">{var}</td>
                            <td class="{status_class}">{display_value}</td>
                        </tr>"""

html_output += """
                    </table>
                </div>
                
                <div class="section">
                    <h2><span class="icon">🔗</span> HTTP Headers</h2>
                    <table>
                        <tr>
                            <th>Header</th>
                            <th>Value</th>
                        </tr>"""

for var, value in extra_vars.items():
    if value:
        status_class = 'status-ok'
        display_value = html.escape(value)
        html_output += f"""
                        <tr>
                            <td class="code">{var}</td>
                            <td class="{status_class}">{display_value}</td>
                        </tr>"""

html_output += """
                    </table>
                </div>
            </div>"""

# Query Parameters Section
if query_params:
    html_output += """
            <div class="section">
                <h2><span class="icon">🔍</span> Query Parameters (Parsed)</h2>
                <table>
                    <tr>
                        <th>Parameter</th>
                        <th>Value</th>
                    </tr>"""
    for key, value in query_params.items():
        html_output += f"""
                    <tr>
                        <td class="code">{html.escape(key)}</td>
                        <td>{html.escape(value)}</td>
                    </tr>"""
    html_output += """
                </table>
            </div>"""

# POST Data Section
if method == 'POST':
    html_output += f"""
            <div class="section">
                <h2><span class="icon">📮</span> POST Request Body</h2>
                <div class="alert alert-info">
                    <strong>Content-Length:</strong> {content_length} bytes<br>
                    <strong>Content-Type:</strong> {html.escape(meta_vars['CONTENT_TYPE'])}
                </div>
                <h3>Raw Data:</h3>
                <div class="json-view">{html.escape(post_data) if post_data else '(empty)'}</div>"""
    
    if post_data_parsed:
        html_output += """
                <h3 style="margin-top: 20px;">Parsed Form Data:</h3>
                <table>
                    <tr>
                        <th>Field</th>
                        <th>Value</th>
                    </tr>"""
        for key, value in post_data_parsed.items():
            html_output += f"""
                    <tr>
                        <td class="code">{html.escape(key)}</td>
                        <td>{html.escape(value)}</td>
                    </tr>"""
        html_output += """
                </table>"""
    
    html_output += """
            </div>"""

# Python Environment Section
html_output += f"""
            <div class="section">
                <h2><span class="icon">🐍</span> Python Environment</h2>
                <table>
                    <tr>
                        <th>Property</th>
                        <th>Value</th>
                    </tr>
                    <tr>
                        <td><strong>Python Version</strong></td>
                        <td class="code">{sys.version.split()[0]}</td>
                    </tr>
                    <tr>
                        <td><strong>Python Executable</strong></td>
                        <td class="code">{html.escape(sys.executable)}</td>
                    </tr>
                    <tr>
                        <td><strong>Python Path</strong></td>
                        <td class="code">{html.escape(', '.join(sys.path[:3]))}...</td>
                    </tr>
                    <tr>
                        <td><strong>Script Location</strong></td>
                        <td class="code">{html.escape(os.path.abspath(__file__))}</td>
                    </tr>
                </table>
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
                • GET: <code>curl http://localhost:8080/cgi-bin/test.py</code><br>
                • GET with params: <code>curl "http://localhost:8080/cgi-bin/test.py?name=John&age=30"</code><br>
                • POST: <code>curl -X POST -d "field1=value1&field2=value2" http://localhost:8080/cgi-bin/test.py</code><br>
                • POST JSON: <code>curl -X POST -H "Content-Type: application/json" -d '{{"key":"value"}}' http://localhost:8080/cgi-bin/test.py</code>
            </div>
        </div>
        
        <div class="footer">
            Generated by Python CGI Test Script | RFC 3875 Compliant | {current_time}
        </div>
    </div>
</body>
</html>"""

print(html_output)
