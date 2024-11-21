
/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "stm32l4xx_hal.h"
#include "app.h"

/* Private define ------------------------------------------------------------*/
#define 	LED_PORT 			GPIOA
#define 	LED0_PIN 		GPIO_PIN_5//pole1 red
#define 	LED1_PIN 		GPIO_PIN_6//pole1 yellow
#define 	LED2_PIN 		GPIO_PIN_7//pole1 green
#define 	LED3_PIN 		GPIO_PIN_8

#define 	LED4_PIN 		GPIO_PIN_10//pole2 red
#define 	LED5_PIN 		GPIO_PIN_11//polle2 yellow
#define 	LED6_PIN 		GPIO_PIN_12//pole2 green
#define 	LED7_PIN 		GPIO_PIN_13




#define 	POLE1_CLEAR		0
#define 	POLE1_WARNING			1
#define 	POLE1_STOP	2
#define 	POLE1_YELLOW_FLASHING	3
#define 	POLE1_RED_FLASHING	4
#define 	POLE1_GREEN	5


#define 	POLE2_CLEAR		6
#define 	POLE2_WARNING	7
#define 	POLE2_STOP	8
#define 	POLE2_YELLOW_FLASHING	9
#define 	POLE2_RED_FLASHING	10
#define 	POLE2_GREEN	11

#define 	STOP_ALLWAY 20



/* Private function prototypes -----------------------------------------------*/
void ShowCommands(void);
void UART_TransmitString(UART_HandleTypeDef *p_huart, char a_string[], int newline);


/* Extern global variables ---------------------------------------------------------*/

extern UART_HandleTypeDef huart2;


//Should be declared as volatile if variables' values are changed in ISR.
volatile char rxData;  //One byte data received from UART
volatile int traffic_mode = POLE1_YELLOW_FLASHING;


void App_Init(void) {

	UART_TransmitString(&huart2, "-----------------", 1);
	UART_TransmitString(&huart2, "~ Nucleo-L476RG ~", 1);
	UART_TransmitString(&huart2, "-----------------", 1);

	ShowCommands();

	//HAL_TIM_Base_Start_IT(&htim3);
	HAL_UART_Receive_IT(&huart2, (uint8_t*) &rxData, 1); //Start the Rx interrupt.
}


void App_MainLoop(void) {
	if (traffic_mode == POLE1_YELLOW_FLASHING) {
		HAL_GPIO_TogglePin(LED_PORT, LED1_PIN);
		HAL_GPIO_TogglePin(LED_PORT, LED4_PIN);
		HAL_Delay(1000);

		}
	else if (traffic_mode == POLE2_YELLOW_FLASHING){
		HAL_GPIO_TogglePin(LED_PORT, LED5_PIN);
		HAL_GPIO_TogglePin(LED_PORT, LED0_PIN);
		HAL_Delay(1000);


	}
	else if (traffic_mode == POLE1_GREEN){
	    HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_SET);
	    HAL_Delay(1000);
	    HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);

	    HAL_GPIO_WritePin(LED_PORT, LED3_PIN, GPIO_PIN_SET);


	}
	else if (traffic_mode == POLE2_GREEN){
	    HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_SET);
	    HAL_Delay(1000);
	    HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_SET);
	    HAL_Delay(1000);
	    HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);

	    HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_SET);


		}
	else if (traffic_mode == STOP_ALLWAY){
		HAL_GPIO_TogglePin(LED_PORT, LED0_PIN);
		HAL_GPIO_TogglePin(LED_PORT, LED4_PIN);
		HAL_Delay(1000);

	}


}





void HAL_UART_RxCpltCallback(UART_HandleTypeDef *p_huart) {
    HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);


	//Process the data received from UART.
	switch (rxData) {
//	case '2':
	//	HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
		//traffic_mode = LED_MODE_ON;
		//break;
//	case '1':
	//	HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
		//traffic_mode = LED_MODE_OFF;
		//break;
	case 'F':

		traffic_mode = POLE1_YELLOW_FLASHING;
		break;
	case 'f':
		traffic_mode = POLE2_YELLOW_FLASHING;
		break;
	case 'g':
		traffic_mode = POLE1_GREEN;
		break;
	case 'G':
		traffic_mode = POLE2_GREEN;
	case 'A':
	case 'a':
		traffic_mode = STOP_ALLWAY;
	case 'H':
	case 'h':
		ShowCommands();
		break;



	}
	HAL_UART_Receive_IT(p_huart, (uint8_t*) &rxData, 1); //Restart the Rx interrupt.
}


void ShowCommands(void) {
	UART_TransmitString(&huart2, "Type on keyboard to send command from PC to MCU:", 1);
	UART_TransmitString(&huart2, "Letter Inputs are case sensitive", 1);
	UART_TransmitString(&huart2, "> 1: Changes pole 1 to green, 2: Changes pole 2 to green, h or H: show commands", 1);
	UART_TransmitString(&huart2, "> F: Changes pole 1 to flashing yellow, f: changes pole 2 to flashing yellow", 1);
}

void UART_TransmitString(UART_HandleTypeDef *p_huart, char a_string[], int newline) {
	HAL_UART_Transmit(p_huart, (uint8_t*) a_string, strlen(a_string), HAL_MAX_DELAY);
	if (newline != 0) {
		HAL_UART_Transmit(p_huart, (uint8_t*) "\n\r", 2, HAL_MAX_DELAY);
	}
}
