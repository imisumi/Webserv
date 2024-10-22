#!/usr/bin/env python3

import os
import cgi
import cgitb
import datetime

# Enable debugging
cgitb.enable()

# Retrieve PATH_INFO
path_info = os.getenv('PATH_INFO', '')

# Extract the directory from PATH_INFO
save_directory = path_info.lstrip('/')

# Ensure the directory exists
if not os.path.exists(save_directory):
    os.makedirs(save_directory)

# Create a unique filename based on the current timestamp
timestamp = datetime.datetime.now().strftime('%Y%m%d%H%M%S')
filename = f"submission_{timestamp}.txt"
file_path = os.path.join(save_directory, filename)

# Get the form data
form = cgi.FieldStorage()

# Extract data from the form
data = form.getvalue('data', 'No data provided')

# Save the data to the file
with open(file_path, 'w') as file:
    file.write(data)

# Output the HTTP headers with proper CRLF (carriage return + line feed)
print("Content-Type: text/html\r\n\r\n")

# Output the HTML content
print("<html>")
print("<head><title>Submission Received</title></head>")
print("<body>")
print("<h1>Submission Received</h1>")
print(f"<p>Data has been saved to <code>{file_path}</code>.</p>")
print("</body>")
print("</html>")