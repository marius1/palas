#include "global.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <avr/io.h>		// include I/O definitions (port names, pin names, etc)
#include <avr/interrupt.h>
#include <util/delay.h>

#include "uart/uart.h"
#include "ascii.h"

volatile uint16_t pulse = 0;
volatile uint16_t start = 0xFFFF;
volatile uint32_t time = 0;

void print_time();
void print_rpm();
void lcd_init();
void lcd_write(uint8_t data, uint8_t mode);
void lcd_clear();
void lcd_gotoXY(uint8_t x, uint8_t y);
void lcd_print_time();
void lcd_write_char(uint8_t d);
void lcd_print_dubble_number(uint8_t number);

#define status_led_init() 	(DDRD    |=  (1 << PD7))
#define status_led_on() 	(PORTD   |=  (1 << PD7))
#define status_led_off() 	(PORTD   &= ~(1 << PD7)) 

#define LCD_CMD 0
#define LCD_DATA 1

#define LCD_X     84
#define LCD_Y     48

#define NUMBER_OFFSET 0x30

int main(void)
{
	status_led_init();
	uart_init();
	lcd_init();
	
	// setup timer with ICP1 interrupt
	TCCR1B |= (1 << CS12) | (1 << CS10);	// prescaler 1024	
	TCCR1B |= (1 << WGM12);
	TIMSK1 |= (1 << ICIE1);
	
	// setup 1ms timer
	TCCR0A |= (1 << WGM01);
	OCR0A = 0x7C;
	TIMSK0 |= (1 << OCIE0A);
	TCCR0B |= (1 << CS01) | (1 << CS00);
	
	sei();	
	
	uart_puts_p(PSTR("Init done!\n"));
	
	while(1)
	{
		//print_rpm();		
		lcd_print_time();
	}
	return 0;
}

void lcd_print_time()
{
	uint8_t seconds = 0, minutes = 0, hours = 0, miliseconds = 0;		
	uint32_t ts = time / 1000;
	uint32_t tc = ts / 60;
	
	miliseconds = (time % 1000) / 100;
	seconds = ts % 60;
	minutes = tc % 60;
	hours = tc / 60;-

	lcd_gotoXY(7,0);
	
	lcd_print_dubble_number(hours);
	lcd_write_char(':');
	lcd_print_dubble_number(minutes);
	lcd_write_char(':');
	lcd_print_dubble_number(seconds);
	lcd_write_char('.');
	lcd_write_char(miliseconds + NUMBER_OFFSET);	
}

void lcd_print_dubble_number(uint8_t number)
{
	uint8_t tmp = 0;
	
	tmp = number / 10;
	lcd_write_char(tmp + NUMBER_OFFSET);
	tmp = number % 10;
	lcd_write_char(tmp + NUMBER_OFFSET);
}

void lcd_write_char(uint8_t d)
{
	lcd_write(0x00, LCD_DATA);
	
	for (int i = 0 ; i < 5 ; i++)
		lcd_write(ASCII[d - 0x20][i], LCD_DATA);
	
	lcd_write(0x00, LCD_DATA);
}

void lcd_init()
{
	DDRB |= (1 << PB1); // DC
	DDRB |= (1 << PB2); // SS -> CE
	DDRB |= (1 << PB3); // MOSI
	DDRB |= (1 << PB5); // SCK
	
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

void lcd_clear()
{
	for (int i = 0 ; i < (LCD_X * LCD_Y / 8) ; i++)
		lcd_write(0x00, LCD_DATA);
		
	lcd_gotoXY(0, 0);
}

void lcd_gotoXY(uint8_t x, uint8_t y)
{
	lcd_write(0x80 | x, LCD_CMD);
	lcd_write(0x40 | y, LCD_CMD);
}

void lcd_write(uint8_t data, uint8_t mode)
{
	PORTB &= ~(1 << PB2 );
	
	if ( mode == LCD_DATA ) // data
		PORTB |= (1 << PB1);
	else // command
		PORTB &= ~(1 << PB1);
		
	SPDR = data;
	
	while(!(SPSR & (1<<SPIF)));	
	
	PORTB |= (1 << PB2 );
}

void print_rpm()
{
	uint16_t rpm = 0;
	
	if (pulse > 0)
	{
		rpm = 468750L / pulse;
		uart_putw_dec(rpm);
		uart_putc('\n');
		pulse = 0;
	}
}

ISR(TIMER0_COMPA_vect)
{
	time++;
	if (time >= 359999999)
		time = 0;
}

ISR(TIMER1_CAPT_vect)
{
	uint16_t t = ICR1;
	
	if ( t > start)
		pulse = t - start;
	else
		pulse = (0xFFFF - start) + t;
	
	start = t;
}
