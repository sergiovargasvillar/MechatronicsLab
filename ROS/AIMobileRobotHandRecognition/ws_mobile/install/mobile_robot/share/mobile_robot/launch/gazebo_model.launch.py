import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription 
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource


from launch_ros.actions import Node
import xacro


def generate_launch_description():
    
    #Name has to match the robot name in the xacro file
    robotXacroName='differential_drive_robot'
    
    #Name of the package, name of the folder
    #Used to define the paths
    namePackage = 'mobile_robot'
    
    # Relative path to the xacro file, defining the model
    modelFileRelativePath = 'model/robot.xacro'
    
    # Relative path Gazebo world file
    worldFileRelativePath = 'model/empty_world.world'

    # Absolute path to the model
    pathModelFile = os.path.join(get_package_share_directory(namePackage),modelFileRelativePath)

    # Absoulute path to the world model
    pathWorldFile = os.path.join(get_package_share_directory(namePackage),worldFileRelativePath)

    # Robot description from the xacro model file
    robotDescription = xacro.process_file(pathModelFile).toxml()

    # Launch file from the gazebo_ros package
    gazebo_rosPackageLaunch = PythonLaunchDescriptionSource(os.path.join(get_package_share_directory('gazebo_ros'),
                                                                         'launch', 'gazebo.launch.py'))

    # Launch description
    gazeboLaunch = IncludeLaunchDescription(gazebo_rosPackageLaunch, launch_arguments={'world': pathWorldFile}.items())

    # Create a gazebo_ros Node
    spawnModelNode = Node(package = 'gazebo_ros', executable='spawn_entity.py',
                          arguments=['-topic', 'robot_description','-entity', robotXacroName],output='screen')

    # Robot State Publisher Node
    nodeRobotStatePublisher = Node(
        package = 'robot_state_publisher',
        executable = 'robot_state_publisher',
        output = 'screen',
        parameters=[{'robot_description': robotDescription,
        'use_sim_time': True}]
    )


    # Create an empty launch description object
    launchDescriptionObject = LaunchDescription()

    # Add gazeboLaunch
    launchDescriptionObject.add_action(gazeboLaunch)
    
    # Add 2 nodes
    launchDescriptionObject.add_action(spawnModelNode)
    launchDescriptionObject.add_action(nodeRobotStatePublisher)

    return launchDescriptionObject