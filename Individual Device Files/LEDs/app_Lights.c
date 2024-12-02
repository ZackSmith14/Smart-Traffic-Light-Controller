
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

#define 	LED4_PIN 		GPIO_PIN_10//pole2 red
#define 	LED5_PIN 		GPIO_PIN_11//polle2 yellow
#define 	LED6_PIN 		GPIO_PIN_12//pole2 green


// Main road Clear, the secondary red
#define 	DEFAULT_MODE			0
// Yellow for main road after sensor detected positive, or prompted to change light
#define 	TRANSITION 	1

#define 	POLE1_RED_FLASHING	40

#define 	POLE2_RED_FLASHING	41

#define 	ALL_RED 42
/* Private function prototypes -----------------------------------------------*/
void ShowCommands(void);
void UART_TransmitString(UART_HandleTypeDef *p_huart, char a_string[], int newline);


/* Extern global variables ---------------------------------------------------------*/

extern UART_HandleTypeDef huart2;


//Should be declared as volatile if variables' values are changed in ISR.
volatile char rxData;  //One byte data received from UART
volatile int traffic_mode = DEFAULT_MODE;
volatile int run_time = 0;

void App_Init(void) {

	UART_TransmitString(&huart2, "-----------------", 1);
	UART_TransmitString(&huart2, "~ Nucleo-L476RG ~", 1);
	UART_TransmitString(&huart2, "-----------------", 1);

	ShowCommands();

	//HAL_TIM_Base_Start_IT(&htim3);
	HAL_UART_Receive_IT(&huart2, (uint8_t*) &rxData, 1); //Start the Rx interrupt.
}


void App_MainLoop(void) {

	if (traffic_mode == DEFAULT_MODE) {

		HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);

		HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_SET);

		HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_SET);

		HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
		}
	else if (traffic_mode == TRANSITION){
		if (run_time ==0){
			// Pole 1 yellow, Pole 2 red
			HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);

			HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
			HAL_Delay(1000);
			// Pole 1 red, Pole 2 Green
			HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);

			HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_SET);
			// Give time for vehicles on the other direction/ pedestrians to cross
			HAL_Delay(10000);
			// Pole 1 still red, but warning for Pole 2
			HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_RESET);

			HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
			HAL_Delay(1000);

			//Turn back to the default state
			HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_SET);

			HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
			// Variable so that the statement does not run again, increment by 1
			run_time++;
		}
	}

	else if (traffic_mode == POLE1_RED_FLASHING){
		// Flash Pole 1 Red, but pole 2 yellow
		HAL_GPIO_TogglePin(LED_PORT, LED0_PIN);
		HAL_GPIO_TogglePin(LED_PORT, LED5_PIN);
		HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);

		HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
		HAL_Delay(1000);
	}
	else if (traffic_mode == POLE2_RED_FLASHING){
		// Flash Pole 2 Red, but pole 1 yellow
		HAL_GPIO_TogglePin(LED_PORT, LED4_PIN);
		HAL_GPIO_TogglePin(LED_PORT, LED1_PIN);
		HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);

		HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);

		HAL_Delay(1000);
	}
	else if (traffic_mode == ALL_RED){
		HAL_GPIO_TogglePin(LED_PORT, LED4_PIN);
		HAL_GPIO_TogglePin(LED_PORT, LED0_PIN);
		HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);

		HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);

		HAL_Delay(1000);

	}


}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *p_huart) {
	//Process the data received from UART.
	switch (rxData) {
	case 'M':
	case 'm':
		traffic_mode = DEFAULT_MODE;
		break;
	case 'C':
	case 'c':
		traffic_mode = TRANSITION;
		run_time = 0;
		break;
	case 'F':
		traffic_mode = POLE1_RED_FLASHING;
		break;
	case 'f':
		traffic_mode = POLE2_RED_FLASHING;
		break;
	case 'A':
	case 'a':
		traffic_mode = ALL_RED;
		break;
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
	UART_TransmitString(&huart2, "> F: Changes pole 1 to flashing red (pole 2 yellow)", 1);
	UART_TransmitString(&huart2, "> f: Changes pole 2 to flashing red (pole 1 yellow)", 1);
	UART_TransmitString(&huart2, "> A or a: Changes both poles to flashing red", 1);
}

void UART_TransmitString(UART_HandleTypeDef *p_huart, char a_string[], int newline) {
	HAL_UART_Transmit(p_huart, (uint8_t*) a_string, strlen(a_string), HAL_MAX_DELAY);
	if (newline != 0) {
		HAL_UART_Transmit(p_huart, (uint8_t*) "\n\r", 2, HAL_MAX_DELAY);
	}
}
