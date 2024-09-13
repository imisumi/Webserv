#!/usr/bin/env python3

import os
import cgi
import cgitb

# Enable debugging
cgitb.enable()

# Directory to list
directory = "/home/kaltevog/Desktop/Webserv/database"

print("Content-Type: text/html\r\n\r\n")
# Output the HTML content
print("<html>")
print("<head><title>File List with Delete</title></head>")
print("""
    <script>
        function deleteFile(filename) {
            if (confirm('Are you sure you want to delete ' + filename + '?')) {
                fetch('/delete/' + encodeURIComponent(filename), {
                    method: 'DELETE'
                }).then(response => {
                    if (response.ok) {
                        alert('File deleted successfully.');
                        window.location.reload();  // Reload the page after deletion
                    } else {
                        alert('Failed to delete file.');
                    }
                }).catch(error => {
                    console.error('Error deleting file:', error);
                });
            }
        }
    </script>
""")
print("</head>")
print("<body>")
print("<h1>Files in Directory</h1>")
print("<ul>")

# Check if the directory exists
if os.path.exists(directory):
    # List files in the directory
    for filename in os.listdir(directory):
        file_path = os.path.join(directory, filename)
        if os.path.isfile(file_path):
            # Output the file name with a delete button that sends a DELETE request using JavaScript
            print(f"<li>{filename} "
                  f"<button onclick=\"deleteFile('{filename}')\">Delete</button>"
                  f"</li>")
else:
    print("<p>Directory does not exist.</p>")
print("</ul>")

# Add the Go Back button at the bottom
print("""
    <button id="goBackButton" style="margin-top: 10px;">Go Back</button>
    <script>
        document.getElementById('goBackButton').addEventListener('click', function() {
            window.history.back();
        });
    </script>
""")

print("</body>")
print("</html>")