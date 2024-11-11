/*
 * app_FinalProject.c
 *
 *  Created on: Nov 4, 2024
 *      Author: Anthony Tran
 */

/* Includes ------------------------------------------------------------------*/
#include "app.h"
#include <stdio.h>
#include "stdlib.h"
#include "string.h"

/* Private define ------------------------------------------------------------*/
#define     TRIG_PIN          GPIO_PIN_8   // Define Trigger pin as PA8
#define     TRIG_PORT         GPIOA        // Define Trigger port as GPIOA
#define     ECHO_PIN          GPIO_PIN_9   // Define Echo pin as PA9
#define     ECHO_PORT         GPIOA        // Define Echo port as GPIOA

#define     LED_PORT          GPIOA
#define     LED_PIN           GPIO_PIN_5
#define     LED_MODE_OFF      0
#define     LED_MODE_ON       1
#define     LED_MODE_FLASHING 2

extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim2;

/* Function prototypes */
void delayMicroseconds(uint32_t us);
uint32_t measureDistance(void);
void ShowCommands(void);
void UART_TransmitString(UART_HandleTypeDef *p_huart, char a_string[], int newline);

/* Private variables ---------------------------------------------------------*/
volatile char rxData;  // One byte data received from UART
volatile int ledMode = LED_MODE_FLASHING;  // LED mode initialization

void App_Init(void)
{
    // Initialize LED pin for debugging
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Configure LED Pin as Output
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    // Turn ON LED to check if initialization works
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);  // Directly turn LED ON for initialization check

    // Initial message to indicate successful startup
    UART_TransmitString(&huart2, "-----------------", 1);
    UART_TransmitString(&huart2, "~ Nucleo-L476RG ~", 1);
    UART_TransmitString(&huart2, "-----------------", 1);
    ShowCommands();

    // Start UART receive interrupt
    HAL_UART_Receive_IT(&huart2, (uint8_t*) &rxData, 1);  // Start the Rx interrupt.

    // Initialize UART and Timer
    HAL_UART_Init(&huart2);
    HAL_TIM_Base_Start(&htim2);  // Ensure the timer is started

    // Initialize GPIO for Trigger and Echo pins
    GPIO_InitStruct.Pin = TRIG_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TRIG_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = ECHO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(ECHO_PORT, &GPIO_InitStruct);

    // Debug message for successful initialization
    char initMsg[] = "HC-SR04 and UART initialized\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)initMsg, sizeof(initMsg) - 1, HAL_MAX_DELAY);

    // Start UART receive interrupt again for any command inputs
    HAL_UART_Receive_IT(&huart2, (uint8_t*)&rxData, 1);  // Start the Rx interrupt again
}

void App_MainLoop(void)
{
    char buffer[50];
    uint32_t distance;

    // LED control based on the current ledMode
    if (ledMode == LED_MODE_FLASHING) {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);  // Toggle LED for flashing effect
        HAL_Delay(500);  // Delay to control the flashing speed (adjust as necessary)
    } else if (ledMode == LED_MODE_ON) {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);  // Turn LED ON
    } else if (ledMode == LED_MODE_OFF) {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);  // Turn LED OFF
    }

    // Measure distance using the sensor
    distance = measureDistance();

    // Format the distance as a string and send over UART
    int len = snprintf(buffer, sizeof(buffer), "Distance: %lu cm\r\n", distance);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);

    // Adjust LED behavior based on distance
    if (distance < 10) {
        ledMode = LED_MODE_ON;  // Turn on LED if distance < 10 cm
    } else if (distance < 50) {
        ledMode = LED_MODE_FLASHING;  // Flash the LED if distance < 50 cm
    } else {
        ledMode = LED_MODE_OFF;  // Turn off LED if distance > 50 cm
    }

    // Delay to avoid flooding the serial monitor
    HAL_Delay(500);  // Adjust delay for more responsive behavior
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *p_huart)
{
    // Process the data received from UART
    switch (rxData) {
        case 'I':
        case 'i':
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);  // Turn ON LED
            ledMode = LED_MODE_ON;  // Update ledMode to ON
            break;
        case 'O':
        case 'o':
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);  // Turn OFF LED
            ledMode = LED_MODE_OFF;  // Update ledMode to OFF
            break;
        case 'F':
        case 'f':
            ledMode = LED_MODE_FLASHING;  // Set LED to flashing mode
            break;
        case 'H':
        case 'h':
            ShowCommands();  // Show available commands
            break;
    }

    // Restart the Rx interrupt
    HAL_UART_Receive_IT(p_huart, (uint8_t*)&rxData, 1);
}

void ShowCommands(void)
{
    UART_TransmitString(&huart2, "Type on keyboard to send command from PC to MCU:", 1);
    UART_TransmitString(&huart2, "> I: turn on LED, O: turn off LED, F: flashing LED, H: show commands", 1);
}

void UART_TransmitString(UART_HandleTypeDef *p_huart, char a_string[], int newline)
{
    HAL_UART_Transmit(p_huart, (uint8_t*) a_string, strlen(a_string), HAL_MAX_DELAY);
    if (newline != 0) {
        HAL_UART_Transmit(p_huart, (uint8_t*) "\n\r", 2, HAL_MAX_DELAY);
    }
}

uint32_t measureDistance(void)
{
    uint32_t start_time, stop_time, pulse_duration;
    uint32_t timeout = 100000;  // Timeout limit for waiting

    // Trigger the sensor by sending a 10µs pulse
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
    delayMicroseconds(10);  // 10µs delay for trigger pulse
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);

    // Wait for the Echo pin to go high (start of the echo pulse)
    while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN) == GPIO_PIN_RESET)
    {
        if (timeout-- == 0) return 0;  // Exit if Echo doesn't go high within the limit
    }

    // Record the start time
    start_time = __HAL_TIM_GET_COUNTER(&htim2);

    // Reset timeout and wait for the Echo pin to go low (end of the echo pulse)
    timeout = 100000;
    while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN) == GPIO_PIN_SET)
    {
        if (timeout-- == 0) return 0;  // Exit if Echo doesn't go low within the limit
    }

    // Record the stop time
    stop_time = __HAL_TIM_GET_COUNTER(&htim2);

    // Calculate the pulse duration
    pulse_duration = stop_time - start_time;

    // Calculate the distance based on the pulse duration
    uint32_t distance_cm = pulse_duration / 58;  // Convert to cm (adjust factor based on your sensor)

    return distance_cm;
}

void delayMicroseconds(uint32_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);  // Reset timer
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);  // Wait for specified delay
}
