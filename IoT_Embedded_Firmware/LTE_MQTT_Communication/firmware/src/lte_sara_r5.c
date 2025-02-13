/*
 * LTE-MQTT Firmware for STM32
 *
 * This embedded C firmware provides LTE and MQTT communication capabilities
 * using an STM32 microcontroller with an external LTE module.
 *
 * Key functionalities:
 * 1. **AT Command Communication** - Sends AT commands to configure and control the LTE module.
 * 2. **UART DMA Reception** - Uses DMA for efficient UART communication with the LTE module.
 * 3. **LTE Network Registration** - Manages network connectivity and PDP context activation.
 * 4. **MQTT Communication** - Establishes an MQTT connection, publishes messages, and handles responses.
 * 5. **Clock Synchronization** - Retrieves real-time clock information via AT commands.
 * 6. **State Machines for LTE & MQTT** - Implements state machines to manage LTE registration and MQTT messaging.
 *
 * This firmware is developed using STM32Cube and HAL libraries.
 */

#include <stdlib.h>
#include "lte_sara_r5.h"
#include "stm32l4xx_hal.h"
#include "main.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"

#define RX_BUF_SIZE  1024
#define MainBuf_SIZE 2048
#define MAXTIMEOUT 10

extern UART_HandleTypeDef huart1;
// Declare hrtc as extern
extern RTC_HandleTypeDef hrtc;

volatile State currentState = SEND_CREG;
volatile int isOK = 0;
volatile int checkCfun = 0;
volatile int registrationStatus = -1;
volatile int pingReceived = 0;
volatile int pdpProfile = 0;
volatile int restartConnection = 0;
volatile int clockFlag = 0;


volatile MQTTProfileState mqttCurrentState = SEND_SERVER_NAME;

volatile MQTTSendMessageState mqttMsgCurrentState = SEND_LOGIN_MQTT;

volatile CCLKSendMessageState clkMsgState = SEND_CCLK;
volatile int mqttLogin = 0;
volatile int mqttLogout = 0;
volatile int mqttMsg = 0;
volatile int mqttTimeOut = 0;
volatile int atErrorFlag = 0;

uint8_t RxBuf[RX_BUF_SIZE];
uint8_t MainBuf[MainBuf_SIZE];
uint16_t oldPos = 0;
uint16_t newPos = 0;
uint16_t rxIndex = 0;

const ATCommand atCommands[] = {
    {"AT+CREG=1\r", "Enable network registration status unsolicited result code", CREG_ENABLE},
    {"AT+CMEE=2\r", "Enable verbose error reporting", CMEE_ENABLE},
    {"AT+CFUN=1\r", "Turn on RF module", CFUN_ON},
    {"AT+CFUN=0\r", "Turn off RF module", CFUN_OFF},
    {"AT+CFUN?\r", "CHECK CFUN STATUS", CHECK_CFUN},
    {"AT+CCLK=?\r", "GET CLOCK OK TEST", GET_CCLK_OK},
    {"AT+CCLK?\r", "GET CLOCK", GET_CCLK},
    {"AT+UMQTT=2,\"test.mosquitto.org\",1883\r", "MQTT server setup", MQTT_SETUP_SERVER},
    {"AT+UMQTT=4,\"\",\"\"\r", "MQTT username and password setup", MQTT_SETUP_USERNAME_PASSWORD},
    {"AT+UMQTT=6,0\r", "MQTT last will QoS setup", MQTT_SETUP_LAST_WILL_QOS},
    {"AT+UMQTT=7,1\r", "MQTT last will retain setup", MQTT_SETUP_LAST_WILL_RETAIN},
    {"AT+UMQTT=8,\"will/iot_device/imosp/lte/cimi_id\"\r", "MQTT last will topic ID setup", MQTT_SETUP_LAST_WILL_TOPIC_ID},
    {"AT+UMQTT=9,\"234500093091850\"\r", "MQTT last will message ID setup", MQTT_SETUP_LAST_WILL_MSG_ID},
    {"AT+UMQTTNV=2\r", "Save MQTT setup in NVM", MQTT_SAVE_SETUP},
    {"AT+CGACT=1,1\r", "Activate PDP context for ping", ACTIVATE_PDP_CONTEXT},
    {"AT+UPSD=0,0,0\r", "Set PDP type to IPV4", SET_PDP_TYPE_IPV4},
    {"AT+UPSD=0,100,1\r", "Set PDP parameters", SET_PDP_PARAMETERS},
    {"AT+UPSDA=0,3\r", "Activate PDP context profile", ACTIVATE_PDP_PROFILE},
    {"AT+UPSDA=0,4\r", "RESET PDP context profile", RESET_PDP_PROFILE},
    {"AT+UPING=\"8.8.8.8\"\r", "Ping Google DNS", PING_GOOGLE_DNS},
    {"AT+UMQTTNV=1\r", "Retrieve MQTT client profile from memory", MQTT_RETRIEVE_CLIENT_PROFILE},
    {"AT+UMQTTC=1\r", "Login to MQTT Broker", MQTT_LOGIN},//max 475 bytes real or octets hex
	{"AT+UMQTTC=2,0,1,1,\"%s\",\"%s\"\r", "HEX message published to MQTT Broker", MQTT_PUBLISH_HEX_MSG},
	{"AT+UMQTTC=2,0,1,0,\"%s\",\"%s\"\r", "message pub to MQTT Broker", MQTT_PUBLISH_STRING_MSG},
    {"AT+UMQTTC=0\r", "Logout from MQTT Broker", MQTT_LOGOUT},
    {"AT+CPWROFF\r", "SARA R5 Power off...", POWEROFF},
    // Add more AT commands as needed
};

const MQTTTopic mqttTopics[] = {
    {DATA_HEX_TOPIC, "iot_device/imosp/lte/data_hex"},
    {MOTH_DATA_STRING_TOPIC, "iot_device/imosp/lte/moth_data"},
    {IOT_DATA_STRING_TOPIC, "iot_device/imosp/lte/iot_data"},
    // Add more topics as needed
};

void sendATCommand(ATCommandName commandName) {
	HAL_GPIO_TogglePin(LEDOrng_GPIO_Port, LEDOrng_Pin);

	isOK = 0;
    const ATCommand *atCommand = &atCommands[commandName];
    printf("Sending AT Command: %s\n", atCommand->description);
    printf("Sending AT Command: %s\n", atCommand->command);
    HAL_UART_Transmit(&huart1, (uint8_t *)atCommand->command, strlen(atCommand->command), HAL_MAX_DELAY);
    osDelay(200/portTICK_PERIOD_MS);
	HAL_GPIO_TogglePin(LEDOrng_GPIO_Port, LEDOrng_Pin);
    osDelay(200/portTICK_PERIOD_MS);
	HAL_GPIO_TogglePin(LEDOrng_GPIO_Port, LEDOrng_Pin);
}

void sendATCommandP(ATCommandName commandName, MQTTTopicName topicName, char *data) {
	HAL_GPIO_TogglePin(LEDOrng_GPIO_Port, LEDOrng_Pin);
	isOK = 0;
    const ATCommand *atCommand = &atCommands[commandName];
    const char *topicValue = mqttTopics[topicName].value;
    char commandBuffer[1024]; // Adjust the buffer size as needed

    printf("Sending AT Command: %s\n", atCommand->description);

    snprintf(commandBuffer, sizeof(commandBuffer), atCommand->command, topicValue, data);
    strcat(commandBuffer, data);

    __HAL_UART_FLUSH_DRREGISTER(&huart1);

    HAL_UART_Transmit(&huart1, (uint8_t *)commandBuffer, strlen(commandBuffer), HAL_MAX_DELAY);
    osDelay(100/portTICK_PERIOD_MS);
	HAL_GPIO_TogglePin(LEDOrng_GPIO_Port, LEDOrng_Pin);
    osDelay(100/portTICK_PERIOD_MS);
	HAL_GPIO_TogglePin(LEDOrng_GPIO_Port, LEDOrng_Pin);
}

void initializeUARTDMAReception(void) {
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t *)RxBuf, RX_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
}

void resetUARTDMA(UART_HandleTypeDef *huart) {
    HAL_UART_DeInit(huart);
    HAL_UART_Init(huart);
    initializeUARTDMAReception();
}

void resetUARTCallback(UART_HandleTypeDef *huart)
{
  // Stop the current UART receive process
  HAL_UART_DMAStop(huart);

  // Reinitialize the UART receive process
  if (HAL_UARTEx_ReceiveToIdle_DMA(huart, RxBuf, RX_BUF_SIZE) != HAL_OK)
  {
    // Handle error
    printf("Error reinitializing UART receive process\n");
  }
  else
  {
    printf("UART receive process reinitialized successfully\n");
  }
}

void SARA_R5_powerOn(void)
{
  //resetUARTDMA(&huart1);
  resetUARTCallback(&huart1);

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (LTEPWR_Pin >= 0)
  {
    // Turn on Blue LED to indicate power on process start
    HAL_GPIO_WritePin(LEDBlue_GPIO_Port, LEDBlue_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(LTEPWR_GPIO_Port, LTEPWR_Pin, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = LTEPWR_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LTEPWR_GPIO_Port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(LTEPWR_GPIO_Port, LTEPWR_Pin, GPIO_PIN_SET);

    osDelay(1000/portTICK_PERIOD_MS);

    // Turn off Blue LED to indicate power on pulse end
    HAL_GPIO_WritePin(LEDBlue_GPIO_Port, LEDBlue_Pin, GPIO_PIN_RESET);

    /* Configure GPIO pin : LTEPWR_Pin to high-impedance state */
    GPIO_InitStruct.Pin = LTEPWR_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(LTEPWR_GPIO_Port, &GPIO_InitStruct);

    osDelay(3000/portTICK_PERIOD_MS);// Do this in init. Wait before sending AT commands to module. 100 is too short.

    printf("Power ON complete!\n");
   // runCheckModule(SEND_CHECK_CFUN);
  //  printf("SENT CFUN CHECK!\n");
  }
}

void resetModule(void)
{
  // Set PWR_ON line to LOW
  printf("Turning OFF the Module, please wait...\n");

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if ((LTEPWR_Pin >= 0) && (MikReset_Pin >= 0))
  {
    HAL_GPIO_WritePin(MikReset_GPIO_Port, MikReset_Pin, GPIO_PIN_SET); // Start by making sure the RESET_N pin is high
    HAL_GPIO_WritePin(LTEPWR_GPIO_Port, LTEPWR_Pin, GPIO_PIN_SET); // Inverted - Asset Tracker

    // Blink LEDs during the delay
    for(int i = 0; i < 23; i++)
    {
      HAL_GPIO_TogglePin(LEDBlue_GPIO_Port, LEDBlue_Pin);
      HAL_GPIO_TogglePin(LEDOrng_GPIO_Port, LEDOrng_Pin);
      osDelay(500/portTICK_PERIOD_MS);
      HAL_GPIO_TogglePin(LEDGreen_GPIO_Port, LEDGreen_Pin);
      osDelay(500/portTICK_PERIOD_MS);
    }

    HAL_GPIO_WritePin(MikReset_GPIO_Port, MikReset_Pin, GPIO_PIN_RESET); // Now pull RESET_N low
    osDelay(100/portTICK_PERIOD_MS); // Wait a little... (The data sheet doesn't say how long for)

    HAL_GPIO_WritePin(LTEPWR_GPIO_Port, LTEPWR_Pin, GPIO_PIN_RESET); // Inverted - Asset Tracker
    osDelay(1500/portTICK_PERIOD_MS); // Wait 1.5 seconds

    HAL_GPIO_WritePin(MikReset_GPIO_Port, MikReset_Pin, GPIO_PIN_SET); // Now pull RESET_N high again

    /* Configure GPIO pins : MikReset_Pin and LTEPWR_Pin to high-impedance state */
    GPIO_InitStruct.Pin = MikReset_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(MikReset_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LTEPWR_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(LTEPWR_GPIO_Port, &GPIO_InitStruct);
  }

  printf("TURNING ON!\n");
  SARA_R5_powerOn();
}



void handleResponse(const char *response) {
   // printf("----------------------------response: %s\n", response);

    // Check for the "OK" message
    if (strstr(response, "OK") != NULL) {
        isOK = 1;
        printf("OK received.\n");
    }


    // Check for the "+CFUN:" CHECK if module is TURN ON.
    const char *cfun = strstr(response, "+CFUN:");
    if (cfun != NULL) {
        int cFunFlag;
        int regStatus;
        if (sscanf(cfun, "+CFUN: %d,%d", &cFunFlag, &regStatus) == 2) {
        	registrationStatus = regStatus;
        	if(cFunFlag) {
        		printf("Module ON.\n");
        		printCREGStatus(registrationStatus);
            	checkCfun = 1;

        	} else {

        	}


        }

    }

    // Check for the "+CREG:" message and extract the status
    const char *cregStr = strstr(response, "+CREG:");
    if (cregStr != NULL) {
        const char *statusStr = cregStr + 7; // Skip "+CREG: "
        int status = atoi(statusStr);

        registrationStatus = status;
        printCREGStatus(status);
        printf("Connected to the Network.\n");

    }

    // Check for the "+UUPSDA:" message and check if received the PDP Profile
    const char *uupsda = strstr(response, "+UUPSDA:");
    if (uupsda != NULL) {
        int pingCount;
        if (sscanf(uupsda, "+UUPSDA: %d,", &pingCount) == 1 && pingCount == 0) {
        	pdpProfile = 1;
            printf("+UUPSDA: PDP Profile received.\n");
        }
    }

    // sOMETIMES RECEIVES THE MESSAGE INCOMPLETE BUT the connection should be ok
    const char *uupsda2 = strstr(response, "T+UPSDA=0,3");
    if (uupsda2 != NULL) {
    	pdpProfile = 1;
		isOK = 1;
        printf("+UUPSDA: PDP Profile not received! but trying to ping....\n");

    }

    // Check for the "+UUPING:" message and check if received 4 packages
    const char *uupingStr = strstr(response, "+UUPING:");
    if (uupingStr != NULL) {
        int pingCount;
        if (sscanf(uupingStr, "+UUPING: %d,", &pingCount) == 1 && pingCount == 4) {
            pingReceived = 1;
            printf("+UUPING: 4 packages received! \n");
        }
    }

    // Check for the "+UUMQTTC:" message and check if received MQTT msg. type
    const char *uumqttc = strstr(response, "+UUMQTTC:");
    if (uumqttc != NULL) {
    	mqttTimeOut++;
        int mqttFlag;
        if (sscanf(uumqttc, "+UUMQTTC: %d,", &mqttFlag) == 1 && mqttFlag == 0) {
        	mqttLogout = 1;
            printf("MQTT Logout.\n");
        } else if (sscanf(uumqttc, "+UUMQTTC: %d,", &mqttFlag) == 1 && mqttFlag == 1) {
        	mqttLogin = 1;
            printf("Login MQTT.\n");

		} else if (sscanf(uumqttc, "+UUMQTTC: %d,", &mqttFlag) == 1 && mqttFlag == 2) {
        	mqttMsg = 1;
            printf("MQTT Message published.\n");
		} else if (mqttTimeOut == 5) {
        	mqttMsg = 1;
        	mqttTimeOut = 0;
            printf("MQTT Message probably missed.\n");
            printf("MQTTC response: %s\n", response);
		}
    }

    const char *cclkStart = strstr(response, "+CCLK:");

	if (cclkStart != NULL) {
		printf("CCLK response received: %s\n", response);
		int year, month, day, hours, minutes, seconds;
		if (sscanf(cclkStart, "+CCLK: \"%d/%d/%d,%d:%d:%d\"", &year, &month,
				&day, &hours, &minutes, &seconds) == 6) {

			printf("Current Date: %02d/%02d/%02d\n", year, month, day);
			printf("Current Time: %02d:%02d:%02d\n", hours, minutes, seconds);
		}

	       // Set the RTC time
		RTC_TimeTypeDef sTime = { 0 };
		sTime.Hours = hours;
		sTime.Minutes = minutes;
		sTime.Seconds = seconds;

		if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
			Error_Handler();
		}

		// Set the RTC date
		RTC_DateTypeDef sDate = { 0 };
		sDate.Year = year; // Adjusting year to be relative to 2000 (e.g., 23 for the year 2023)
		sDate.Month = month;
		sDate.Date = day;

		if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
			Error_Handler();
		}
	}




    const char *atError = strstr(response, "ERROR");
    if (atError != NULL) {
    	atErrorFlag = 1;
        printf("AT Error message received\n");
    }

}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
	HAL_GPIO_TogglePin(LEDGreen_GPIO_Port, LEDGreen_Pin);
    printf("Callback message received\n");
	if (huart->Instance == USART1)
	{
		oldPos = newPos;  // Update the last position before copying new data

		if (oldPos+Size >= MainBuf_SIZE)  // If the current position + new data size is greater than the main buffer
		{
			uint16_t datatocopy = MainBuf_SIZE-oldPos;  // find out how much space is left in the main buffer
			memcpy ((uint8_t *)MainBuf+oldPos, RxBuf, datatocopy);  // copy data in that remaining space

			oldPos = 0;  // point to the start of the buffer
			memcpy ((uint8_t *)MainBuf, (uint8_t *)RxBuf+datatocopy, (Size-datatocopy));  // copy the remaining data
			newPos = (Size-datatocopy);  // update the position
		}

		/* if the current position + new data size is less than the main buffer
		 * we will simply copy the data into the buffer and update the position
		 */
		else
		{
			memcpy ((uint8_t *)MainBuf+oldPos, RxBuf, Size);
			newPos = Size+oldPos;
		}

		/* start the DMA again */
		initializeUARTDMAReception();

	}

    RxBuf[Size] = '\0';  // Null-terminate the received data
    printf("Response RxBuf: %s\n", (char *)RxBuf);
    // Handle the response
    handleResponse((char *)RxBuf);
    HAL_GPIO_TogglePin(LEDGreen_GPIO_Port, LEDGreen_Pin);
    resetUARTDMA(huart);
}

void printCREGStatus(int status) {

	printf("CREG: %d message received:  ", status);

	switch (status) {
	case 0: printf("Not registered, not searching.\n"); break;
	case 1: printf("Registered, home network.\n"); break;
	case 2:	printf("Not registered, searching.\n"); break;
	case 3:	printf("Registration denied.\n"); break;
	case 4:	printf("Unknown status.\n"); break;
	case 5:	printf("Registered, roaming.\n"); break;
	case 6:	printf("Registered for SMS only, home network.\n");	break;
	case 7:	printf("Registered for SMS only, roaming.\n"); break;
	case 8:	printf("Attached for emergency bearer services only.\n"); break;
	case 9:	printf("Registered for CSFB not preferred, home network.\n"); break;
	case 10: printf("Registered for CSFB not preferred, roaming.\n"); break;
	default: printf("Unknown CREG status.\n"); break;
	}
}

void runLteStateMachine(State firstState) {
	currentState = firstState;
	int timeOut = 0;

	while (currentState != DONE && currentState != AT_ERROR) {
	    osDelay(300/portTICK_PERIOD_MS);
		switch (currentState) {
		case SEND_CREG:
			printf("Current State: SEND_CREG: %d\n", (int) currentState);
			isOK = 0;
			sendATCommand(CREG_ENABLE);
			currentState = WAIT_CREG_OK;
			break;

		case WAIT_CREG_OK:
			printf("Current State: WAIT_CREG_OK: %d\n", (int) currentState);
			timeOut++;
			if (isOK) {
				isOK = 0;
				currentState = SEND_CMEE;
			} else if (timeOut == MAXTIMEOUT) {
    			resetModule();
				resetUARTCallback(&huart1);
				currentState = SEND_CREG;
				timeOut = 0;
			}
			break;

		case SEND_CMEE:
			printf("Current State: SEND_CMEE: %d\n", (int) currentState);
			isOK = 0;
			sendATCommand(CMEE_ENABLE);
			currentState = WAIT_CMEE_OK;
			break;

		case WAIT_CMEE_OK:
			timeOut++;
			printf("Current State: WAIT_CMEE_OK: %d\n", (int) currentState);
			if (isOK) {
				isOK = 0;
				currentState = SEND_CHECK_CFUN;
			} else if (timeOut == MAXTIMEOUT) {
    			resetModule();
				resetUARTCallback(&huart1);
				currentState = SEND_CREG;
				timeOut = 0;
			}
			break;

		case SEND_CHECK_CFUN:
			printf("Current State: SEND_CHECK_CFUN: %d\n", (int) currentState);
			isOK = 0;
			sendATCommand(SEND_CHECK_CFUN);
			currentState = WAIT_CHECK_CFUN;
			break;

		case WAIT_CHECK_CFUN:
			timeOut++;
			printf("Current State: WAIT_CHECK_CFUN: %d\n", (int) currentState);
			osDelay(1000/portTICK_PERIOD_MS);
			if (isOK && checkCfun) {
				isOK = 0;
				currentState = WAIT_TURN_ON_CREG;
			} else if(isOK && !checkCfun) {
				isOK = 0;
				currentState = SEND_TURN_ON;
                resetUARTCallback(&huart1);
			} else if (timeOut == MAXTIMEOUT) {
    			resetModule();
				resetUARTCallback(&huart1);
				currentState = SEND_CREG;
				timeOut = 0;
			}
			break;

		case SEND_TURN_OFF:
			printf("Current State: SEND_TURN_OFF: %d\n", (int) currentState);
			isOK = 0;
			registrationStatus = -1;
			sendATCommand(CFUN_OFF);
			currentState = WAIT_TURN_OFF_OK_CREG0;
			break;

		case WAIT_TURN_OFF_OK_CREG0:
			timeOut++;
			printf("Current State: WAIT_TURN_OFF_OK_CREG0: %d\n",
					(int) currentState);
			restartConnection++;
			osDelay(1000/portTICK_PERIOD_MS);
			if (isOK && registrationStatus == 0) {
				isOK = 0;
				registrationStatus = -1;
				currentState = SEND_TURN_ON;
			} else if (restartConnection == 5) {
				sendATCommand(CFUN_ON);
				currentState = SEND_TURN_OFF;
				printf(
						"ERROR, failed to turn off the module %d times failed, Turning ON again\n",
						(int) restartConnection);
				restartConnection = 0;
			} else if (timeOut == MAXTIMEOUT) {
    			resetModule();
				resetUARTCallback(&huart1);
				currentState = SEND_CREG;
				timeOut = 0;
			}
			break;

		case SEND_TURN_ON:
			printf("Current State: SEND_TURN_ON: %d\n", (int) currentState);
			isOK = 0;
			registrationStatus = -1;
			sendATCommand(CFUN_ON);
			currentState = WAIT_TURN_ON_CREG;
			break;

		case WAIT_TURN_ON_CREG:
			timeOut++;
			printf("Current State: WAIT_TURN_ON_CREG: %d\n",
					(int) currentState);
			if (registrationStatus == 7) {
				currentState = SEND_ACTIVATE_PDP_CONTEXT;
			} else if (timeOut == MAXTIMEOUT) {
    			resetModule();
				resetUARTCallback(&huart1);
				currentState = SEND_CREG;
				timeOut = 0;
			}
			break;

		case SEND_ACTIVATE_PDP_CONTEXT:
			printf("Current State: SEND_ACTIVATE_PDP_CONTEXT: %d\n",
					(int) currentState);
			isOK = 0;
			sendATCommand(ACTIVATE_PDP_CONTEXT);
			currentState = WAIT_ACTIVATE_PDP_CONTEXT;
			break;

		case WAIT_ACTIVATE_PDP_CONTEXT:
			printf("Current State: WAIT_ACTIVATE_PDP_CONTEXT: %d\n",
					(int) currentState);
			timeOut++;
			if (isOK) {
				isOK = 0;
				currentState = SEND_PDP_IPV4;
			} else if (timeOut > MAXTIMEOUT) {
				currentState = SEND_RESET_PDP_PROFILE;
    			printf("Failed, to activate PDP Context...\n");
    			timeOut = 0;
            }
			break;

		case SEND_PDP_IPV4:
			printf("Current State: SEND_PDP_IPV4: %d\n", (int) currentState);
			isOK = 0;
			sendATCommand(SET_PDP_TYPE_IPV4);
			currentState = WAIT_PDP_IPV4;
			break;

		case WAIT_PDP_IPV4:
			printf("Current State: WAIT_PDP_IPV4: %d\n", (int) currentState);
			timeOut++;
			if (isOK) {
				isOK = 0;
				currentState = SEND_PDP_PARAMETERS;
			} else if (timeOut > MAXTIMEOUT) {
				currentState = SEND_RESET_PDP_PROFILE;
    			printf("Failed, to activate PDP IP V4...\n");
    			timeOut = 0;
            }
			break;
		case SEND_PDP_PARAMETERS:
			printf("Current State: SEND_PDP_PARAMETERS: %d\n",
					(int) currentState);
			isOK = 0;
			sendATCommand(SET_PDP_PARAMETERS);
			currentState = WAIT_PDP_PARAMETERS;
			break;

		case WAIT_PDP_PARAMETERS:
			printf("Current State: WAIT_PDP_PARAMETERS: %d\n",
					(int) currentState);
			if (isOK) {
				isOK = 0;
				currentState = SEND_ACTIVATE_PDP_PROFILE;
			}
			break;
		case SEND_ACTIVATE_PDP_PROFILE:
			printf("Current State: SEND_ACTIVATE_PDP_PROFILE: %d\n",
					(int) currentState);
			isOK = 0;
			sendATCommand(ACTIVATE_PDP_PROFILE);
			currentState = WAIT_ACTIVATE_PDP_PROFILE;
			break;

		case WAIT_ACTIVATE_PDP_PROFILE:
			timeOut++;
			printf("Current State: WAIT_ACTIVATE_PDP_PROFILE: %d\n",
					(int) currentState);
			osDelay(1000/portTICK_PERIOD_MS);
			if (isOK && pdpProfile) {
				isOK = 0;
				pdpProfile = 0;
				currentState = SEND_PING;
			} else if (timeOut > MAXTIMEOUT) {
				currentState = SEND_RESET_PDP_PROFILE;
    			printf("Failed, to activate PDP Profile...\n");
    			timeOut = 0;
            }
			break;
		case SEND_PING:
			printf("Current State: SEND_PING: %d\n", (int) currentState);
			isOK = 0;
			sendATCommand(PING_GOOGLE_DNS);
			currentState = WAIT_PING;
			break;

		case WAIT_PING:
			timeOut++;
			printf("Current State: WAIT_PING: %d\n", (int) currentState);
			osDelay(500/portTICK_PERIOD_MS);
			if (isOK && pingReceived) {
				isOK = 0;
				pingReceived = 0;
				currentState = DONE;
				printf("Connected to Internet.\n");
			} else if (timeOut > MAXTIMEOUT + 10) {
				currentState = SEND_CREG;
    			printf("No Internet connection trying again later...\n");
    			timeOut = 0;
            }
			break;
		case AT_ERROR:
			printf("An error occurred.\n");
			break;
		case SEND_RESET_PDP_PROFILE:
			printf("Current State: SEND_PDP_IPV4: %d\n", (int) currentState);
			isOK = 0;
			sendATCommand(RESET_PDP_PROFILE);
			currentState = WAIT_RESET_PDP_PROFILE;
			break;

		case WAIT_RESET_PDP_PROFILE:
			timeOut++;
			printf("Current State: WAIT_PDP_IPV4 reset Context: %d\n", (int) currentState);
			if (isOK) {
				isOK = 0;
				currentState = SEND_CREG;
			} else if (timeOut > MAXTIMEOUT) {
				currentState = SEND_CREG;
    			printf("Failed, to re-activate PDP Profile...\n");
    			timeOut = 0;
    			resetModule();
                resetUARTCallback(&huart1);
            }
			break;
		default:
			break;
		}
	}

}

void runPowerOFFModule(State firstState) {
	currentState = firstState;
	int timeOut = 0;

	while (currentState != DONE && currentState != AT_ERROR) {
	    osDelay(300/portTICK_PERIOD_MS);
		switch (currentState) {

		case SEND_POWEROFF:
			printf("Current State: SEND_POWEROFF: %d\n", (int) currentState);
			isOK = 0;
			sendATCommand(POWEROFF);
			currentState = WAIT_POWEROFF;
			break;

		case WAIT_POWEROFF:
			timeOut++;
			printf("Current State: WAIT_POWEROFF: %d\n", (int) currentState);
			osDelay(1000/portTICK_PERIOD_MS);
			if (isOK && checkCfun) {
				isOK = 0;
				currentState = DONE;
			} else if (timeOut > MAXTIMEOUT) {
				currentState = SEND_POWEROFF;
    			printf("Failed, to SEND_POWEROFF Command trying again...\n");
    			timeOut = 0;
    			resetModule();
                resetUARTCallback(&huart1);
            }
			break;
		default:
			break;
		}
	}
    HAL_GPIO_WritePin(LEDBlue_GPIO_Port, LEDBlue_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDGreen_GPIO_Port, LEDGreen_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDOrng_GPIO_Port, LEDOrng_Pin, GPIO_PIN_RESET);
}

void runCheckModule(State firstState) {
	currentState = firstState;
	int timeOut = 0;

	while (currentState != DONE && currentState != AT_ERROR) {
	    osDelay(300/portTICK_PERIOD_MS);
		switch (currentState) {

		case SEND_CHECK_CFUN:
			printf("Current State: SEND_CHECK_CFUN: %d\n", (int) currentState);
			isOK = 0;
			sendATCommand(SEND_CHECK_CFUN);
			currentState = WAIT_CHECK_CFUN;
			break;

		case WAIT_CHECK_CFUN:
			timeOut++;
			printf("Current State: WAIT_CHECK_CFUN: %d\n", (int) currentState);
			osDelay(1000/portTICK_PERIOD_MS);
			if (isOK && checkCfun) {
				isOK = 0;
				currentState = DONE;
			} else if (timeOut > MAXTIMEOUT) {
				currentState = SEND_CHECK_CFUN;
    			printf("Failed, to receive CFUN Command trying again...\n");
    			timeOut = 0;
    			resetModule();
                resetUARTCallback(&huart1);
				currentState = DONE;

            }
			break;
		default:
			break;
		}
	}

}

void runMQTTSaveProfileStateMachine(MQTTProfileState ProfileState) {

	mqttCurrentState = ProfileState;
	int timeOut = 0;
	while (mqttCurrentState != MQTT_DONE && mqttCurrentState != MQTT_AT_ERROR) {
		osDelay(200/portTICK_PERIOD_MS);

		switch (mqttCurrentState) {
		case SEND_SERVER_NAME:
			printf("Current MQTT State: SEND_SERVER_NAME: %d\n", (int) mqttCurrentState);
			isOK = 0;
			sendATCommand(MQTT_SETUP_SERVER);
			mqttCurrentState = WAIT_SERVER_NAME_OK;
			break;

		case WAIT_SERVER_NAME_OK:
			timeOut++;
			printf("Current MQTT State: WAIT_SERVER_NAME_OK: %d\n", (int) mqttCurrentState);
			if (isOK) {
				isOK = 0;
				mqttCurrentState = SEND_NAME_PASSWORD;
			} else if (timeOut > MAXTIMEOUT) {
				mqttCurrentState = SEND_SERVER_NAME;
    			printf("Failed, to send MQTT WAIT_SERVER_NAME_OK message...\n");
    			timeOut = 0;
    			resetModule();
                resetUARTCallback(&huart1);
            }
			break;

		case SEND_NAME_PASSWORD:
			printf("Current MQTT State: SEND_NAME_PASSWORD: %d\n", (int) mqttCurrentState);
			isOK = 0;
			sendATCommand(MQTT_SETUP_USERNAME_PASSWORD);
			mqttCurrentState = WAIT_NAME_PASSWORD_OK;
			break;

		case WAIT_NAME_PASSWORD_OK:
			timeOut++;
			printf("Current MQTT State: WAIT_NAME_PASSWORD_OK: %d\n", (int) mqttCurrentState);
			if (isOK) {
				isOK = 0;
				mqttCurrentState = SEND_WILL_QOS;
			} else if (timeOut > MAXTIMEOUT) {
				mqttCurrentState = SEND_SERVER_NAME;
    			printf("Failed, to send MQTT WAIT_NAME_PASSWORD_OK message...\n");
    			timeOut = 0;
            }
			break;

		case SEND_WILL_QOS:
			printf("Current MQTT State: SEND_WILL_QoS: %d\n", (int) mqttCurrentState);
			isOK = 0;
			sendATCommand(MQTT_SETUP_LAST_WILL_QOS);
			mqttCurrentState = WAIT_WILL_QOS_OK;
			break;

		case WAIT_WILL_QOS_OK:
			timeOut++;
			printf("Current MQTT State: WAIT_WILL_QoS_OK: %d\n", (int) mqttCurrentState);
			if (isOK) {
				isOK = 0;
				mqttCurrentState = SEND_WILL_RETAIN;
			} else if (timeOut > MAXTIMEOUT) {
				mqttCurrentState = SEND_SERVER_NAME;
    			printf("Failed, to send MQTT WAIT_WILL_QoS_OK message...\n");
    			timeOut = 0;
            }
			break;

		case SEND_WILL_RETAIN:
			printf("Current MQTT State: SEND_WILL_RETAIN: %d\n", (int) mqttCurrentState);
			isOK = 0;
			sendATCommand(MQTT_SETUP_LAST_WILL_QOS);
			mqttCurrentState = WAIT_WILL_RETAIN_OK;
			break;

		case WAIT_WILL_RETAIN_OK:
			timeOut++;
			printf("Current MQTT State: WAIT_WILL_RETAIN_OK: %d\n", (int) mqttCurrentState);
			if (isOK) {
				isOK = 0;
				mqttCurrentState = SEND_WILL_TOPIC;
			} else if (timeOut > MAXTIMEOUT) {
				mqttCurrentState = SEND_SERVER_NAME;
    			printf("Failed, to save MQTT WAIT_WILL_RETAIN_OK message...\n");
    			timeOut = 0;
            }
			break;

		case SEND_WILL_TOPIC:
			printf("Current MQTT State: SEND_WILL_TOPIC: %d\n", (int) mqttCurrentState);
			isOK = 0;
			sendATCommand(MQTT_SETUP_LAST_WILL_TOPIC_ID);
			mqttCurrentState = WAIT_WILL_TOPIC_OK;
			break;

		case WAIT_WILL_TOPIC_OK:
			timeOut++;
			printf("Current MQTT State: WAIT_WILL_TOPIC_OK: %d\n", (int) mqttCurrentState);
			if (isOK) {
				isOK = 0;
				mqttCurrentState = SEND_WILL_MSG;
			} else if (timeOut > MAXTIMEOUT) {
				mqttCurrentState = SEND_SERVER_NAME;
    			printf("Failed, to save MQTT WILL message...\n");
    			timeOut = 0;
            }
			break;

		case SEND_WILL_MSG:
			printf("Current MQTT State: SEND_WILL_MSG: %d\n", (int) mqttCurrentState);
			isOK = 0;
			sendATCommand(MQTT_SETUP_LAST_WILL_MSG_ID);
			mqttCurrentState = WAIT_WILL_MSG_OK;
			break;

		case WAIT_WILL_MSG_OK:
			timeOut++;
			printf("Current MQTT State: WAIT_WILL_MSG_OK: %d\n", (int) mqttCurrentState);
			if (isOK) {
				isOK = 0;
				mqttCurrentState = MQTT_DONE;
			} else if (timeOut > MAXTIMEOUT) {
				mqttCurrentState = SEND_SERVER_NAME;
    			printf("Failed, to send MQTT WAIT_WILL_MSG_OK message...\n");
    			timeOut = 0;
            }
			break;

		case SEND_SAVE_NVM:
			printf("Current MQTT State: SEND_SAVE_NVM: %d\n", (int) mqttCurrentState);
			isOK = 0;
			sendATCommand(MQTT_SAVE_SETUP);
			mqttCurrentState = WAIT_SAVE_NVM;
			break;

		case WAIT_SAVE_NVM:
			timeOut++;
			printf("Current MQTT State: WAIT_SAVE_NVM: %d\n", (int) mqttCurrentState);
			if (isOK) {
				isOK = 0;
				mqttCurrentState = MQTT_DONE;
				printf("MQTT profile has been successfully saved in NVM \n");

			} else if (timeOut > MAXTIMEOUT) {
				mqttCurrentState = MQTT_DONE;
    			printf("Failed, to save NVM or ok not received...\n");
    			timeOut = 0;
            }
			break;

		case MQTT_AT_ERROR:
			printf("An error occurred.\n");
			break;


		default:
			break;
		}
	}

}

void runMQTTSendMsgStateMachine(MQTTSendMessageState msgState, MQTTTopicName topicName, char *data, uint16_t  devID) {

	mqttMsgCurrentState = msgState;
	int timeOut = 0;
	while (mqttMsgCurrentState != MQTT_MSG_DONE && mqttMsgCurrentState != MQTT_MSG_AT_ERROR) {
	    osDelay(1000/portTICK_PERIOD_MS);
		switch (mqttMsgCurrentState) {
		case SEND_LOAD_NVM:
			printf("Current MQTT State: SEND_LOAD_NVM: %d\n", (int) mqttMsgCurrentState);
			isOK = 0;
			sendATCommand(MQTT_RETRIEVE_CLIENT_PROFILE);
			mqttMsgCurrentState = WAIT_LOAD_NVM_OK;
			break;

		case WAIT_LOAD_NVM_OK:
			timeOut++;
			printf("Current MQTT State: WAIT_LOAD_NVM_OK: %d\n", (int) mqttMsgCurrentState);
			if (isOK) {
				isOK = 0;
				mqttMsgCurrentState = SEND_LOGIN_MQTT;
			} else if (timeOut > MAXTIMEOUT) {
				mqttMsgCurrentState = SEND_LOGIN_MQTT;
    			printf("Failed, to Load NVM or ok not received...\n");
    			timeOut = 0;
            }
			break;

		case SEND_LOGIN_MQTT:
			printf("Current MQTT State: SEND_LOGIN_MQTT: %d\n", (int) mqttMsgCurrentState);
			isOK = 0;
			sendATCommand(MQTT_LOGIN);
			mqttMsgCurrentState = WAIT_LOGIN_MQTT_OK;
			break;

		case WAIT_LOGIN_MQTT_OK:
			timeOut++;
			printf("Current MQTT State: WAIT_LOGIN_MQTT_OK: %d\n", (int) mqttMsgCurrentState);
			if (isOK && mqttLogin) {
				isOK = 0;
				mqttLogin = 0;
				mqttMsgCurrentState = SEND_MSG_MQTT;
			} else if (atErrorFlag) {
				atErrorFlag = 0;
				currentState = SEND_CREG;
				mqttMsgCurrentState = SEND_LOGIN_MQTT;
				printf("Failed to login MQTT server.\n");
				runLteStateMachine(SEND_CREG);
			} else if (timeOut > MAXTIMEOUT) {
							mqttCurrentState = MQTT_DONE;
			    			printf("Failed 10 times, waiting for MQTT ok\n");
			    			timeOut = 0;
			    			resetModule();
			                resetUARTCallback(&huart1);
			            }
			break;

		case SEND_MSG_MQTT:
		    printf("Current MQTT State: SEND_MSG_MQTT: %d\n", (int) mqttMsgCurrentState);
		    isOK = 0;

		    // Convert devID to string
		    char devIDStr[6]; // Enough to hold max uint16_t value (65535) and null terminator
		    snprintf(devIDStr, sizeof(devIDStr), "%u", devID);
		    printf("devID: %u\n", devID); // Print the devID
		    printf("devIDStr: %s\n", devIDStr); // Print the devIDStr
		    // Concatenate devIDStr and data with a comma
		    int len = strlen(devIDStr) + strlen(data) + 2; // +2 for comma and null terminator
		    char *concatenatedData = (char *)malloc(len);
		    if (concatenatedData != NULL) {
		        snprintf(concatenatedData, len, "%s,%s", devIDStr, data);
		        sendATCommandP(MQTT_PUBLISH_STRING_MSG, topicName, concatenatedData);
		        free(concatenatedData);
		    } else {
		        printf("Failed to allocate memory for concatenated data.\n");
		        mqttMsgCurrentState = MQTT_MSG_AT_ERROR;
		        break;
		    }

		    mqttMsgCurrentState = WAIT_MSG_MQTT_OK;
		    break;

		case WAIT_MSG_MQTT_OK:
			timeOut++;
			printf("Current MQTT State: WAIT_MSG_MQTT_OK: %d\n", (int) mqttMsgCurrentState);
			if (isOK && mqttMsg) {
				isOK = 0;
				mqttMsg = 0;
				mqttMsgCurrentState = SEND_LOGOUT_MQTT;
			} else if (timeOut > MAXTIMEOUT) {
				mqttCurrentState = SEND_LOGIN_MQTT;
    			printf("Failed 10 times, waiting for MQTT ok\n");
    			timeOut = 0;
    			resetModule();
				runLteStateMachine(SEND_CREG);
            }
			break;

		case SEND_LOGOUT_MQTT:
			printf("Current MQTT State: SEND_LOGOUT_MQTT: %d\n", (int) mqttMsgCurrentState);
			isOK = 0;
			sendATCommand(MQTT_LOGOUT);
			mqttMsgCurrentState = WAIT_LOGOUT_MQTT_OK;
			break;

		case WAIT_LOGOUT_MQTT_OK:
			printf("Current MQTT State: WAIT_LOGOUT_MQTT_OK: %d\n", (int) mqttMsgCurrentState);
			timeOut++;
			if (isOK) {
				isOK = 0;
				mqttLogout = 0;
				mqttMsgCurrentState = MQTT_MSG_DONE;
				printf("MQTT message has been sent successfully\n");
			} else if (timeOut > MAXTIMEOUT) {
				mqttMsgCurrentState = MQTT_DONE;
    			printf("Failed 10 times, forcing logging out\n");
    			timeOut = 0;
            }
			break;

		case MQTT_MSG_AT_ERROR:
			printf("An error occurred.\n");
			break;

		default:
			break;
		}
	}

}

void runClockStateMachine(CCLKSendMessageState mState) {
	clkMsgState = mState;
	int timeOut = 0;
	while (clkMsgState != CLK_MSG_DONE && clkMsgState != CLK_MSG_AT_ERROR) {
	    osDelay(100/portTICK_PERIOD_MS);

		switch (clkMsgState) {
		case SEND_GET_CCLK_OK:
			printf("Current CLK State: SEND_CLK_OK:: %d\n", (int) clkMsgState);
			isOK = 0;
			//sendATCommand(CHECK_CFUN);
			sendATCommand(GET_CCLK_OK);
			clkMsgState = WAIT_SEND_CCLK_OK;

			break;

		case WAIT_SEND_CCLK_OK:
			printf("Current CLK State: WAIT_CLK_OK: %d\n", (int) clkMsgState);
			timeOut++;
			if (isOK) {
				isOK = 0;
				clkMsgState = SEND_CCLK;
				printf("Clock AT Test command Ok Received.\n");

			} else if (timeOut > MAXTIMEOUT ) {
                clkMsgState = SEND_GET_CCLK_OK;
                timeOut = 0;
    			printf("Failed to get the time OK NOT RECEIVED.\n");
            }

			break;
		case SEND_CCLK:
			printf("Current CLK State: SEND_CCLK: %d\n", (int) clkMsgState);
			isOK = 0;
			sendATCommand(GET_CCLK);
			clkMsgState = WAIT_CCLK_OK;
			break;

		case WAIT_CCLK_OK:
			printf("Current CLK State: WAIT_CCLK_OK: %d\n", (int) clkMsgState);
			timeOut++;
			if (isOK) {
				isOK = 0;
				clkMsgState = CLK_MSG_DONE;
    			printf("CLK UPDATED!\n");

			} else if (timeOut > 1) {
	                clkMsgState = SEND_GET_CCLK_OK;
	    			printf("Failed to get the time OK NOT RECEIVED.\n");
	            }
			break;

		default:
			break;
		}

	}

}
