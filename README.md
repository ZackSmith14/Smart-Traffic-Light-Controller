# Smart Traffic Light Controller  

This project simulates a **Smart Traffic Light Controller** designed to manage traffic flow efficiently using sensor data and user inputs. The implementation is based on the STM32 Nucleo-L476RG microcontroller and is developed in **STM32CubeIDE**.
The system includes traffic light LEDs, ultrasonic distance sensors, an LCD keypad shield for user interaction, and a pushbutton for manual control.

## Features  
- **Traffic Light Control:** Simulates standard traffic light sequences (red, yellow, green).  
- **Ultrasonic Sensor Integration:** Detects vehicles approaching the intersection to dynamically adjust the light cycle.  
- **LCD Interface:** Displays crosswalk messages.  
- **Pushbutton Control:** Allows manual override for specific scenarios.  
- **UART Communication:** Debugging and real-time system monitoring via a serial interface.  

## Hardware Components  
- STM32 Nucleo-L476RG microcontroller  
- HC-SR04 ultrasonic distance sensors  
- Mini traffic light LEDs  
- LCD module (HD44780 1602 display)  
- Pushbutton module

## Software Tools  
- STM32CubeIDE (for embedded software development)  
- PuTTY (or any serial communication tool for UART debugging)  

