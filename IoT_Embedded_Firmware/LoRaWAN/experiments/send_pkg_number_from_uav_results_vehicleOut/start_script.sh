#!/bin/bash

# Name of the screen session
SESSION_NAME="send_loRaWAN_pkg_vehicle"

# Path to your Python script
SCRIPT_PATH="send_pkg_number_timestamp_vehicle_lostik.py"

# Create a new screen session and run the Python script
screen -dmS $SESSION_NAME bash -c "python3 $SCRIPT_PATH; exec bash"

# Check if the screen session was created successfully
if screen -list | grep -q "$SESSION_NAME"; then
  echo "Screen session '$SESSION_NAME' started successfully."
  echo "Use 'screen -r $SESSION_NAME' to reattach to the session."
else
  echo "Failed to start screen session '$SESSION_NAME'."
fi

