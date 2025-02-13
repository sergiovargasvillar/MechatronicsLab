"""
Image Capturer with Timestamp

This Python script captures an image using a connected webcam and saves it with a timestamped filename.

Key functionalities:
1. **Generates a unique timestamp** to name the image file.
2. **Uses `fswebcam`** to capture an image from the specified camera device.
3. **Stores the image** in the `/home/sergiovargas/keras-yolo3/pool_images/` directory.
4. **Prints the generated timestamp** and the output of the command execution.

This script is useful for applications requiring time-labeled image captures, such as robotic vision or surveillance.
"""

import os
import calendar
import time

# Generate a unique timestamp
ts = calendar.timegm(time.gmtime())
timestamp = str(ts)
print("Generated timestamp:", timestamp)

# Capture an image using fswebcam and save it with the timestamped filename
command = os.popen(
    'fswebcam -d v4l2:/dev/video1 -S 3 -F 2 -r 640x480 --no-banner '
    f'/home/sergiovargas/keras-yolo3/pool_images/{timestamp}.jpg'
)

# Print the output of the command execution
print("fswebcam output:", command.read())
