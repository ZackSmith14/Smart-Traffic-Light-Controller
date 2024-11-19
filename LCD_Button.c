#include "main.h"
#include "stdio.h"

#define LCD_EN GPIO_PIN_7
#define LCD_RS GPIO_PIN_6
#define LCD_D4 GPIO_PIN_8
#define LCD_D5 GPIO_PIN_9
#define LCD_D6 GPIO_PIN_10
#define LCD_D7 GPIO_PIN_11
#define Button GPIO_PIN_12

void LCD_STROBE()
{
	HAL_GPIO_WritePin(GPIOB,LCD_EN,GPIO_PIN_SET);
	HAL_Delay(0.1);
	HAL_GPIO_WritePin(GPIOB,LCD_EN,GPIO_PIN_RESET);
	HAL_Delay(0.1);
}

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

void lcd_clear(void)
{
	lcd_write_cmd(0x1);
	HAL_Delay(5);
}

void lcd_puts(const char * s)
{
	while(* s)
		lcd_write_data(* s++);
}

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

void lcd_goto(int col, int row)
{
	char address;
	if(row == 0)address = 0;
	if(row == 1)address = 0x40;
	address += col - 1;
	lcd_write_cmd(0x80 | address);
}

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

void display_cross_countdown()
{
    // Display "CROSS" centered on the top line
    lcd_clear();
    lcd_goto(6, 0); // Adjust the column for centering based on LCD size
    lcd_puts("CROSS");

    // 5-second delay before starting the countdown
    HAL_Delay(3000);

    // Countdown from 10 to 1 on the second line
    for (int i = 10; i >= 1; i--) {
        lcd_goto(8, 1); // Adjust the column for centering based on LCD size
        lcd_puts("  "); // Clear previous countdown characters
        lcd_goto(8, 1); // Move back to the same position
        char buffer[3];
        snprintf(buffer, sizeof(buffer), "%d", i); // Safely format countdown number
        lcd_puts(buffer);
        HAL_Delay(1200); // 1.5-second delay for countdown
    }

    // When countdown reaches 0, blink the 0
    for (int i = 0; i < 6; i++) { // Blink 6 times
        lcd_goto(8, 1); // Move to countdown position
        if (i % 2 == 0) {
        	lcd_goto(5,0);
        	lcd_puts("  STOP    ");
        	lcd_goto(8,1);
            lcd_puts("0"); // Display "0"
        } else {
        	lcd_goto(8,1);
            lcd_puts(" "); // Clear the display (blinking effect)
        }
        HAL_Delay(800); // Half-second delay to blink
    }

    // Replace "CROSS" with "STOP" on the top line
    lcd_goto(5, 0); // Adjust column position if needed
    lcd_puts("  STOP    "); // Clear any extra characters
    lcd_goto(8, 1);
    lcd_puts(" ");

    // 3-second delay before changing to "DO NOT CROSS"
    HAL_Delay(3000);

    lcd_clear();
    lcd_goto(2, 0);
    lcd_puts("IF YOU GET HIT");
    lcd_goto(3, 1);
    lcd_puts("ITS ON YOU");

    HAL_Delay(1500);

    // Display "DO NOT CROSS" on the top line
    lcd_clear();
    lcd_goto(3, 0); // Adjust column for centering if necessary
    lcd_puts("DO NOT CROSS");
}

void wating_for_pedestrian()
{
	lcd_clear();
	    lcd_goto(3, 0);
	    lcd_puts("DO NOT CROSS");
}
