
#!/usr/bin/env python3

import cgi
import cgitb
import time # Import the time module for sleep functionality

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
# Check if the request method is POST
# if request_method == "POST":
#     # Get the content length from the environment
#     content_length = int(os.environ.get("CONTENT_LENGTH", 0))

#     # Read the request body if content length is available
#     if content_length > 0:
#         request_body = sys.stdin.read(content_length)
#         print("<h2>Request Body:</h2>")
#         print(f"<pre>{cgi.escape(request_body)}</pre>")  # Escaping for HTML safety

# # General message for other methods or no body
# else:
#     print("<p>This is a Python CGI script.</p>")
print("</body>")
print("</html>")
 