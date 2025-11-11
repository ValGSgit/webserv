#!/usr/bin/env ruby

require 'cgi'

cgi = CGI.new
color = cgi['color'] || '6366f1'
palette_type = cgi['type'] || 'monochrome'

# Parse hex color
color = color.gsub('#', '')
r = color[0..1].to_i(16)
g = color[2..3].to_i(16)
b = color[4..5].to_i(16)

def rgb_to_hex(r, g, b)
  "#%02x%02x%02x" % [r.clamp(0, 255), g.clamp(0, 255), b.clamp(0, 255)]
end

def adjust_brightness(r, g, b, factor)
  [
    (r * factor).round,
    (g * factor).round,
    (b * factor).round
  ]
end

# Generate color palette
colors = []
palette_name = ""

case palette_type
when 'monochrome'
  palette_name = "Monochrome Palette"
  colors = [
    rgb_to_hex(*adjust_brightness(r, g, b, 1.4)),
    rgb_to_hex(*adjust_brightness(r, g, b, 1.2)),
    rgb_to_hex(r, g, b),
    rgb_to_hex(*adjust_brightness(r, g, b, 0.8)),
    rgb_to_hex(*adjust_brightness(r, g, b, 0.6))
  ]
when 'complementary'
  palette_name = "Complementary Colors"
  colors = [
    rgb_to_hex(r, g, b),
    rgb_to_hex(255 - r, 255 - g, 255 - b),
    rgb_to_hex((r + 50).clamp(0, 255), g, b),
    rgb_to_hex(r, (g + 50).clamp(0, 255), b),
    rgb_to_hex(r, g, (b + 50).clamp(0, 255))
  ]
when 'analogous'
  palette_name = "Analogous Colors"
  colors = [
    rgb_to_hex((r - 30).clamp(0, 255), g, (b + 30).clamp(0, 255)),
    rgb_to_hex((r - 15).clamp(0, 255), g, (b + 15).clamp(0, 255)),
    rgb_to_hex(r, g, b),
    rgb_to_hex((r + 15).clamp(0, 255), g, (b - 15).clamp(0, 255)),
    rgb_to_hex((r + 30).clamp(0, 255), g, (b - 30).clamp(0, 255))
  ]
when 'triadic'
  palette_name = "Triadic Colors"
  colors = [
    rgb_to_hex(r, g, b),
    rgb_to_hex(b, r, g),
    rgb_to_hex(g, b, r),
    rgb_to_hex((r + b) / 2, (g + r) / 2, (b + g) / 2),
    rgb_to_hex((r + 128).clamp(0, 255), (g + 128).clamp(0, 255), (b + 128).clamp(0, 255))
  ]
end

puts "Content-Type: text/html\r\n\r\n"
puts <<-HTML
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body {
            font-family: 'Arial', sans-serif;
            background: linear-gradient(135deg, #434343 0%, #000000 100%);
            padding: 20px;
            margin: 0;
        }
        .container {
            background: white;
            border-radius: 20px;
            padding: 30px;
            max-width: 700px;
            margin: 0 auto;
            box-shadow: 0 20px 60px rgba(0,0,0,0.5);
        }
        .header {
            text-align: center;
            color: #2c3e50;
            font-size: 1.8em;
            margin-bottom: 30px;
        }
        .palette {
            display: grid;
            grid-template-columns: repeat(5, 1fr);
            gap: 15px;
            margin: 30px 0;
        }
        .color-swatch {
            aspect-ratio: 1;
            border-radius: 15px;
            box-shadow: 0 5px 15px rgba(0,0,0,0.2);
            transition: transform 0.3s;
            cursor: pointer;
            position: relative;
            overflow: hidden;
        }
        .color-swatch:hover {
            transform: scale(1.1);
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
        }
        .color-code {
            position: absolute;
            bottom: 0;
            left: 0;
            right: 0;
            background: rgba(0,0,0,0.7);
            color: white;
            padding: 8px;
            font-size: 0.85em;
            text-align: center;
            font-family: 'Courier New', monospace;
        }
        .base-color {
            background: ##{color};
            color: white;
            padding: 30px;
            border-radius: 15px;
            text-align: center;
            margin-bottom: 30px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.2);
        }
        .base-color-code {
            font-size: 2em;
            font-family: 'Courier New', monospace;
            margin-top: 10px;
        }
        .info {
            background: #e7f3ff;
            border-left: 4px solid #2196F3;
            padding: 15px;
            border-radius: 5px;
            margin-top: 20px;
        }
        .badge {
            display: inline-block;
            background: #e91e63;
            color: white;
            padding: 5px 15px;
            border-radius: 20px;
            font-size: 0.85em;
            margin-top: 15px;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">💎 #{palette_name}</div>
        
        <div class="base-color">
            <div style="font-size: 1.2em; opacity: 0.9;">Base Color</div>
            <div class="base-color-code">##{color}</div>
        </div>
        
        <div class="palette">
HTML

colors.each do |hex_color|
  puts <<-HTML
            <div class="color-swatch" style="background: #{hex_color};">
                <div class="color-code">#{hex_color.upcase}</div>
            </div>
  HTML
end

puts <<-HTML
        </div>
        
        <div class="info">
            <strong>💎 Ruby Color Generator</strong><br>
            Algorithm: #{palette_type.capitalize}<br>
            Generated #{colors.length} harmonious colors from base color
        </div>
        
        <div style="text-align: center;">
            <span class="badge">✓ Ruby CGI</span>
        </div>
    </div>
</body>
</html>
HTML
