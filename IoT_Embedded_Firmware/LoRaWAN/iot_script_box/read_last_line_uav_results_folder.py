import os
import time
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

"""
Real-time monitoring of a specified folder for changes, focusing on modifications to files ending with 'vehicleOut.txt'. inside the Results folder.
Using the watchdog library, the script utilizes a custom MyHandler class to handle filesystem events, particularly file modifications.
Within this class, the on_modified method is invoked upon file modification, extracting the most recent line from the modified file.
Auxiliary functions, namely get_last_line and get_last_modified_file, assist in retrieving the last line of a file and determining 
the most recently modified file in a folder, respectively. 
The script operates within a continuous loop, periodically checking for updates every second.
Upon detecting a change, it yields the latest line, ensuring that only new lines are processed.

Maintainer: Sergio Vargas, svargas3@ncsu.edu
last date updated: 03/05/2024

"""

# Function to monitor a folder for changes
def monitor_folder(folder_to_watch):
    # Define a handler for filesystem events
    class MyHandler(FileSystemEventHandler):
        def __init__(self):
            super().__init__()
            self.last_line = None
            self.yielded_line = None

        # This method is called when a file is modified
        def on_modified(self, event):
            # Ignore directories
            if event.is_directory:
                return

            # Only act on files that end with 'vehicleOut.txt'
            if event.event_type == 'modified' and event.src_path.endswith('vehicleOut.txt'):
                # Get the directory of the modified file
                directory = os.path.dirname(event.src_path)
                # Get the most recently modified file in the directory
                last_modified_file = get_last_modified_file(directory)
                # Get the last line of the file
                self.last_line = get_last_line(last_modified_file)

    # Function to get the last line of a file
    def get_last_line(file_path):
        with open(file_path, 'r') as file:
            lines = file.readlines()
            # If the file has lines, return the last one
            if lines:
                return lines[-1].strip()
            else:
                return None

    # Function to get the most recently modified file in a folder
    def get_last_modified_file(folder_path):
        # Get a list of all files in the folder that end with 'vehicleOut.txt'
        files = [f for f in os.listdir(folder_path) if f.endswith('vehicleOut.txt')]
        # Create full paths for all the files
        full_paths = [os.path.join(folder_path, f) for f in files]
        # Return the file with the most recent modification time
        return max(full_paths, key=os.path.getmtime, default=None)

    # Create an observer to monitor the filesystem
    observer = Observer()
    # Create an instance of the handler
    event_handler = MyHandler()
    # Schedule the observer to call the handler when a file is modified
    observer.schedule(event_handler, path=folder_to_watch, recursive=False)
    # Start the observer
    observer.start()

    try:
        while True:
            time.sleep(1)
            # Continuously yield the latest line
            if event_handler.last_line and event_handler.last_line != event_handler.yielded_line:
                event_handler.yielded_line = event_handler.last_line
                yield event_handler.last_line
    except KeyboardInterrupt:
        observer.stop()
    observer.join()
