"""
YOLO Image and Video Detector

This script performs object detection using the YOLO deep learning model. It supports both image
and video detection modes, automatically processing images from a monitored directory.

Key functionalities:
1. **Monitors a directory** (`/home/sergiovargas/keras-yolo3/pool_images/`) for new image files.
2. **Processes newly detected images** using the YOLO object detection model.
3. **Saves processed results** to `/home/sergiovargas/results/` with the same filename.
4. **Supports both image and video detection modes** via command-line arguments.

Usage:
- To run in image detection mode, provide `--image` flag.
- To run in video detection mode, provide `--input <video_path>`.
- Specify YOLO model, anchor, and class definitions as optional arguments.

"""

import sys
import argparse
import numpy as np
import cv2
from yolo import YOLO, detect_video
from PIL import Image
import os
import time
from inotify import adapters

def detect_images(yolo):
    """
    Monitors a directory for new images, processes them with YOLO,
    and saves the detected objects' results in the output directory.
    """
    i = adapters.Inotify()
    i.add_watch('/home/sergiovargas/keras-yolo3/pool_images/')  # Directory to monitor
    
    print("Ready! Waiting for new images...")
    
    for event in i.event_gen():
        if event is not None:
            (header, type_names, watch_path, filename) = event
            
            # Detect new images when file is closed after writing
            if type_names == ['IN_CLOSE_WRITE']:
                print("New image detected:", watch_path + filename)
                
                # Load and process image with YOLO
                image = Image.open(watch_path + filename)
                processed_image = yolo.detect_image(image)
                
                # Save processed image
                result_path = f"/home/sergiovargas/results/result_{filename}"
                result_array = np.asarray(processed_image)
                cv2.imwrite(result_path, result_array)
                
                print("Processed image saved at:", result_path)

    yolo.close_session()

if __name__ == '__main__':
    # Set up argument parser
    parser = argparse.ArgumentParser(description="YOLO Image and Video Detector")
    
    parser.add_argument('--model', type=str, help='Path to model weight file')
    parser.add_argument('--anchors', type=str, help='Path to anchor definitions')
    parser.add_argument('--classes', type=str, help='Path to class definitions')
    parser.add_argument('--gpu_num', type=int, help='Number of GPUs to use')
    
    # Detection mode selection
    parser.add_argument('--image', default=False, action="store_true", help='Enable image detection mode')
    parser.add_argument('--input', type=str, required=False, help='Path to video input file')
    parser.add_argument('--output', type=str, default="", help='Optional output video path')
    
    FLAGS = parser.parse_args()
    
    if FLAGS.image:
        print("Running in image detection mode...")
        detect_images(YOLO(**vars(FLAGS)))
    elif FLAGS.input:
        print(f"Running video detection on: {FLAGS.input}")
        detect_video(YOLO(**vars(FLAGS)), FLAGS.input, FLAGS.output)
    else:
        print("Error: Must specify either --image or --input <video_path>. Use --help for details.")
