#!/usr/bin/env python3

"""
This script is designed to interact with a LoRaWAN module (specifically RN2903) via a serial port connection. It sends commands to the module and logs any responses or exceptions.

The script performs the following actions:
1. Sets the current working directory to the directory where the script is located.
2. Imports necessary modules and libraries, including os, serial, time, sys, enum, datetime, and custom modules from the project directory.
3. Reads configuration settings from a file named 'config.txt'.
4. Opens a serial connection to the LoRaWAN module using the specified port and baud rate.
5. Defines a function to set up logging to a file named 'logfile.log'.
6. Initiates a reader thread to handle incoming data from the serial port.
7. Enters a loop to continuously prompt the user for commands to send to the LoRaWAN module.
8. Sends the user input as commands to the module via the serial port.
9. Logs any exceptions related to the serial port connection.

Maintainer: Sergio Vargas, svargas3@ncsu.edu
last date updated: 05/06/2024
"""

import os
import serial
import time
import sys
import logging

# Set the current working directory to the script's directory
script_dir = os.path.dirname(os.path.realpath(__file__))
os.chdir(script_dir)

# Insert the iot_script_box directory into sys.path
sys.path.insert(1, os.path.abspath('../../iot_script_box'))

from enum import IntEnum
from datetime import datetime
from config import read_config_file
from lora_connection import PrintLines, ConnectionState
from serial.threaded import ReaderThread
from logger import setup_logger

# Define a function to set up logging
def setup_logger():
    # Configure logging parameters
    logging.basicConfig(filename='logfile.log', level=logging.INFO,
                        format='%(asctime)s - %(levelname)s - %(message)s')

setup_logger()

config = read_config_file('config.txt')
ser = serial.Serial(config['LORA_PORT'], baudrate = config['LORA_BAUD'])


with ReaderThread(ser, PrintLines) as protocol:
    time.sleep(2)
    while True:
      if protocol.state != ConnectionState.CONNECTED:
        logging.info("Not Connected to LoRaWAN Network...")
        time.sleep(1)
        continue
      try:
        test = input("waiting for RN2903 command\n")
        protocol.send_cmd(test)
        # -------------------------
        #protocol.send_cmd("mac set dr %d" % dr,1)                               
        #protocol.send_cmd("mac tx uncnf 1 " + merged_coordinates, dr_delay[dr])
                
      except Exception as e:
          # Log any other exceptions
          logging.error(f"Exception: {e}")
          time.sleep(5)
          sys.exit(1)
