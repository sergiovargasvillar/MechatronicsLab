# CSC-ECE591-ROS-final-project

# RoboGesture

**Team Members**: Michael Dacanay and Sergio Vargas Villar

**Course**: CSC 591 Software for Robotics

**Date**: 2/27/2024

## Summary

We manipulate a simulated rover through hand gestures. Neural networks analyze these gestures and relay the corresponding commands to the robot. This bridges the gap between the real world and simulation.

## Motivation

The motivation for this project is to explore the potential of using hand gestures as a natural and intuitive way of controlling a simulated rover. This could have several benefits, such as:

- Enhancing the user experience and ergonomics by allowing the user to interact with the rover in a more immersive and expressive manner.
- Reducing the complexity of using traditional interfaces, such as keyboards, mice, or joysticks, which may not be suitable or available in certain scenarios or environments.

Like the Apple Vision Pro hand gestures interface has shown, hand gestures are one of the interfaces for devices in the future, especially the metaverse.

## Challenge

Connecting the real-world environment to simulation (Gesture mapping and communication). The step for training the neural network to recognize and classify the gestures will require some time and development. Also, integration of the camera hardware sensor to the simulation environment as this is something that we have not done before.

## Expected Approach

We will use a Python-based program that will capture the images of the user’s hand from a webcam and process them using a machine learning model that can detect, track, and recognize hand gestures.

For the neural network, we will add different classes for each gesture that we want to use, such as forward, backward, left, right, stop, etc. We will provide at least 20 samples of each gesture using the webcam or by uploading the files from the local folder. Next, we’ll train the model using the data that we have collected, then finetune and validate. We will export the model to ROS2 with a simulation environment like RViZ or Gazebo.

## Related Work

- **Improvement in Design of Gesture Control Rover**: This paper describes a rover that is controlled by a hand glove with an accelerometer attached to it. The accelerometer detects the motion of the hand and sends the signal to an Arduino mega, which then communicates with a Raspberry Pi that controls the DC motors and servomotors of the rover. The rover also has a video camera that provides feedback to the user. The rover uses a rocker-bogie mechanism to increase its stability and mobility.
- **A Survey on Hand Gesture Controlled Rover**: This paper reviews various techniques and methods for using hand gestures to control rovers, such as image processing, computer vision, machine learning, and wireless communication. The paper also discusses the advantages and challenges of using hand gestures, such as naturalness, intuitiveness, complexity, and variability. The paper also suggests some future directions and applications for gesture-controlled rovers, such as space exploration, disaster management, and entertainment.
- **Gesture Controlled Mars Rover**: This project demonstrates how to use the Machine Learning Environment to create and train a custom hand gesture classifier using a graphical interface. The project uses the Hand Classifier extension of the Machine Learning Environment, which analyzes the hand position using 21 data points. The project maps the recognized gestures to the corresponding actions or commands for the rover, such as move forward, turn left, stop, etc.

## Milestones

| Name or # | Start Date | End Date | Description |
| --- | --- | --- | --- |
| 1 | 03/05/24 | 03/06/24 | Designing and planning. Develop a detailed project plan. Create wireframes, mockups, or architectural diagrams. |
| 2 | 03/01/24 | 03/06/24 | Data collection and preprocessing: Capture and label the images of the hand gestures using the webcam or the local folder. |
| 3 | 03/06/24 | 03/06/24 | Start coding or building the solution. Deploy infrastructure or software components. |
| 4 | 03/05/24 | 03/09/24 | Model testing and evaluation: Test the model on new data and evaluate its accuracy and performance. |
| 5 | 03/09/24 | 03/14/24 | Simulation setup and visualization: Install and configure ROS 2, rviz2, or Gazebo to simulate the rover and visualize the data from the camera. |
| 6 | 04/14/24 | 04/15/24 | Simulation testing and demonstration: Run the simulation and use the hand gestures to control the rover. Record a video of the simulation and prepare a presentation of the project results. |

Some of the milestones/tasks will be done in parallel.

## Materials Needed

- Camera
- Computer (Ubuntu VM)
- Python is our language of choice, rospy library
- Simulation environment: RViz/Gazebo

## References

[1] Rajguru, et. al. "Improvement in Design of Gesture Control Rover," International Journal of Scientific & Engineering Research Volume 10, Issue 5, May-2019. Link

[2] J.L. Raheja, G.A. Rajsekhar, A. Chaudhary, “Controlling a remotely located Robot using Hand Gestures in real time: A DSP implementation”, 5th IEEE International conference on Wireless Network and Embedded Systems, India, Oct 16-17, 2016, pp. 1-5. Link

[3] J. L. Raheja, R. Shyam, U. Kumar and P. B. Prasad, "Real-Time Robotic Hand Control Using Hand Gestures," 2010 Second International Conference on Machine Learning and Computing, Bangalore, India, 2010, pp. 12-16, doi: 10.1109/ICMLC.2010.12. Link

[4] Jared Floersch, Perry Y. Li, "Human Gesture Robot Control Using a Camera/Accelerometer-in-Palm Sensor," IFAC-PapersOnLine, Volume 54, Issue 20, 2021. Link

