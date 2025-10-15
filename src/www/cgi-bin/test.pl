#!/usr/bin/perl
use strict;
use warnings;
use CGI;

my $cgi = CGI->new;

print $cgi->header('text/html');

print <<'HTML';
<!DOCTYPE html>
<html>
<head>
    <title>Perl CGI Test</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f0f8ff; }
        .container { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #4b0082; }
        .section { background: #f5f5f5; padding: 15px; margin: 15px 0; border-radius: 5px; }
        table { width: 100%; border-collapse: collapse; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #4b0082; color: white; }
        .code { font-family: monospace; background: #f1f1f1; padding: 2px 4px; }
        pre { background: #f8f8f8; padding: 10px; border-radius: 3px; overflow-x: auto; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🐪 Perl CGI Test Script</h1>
        
        <div class="section">
            <h2>Perl Information</h2>
            <table>
                <tr><th>Property</th><th>Value</th></tr>
HTML

print "<tr><td>Perl Version</td><td>$]</td></tr>";
print "<tr><td>Perl Executable</td><td>$^X</td></tr>";
print "<tr><td>Request Method</td><td>" . ($ENV{REQUEST_METHOD} || 'Unknown') . "</td></tr>";
print "<tr><td>Request URI</td><td>" . ($ENV{REQUEST_URI} || 'Unknown') . "</td></tr>";
print "<tr><td>Query String</td><td>" . ($ENV{QUERY_STRING} || '(empty)') . "</td></tr>";
print "<tr><td>Server Time</td><td>" . localtime() . "</td></tr>";

print <<'HTML';
            </table>
        </div>
        
        <div class="section">
            <h2>Environment Variables</h2>
            <table>
                <tr><th>Variable</th><th>Value</th></tr>
HTML

my @env_vars = qw(GATEWAY_INTERFACE SERVER_NAME SERVER_PORT SERVER_PROTOCOL 
                  DOCUMENT_ROOT SCRIPT_NAME SCRIPT_FILENAME CONTENT_TYPE CONTENT_LENGTH);

foreach my $var (@env_vars) {
    my $value = $ENV{$var} || 'Not set';
    print "<tr><td class='code'>$var</td><td>$value</td></tr>";
}

print <<'HTML';
            </table>
        </div>
        
        <div class="section">
            <h2>HTTP Headers</h2>
            <table>
                <tr><th>Header</th><th>Value</th></tr>
HTML

foreach my $key (sort keys %ENV) {
    if ($key =~ /^HTTP_(.+)/) {
        my $header = $1;
        $header =~ s/_/-/g;
        print "<tr><td class='code'>$header</td><td>$ENV{$key}</td></tr>";
    }
}

print <<'HTML';
            </table>
        </div>
HTML

if ($ENV{REQUEST_METHOD} eq 'POST') {
    print "<div class='section'>";
    print "<h2>POST Data</h2>";
    my $post_data = $cgi->param('POSTDATA') || '';
    if (!$post_data) {
        read(STDIN, $post_data, $ENV{CONTENT_LENGTH} || 0);
    }
    print "<pre>" . $cgi->escapeHTML($post_data) . "</pre>";
    print "</div>";
}

print <<'HTML';
        
        <div class="section">
            <h2>System Information</h2>
            <table>
                <tr><th>Property</th><th>Value</th></tr>
HTML

print "<tr><td>Operating System</td><td>$^O</td></tr>";
print "<tr><td>Process ID</td><td>$$</td></tr>";
print "<tr><td>Real User ID</td><td>$<</td></tr>";
print "<tr><td>Effective User ID</td><td>$></td></tr>";

print <<'HTML';
            </table>
        </div>
        
        <div class="section">
            <h2>Perl Modules Test</h2>
            <table>
                <tr><th>Module</th><th>Status</th></tr>
HTML

my @modules = qw(CGI Digest::MD5 File::Basename JSON Time::HiRes);
foreach my $module (@modules) {
    eval "require $module";
    my $status = $@ ? 'Not Available' : 'Available';
    print "<tr><td class='code'>$module</td><td>$status</td></tr>";
}

print <<'HTML';
            </table>
        </div>
        
        <div class="section">
            <h2>URL Parameters</h2>
HTML

my @param_names = $cgi->param();
if (@param_names) {
    print "<table><tr><th>Parameter</th><th>Value</th></tr>";
    foreach my $param (@param_names) {
        my $value = $cgi->param($param);
        print "<tr><td class='code'>$param</td><td>" . $cgi->escapeHTML($value) . "</td></tr>";
    }
    print "</table>";
} else {
    print "<p>No parameters received. Try adding ?name=value to the URL.</p>";
}

print <<'HTML';
        </div>
    </div>
</body>
</html>
HTML
