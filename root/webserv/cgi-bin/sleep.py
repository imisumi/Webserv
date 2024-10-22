#!/usr/bin/env python3

import cgi
import cgitb
import sys
import os
import html  # Use html.escape instead of deprecated cgi.escape

# Enable debugging
cgitb.enable()

# try:
    # Output the HTTP headers with proper CRLF (carriage return + line feed)
print("Content-Type: text/html\r\n\r\n")

# Output the HTML content
print("<html>")
print("<head><title>Hello, CGI!</title></head>")
print("<body>")
print("<h1>Hello, CGI!</h1>")
print("<p>This is a Python CGI script.</p>")

request_method = os.environ.get('REQUEST_METHOD', 'GET')
if request_method == 'POST':
	print("<h2>Request Body:</h2>")
	content_type = os.environ.get('CONTENT_TYPE', '')

	# Read the content length (how much data is being posted)
	content_length = os.environ.get('CONTENT_LENGTH')
	print(f"<p>Content Length: {content_length}</p>")
	print(f"<p>Content Type: {content_type}</p>")

	if content_length:
		# Read the request body
		request_body = sys.stdin.read(int(content_length))
		print(f"<pre>{html.escape(request_body)}</pre>")

print("</body>")
print("</html>")

# except BrokenPipeError:
#     # This error occurs if the client disconnects before the response is finished
#     sys.stderr.write("BrokenPipeError: Client disconnected before response could be completed.\n")
