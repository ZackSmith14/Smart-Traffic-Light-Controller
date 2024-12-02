/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stm32l4xx_hal.h"
#include "Header_Final.h"
#include "main.h"

/* Private define ------------------------------------------------------------*/

#define 	TRIG1_PIN 				GPIO_PIN_1       // Define Trigger pin as PA8
#define 	TRIG2_PIN 				GPIO_PIN_8       // Define Trigger pin as PA8
#define 	TRIG_PORT 				GPIOA           // Define Trigger port as GPIOA
#define 	ECHO1_PIN 				GPIO_PIN_4       // Define Echo pin as PA9
#define 	ECHO2_PIN 				GPIO_PIN_9       // Define Echo pin as PA9
#define 	ECHO_PORT 				GPIOA           // Define Echo port as GPIOA

#define 	LED_PORT 				GPIOA
#define 	LED0_PIN 				GPIO_PIN_5  // Main road red light
#define 	LED1_PIN 				GPIO_PIN_6  // Main road yellow light
#define 	LED2_PIN 				GPIO_PIN_7  // Main road green light

#define 	LED4_PIN 				GPIO_PIN_10 // Secondary road red light
#define 	LED5_PIN 				GPIO_PIN_11 // Secondary road yellow light
#define 	LED6_PIN 				GPIO_PIN_12 // Secondary road green light

#define 	LCD_PORT				GPIOB
#define 	LCD_EN					GPIO_PIN_7 //Enable
#define 	LCD_RS 					GPIO_PIN_6 //Register Select
#define 	LCD_D4 					GPIO_PIN_8 //Data Pin
#define 	LCD_D5 					GPIO_PIN_9 //Data Pin
#define 	LCD_D6 					GPIO_PIN_10 //Data Pin
#define 	LCD_D7 					GPIO_PIN_11 //Data Pin
#define 	Button 					GPIO_PIN_12 //External Button

/* Traffic light states */
// Main road Clear, the secondary red
#define 	DEFAULT_MODE			0
// Yellow for main road after sensor detected positive, or prompted to change light
#define 	TRANSITION 				1
#define 	POLE1_RED_FLASHING		40
#define 	POLE2_RED_FLASHING		41
#define 	ALL_RED 				42

extern 		UART_HandleTypeDef 		huart2;
extern 		TIM_HandleTypeDef 		htim2;
extern 		TIM_HandleTypeDef 		htim3;

/* Private function prototypes -----------------------------------------------*/
void 		delayMicroseconds(uint32_t us);
uint32_t 	measureDistanceSensor1(void);
uint32_t 	measureDistanceSensor2(void);
void 		UpdateTrafficLights(void);
void 		ShowCommands(void);
void 		UART_TransmitString(UART_HandleTypeDef *p_huart, char a_string[], int newline);
void 		App_Init(void);
void 		App_MainLoop(void);
void 		LCD_STROBE(void);
void 		lcd_write_cmd(unsigned char);
void 		lcd_write_data(unsigned char);
void 		lcd_clear(void);
void 		lcd_puts(const char * s);
void 		lcd_putch(char c);
void 		lcd_goto(int, int);
void 		lcd_init(void);
void 		pedestrian_crossing(void);
void 		waiting_for_pedestrian(void);

/* Private variables */
volatile char 						rxData;
volatile int 						traffic_mode = DEFAULT_MODE;
uint32_t 							distance1;
uint32_t 							distance2;

/* Initialization function */
void App_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Initialize UART for debugging
    UART_TransmitString(&huart2, "Traffic Light System Initialized", 1);
    UART_TransmitString(&huart2, "-----------------", 1);
    UART_TransmitString(&huart2, "~ Nucleo-L476RG ~", 1);
    UART_TransmitString(&huart2, "-----------------", 1);
    ShowCommands();

    // Initialize HC-SR04 sensor GPIO
    GPIO_InitStruct.Pin = TRIG1_PIN | TRIG2_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TRIG_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = ECHO1_PIN | ECHO2_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(ECHO_PORT, &GPIO_InitStruct);

    // Initialize traffic light LEDs as output
    GPIO_InitStruct.Pin = LED0_PIN | LED1_PIN | LED2_PIN | LED4_PIN | LED5_PIN | LED6_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    // Start UART and Timer
    HAL_UART_Receive_IT(&huart2, (uint8_t*)&rxData, 1);
    HAL_TIM_Base_Start_IT(&htim2);
    HAL_TIM_Base_Start_IT(&htim3);

    // Show initial state of traffic lights
    UpdateTrafficLights();
}

void App_MainLoop(void) {
    char buffer1[50];  // Buffer for UART Sensor 1 output
    char buffer2[50];  // Buffer for UART Sensor 2 output
    distance1 = measureDistanceSensor1();
    distance2 = measureDistanceSensor2();

    // Transmit distance over UART for monitoring
    int len1 = snprintf(buffer1, sizeof(buffer1), "Distance from Sensor 1: %lu cm\r\n", distance1);
    int len2 = snprintf(buffer2, sizeof(buffer2), "Distance from Sensor 2: %lu cm\r\n", distance2);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer1, len1, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer2, len2, HAL_MAX_DELAY);
    UART_TransmitString(&huart2, " ", 1);

    switch (traffic_mode) {
        case DEFAULT_MODE:
            if (distance1 < 5 || HAL_GPIO_ReadPin(GPIOB, Button) == 0) {
                traffic_mode = TRANSITION;
            }
            else if (distance2 < 5 || HAL_GPIO_ReadPin(GPIOB, Button) == 0) {
                traffic_mode = TRANSITION;
            }
            break;
        case TRANSITION:
        	if (HAL_GPIO_ReadPin(GPIOB, Button) == 0) {
        		pedestrian_crossing();
        	}
        	else {
        	UpdateTrafficLights();
        	}

        	traffic_mode = DEFAULT_MODE;  // Reset to default mode after the cycle
            break;
    }
    HAL_Delay(500);  // Delay to avoid flooding the UART with distance readings
}

/* Update Traffic Lights based on State */
void UpdateTrafficLights(void) {
    // Reset all lights
    HAL_GPIO_WritePin(LED_PORT, LED0_PIN | LED1_PIN | LED2_PIN | LED4_PIN | LED5_PIN | LED6_PIN, GPIO_PIN_RESET);

    switch (traffic_mode) {
        case DEFAULT_MODE:
        	HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_RESET);
        	HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
        	HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_SET);
        	HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_SET);
        	HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
        	HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
            break;

        case TRANSITION:
        	// Pole 1 yellow, Pole 2 red
            HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
            HAL_Delay(2000);

            // Pole 1 red, Pole 2 red
            HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
            HAL_Delay(2000);

            // Pole 1 red, Pole 2 green
            HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_SET);
            HAL_Delay(8000);

            // Pole 1 still red, but warning for Pole 2
            HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
            HAL_Delay(2000);

            // Pole 1 red, Pole 2 red
            HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
            HAL_Delay(2000);

            HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);

            break;

        case POLE1_RED_FLASHING:
        case POLE2_RED_FLASHING:
        case ALL_RED:
        	// Lights handled in the timer interrupt callback
            break;
    }
}

/* Show Commands over UART */
void ShowCommands(void) {
    UART_TransmitString(&huart2, "Traffic Light Commands:", 1);
    UART_TransmitString(&huart2, "M/m: Main Road Is Green", 1);
    UART_TransmitString(&huart2, "C/c: Transition Mode", 1);
    UART_TransmitString(&huart2, "F: Main Road Is Red Flashing", 1);
    UART_TransmitString(&huart2, "f: Secondary Road Is Red Flashing", 1);
    UART_TransmitString(&huart2, "A/a: Both Roads Are Red", 1);
    UART_TransmitString(&huart2, "H/h: Show commands", 1);
}

/* Timer Callback */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *p_htim) {
    if (p_htim == &htim3) {
        // Handle LED flashing
        switch (traffic_mode) {
            case POLE1_RED_FLASHING:
            	HAL_GPIO_TogglePin(LED_PORT, LED0_PIN); // Flash Pole 1 Red
            	HAL_GPIO_TogglePin(LED_PORT, LED5_PIN); // Pole 2 Yellow
            	HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
            	HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
            	HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_RESET);
            	HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
                break;

            case POLE2_RED_FLASHING:
            	HAL_GPIO_TogglePin(LED_PORT, LED4_PIN); // Flash Pole 2 Red
            	HAL_GPIO_TogglePin(LED_PORT, LED1_PIN); // Pole 1 Yellow
            	HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_RESET);
            	HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
            	HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
            	HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
                break;

            case ALL_RED:
            	HAL_GPIO_TogglePin(LED_PORT, LED4_PIN); // Flash Pole 2 Red
            	HAL_GPIO_TogglePin(LED_PORT, LED0_PIN); // Flash Pole 1 Red
            	HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
            	HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
            	HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
            	HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
                break;

            default:
                // No flashing logic required for other modes
                break;
        }
    }
}

/* UART Callback Function */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *p_huart) {
	//Process the data received from UART.
	switch (rxData) {
		case 'M':
		case 'm':
			traffic_mode = DEFAULT_MODE;
			UpdateTrafficLights();
		break;

		case 'C':
		case 'c':
			traffic_mode = TRANSITION;
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

/* UART Transmit String */
void UART_TransmitString(UART_HandleTypeDef *p_huart, char a_string[], int newline)
{
    HAL_UART_Transmit(p_huart, (uint8_t*) a_string, strlen(a_string), HAL_MAX_DELAY);
    if (newline != 0) {
        HAL_UART_Transmit(p_huart, (uint8_t*) "\n\r", 2, HAL_MAX_DELAY);
    }
}

/* Measure distance using HC-SR04 */
uint32_t measureDistanceSensor1(void) {
uint32_t localTimeSensor1 = 0;
uint32_t startTimeSensor1 = 0;

    // Trigger Sensor 1
    HAL_GPIO_WritePin(TRIG_PORT, TRIG1_PIN, GPIO_PIN_SET);
    delayMicroseconds(10);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG1_PIN, GPIO_PIN_RESET);

    // Wait for ECHO1 pin to go HIGH
    while (!(HAL_GPIO_ReadPin(ECHO_PORT, ECHO1_PIN)));

    // Record the time ECHO1 is HIGH
    startTimeSensor1 = __HAL_TIM_GET_COUNTER(&htim2);
    while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO1_PIN)) {
        localTimeSensor1 = __HAL_TIM_GET_COUNTER(&htim2) - startTimeSensor1;
    }

    // Calculate distance (speed of sound: 34300 cm/s)
    return (localTimeSensor1 * 0.034 / 2);
}

/* Measure distance using HC-SR04 */
uint32_t measureDistanceSensor2(void) {
uint32_t localTimeSensor2 = 0;
uint32_t startTimeSensor2 = 0;

    // Trigger Sensor 2
    HAL_GPIO_WritePin(TRIG_PORT, TRIG2_PIN, GPIO_PIN_SET);
    delayMicroseconds(10);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG2_PIN, GPIO_PIN_RESET);

    // Wait for ECHO2 pin to go HIGH
    while (!(HAL_GPIO_ReadPin(ECHO_PORT, ECHO2_PIN)));

    // Record the time ECHO2 is HIGH
    startTimeSensor2 = __HAL_TIM_GET_COUNTER(&htim2);
    while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO2_PIN)) {
        localTimeSensor2 = __HAL_TIM_GET_COUNTER(&htim2) - startTimeSensor2;
    }

    // Calculate distance (speed of sound: 34300 cm/s)
    return (localTimeSensor2 * 0.034 / 2);
}

void delayMicroseconds(uint32_t us) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

//Sends a clock pulse to LCD via enable pin for commands or data
void LCD_STROBE()
{
	HAL_GPIO_WritePin(GPIOB,LCD_EN,GPIO_PIN_SET);
	HAL_Delay(0.1);
	HAL_GPIO_WritePin(GPIOB,LCD_EN,GPIO_PIN_RESET);
	HAL_Delay(0.1);
}

//Sends commands byte to LCD
void lcd_write_cmd(unsigned char c)
{
	unsigned int d;
	d = c;
	d = (d << 4) & 0x0F00;
	GPIOB->ODR = d;
	HAL_GPIO_WritePin(GPIOB,LCD_RS,GPIO_PIN_RESET);
	LCD_STROBE();
	d = c;
	d = (d << 8) & 0x0F00;
	GPIOB->ODR = d;
	HAL_GPIO_WritePin(GPIOB,LCD_RS,GPIO_PIN_RESET);
	LCD_STROBE();
	HAL_Delay(0.1);
}

//Sends data to LCD
void lcd_write_data(unsigned char c)
{
	unsigned int d;

	d = c;
	d = (d << 4) & 0x0F00;
	GPIOB->ODR = d;
	HAL_GPIO_WritePin(GPIOB,LCD_RS,GPIO_PIN_SET);
	LCD_STROBE();
	d = c;
	d = (d << 8) & 0x0F00;
	GPIOB->ODR = d;
	HAL_GPIO_WritePin(GPIOB,LCD_RS,GPIO_PIN_SET);
	LCD_STROBE();
}

//Clears LCD
void lcd_clear(void)
{
	lcd_write_cmd(0x1);
	HAL_Delay(5);
}

//Displays string output on LCD
void lcd_puts(const char * s)
{
	while(* s)
		lcd_write_data(* s++);
}

//Displays a single character on LCD
void lcd_putch(char c)
{
	unsigned int d;
	d = c;
	d = (d << 4) & 0x0F00;
	GPIOB->ODR = d;
	HAL_GPIO_WritePin(GPIOB,LCD_RS,GPIO_PIN_SET);
	LCD_STROBE();
	d = c;
	d = (d << 8) & 0x0F00;
	GPIOB->ODR = d;
	HAL_GPIO_WritePin(GPIOB,LCD_RS,GPIO_PIN_SET);
	LCD_STROBE();
}

//Positions cursor to a specified row and column
void lcd_goto(int col, int row)
{
	char address;
	if(row == 0)address = 0;
	if(row == 1)address = 0x40;
	address += col - 1;
	lcd_write_cmd(0x80 | address);
}

//Initializes LCD
void lcd_init(void)
{
	GPIOB->ODR = 0;
	HAL_Delay(50);
	GPIOB->ODR = 0x0300;
	LCD_STROBE();
	HAL_Delay(30);
	LCD_STROBE();
	HAL_Delay(20);
	LCD_STROBE();
	HAL_Delay(20);
	GPIOB->ODR = 0x0200;
	LCD_STROBE();
	HAL_Delay(5);
	lcd_write_cmd(0x28);
	HAL_Delay(5);
	lcd_write_cmd(0x0C);
	HAL_Delay(5);
	lcd_write_cmd(0x01);
	HAL_Delay(5);
	lcd_write_cmd(0x06);
	HAL_Delay(5);
}

//
void pedestrian_crossing()
{
	//LCD displays wait
	lcd_clear();
	lcd_goto(7, 0);
	lcd_puts("WAIT");

	//Pole 1 yellow, Pole 2 red
	HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
	HAL_Delay(3000);

	// Pole 1 red, Pole 2 red
	HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
	HAL_Delay(3000);

	// Pole 1 red, Pole 2 green
	HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_SET);

    // Display "CROSS" centered on the top line
    lcd_clear();
    lcd_goto(6, 0);
    lcd_puts("CROSS");
    HAL_Delay(3000);

    // Countdown from 10 to 1 on the second line
    for (int i = 10; i >= 1; i--) {

        lcd_goto(8, 1);
        lcd_puts("  "); // Clears previous countdown characters

        lcd_goto(8, 1);
        char buffer[3];
        snprintf(buffer, sizeof(buffer), "%d", i);
        lcd_puts(buffer);
        HAL_Delay(1200);
    }

    // Blink when countdown reaches 0
    for (int i = 0; i < 6; i++) {

        lcd_goto(8, 1);
        if (i % 2 == 0) {
        	lcd_goto(5,0);
        	lcd_puts("  STOP    ");

        	lcd_goto(8,1);
            lcd_puts("0");
        }

        else {
        	lcd_goto(8,1);
            lcd_puts(" ");
        }

        HAL_Delay(800);
    }

    // Replace "CROSS" with "STOP" on the top line and clear bottom line
    lcd_goto(5, 0);
    lcd_puts("  STOP    ");

    lcd_goto(8, 1);
    lcd_puts(" ");

    // Pole 1 still red, but warning for Pole 2
    HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
    HAL_Delay(3000);

    //Display final warning
    lcd_clear();
    lcd_goto(2, 0);
    lcd_puts("IF YOU GET HIT");
    lcd_goto(3, 1);
    lcd_puts("ITS ON YOU");
    // Pole 1 red, Pole 2 red
    HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);

    HAL_Delay(1500);

    lcd_clear();
    lcd_goto(3, 0);
    lcd_puts("DO NOT CROSS");

    // Pole 1 red, Pole 2 red
    HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_PORT, LED4_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_PORT, LED5_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED6_PIN, GPIO_PIN_RESET);
}

//Displays do not cross on LCD while waiting for button push
void waiting_for_pedestrian(void)
{
	lcd_clear();
	    lcd_goto(3, 0);
	    lcd_puts("DO NOT CROSS");
}
