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

#define BUTTON_PORT		PORTC
#define BUTTON_DDR		DDRC
#define BUTTON_PIN		PINC
#define BUTTON_UP		PC3
#define BUTTON_ENTER	PC4
#define BUTTON_DOWN		PC5

#define MODE_RPM		0
#define MODE_MENU		1
#define MAX_MENU_POS 	2

#define MENU_BL			0
#define MENU_CONTRAST	1
#define MENU_BACK		2

volatile uint16_t pulse = 0;
volatile uint8_t prevRpm = 1;
volatile uint32_t time = 0;
volatile uint16_t battery = 0;
volatile uint16_t button_state[3];
volatile uint8_t mode = MODE_RPM;
volatile uint8_t menu_pos = 0;

void buttons_init();
void timers_init();	
void print_rpm();
void print_time();
void print_buttons();
void clear_rpm_area();
void adc_init();
void timers_stop();
void timers_start();
void timers_clear();
uint8_t timers_running();
void print_menu();

int main(void)
{
	
	uart_init();
	lcd_init();
	lcd_set_contrast(0x45);
	lcd_set_backlight(0);
	timers_init();	
	adc_init();
	buttons_init();
	
	sei();	
	
	uart_puts_p(PSTR("Init done!\n"));
		
	while(1)
	{
		switch(mode)
		{
			case MODE_RPM:
				print_rpm();
				print_time();
				print_buttons();
				break;
			case MODE_MENU:
				print_menu();
				break;
		}
		
		lcd_print_battery(battery);
	}
	return 0;
}

const unsigned char play[][10] = {
	{ 0xF8, 0xF8, 0x00, 0xF8, 0xF8, 0xF0, 0xE0, 0xC0, 0xC0, 0x80 },
	{ 0x0F, 0x0F, 0x00, 0x0F, 0x0F, 0x07, 0x03, 0x01, 0x01, 0x00 }
};

const unsigned char reset[][11] = {
	{ 0x78, 0x70, 0x78, 0x5C, 0x0C, 0x0C, 0x0C, 0x1C, 0x38, 0xF0, 0xE0 },
	{ 0x00, 0x06, 0x0E, 0x1C, 0x18, 0x18, 0x18, 0x1C, 0x0E, 0x07, 0x03 }
};

const unsigned char settings[][10] = {
	{ 0x00, 0xF8, 0x00, 0x80, 0xF8, 0x80, 0x30, 0xF8, 0x30, 0x00 },
	{ 0x0C, 0x1F, 0x0C, 0x01, 0x1F, 0x01, 0x00, 0x1F, 0x00, 0x00 }
};

void print_buttons()
{
	uint8_t i, j;
	
	for(i = 0; i < 2; i++)
	{
		lcd_goto_xy(74,i * 1);
		for(j = 0; j < 10; j++)
			lcd_write(play[i][j], LCD_DATA);
	}
	
	for(i = 0; i < 2; i++)
	{
		lcd_goto_xy(73,(i * 1) + 2);
		for(j = 0; j < 11; j++)
			lcd_write(reset[i][j], LCD_DATA);
	}
	
	for(i = 0; i < 2; i++)
	{
		lcd_goto_xy(74, (i * 1) + 4);
		for(j = 0; j < 10; j++)
			lcd_write(settings[i][j], LCD_DATA);
	}
}

void print_menu()
{
	uint8_t i = 0;

	lcd_goto_xy(14,0);
	lcd_write_string("BL ");
	if (lcd_get_backlight())
		lcd_write_string("(on) ");
	else
		lcd_write_string("(off)");
	
	lcd_goto_xy(14,1);
	lcd_write_string("Contrast");
	lcd_goto_xy(14,2);
	lcd_write_string("Back");
	
	for(i = 0; i <= MAX_MENU_POS; i++)
	{
		lcd_goto_xy(0, i);
		lcd_write_char(' ');
	}
	
	lcd_goto_xy(0, menu_pos * 1);
	lcd_write_char('>');
}

ISR(PCINT1_vect)
{
	if (key_state(BUTTON_PIN, BUTTON_UP))
	{
		if (mode == MODE_RPM) 
		{
			if (timers_running())
				timers_stop();
			else
				timers_start();
		}
		else
		{
			menu_pos = (menu_pos == 0) ? MAX_MENU_POS : menu_pos - 1;
		}
	}
	
	if (key_state(BUTTON_PIN, BUTTON_ENTER))
	{
		if (mode == MODE_RPM) 
		{
			if (!timers_running())
				timers_clear();
		}
		else
		{
			menu_pos = (menu_pos == MAX_MENU_POS) ? 0 : menu_pos + 1;
		}
	}
	
	if (key_state(BUTTON_PIN, BUTTON_DOWN))
	{
		if (mode == MODE_RPM) 
		{
			lcd_clear();
			mode = MODE_MENU;
		}
		else
		{
			switch(menu_pos)
			{
				case MENU_BL:
					lcd_toggle_backlight();
					break;
				case MENU_CONTRAST:
					break;
				case MENU_BACK:
					lcd_clear();
					mode = MODE_RPM;
					menu_pos = 0;
					break;
				
			}
		}
	}
}

void buttons_init()
{
	// set as input
	BUTTON_DDR &= ~(1 << BUTTON_UP);
	BUTTON_DDR &= ~(1 << BUTTON_ENTER);
	BUTTON_DDR &= ~(1 << BUTTON_DOWN);
	
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

	lcd_goto_xy(0,0);
	
	lcd_print_dubble_number(hours);
	lcd_write_char(':');
	lcd_print_dubble_number(minutes);
	lcd_write_char(':');
	lcd_print_dubble_number(seconds);
	lcd_write_char('.');
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
		rpm = 468720 / pulse;
		rpm = (rpm > 999) ? 999 : rpm;
	}
	else
	{
		rpm = 0;
	}
	
	if ( rpm != prevRpm )
	{
		s = rpm % 10;
		t = ((rpm % 100) - s) / 10;
		h = (rpm - t - s) / 100;
		
		clear_rpm_area();
		
		if (h > 0)
		{
			lcd_write_large_number(h, 10, 2);
			lcd_write_large_number(t, 31, 2);
			lcd_write_large_number(s, 52, 2);
		}
		else if (t > 0 || ( t == 0 && h > 0))
		{
			lcd_write_large_number(t, 20, 2);
			lcd_write_large_number(s, 41, 2);
		}
		else
		{			
			lcd_write_large_number(s, 30, 2);
		}
		
		prevRpm = rpm;
		uart_putw_dec(rpm);
		uart_putc('\n');
	}
}

void adc_init()
{
	// REFS0 + REFS1: Internal 1.1v
	ADMUX = (1 << REFS0) | (1 << REFS1);
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
	TIMSK1 |= (1 << ICIE1) | (1 << TOIE1);
	
	
	// setup 1ms timer
	TCCR0A |= (1 << WGM01);
	OCR0A = 0x7C;
	TIMSK0 |= (1 << OCIE0A);
}

uint8_t timers_running()
{
	uint8_t tmp = (1 << CS01) | (1 << CS00);
	return (TCCR0B & tmp) > 0;
}

void timers_start()
{
	TCCR0B |= (1 << CS01) | (1 << CS00);
}

void timers_stop()
{
	TCCR0B &= ~(1 << CS01);
	TCCR0B &= ~(1 << CS00);
}

void timers_clear()
{
	time = 0;
}

ISR(TIMER0_COMPA_vect)
{
	time++;
	if (time >= 359999999)
		time = 0;
}

ISR(TIMER1_OVF_vect) 
{
	pulse = 0;
}

ISR(TIMER1_CAPT_vect)
{
	pulse = ICR1;
	TCNT1 = 0;
}
