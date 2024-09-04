#!/usr/bin/env python3

import cgi
import cgitb

# Enable debugging
cgitb.enable()

# Output the HTTP headers
print("Content-Type: text/html\n")

# Output the HTML content
print("<html>")
print("<head><title>Hello, CGI!</title></head>")
print("<body>")
print("<h1>Hello, CGI!</h1>")
print("<p>This is a Python CGI script.</p>")
print("</body>")
print("</html>")
