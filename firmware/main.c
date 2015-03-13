#include <avr/io.h>
#include <avr/interrupt.h>

#include "lcd.h"
#include "adc.h"

void PWM_init(void)
{	
	// configure Timer 1
	ICR1 = 19999;	// uper limit of Timer 1 count, yields resolution of 1 mV
	OCR1A = 0;
	OCR1B = 0;
	TCCR1A = 0xA2; // fast non-inverting PWM mode
	TCCR1B = 0x19; // prescaler 1
	// configure I/O
	DDRB |= (1<<PB2) | (1<<PB1);
}

/* sets the output voltage of the PSU
 * takes argument in mV */
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

int main(void)
{
	LCD_init(0x28, 0x0C);	
	
	// display boot message
	LCD_set_cursor(1,0);
	LCD_puts(" ElbSupply v0.1 ");
	LCD_set_cursor(2,0);
	LCD_puts(" 2015 devthrash ");
	
	ADC_init();
	PWM_init();
	
	// set up SysTick timer
	// Timer 0
	
	while (1)
	{

	}
}
