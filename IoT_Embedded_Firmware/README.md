# LTE-MQTT Firmware for STM32

## Overview
This library provides **LTE and MQTT communication capabilities** for STM32 microcontrollers, enabling IoT applications that require reliable wireless connectivity. The firmware is designed for **embedded systems** using STM32Cube and HAL libraries, interfacing with an external **LTE module (u-blox SARA R5)** to establish MQTT-based communication over cellular networks.

## Features
- **AT Command Communication** - Sends AT commands to control the LTE module.
- **UART DMA Reception** - Uses DMA for efficient serial communication.
- **LTE Network Registration** - Manages cellular network connectivity.
- **MQTT Client Integration** - Publishes messages and handles subscriptions.
- **Clock Synchronization** - Retrieves real-time clock via AT commands.
- **State Machines for LTE & MQTT** - Ensures robust connection handling.
- **Low Power Support** - Optimized for embedded applications.

## Requirements
### Hardware
- STM32 microcontroller (e.g., STM32L5 series)
- u-blox SARA R5 LTE module

### Software
- STM32CubeIDE
- HAL (Hardware Abstraction Layer) Libraries
- FreeRTOS (Optional for multitasking)
- MQTT Broker (e.g., Mosquitto)

## Installation
### 1. Clone Repository
```bash
git clone https://github.com/your-repo/LTE-MQTT-Firmware.git
```

### 2. Open Project in STM32CubeIDE
- Import the project into STM32CubeIDE.
- Configure **UART3** for LTE module communication.

### 3. Flash Firmware
- Build the project.
- Flash the firmware to the STM32 board.

## Usage
### 1. Initialize LTE Module
Call the function to start the LTE connection process:
```c
runLteStateMachine(SEND_CREG);
```

### 2. Establish MQTT Connection
```c
runMQTTSaveProfileStateMachine(SEND_SERVER_NAME);
runMQTTSendMsgStateMachine(SEND_LOAD_NVM, "payload_data");
```

### 3. Send MQTT Message
```c
sendATCommandP(MQTT_PUBLISH_HEX_MSG, DATA_HEX_TOPIC, "test message");
```

### 4. Handle Incoming Data
UART responses are automatically processed in:
```c
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
```

## Troubleshooting
### Common Issues & Solutions
- **Module Not Connecting?** Check power connections and antenna.
- **No MQTT Response?** Verify broker settings and APN configuration.
- **UART Not Receiving?** Ensure correct baud rate and DMA settings.

## License
This project is licensed under the **MIT License**. See `LICENSE` for details.


