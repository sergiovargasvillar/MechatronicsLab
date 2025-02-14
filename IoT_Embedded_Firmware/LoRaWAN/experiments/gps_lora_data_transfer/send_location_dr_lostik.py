#!/usr/bin/env python3

"""

The provided code establishes a comprehensive system for real-time monitoring and transmission of GPS data over LoRaWAN.
Initially, the script configures a serial reader thread to communicate with a LoStik device, ensuring seamless interaction for data exchange.
Subsequently, it sets up the LoRaWAN protocol, enabling transmission of GPS coordinates to a designated LoRa cloud server. 

The core functionality revolves around continuous monitoring of a specified folder, where a log file containing GPS data is updated. 
Upon detecting new entries in the log file, the script parses the data, converting latitude and longitude coordinates into integer 
representations suitable for LoRa transmission. 

The script dynamically adjusts the data rate for each transmission, based on the configured range specified in the config.txt file. 
The config.txt file configures various parameters for device operation. It includes settings such as LoRaWAN session keys, device addresses,
and LoRaWAN protocol settings like automatic reply state, adaptive data rate, and the number of retransmissions.

Maintainer: Sergio Vargas, svargas3@ncsu.edu
last date updated: 03/05/2024

"""

# Import necessary modules
import os
import serial
import time
import logging
import sys

# Set the current working directory to the script's directory
script_dir = os.path.dirname(os.path.realpath(__file__))
os.chdir(script_dir)

# Insert the iot_script_box directory into sys.path
sys.path.insert(1, os.path.abspath('../../iot_script_box'))

# Import additional modules
from enum import IntEnum
from datetime import datetime
from config import read_config_file
from lora_connection import PrintLines, ConnectionState
from serial.threaded import ReaderThread
from logger import setup_logger
from read_last_line_uav_results_folder import monitor_folder

# Define a function to set up logging
def setup_logger():
    # Configure logging parameters
    logging.basicConfig(filename='logfile.log', level=logging.INFO,
                        format='%(asctime)s - %(levelname)s - %(message)s')

# Read configuration from config.txt
config = read_config_file('config.txt')

# Open a serial connection
ser = serial.Serial(config['LORA_PORT'], baudrate=config['LORA_BAUD'])

# Set up logging
setup_logger()

# Set the folder to watch for results
folder_to_watch = config['RESULTS_DIRECTORY']

# Monitor the specified folder for changes
folder_monitor = monitor_folder(folder_to_watch)

# Start reading from serial port
with ReaderThread(ser, PrintLines) as protocol:
    time.sleep(2)
    dr = config['MIN_DR']
    dr_delay = [1.5, 1.5, 1.5, 1.5, 1.5]
    package_counter = 0
    
    # Main loop
    while True:
        # Check LoRaWAN connection state
        if protocol.state != ConnectionState.CONNECTED:
            logging.info("Not Connected to LoRaWAN Network...")
            time.sleep(1)
            continue
            
        try:
            # Read the last line from the monitored folder
            last_line = next(folder_monitor)
            
            # Process data if available
            if last_line is not None:
                # Split the GPS data into components using comma as the separator
                components = last_line.split(',')

                # Convert the first component to a floating-point number
                first_number = float(components[0])

                # Convert the latitude coordinate to an integer representation suitable for transmission over LoRaWAN
                # by first converting it to a float, multiplying by 10^8, taking the absolute value,
                # and finally converting it to an integer
                lat_int = int(abs(float(components[1]) * 1e8))

                # Convert the longitude coordinate to an integer representation suitable for transmission over LoRaWAN
                # by first converting it to a float, multiplying by 10^6, taking the absolute value,
                # and finally converting it to an integer
                lon_int = int(abs(float(components[2]) * 1e6))

                # Merge the latitude and longitude coordinates into a single string representation
                # by concatenating their integer representations as strings
                merged_coordinates = str(lat_int) + str(lon_int)
                
                # Set LoRaWAN data rate and send data
                protocol.send_cmd("mac set dr %d" % dr, 1)
                protocol.send_cmd("mac tx uncnf 1 " + merged_coordinates, dr_delay[dr])
                
                # Update data rate and package counter
                dr = dr + 1 if dr < config['MAX_DR'] else config['MIN_DR']
                package_counter += 1
                
                # Log package information
                msg_to_log = "Package number: " + str(package_counter)
                logging.info(msg_to_log)

        except Exception as e:
            # Log any other exceptions
            logging.error(f"Exception: {e}")
            time.sleep(5)
            sys.exit(1)