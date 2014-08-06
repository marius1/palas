#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "lcd.h"
#include "ascii.h"
#include "large_numbers.h"
#include "battery.h"

void lcd_init()
{
	LCD_DDR |= (1 << LCD_DC) | (1 << LCD_CE) | (1 << LCD_MOSI) | (1 << LCD_SCK);	
	LCD_DDR2 |= (1 << LCD_RST) | (1 << LCD_BL);
		
	LCD_DDR2 |=  (1 << LCD_RST);
	LCD_DDR2 &= ~(1 << LCD_RST);
	_delay_us(1);
	
	SPCR |= (1 << SPE) | (1 << MSTR);
	SPSR |= (1 << SPI2X);

	lcd_write( 0x21, LCD_CMD); // LCD Extended Commands
	lcd_write( 0xA1, LCD_CMD); // Set LCD Vop (Contrast)
	lcd_write( 0x04, LCD_CMD); // Set Temp coefficent
	lcd_write( 0x13, LCD_CMD); // LCD bias mode 1:48: Try 0x13 or 0x14
	lcd_write( 0x20, LCD_CMD); // We must send 0x20 before modifying the display control mode
	lcd_write( 0x0C, LCD_CMD); // Set display control, normal mode (0x0C). 0x0D for inverse
	
	lcd_clear();
}

void lcd_write(uint8_t data, uint8_t mode)
{
	LCD_PORT &= ~(1 << LCD_CE );
	
	if ( mode == LCD_DATA ) // data
		LCD_PORT |= (1 << LCD_DC);
	else // command
		LCD_PORT &= ~(1 << LCD_DC);
		
	SPDR = data;
	
	while(!(SPSR & (1<<SPIF)));	
	
	LCD_PORT |= (1 << LCD_CE );
}

void lcd_set_contrast(uint8_t contrast)
{
	if (contrast > 0x7F)
		contrast = 0x7F;

	lcd_write( 0x21, LCD_CMD);
	lcd_write( 0x80 | contrast, LCD_CMD);
	lcd_write( 0x20, LCD_CMD);
}

void lcd_clear()
{
	for (int i = 0 ; i < (LCD_X * LCD_Y / 8); i++)
		lcd_write(0x00, LCD_DATA);
		
	lcd_goto_xy(0, 0);
}

void lcd_write_char(uint8_t d)
{
	lcd_write(0x00, LCD_DATA);
	
	for (int i = 0; i < 5; i++)
		lcd_write(ASCII[d - ASCII_OFFSET][i], LCD_DATA);
	
	lcd_write(0x00, LCD_DATA);
}

void lcd_write_string(char *str) 
{
  while (*str)
    lcd_write_char(*str++);
}

void lcd_write_string_p(PGM_P str) 
{
	while(1)
    {
        uint8_t b = pgm_read_byte_near(str++);
        if(!b)
            break;

        lcd_write_char(b);
    }
}

void lcd_write_large_number(uint8_t number, uint8_t x, uint8_t y)
{
	uint8_t i;
	for (i = 0; i < 57; i++)
	{
		if (i%19 == 0) 
		{
			lcd_write(0x40 | (y+(i/19)), LCD_CMD);
			lcd_write(0x80 | x, LCD_CMD);
		}
		lcd_write(large_numbers[number][i], LCD_DATA);		
	}	
}

// TODO, should be WAY smarter calculation
void lcd_print_battery(uint16_t value)
{
	uint8_t i,j,k = 2;
	
	lcd_goto_xy(0, 5);
	for(i = 0; i < 16; i++)
		lcd_write(battery_house[i], LCD_DATA);
	
	for(j = 0; j < value / 240; j++)
	{
		lcd_goto_xy(k, 5);
		lcd_write(0xBD, LCD_DATA);
		lcd_write(0xBD, LCD_DATA);
		k += 3;
	}
}

void lcd_print_dubble_number(uint8_t number)
{
	uint8_t tmp = 0;
	
	tmp = number / 10;
	lcd_write_char(tmp + NUMBER_OFFSET);
	tmp = number % 10;
	lcd_write_char(tmp + NUMBER_OFFSET);
}

void lcd_goto_xy(uint8_t x, uint8_t y)
{
	lcd_write(0x80 | x, LCD_CMD);
	lcd_write(0x40 | y, LCD_CMD);
}

uint8_t lcd_get_backlight()
{
	return (LCD_PORT2 & (1 << LCD_BL)) > 0;
}

void lcd_set_backlight(uint8_t value)
{
	if (value == 1)
		LCD_PORT2   |=  (1 << LCD_BL);
	else
		LCD_PORT2   &= ~(1 << LCD_BL); 
}

void lcd_toggle_backlight()
{
	LCD_PORT2 ^= (1 << LCD_BL);
}