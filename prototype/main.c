#include "global.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <avr/io.h>		// include I/O definitions (port names, pin names, etc)
#include <avr/interrupt.h>
#include <util/delay.h>

#include "uart/uart.h"
#include "lcd/lcd.h"

#define key_state(port,pin) !((port) & (1<<pin))

volatile uint16_t pulse = 0;
volatile uint16_t start = 0;
volatile uint32_t time = 0;
volatile uint16_t battery = 0;

void buttons_init();
void timers_init();	
void print_rpm();
void print_time();
void clear_rpm_area();
void adc_init();

int main(void)
{
	
	uart_init();
	lcd_init();
	lcd_set_contrast(0x34);
	//lcd_set_contrast(0x29);
	lcd_set_backlight(1);
	timers_init();	
	adc_init();
	
	sei();	
	
	uart_puts_p(PSTR("Init done!\n"));
	
	lcd_write_large_number(0, 30, 2);
	
	while(1)
	{	
		print_rpm();		
		print_time();
		
		lcd_print_battery(battery);
	}
	return 0;
}

ISR(PCINT1_vect)
{
	if (key_state(PORTC, PC3))
	{
		uart_putc('3');
		uart_putc('\n');
	}
	
	if (key_state(PORTC, PC4))
	{
		uart_putc('4');
		uart_putc('\n');
	}
	
	if (key_state(PORTC, PC5))
	{
		uart_putc('5');
		uart_putc('\n');
	}
}

void buttons_init()
{
	// set as input
	DDRC &= ~(1<<PC3);
	DDRC &= ~(1<<PC4);
	DDRC &= ~(1<<PC5);
	
	// set interupts on
	PCICR |= (1<<PCIE1);
	PCMSK1 |= (1<<PCINT11);
	PCMSK1 |= (1<<PCINT12);
	PCMSK1 |= (1<<PCINT13);
}

void print_time()
{
	uint8_t seconds = 0, minutes = 0, hours = 0, miliseconds = 0;		
	uint32_t ts = time / 1000;
	uint32_t tc = ts / 60;
	
	miliseconds = (time % 1000) / 100;
	seconds = ts % 60;
	minutes = tc % 60;
	hours = tc / 60;

	lcd_goto_xy(7,0);
	
	lcd_print_dubble_number(hours);
	lcd_write_char(10); //':');
	lcd_print_dubble_number(minutes);
	lcd_write_char(10); //':');
	lcd_print_dubble_number(seconds);
	lcd_write_char(10); //'.');
	lcd_write_char(miliseconds + NUMBER_OFFSET);	
}

void clear_rpm_area()
{
	uint8_t i;
	lcd_goto_xy(0, 2);
	for( i = 0; i < 252; i++)
		lcd_write(0x00, LCD_DATA);
}

void print_rpm()
{
	uint16_t rpm = 0;
	uint8_t offset = 30;
	uint8_t s;
	uint8_t t;
	uint8_t h;
	
	if (pulse > 0)
	{
		clear_rpm_area();
		rpm = 468750L / pulse;
		
		s = rpm % 10;
		t = ((rpm % 100) - s) / 10;
		h = (rpm - t - s) / 100;

		offset -= (t > 0) ? 10 : 0;
		offset -= (h > 0) ? 10 : 0;
		
		if (h > 0)
			lcd_write_large_number(h, 42 + offset, 2);
		
		if (t > 0)
			lcd_write_large_number(t, 21 + offset, 2);
			
		lcd_write_large_number(s, 0 + offset, 2);
		
		uart_putw_dec(rpm);
		uart_putc('\n');
		
		pulse = 0;
	}
}

void adc_init()
{
	// REFS0: AVCC with external capacitor at AREF pin
	ADMUX = (1 << REFS0);
	// no MUX is ADC0
	// ADMUX |= (1 << MUX3) | (1 << MUX2) | (1 << MUX1); // 1.1 Vgb
	
	// ADEN: ADC Enable
	// ADPS2 | ADPS1: Division factor 64
	// ADIE: interupt enabled
	// ADFR: free running mode
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADIE) | (1 << ADATE);
	// TODO 
	//ADCSRB = ;
	
	ADCSRA |= (1 << ADSC);
}

ISR(ADC_vect)
{
	battery = (ADCL | (ADCH << 8));
}

void timers_init()
{
	// setup timer with ICP1 interrupt
	TCCR1B |= (1 << CS12) | (1 << CS10);	// prescaler 1024	
	TCCR1B |= (1 << WGM12);
	TIMSK1 |= (1 << ICIE1);
	
	// setup 1ms timer
	TCCR0A |= (1 << WGM01);
	OCR0A = 0x7C;
	TIMSK0 |= (1 << OCIE0A);
	
	TCCR0B |= (1 << CS01) | (1 << CS00);
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
