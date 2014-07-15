#include "global.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <avr/io.h>		// include I/O definitions (port names, pin names, etc)
#include <avr/interrupt.h>
#include <util/delay.h>

#include "uart/uart.h"

volatile uint16_t count = 0;
volatile uint16_t rpm = 0;

int main(void)
{
	uart_init();
	DDRD &= ~(1 << DDD2);
	EICRA |= (1 << ISC01);  // The falling edge of INT0 generates an interrupt request.
	EIMSK |= (1 << INT0);	// Turn on INT0
	
	sei();
	
	uart_puts_p(PSTR("Init done!\n"));
	
	while(1)
	{
		
	}
	return 0;
}

ISR(INT0_vect)
{
	uart_putc('.');
}