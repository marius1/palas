#include "global.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <avr/io.h>		// include I/O definitions (port names, pin names, etc)
#include <avr/interrupt.h>
#include <util/delay.h>

#include "uart/uart.h"

volatile uint16_t pulse = 0;

int main(void)
{
	uint16_t rpm = 0;

	uart_init();
	
	TCCR1B |= (1 << CS12) | (1 << CS10);	// prescaler 1024
	TIMSK1 |= (1 << ICIE1);
	
	sei();
	
	uart_puts_p(PSTR("Init done!\n"));
	
	while(1)
	{
		if (pulse > 0)
		{
			rpm = 937500L / pulse;
			uart_putw_dec(pulse);
			uart_putc(',');
			uart_putw_dec(rpm);
			uart_putc('\n');
			pulse = 0;
		}
	}
	return 0;
}

ISR(TIMER1_CAPT_vect)
{
	static uint16_t start;
	uint16_t t = ICR1;
	if ( t > start)
		pulse = t - start;
	else
		pulse = (0xFFFF ^ start) + t;
	start = t;
}
