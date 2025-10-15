#!/usr/bin/ruby
require 'cgi'
require 'time'

cgi = CGI.new

puts cgi.header('text/html')

puts <<~HTML
<!DOCTYPE html>
<html>
<head>
    <title>Ruby CGI Test</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #ffe4e1; }
        .container { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #dc143c; }
        .section { background: #fff5ee; padding: 15px; margin: 15px 0; border-radius: 5px; border-left: 4px solid #dc143c; }
        table { width: 100%; border-collapse: collapse; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #dc143c; color: white; }
        .code { font-family: monospace; background: #f1f1f1; padding: 2px 4px; border-radius: 3px; }
        pre { background: #f8f8f8; padding: 10px; border-radius: 3px; overflow-x: auto; }
        .gem-status { padding: 3px 6px; border-radius: 3px; color: white; font-size: 0.8em; }
        .available { background-color: #28a745; }
        .unavailable { background-color: #dc3545; }
    </style>
</head>
<body>
    <div class="container">
        <h1>💎 Ruby CGI Test Script</h1>
        
        <div class="section">
            <h2>Ruby Information</h2>
            <table>
                <tr><th>Property</th><th>Value</th></tr>
                <tr><td>Ruby Version</td><td>#{RUBY_VERSION}</td></tr>
                <tr><td>Ruby Platform</td><td>#{RUBY_PLATFORM}</td></tr>
                <tr><td>Ruby Engine</td><td>#{RUBY_ENGINE}</td></tr>
                <tr><td>Request Method</td><td>#{ENV['REQUEST_METHOD'] || 'Unknown'}</td></tr>
                <tr><td>Request URI</td><td>#{ENV['REQUEST_URI'] || 'Unknown'}</td></tr>
                <tr><td>Query String</td><td>#{ENV['QUERY_STRING'].empty? ? '(empty)' : ENV['QUERY_STRING']}</td></tr>
                <tr><td>Server Time</td><td>#{Time.now.strftime('%Y-%m-%d %H:%M:%S %Z')}</td></tr>
            </table>
        </div>
        
        <div class="section">
            <h2>CGI Environment Variables</h2>
            <table>
                <tr><th>Variable</th><th>Value</th></tr>
HTML

env_vars = %w[GATEWAY_INTERFACE SERVER_NAME SERVER_PORT SERVER_PROTOCOL 
              DOCUMENT_ROOT SCRIPT_NAME SCRIPT_FILENAME CONTENT_TYPE CONTENT_LENGTH]

env_vars.each do |var|
  value = ENV[var] || 'Not set'
  puts "<tr><td class='code'>#{var}</td><td>#{CGI.escapeHTML(value)}</td></tr>"
end

puts <<~HTML
            </table>
        </div>
        
        <div class="section">
            <h2>HTTP Headers</h2>
            <table>
                <tr><th>Header</th><th>Value</th></tr>
HTML

ENV.select { |k, v| k.start_with?('HTTP_') }.sort.each do |key, value|
  header = key.sub('HTTP_', '').tr('_', '-')
  puts "<tr><td class='code'>#{header}</td><td>#{CGI.escapeHTML(value)}</td></tr>"
end

puts <<~HTML
            </table>
        </div>
HTML

if ENV['REQUEST_METHOD'] == 'POST'
  puts <<~HTML
        <div class="section">
            <h2>POST Data</h2>
            <pre>#{CGI.escapeHTML(STDIN.read)}</pre>
        </div>
  HTML
end

puts <<~HTML
        <div class="section">
            <h2>System Information</h2>
            <table>
                <tr><th>Property</th><th>Value</th></tr>
                <tr><td>Process ID</td><td>#{Process.pid}</td></tr>
                <tr><td>User ID</td><td>#{Process.uid}</td></tr>
                <tr><td>Group ID</td><td>#{Process.gid}</td></tr>
                <tr><td>Working Directory</td><td>#{Dir.pwd}</td></tr>
                <tr><td>Ruby Load Path Size</td><td>#{$LOAD_PATH.size} directories</td></tr>
            </table>
        </div>
        
        <div class="section">
            <h2>Ruby Gems Test</h2>
            <table>
                <tr><th>Gem</th><th>Status</th></tr>
HTML

gems = %w[json yaml uri net-http fileutils digest]
gems.each do |gem_name|
  begin
    require gem_name
    status = "<span class='gem-status available'>Available</span>"
  rescue LoadError
    status = "<span class='gem-status unavailable'>Not Available</span>"
  end
  puts "<tr><td class='code'>#{gem_name}</td><td>#{status}</td></tr>"
end

puts <<~HTML
            </table>
        </div>
        
        <div class="section">
            <h2>URL Parameters</h2>
HTML

if cgi.params.any?
  puts "<table><tr><th>Parameter</th><th>Value(s)</th></tr>"
  cgi.params.each do |key, values|
    values_str = values.map { |v| CGI.escapeHTML(v) }.join(', ')
    puts "<tr><td class='code'>#{CGI.escapeHTML(key)}</td><td>#{values_str}</td></tr>"
  end
  puts "</table>"
else
  puts "<p>No parameters received. Try adding ?name=value to the URL.</p>"
end

puts <<~HTML
        </div>
        
        <div class="section">
            <h2>Ruby Code Example</h2>
            <p>Here's a simple Ruby calculation:</p>
            <pre>
# Calculate Fibonacci sequence
def fibonacci(n)
  return n if n <= 1
  fibonacci(n-1) + fibonacci(n-2)
end

# First 10 Fibonacci numbers
result = (0..9).map { |i| fibonacci(i) }
# => #{(0..9).map { |i| 
    def fibonacci(n)
      return n if n <= 1
      fibonacci(n-1) + fibonacci(n-2)
    end
    fibonacci(i)
  }.inspect}
            </pre>
        </div>
    </div>
</body>
</html>
HTML
