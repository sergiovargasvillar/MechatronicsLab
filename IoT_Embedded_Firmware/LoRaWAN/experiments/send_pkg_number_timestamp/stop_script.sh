#!/bin/bash

# Name of the screen session
SESSION_NAME="send_loRaWAN_pkg"

# Check if the screen session exists
if screen -list | grep -q "$SESSION_NAME"; then
  echo "Stopping the script in screen session '$SESSION_NAME'..."
  
  # Send Ctrl+C to stop the Python script
  screen -S $SESSION_NAME -X stuff "^C"

  # Sleep for a short while to ensure the script has stopped
  sleep 2

  # Close the screen session
  screen -S $SESSION_NAME -X quit

  echo "Screen session '$SESSION_NAME' has been stopped and closed."
else
  echo "No screen session named '$SESSION_NAME' found."
fi
