#ifndef LCD_H
#define LCD_H

#include <avr/pgmspace.h>

#define LCD_CMD 	0
#define LCD_DATA 	1

#define LCD_DDR 	DDRB
#define LCD_PORT 	PORTB
#define LCD_DC 		PB1
#define LCD_CE		PB2
#define LCD_MOSI	PB3
#define LCD_SCK		PB5

#define LCD_DDR2 	DDRC
#define LCD_PORT2 	PORTC
#define LCD_RST		PC1
#define LCD_BL		PC2

#define LCD_X     84
#define LCD_Y     48

#define ASCII_OFFSET 0x20 
//0x2E
#define NUMBER_OFFSET 0x30
//ASCII_OFFSET + 0x2

void lcd_init();
void lcd_write(uint8_t data, uint8_t mode);
void lcd_set_contrast(uint8_t contrast);
void lcd_clear();
void lcd_goto_xy(uint8_t x, uint8_t y);
void lcd_write_char(uint8_t d);
void lcd_write_string(char *str);
void lcd_write_string_p(PGM_P str);
void lcd_write_large_number(uint8_t number, uint8_t x, uint8_t y);
void lcd_print_dubble_number(uint8_t number);
void lcd_print_battery(uint16_t value);
uint8_t lcd_get_backlight();
void lcd_set_backlight(uint8_t value);
void lcd_toggle_backlight();

#endif