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

// define some flags for easier readability 
#define OUTMODE_CV 	0 
#define OUTMODE_CC 	1

// IO register buffers. Index 1 is current reading, index 0 is last reading
volatile uint8_t PINBBuffer[2] = {0xFF, 0xFF};
volatile uint8_t PINDBuffer[2] = {0xFF, 0xFF};

// variables for LCD
char displayBuffer[32] = {"00.00V 0000mA CV00.00V 0000mA   "};
uint8_t displayCurpos = 0;	// cursor position inside the display buffer

// voltage/current set points and actual measurements
// Adc = measured values
// Set = set values input by the user
// SetBuf = internal set buffer used for updating the PWM
uint16_t voltageSet, voltageSetBuf, voltageAdc = 0;
uint16_t currentSet, currentSetBuf, currentAdc = 0;

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
	stateCurrent = STATE_LCDUPDATE;
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
				// shift the display cursor right by one
				displayCurpos++;
				LCD_setpos(displayCurpos);
				stateNext = STATE_IDLE;
				break;
				
			case STATE_CLEFT:
				// shift the display cursor left by one
				displayCurpos--;
				LCD_setpos(displayCurpos);
				stateNext = STATE_IDLE;
				break;
				
			case STATE_ENCINC:
				// increase output voltage or current depending on cursor position
				switch(displayCurpos)
				{
					case 0: 
						voltageSet += 1000;
						break;
					
					case 1:
						voltageSet += 100;
						break;
						
					case 3:
						voltageSet += 10;
						break;
						
					case 4:
						voltageSet += 1;
						break;
						
					case 7:
						currentSet += 1000;
						break;
						
					case 8:
						currentSet += 100;
						break;
						
					case 9:
						currentSet += 10;
						break;
						
					case 10:
						currentSet += 1;
						break;
				}
				
				// check wether the output is enabled
				if (psuOutMode == 1)
				{
					// update internal set point buffers
					voltageSetBuf = voltageSet;
					currentSetBuf = currentSet;
					// update PWM
					stateNext= STATE_PWMUPDATE;
				}
				else
				{
					stateNext = STATE_IDLE;
				}
				break;
				
			case STATE_ENCDEC:
				// reduce output voltage or current depending on cursor position
				switch(displayCurpos)
				{
					case 0: 
						voltageSet -= 1000;
						break;
					
					case 1:
						voltageSet -= 100;
						break;
						
					case 3:
						voltageSet -= 10;
						break;
						
					case 4:
						voltageSet -= 1;
						break;
						
					case 7:
						currentSet -= 1000;
						break;
						
					case 8:
						currentSet -= 100;
						break;
						
					case 9:
						currentSet -= 10;
						break;
						
					case 10:
						currentSet -= 1;
						break;
				}
				
				// check wether the output is enabled
				if (psuOutMode == 1)
				{
					// update internal set point buffers
					voltageSetBuf = voltageSet;
					currentSetBuf = currentSet;
					// update PWM
					stateNext= STATE_PWMUPDATE;
				}
				else
				{
					stateNext = STATE_IDLE;
				}
				break;
				
			case STATE_OE:
				// toggle output enable status bit
				psuOutEnabled ^= 1; 
					
				// enable output	
				if (psuOutEnabled == 1)
				{
					// constant current mode
					if (psuOutMode == OUTMODE_CC)
					{
						voltageSetBuf = 2000;
					}
					// constant voltage with current limiting mode
					else
					{
						voltageSetBuf = voltageSet;
					}
					
					currentSetBuf = currentSet;
				}
				// disable output
				else 
				{
					voltageSetBuf = 0;
					currentSetBuf = 0;
				}
			
				stateNext = STATE_PWMUPDATE;
				break;
				
			case STATE_CV:
				// set internal output mode flag and disable output
				psuOutMode = OUTMODE_CV;
				stateNext = STATE_OE;
				break;
				
			case STATE_CC:
				// set internal output mode flag and disable output
				psuOutMode = OUTMODE_CC;
				stateNext = STATE_OE;
				break;
				
			case STATE_PWMUPDATE:
				PWM_setPSUOutV(voltageSetBuf);
				PWM_setPSUOutI(currentSetBuf);
				stateNext = STATE_LCDUPDATE;
				break;
				
			case STATE_LCDUPDATE:
				// measure actual output voltage and current
				voltageAdc = ADC_readPSUOutV();
				currentAdc = ADC_readPSUOutI();
				
				// format output voltage reading 
				displayBuffer[16] = (voltageAdc / 1000) + 48;
				displayBuffer[17] = ((voltageAdc % 1000) / 100) + 48;
				displayBuffer[19] = ((voltageAdc % 100) / 10) + 48;
				displayBuffer[20] = (voltageAdc % 10) + 48;
				
				// format output current reading
				displayBuffer[23] = (currentAdc / 1000) + 48;
				displayBuffer[24] = ((currentAdc % 1000) / 100) + 48;
				displayBuffer[25] = ((currentAdc % 100) / 10) + 48;
				displayBuffer[26] = (currentAdc % 10) + 48;
				
				// format output voltage setting
				displayBuffer[0] = (voltageSet / 1000) + 48;
				displayBuffer[1] = ((voltageSet % 1000) / 100) + 48;
				displayBuffer[3] = ((voltageSet % 100) / 10) + 48;
				displayBuffer[4] = (voltageSet % 10) + 48;
				
				// format output current setting
				displayBuffer[7] = (currentSet / 1000) + 48;
				displayBuffer[8] = ((currentSet % 1000) / 100) + 48;
				displayBuffer[9] = ((currentSet % 100) / 10) + 48;
				displayBuffer[10] = (currentSet % 10) + 48;
				
				// display "OE" marker if output is enabled
				if (psuOutEnabled == 1)
				{
					displayBuffer[30] = 'O';
					displayBuffer[31] = 'E';
				}
				else
				{
					displayBuffer[30] = ' ';
					displayBuffer[31] = ' ';
				}
				
				// indicate wether PSU is in CC or CV mode
				if (psuOutMode == OUTMODE_CC)
				{
					displayBuffer[15] = 'C';
				}
				else
				{
					displayBuffer[15] = 'V';
				}
				
				// push updated displayBuffer to LCD
				LCD_setpos(0);
				LCD_puts(displayBuffer);
				LCD_setpos(displayCurpos);
				
				// return to idle state
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
