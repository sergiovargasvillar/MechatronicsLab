"""
Coordinate File Watcher

This Python script continuously monitors a directory for new coordinate files.
When a new file is detected, it extracts the last recorded X and Y coordinates
and prints them to the console.

Key functionalities:
1. **Monitors a specified directory** (`/home/sergiovargas/keras-yolo3/coordinates/`) for new files.
2. **Waits for file closure events** to ensure data is fully written.
3. **Reads the last line of each new file** to extract coordinate values.
4. **Prints the extracted X and Y coordinates** for further processing.

This script is useful for real-time robotic applications that rely on external coordinate data.
"""

import numpy as np
import os
import time
from inotify import adapters

while True:
    # Initialize an inotify adapter to monitor file changes
    i = adapters.Inotify()
    i.add_watch('/home/sergiovargas/keras-yolo3/coordinates/')  # Directory to monitor
    
    print("Ready!")
    print("Waiting for new Robot coordinates...")
    
    # Listen for file system events
    for event in i.event_gen():
        if event is not None:
            (header, type_names, watch_path, filename) = event
            
            # If a file is closed after writing, process it
            if type_names == ['IN_CLOSE_WRITE']:
                print("New coordinate file detected!", watch_path + filename)
                
                # Open and read the last line of the file (coordinate data)
                with open(watch_path + filename, "r") as fileHandle:
                    lineList = fileHandle.readlines()
                
                print("File contents:", lineList)
                print("Extracting last recorded coordinate:")
                
                # Extract and parse X and Y values
                xy = lineList[-1].split(" ")
                x = xy[0]
                y = xy[1]
                
                print("X:", x, "Y:", y)
