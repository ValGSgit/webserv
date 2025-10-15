#!/usr/bin/env python3

import os
import sys
from datetime import datetime

# Print CGI headers
print("Content-Type: text/html")
print()  # Empty line required after headers

# Get CGI environment variables
method = os.environ.get('REQUEST_METHOD', 'Unknown')
uri = os.environ.get('REQUEST_URI', 'Unknown')
query = os.environ.get('QUERY_STRING', '')
content_type = os.environ.get('CONTENT_TYPE', 'Unknown')
content_length = os.environ.get('CONTENT_LENGTH', '0')

# Read POST data if any
post_data = ""
if method == 'POST' and content_length != '0':
    try:
        post_data = sys.stdin.read(int(content_length))
    except:
        post_data = "(error reading POST data)"

# Generate HTML response
html = f"""<!DOCTYPE html>
<html>
<head>
    <title>CGI Test Script</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 40px; }}
        h1 {{ color: #007acc; }}
        .info {{ background: #f0f8ff; padding: 15px; margin: 10px 0; border-radius: 5px; }}
        .data {{ background: #fff5ee; padding: 15px; margin: 10px 0; border-radius: 5px; }}
        table {{ border-collapse: collapse; width: 100%; }}
        th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
        th {{ background-color: #f2f2f2; }}
    </style>
</head>
<body>
    <h1>🐍 Python CGI Test</h1>
    
    <div class="info">
        <h3>Request Information</h3>
        <table>
            <tr><th>Property</th><th>Value</th></tr>
            <tr><td>Request Method</td><td>{method}</td></tr>
            <tr><td>Request URI</td><td>{uri}</td></tr>
            <tr><td>Query String</td><td>{query or '(empty)'}</td></tr>
            <tr><td>Content Type</td><td>{content_type}</td></tr>
            <tr><td>Content Length</td><td>{content_length}</td></tr>
            <tr><td>Server Time</td><td>{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</td></tr>
        </table>
    </div>
    
    <div class="data">
        <h3>Environment Variables</h3>
        <table>
"""

# Add environment variables
env_vars = ['SERVER_NAME', 'SERVER_PORT', 'SERVER_PROTOCOL', 'SERVER_SOFTWARE', 
           'GATEWAY_INTERFACE', 'SCRIPT_NAME', 'SCRIPT_FILENAME', 'DOCUMENT_ROOT']

for var in env_vars:
    value = os.environ.get(var, '(not set)')
    html += f"            <tr><td>{var}</td><td>{value}</td></tr>\n"

html += """        </table>
    </div>"""

# Add POST data if available
if post_data:
    html += f"""
    <div class="data">
        <h3>POST Data</h3>
        <pre>{post_data}</pre>
    </div>"""

# Add query parameters if available
if query:
    html += f"""
    <div class="data">
        <h3>Query Parameters</h3>
        <ul>"""
    
    for param in query.split('&'):
        if '=' in param:
            key, value = param.split('=', 1)
            html += f"            <li><strong>{key}:</strong> {value}</li>\n"
        else:
            html += f"            <li>{param}</li>\n"
    
    html += """        </ul>
    </div>"""

html += """
    <div class="info">
        <p>✅ CGI is working correctly!</p>
        <p><a href="/">← Back to home</a></p>
    </div>
</body>
</html>"""

print(html)

print(html)
