#include <stdio.h>
#include <avr/io.h>

#include "lcd.h"
#include "ascii.h"

void lcd_init()
{
	LCD_DDR |= (1 << PB1) | (1 << PB2) | (1 << PB3) | (1 << PB5);
	
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

void lcd_clear()
{
	for (int i = 0 ; i < (LCD_X * LCD_Y / 8) ; i++)
		lcd_write(0x00, LCD_DATA);
		
	lcd_goto_xy(0, 0);
}

void lcd_write_char(uint8_t d)
{
	lcd_write(0x00, LCD_DATA);
	
	for (int i = 0 ; i < 5 ; i++)
		lcd_write(ASCII[d - 0x20][i], LCD_DATA);
	
	lcd_write(0x00, LCD_DATA);
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