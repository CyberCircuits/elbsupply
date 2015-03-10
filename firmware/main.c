#include <avr/io.h>
#include <avr/interrupt.h>

#include "lcd.h"
#include "adc.h"

int main(void)
{
	LCD_init(0x28, 0x0C);	
	
	// display boot message
	LCD_set_cursor(1,0);
	LCD_puts(" ElbSupply v0.1 ");
	LCD_set_cursor(2,0);
	LCD_puts(" 2015 devthrash ");
	
	ADC_init();
	
	while (1)
	{

	}
}
