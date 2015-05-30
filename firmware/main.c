#define F_CPU 8000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "lcd.h"
#include "adc.h"
#include "pwm.h"

/* I/O mapping on the PCB
 * 
 * Inputs:
 * Button Left: 			PD0
 * Button right: 			PD1
 * Encoder button: 			PD2
 * Encoder A:				PD3
 * Encoder B:				PD4				
 * Button output enable: 	PD5
 * Button mode: 			PD6
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
#define BTN_LEFT	0x01
#define BTN_RIGHT	0x02
#define ENC_BTN		0x04
#define ENC_A		0x08
#define ENC_B		0x10
#define BTN_OE 		0x20
#define BTN_MODE	0x40

// define some flags for easier readability 
#define OUTMODE_CV 	0 
#define OUTMODE_CC 	1

// cursor shift directions
#define CURSHIFT_LEFT 0
#define CURSHIFT_RIGHT 1

// variables for LCD
char displayBuffer[2][16] = {
	"00.00V 0000mA CV",
	"00.00V 0000mA   "
};

uint8_t displayCurpos = 0;	// cursor position inside the display buffer
volatile uint16_t adcResampleTimeout = 0;

// voltage/current set points and actual measurements
// Adc = measured values
// Set = set values input by the user
// SetBuf = internal set buffer used for updating the PWM
uint16_t voltageSet, voltageSetBuf, voltageAdc = 0;
uint16_t currentSet, currentSetBuf, currentAdc = 0;

// PSU output mode and status
// 0 = output disabled, 1 = output enabled
uint8_t psuOutEnabled = 0;		
// 0 = constant voltage, 1 = constant current
uint8_t psuOutMode = 0;			

/* This approach to switch debouncing has been borrowed from Jack Ganssle
 * See http://www.ganssle.com/debouncing-pt2.htm for detailed description
 */
// number of timer interrupts after which a switch is considered debounced
#define SW_CHECKS 30
uint8_t swDebouncedState; 						// debounced state of the switches
volatile uint8_t swStateBuf[SW_CHECKS] = {0}; 	// holds SW_CHECKS consecutive switch state readings
volatile uint8_t swStateIndex = 0;				// array index for number of checks performed

/* Reads the raw state of the port input registers and fuses them
 * together into one byte to be used in the debouncing routine
 * 
 * The byte layout is as follows:
 * Bit	6		5		 4		  3		    2 		1 			0
 * BTN_MODE | BTN_OE | ENC_B | ENC_A | ENC_BTN | BTN_RIGHT | BTN_LEFT
 */
uint8_t getSwitchRaw(void)
{
	uint8_t tmp;
	tmp = (PIND & 0x7F) ^ 0xFF;	// invert read values because switches are active low
	return tmp;
}

/* Returns debounced state of the switches 
 * Return bit values: 1 if switch has changed, 0 otherwise 
 */
uint8_t getSwitchDebounced(void)
{
	uint8_t i,j,tmp;
	j = 0xFF;
	for (i = 0; i < SW_CHECKS; i++) j = j & swStateBuf[i];
	tmp = swDebouncedState ^ j;
	swDebouncedState = j;
	return tmp;
}

/* variables for encoder decoding
 * Encoder outputs are connected to external interrupt pins on 
 * the AVR which generate interrupts on any level change.
 *
 * In the ISR patterns are generated which are compared to the
 * reference patterns in order to determine which way the encoder
 * was spun
 */
uint8_t encRefLeft[5] = {0, 2, 3, 1, 0};
uint8_t encRefRight[5] = {0, 1, 3, 2, 0};
volatile uint8_t encTurnBuf[5];
volatile uint8_t encTurnIndex = 0;

int8_t getEncoderTurn(void)
{
	uint8_t i,j;
	// check wether the pattern matches with being turned right
	j = 5;
	for (i = 0; i < 5; i++)
	{
		if (encTurnBuf[i] == encRefRight[i]) j--;
	}
	
	if (j == 0) return 1;
	// check wether the pattern matches with being turend left
	j = 5;
	for (i = 0; i < 5; i++)
	{
		if (encTurnBuf[i] == encRefLeft[i]) j--;
	}
	
	if (j == 0) return -1;
	
	return 0;
}

/* shifts the cursor display position left or right depending on inc
 * if inc = 1: shift right
 * if inc = 0: shift left
 * this function ensures that the cursor is only moved to a position
 * of one of the digits of the set points on the LCD (cursor doesn't
 * move to digits that can't be modified)
 */
void lcdShiftCursor(uint8_t inc)
{
	switch (displayCurpos)
	{
	case 0:
		if (inc) displayCurpos++; 
		break;
		
	case 1:
		if (inc) displayCurpos += 2; 
		else displayCurpos--;
		break;
		
	case 3:
		if (inc) displayCurpos++;
		else displayCurpos -= 2;
		break;
		
	case 4: 
		if (inc) displayCurpos += 3;
		else displayCurpos--;
		break;
	
	case 7:
		if (inc) displayCurpos++;
		else displayCurpos -= 3;
		break;
	
	case 8:
		if (inc) displayCurpos++;
		else displayCurpos--;
		break;
				
	case 9:
		if (inc) displayCurpos++;
		else displayCurpos--;
		break;
						
	case 10:
		if (!inc) displayCurpos--;
		break;
	}
}

// define possible states of the finite state machine
typedef enum{
	STATE_IDLE,
	STATE_CRIGHT,
	STATE_CLEFT,
	STATE_ENCINC,
	STATE_ENCDEC,
	STATE_OE,
	STATE_MODE,
	STATE_PWMUPDATE,
	STATE_LCDUPDATE
}
SM_STATE;

// finite state machine registers
SM_STATE stateNext;

// each state of the FSM has it's own function
// return value is the next state of the FSM
SM_STATE stateIdle(void)
{
	uint8_t tmp;
	int8_t encState;
	
	tmp = getSwitchDebounced();
	encState = getEncoderTurn();
	
	if ( (swDebouncedState & BTN_OE) && (tmp & BTN_OE) ) {
		return STATE_OE;
	} else if ( (swDebouncedState & BTN_MODE) && (tmp & BTN_MODE)) {
		return STATE_MODE;
	} else if ( (swDebouncedState & BTN_LEFT) && (tmp & BTN_LEFT) ) {
		return STATE_CLEFT;
	} else if ( (swDebouncedState & BTN_RIGHT) && (tmp & BTN_RIGHT) ) {
		return STATE_CRIGHT;
	} else if ( encState == 1 ) {
		return STATE_ENCINC;
	} else if ( encState == -1 ) {
		return STATE_ENCDEC;
	} else if (adcResampleTimeout >= 244) {
		adcResampleTimeout = 0;
		return STATE_LCDUPDATE;
	} else {
		return STATE_IDLE;
	}
}

SM_STATE stateCRight(void)
{
	// shift the display cursor right by one
	lcdShiftCursor(CURSHIFT_RIGHT);
	LCD_setpos(displayCurpos);
	return STATE_IDLE;
}

SM_STATE stateCLeft(void)
{
	// shift the display cursor left by one
	lcdShiftCursor(CURSHIFT_LEFT);
	LCD_setpos(displayCurpos);
	return STATE_IDLE;
}

SM_STATE stateEncInc(void)
{
	// increase output voltage or current depending on cursor position
	switch (displayCurpos){
	case 0:
		if (voltageSet < 501) voltageSet += 1000;
		break;
					
	case 1:
		if (voltageSet < 1401) voltageSet += 100;
		break;
						
	case 3:
		if (voltageSet < 1491) voltageSet += 10;
		break;
						
	case 4:
		if (voltageSet < 1500) voltageSet += 1;
		break;
						
	case 7:
		if (currentSet < 2001) currentSet += 1000;
		break;
						
	case 8:
		if (currentSet < 2901) currentSet += 100;
		break;
						
	case 9:
		if (currentSet < 2991) currentSet += 10;
		break;
						
	case 10:
		if (currentSet < 2999) currentSet += 1;
		break;
	}
				
	// check wether the output is enabled
	if (psuOutEnabled == 1){
		// update internal set point buffers
		voltageSetBuf = voltageSet;
		currentSetBuf = currentSet;
		// update PWM
		return STATE_PWMUPDATE;
	} else {
		return STATE_LCDUPDATE;
	}
}

SM_STATE stateEncDec(void)
{
	// reduce output voltage or current depending on cursor position
	switch (displayCurpos){
	case 0: 
		if (voltageSet > 999) voltageSet -= 1000;
		break;
					
	case 1:
		if (voltageSet > 99) voltageSet -= 100;
		break;
						
	case 3:
		if (voltageSet > 9) voltageSet -= 10;
		break;
						
	case 4:
		if (voltageSet > 0) voltageSet -= 1;
		break;
						
	case 7:
		if (currentSet > 999) currentSet -= 1000;
		break;
						
	case 8:
		if (currentSet > 99) currentSet -= 100;
		break;
						
	case 9:
		if (currentSet > 9) currentSet -= 10;
		break;
					
	case 10:
		if (currentSet > 0) currentSet -= 1;
		break;
	}
	
	// check wether the output is enabled
	if (psuOutEnabled == 1){
		// update internal set point buffers
		voltageSetBuf = voltageSet;
		currentSetBuf = currentSet;
		// update PWM
		return STATE_PWMUPDATE;
	} else {
		return STATE_LCDUPDATE;
	}	
}

SM_STATE stateOE(void)
{
	// toggle output enable status bit
	psuOutEnabled ^= 1; 
					
	// enable output	
	if (psuOutEnabled == 1){
		// constant current mode
		if (psuOutMode == OUTMODE_CC){
			voltageSetBuf = 1500;
		} else { 						// constant voltage with current limiting mode
			voltageSetBuf = voltageSet;
		}
					
		currentSetBuf = currentSet;
	} else { 							// disable output
		voltageSetBuf = 0;
		currentSetBuf = 0;
	}
	
	return STATE_PWMUPDATE;
}

SM_STATE stateMode(void)
{
	// toggle internal output mode flag and disable output
	psuOutMode ^= 1;
	return STATE_OE;
}

SM_STATE statePWMUpdate(void)
{
	PWM_setPSUOutV(voltageSetBuf);
	PWM_setPSUOutI(currentSetBuf);
	return STATE_LCDUPDATE;
}

SM_STATE stateLCDUpdate(void)
{
	// measure actual output voltage and current
	voltageAdc = ADC_readPSUOutV();
	currentAdc = ADC_readPSUOutI();
	
	// format output voltage reading 
	displayBuffer[1][0] = (voltageAdc / 1000) + 48;
	displayBuffer[1][1] = ((voltageAdc % 1000) / 100) + 48;
	displayBuffer[1][3] = ((voltageAdc % 100) / 10) + 48;
	displayBuffer[1][4] = (voltageAdc % 10) + 48;
				
	// format output current reading
	displayBuffer[1][7] = (currentAdc / 1000) + 48;
	displayBuffer[1][8] = ((currentAdc % 1000) / 100) + 48;
	displayBuffer[1][9] = ((currentAdc % 100) / 10) + 48;
	displayBuffer[1][10] = (currentAdc % 10) + 48;
				
	// format output voltage setting
	displayBuffer[0][0] = (voltageSet / 1000) + 48;
	displayBuffer[0][1] = ((voltageSet % 1000) / 100) + 48;
	displayBuffer[0][3] = ((voltageSet % 100) / 10) + 48;
	displayBuffer[0][4] = (voltageSet % 10) + 48;
				
	// format output current setting
	displayBuffer[0][7] = (currentSet / 1000) + 48;
	displayBuffer[0][8] = ((currentSet % 1000) / 100) + 48;
	displayBuffer[0][9] = ((currentSet % 100) / 10) + 48;
	displayBuffer[0][10] = (currentSet % 10) + 48;
	
	// display "OE" marker if output is enabled
	if (psuOutEnabled == 1){
		displayBuffer[1][14] = 'O';
		displayBuffer[1][15] = 'E';
	} else {
		displayBuffer[1][14] = ' ';
		displayBuffer[1][15] = ' ';
	}
				
	// indicate wether PSU is in CC or CV mode
	if (psuOutMode == OUTMODE_CC)
	{
		displayBuffer[0][15] = 'C';
	} else {
		displayBuffer[0][15] = 'V';
	}
	
	cli();		
	// push updated displayBuffer to LCD
	LCD_set_cursor(0, 0);
	LCD_puts(displayBuffer[0]);
	LCD_set_cursor(1, 0);
	LCD_puts(displayBuffer[1]);
	LCD_setpos(displayCurpos);
	sei();
	
	return STATE_IDLE;
}

int main(void)
{
	// low level initialization
	LCD_init(0x28, 0x0E);	
	ADC_init();
	PWM_init();
	
	// input switches
	DDRB &= ~0xC0;
	DDRD &= ~0x7F;
		
	// enable pull up resistors
	PORTD |= 0x7F;

	voltageSet = 500;
	currentSet = 3000;
	
	// initialize FSM registers
	stateNext = STATE_LCDUPDATE;

	// configure external interrupts for encoder
	// any logical change in the INT0/1 pins causes interrupt
	MCUCR |= (1<<ISC10) | (1<<ISC00);
	GICR |= (1<<INT1) | (1<<INT0);

	// set up SysTick timer
	// Timer 0
	TIMSK |= (1<<TOIE0);	// enable overflow interrupt
	TCCR0 |= (1<<CS01) | (1<<CS00); // prescaler 64, interrupt freq: 8MHz/(256*64) = 488Hz
	sei();
	
	// simply execute the FSM
	while (1){
		
		switch (stateNext){
		case STATE_IDLE:
			stateNext = stateIdle();
			break;
				
		case STATE_CRIGHT:
			stateNext = stateCRight();
			break;
				
		case STATE_CLEFT:
			stateNext = stateCLeft();
			break;
				
		case STATE_ENCINC:
			stateNext = stateEncInc();
			break;
				
		case STATE_ENCDEC:
			stateNext = stateEncDec();
			break;
				
		case STATE_OE:
			stateNext = stateOE();
			break;
				
		case STATE_MODE:
			stateNext = stateMode();	
			break;
				
		case STATE_PWMUPDATE:
			stateNext = statePWMUpdate();
			break;
				
		case STATE_LCDUPDATE:
			stateNext = stateLCDUpdate();
			break;
		}
		
	}
}

ISR(TIMER0_OVF_vect)
{
	// acumulate SW_CHECKS number of samples of the switch states
	swStateBuf[swStateIndex] = getSwitchRaw();
	++swStateIndex;
	if (swStateIndex >= SW_CHECKS) swStateIndex = 0;
			
	// timeout counter for next ADC read/display update
	adcResampleTimeout++;
}

ISR(INT0_vect)
{
	if (PIND & 0x08)
	{
		encTurnBuf[encTurnIndex] = 2;
	} else {
		encTurnBuf[encTurnIndex] = 1;
	}
	++encTurnIndex;
	if (encTurnIndex > 4) encTurnIndex = 0;
}

ISR(INT1_vect){
	if (PIND & 0x04)
	{
		encTurnBuf[encTurnIndex] = 3;
	} else {
		encTurnBuf[encTurnIndex] = 0;
	}
	++encTurnIndex;
	if (encTurnIndex > 4) encTurnIndex = 0;
}

