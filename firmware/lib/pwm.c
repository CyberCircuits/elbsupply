#include <avr/io.h>
#include "pwm.h"

void PWM_init(void)
{	
	// configure Timer 1
	ICR1 = 14999;	// uper limit of Timer 1 count, yields resolution of 1 mV
	OCR1A = 0;
	OCR1B = 0;
	TCCR1A = 0xA2; // fast non-inverting PWM mode
	TCCR1B = 0x19; // prescaler 1
	// configure I/O
	DDRB |= (1<<PB2) | (1<<PB1);
}

/* sets the output voltage of the PSU
 * takes argument in 10 mV */
void PWM_setPSUOutV(uint16_t voltage)
{
	OCR1A = voltage * 10;
}

/* sets the output current limit of the PSU
 * takes argument in mA */
void PWM_setPSUOutI(uint16_t current)
{
	OCR1B = (current<<2);
}
