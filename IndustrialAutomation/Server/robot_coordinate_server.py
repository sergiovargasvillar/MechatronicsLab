"""
Robot Coordinate Server

This Python script implements a TCP/IP socket server that communicates with an ABB industrial robot.
The server performs the following tasks:

1. **Establishes a socket connection** to listen for incoming connections from the ABB robot.
2. **Receives commands from the robot** to determine the next action (e.g., requesting coordinates).
3. **Executes an external script** (`take_picture.py`) to capture an image when requested.
4. **Monitors a directory** (`/home/pi/tfm/keras-yolo3/coordinates/`) for new coordinate files.
5. **Extracts the latest X and Y coordinates** from the most recent file and sends them back to the robot.
6. **Handles errors and ensures smooth communication**, closing connections properly when needed.

This script is essential for dynamic coordination between the robot and an external vision system.
"""

import socket
import sys
import time
import os
from inotify import adapters

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

# Define server IP address and port
server_address = ('192.168.0.158', 1026)
print('Starting up on {} port {}'.format(*server_address))

# Bind the socket to the specified address and port
sock.bind(server_address)

# Listen for incoming connections (only one client at a time)
sock.listen(1)

while True:
    # Initialize an inotify adapter to monitor file changes
    i = adapters.Inotify()
    
    # Wait for a client to connect
    print('Waiting for a connection')
    connection, client_address = sock.accept()
    
    try:
        print('Connection established with', client_address)
        
        # Keep receiving data from the client
        while True:
            data = connection.recv(16)  # Receive up to 16 bytes of data
            data = data.decode("utf-8")  # Decode received data to string
            print("Received:", data)
            
            if data == "hello server":
                print('Responding to ABB robot')
                connection.sendall(b'wsupabb!')  # Send acknowledgment
            
            elif data == "coordinates!":
                print('Sending coordinates to', client_address)
                
                # Execute an external Python script to take a picture
                myCmd = os.popen('python3 take_picture.py').read()
                print(myCmd)
                
                # Monitor a specific directory for new coordinate files
                i.add_watch('/home/pi/tfm/keras-yolo3/coordinates/')  
                print("Ready! Waiting for new robot coordinates...")
                
                # Wait for new coordinate file
                for event in i.event_gen():
                    if event is not None:
                        (header, type_names, watch_path, filename) = event
                        
                        # If a file is closed after writing, process it
                        if type_names == ['IN_CLOSE_WRITE']:
                            print("New coordinate detected!", watch_path + filename)
                            
                            # Open and read the last line of the file (coordinate data)
                            with open(watch_path + filename, "r") as fileHandle:
                                lineList = fileHandle.readlines()
                            
                            print("Coordinate list:", lineList)
                            print("Last recorded coordinate:", lineList[-1])
                            
                            # Extract X and Y values
                            xy = lineList[-1].split(" ")
                            x = "X" + xy[0]
                            y = "Y" + xy[1]
                            
                            print("Sending coordinates - X:", x, "Y:", y)
                            connection.sendall(x.encode('ascii'))
                            connection.sendall(y.encode('ascii'))
            
            else:
                print('No valid data received, closing connection with', client_address)
                break
                
    finally:
        # Close the connection and socket
        connection.close()
        sock.close()
