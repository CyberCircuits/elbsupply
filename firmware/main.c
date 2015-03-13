#include <avr/io.h>
#include <avr/interrupt.h>

#include "lcd.h"
#include "adc.h"

/* I/O mapping on the PCB
 * 
 * Inputs:
 * Button Left: 			PB6
 * Button right: 			PB7
 * Encoder button: 			PD2
 * Encoder A:				PD3
 * Encoder B:				PD4				
 * Button mode: 			PD5
 * Button output enable: 	PD6
 * 
 * output voltage:			ADC6
 * output current:			ADC7
 * 
 * Outputs:
 * Voltage set PWM:			PB1
 * Current set PWM:			PB2
 * LCD_RS:					PC0
 * LCD_E:					PC1
 * LCD_D4:					PC2
 * LCD_D5:					PC3
 * LCD_D6:					PC4
 * LCD_D7:					PC5
 */
#define BTN_LEFT	0x40
#define BTN_RIGHT	0x80
#define ENC_BTN		0x04
#define ENC_A		0x08
#define ENC_BTN		0x10
#define BTN_MODE 	0x20
#define BTN_OE		0x40

volatile uint8_t PINB_buffer;
volatile uint8_t PIND_buffer;

// define possible states of the finite state machine
typedef enum{
	STATE_IDLE,
	STATE_CRIGHT,
	STATE_CLEFT,
	STATE_ENCINC,
	STATE_ENCDEC,
	STATE_OE,
	STATE_CV,
	STATE_CC,
	STATE_PWMUPDATE,
	STATE_LCDUPDATE
}
SM_STATE;

// finite state machine registers
volatile SM_STATE state_current;
volatile SM_STATE state_next;

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
	
	state_current = STATE_IDLE;
	state_next = STATE_IDLE;
	
	// set up SysTick timer
	// Timer 0
	TIMSK |= (1<<TOIE0);	// enable overflow interrupt
	TCCR0 |= (1<<CS01); // prescaler 8, interrupt freq: 16MHz/(256*8) = 7.8125kHz
	sei();
	
	while (1)
	{

	}
}

ISR(TIMER0_OVF_vect){
	
}
