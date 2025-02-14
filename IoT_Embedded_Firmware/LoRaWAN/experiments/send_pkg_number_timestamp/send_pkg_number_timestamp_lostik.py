#!/usr/bin/env python3

"""
This script enables real-time transmission of package numbers and timestamp data over LoRaWAN using a LoStik device.
It sets up a serial reader thread to ensure reliable communication and interaction with the LoStik hardware,
allowing data to be transmitted to a LoRa cloud server.

The main functionality of the script includes:
1. Continuously incrementing and tracking a package number, which is combined with a Unix timestamp (in milliseconds)
   and converted to hexadecimal (HEX) format for transmission.
2. Monitoring the connection state with the LoRaWAN network and, if disconnected, logging the status and retrying.

The script also dynamically adjusts the data rate (DR) for each transmission within a configured range specified 
in `config.txt`. This configuration file includes other parameters essential for device operation, such as 
LoRaWAN session keys, device addresses, and protocol settings (e.g., adaptive data rate, port numbers).

In cases where a "busy" status is returned multiple times (threshold defined in `max_busy_count`), the script 
automatically restarts to ensure continuity in data transmission.

Maintainer: Sergio Vargas, svargas3@ncsu.edu
Last updated: 01/16/2025

"""

# Import necessary modules
import os
import serial
import time
import logging
import sys
import subprocess

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
    time.sleep(4)
    dr = config['MIN_DR']
    dr_delay = [2.5, 2.5, 2.5, 2.5, 2.5]
    f_port = config['portno']
    package_number = 0

    # Main loop
    while True:
        # Check LoRaWAN connection state
        if protocol.state != ConnectionState.CONNECTED:
            logging.info("Not Connected to LoRaWAN Network...")
            time.sleep(1)
            continue

        try:

             # Convert the datetime object to a Unix timestamp

             timestamp_ms = int(time.time() * 1000)

             # timestamp_ms = int(timestamp_obj.timestamp() * 1000)

             # Save the package number from the array components
             package_number = package_number + 1

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


              # Check if the response is valid and contains "busy"
             if response is not None and "busy" in response.lower():
                 busy_count += 1
                 logging.info(f"'busy' status received {busy_count} times")
                 if busy_count >= max_busy_count:
                    logging.info("Received 'busy' status 3 times - Restarting the script.")
                    restart_script()
             else:
                  busy_count = 0  # Reset busy count on successful response

             # Update data rate and package counter
             dr = dr + 1 if dr < config['MAX_DR'] else config['MIN_DR']

        except Exception as e:
            # Log any other exceptions
            logging.error(f"Exception: {e}")
            time.sleep(5)
            sys.exit(1)
