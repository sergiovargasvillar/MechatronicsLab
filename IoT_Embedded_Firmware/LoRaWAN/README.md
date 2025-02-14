# Directory Structure Overview

This repository contains scripts and configurations related to LoRaWAN data transmission and monitoring, organized into various directories as follows:

## iot_script_box

This directory contains setup and utilities for LoRaWAN connection. Key components include:

- `config.py`: A Python script to read preconfigured settings.
- `logger.py`: A logger script to generate logs with different severity levels (warn, info, error).
- `lora_connection.py`: Setup for LoRaWAN connection.

## experiments

This directory houses experiments and scripts related to LoRaWAN data transmission experiments. Notable items include:

- `gps_lora_data_transfer`: An experiment directory containing scripts for GPS data transmission over LoRaWAN.
  - `config.txt`: Configuration file for preconfigured settings such as port, baudrate, etc.
  - `send_location_dr_lostik.py`: Python script for transmitting GPS location data over LoRaWAN.
  - `read_location_from_results_folder.py`: Script for reading location data from results folders.
- `old_scripts`: A directory containing older scripts related to LoRaWAN experiments.

## grafana

This directory holds Grafana dashboard configurations and SQL queries used for visualization and analysis. Components include:

- `grafana_dashboard`: Grafana dashboard configurations.
- `querys_SQL`: SQL queries used in Grafana for data analysis.

# Project Directory Structure

``` 
.
|-- README.md
|-- __pycache__
|   |-- config.cpython-310.pyc
|   |-- logger.cpython-310.pyc
|   |-- lora_connection.cpython-310.pyc
|   `-- readGpsLogs.cpython-310.pyc
|-- experiments
|   |-- gps_lora_data_transfer
|   |   |-- __pycache__
|   |   |   |-- readGpsLogs.cpython-310.pyc
|   |   |   `-- read_location_from_results_folder.cpython-310.pyc
|   |   |-- config.txt
|   |   |-- logs
|   |   |   `-- 2024-03-05.log
|   |   |-- pipPackages
|   |   |   |-- pynmea2-1.19.0-py3-none-any.whl
|   |   |   |-- pyserial-3.4-py2.py3-none-any.whl
|   |   |   `-- watchdog-4.0.0-py3-none-manylinux2014_x86_64.whl
|   |   |-- read_location_from_results_folder.py
|   |   `-- send_location_dr_lostik.py
|   `-- old_scripts
|       |-- callGps.py
|       `-- gpsLora.py
|-- grafana
|   |-- grafana_dashboard
|   `-- querys_SQL
`-- iot_script_box
    |-- __pycache__
    |   |-- config.cpython-310.pyc
    |   |-- logger.cpython-310.pyc
    |   `-- lora_connection.cpython-310.pyc
    |-- config.py
    |-- logger.py
    `-- lora_connection.py
```

Please refer to individual directories and scripts for detailed usage instructions and configurations.

---

### Installation and Setup Instructions

Before using the scripts, please follow these steps to set up the environment:

### Online Installation:

```
pip3 install watchdog pynmea2 pyserial==3.4
```
### Offline Installation:
```
sudo pip3 download --dest . pyserial==3.4
sudo pip3 download --dest . pynmea2
sudo pip3 download --dest . watchdog
```
### Identify LoStik Connection Port
```
sudo dmesg | grep tty
ls /dev/ttyUSB*
```
### Example output:

Bus 003 Device 005: ID 1a86:7523 QinHeng Electronics CH340 serial converter
```
/dev/ttyUSB0
```
### Prevent BRLTTY Service from Starting
#### /dev/ttyUSB0 not present in Ubuntu 22.04
We need to disable BRLTTY because it prevents us from connecting to the LoStik dongle. BRLTTY is a program designed to provide access to the Linux/Unix console for visually impaired individuals using a refreshable braille display. However, in our case, its operation interferes with the functionality of the LoStik dongle.
```
sudo systemctl mask brltty.path

```
### Or if you want to remove the brltty service:
```
sudo apt autoremove brltty
```
Replug LoStik and Update Configurations
Unplug and plug the LoStik again, then update the port connection in the config.txt file to match the identified port.

```
ls /dev/ttyUSB*

/dev/ttyUSB0
```

Update the config.txt file with the correct port connection (/dev/ttyUSB0 or /dev/ttyUSB1).


### Logger

The logs will show like this example:

```
INFO - 19:25:20 - mac set devaddr 00dda0f5
INFO - 19:25:21 - mac set appskey fd3add07a556a0eb8d9e83de5e165518
INFO - 19:25:21 - mac set nwkskey d9a9ba84de42adc98c44dc3fb79665f5
INFO - 19:25:22 - mac join abp
INFO - 19:25:24 - Setting init variables...
INFO - 19:25:24 - mac set ar off
INFO - 19:25:25 - mac set pwridx 5
INFO - 19:25:25 - mac set class a
INFO - 19:25:26 - mac set retx 7
INFO - 19:25:26 - mac set adr on
INFO - 19:25:27 - Done...
INFO - 19:25:27 - UPDATING STATE to connected
INFO - 19:25:35 - mac set dr 0
INFO - 19:25:36 - mac tx uncnf 1 786961699035727329
INFO - 19:25:37 - Package 1
INFO - 19:25:44 - mac set dr 1
INFO - 19:25:45 - mac tx uncnf 1 786961699035727329
```

### Postgres dB

To access the Postgres DB Container use the following commands:

```
sudo docker exec -it tenant-postgres-1 bash
psql -U chirpstack_as_events
```

For more information, visit the [LoStik GitHub repository](https://github.com).

### Node RED

### Pre installed packages:

ChirpStack provides a package called node-red-contrib-chirpstack which provides several Node-RED nodes that will help you to integrate with ChirpStack.

Install in the NodeRED container the following package:
```
npm install @chirpstack/node-red-contrib-chirpstack
```

## Important commands if AERPAW06 server is restarted
### Run locally the following commands in the AERPAW06 server:
```
sudo ip addr flush dev eno1
sudo dhclient -v eno1
```

### review if the folder permissions are for root:

Change ownership of the folders to root:

```
sudo chown -R root:root chirpstack-gateway-bridge
sudo chown -R root:root nats
```

Change permissions to ensure readability:

You can ensure that the folders and files are readable by the root user. For simplicity, you can set the permissions to allow read and execute access for others (the root user in the container):

```
sudo chmod -R 755 chirpstack-gateway-bridge
sudo chmod -R 755 nats
```

### Connect to the server and check if all the containers are up, if some of the containers are restarting, change to AERPAWOPS user, and go to the following directory:

```
cd /home/aerpawops/aerpaw-lorawan-infrastructure/infrastructure/testbed
docker-compose down
docker-compose up -d
/home/aerpawops/aerpaw-lorawan-infrastructure/infrastructure/tenant
docker-compose down
docker-compose up -d
/home/aerpawops/aerpaw-lorawan-infrastructure/infrastructure/tenant2
```

##Bind the NodeRED tenant containers to the testbed Network
```
docker network connect testbed_default a06e0050551c
```
TODO:  update the network config in the docker-compose files:
EXAMPLE:
version: '3'
services:
  service1:
    image: your_image1
    networks:
      - network1
      - network2

  service2:
    image: your_image2
    networks:
      - network2

networks:
  network1:
    driver: bridge
  network2:
    driver: bridge
    
## Common problems
### If you have permission problems when you are creating a new service container try changing the permissions:
```

chmod 755 ./{{"service_container_volume_folder"}}
drwxr--r--    2 1000     1000          4096 


```
