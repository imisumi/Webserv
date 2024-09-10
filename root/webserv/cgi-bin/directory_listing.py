#!/usr/bin/env python3

import os
import argparse

def generate_directory_listing(path):
    # Make sure the path exists
    if not os.path.exists(path):
        print(f"Error: The directory '{path}' does not exist.")
        return

    # HTML Template for the directory listing with dark theme
    html_template = """
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Directory Listing</title>
        <style>
            body {{ 
                font-family: Arial, sans-serif; 
                margin: 20px; 
                background-color: #121212;
                color: #f0f0f0;
            }}
            table {{ 
                width: 100%; 
                border-collapse: collapse; 
                margin-top: 20px; 
                background-color: #1e1e1e;
            }}
            th, td {{ 
                padding: 12px 15px; 
                text-align: left; 
                border-bottom: 1px solid #333; 
            }}
            th {{ 
                background-color: #333; 
                color: #ffffff; 
                text-align: left; 
                font-weight: bold; 
            }}
            td {{ 
                color: #e0e0e0; 
            }}
            a {{ 
                text-decoration: none; 
                color: #1e90ff; 
            }}
            a:hover {{ 
                text-decoration: underline; 
            }}
            tr:hover {{ 
                background-color: #333; 
            }}
        </style>
    </head>
    <body>
        <h1>Directory Listing for: {path}</h1>
        <table>
            <thead>
                <tr>
                    <th>Name</th>
                    <th>Type</th>
                    <th>Size (Bytes)</th>
                </tr>
            </thead>
            <tbody>
                {rows}
            </tbody>
        </table>
    </body>
    </html>
    """

    # Generate rows for each file/directory in the specified path
    rows = []
    for item in os.listdir(path):
        item_path = os.path.join(path, item)
        item_type = "Directory" if os.path.isdir(item_path) else "File"
        item_size = os.path.getsize(item_path) if os.path.isfile(item_path) else "-"
        row = f"""
        <tr>
            <td><a href="{item}">{item}</a></td>
            <td>{item_type}</td>
            <td>{item_size}</td>
        </tr>
        """
        rows.append(row)
    
    # Combine all rows into a single string
    rows_html = "\n".join(rows)

    # Fill in the template with directory path and rows
    html_content = html_template.format(path=path, rows=rows_html)

    # Print the HTML content to stdout
    print(html_content)

# Main function to handle argument parsing
def main():
    parser = argparse.ArgumentParser(description="Generate HTML directory listing.")
    parser.add_argument("path", nargs="?", default="./", help="The path of the directory to list. Defaults to current directory.")
    args = parser.parse_args()

    # Print HTML content type header (for CGI)
    print("Content-Type: text/html\r\n\r\n")

    # Generate the directory listing
    generate_directory_listing(args.path)

if __name__ == "__main__":
    main()
