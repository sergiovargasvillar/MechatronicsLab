#ifndef LTE_SARA_R5_H
#define LTE_SARA_R5_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "stm32l4xx_hal.h"

extern volatile int isOK;
extern volatile int registrationStatus;
extern volatile int pingReceived;
extern volatile int pdpProfile;
extern volatile int restartConnection;


extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;

// Enumeration for AT Command Names
typedef enum {
    CREG_ENABLE = 0,
    CMEE_ENABLE,
    CFUN_ON,
    CFUN_OFF,
	CHECK_CFUN,
	GET_CCLK_OK,
	GET_CCLK,
    MQTT_SETUP_SERVER,
    MQTT_SETUP_USERNAME_PASSWORD,
    MQTT_SETUP_LAST_WILL_QOS,
    MQTT_SETUP_LAST_WILL_RETAIN,
    MQTT_SETUP_LAST_WILL_TOPIC_ID,
    MQTT_SETUP_LAST_WILL_MSG_ID,
    MQTT_SAVE_SETUP,
    ACTIVATE_PDP_CONTEXT,
    SET_PDP_TYPE_IPV4,
    SET_PDP_PARAMETERS,
	ACTIVATE_PDP_PROFILE,
	RESET_PDP_PROFILE,
    PING_GOOGLE_DNS,
    MQTT_RETRIEVE_CLIENT_PROFILE,
    MQTT_LOGIN,
    MQTT_PUBLISH_HEX_MSG,
	MQTT_PUBLISH_STRING_MSG,
    MQTT_LOGOUT,
	POWEROFF,
    // Add more AT command enums as needed
    AT_CMD_COUNT // Keep this as the last enum to represent the total number of AT commands
} ATCommandName;

typedef struct {
    const char *command;
    const char *description;
    ATCommandName name;
} ATCommand;

// Array of AT Commands
extern const ATCommand atCommands[AT_CMD_COUNT];

typedef enum {
    SEND_CREG,
    WAIT_CREG_OK,
    SEND_CMEE,
    WAIT_CMEE_OK,
	SEND_CHECK_CFUN,
	WAIT_CHECK_CFUN,
    SEND_TURN_OFF,
    WAIT_TURN_OFF_OK_CREG0,
    SEND_TURN_ON,
    WAIT_TURN_ON_CREG,
	SEND_ACTIVATE_PDP_CONTEXT,
	WAIT_ACTIVATE_PDP_CONTEXT,
	SEND_RESET_PDP_PROFILE,
	WAIT_RESET_PDP_PROFILE,
	SEND_PDP_IPV4,
	WAIT_PDP_IPV4,
	SEND_PDP_PARAMETERS,
	WAIT_PDP_PARAMETERS,
	SEND_ACTIVATE_PDP_PROFILE,
	WAIT_ACTIVATE_PDP_PROFILE,
	SEND_PING,
	WAIT_PING,
	SEND_POWEROFF,
	WAIT_POWEROFF,
    AT_ERROR,
    DONE
} State;

typedef enum {
	SEND_SERVER_NAME,
	WAIT_SERVER_NAME_OK,
	SEND_NAME_PASSWORD,
	WAIT_NAME_PASSWORD_OK,
	SEND_WILL_QOS,
	WAIT_WILL_QOS_OK,
	SEND_WILL_RETAIN,
	WAIT_WILL_RETAIN_OK,
	SEND_WILL_TOPIC,
	WAIT_WILL_TOPIC_OK,
	SEND_WILL_MSG,
	WAIT_WILL_MSG_OK,
	SEND_SAVE_NVM,
	WAIT_SAVE_NVM,
	MQTT_AT_ERROR,
	MQTT_DONE
} MQTTProfileState;

typedef enum {
	SEND_LOAD_NVM,
	WAIT_LOAD_NVM_OK,
	SEND_LOGIN_MQTT,
	WAIT_LOGIN_MQTT_OK,
	SEND_MSG_MQTT,
	WAIT_MSG_MQTT_OK,
	SEND_LOGOUT_MQTT,
	WAIT_LOGOUT_MQTT_OK,
	MQTT_MSG_AT_ERROR,
	MQTT_MSG_DONE
} MQTTSendMessageState;

typedef enum {
	SEND_GET_CCLK_OK,
	WAIT_SEND_CCLK_OK,
	SEND_CCLK,
	WAIT_CCLK_OK,
	CLK_MSG_AT_ERROR,
	CLK_MSG_DONE
} CCLKSendMessageState;

typedef enum {
    DATA_HEX_TOPIC,
	MOTH_DATA_STRING_TOPIC,
	IOT_DATA_STRING_TOPIC
    // Add more topics as needed
} MQTTTopicName;

typedef struct {
    MQTTTopicName name;
    const char *value;
} MQTTTopic;

extern volatile State currentState;
extern volatile MQTTProfileState mqttCurrentState;
extern volatile MQTTSendMessageState mqttMsgCurrentState;
extern volatile CCLKSendMessageState clkMsgState;

// Function declarations
void runLteStateMachine(State firstState);
void runMQTTSaveProfileStateMachine(MQTTProfileState ProfileState);
void runMQTTSendMsgStateMachine(MQTTSendMessageState msgState, MQTTTopicName topicName, char *data, uint16_t devID);
void runClockStateMachine(CCLKSendMessageState msgState);
void handleResponse(const char *response);
void printCREGStatus(int status);
void resetModule(void);

// Declare the callback function
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
void resetUARTCallback(UART_HandleTypeDef *huart);

// Declare other functions like handleResponse
void handleResponse(const char *response);

void initializeUARTDMAReception(void);

void sendATCommand(ATCommandName commandName);
void sendATCommandP(ATCommandName commandName, MQTTTopicName topicName, char *hexData);
void SARA_R5_powerOn(void);
void runCheckModule(State firstState);
void runPowerOFFModule(State firstState);


#endif /* LTE_SARA_R5_H */
