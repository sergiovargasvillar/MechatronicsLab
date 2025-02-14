#!/usr/bin/env python3

"""

The provided code establishes a comprehensive system for real-time monitoring and transmission of package number and timestamp data over LoRaWAN.
Initially, the script configures a serial reader thread to communicate with a LoStik device, ensuring seamless interaction for data exchange.
Subsequently, it sets up the LoRaWAN protocol, enabling transmission of GPS coordinates to a designated LoRa cloud server. 

The core functionality revolves around continuous monitoring of a specified folder, in this case the AERPAW UAV Results folder where a log file containing timestamp and package number data is updated. 
Upon detecting new entries in the log file, the script parses the data package_id + timestamp into Hexadecimal HEX datatype, generating an unique id for post processing.

The script dynamically adjusts the data rate for each transmission, based on the configured range specified in the config.txt file. 
The config.txt file configures various parameters for device operation. It includes settings such as LoRaWAN session keys, device addresses,
and LoRaWAN protocol settings like automatic reply state, adaptive data rate, and the number of retransmissions.

In cases where a "busy" status is returned multiple times (threshold defined in `max_busy_count`), the script 
automatically restarts to ensure continuity in data transmission.

Maintainer: Sergio Vargas, svargas3@ncsu.edu
last date updated: 01/16/2025

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

# Initialize busy count
busy_count = 0
max_busy_count = 3

def restart_script():
    logging.info("Restarting the script...")
    python = sys.executable
    os.execl(python, python, *sys.argv)

# Start reading from serial port
with ReaderThread(ser, PrintLines) as protocol:
    time.sleep(2)
    dr = config['MIN_DR']
    dr_delay = [1.5, 1.5, 1.5, 1.5, 1.5]
    f_port = config['portno']

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

                # Remove unnecessary characters from the last line to prepare for parsing
                last_line = last_line.replace('"', '').replace('(', '').replace(')', '')

		# Split the cleaned line into components using comma as the separator
                components = last_line.split(',')

                # Parse the string into a datetime object
                timestamp_obj = datetime.strptime(components[11], "%Y-%m-%d %H:%M:%S.%f")

                # Convert the datetime object to a Unix timestamp
                timestamp_ms = int(timestamp_obj.timestamp() * 1000)

                # Save the package number from the array components
                package_number = int(components[0])

                # Convert package number to hexadecimal
                package_hex = hex(package_number)[2:]  # [2:] to remove '0x' prefix

                # Convert timestamp to hexadecimal
                timestamp_hex = hex(timestamp_ms)[2:]  # [2:] to remove '0x' prefix

                # Concatenate package number and timestamp in hexadecimal format
                data_hex = package_hex + timestamp_hex

                # Log package information
                msg_to_log = "Package number: " + str(package_number) + " | Package HEX: " + str(data_hex) + " | timestamp: " + str(timestamp_ms) + " | DR: " + str(dr)
                logging.info(msg_to_log)

                # Concatenate package number and timestamp in hexadecimal format
                data_hex = package_hex + timestamp_hex

                # Set LoRaWAN data rate and send data
                protocol.send_cmd("mac set dr %d" % dr, 0.01)
                response = protocol.send_cmd("mac tx uncnf " + str(f_port) + " " + data_hex, dr_delay[dr])

                # Update data rate and package counter
                dr = dr + 1 if dr < config['MAX_DR'] else config['MIN_DR']

                # Check if the response is valid and contains "busy"
                if response is not None and "busy" in response.lower():
                    busy_count += 1
                    logging.info(f"'busy' status received {busy_count} times")
                    if busy_count >= max_busy_count:
                       logging.info("Received 'busy' status 3 times - Restarting the script.")
                       restart_script()
                else:
                     busy_count = 0  # Reset busy count on successful response


        except Exception as e:
            # Log any other exceptions
            logging.error(f"Exception: {e}")
            time.sleep(5)
            sys.exit(1)
