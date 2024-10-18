#!/usr/bin/env python3

import cgi
import cgitb
import time  # Import the time module for sleep functionality

# Enable debugging
cgitb.enable()

# Introduce a delay of 10 seconds
# time.sleep(3)

# Output the HTTP headers with proper CRLF (carriage return + line feed)
print("Content-Type: text/html\r\n\r\n")

# Output the HTML content
print("<html>")
print("<head><title>Hello, CGI!</title></head>")
print("<body>")
print("<h1>Hello, CGI!</h1>")
print("<p>This is a Python CGI script.</p>")
print("</body>")
print("</html>")
