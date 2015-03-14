#include <avr/io.h>
#include <avr/interrupt.h>

#include "lcd.h"
#include "adc.h"
#include "pwm.h"

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
#define ENC_B		0x10
#define BTN_MODE 	0x20
#define BTN_OE		0x40

// IO register buffers. Index 1 is current reading, index 0 is last reading
volatile uint8_t PINBBuffer[2] = {0xFF, 0xFF};
volatile uint8_t PINDBuffer[2] = {0xFF, 0xFF};

// variables for LCD
char displayBuffer[32] = {"00.00V 0000mA CV00.00V 0000mA   "};
uint8_t displayCurpos = 0;	// cursor position inside the display buffer

// voltage/current set points and actual measurements
uint16_t voltageSet, voltageAdc = 0;
uint16_t currentSet, currentAdc = 0;

// PSU output mode and status
uint8_t psuOutEnabled = 0;		// 0 = output disabled, 1 = output enabled
uint8_t psuOutMode = 0;			// 0 = constant voltage, 1 = constant current

// define possible states of the finite state machine
typedef enum
{
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
SM_STATE stateCurrent, stateNext;

int main(void)
{
	// low level initialization
	LCD_init(0x28, 0x0C);	
	ADC_init();
	PWM_init();
		
	// initialize FSM registers
	stateCurrent = STATE_IDLE;
	stateNext = STATE_IDLE;
	
	// set up SysTick timer
	// Timer 0
	TIMSK |= (1<<TOIE0);	// enable overflow interrupt
	TCCR0 |= (1<<CS01); // prescaler 8, interrupt freq: 16MHz/(256*8) = 7.8125kHz
	sei();
	
	while (1)
	{	
		switch (stateCurrent)
		{
			case STATE_IDLE:
				break;
				
			case STATE_CRIGHT:
				break;
				
			case STATE_CLEFT:
				break;
				
			case STATE_ENCINC:
				break;
				
			case STATE_ENCDEC:
				break;
				
			case STATE_OE:
				break;
				
			case STATE_CV:
				break;
				
			case STATE_CC:
				break;
				
			case STATE_PWMUPDATE:
				break;
				
			case STATE_LCDUPDATE:
				
				voltageAdc = ADC_readPSUOutV();
				currentAdc = ADC_readPSUOutI();
				
				LCD_setpos(0);
				LCD_puts(displayBuffer);
				
				stateNext = STATE_IDLE;
				break;
		}
		
		stateCurrent = stateNext;
		
	}
}

ISR(TIMER0_OVF_vect)
{
	// read IO input registers
	PINBBuffer[1] = PINB;
	PINDBuffer[1] = PIND;
	
	
	
	// save current IO input
	PINBBuffer[0] = PINBBuffer[1];
	PINDBuffer[0] = PINDBuffer[1];
}
